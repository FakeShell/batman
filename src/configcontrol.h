/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2023 Erik Inkinen <erik.inkinen@erikinkinen.fi>
 * Copyright (C) 2025 Bardia Moshiri <bardia@furilabs.com>
 */

#ifndef CONFIG_CONTROL_H
#define CONFIG_CONTROL_H

#include <gtk/gtk.h>
#include <adwaita.h>

#define TEMP_FILE "/var/lib/batman/config.tmp"
#define CONFIG_FILE "/var/lib/batman/config"

typedef struct {
    gboolean offline;
    gboolean powersave;
    gboolean chargesave;
    gboolean bussave;
    gboolean gpusave;
    gboolean btsave;
    gboolean hybrissave;
    gboolean wifisave;
} Config;

Config read_config();
void update_config_value(const char *config_key, const char *config_value);
gboolean powersave_switch_state_set(GtkSwitch *sender, gboolean state, gpointer data);
gboolean offline_switch_state_set(GtkSwitch *sender, gboolean state, gpointer data);
gboolean gpusave_switch_state_set(GtkSwitch *sender, gboolean state, gpointer data);
gboolean chargesave_switch_state_set(GtkSwitch *sender, gboolean state, gpointer data);
gboolean bussave_switch_state_set(GtkSwitch *sender, gboolean state, gpointer data);
gboolean btsave_switch_state_set(GtkSwitch *sender, gboolean state, gpointer data);
gboolean hybrissave_switch_state_set(GtkSwitch *sender, gboolean state, gpointer data);
gboolean wifisave_switch_state_set(GtkSwitch *sender, gboolean state, gpointer data);

#endif /* CONFIG_CONTROL_H */
