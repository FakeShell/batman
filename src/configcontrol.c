#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#define CONFIG_FILE "/var/lib/batman/config"
#define TEMP_FILE "/var/lib/batman/config.tmp"

void update_config_value(const char* config_key, const char* config_value) {
    FILE *src, *dst;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int found = 0; // To check if key has been found

    src = fopen(CONFIG_FILE, "r");
    if (src == NULL) {
        perror("Failed to open file");
        return;
    }

    dst = fopen(TEMP_FILE, "w");
    if (dst == NULL) {
        perror("Failed to open temp file");
        fclose(src);
        return;
    }

    while ((read = getline(&line, &len, src)) != -1) {
        if (strstr(line, config_key) == line) {
            // This is the line to replace
            fprintf(dst, "%s=%s\n", config_key, config_value);
            found = 1; // Set flag to indicate key has been found
        } else {
            // This line remains unchanged
            fprintf(dst, "%s", line);
        }
    }

    // If key not found, add it
    if (!found) {
        fprintf(dst, "%s=%s\n", config_key, config_value);
    }

    free(line);
    fclose(src);
    fclose(dst);

    // Replace the original file with the modified one
    rename(TEMP_FILE, CONFIG_FILE);
}

void powersave_off(GtkWidget *widget, gpointer data) {
    update_config_value("POWERSAVE", "false");
}

void powersave_on(GtkWidget *widget, gpointer data) {
    update_config_value("POWERSAVE", "true");
}

void offline_off(GtkWidget *widget, gpointer data) {
    update_config_value("OFFLINE", "false");
}

void offline_on(GtkWidget *widget, gpointer data) {
    update_config_value("OFFLINE", "true");
}

void gpusave_off(GtkWidget *widget, gpointer data) {
    update_config_value("GPUSAVE", "false");
}

void gpusave_on(GtkWidget *widget, gpointer data) {
    update_config_value("GPUSAVE", "true");
}

void chargesave_off(GtkWidget *widget, gpointer data) {
    update_config_value("CHARGESAVE", "false");
}

void chargesave_on(GtkWidget *widget, gpointer data) {
    update_config_value("CHARGESAVE", "true");
}

void bussave_off(GtkWidget *widget, gpointer data) {
    update_config_value("BUSSAVE", "false");
}

void bussave_on(GtkWidget *widget, gpointer data) {
    update_config_value("BUSSAVE", "true");
}

void set_max_cpu_usage(GtkSpinButton *spin_button, gpointer user_data) {
    int max_cpu_usage = gtk_spin_button_get_value_as_int(spin_button);

    if (max_cpu_usage < 0 || max_cpu_usage > 100) {
        fprintf(stderr, "CPU usage must be between 0 and 100\n");
        max_cpu_usage = 0;  // Set default value
    }

    FILE *file = fopen(CONFIG_FILE, "r");
    if (file == NULL) {
        perror("Unable to open config file");
        exit(1);
    }

    char line[256];
    char config_data[1024] = "";  // Assuming config file is less than 1024 characters
    bool found = false;

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "MAX_CPU_USAGE=", 14) == 0) {
            sprintf(line, "MAX_CPU_USAGE=%d\n", max_cpu_usage);
            found = true;
        }
        strcat(config_data, line);
    }

    // If MAX_CPU_USAGE is not found in the file, add it.
    if (!found) {
        sprintf(line, "MAX_CPU_USAGE=%d\n", max_cpu_usage);
        strcat(config_data, line);
    }

    fclose(file);

    // Now write the modified config data back to the file
    file = fopen(CONFIG_FILE, "w");
    if (file == NULL) {
        perror("Unable to open config file");
        exit(1);
    }

    fprintf(file, "%s", config_data);

    fclose(file);
}
