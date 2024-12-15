/**
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2024 Bardia Moshiri <fakeshell@bardia.tech>
 *
 * This program daemon uses the "connector" subsystem (NETLINK_CONNECTOR) to receive
 * PROC_EVENT_FORK messages from the kernel every time a new process is created via a netlink socket.
 * So it needs CONFIG_CONNECTOR and CONFIG_PROC_EVENTS (or modules "cn" and "cn_proc")
 */

#include <stdio.h>
#include <gio/gio.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <linux/netlink.h>
#include <linux/cn_proc.h>
#include <linux/connector.h>

#define CONFIG_FILE "/etc/nicerdicer.conf"
#define MAX_LINE_LENGTH 256
#define MAX_PROGRAMS 100

typedef struct {
    gchar programs[MAX_PROGRAMS][MAX_LINE_LENGTH];
    gint niceness[MAX_PROGRAMS];
    gint count;
} ProgramData;

static gboolean
is_number (const gchar *str)
{
    while (*str) {
        if (!g_ascii_isdigit (*str))
            return FALSE;
        str++;
    }
    return TRUE;
}

static gboolean
get_program_name (pid_t pid, gchar *program_name, size_t program_name_size)
{
    gchar path[64];
    g_snprintf (path, sizeof (path), "/proc/%d/comm", pid);
    FILE *file = fopen (path, "r");
    if (file) {
        if (fgets (program_name, program_name_size, file) != NULL) {
            size_t len = strlen (program_name);
            if (len > 0 && program_name[len - 1] == '\n')
                program_name[len - 1] = '\0';
            fclose (file);
            return TRUE;
        }
        fclose (file);
    }
    return FALSE;
}

static void
set_process_niceness (pid_t pid, gint niceness, const gchar *program_name)
{
    if (setpriority (PRIO_PROCESS, pid, niceness) == -1) {
        perror ("setpriority");
    } else {
        g_print ("Set niceness of program '%s' with PID %d to %d\n", program_name, pid, niceness);
        gchar path[PATH_MAX];
        g_snprintf (path, sizeof (path), "/proc/%d/task", pid);
        DIR *task_dir = opendir (path);
        if (task_dir) {
            struct dirent *entry;
            while ((entry = readdir (task_dir)) != NULL) {
                if (entry->d_type != DT_DIR || !is_number (entry->d_name))
                    continue;
                pid_t tid = atoi (entry->d_name);
                if (setpriority (PRIO_PROCESS, tid, niceness) == -1)
                    perror ("setpriority");
                else
                    g_print ("Set niceness of thread %d of program '%s' with PID %d to %d\n",
                             tid, program_name, pid, niceness);
            }
            closedir (task_dir);
        }
    }
}

static void
read_config (ProgramData *data)
{
    FILE *file = fopen (CONFIG_FILE, "r");
    if (!file) {
        perror ("fopen");
        return;
    }

    gchar line[MAX_LINE_LENGTH];
    while (fgets (line, sizeof (line), file) && data->count < MAX_PROGRAMS) {
        if (sscanf (line, "%[^,],%d", data->programs[data->count], &data->niceness[data->count]) == 2)
            data->count++;
    }

    fclose (file);
}

static void
write_config (const ProgramData *data)
{
    FILE *file = fopen (CONFIG_FILE, "w");
    if (!file) {
        perror ("fopen");
        return;
    }

    for (gint i = 0; i < data->count; i++) {
        fprintf (file, "%s,%d\n", data->programs[i], data->niceness[i]);
    }

    fclose (file);
}

static gboolean
add_or_update_program (ProgramData *data, const gchar *program_name, gint prio)
{
    prio = CLAMP (prio, -20, 19);

    for (gint i = 0; i < data->count; i++) {
        if (g_strcmp0 (data->programs[i], program_name) == 0) {
            if (data->niceness[i] != prio) {
                data->niceness[i] = prio;
                write_config (data);
                return TRUE;
            }
            return FALSE;
        }
    }

    if (data->count < MAX_PROGRAMS) {
        g_strlcpy (data->programs[data->count], program_name, MAX_LINE_LENGTH);
        data->niceness[data->count] = prio;
        data->count++;
        write_config (data);
        return TRUE;
    }

    return FALSE;
}

static int
set_proc_ev_listen (int nl_sock, int enable)
{
    struct {
        struct nlmsghdr nl_hdr;
        struct cn_msg cn_hdr;
        enum proc_cn_mcast_op mcast_op;
    } __attribute__ ((packed)) msg;

    memset (&msg, 0, sizeof (msg));
    msg.nl_hdr.nlmsg_len = sizeof (msg);
    msg.nl_hdr.nlmsg_type = NLMSG_DONE;
    msg.nl_hdr.nlmsg_flags = 0;
    msg.nl_hdr.nlmsg_seq = 0;
    msg.nl_hdr.nlmsg_pid = getpid ();

    msg.cn_hdr.id.idx = CN_IDX_PROC;
    msg.cn_hdr.id.val = CN_VAL_PROC;
    msg.cn_hdr.len = sizeof (enum proc_cn_mcast_op);

    msg.mcast_op = enable ? PROC_CN_MCAST_LISTEN : PROC_CN_MCAST_IGNORE;

    int ret = send (nl_sock, &msg, sizeof (msg), 0);
    if (ret == -1) {
        perror ("send");
        return -1;
    }
    return 0;
}

static void
handle_process_fork (ProgramData *data, pid_t child_pid)
{
    gchar pname[256];
    if (!get_program_name (child_pid, pname, sizeof (pname)))
        return;

    for (gint i = 0; i < data->count; i++) {
        if (g_strcmp0 (data->programs[i], pname) == 0) {
            set_process_niceness (child_pid, data->niceness[i], pname);
            break;
        }
    }
}

static gboolean
netlink_event_callback (GIOChannel *source, GIOCondition condition, gpointer user_data)
{
    ProgramData *data = (ProgramData *) user_data;

    if (condition & (G_IO_ERR | G_IO_HUP | G_IO_NVAL)) {
        g_warning ("Netlink socket error or hangup");
        return FALSE;
    }

    int nl_sock = g_io_channel_unix_get_fd (source);
    char buf[4096];
    ssize_t len = recv (nl_sock, buf, sizeof (buf), 0);
    if (len < 0) {
        if (errno != EINTR)
            perror ("recv");
        return TRUE;
    }

    struct nlmsghdr *nlh = (struct nlmsghdr *) buf;
    for (; NLMSG_OK (nlh, (unsigned) len); nlh = NLMSG_NEXT (nlh, len)) {
        if (nlh->nlmsg_type == NLMSG_NOOP) {
            continue;
        }
        if (nlh->nlmsg_type == NLMSG_ERROR) {
            fprintf (stderr, "Received error message from netlink\n");
            continue;
        }
        if (nlh->nlmsg_type == NLMSG_DONE) {
            struct cn_msg *cn_hdr = NLMSG_DATA (nlh);
            if (cn_hdr->id.idx == CN_IDX_PROC && cn_hdr->id.val == CN_VAL_PROC) {
                struct proc_event *ev = (struct proc_event *) cn_hdr->data;
                if (ev->what == PROC_EVENT_FORK) {
                    pid_t child_pid = ev->event_data.fork.child_pid;
                    handle_process_fork (data, child_pid);
                }
            }
        }
    }

    return TRUE;
}

static void
initial_process_set (ProgramData *data)
{
    DIR *proc_dir = opendir ("/proc");
    if (!proc_dir) {
        perror ("opendir");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir (proc_dir)) != NULL) {
        if (entry->d_type != DT_DIR || !is_number (entry->d_name))
            continue;

        pid_t pid = atoi (entry->d_name);
        gchar pname[256];
        if (get_program_name (pid, pname, sizeof (pname))) {
            for (gint i = 0; i < data->count; i++) {
                if (g_strcmp0 (data->programs[i], pname) == 0) {
                    set_process_niceness (pid, data->niceness[i], pname);
                    break;
                }
            }
        }
    }
    closedir (proc_dir);
}

static const gchar introspection_xml[] =
    "<node>"
    "  <interface name='io.FuriOS.NicerDicer'>"
    "    <method name='AddProgram'>"
    "      <arg type='s' name='program_name' direction='in'/>"
    "      <arg type='i' name='priority' direction='in'/>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    "  </interface>"
    "</node>";

static void
handle_method_call (GDBusConnection       *connection,
                    const gchar           *sender,
                    const gchar           *object_path,
                    const gchar           *interface_name,
                    const gchar           *method_name,
                    GVariant              *parameters,
                    GDBusMethodInvocation *invocation,
                    gpointer               user_data)
{
    ProgramData *data = (ProgramData *) user_data;

    if (g_strcmp0 (method_name, "AddProgram") == 0) {
        const gchar *program_name;
        gint priority;
        g_variant_get (parameters, "(&si)", &program_name, &priority);

        gboolean success = add_or_update_program (data, program_name, priority);
        g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", success));
    }
}

static void
on_bus_acquired (GDBusConnection *connection,
                 const gchar     *name,
                 gpointer         user_data)
{
    static GDBusInterfaceVTable interface_vtable = {
        handle_method_call,
        NULL,
        NULL,
        { 0 }
    };

    GDBusNodeInfo *introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);

    g_dbus_connection_register_object (connection,
                                       "/io/FuriOS/NicerDicer",
                                       introspection_data->interfaces[0],
                                       &interface_vtable,
                                       user_data,
                                       NULL,
                                       NULL);

    g_dbus_node_info_unref (introspection_data);
}

static void
on_name_acquired (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
    g_print ("Acquired the name %s on the system bus\n", name);
}

static void
on_name_lost (GDBusConnection *connection,
              const gchar     *name,
              gpointer         user_data)
{
    g_print ("Lost the name %s on the system bus\n", name);
}

int
main (int   argc,
      char *argv[])
{
    ProgramData data = {0};
    read_config (&data);

    initial_process_set (&data);

    int nl_sock = socket (PF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);
    if (nl_sock < 0) {
        perror ("socket");
        return EXIT_FAILURE;
    }

    struct sockaddr_nl addr;
    memset (&addr, 0, sizeof (addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid ();
    addr.nl_groups = CN_IDX_PROC;

    if (bind (nl_sock, (struct sockaddr *) &addr, sizeof (addr)) < 0) {
        perror ("bind");
        close (nl_sock);
        return EXIT_FAILURE;
    }

    if (set_proc_ev_listen (nl_sock, 1) < 0) {
        close (nl_sock);
        return EXIT_FAILURE;
    }

    GMainLoop *loop = g_main_loop_new (NULL, FALSE);
    GIOChannel *nl_channel = g_io_channel_unix_new (nl_sock);
    g_io_channel_set_encoding (nl_channel, NULL, NULL);
    g_io_channel_set_flags (nl_channel, G_IO_FLAG_NONBLOCK, NULL);
    g_io_add_watch (nl_channel, G_IO_IN | G_IO_HUP | G_IO_ERR, netlink_event_callback, &data);

    guint owner_id = g_bus_own_name (G_BUS_TYPE_SYSTEM,
                                     "io.FuriOS.NicerDicer",
                                     G_BUS_NAME_OWNER_FLAGS_NONE,
                                     on_bus_acquired,
                                     on_name_acquired,
                                     on_name_lost,
                                     &data,
                                     NULL);

    g_main_loop_run (loop);

    g_bus_unown_name (owner_id);
    g_main_loop_unref (loop);

    set_proc_ev_listen (nl_sock, 0);
    close (nl_sock);

    return 0;
}
