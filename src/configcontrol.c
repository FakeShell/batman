/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2023 Erik Inkinen <erik.inkinen@erikinkinen.fi>
 * Copyright (C) 2025 Bardia Moshiri <bardia@furilabs.com>
 */

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "configcontrol.h"

Config
read_config()
{
    Config config;
    GKeyFile *keyfile = g_key_file_new();
    GError *error = NULL;

    if (!g_key_file_load_from_file(keyfile, CONFIG_FILE, G_KEY_FILE_NONE, &error)) {
        g_debug("Error loading config file: %s", error->message);
    } else {
        config.offline = g_key_file_get_boolean(keyfile, "Settings", "OFFLINE", NULL);
        config.powersave = g_key_file_get_boolean(keyfile, "Settings", "POWERSAVE", NULL);
        config.chargesave = g_key_file_get_boolean(keyfile, "Settings", "CHARGESAVE", NULL);
        config.bussave = g_key_file_get_boolean(keyfile, "Settings", "BUSSAVE", NULL);
        config.gpusave = g_key_file_get_boolean(keyfile, "Settings", "GPUSAVE", NULL);
        config.btsave = g_key_file_get_boolean(keyfile, "Settings", "BTSAVE", NULL);
        config.hybrissave = g_key_file_get_boolean(keyfile, "Settings", "HYBRIS", NULL);
        config.wifisave = g_key_file_get_boolean(keyfile, "Settings", "WIFI", NULL);
    }

    g_key_file_free(keyfile);

    return config;
}

void
update_config_value(const char *config_key, const char *config_value)
{
    FILE *src, *dst;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int found = 0;

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
            fprintf(dst, "%s=%s\n", config_key, config_value);
            found = 1;
        } else {
            fprintf(dst, "%s", line);
        }
    }

    if (!found)
        fprintf(dst, "%s=%s\n", config_key, config_value);

    free(line);
    fclose(src);
    fclose(dst);

    rename(TEMP_FILE, CONFIG_FILE);
}

gboolean
powersave_switch_state_set(GtkSwitch *, gboolean state, gpointer)
{
    update_config_value("POWERSAVE", state ? "true" : "false");
}

gboolean
offline_switch_state_set(GtkSwitch *, gboolean state, gpointer)
{
    update_config_value("OFFLINE", state ? "true" : "false");
}

gboolean
gpusave_switch_state_set(GtkSwitch *, gboolean state, gpointer)
{
    update_config_value("GPUSAVE", state ? "true" : "false");
}

gboolean
chargesave_switch_state_set(GtkSwitch *, gboolean state, gpointer)
{
    update_config_value("CHARGESAVE", state ? "true" : "false");
}

gboolean
bussave_switch_state_set(GtkSwitch *, gboolean state, gpointer)
{
    update_config_value("BUSSAVE", state ? "true" : "false");
}

gboolean
btsave_switch_state_set(GtkSwitch *, gboolean state, gpointer)
{
    update_config_value("BTSAVE", state ? "true" : "false");
}

gboolean
hybrissave_switch_state_set(GtkSwitch *, gboolean state, gpointer)
{
    update_config_value("HYBRIS", state ? "true" : "false");
}

gboolean
wifisave_switch_state_set(GtkSwitch *, gboolean state, gpointer)
{
    update_config_value("WIFI", state ? "true" : "false");
}
