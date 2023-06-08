#ifndef CONFIG_CONTROL_H
#define CONFIG_CONTROL_H

#include <gtk/gtk.h>

void update_config_value(const char* config_key, const char* config_value);

void powersave_off(GtkWidget *widget, gpointer data);
void powersave_on(GtkWidget *widget, gpointer data);
void offline_off(GtkWidget *widget, gpointer data);
void offline_on(GtkWidget *widget, gpointer data);
void gpusave_off(GtkWidget *widget, gpointer data);
void gpusave_on(GtkWidget *widget, gpointer data);
void chargesave_off(GtkWidget *widget, gpointer data);
void chargesave_on(GtkWidget *widget, gpointer data);
void bussave_off(GtkWidget *widget, gpointer data);
void bussave_on(GtkWidget *widget, gpointer data);
void set_max_cpu_usage(GtkSpinButton *spin_button, gpointer user_data);

#endif /* CONFIG_CONTROL_H */
