#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/utsname.h>

volatile sig_atomic_t keep_going = 1;

const char *paths[] = {
    "/sys/class/devfreq/soc:qcom,cci/governor",
    "/sys/class/devfreq/soc:qcom,cpubw/governor",
    "/sys/class/devfreq/soc:qcom,gpubw/governor",
    "/sys/class/devfreq/soc:qcom,kgsl-busmon/governor",
    "/sys/class/devfreq/soc:qcom,mincpubw/governor",
    "/sys/class/devfreq/soc:qcom,l3-cpu0/governor",
    "/sys/class/devfreq/soc:qcom,l3-cpu6/governor",
    "/sys/class/devfreq/aa00000.qcom,vidc1:arm9_bus_ddr/governor"
    "/sys/class/devfreq/aa00000.qcom,vidc1:bus_cnoc/governor",
    "/sys/class/devfreq/aa00000.qcom,vidc1:venus_bus_ddr/governor",
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

void get_system_info() {
    char buf[1024];
    FILE *file;
    int first_core = -1, last_core = -1, core;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    size_t n_paths = sizeof(paths) / sizeof(paths[0]);
    FILE *fp;
    char path[1035];

    char cmd[512];
    DIR *d;
    struct dirent *dir;
    char dir_path[] = "/run/user";

    d = opendir(dir_path);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_DIR && strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
                snprintf(cmd, sizeof(cmd), "XDG_RUNTIME_DIR=%s/%s batman-helper wlrdisplay", dir_path, dir->d_name);
                break;
            }
        }
        closedir(d);
    } else {
        printf("Could not open directory %s\n", dir_path);
        return;
    }

    fp = popen(cmd, "r");
    if (fp == NULL) {
        printf("Failed to run command\n");
        return;
    }

    while (fgets(path, sizeof(path), fp) != NULL) {
        printf("screen status [enabled]: %s", path);
    }

    pclose(fp);

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

    int x86 = is_arch_x86();
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

    while(keep_going) {
        printf("\033[H\033[J");  // Clear the screen
        get_system_info();
        sleep(1);
    }

    return 0;
}
