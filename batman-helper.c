#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define MEMINFO "/proc/meminfo"

struct meminfo {
    long long int memtotal;
    long long int memfree;
    long long int buffers;
    long long int cached;
    long long int sreclaimable;
};

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
        printf("Usage: %s [cpu|mem]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "cpu") == 0) {
        return cpuUsage();
    } else if (strcmp(argv[1], "mem") == 0) {
        return memUsage();
    } else {
        printf("Invalid option. Usage: %s [cpu|mem]\n", argv[0]);
        return EXIT_FAILURE;
    }
}
