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

#define MEMINFO "/proc/meminfo"

struct meminfo {
    long long int memtotal;
    long long int memfree;
    long long int buffers;
    long long int cached;
    long long int sreclaimable;
};

#ifdef WITH_UPOWER
UpDevice *findBattery(UpClient *upower)
{
    UpDevice *device = NULL;

    GPtrArray *devices = up_client_get_devices2(upower);

    for (int i = 0; i < devices->len; i++) {
        UpDevice *this_dev = g_ptr_array_index(devices, i);

        gboolean power_supply;
        UpDeviceKind kind;

        g_object_get(this_dev, "power-supply", &power_supply, "kind", &kind, NULL);

        if (power_supply == TRUE && kind == UP_DEVICE_KIND_BATTERY)
            device = this_dev;
    }

    if (device != NULL)
        g_object_ref(device);

    g_ptr_array_unref(devices);

    if (device == NULL) {
        g_print("no battery\n");
        return NULL;
    }

    UpDeviceState state;

    g_object_get(device, "state", &state, NULL);

    const gchar *statelabel;

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

    if (statelabel != NULL) {
        g_print("%s\n", statelabel);
    }

    g_object_unref(device);

    return device;
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

int cpuUsage() {
    long long total_cpu_time_1 = getTotalCPUTime();
    long long idle_cpu_time_1 = getIdleCPUTime();

    usleep(100000);

    long long total_cpu_time_2 = getTotalCPUTime();
    long long idle_cpu_time_2 = getIdleCPUTime();

    double total_diff = (double)(total_cpu_time_2 - total_cpu_time_1);
    double idle_diff = (double)(idle_cpu_time_2 - idle_cpu_time_1);

    double cpu_usage = 100.0 * (1.0 - idle_diff / total_diff);

    printf("%.lf\n", cpu_usage);

    return 0;
}

int memUsage() {
    struct meminfo meminfo_new;
    memset(&meminfo_new, 0x00, sizeof(struct meminfo));

    if (!readMemInfo(&meminfo_new)) {
        return EXIT_FAILURE;
    }

    long double used = meminfo_new.memtotal - meminfo_new.memfree -
                       meminfo_new.buffers - meminfo_new.cached - meminfo_new.sreclaimable;
    long double total = meminfo_new.memtotal;

    used /= 1000;
    total /= 1000;

    long double percentage = (used / total) * 100;

    printf("%.Lf\n", percentage);
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s [cpu|mem|wlrdisplay|battery]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "cpu") == 0) {
        return cpuUsage();
    } else if (strcmp(argv[1], "mem") == 0) {
        return memUsage();
    } else if (strcmp(argv[1], "wlrdisplay") == 0) {
        #ifdef WITH_WLRDISPLAY
        return wlrdisplay(argc, argv);
        #else
        printf("wlrdisplay support is not enabled. Recompile with -DWITH_WLRDISPLAY to enable it.\n");
        return EXIT_FAILURE;
        #endif
    } else if (strcmp(argv[1], "battery") == 0) {
        #ifdef WITH_UPOWER
        UpClient *upower = up_client_new();

        if (upower == NULL) {
            g_print("Could not connect to upower");
            return 2;
        }

        UpDevice *battery = findBattery(upower);

        g_object_unref(upower);

        return 0;
        #else
        printf("Upower support is not enabled. Recompile with -DWITH_UPOWER to enable it.\n");
        return EXIT_FAILURE;
        #endif
    } else {
        printf("Invalid option. Usage: %s [cpu|mem|battery]\n", argv[0]);
        return EXIT_FAILURE;
    }
}
