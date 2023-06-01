#include <gtk/gtk.h>
#include <glib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <glob.h>
#include <string.h>
#include <sys/sysinfo.h>

#define FILE_PATH_CPU "/var/lib/batman/default_cpu_governor"
#define FILE_PATH_GPU "/var/lib/batman/default_gpu_governor"

void append_to_gstring(GString *string, char *format, ...) {
    va_list args;
    va_start(args, format);
    gchar *formatted_string = g_strdup_vprintf(format, args);
    g_string_append(string, formatted_string);
    g_string_append(string, "\n");
    g_free(formatted_string);
    va_end(args);
}

void display_info(GtkApplication* app, gpointer user_data) {
    FILE *file;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    GString *string = g_string_new(NULL);

    uid_t uid = getuid();
    append_to_gstring(string, "Current UID: %d", uid);

    file = fopen("/proc/cpuinfo", "r");
    if (file == NULL) {
        printf("Could not open file /proc/cpuinfo");
        return;
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

    append_to_gstring(string, "First CPU core: %d", first_core);
    append_to_gstring(string, "Last CPU core: %d", last_core);

    file = fopen(FILE_PATH_CPU, "r");
    if (file == NULL) {
        printf("Could not open file %s", FILE_PATH_CPU);
        return;
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
        return;
    }

    if (getline(&line, &len, file) != -1) {
        append_to_gstring(string, "Default GPU Governor: %s", line);
    }
    fclose(file);
    free(line);
    line = NULL;

    glob_t glob_result;
    glob("/sys/devices/system/cpu/cpufreq/*policy*", GLOB_TILDE, NULL, &glob_result);
    if (glob_result.gl_pathc > 0) {
        append_to_gstring(string, "First CPU policy group %s", glob_result.gl_pathv[0]);
    }
    globfree(&glob_result);

    // Get uptime
    struct sysinfo sys_info;
    if(sysinfo(&sys_info) != 0){
        printf("sysinfo failed!\n");
    } else {
        append_to_gstring(string, "Uptime: %ld minutes", sys_info.uptime / 60);
    }

    // Get charging status
    file = popen("batman-helper battery", "r");
    if (file == NULL) {
        printf("Could not run batman-helper battery");
        return;
    }
    if ((read = getline(&line, &len, file)) != -1) {
        append_to_gstring(string, "Charging status: %s", line);
    }
    pclose(file);
    free(line);
    line = NULL;

    file = popen("pgrep batman", "r");
    if (file == NULL) {
        printf("Could not run pgrep batman");
        return;
    }
    if ((read = getline(&line, &len, file)) != -1) {
        append_to_gstring(string, "PID of batman: %s", line);
    }
    pclose(file);
    free(line);
    line = NULL;

    // CPU Usage
    file = popen("batman-helper cpu", "r");
    if (file == NULL) {
        printf("Could not run batman-helper cpu");
        return;
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
            line[strcspn(line, "\n")] = 0; // Remove newline if present
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
        return;
    }

    append_to_gstring(string, "\nBatman Configuration: \n");

    char batman_config_line[256];
    while(fgets(batman_config_line, sizeof(batman_config_line), file)) {
        append_to_gstring(string, "%s", batman_config_line);
    }
    fclose(file);

    GtkWidget *window = gtk_application_window_new(app);
    GtkWidget *scrollable = gtk_scrolled_window_new();
    GtkWidget *text_view = gtk_text_view_new();

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_set_text(buffer, string->str, -1);

    gtk_window_set_title(GTK_WINDOW(window), "batman-gui");
    gtk_window_set_default_size(GTK_WINDOW(window), 500, 500);

    gtk_window_set_child(GTK_WINDOW(window), scrollable);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrollable), text_view);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("batman.gui", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(display_info), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
