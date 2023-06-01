#include <gtk/gtk.h>
#include <glib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <glob.h>
#include <string.h>
#include <libgen.h>
#include <sys/sysinfo.h>

#define FILE_PATH_CPU "/var/lib/batman/default_cpu_governor"
#define FILE_PATH_GPU "/var/lib/batman/default_gpu_governor"

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
    gtk_stack_set_visible_child_name(GTK_STACK(stack), "settings");
}

void powersave_off(GtkWidget *widget, gpointer data) {
    system("sed -i 's/POWERSAVE.*/POWERSAVE=false/g' /var/lib/batman/config");
}

void powersave_on(GtkWidget *widget, gpointer data) {
    system("sed -i 's/POWERSAVE.*/POWERSAVE=true/g' /var/lib/batman/config");
}

void offline_off(GtkWidget *widget, gpointer data) {
    system("sed -i 's/OFFLINE.*/OFFLINE=false/g' /var/lib/batman/config");
}

void offline_on(GtkWidget *widget, gpointer data) {
    system("sed -i 's/OFFLINE.*/OFFLINE=true/g' /var/lib/batman/config");
}

void gpusave_off(GtkWidget *widget, gpointer data) {
    system("sed -i 's/GPUSAVE.*/GPUSAVE=false/g' /var/lib/batman/config");
}

void gpusave_on(GtkWidget *widget, gpointer data) {
    system("sed -i 's/GPUSAVE.*/GPUSAVE=true/g' /var/lib/batman/config");
}

void chargesave_off(GtkWidget *widget, gpointer data) {
    system("sed -i 's/CHARGESAVE.*/CHARGESAVE=false/g' /var/lib/batman/config");
}

void chargesave_on(GtkWidget *widget, gpointer data) {
    system("sed -i 's/CHARGESAVE.*/CHARGESAVE=true/g' /var/lib/batman/config");
}

void bussave_off(GtkWidget *widget, gpointer data) {
    system("sed -i 's/BUSSAVE.*/BUSSAVE=false/g' /var/lib/batman/config");
}

void bussave_on(GtkWidget *widget, gpointer data) {
    system("sed -i 's/BUSSAVE.*/BUSSAVE=true/g' /var/lib/batman/config");
}

void display_config_page(GtkWidget *widget, gpointer data) {
    GtkApplication *app = (GtkApplication *)data;

    GtkWidget *config_window = gtk_application_window_new(app);
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    GtkWidget *powersave_off_button = gtk_button_new_with_label("Powersave off");
    g_signal_connect(powersave_off_button, "clicked", G_CALLBACK(powersave_off), NULL);
    gtk_box_append(GTK_BOX(vbox), powersave_off_button);

    GtkWidget *powersave_on_button = gtk_button_new_with_label("Powersave on");
    g_signal_connect(powersave_on_button, "clicked", G_CALLBACK(powersave_on), NULL);
    gtk_box_append(GTK_BOX(vbox), powersave_on_button);

    GtkWidget *offline_off_button = gtk_button_new_with_label("Offline off");
    g_signal_connect(offline_off_button, "clicked", G_CALLBACK(offline_off), NULL);
    gtk_box_append(GTK_BOX(vbox), offline_off_button);

    GtkWidget *offline_on_button = gtk_button_new_with_label("Offline on");
    g_signal_connect(offline_on_button, "clicked", G_CALLBACK(offline_on), NULL);
    gtk_box_append(GTK_BOX(vbox), offline_on_button);

    GtkWidget *gpusave_off_button = gtk_button_new_with_label("GPUsave off");
    g_signal_connect(gpusave_off_button, "clicked", G_CALLBACK(gpusave_off), NULL);
    gtk_box_append(GTK_BOX(vbox), gpusave_off_button);

    GtkWidget *gpusave_on_button = gtk_button_new_with_label("GPUsave on");
    g_signal_connect(gpusave_on_button, "clicked", G_CALLBACK(gpusave_on), NULL);
    gtk_box_append(GTK_BOX(vbox), gpusave_on_button);

    GtkWidget *chargesave_off_button = gtk_button_new_with_label("Chargesave off");
    g_signal_connect(chargesave_off_button, "clicked", G_CALLBACK(chargesave_off), NULL);
    gtk_box_append(GTK_BOX(vbox), chargesave_off_button);

    GtkWidget *chargesave_on_button = gtk_button_new_with_label("Chargesave on");
    g_signal_connect(chargesave_on_button, "clicked", G_CALLBACK(chargesave_on), NULL);
    gtk_box_append(GTK_BOX(vbox), chargesave_on_button);

    GtkWidget *bussave_off_button = gtk_button_new_with_label("Bussave off");
    g_signal_connect(bussave_off_button, "clicked", G_CALLBACK(bussave_off), NULL);
    gtk_box_append(GTK_BOX(vbox), bussave_off_button);

    GtkWidget *bussave_on_button = gtk_button_new_with_label("Bussave on");
    g_signal_connect(bussave_on_button, "clicked", G_CALLBACK(bussave_on), NULL);
    gtk_box_append(GTK_BOX(vbox), bussave_on_button);

    gtk_window_set_child(GTK_WINDOW(config_window), vbox);

    gtk_window_set_title(GTK_WINDOW(config_window), "Configuration");
    gtk_window_set_default_size(GTK_WINDOW(config_window), 200, 200);
    gtk_widget_show(config_window);
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
        printf("Could not open file /proc/cpuinfo");
        return NULL;
    }

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

    file = fopen(FILE_PATH_CPU, "r");
    if (file == NULL) {
        printf("Could not open file %s", FILE_PATH_CPU);
        return NULL;
    }

    if (getline(&line, &len, file) != -1) {
        append_to_gstring(string, "Default CPU Governor: %s", line);
    }
    fclose(file);
    free(line);
    line = NULL;

    file = fopen(FILE_PATH_GPU, "r");
    if (file == NULL) {
        printf("Could not open file %s", FILE_PATH_GPU);
        return NULL;
    }

    if (getline(&line, &len, file) != -1) {
        append_to_gstring(string, "Default GPU Governor: %s", line);
    }
    fclose(file);
    free(line);
    line = NULL;

    glob_t glob_result;

    if (glob("/sys/devices/system/cpu/cpufreq/*policy*", GLOB_TILDE, NULL, &glob_result) == 0) {
        num_files = glob_result.gl_pathc;
        globfree(&glob_result);
    } else {
        printf("Error occurred while searching for files.\n");
    }

    append_to_gstring(string, "Available CPU policy groups: %d\n", num_files);

    char *first_pol = NULL;

    glob("/sys/devices/system/cpu/cpufreq/*policy*", GLOB_TILDE, NULL, &glob_result);
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
            printf("Could not open file %s", related_cpus_path);
            free(first_pol);
            return NULL;
        }

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

    // free the duplicated string
    free(first_pol);

    append_to_gstring(string, "", line);
    file = popen("systemctl is-active batman", "r");
    if (file == NULL) {
        printf("Could not get the status of batman");
        return NULL;
    }
    if ((read = getline(&line, &len, file)) != -1) {
        append_to_gstring(string, "batman is %s", line);
    }
    pclose(file);
    free(line);
    line = NULL;

    file = popen("systemctl show --property MainPID --value batman", "r");
    if (file == NULL) {
        printf("Could not get the status of batman");
        return NULL;
    }
    if ((read = getline(&line, &len, file)) != -1) {
        append_to_gstring(string, "PID of batman: %s", line);
    }
    pclose(file);
    free(line);
    line = NULL;

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
        printf("Could not run batman-helper wlrdisplay");
        return NULL;
    }
    if ((read = getline(&line, &len, file)) != -1) {
        append_to_gstring(string, "Screen status: %s", line);
    }
    pclose(file);
    free(line);
    line = NULL;

    // Get charging status
    file = popen("batman-helper battery", "r");
    if (file == NULL) {
        printf("Could not run batman-helper battery");
        return NULL;
    }
    if ((read = getline(&line, &len, file)) != -1) {
        append_to_gstring(string, "Charging status: %s", line);
    }
    pclose(file);
    free(line);
    line = NULL;

    // CPU Usage
    file = popen("batman-helper cpu", "r");
    if (file == NULL) {
        printf("Could not run batman-helper cpu");
        return NULL;
    }
    if ((read = getline(&line, &len, file)) != -1) {
        append_to_gstring(string, "CPU usage: %s", line);
    }
    pclose(file);
    free(line);
    line = NULL;

    file = popen("systemctl is-active batman", "r");
    if (file != NULL) {
        if ((read = getline(&line, &len, file)) != -1) {
            line[strcspn(line, "\n")] = 0;
            append_to_gstring(string, "Batman is: %s", line);
        }
        pclose(file);
        free(line);
        line = NULL;
    }

    // Get Batman configuration
    file = fopen("/var/lib/batman/config", "r");
    if (file == NULL) {
        printf("Could not open file /var/lib/batman/config");
        return NULL;
    }

    append_to_gstring(string, "\nBatman Configuration: \n");

    char batman_config_line[256];
    while(fgets(batman_config_line, sizeof(batman_config_line), file)) {
        append_to_gstring(string, "%s", batman_config_line);
    }
    fclose(file);

    for (int i = 0; i < 2; i++) {
        if (string->len > 0 && string->str[string->len - 1] == '\n') {
            g_string_truncate(string, string->len - 1);
        }
    }

    return string;
}

void activate(GtkApplication* app, gpointer user_data) {
    GString* info_string = display_info();

    GtkWidget *window = gtk_application_window_new(app);
    GtkWidget *stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);

    // Main page
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_stack_add_named(GTK_STACK(stack), vbox, "main_page");

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
    g_signal_connect(button_config, "clicked", G_CALLBACK(display_config_page), app);
    gtk_box_append(GTK_BOX(vbox), button_config);

    // Add the display_info() text view below the buttons
    GtkWidget *scrollable = gtk_scrolled_window_new();
    GtkWidget *text_view = gtk_text_view_new();

    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_set_text(buffer, info_string->str, -1);

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

    app = gtk_application_new("batman.gui", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
