// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2024 Bardia Moshiri <fakeshell@bardia.tech>

#include "batman-wrappers.h"
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include "wlrdisplay.h"
#include "getinfo.h"

#define MEMINFO "/proc/meminfo"
#define STAT "/proc/stat"
#define NUM_SAMPLES 3
#define SAMPLE_INTERVAL_MS 100

static void
_get_battery_info(UpClient *upower,
                  batman_state_t *state,
                  gdouble *percentage,
                  const gchar **statelabel)
{
    UpDevice *device = NULL;
    *state = BATMAN_NO_BATTERY;
    if (statelabel)
        *statelabel = NULL;
    if (percentage)
        *percentage = 0.0;

    device = up_device_new();

    if (!up_device_set_object_path_sync(device, "/org/freedesktop/UPower/devices/DisplayDevice", NULL, NULL)) {
        g_object_unref(device);
        g_debug("Failed to set device object path");
        return;
    }

    if (device != NULL) {
        UpDeviceState up_state;
        gboolean power_supply;
        UpDeviceKind kind;
        gdouble percent;

        g_object_get(device,
                     "power-supply", &power_supply,
                     "kind", &kind,
                     "state", &up_state,
                     "percentage", &percent,
                     NULL);

        if (percentage != NULL)
            *percentage = percent;

        if (power_supply == TRUE && kind == UP_DEVICE_KIND_BATTERY) {
            switch (up_state) {
                case UP_DEVICE_STATE_CHARGING:
                    *state = BATMAN_CHARGING;
                    if (statelabel)
                        *statelabel = "charging";
                    break;
                case UP_DEVICE_STATE_DISCHARGING:
                    *state = BATMAN_DISCHARGING;
                    if (statelabel)
                        *statelabel = "discharging";
                    break;
                case UP_DEVICE_STATE_FULLY_CHARGED:
                    *state = BATMAN_FULLY_CHARGED;
                    if (statelabel)
                        *statelabel = "fully-charged";
                    break;
                default:
                    *state = BATMAN_UNKNOWN;
                    if (statelabel)
                        *statelabel = NULL;
            }
        }

        g_object_unref(device);
    }

    if (*state == BATMAN_NO_BATTERY)
        g_debug("no battery");
}

const gchar *
get_battery_all(UpClient *upower,
                gdouble *percentage)
{
    batman_state_t state;
    const gchar *statelabel = NULL;

    _get_battery_info(upower, &state, percentage, &statelabel);
    return statelabel;
}

gdouble
get_battery_percentage(UpClient *upower)
{
    batman_state_t state;
    gdouble percentage = 0.0;

    _get_battery_info(upower, &state, &percentage, NULL);
    return percentage;
}

batman_state_t
get_battery_state(UpClient *upower)
{
    batman_state_t state;

    _get_battery_info(upower, &state, NULL, NULL);
    return state;
}

const gchar *
findBattery(UpClient *upower,
            gdouble *percentage)
{
    return get_battery_all(upower, percentage);
}

const gchar *
find_battery(UpClient *upower,
             gdouble *percentage)
{
    return get_battery_all(upower, percentage);
}

int
read_mem_info(struct meminfo *mem)
{
    if (mem == NULL) {
        errno = EINVAL;
        return 0;
    }

    FILE *fp = fopen(MEMINFO, "r");
    if (fp == NULL)
        err(EXIT_FAILURE, "fopen(%s, \"r\")", MEMINFO);

    char line[512];
    int flag = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (!strncmp(line, "MemTotal:", 9) && ++flag)
            sscanf(line, "MemTotal: %lld", &(mem->memtotal));
        else if (!strncmp(line, "MemFree:", 8) && ++flag)
            sscanf(line, "MemFree: %lld", &(mem->memfree));
        else if (!strncmp(line, "Buffers:", 8) && ++flag)
            sscanf(line, "Buffers: %lld", &(mem->buffers));
        else if (!strncmp(line, "Cached:", 7) && ++flag)
            sscanf(line, "Cached: %lld", &(mem->cached));
        else if (!strncmp(line, "SReclaimable:", 13) && ++flag)
            sscanf(line, "SReclaimable: %lld", &(mem->sreclaimable));
        else {
            if (flag == 5)
                break;
        }
    }

    if (fclose(fp) == EOF)
        warn("fclose()");

    return 1;
}

int
readMemInfo(struct meminfo *mem)
{
    return read_mem_info(mem);
}

int
read_cpu_stats(cpu_time_t *cpu_time)
{
    FILE *fp = fopen(STAT, "r");
    if (!fp) {
        fprintf(stderr, "Error opening %s: %s\n", STAT, strerror(errno));
        return -1;
    }

    char buffer[256];
    if (!fgets(buffer, sizeof(buffer), fp)) {
        fclose(fp);
        return -1;
    }

    fclose(fp);

    return sscanf(buffer, "cpu  %lld %lld %lld %lld %lld %lld %lld %lld",
           &cpu_time->user, &cpu_time->nice, &cpu_time->system,
           &cpu_time->idle, &cpu_time->iowait, &cpu_time->irq,
           &cpu_time->softirq, &cpu_time->steal);
}

long long
get_total_time(const cpu_time_t *cpu_time)
{
    return cpu_time->user + cpu_time->nice + cpu_time->system +
           cpu_time->idle + cpu_time->iowait + cpu_time->irq +
           cpu_time->softirq + cpu_time->steal;
}

long long
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
    for (int i = start_idx; i < start_idx + num_samples; i++) {
        if (read_cpu_stats(&samples[i]) < 0)
            return -1.0;
        if (i < start_idx + num_samples - 1)
            usleep(SAMPLE_INTERVAL_MS * 1000);
    }

    double total_usage = 0.0;
    for (int i = start_idx; i < start_idx + num_samples - 1; i++) {
        long long total_diff = get_total_time(&samples[i + 1]) -
                               get_total_time(&samples[i]);
        long long idle_diff = get_idle_time(&samples[i + 1]) -
                              get_idle_time(&samples[i]);
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

    double initial_avg = get_samples_average(samples, usage_samples, 0, NUM_SAMPLES);
    if (initial_avg < 0)
        return -1.0;

    // if cpu usage is over 80, get another set of samples. this can happen during switches between online and offline
    if (initial_avg > 80.0) {
        double additional_avg = get_samples_average(samples, usage_samples, NUM_SAMPLES, NUM_SAMPLES);
        if (additional_avg < 0)
            return -1.0;

        return (initial_avg + additional_avg) / 2;
    }

    return initial_avg;
}

double
cpuUsage(void)
{
    return get_cpu_usage();
}

long double
mem_usage(void)
{
    struct meminfo meminfo_new;
    memset(&meminfo_new, 0x00, sizeof(struct meminfo));

    if (!read_mem_info(&meminfo_new))
        return -1.0L;

    long double used = meminfo_new.memtotal - meminfo_new.memfree - meminfo_new.buffers - meminfo_new.cached - meminfo_new.sreclaimable;
    long double total = meminfo_new.memtotal;

    used /= 1000;
    total /= 1000;

    long double percentage = (used / total) * 100;
    return percentage;
}

long double
memUsage(void)
{
    return mem_usage();
}

static void
on_display_signal(GDBusConnection *connection,
                  const gchar     *sender_name,
                  const gchar     *object_path,
                  const gchar     *interface_name,
                  const gchar     *signal_name,
                  GVariant        *parameters,
                  gpointer         user_data)
{
    GMainLoop *loop = (GMainLoop *)user_data;
    g_debug("Signal received");

    gchar *params_str = g_variant_print(parameters, TRUE);
    g_debug("Parameters: %s", params_str);
    g_free(params_str);

    g_main_loop_quit(loop);
}

void
block_display_changed(void)
{
    GDBusConnection *connection;
    GError *error = NULL;
    guint subscription_id;
    GMainLoop *loop;

    connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (error != NULL) {
        g_print("Could not connect to system bus: %s\n", error->message);
        g_error_free(error);
        return;
    }

    loop = g_main_loop_new(NULL, FALSE);

    subscription_id = g_dbus_connection_signal_subscribe(
        connection,
        "org.freedesktop.login1",
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        "/org/freedesktop/login1",
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        on_display_signal,
        loop,
        NULL
    );

    g_main_loop_run(loop);

    g_dbus_connection_signal_unsubscribe(connection, subscription_id);
    g_object_unref(connection);
    g_main_loop_unref(loop);
}
