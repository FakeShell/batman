#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <glob.h>
#include <glib.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/sysinfo.h>

#define FILE_PATH_CPU "/var/lib/batman/default_cpu_governor"
#define FILE_PATH_GPU "/var/lib/batman/default_gpu_governor"
#define CONFIG_FILE "/var/lib/batman/config"

void append_to_gstring(GString *string, char *format, ...) {
    va_list args;
    va_start(args, format);
    gchar *formatted_string = g_strdup_vprintf(format, args);
    g_string_append(string, formatted_string);
    g_string_append(string, "\n");
    g_free(formatted_string);
    va_end(args);
}

GString* display_info() {
    FILE *file;
    char *line = NULL;
    size_t len = 0;
    int num_files = 0;
    ssize_t read;
    GString *string = g_string_new(NULL);

    uid_t uid = getuid();
    append_to_gstring(string, "UID: %d\n", uid);

    file = fopen("/proc/cpuinfo", "r");
    if (file == NULL) {
        append_to_gstring(string, "CPU Information: unknown\n");
    } else {
        int first_core = -1, last_core = -1, core;
        while ((read = getline(&line, &len, file)) != -1) {
            if (sscanf(line, "processor : %d", &core) == 1) {
                if (first_core == -1) first_core = core;
                last_core = core;
            }
        }
        fclose(file);
        free(line);
        line = NULL;

        append_to_gstring(string, "First CPU core: %d\n", first_core);
        append_to_gstring(string, "Last CPU core: %d\n", last_core);
    }

    file = fopen(FILE_PATH_CPU, "r");
    if (file == NULL) {
        append_to_gstring(string, "Default CPU Governor: unknown\n");
    } else {
        if (getline(&line, &len, file) != -1) {
            append_to_gstring(string, "Default CPU Governor: %s", line);
        }
        fclose(file);
        free(line);
        line = NULL;
    }

    file = fopen(FILE_PATH_GPU, "r");
    if (file == NULL) {
        append_to_gstring(string, "Default GPU Governor: unknown\n");
    } else {
        if (getline(&line, &len, file) != -1) {
            append_to_gstring(string, "Default GPU Governor: %s", line);
        }
        fclose(file);
        free(line);
        line = NULL;
    }

    glob_t glob_result;

    if (glob("/sys/devices/system/cpu/cpufreq/policy*", GLOB_TILDE, NULL, &glob_result) == 0) {
        num_files = glob_result.gl_pathc;
        globfree(&glob_result);
    } else {
        printf("Error occurred while searching for files.\n");
    }

    append_to_gstring(string, "Available CPU policy groups: %d\n", num_files);

    char *first_pol = NULL;

    glob("/sys/devices/system/cpu/cpufreq/policy*", GLOB_TILDE, NULL, &glob_result);
    if (glob_result.gl_pathc > 0) {
        first_pol = strdup(basename(glob_result.gl_pathv[0]));  // duplicate the string
        append_to_gstring(string, "First CPU policy group: %s\n", first_pol);
    }
    globfree(&glob_result);

    if (first_pol != NULL) {
        char related_cpus_path[256];
        snprintf(related_cpus_path, sizeof(related_cpus_path), "/sys/devices/system/cpu/cpufreq/%s/related_cpus", first_pol);

        file = fopen(related_cpus_path, "r");
        if (file == NULL) {
            append_to_gstring(string, "CPU policy related CPUs: unknown\n");
        } else {
            char ch;
            // get the first character
            if (fscanf(file, "%c", &ch) == 1) {
                append_to_gstring(string, "First core of %s: %c\n", first_pol, ch);
            }

            // find the last character
            char last_char;
            while (fscanf(file, "%c", &ch) == 1) {
                // ignore newline characters
                if (ch != '\n') {
                    last_char = ch;
                }
            }

            append_to_gstring(string, "Last core of %s: %c\n", first_pol, last_char);

            fclose(file);
        }

        free(first_pol);
    }

    file = popen("systemctl is-active batman", "r");
    if (file == NULL) {
        append_to_gstring(string, "batman status: unknown\n");
    } else {
        if ((read = getline(&line, &len, file)) != -1) {
            append_to_gstring(string, "batman is %s", line);
        }
        pclose(file);
        free(line);
        line = NULL;
    }

    file = popen("systemctl show --property MainPID --value batman", "r");
    if (file == NULL) {
        append_to_gstring(string, "PID of batman: unknown\n");
    } else {
        if ((read = getline(&line, &len, file)) != -1) {
            append_to_gstring(string, "PID of batman: %s", line);
        }
        pclose(file);
        free(line);
        line = NULL;
    }

    // Get uptime
    struct sysinfo sys_info;
    if(sysinfo(&sys_info) != 0){
        printf("sysinfo failed!\n");
    } else {
        append_to_gstring(string, "Uptime: %ld minutes\n", sys_info.uptime / 60);
    }

    // Get screen status
    file = popen("batman-helper wlrdisplay", "r");
    if (file == NULL) {
        append_to_gstring(string, "Screen status: unknown\n");
    } else {
        if ((read = getline(&line, &len, file)) != -1) {
            append_to_gstring(string, "Screen status: %s", line);
        }
        pclose(file);
        free(line);
        line = NULL;
    }

    // Get charging status
    file = popen("batman-helper battery", "r");
    if (file == NULL) {
        append_to_gstring(string, "Charging status: unknown\n");
    } else {
        if ((read = getline(&line, &len, file)) != -1) {
            append_to_gstring(string, "Charging status: %s", line);
        }
        pclose(file);
        free(line);
        line = NULL;
    }

    // CPU Usage
    file = popen("batman-helper cpu", "r");
    if (file == NULL) {
        append_to_gstring(string, "CPU usage: unknown\n");
    } else {
        if ((read = getline(&line, &len, file)) != -1) {
            append_to_gstring(string, "CPU usage: %s", line);
        }
        pclose(file);
        free(line);
        line = NULL;
    }

    // Get Batman configuration
    file = fopen(CONFIG_FILE, "r");
    if (file != NULL) {
        char config_line[1024] = {0};

        while (fgets(config_line, sizeof(config_line), file)) {
            // Remove the newline character
            config_line[strcspn(config_line, "\n")] = 0;
            append_to_gstring(string, "%s\n", config_line);
        }
        fclose(file);
    }

    return string;
}
