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

static void _get_battery_info(UpClient *upower, batman_state_t *state, gdouble *percentage, const gchar **statelabel) {
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

const gchar *get_battery_all(UpClient *upower, gdouble *percentage) {
    batman_state_t state;
    const gchar *statelabel = NULL;

    _get_battery_info(upower, &state, percentage, &statelabel);
    return statelabel;
}

gdouble get_battery_percentage(UpClient *upower) {
    batman_state_t state;
    gdouble percentage = 0.0;

    _get_battery_info(upower, &state, &percentage, NULL);
    return percentage;
}

batman_state_t get_battery_state(UpClient *upower) {
    batman_state_t state;

    _get_battery_info(upower, &state, NULL, NULL);
    return state;
}

const gchar *findBattery(UpClient *upower, gdouble *percentage) {
    return get_battery_all(upower, percentage);
}

const gchar *find_battery(UpClient *upower, gdouble *percentage) {
    return get_battery_all(upower, percentage);
}

int read_mem_info(struct meminfo *mem) {
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

int readMemInfo(struct meminfo *mem) {
    return read_mem_info(mem);
}

long long get_total_cpu_time(void) {
    FILE *fp;
    char buffer[128];
    long long user, nice, system, idle, iowait, irq, softirq, steal;

    fp = fopen(STAT, "r");
    if (fp == NULL) {
        perror("Error opening /proc/stat");
        return -1;
    }

    fgets(buffer, 128, fp);
    fclose(fp);

    sscanf(buffer, "cpu  %lld %lld %lld %lld %lld %lld %lld %lld",
           &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
    return user + nice + system + idle + iowait + irq + softirq + steal;
}

long long getTotalCPUTime(void) {
    return get_total_cpu_time();
}

long long get_idle_cpu_time(void) {
    FILE *fp;
    char buffer[128];
    long long user, nice, system, idle, iowait, irq, softirq, steal;

    fp = fopen(STAT, "r");
    if (fp == NULL) {
        perror("Error opening /proc/stat");
        return -1;
    }

    fgets(buffer, 128, fp);
    fclose(fp);

    sscanf(buffer, "cpu  %lld %lld %lld %lld %lld %lld %lld %lld",
           &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
    return idle + iowait;
}

long long getIdleCPUTime(void) {
    return get_idle_cpu_time();
}

double get_cpu_usage(void) {
    long long total_cpu_time_1 = get_total_cpu_time();
    long long idle_cpu_time_1 = get_idle_cpu_time();

    usleep(500000);

    long long total_cpu_time_2 = get_total_cpu_time();
    long long idle_cpu_time_2 = get_idle_cpu_time();

    double total_diff = (double)(total_cpu_time_2 - total_cpu_time_1);
    double idle_diff = (double)(idle_cpu_time_2 - idle_cpu_time_1);

    if (total_diff <= idle_diff)
        return 0.0;

    double usage = 100.0 * (1.0 - idle_diff / total_diff);
    return usage;
}

double cpuUsage(void) {
    return get_cpu_usage();
}

long double mem_usage(void) {
    struct meminfo meminfo_new;
    memset(&meminfo_new, 0x00, sizeof(struct meminfo));

    if (!read_mem_info(&meminfo_new))
        return -1.0L;

    long double used = meminfo_new.memtotal - meminfo_new.memfree -
                      meminfo_new.buffers - meminfo_new.cached - meminfo_new.sreclaimable;
    long double total = meminfo_new.memtotal;

    used /= 1000;
    total /= 1000;

    long double percentage = (used / total) * 100;
    return percentage;
}

long double memUsage(void) {
    return mem_usage();
}
