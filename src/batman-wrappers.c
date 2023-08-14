// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2023 Bardia Moshiri <fakeshell@bardia.tech>

#include "batman-wrappers.h"
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#ifdef WITH_UPOWER
#include <upower.h>
#endif

#ifdef WITH_WLRDISPLAY
#include "wlrdisplay.h"
#endif

#ifdef WITH_GETINFO
#include "getinfo.h"
#endif

#define MEMINFO "/proc/meminfo"

struct meminfo {
    long long int memtotal;
    long long int memfree;
    long long int buffers;
    long long int cached;
    long long int sreclaimable;
};

#ifdef WITH_UPOWER
const gchar *findBattery(UpClient *upower) {
    UpDevice *device = NULL;
    const gchar *statelabel = NULL;

    GPtrArray *devices = up_client_get_devices2(upower);

    for (int i = 0; i < devices->len; i++) {
        UpDevice *this_dev = g_ptr_array_index(devices, i);

        gboolean power_supply;
        UpDeviceKind kind;

        g_object_get(this_dev, "power-supply", &power_supply, "kind", &kind, NULL);

        if (power_supply == TRUE && kind == UP_DEVICE_KIND_BATTERY) {
            device = this_dev;
            g_object_ref(device);
            break;
        }
    }

    if (device != NULL) {
        UpDeviceState state;
        g_object_get(device, "state", &state, NULL);

        switch (state) {
        case UP_DEVICE_STATE_CHARGING:
            statelabel = "charging";
            break;
        case UP_DEVICE_STATE_DISCHARGING:
            statelabel = "discharging";
            break;
        case UP_DEVICE_STATE_FULLY_CHARGED:
            statelabel = "fully-charged";
            break;
        default:
            statelabel = NULL;
        }

        g_object_unref(device);
    }

    g_ptr_array_unref(devices);

    if (statelabel == NULL) {
        g_print("no battery\n");
    }

    return statelabel;
}
#endif

int readMemInfo(struct meminfo *mem) {
    if (mem == NULL) {
        errno = EINVAL;
        return 0;
    }

    FILE *fp = fopen(MEMINFO, "r");
    if (fp == NULL) {
        err(EXIT_FAILURE, "fopen(%s, \"r\")", MEMINFO);
    }

    char line[512];
    int flag = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (!strncmp(line, "MemTotal:", 9) && ++flag) {
            sscanf(line, "MemTotal: %lld", &(mem->memtotal));
        } else if (!strncmp(line, "MemFree:", 8) && ++flag) {
            sscanf(line, "MemFree: %lld", &(mem->memfree));
        } else if (!strncmp(line, "Buffers:", 8) && ++flag) {
            sscanf(line, "Buffers: %lld", &(mem->buffers));
        } else if (!strncmp(line, "Cached:", 7) && ++flag) {
            sscanf(line, "Cached: %lld", &(mem->cached));
        } else if (!strncmp(line, "SReclaimable:", 13) && ++flag) {
            sscanf(line, "SReclaimable: %lld", &(mem->sreclaimable));
        } else {
            if (flag == 5) {
                break;
            }
        }
    }

    if (fclose(fp) == EOF) {
        warn("fclose()");
    }

    return 1;
}

long long getTotalCPUTime() {
    FILE *fp;
    char buffer[128];
    long long user, nice, system, idle, iowait, irq, softirq, steal;

    fp = fopen("/proc/stat", "r");
    if (fp == NULL) {
        perror("Error opening /proc/stat");
        return -1;
    }

    fgets(buffer, 128, fp);
    fclose(fp);

    sscanf(buffer, "cpu  %lld %lld %lld %lld %lld %lld %lld %lld", &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
    return user + nice + system + idle + iowait + irq + softirq + steal;
}

long long getIdleCPUTime() {
    FILE *fp;
    char buffer[128];
    long long user, nice, system, idle, iowait, irq, softirq, steal;

    fp = fopen("/proc/stat", "r");
    if (fp == NULL) {
        perror("Error opening /proc/stat");
        return -1;
    }

    fgets(buffer, 128, fp);
    fclose(fp);

    sscanf(buffer, "cpu  %lld %lld %lld %lld %lld %lld %lld %lld", &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
    return idle + iowait;
}

double cpuUsage() {
    long long total_cpu_time_1 = getTotalCPUTime();
    long long idle_cpu_time_1 = getIdleCPUTime();

    usleep(250000);

    long long total_cpu_time_2 = getTotalCPUTime();
    long long idle_cpu_time_2 = getIdleCPUTime();

    double total_diff = (double)(total_cpu_time_2 - total_cpu_time_1);
    double idle_diff = (double)(idle_cpu_time_2 - idle_cpu_time_1);

    if (total_diff <= idle_diff) {
        return 0.0;
    }

    double cpu_usage = 100.0 * (1.0 - idle_diff / total_diff);
    return cpu_usage;
}

long double memUsage() {
    struct meminfo meminfo_new;
    memset(&meminfo_new, 0x00, sizeof(struct meminfo));

    if (!readMemInfo(&meminfo_new)) {
        return -1.0L;
    }

    long double used = meminfo_new.memtotal - meminfo_new.memfree -
                       meminfo_new.buffers - meminfo_new.cached - meminfo_new.sreclaimable;
    long double total = meminfo_new.memtotal;

    used /= 1000;
    total /= 1000;

    long double percentage = (used / total) * 100;
    return percentage;
}
