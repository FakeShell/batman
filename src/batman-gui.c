// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2023 Bardia Moshiri <fakeshell@bardia.tech>
// Copyright (C) 2023 Erik Inkinen <erik.inkinen@erikinkinen.fi>

#include <gtk/gtk.h>
#include <adwaita.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "configcontrol.h"
#include "getinfo.h"

void about_activated(GSimpleAction *action, GVariant *parameter, gpointer app) {
    const char *developers[] = {
        "Bardia Moshiri <fakeshell@bardia.tech>",
        "Erik Inkinen <erik.inkinen@erikinkinen.fi>",
        NULL
    };

    const char *designers[] = {
        "Erik Inkinen <erik.inkinen@erikinkinen.fi>",
        NULL
    };

    adw_show_about_window(
        gtk_application_get_active_window(app),
        "application-name", "Batman GUI",
        "application-icon", "batman",
        "version", "1.42",
        "copyright", "© 2023 Bardia Moshiri, Erik Inkinen",
        "issue-url", "https://github.com/fakeshell/batman/issues/new",
        "license-type", GTK_LICENSE_GPL_2_0_ONLY,
        "developers", developers,
        "designers", designers,
        NULL);
}

void ctl_active_cb(GObject* src_ctl, GAsyncResult*, gpointer sender) {
    check_batman_active();
    gtk_switch_set_state(GTK_SWITCH(sender), bm_state.active);
    gtk_switch_set_active(GTK_SWITCH(sender), bm_state.active);

    g_object_unref(src_ctl);
}

void ctl_enabled_cb(GObject* src_ctl, GAsyncResult*, gpointer sender) {
    check_batman_enabled();
    gtk_switch_set_state(GTK_SWITCH(sender), bm_state.enabled);
    gtk_switch_set_active(GTK_SWITCH(sender), bm_state.enabled);

    g_object_unref(src_ctl);
}

gboolean service_active_switch_state_set(GtkSwitch* sender, gboolean state, gpointer) {
    printf("%d\n", state);
    if (state == bm_state.active) return FALSE;

    const gchar* ctl_argv[] = {
        "pkexec", "systemctl", (state) ? "start" : "stop", "batman", NULL
    };
    GSubprocess* ctl_proc = g_subprocess_newv(ctl_argv, G_SUBPROCESS_FLAGS_NONE, NULL);
    g_subprocess_communicate_async(ctl_proc, NULL, NULL, ctl_active_cb, sender);
    return TRUE;
}

gboolean service_enabled_switch_state_set(GtkSwitch* sender, gboolean state, gpointer) {
    printf("%d\n", state);
    if (state == bm_state.enabled) return FALSE;

    const gchar* ctl_argv[] = {
        "pkexec", "systemctl", (state) ? "enable" : "disable", "batman", NULL
    };
    GSubprocess* ctl_proc = g_subprocess_newv(ctl_argv, G_SUBPROCESS_FLAGS_NONE, NULL);
    g_subprocess_communicate_async(ctl_proc, NULL, NULL, ctl_enabled_cb, sender);
    return TRUE;
}

GActionEntry app_entries[] = {
    { "about", about_activated, NULL, NULL, NULL }
};

void activate(GtkApplication* app, gpointer user_data) {
    Config config = read_config();
    g_action_map_add_action_entries(G_ACTION_MAP (app),
        app_entries, G_N_ELEMENTS (app_entries), app);

    // main window
    GtkWidget *window = adw_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Batman");
    gtk_window_set_default_size(GTK_WINDOW(window), 500, 500);

    GtkWidget *wbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *hdr_bar = adw_header_bar_new();

    GtkWidget *menu_btn = gtk_menu_button_new();
    gtk_menu_button_set_primary(GTK_MENU_BUTTON(menu_btn), TRUE);
    gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(menu_btn), "open-menu-symbolic");

    GMenu *main_menu = g_menu_new();
    g_menu_append(main_menu, "About Batman", "app.about");

    gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(menu_btn), G_MENU_MODEL(main_menu));
    adw_header_bar_pack_end(ADW_HEADER_BAR(hdr_bar), menu_btn);
    gtk_box_append(GTK_BOX(wbox), hdr_bar);

    GtkWidget *scrolled = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scrolled, TRUE);

    GtkWidget *clamp = adw_clamp_new();
    gtk_widget_set_margin_top(clamp, 18);
    gtk_widget_set_margin_bottom(clamp, 32);
    gtk_widget_set_margin_start(clamp, 18);
    gtk_widget_set_margin_end(clamp, 18);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_set_spacing(GTK_BOX(vbox), 18);

    // Service management

    GtkWidget *common_list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(common_list_box), GTK_SELECTION_NONE);
    gtk_widget_add_css_class(common_list_box, "boxed-list");

    GtkWidget *service_active_action_row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(service_active_action_row), "Batman service");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(service_active_action_row), "Start/stop Batman daemon");

    GtkWidget *service_active_switch = gtk_switch_new();
    gtk_widget_set_valign(service_active_switch, GTK_ALIGN_CENTER);
    check_batman_active();
    gtk_switch_set_state(GTK_SWITCH(service_active_switch), bm_state.active);
    gtk_switch_set_active(GTK_SWITCH(service_active_switch), bm_state.active);
    g_signal_connect(service_active_switch, "state-set", G_CALLBACK(service_active_switch_state_set), NULL);

    adw_action_row_add_suffix(ADW_ACTION_ROW(service_active_action_row), service_active_switch);
    gtk_list_box_append(GTK_LIST_BOX(common_list_box), service_active_action_row);

    GtkWidget *service_enabled_action_row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(service_enabled_action_row), "Auto-start");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(service_enabled_action_row), "Start Batman daemon automatically on boot");

    GtkWidget *service_enabled_switch = gtk_switch_new();
    gtk_widget_set_valign(service_enabled_switch, GTK_ALIGN_CENTER);
    check_batman_enabled();
    gtk_switch_set_state(GTK_SWITCH(service_enabled_switch), bm_state.enabled);
    gtk_switch_set_active(GTK_SWITCH(service_enabled_switch), bm_state.enabled);
    g_signal_connect(service_enabled_switch, "state-set", G_CALLBACK(service_enabled_switch_state_set), NULL);

    adw_action_row_add_suffix(ADW_ACTION_ROW(service_enabled_action_row), service_enabled_switch);
    gtk_list_box_append(GTK_LIST_BOX(common_list_box), service_enabled_action_row);
    gtk_box_append(GTK_BOX(vbox), common_list_box);

    // Configuration

    GtkWidget *config_list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(config_list_box), GTK_SELECTION_NONE);
    gtk_widget_add_css_class(config_list_box, "boxed-list");

    // Config : Max CPU

    GtkWidget *max_cpu_entry_row = adw_entry_row_new();
    adw_entry_row_set_show_apply_button(ADW_ENTRY_ROW(max_cpu_entry_row), TRUE);
    adw_entry_row_set_input_purpose(ADW_ENTRY_ROW(max_cpu_entry_row), GTK_INPUT_PURPOSE_NUMBER);
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(max_cpu_entry_row), "CPU usage threshold to leave powersave");

    GString *max_cpu_str = g_string_new(NULL);
    g_string_printf(max_cpu_str, "%d", config.max_cpu_usage);
    printf("\"%s\", %d\n", max_cpu_str->str, config.max_cpu_usage);
    gtk_editable_set_text(GTK_EDITABLE(max_cpu_entry_row), max_cpu_str->str);
    g_string_free(max_cpu_str, TRUE);

    g_signal_connect(max_cpu_entry_row, "apply", G_CALLBACK(max_cpu_entry_apply), NULL);
    gtk_list_box_append(GTK_LIST_BOX(config_list_box), max_cpu_entry_row);

    // Config : Powersave

    GtkWidget *powersave_action_row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(powersave_action_row), "Powersave");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(powersave_action_row), "Allow batman to set the CPU into powersave mode when discharging");

    GtkWidget *powersave_switch = gtk_switch_new();
    gtk_widget_set_valign(powersave_switch, GTK_ALIGN_CENTER);
    gtk_switch_set_state(GTK_SWITCH(powersave_switch), config.powersave);
    gtk_switch_set_active(GTK_SWITCH(powersave_switch), config.powersave);
    g_signal_connect(powersave_switch, "state-set", G_CALLBACK(powersave_switch_state_set), NULL);

    adw_action_row_add_suffix(ADW_ACTION_ROW(powersave_action_row), powersave_switch);
    gtk_list_box_append(GTK_LIST_BOX(config_list_box), powersave_action_row);

    // Config : Charge Save

    GtkWidget *chargesave_action_row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(chargesave_action_row), "Charge save");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(chargesave_action_row), "Allow batman to set the CPU into powersave mode when charging");

    GtkWidget *chargesave_switch = gtk_switch_new();
    gtk_widget_set_valign(chargesave_switch, GTK_ALIGN_CENTER);
    gtk_switch_set_state(GTK_SWITCH(chargesave_switch), config.chargesave);
    gtk_switch_set_active(GTK_SWITCH(chargesave_switch), config.chargesave);
    g_signal_connect(chargesave_switch, "state-set", G_CALLBACK(chargesave_switch_state_set), NULL);

    adw_action_row_add_suffix(ADW_ACTION_ROW(chargesave_action_row), chargesave_switch);
    gtk_list_box_append(GTK_LIST_BOX(config_list_box), chargesave_action_row);

    gtk_box_append(GTK_BOX(vbox), config_list_box);

    // Config : Offline

    GtkWidget *offline_action_row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(offline_action_row), "Offline");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(offline_action_row), "Allow batman to turn off CPU cores");

    GtkWidget *offline_switch = gtk_switch_new();
    gtk_widget_set_valign(offline_switch, GTK_ALIGN_CENTER);
    gtk_switch_set_state(GTK_SWITCH(offline_switch), config.offline);
    gtk_switch_set_active(GTK_SWITCH(offline_switch), config.offline);
    g_signal_connect(offline_switch, "state-set", G_CALLBACK(offline_switch_state_set), NULL);

    adw_action_row_add_suffix(ADW_ACTION_ROW(offline_action_row), offline_switch);
    gtk_list_box_append(GTK_LIST_BOX(config_list_box), offline_action_row);

    // Config : GPU Save

    GtkWidget *gpusave_action_row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(gpusave_action_row), "GPU save");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(gpusave_action_row), "Allow batman to turn on the GPU powersave mode");

    GtkWidget *gpusave_switch = gtk_switch_new();
    gtk_widget_set_valign(gpusave_switch, GTK_ALIGN_CENTER);
    gtk_switch_set_state(GTK_SWITCH(gpusave_switch), config.gpusave);
    gtk_switch_set_active(GTK_SWITCH(gpusave_switch), config.gpusave);
    g_signal_connect(gpusave_switch, "state-set", G_CALLBACK(gpusave_switch_state_set), NULL);

    adw_action_row_add_suffix(ADW_ACTION_ROW(gpusave_action_row), gpusave_switch);
    gtk_list_box_append(GTK_LIST_BOX(config_list_box), gpusave_action_row);

    gtk_box_append(GTK_BOX(vbox), config_list_box);

    // Config : Bus Save

    GtkWidget *bussave_action_row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(bussave_action_row), "Bus save");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(bussave_action_row), "Allow batman to turn on powersave mode in devfreq bus nodes");

    GtkWidget *bussave_switch = gtk_switch_new();
    gtk_widget_set_valign(bussave_switch, GTK_ALIGN_CENTER);
    gtk_switch_set_state(GTK_SWITCH(bussave_switch), config.bussave);
    gtk_switch_set_active(GTK_SWITCH(bussave_switch), config.bussave);
    g_signal_connect(bussave_switch, "state-set", G_CALLBACK(bussave_switch_state_set), NULL);

    adw_action_row_add_suffix(ADW_ACTION_ROW(bussave_action_row), bussave_switch);
    gtk_list_box_append(GTK_LIST_BOX(config_list_box), bussave_action_row);

    // Config : BT Save

    GtkWidget *btsave_action_row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(btsave_action_row), "Bluetooth save");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(btsave_action_row), "Allow batman to turn off bluetooth according to device usage");

    GtkWidget *btsave_switch = gtk_switch_new();
    gtk_widget_set_valign(btsave_switch, GTK_ALIGN_CENTER);
    gtk_switch_set_state(GTK_SWITCH(btsave_switch), config.btsave);
    gtk_switch_set_active(GTK_SWITCH(btsave_switch), config.btsave);
    g_signal_connect(btsave_switch, "state-set", G_CALLBACK(btsave_switch_state_set), NULL);

    adw_action_row_add_suffix(ADW_ACTION_ROW(btsave_action_row), btsave_switch);
    gtk_list_box_append(GTK_LIST_BOX(config_list_box), btsave_action_row);

    // END : Config

    gtk_box_append(GTK_BOX(vbox), config_list_box);

    adw_clamp_set_child(ADW_CLAMP(clamp), vbox);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), clamp);
    gtk_box_append(GTK_BOX(wbox), scrolled);
    adw_application_window_set_content(ADW_APPLICATION_WINDOW(window), wbox);
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("batman-gui version: 1.42\n");
            return 0;
        }
    }

    app = gtk_application_new("org.droidian.batman-gui", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
