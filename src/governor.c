// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2024 Bardia Moshiri <fakeshell@bardia.tech>

#include <stdio.h>
#include <time.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "wlrdisplay.h"
#include "governor.h"

volatile sig_atomic_t keep_going = 1;
char cpu_usage[1024] = "unknown\n";
pthread_mutex_t cpu_usage_mutex = PTHREAD_MUTEX_INITIALIZER;

const char *paths[] = {
    "/sys/class/devfreq/soc:qcom,cci/governor",
    "/sys/class/devfreq/soc:qcom,cpubw/governor",
    "/sys/class/devfreq/soc:qcom,gpubw/governor",
    "/sys/class/devfreq/soc:qcom,kgsl-busmon/governor",
    "/sys/class/devfreq/soc:qcom,mincpubw/governor",
    "/sys/class/devfreq/soc:qcom,l3-cpu0/governor",
    "/sys/class/devfreq/soc:qcom,l3-cpu6/governor",
    "/sys/class/devfreq/aa00000.qcom,vidc1:arm9_bus_ddr/governor",
    "/sys/class/devfreq/aa00000.qcom,vidc1:bus_cnoc/governor",
    "/sys/class/devfreq/aa00000.qcom,vidc1:venus_bus_ddr/governor",
    "/sys/class/devfreq/aa00000.qcom,vidc:arm9_bus_ddr/governor",
    "/sys/class/devfreq/aa00000.qcom,vidc:bus_cnoc/governor",
    "/sys/class/devfreq/aa00000.qcom,vidc:venus_bus_ddr/governor",
    "/sys/class/devfreq/soc:qcom,l3-cdsp/governor",
    "/sys/class/devfreq/soc:qcom,memlat-cpu0/governor",
    "/sys/class/devfreq/soc:qcom,memlat-cpu4/governor",
    "/sys/class/devfreq/soc:qcom,memlat-cpu6/governor",
    "/sys/class/devfreq/soc:qcom,mincpu0bw/governor",
    "/sys/class/devfreq/soc:qcom,mincpu6bw/governor",
    "/sys/class/devfreq/soc:devfreq_spdm_cpu/governor",
    "/sys/class/devfreq/soc:qcom,cdsp-cdsp-l3-lat/governor",
    "/sys/class/devfreq/soc:qcom,cpu0-cpu-ddr-latfloor/governor",
    "/sys/class/devfreq/soc:qcom,cpu0-cpu-l3-lat/governor",
    "/sys/class/devfreq/soc:qcom,cpu0-cpu-llcc-lat/governor",
    "/sys/class/devfreq/soc:qcom,cpu6-cpu-ddr-latfloor/governor",
    "/sys/class/devfreq/soc:qcom,cpu6-cpu-l3-lat/governor",
    "/sys/class/devfreq/soc:qcom,cpu6-cpu-llcc-lat/governor",
    "/sys/class/devfreq/soc:qcom,cpu6-llcc-ddr-lat/governor",
    "/sys/class/devfreq/soc:qcom,cpu-cpu-llcc-bw/governor",
    "/sys/class/devfreq/soc:qcom,cpu-llcc-ddr-bw/governor",
    "/sys/class/devfreq/soc:qcom,npudsp-npu-ddr-bw/governor",
    "/sys/class/kgsl/kgsl-3d0/devfreq/governor",
    "/sys/class/kgsl/kgsl-3d0/governor",
    "/sys/class/devfreq/1c00000.qcom,kgsl-3d0/governor",
    "/sys/class/devfreq/5000000.qcom,kgsl-3d0/governor",
    "/sys/class/devfreq/ddr_devfreq/governor",
    "/sys/class/devfreq/graphics/governor",
    "/sys/kernel/gpu/gpu_governor",
    "/sys/power/cpufreq_max_limit",
    "/sys/power/cpufreq_min_limit",
    "/sys/class/devfreq/17000010.devfreq_mif/governor",
    "/sys/class/devfreq/17000020.devfreq_int/governor",
    "/sys/class/devfreq/17000030.devfreq_intcam/governor",
    "/sys/class/devfreq/17000040.devfreq_disp/governor",
    "/sys/class/devfreq/17000050.devfreq_cam/governor",
    "/sys/class/devfreq/17000060.devfreq_tnr/governor",
    "/sys/class/devfreq/17000070.devfreq_mfc/governor",
    "/sys/class/devfreq/17000080.devfreq_bo/governor",
    "/sys/class/devfreq/gs_memlat_devfreq:devfreq_mif_cpu0_memlat@17000010/governor",
    "/sys/class/devfreq/gs_memlat_devfreq:devfreq_mif_cpu1_memlat@17000010/governor",
    "/sys/class/devfreq/gs_memlat_devfreq:devfreq_mif_cpu2_memlat@17000010/governor",
    "/sys/class/devfreq/gs_memlat_devfreq:devfreq_mif_cpu3_memlat@17000010/governor",
    "/sys/class/devfreq/gs_memlat_devfreq:devfreq_mif_cpu4_memlat@17000010/governor",
    "/sys/class/devfreq/gs_memlat_devfreq:devfreq_mif_cpu5_memlat@17000010/governor",
    "/sys/class/devfreq/gs_memlat_devfreq:devfreq_mif_cpu6_memlat@17000010/governor",
    "/sys/class/devfreq/gs_memlat_devfreq:devfreq_mif_cpu7_memlat@17000010/governor",
    "/proc/cpufreq/cpufreq_power_mode"
    "/proc/cpufreq/cpufreq_cci_mode",
    "/proc/cpufreq/cpufreq_sched_disable",
    "/sys/devices/system/cpu/perf/enable",
    "/sys/devices/system/cpu/sched/sched_boost",
    "/proc/cpufreq/cpufreq_stress_test",
    "/sys/module/ged/parameters/boost_gpu_enable",
    "/sys/module/ged/parameters/boost_extra",
    "/sys/module/ged/parameters/gx_game_mode",
    "/sys/module/ged/parameters/gx_boost_on",
    "/sys/kernel/gbe/gbe_enable1",
    "/sys/kernel/gbe/gbe_enable2",
    "/sys/kernel/gbe/gbe2_max_boost_cnt",
    "/sys/kernel/gbe/gbe2_loading_th",
    "/sys/module/ged/parameters/ged_force_mdp_enable",
    "/sys/module/lpm_levels/parameters/sleep_disabled",
    "/sys/module/lpm_levels/parameters/lpm_prediction",
    "/sys/module/lpm_levels/parameters/lpm_ipi_prediction",
    "/sys/kernel/debug/msm_vidc/fw_low_power_mode",
    "/sys/devices/platform/soc/1d84000.ufshc/hibern8_on_idle_enable",
    "/sys/devices/platform/soc/1d84000.ufshc/clkgate_enable",
    "/sys/devices/platform/soc/1d84000.ufshc/clkscale_enable",
    "/proc/cpufreq/MT_CPU_DVFS_CCI/cpufreq_turbo_mode",
    "/proc/cpufreq/MT_CPU_DVFS_L/cpufreq_turbo_mode",
    "/proc/cpufreq/MT_CPU_DVFS_LL/cpufreq_turbo_mode"
};

// Signal handler to handle Ctrl+C
void handle_sigint(int sig)
{
    keep_going = 0;
}

char *get_node_name(const char *path) {
    char *last_slash = strrchr(path, '/');
    if (last_slash != NULL) {
        char *second_last_slash = last_slash;
        while (second_last_slash > path) {
            --second_last_slash;
            if (*second_last_slash == '/') {
                return second_last_slash + 1;
            }
        }
    }

    return "unknown";
}

const char *arch_x86[] = {"i686", "x86_64"};

int is_arch_x86() {
    struct utsname buffer;

    if (uname(&buffer) != 0) {
        fprintf(stderr, "uname() failed\n");
        return -1;
    }

    for(int i = 0; i < sizeof(arch_x86) / sizeof(arch_x86[0]); ++i) {
        if(strcmp(buffer.machine, arch_x86[i]) == 0) {
            return 1;
        }
    }

    return 0;
}

void *update_cpu_usage(void *arg) {
    const int sleep_interval_in_seconds = 2;
    const int checks_per_interval = 10;
    const int sleep_time = sleep_interval_in_seconds / checks_per_interval;

    for(int i = 0; i < sleep_interval_in_seconds; i += sleep_time) {
        if(!keep_going) {
            break;
        }

        if (i % sleep_interval_in_seconds == 0) {
            FILE *fp = popen("batman-helper cpu", "r");
            if (fp != NULL) {
                char new_cpu_usage[1024];
                if (fgets(new_cpu_usage, sizeof(new_cpu_usage), fp) != NULL) {
                    pthread_mutex_lock(&cpu_usage_mutex);
                    strncpy(cpu_usage, new_cpu_usage, sizeof(cpu_usage) - 1);
                    pthread_mutex_unlock(&cpu_usage_mutex);
                }

                pclose(fp);
            }
        }

        sleep(sleep_time);
    }

    return NULL;
}

void get_system_info(int x86) {
    static time_t last_cpu_update = 0;

    time_t current_time = time(NULL);

    char buf[1024];
    FILE *file;
    int first_core = -1, last_core = -1, core;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    size_t n_paths = sizeof(paths) / sizeof(paths[0]);
    char path[1035];

    char cmd[512];
    DIR *d;
    struct dirent *dir;
    char dir_path[] = "/run/user";
    char uid_path[256];

    d = opendir(dir_path);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_DIR && strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
                snprintf(uid_path, sizeof(uid_path), "%s/%s", dir_path, dir->d_name);

                if(setenv("XDG_RUNTIME_DIR", uid_path, 1) != 0) {
                    printf("Could not set XDG_RUNTIME_DIR\n");
                    closedir(d);
                    return;
                }

                int result = wlrdisplay(0, NULL);
                printf("wlroots screen status for UID %s: %s", dir->d_name, result == 0 ? "yes\n" : "no\n");
                break;
            }
        }

        closedir(d);
    } else {
        printf("Could not open directory %s\n", dir_path);
        return;
    }

    pthread_mutex_lock(&cpu_usage_mutex);
    printf("cpu usage: %s", cpu_usage);
    pthread_mutex_unlock(&cpu_usage_mutex);

    file = fopen("/proc/cpuinfo", "r");
    if (file == NULL) {
        printf("CPU Information: unknown\n");
    } else {
        while ((read = getline(&line, &len, file)) != -1) {
            if (sscanf(line, "processor : %d", &core) == 1) {
                if (first_core == -1) first_core = core;
                last_core = core;
            }
        }

        fclose(file);
        free(line);
        line = NULL;
    }

    const char *CPUFREQ;
    if (x86 == 1) {
        CPUFREQ = "scaling_cur_freq";
    } else if (x86 == 0) {
        CPUFREQ = "cpuinfo_cur_freq";
    } else {
        CPUFREQ= "unknown"; // Failed to get architecture
    }

    for(int i = first_core; i <= last_core; ++i) {
        char path[1024];

        sprintf(path, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_governor", i);
        file = fopen(path, "r");
        if (file != NULL) {
            if(fgets(buf, sizeof(buf), file) != NULL) {
                char node_name[1024];
                strncpy(node_name, get_node_name(path), sizeof(node_name));
                printf("%s: %s", node_name, buf);
            }

            fclose(file);
        }

        sprintf(path, "/sys/devices/system/cpu/cpu%d/cpufreq/%s", i, CPUFREQ);
        file = fopen(path, "r");
        if (file != NULL) {
            if(fgets(buf, sizeof(buf), file) != NULL) {
                char node_name[1024];
                strncpy(node_name, get_node_name(path), sizeof(node_name));
                printf("%s: %s", node_name, buf);
            }

            fclose(file);
        }

        sprintf(path, "/sys/devices/system/cpu/cpu%d/online", i);
        file = fopen(path, "r");
        if (file != NULL) {
            if(fgets(buf, sizeof(buf), file) != NULL) {
                char node_name[1024];
                strncpy(node_name, get_node_name(path), sizeof(node_name));
                printf("%s: %s", node_name, buf);
            }

            fclose(file);
        }
    }

    for(size_t i = 0; i < n_paths; ++i) {
//        printf("path: %s\n", paths[i]);
        file = fopen(paths[i], "r");

        if (file != NULL) {
            if(fgets(buf, sizeof(buf), file) != NULL) {
                // Get node name.
                char node_name[1024];
                strncpy(node_name, get_node_name(paths[i]), sizeof(node_name));
                char *last_slash = strchr(node_name, '/');
                if (last_slash != NULL) {
                    *last_slash = '\0';  // Null terminate at the last slash to get only the node name.
                }

                printf("%s: %s", node_name, buf);
            }

            fclose(file);
        }
    }
}

int main() {
    // Set up the signal handler for Ctrl+C
    signal(SIGINT, handle_sigint);

    int x86 = is_arch_x86();

    pthread_t cpu_usage_thread;
    pthread_create(&cpu_usage_thread, NULL, update_cpu_usage, NULL);

    while(keep_going) {
        printf("\033[H\033[J");  // Clear the screen
        get_system_info(x86);
        sleep(1);
    }

    pthread_join(cpu_usage_thread, NULL);

    return 0;
}
