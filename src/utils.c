/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2026 Bardia Moshiri <bardia@furilabs.com>
 */

#define _GNU_SOURCE

#include "utils.h"

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>

#define STAT "/proc/stat"
#define NUM_SAMPLES 3
#define SAMPLE_INTERVAL_MS 100

#define SYSTEMD_BUS_NAME "org.freedesktop.systemd1"
#define SYSTEMD_OBJ_PATH "/org/freedesktop/systemd1"
#define SYSTEMD_MANAGER_IFACE "org.freedesktop.systemd1.Manager"

/**
 * Structure containing CPU time information
 */
typedef struct {
    long long user;     /** Time spent in user mode */
    long long nice;     /** Time spent in user mode with low priority */
    long long system;   /** Time spent in system mode */
    long long idle;     /** Time spent in idle task */
    long long iowait;   /** Time waiting for I/O to complete */
    long long irq;      /** Time servicing interrupts */
    long long softirq;  /** Time servicing softirqs */
    long long steal;    /** Stolen time */
} cpu_time_t;

gboolean
exists(const char *path)
{
    if (path == NULL)
        return FALSE;
    if (access(path, F_OK) == 0)
        return TRUE;
    return FALSE;
}

gboolean
dir_exists(const char *path)
{
    struct stat st;

    if (path == NULL || path[0] == '\0')
        return FALSE;
    if (stat(path, &st) != 0)
        return FALSE;
    if (S_ISDIR(st.st_mode))
        return TRUE;
    return FALSE;
}

void
ensure_dir(const char *path, unsigned int mode)
{
    struct stat st;

    if (path == NULL)
        return;

    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode))
            return;

        g_warning("%s exists but is not a directory", path);
        return;
    }

    if (mkdir(path, (mode_t)mode) != 0)
        g_warning("mkdir(%s) failed: %s", path, g_strerror(errno));
}

void
write_str(const char *path, const char *value)
{
    int fd;
    ssize_t want;
    ssize_t wrote;

    if (path == NULL || value == NULL)
        return;

    g_debug("write_str: writing to %s", path);

    fd = open(path, O_WRONLY | O_CLOEXEC | O_CREAT | O_TRUNC, 0644);

    if (fd < 0) {
        g_debug("open(%s) for write failed: %s", path, g_strerror(errno));
        return;
    }

    want = (ssize_t)strlen(value);
    wrote = write(fd, value, (size_t)want);

    if (wrote != want)
        g_debug("write_str: write(%s) failed: %s", path, g_strerror(errno));
    else
        g_debug("write_str: wrote %zd bytes to %s", wrote, path);

    close(fd);
}

void
write_int(const char *path, long long value)
{
    char buf[64];

    if (path == NULL)
        return;

    g_snprintf(buf, sizeof(buf), "%lld", value);
    write_str(path, buf);
}

gboolean
read_str(const char *path, char *out, size_t out_len)
{
    int fd;
    ssize_t n;
    size_t len;

    if (path == NULL)
        return FALSE;
    if (out == NULL)
        return FALSE;
    if (out_len == 0)
        return FALSE;

    fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0)
        return FALSE;

    n = read(fd, out, out_len - 1);

    close(fd);

    if (n <= 0)
        return FALSE;

    out[n] = '\0';

    /* trim trailing whitespace */
    len = (size_t)n;
    while (len > 0) {
        char c = out[len - 1];

        if (c == '\n' || c == '\r' || c == ' ' || c == '\t') {
            out[len - 1] = '\0';
            len--;
        } else {
            break;
        }
    }

    return TRUE;
}

gboolean
string_contains_token(const char *haystack, const char *needle)
{
    if (haystack == NULL)
        return FALSE;
    if (needle == NULL)
        return FALSE;
    if (strstr(haystack, needle) != NULL)
        return TRUE;
    return FALSE;
}

gboolean
file_contains_token(const char *path, const char *needle)
{
    char buf[4096];

    if (path == NULL || needle == NULL)
        return FALSE;

    buf[0] = '\0';
    if (!read_str(path, buf, sizeof(buf)))
        return FALSE;

    if (buf[0] == '\0')
        return FALSE;

    return string_contains_token(buf, needle) ? TRUE : FALSE;
}

static int
read_cpu_stats(cpu_time_t *cpu_time)
{
    FILE *fp;
    char buffer[256];
    int ret;

    fp = fopen(STAT, "r");
    if (fp == NULL) {
        g_debug("Error opening %s: %s", STAT, strerror(errno));
        return -1;
    }

    if (!fgets(buffer, sizeof(buffer), fp)) {
        fclose(fp);
        return -1;
    }

    fclose(fp);

    ret = sscanf(buffer, "cpu  %lld %lld %lld %lld %lld %lld %lld %lld",
                 &cpu_time->user, &cpu_time->nice, &cpu_time->system,
                 &cpu_time->idle, &cpu_time->iowait, &cpu_time->irq,
                 &cpu_time->softirq, &cpu_time->steal);

    return ret;
}

static long long
get_total_time(const cpu_time_t *cpu_time)
{
    return cpu_time->user + cpu_time->nice + cpu_time->system +
           cpu_time->idle + cpu_time->iowait + cpu_time->irq +
           cpu_time->softirq + cpu_time->steal;
}

static long long
get_idle_time(const cpu_time_t *cpu_time)
{
    return cpu_time->idle + cpu_time->iowait;
}

static double
get_samples_average(cpu_time_t *samples,
                    double *usage_samples,
                    int start_idx,
                    int num_samples)
{
    int i;
    double total_usage;

    for (i = start_idx; i < start_idx + num_samples; i++) {
        if (read_cpu_stats(&samples[i]) < 0)
            return -1.0;
        if (i < start_idx + num_samples - 1)
            usleep(SAMPLE_INTERVAL_MS * 1000);
    }

    total_usage = 0.0;

    for (i = start_idx; i < start_idx + num_samples - 1; i++) {
        long long total_diff;
        long long idle_diff;

        total_diff = get_total_time(&samples[i + 1]) - get_total_time(&samples[i]);
        idle_diff = get_idle_time(&samples[i + 1]) - get_idle_time(&samples[i]);

        if (total_diff <= idle_diff || total_diff == 0)
            usage_samples[i] = 0.0;
        else
            usage_samples[i] = 100.0 * (1.0 - (double)idle_diff / total_diff);

        total_usage += usage_samples[i];
    }

    return total_usage / (num_samples - 1);
}

double
get_cpu_usage(void)
{
    cpu_time_t samples[NUM_SAMPLES * 2];
    double usage_samples[NUM_SAMPLES * 2 - 1];
    double initial_avg;

    initial_avg = get_samples_average(samples, usage_samples, 0, NUM_SAMPLES);
    if (initial_avg < 0)
        return -1.0;

    /* if cpu usage is over 80, get another set of samples. this can happen during switches between online and offline */
    if (initial_avg > 80.0) {
        double additional_avg;

        additional_avg = get_samples_average(samples, usage_samples, NUM_SAMPLES, NUM_SAMPLES);
        if (additional_avg < 0)
            return -1.0;

        return (initial_avg + additional_avg) / 2;
    }

    return initial_avg;
}

void
chown_recursive(const char *path, uid_t uid, gid_t gid)
{
    if (path == NULL || path[0] == '\0')
        return;

    struct stat st;
    if (lstat(path, &st) != 0)
        return;

    if (lchown(path, uid, gid) != 0)
        return;

    if (!S_ISDIR(st.st_mode))
        return;

    DIR *dir = opendir(path);
    if (!dir)
        return;

    struct dirent *de = NULL;
    while ((de = readdir(dir)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;

        char child[1024];
        int n = snprintf(child, sizeof(child), "%s/%s", path, de->d_name);
        if (n <= 0 || (size_t)n >= sizeof(child))
            continue;

        chown_recursive(child, uid, gid);
    }

    closedir(dir);
}

static char *
normalize_service_unit(const char *unit)
{
    if (unit == NULL || unit[0] == '\0')
        return NULL;

    if (g_str_has_suffix(unit, ".service"))
        return g_strdup(unit);

    return g_strdup_printf("%s.service", unit);
}

static void
systemd_call_done(GObject *source, GAsyncResult *res, gpointer userdata)
{
    (void)userdata;

    g_autoptr(GError) error = NULL;
    g_autoptr(GVariant) reply = g_dbus_connection_call_finish(G_DBUS_CONNECTION(source), res, &error);

    if (!reply) {
        g_debug("systemd D-Bus call failed: %s", error ? error->message : "unknown error");
        return;
    }

    const char *job_path = NULL;
    g_variant_get(reply, "(&o)", &job_path);
    g_debug("systemd job: %s", job_path ? job_path : "(null)");
}

static void
systemd_service_action_async(const char *method, const char *unit)
{
    if (method == NULL || method[0] == '\0')
        return;

    g_autofree char *u = normalize_service_unit(unit);
    if (u == NULL)
        return;

    g_autoptr(GError) error = NULL;
    g_autoptr(GDBusConnection) bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);

    if (!bus) {
        g_debug("g_bus_get_sync(system) failed: %s", error ? error->message : "unknown error");
        return;
    }

    g_dbus_connection_call(bus,
                           SYSTEMD_BUS_NAME,
                           SYSTEMD_OBJ_PATH,
                           SYSTEMD_MANAGER_IFACE,
                           method,
                           g_variant_new("(ss)", u, "replace"),
                           G_VARIANT_TYPE("(o)"),
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL,
                           systemd_call_done,
                           NULL);
}

void
systemd_service_start_async(const char *unit)
{
    g_debug("systemd: starting %s", unit);
    systemd_service_action_async("StartUnit", unit);
}

void
systemd_service_stop_async(const char *unit)
{
    g_debug("systemd: stopping %s", unit);
    systemd_service_action_async("StopUnit", unit);
}

gboolean
systemd_service_exists(const char *unit)
{
    g_autofree char *u = normalize_service_unit(unit);
    if (u == NULL)
        return FALSE;

    g_autoptr(GError) error = NULL;
    g_autoptr(GDBusConnection) bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);

    if (!bus)
        return FALSE;

    g_autoptr(GVariant) reply = g_dbus_connection_call_sync(bus,
                                                            SYSTEMD_BUS_NAME,
                                                            SYSTEMD_OBJ_PATH,
                                                            SYSTEMD_MANAGER_IFACE,
                                                            "GetUnit",
                                                            g_variant_new("(s)", u),
                                                            G_VARIANT_TYPE("(o)"),
                                                            G_DBUS_CALL_FLAGS_NONE,
                                                            2000,
                                                            NULL,
                                                            &error);

    if (!reply)
        return FALSE;

    return TRUE;
}

gchar *
make_valid_utf8(const gchar *s)
{
    if (!s)
        return g_strdup("");

    if (g_utf8_validate(s, -1, NULL))
        return g_strdup(s);

    return g_utf8_make_valid(s, -1);
}
