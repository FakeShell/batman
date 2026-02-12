/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2026 Bardia Moshiri <bardia@furilabs.com>
 */

#include "config.h"
#include "utils.h"

#include <stdio.h>

#define ETC_CONFIG_PATH "/etc/batman/config"
#define VAR_CONFIG_PATH "/var/lib/batman/config"

static gboolean
config_parse_bool(const char *v, gboolean fallback)
{
    if (v == NULL)
        return fallback;
    if (g_ascii_strcasecmp(v, "true") == 0)
        return TRUE;
    if (g_ascii_strcasecmp(v, "yes") == 0)
        return TRUE;
    if (g_ascii_strcasecmp(v, "on") == 0)
        return TRUE;
    if (strcmp(v, "1") == 0)
        return TRUE;
    if (g_ascii_strcasecmp(v, "false") == 0)
        return FALSE;
    if (g_ascii_strcasecmp(v, "no") == 0)
        return FALSE;
    if (g_ascii_strcasecmp(v, "off") == 0)
        return FALSE;
    if (strcmp(v, "0") == 0)
        return FALSE;

    return fallback;
}

static void
config_strip_quotes(char **val)
{
    size_t n;

    if (val == NULL)
        return;
    if (*val == NULL)
        return;

    n = strlen(*val);

    if (n >= 2) {
        if (((*val)[0] == '"' && (*val)[n - 1] == '"') ||
            ((*val)[0] == '\'' && (*val)[n - 1] == '\'')) {
            (*val)[n - 1] = '\0';
            (*val)++;
        }
    }
}

void
config_set_defaults(BatmanConfig *cfg)
{
    if (cfg == NULL)
        return;

    memset(cfg, 0, sizeof(*cfg));

    cfg->offline_enabled = TRUE;
    cfg->powersave_enabled = TRUE;
    cfg->chargesave_enabled = TRUE;
    cfg->gpu_powersave_enabled = TRUE;
    cfg->bus_powersave_enabled = TRUE;
    cfg->bluetooth_powersave_enabled = TRUE;
    cfg->binder_enabled = FALSE;
    cfg->wifi_enabled = FALSE;

    cfg->devfreq_gpu_path[0] = '\0';
    cfg->config_path[0] = '\0';

    cfg->monitor_etc = NULL;
    cfg->monitor_var = NULL;
    cfg->reload_source_id = 0;
    cfg->changed_cb = NULL;
    cfg->changed_cb_userdata = NULL;
}

void
config_load(BatmanConfig *cfg)
{
    const char *paths[] = { ETC_CONFIG_PATH, VAR_CONFIG_PATH };
    const char *chosen;
    FILE *f;
    char line[4096];
    gboolean first_line;

    if (cfg == NULL)
        return;

    chosen = NULL;

    if (exists(paths[0]))
        chosen = paths[0];
    else if (exists(paths[1]))
        chosen = paths[1];

    if (chosen == NULL) {
        g_info("config file does not exist. using default values");
        cfg->config_path[0] = '\0';
        return;
    }

    g_strlcpy(cfg->config_path, chosen, sizeof(cfg->config_path));
    g_debug("Parsing %s", cfg->config_path);

    f = fopen(cfg->config_path, "r");
    if (f == NULL) {
        g_warning("failed to open config %s", cfg->config_path);
        return;
    }

    first_line = TRUE;

    while (fgets(line, sizeof(line), f) != NULL) {
        char *s;
        char *eq;
        char *key;
        char *val;

        if (first_line) {
            first_line = FALSE;
            continue;
        }

        s = g_strstrip(line);
        if (s[0] == '\0')
            continue;
        if (s[0] == '#')
            continue;

        eq = strchr(s, '=');
        if (eq == NULL)
            continue;

        *eq = '\0';
        key = g_strstrip(s);
        val = g_strstrip(eq + 1);

        config_strip_quotes(&val);

        if (strcmp(key, "OFFLINE") == 0)
            cfg->offline_enabled = config_parse_bool(val, cfg->offline_enabled);
        else if (strcmp(key, "POWERSAVE") == 0)
            cfg->powersave_enabled = config_parse_bool(val, cfg->powersave_enabled);
        else if (strcmp(key, "CHARGESAVE") == 0)
            cfg->chargesave_enabled = config_parse_bool(val, cfg->chargesave_enabled);
        else if (strcmp(key, "GPUSAVE") == 0)
            cfg->gpu_powersave_enabled = config_parse_bool(val, cfg->gpu_powersave_enabled);
        else if (strcmp(key, "BUSSAVE") == 0)
            cfg->bus_powersave_enabled = config_parse_bool(val, cfg->bus_powersave_enabled);
        else if (strcmp(key, "BTSAVE") == 0)
            cfg->bluetooth_powersave_enabled = config_parse_bool(val, cfg->bluetooth_powersave_enabled);
        else if (strcmp(key, "BINDER") == 0 || strcmp(key, "HYBRIS") == 0)
            cfg->binder_enabled = config_parse_bool(val, cfg->binder_enabled);
        else if (strcmp(key, "WIFI") == 0)
            cfg->wifi_enabled = config_parse_bool(val, cfg->wifi_enabled);
        else if (strcmp(key, "DEVFREQ_GPU_PATH") == 0)
            g_strlcpy(cfg->devfreq_gpu_path, val, sizeof(cfg->devfreq_gpu_path));
    }

    fclose(f);

    g_debug("config loaded:");
    g_debug("  OFFLINE=%d", cfg->offline_enabled);
    g_debug("  POWERSAVE=%d", cfg->powersave_enabled);
    g_debug("  CHARGESAVE=%d", cfg->chargesave_enabled);
    g_debug("  GPUSAVE=%d", cfg->gpu_powersave_enabled);
    g_debug("  BUSSAVE=%d", cfg->bus_powersave_enabled);
    g_debug("  BTSAVE=%d", cfg->bluetooth_powersave_enabled);
    g_debug("  BINDER=%d", cfg->binder_enabled);
    g_debug("  WIFI=%d", cfg->wifi_enabled);
    g_debug("  DEVFREQ_GPU_PATH=%s", cfg->devfreq_gpu_path[0] ? cfg->devfreq_gpu_path : "(unset)");
}

static gboolean
config_reload_timeout_cb(gpointer userdata)
{
    BatmanConfig *cfg = (BatmanConfig *)userdata;
    if (!cfg)
        return G_SOURCE_REMOVE;

    cfg->reload_source_id = 0;

    config_load(cfg);

    if (cfg->changed_cb)
        cfg->changed_cb((const BatmanConfig *)cfg, cfg->changed_cb_userdata);

    return G_SOURCE_REMOVE;
}

static void
config_schedule_reload(BatmanConfig *cfg)
{
    if (!cfg)
        return;

    /* debounce bursty events (atomic save, rename, temp files) */
    if (cfg->reload_source_id != 0)
        return;

    cfg->reload_source_id = g_timeout_add(250, config_reload_timeout_cb, cfg);
}

static void
on_config_file_changed(GFileMonitor      *monitor,
                       GFile             *file,
                       GFile             *other_file,
                       GFileMonitorEvent  event_type,
                       gpointer           user_data)
{
    (void)monitor;
    (void)file;
    (void)other_file;

    BatmanConfig *cfg = (BatmanConfig *)user_data;
    if (!cfg)
        return;

    switch (event_type) {
    case G_FILE_MONITOR_EVENT_CHANGED:
    case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
    case G_FILE_MONITOR_EVENT_CREATED:
    case G_FILE_MONITOR_EVENT_DELETED:
    case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED:
    case G_FILE_MONITOR_EVENT_MOVED:
    case G_FILE_MONITOR_EVENT_RENAMED:
        config_schedule_reload(cfg);
        break;
    default:
        break;
    }
}

static GFileMonitor *
start_file_monitor(const char *path, BatmanConfig *cfg)
{
    g_autoptr(GError) error = NULL;

    GFile *f = g_file_new_for_path(path);
    if (!f)
        return NULL;

    GFileMonitor *m = g_file_monitor_file(f,
                                          G_FILE_MONITOR_NONE,
                                          NULL,
                                          &error);
    g_object_unref(f);

    if (!m) {
        if (error)
            g_debug("config: failed to monitor %s: %s", path, error->message);
        return NULL;
    }

    g_signal_connect(m, "changed", G_CALLBACK(on_config_file_changed), cfg);
    return m;
}

gboolean
config_monitor_start(BatmanConfig *cfg,
                     BatmanConfigChangedCb cb,
                     void *userdata)
{
    if (!cfg)
        return FALSE;

    config_monitor_stop(cfg);

    cfg->changed_cb = cb;
    cfg->changed_cb_userdata = userdata;

    cfg->monitor_etc = start_file_monitor(ETC_CONFIG_PATH, cfg);
    cfg->monitor_var = start_file_monitor(VAR_CONFIG_PATH, cfg);

    if (!cfg->monitor_etc && !cfg->monitor_var) {
        cfg->changed_cb = NULL;
        cfg->changed_cb_userdata = NULL;
        return FALSE;
    }

    return TRUE;
}

void
config_monitor_stop(BatmanConfig *cfg)
{
    if (!cfg)
        return;

    if (cfg->reload_source_id != 0) {
        g_source_remove(cfg->reload_source_id);
        cfg->reload_source_id = 0;
    }

    if (cfg->monitor_etc) {
        g_object_unref(cfg->monitor_etc);
        cfg->monitor_etc = NULL;
    }

    if (cfg->monitor_var) {
        g_object_unref(cfg->monitor_var);
        cfg->monitor_var = NULL;
    }

    cfg->changed_cb = NULL;
    cfg->changed_cb_userdata = NULL;
}
