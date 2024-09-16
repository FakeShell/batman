// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2024 Bardia Moshiri <fakeshell@bardia.tech>

#include <gio/gio.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/resource.h>

#define CONFIG_FILE "/etc/nicerdicer.conf"
#define MAX_LINE_LENGTH 256
#define MAX_PROGRAMS 100
#define MONITOR_INTERVAL 10

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
get_program_name (const gchar *pid, gchar *program_name, size_t program_name_size)
{
    gchar path[64];
    g_snprintf (path, sizeof (path), "/proc/%s/comm", pid);
    FILE *file = fopen (path, "r");
    if (file) {
        if (fgets(program_name, program_name_size, file) != NULL) {
            size_t len = strlen(program_name);
            if (len > 0 && program_name[len-1] == '\n')
                program_name[len-1] = '\0';
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
    if (setpriority (PRIO_PROCESS, pid, niceness) == -1)
        perror ("setpriority");
    else {
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
                    g_print ("Set niceness of thread %d of program '%s' with PID %d to %d\n", tid, program_name, pid, niceness);
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

static gboolean
pid_exists (pid_t pid)
{
    gchar path[64];
    g_snprintf (path, sizeof (path), "/proc/%d", pid);
    struct stat st;
    return (stat (path, &st) == 0);
}

static void
monitor_processes (const gchar *program_name, gint niceness)
{
    pid_t pid = -1;
    gchar pname[256];

    while (TRUE) {
        if (pid == -1 || !pid_exists (pid)) {
            DIR *proc_dir = opendir ("/proc");
            if (!proc_dir) {
                perror ("opendir");
                return;
            }

            struct dirent *entry;
            while ((entry = readdir (proc_dir)) != NULL) {
                if (entry->d_type != DT_DIR || !is_number (entry->d_name)) // shiesty!
                    continue;

                if (get_program_name(entry->d_name, pname, sizeof(pname))) {
                    if (g_strcmp0 (pname, program_name) == 0) {
                        pid = atoi (entry->d_name);
                        set_process_niceness (pid, niceness, program_name);
                        break;
                    }
                }
            }
            closedir (proc_dir);
        }
        sleep (MONITOR_INTERVAL);
    }
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
    ProgramData *data = (ProgramData *)user_data;

    if (g_strcmp0 (method_name, "AddProgram") == 0) {
        const gchar *program_name;
        gint priority;
        g_variant_get (parameters, "(&si)", &program_name, &priority);

        gboolean success = add_or_update_program (data, program_name, priority);

        if (success) {
            if (fork () == 0) {
                monitor_processes (program_name, priority);
                exit (0);
            }
        }

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

    GMainLoop *loop = g_main_loop_new (NULL, FALSE);

    guint owner_id = g_bus_own_name (G_BUS_TYPE_SYSTEM,
                                     "io.FuriOS.NicerDicer",
                                     G_BUS_NAME_OWNER_FLAGS_NONE,
                                     on_bus_acquired,
                                     on_name_acquired,
                                     on_name_lost,
                                     &data,
                                     NULL);

    for (gint i = 0; i < data.count; i++) {
        if (fork () == 0) {
            monitor_processes (data.programs[i], data.niceness[i]);
            exit (0);
        }
    }

    g_main_loop_run (loop);

    g_bus_unown_name (owner_id);
    g_main_loop_unref (loop);

    while (wait (NULL) > 0);

    return 0;
}
