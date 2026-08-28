/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2026 Bardia Moshiri <bardia@furilabs.com>
 */

#include "nice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <linux/netlink.h>
#include <linux/cn_proc.h>
#include <linux/connector.h>

#define CONFIG_FILE "/var/lib/batman/nice.conf"

#define MAX_LINE_LENGTH 256
#define MAX_PROGRAMS 100

typedef struct {
    gchar programs[MAX_PROGRAMS][MAX_LINE_LENGTH];
    gint niceness[MAX_PROGRAMS];
    gint count;
} ProgramData;

struct NiceContext {
    ProgramData programs;

    int nl_sock;
    GIOChannel *nl_channel;
    guint nl_watch_id;

    gboolean enabled;
};

static gboolean
is_number(const gchar *str)
{
    if (!str || *str == '\0')
        return FALSE;

    while (*str) {
        if (!g_ascii_isdigit(*str))
            return FALSE;

        str++;
    }

    return TRUE;
}

static gboolean
get_program_name(pid_t pid,
                 gchar *program_name,
                 size_t program_name_size)
{
    if (!program_name || program_name_size == 0)
        return FALSE;

    gchar path[64];
    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);

    FILE *file = fopen(path, "r");
    if (!file)
        return FALSE;

    gchar cmdline[PATH_MAX];
    size_t len = fread(cmdline, 1, sizeof(cmdline) - 1, file);

    fclose(file);

    if (len == 0)
        return FALSE;

    cmdline[len] = '\0';

    /*
     * /proc/<pid>/cmdline is NUL-separated.
     * The first entry is argv[0].
     */
    const gchar *argv0 = cmdline;

    if (argv0[0] == '\0')
        return FALSE;

    const gchar *basename = strrchr(argv0, '/');
    if (basename)
        basename++;
    else
        basename = argv0;

    if (basename[0] == '\0')
        return FALSE;

    g_strlcpy(program_name, basename, program_name_size);

    return TRUE;
}

static void
set_process_niceness(pid_t pid,
                     gint niceness,
                     const gchar *program_name)
{
    if (setpriority(PRIO_PROCESS, pid, niceness) == -1) {
        g_warning("nice: failed to set niceness of %s (%d) to %d: %s",
                  program_name ? program_name : "",
                  pid,
                  niceness,
                  g_strerror(errno));
        return;
    }

    g_debug("nice: set %s (%d) niceness to %d",
            program_name ? program_name : "",
            pid,
            niceness);

    gchar path[PATH_MAX];
    snprintf(path, sizeof(path), "/proc/%d/task", pid);

    DIR *task_dir = opendir(path);
    if (!task_dir)
        return;

    struct dirent *entry;

    while ((entry = readdir(task_dir)) != NULL) {
        if (entry->d_type != DT_DIR && entry->d_type != DT_UNKNOWN)
            continue;

        if (!is_number(entry->d_name))
            continue;

        pid_t tid = (pid_t)atoi(entry->d_name);

        if (setpriority(PRIO_PROCESS, tid, niceness) == -1) {
            if (errno != ESRCH)
                g_warning("nice: failed to set thread %d of %s (%d) to %d: %s",
                          tid,
                          program_name ? program_name : "",
                          pid,
                          niceness,
                          g_strerror(errno));

            continue;
        }

        g_debug("nice: set thread %d of %s (%d) niceness to %d",
                tid,
                program_name ? program_name : "",
                pid,
                niceness);
    }

    closedir(task_dir);
}

static void
read_config(ProgramData *data)
{
    if (!data)
        return;

    memset(data, 0, sizeof(*data));

    FILE *file = fopen(CONFIG_FILE, "r");
    if (!file) {
        g_warning("nice: failed to open %s: %s",
                  CONFIG_FILE,
                  g_strerror(errno));
        return;
    }

    gchar line[MAX_LINE_LENGTH];

    while (fgets(line, sizeof(line), file) &&
           data->count < MAX_PROGRAMS) {
        gchar program[MAX_LINE_LENGTH];
        gint niceness;

        if (sscanf(line, " %255[^,],%d", program, &niceness) != 2)
            continue;

        g_strstrip(program);

        if (program[0] == '\0')
            continue;

        niceness = CLAMP(niceness, -20, 19);

        g_strlcpy(data->programs[data->count],
                  program,
                  sizeof(data->programs[data->count]));

        data->niceness[data->count] = niceness;
        data->count++;
    }

    fclose(file);

    g_debug("nice: loaded %d entries", data->count);
}

static gboolean
find_program_niceness(const ProgramData *data,
                      const gchar *program_name,
                      gint *niceness)
{
    if (!data || !program_name)
        return FALSE;

    for (gint i = 0; i < data->count; i++) {
        if (g_strcmp0(data->programs[i], program_name) != 0)
            continue;

        if (niceness)
            *niceness = data->niceness[i];

        return TRUE;
    }

    return FALSE;
}

static int
set_proc_ev_listen(int nl_sock, gboolean enabled)
{
    struct {
        struct nlmsghdr nl_hdr;
        struct cn_msg cn_hdr;
        enum proc_cn_mcast_op mcast_op;
    } __attribute__((packed)) msg;

    memset(&msg, 0, sizeof(msg));

    msg.nl_hdr.nlmsg_len = sizeof(msg);
    msg.nl_hdr.nlmsg_type = NLMSG_DONE;
    msg.nl_hdr.nlmsg_pid = getpid();

    msg.cn_hdr.id.idx = CN_IDX_PROC;
    msg.cn_hdr.id.val = CN_VAL_PROC;
    msg.cn_hdr.len = sizeof(enum proc_cn_mcast_op);

    msg.mcast_op = enabled ? PROC_CN_MCAST_LISTEN :
                             PROC_CN_MCAST_IGNORE;

    if (send(nl_sock, &msg, sizeof(msg), 0) == -1) {
        g_warning("nice: failed to %s process events: %s",
                  enabled ? "enable" : "disable",
                  g_strerror(errno));
        return -1;
    }

    return 0;
}

static void
handle_process_exec(NiceContext *nice, pid_t pid)
{
    if (!nice || !nice->enabled)
        return;

    gchar program_name[MAX_LINE_LENGTH];

    if (!get_program_name(pid,
                          program_name,
                          sizeof(program_name)))
        return;

    gint niceness;

    if (!find_program_niceness(&nice->programs,
                               program_name,
                               &niceness))
        return;

    set_process_niceness(pid, niceness, program_name);
}

static void
handle_thread_fork(NiceContext *nice,
                   pid_t tid,
                   pid_t tgid)
{
    if (!nice || !nice->enabled)
        return;

    if (tid == tgid)
        return;

    gchar program_name[MAX_LINE_LENGTH];

    if (!get_program_name(tgid,
                          program_name,
                          sizeof(program_name)))
        return;

    gint niceness;

    if (!find_program_niceness(&nice->programs,
                               program_name,
                               &niceness))
        return;

    if (setpriority(PRIO_PROCESS, tid, niceness) == -1) {
        if (errno != ESRCH)
            g_warning("nice: failed to set thread %d of %s (%d) to %d: %s",
                      tid,
                      program_name,
                      tgid,
                      niceness,
                      g_strerror(errno));

        return;
    }

    g_debug("nice: set thread %d of %s (%d) niceness to %d",
            tid,
            program_name,
            tgid,
            niceness);
}

static gboolean
netlink_event_callback(GIOChannel *source,
                       GIOCondition condition,
                       gpointer userdata)
{
    NiceContext *nice = userdata;

    if (!nice)
        return G_SOURCE_REMOVE;

    if (!nice->enabled)
        return G_SOURCE_REMOVE;

    int nl_sock = g_io_channel_unix_get_fd(source);

    if (condition & G_IO_NVAL) {
        g_warning("nice: netlink socket invalid");

        nice->nl_watch_id = 0;
        nice->enabled = FALSE;

        return G_SOURCE_REMOVE;
    }

    if (condition & G_IO_HUP) {
        g_warning("nice: netlink socket hangup");

        nice->nl_watch_id = 0;
        nice->enabled = FALSE;

        return G_SOURCE_REMOVE;
    }

    if (condition & G_IO_ERR) {
        int error = 0;
        socklen_t error_len = sizeof(error);

        if (getsockopt(nl_sock,
                       SOL_SOCKET,
                       SO_ERROR,
                       &error,
                       &error_len) == 0 &&
            error != 0) {
            g_warning("nice: netlink socket error: %s",
                      g_strerror(error));
        } else {
            g_warning("nice: netlink socket error");
        }
    }

    char buf[4096];

    for (;;) {
        ssize_t len = recv(nl_sock, buf, sizeof(buf), MSG_DONTWAIT);

        if (len < 0) {
            if (errno == EINTR)
                continue;

            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;

            if (errno == ENOBUFS) {
                g_warning("nice: netlink receive buffer overflow");
                break;
            }

            g_warning("nice: netlink recv failed: %s", g_strerror(errno));
            break;
        }

        if (len == 0)
            break;

        struct nlmsghdr *nlh = (struct nlmsghdr *)buf;
        int remaining = (int)len;

        while (NLMSG_OK(nlh, remaining)) {
            if (nlh->nlmsg_type == NLMSG_DONE) {
                struct cn_msg *cn_hdr = NLMSG_DATA(nlh);

                if (cn_hdr->id.idx == CN_IDX_PROC &&
                    cn_hdr->id.val == CN_VAL_PROC) {
                    struct proc_event *event = (struct proc_event *)cn_hdr->data;

                    if (event->what == PROC_EVENT_EXEC) {
                        pid_t pid = event->event_data.exec.process_tgid;
                        handle_process_exec(nice, pid);
                    } else if (event->what == PROC_EVENT_FORK) {
                        pid_t tid = event->event_data.fork.child_pid;
                        pid_t tgid = event->event_data.fork.child_tgid;
                        handle_thread_fork(nice, tid, tgid);
                    }
                }
            }

            nlh = NLMSG_NEXT(nlh, remaining);
        }
    }

    return G_SOURCE_CONTINUE;
}

static void
initial_process_set(NiceContext *nice)
{
    if (!nice)
        return;

    DIR *proc_dir = opendir("/proc");
    if (!proc_dir) {
        g_warning("nice: failed to open /proc: %s",
                  g_strerror(errno));
        return;
    }

    struct dirent *entry;

    while ((entry = readdir(proc_dir)) != NULL) {
        if (entry->d_type != DT_DIR && entry->d_type != DT_UNKNOWN)
            continue;

        if (!is_number(entry->d_name))
            continue;

        pid_t pid = (pid_t)atoi(entry->d_name);

        gchar program_name[MAX_LINE_LENGTH];

        if (!get_program_name(pid,
                              program_name,
                              sizeof(program_name)))
            continue;

        gint niceness;

        if (!find_program_niceness(&nice->programs,
                                   program_name,
                                   &niceness))
            continue;

        set_process_niceness(pid, niceness, program_name);
    }

    closedir(proc_dir);
}

/*
 * This listener uses the "connector" subsystem (NETLINK_CONNECTOR) to receive
 * PROC_EVENT_FORK and PROC_EVENT_EXEC messages from the kernel via a netlink socket.
 * So it needs CONFIG_CONNECTOR and CONFIG_PROC_EVENTS (or modules "cn" and "cn_proc")
 */
NiceContext *
nice_init(void)
{
    NiceContext *nice = g_new0(NiceContext, 1);

    nice->nl_sock = -1;

    read_config(&nice->programs);

    nice->nl_sock = socket(PF_NETLINK,
                           SOCK_DGRAM | SOCK_CLOEXEC,
                           NETLINK_CONNECTOR);
    if (nice->nl_sock < 0) {
        g_warning("nice: failed to create netlink socket: %s",
                  g_strerror(errno));
        nice_cleanup(nice);
        return NULL;
    }

    struct sockaddr_nl addr;
    memset(&addr, 0, sizeof(addr));

    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();
    addr.nl_groups = CN_IDX_PROC;

    if (bind(nice->nl_sock,
             (struct sockaddr *)&addr,
             sizeof(addr)) < 0) {
        g_warning("nice: failed to bind netlink socket: %s",
                  g_strerror(errno));
        nice_cleanup(nice);
        return NULL;
    }

    nice->nl_channel = g_io_channel_unix_new(nice->nl_sock);
    if (!nice->nl_channel) {
        g_warning("nice: failed to create GIOChannel");
        nice_cleanup(nice);
        return NULL;
    }

    g_io_channel_set_encoding(nice->nl_channel, NULL, NULL);
    g_io_channel_set_close_on_unref(nice->nl_channel, FALSE);

    GIOFlags flags = g_io_channel_get_flags(nice->nl_channel);

    g_autoptr(GError) error = NULL;
    if (g_io_channel_set_flags(nice->nl_channel,
                               flags | G_IO_FLAG_NONBLOCK,
                               &error) != G_IO_STATUS_NORMAL) {
        g_warning("nice: failed to set nonblocking mode: %s",
                  error ? error->message : "unknown error");
        nice_cleanup(nice);
        return NULL;
    }

    nice->enabled = FALSE;

    return nice;
}

gboolean
nice_enable(NiceContext *nice, gboolean enabled)
{
    if (!nice)
        return FALSE;

    enabled = enabled ? TRUE : FALSE;

    if (nice->enabled == enabled)
        return TRUE;

    if (enabled) {
        read_config(&nice->programs);

        if (set_proc_ev_listen(nice->nl_sock, TRUE) < 0)
            return FALSE;

        nice->enabled = TRUE;

        nice->nl_watch_id = g_io_add_watch(nice->nl_channel,
                                           G_IO_IN |
                                           G_IO_HUP |
                                           G_IO_ERR |
                                           G_IO_NVAL,
                                           netlink_event_callback,
                                           nice);

        if (nice->nl_watch_id == 0) {
            nice->enabled = FALSE;
            set_proc_ev_listen(nice->nl_sock, FALSE);
            return FALSE;
        }

        initial_process_set(nice);

        g_debug("nice: enabled");
        return TRUE;
    }

    nice->enabled = FALSE;

    if (nice->nl_watch_id) {
        g_source_remove(nice->nl_watch_id);
        nice->nl_watch_id = 0;
    }

    if (set_proc_ev_listen(nice->nl_sock, FALSE) < 0)
        return FALSE;

    g_debug("nice: disabled");
    return TRUE;
}

gboolean
nice_is_enabled(const NiceContext *nice)
{
    if (!nice)
        return FALSE;

    return nice->enabled ? TRUE : FALSE;
}

void
nice_cleanup(NiceContext *nice)
{
    if (!nice)
        return;

    if (nice->enabled)
        nice_enable(nice, FALSE);

    if (nice->nl_watch_id) {
        g_source_remove(nice->nl_watch_id);
        nice->nl_watch_id = 0;
    }

    if (nice->nl_channel) {
        g_io_channel_unref(nice->nl_channel);
        nice->nl_channel = NULL;
    }

    if (nice->nl_sock >= 0) {
        close(nice->nl_sock);
        nice->nl_sock = -1;
    }

    g_free(nice);
}
