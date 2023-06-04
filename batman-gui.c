#include <gtk/gtk.h>
#include <glib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <glob.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <sys/sysinfo.h>

#define FILE_PATH_CPU "/var/lib/batman/default_cpu_governor"
#define FILE_PATH_GPU "/var/lib/batman/default_gpu_governor"
#define CONFIG_FILE "/var/lib/batman/config"
#define TEMP_FILE "/var/lib/batman/config.tmp"

typedef struct {
    gboolean offline;
    gboolean powersave;
    int max_cpu_usage;
    gboolean chargesave;
    gboolean bussave;
    gboolean gpusave;
} Config;

void button_restart_clicked(GtkWidget *widget, gpointer data) {
    system("pkexec systemctl restart batman");
}

void button_stop_clicked(GtkWidget *widget, gpointer data) {
    system("pkexec systemctl stop batman");
}

void button_start_clicked(GtkWidget *widget, gpointer data) {
    system("pkexec systemctl start batman");
}

void append_to_gstring(GString *string, char *format, ...) {
    va_list args;
    va_start(args, format);
    gchar *formatted_string = g_strdup_vprintf(format, args);
    g_string_append(string, formatted_string);
    g_string_append(string, "\n");
    g_free(formatted_string);
    va_end(args);
}

void switch_page(GtkWidget *widget, gpointer data) {
    GtkWidget *stack = GTK_WIDGET(data);
    const gchar *label = gtk_button_get_label(GTK_BUTTON(widget));

    if (strcmp(label, "Back") == 0) {
        gtk_stack_set_visible_child_name(GTK_STACK(stack), "main_page");
    } else if (strcmp(label, "Configuration") == 0) {
        gtk_stack_set_visible_child_name(GTK_STACK(stack), "config_page");
    }
}

Config read_config() {
    Config config;
    GKeyFile *keyfile = g_key_file_new();
    GError *error = NULL;

    if (!g_key_file_load_from_file(keyfile, CONFIG_FILE, G_KEY_FILE_NONE, &error)) {
        g_error("Error loading config file: %s\n", error->message);
    } else {
        config.offline = g_key_file_get_boolean(keyfile, "Settings", "OFFLINE", NULL);
        config.powersave = g_key_file_get_boolean(keyfile, "Settings", "POWERSAVE", NULL);
        config.max_cpu_usage = g_key_file_get_integer(keyfile, "Settngs", "MAX_CPU_USAGE", NULL);
        config.chargesave = g_key_file_get_boolean(keyfile, "Settings", "CHARGESAVE", NULL);
        config.bussave = g_key_file_get_boolean(keyfile, "Settings", "BUSSAVE", NULL);
        config.gpusave = g_key_file_get_boolean(keyfile, "Settings", "GPUSAVE", NULL);
    }

    g_key_file_free(keyfile);

    return config;
}

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

int get_max_cpu_usage() {
    FILE *file = fopen(CONFIG_FILE, "r");
    if (file == NULL) {
        perror("Unable to open config file");
        exit(1);
    }

    char line[256];
    int max_cpu_usage = -1;

    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "MAX_CPU_USAGE=%d", &max_cpu_usage);
    }

    fclose(file);

    return max_cpu_usage;
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

static void on_stack_switch_page(GObject *gobject, GParamSpec *pspec, gpointer user_data) {
    GtkStack *stack = GTK_STACK(gobject);
    GtkWidget *text_view = GTK_WIDGET(user_data);
    const gchar *name = gtk_stack_get_visible_child_name(stack);

    if (g_strcmp0(name, "main_page") == 0 || g_strcmp0(name, "config_page") == 0) {
        GString* info_string = display_info();
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
        gtk_text_buffer_set_text(buffer, info_string->str, -1);
        g_string_free(info_string, TRUE);
    }
}

void activate(GtkApplication* app, gpointer user_data) {
    Config config = read_config();
    GString* info_string = display_info();

    GtkWidget *window = gtk_application_window_new(app);
    GtkWidget *stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);

    // Initialize CSS
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
    "* {"
    "  font-size: 14px;"
    "}"
    "button {"
    "  border-radius: 5px;"
    "  border-width: 1px;"
    "}"
    "button:hover {"
    "  background: #E1E1E1;"
    "}", -1);

    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                               GTK_STYLE_PROVIDER(provider),
                                               GTK_STYLE_PROVIDER_PRIORITY_USER);

    // Main page
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_stack_add_named(GTK_STACK(stack), vbox, "main_page");

    // Create the text_view here so it's accessible later
    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_widget_set_focusable(text_view, FALSE);

    // Set the initial text in the text_view
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_set_text(buffer, info_string->str, -1);

    // Connect your stack switch page handler here
    g_signal_connect(stack, "notify::visible-child-name", G_CALLBACK(on_stack_switch_page), text_view);

    // Add the buttons to the main page
    GtkWidget *button_restart = gtk_button_new_with_label("Restart");
    g_signal_connect(button_restart, "clicked", G_CALLBACK(button_restart_clicked), NULL);
    gtk_box_append(GTK_BOX(vbox), button_restart);

    GtkWidget *button_stop = gtk_button_new_with_label("Stop");
    g_signal_connect(button_stop, "clicked", G_CALLBACK(button_stop_clicked), NULL);
    gtk_box_append(GTK_BOX(vbox), button_stop);

    GtkWidget *button_start = gtk_button_new_with_label("Start");
    g_signal_connect(button_start, "clicked", G_CALLBACK(button_start_clicked), NULL);
    gtk_box_append(GTK_BOX(vbox), button_start);

    GtkWidget *button_config = gtk_button_new_with_label("Configuration");
    g_signal_connect(button_config, "clicked", G_CALLBACK(switch_page), stack);
    gtk_box_append(GTK_BOX(vbox), button_config);

    // Configuration page
    GtkWidget *config_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_margin_top(config_page, 10);
    gtk_widget_set_margin_bottom(config_page, 10);
    gtk_widget_set_margin_start(config_page, 10);
    gtk_widget_set_margin_end(config_page, 10);

    gtk_stack_add_named(GTK_STACK(stack), config_page, "config_page");

    // Add a back button to the configuration page
    GtkWidget *back_button = gtk_button_new_with_label("Back");
    g_signal_connect(back_button, "clicked", G_CALLBACK(switch_page), stack);
    gtk_box_append(GTK_BOX(config_page), back_button);

    // Add configuration buttons
    if(config.powersave) {
        GtkWidget *powersave_off_button = gtk_button_new_with_label("Powersave off");
        g_signal_connect(powersave_off_button, "clicked", G_CALLBACK(powersave_off), NULL);
        gtk_box_append(GTK_BOX(config_page), powersave_off_button);
    } else {
        GtkWidget *powersave_on_button = gtk_button_new_with_label("Powersave on");
        g_signal_connect(powersave_on_button, "clicked", G_CALLBACK(powersave_on), NULL);
        gtk_box_append(GTK_BOX(config_page), powersave_on_button);
    }

    if(config.offline) {
        GtkWidget *offline_off_button = gtk_button_new_with_label("Offline off");
        g_signal_connect(offline_off_button, "clicked", G_CALLBACK(offline_off), NULL);
        gtk_box_append(GTK_BOX(config_page), offline_off_button);
    } else {
        GtkWidget *offline_on_button = gtk_button_new_with_label("Offline on");
        g_signal_connect(offline_on_button, "clicked", G_CALLBACK(offline_on), NULL);
        gtk_box_append(GTK_BOX(config_page), offline_on_button);
    }

    if(config.gpusave) {
        GtkWidget *gpusave_off_button = gtk_button_new_with_label("GPUsave off");
        g_signal_connect(gpusave_off_button, "clicked", G_CALLBACK(gpusave_off), NULL);
        gtk_box_append(GTK_BOX(config_page), gpusave_off_button);
    } else {
        GtkWidget *gpusave_on_button = gtk_button_new_with_label("GPUsave on");
        g_signal_connect(gpusave_on_button, "clicked", G_CALLBACK(gpusave_on), NULL);
        gtk_box_append(GTK_BOX(config_page), gpusave_on_button);
    }

    if(config.chargesave) {
        GtkWidget *chargesave_off_button = gtk_button_new_with_label("Chargesave off");
        g_signal_connect(chargesave_off_button, "clicked", G_CALLBACK(chargesave_off), NULL);
        gtk_box_append(GTK_BOX(config_page), chargesave_off_button);
    } else {
        GtkWidget *chargesave_on_button = gtk_button_new_with_label("Chargesave on");
        g_signal_connect(chargesave_on_button, "clicked", G_CALLBACK(chargesave_on), NULL);
        gtk_box_append(GTK_BOX(config_page), chargesave_on_button);
    }

    if(config.bussave) {
        GtkWidget *bussave_off_button = gtk_button_new_with_label("Bussave off");
        g_signal_connect(bussave_off_button, "clicked", G_CALLBACK(bussave_off), NULL);
        gtk_box_append(GTK_BOX(config_page), bussave_off_button);
    } else {
        GtkWidget *bussave_on_button = gtk_button_new_with_label("Bussave on");
        g_signal_connect(bussave_on_button, "clicked", G_CALLBACK(bussave_on), NULL);
        gtk_box_append(GTK_BOX(config_page), bussave_on_button);
    }

    // MAX_CPU_USAGE Entry
    GtkWidget *max_cpu_usage_label = gtk_label_new("MAX_CPU_USAGE: ");
    gtk_box_append(GTK_BOX(config_page), max_cpu_usage_label);

    GtkWidget *max_cpu_usage_spin = gtk_spin_button_new_with_range(0, 100, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(max_cpu_usage_spin), config.max_cpu_usage);
    g_signal_connect(max_cpu_usage_spin, "value-changed", G_CALLBACK(set_max_cpu_usage), NULL);
    gtk_box_append(GTK_BOX(config_page), max_cpu_usage_spin);

    GtkWidget *scrollable = gtk_scrolled_window_new();

    gtk_window_set_title(GTK_WINDOW(window), "Batman GUI");
    gtk_window_set_default_size(GTK_WINDOW(window), 500, 500);

    gtk_scrolled_window_set_propagate_natural_height(GTK_SCROLLED_WINDOW(scrollable), TRUE);
    gtk_scrolled_window_set_propagate_natural_width(GTK_SCROLLED_WINDOW(scrollable), TRUE);

    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(text_view), 10);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(text_view), 10);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text_view), 10);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(text_view), 10);

    gtk_text_view_set_monospace(GTK_TEXT_VIEW(text_view), TRUE);

    gtk_box_append(GTK_BOX(vbox), scrollable);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrollable), text_view);

    gtk_window_set_child(GTK_WINDOW(window), stack);
    gtk_stack_set_visible_child_name(GTK_STACK(stack), "main_page");
    gtk_window_present(GTK_WINDOW(window));

    g_string_free(info_string, TRUE);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("batman-gui version: 1.38\n");
            return 0;
        }
    }

    app = gtk_application_new("tech.bardia.batman-gui", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
