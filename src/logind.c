/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2026 Bardia Moshiri <bardia@furilabs.com>
 */

#include "logind.h"

#define LOGIN1_BUS_NAME "org.freedesktop.login1"
#define LOGIN1_OBJ_PATH "/org/freedesktop/login1"
#define LOGIN1_MGR_IFACE "org.freedesktop.login1.Manager"
#define LOGIN1_SESSION_IFACE "org.freedesktop.login1.Session"
#define DBUS_PROPS_IFACE "org.freedesktop.DBus.Properties"

#define SWITCH_RETRY_DELAY_SECONDS 1
#define SWITCH_RETRY_MAX_ATTEMPTS 30
#define TARGET_VTNR 7

static void
on_properties_changed(GDBusConnection *connection,
                      const gchar *sender_name,
                      const gchar *object_path,
                      const gchar *interface_name,
                      const gchar *signal_name,
                      GVariant *parameters,
                      gpointer user_data);

static void
logind_emit_screen_state(LogindMonitor *m, LogindScreenState new_state)
{
    if (!m || m->screen_state == new_state)
        return;

    m->screen_state = new_state;

    if (m->on_screen_changed)
        m->on_screen_changed(m->screen_state, m->user_data);
}

static void
logind_cancel_switch_retry(LogindMonitor *m)
{
    if (!m)
        return;

    if (m->switch_retry_source_id != 0) {
        g_source_remove(m->switch_retry_source_id);
        m->switch_retry_source_id = 0;
    }

    m->switch_retry_attempts = 0;
}

static void
logind_clear_current_session(LogindMonitor *m, gboolean reset_state)
{
    if (!m)
        return;

    if (m->connection && m->properties_changed_id > 0) {
        g_dbus_connection_signal_unsubscribe(m->connection, m->properties_changed_id);
        m->properties_changed_id = 0;
    }

    g_clear_object(&m->session_props_proxy);
    g_clear_pointer(&m->session_id, g_free);
    g_clear_pointer(&m->session_path, g_free);

    if (reset_state)
        m->screen_state = LOGIND_SCREEN_UNKNOWN;
}

static void
logind_reset_session(LogindMonitor *m)
{
    if (!m)
        return;

    logind_cancel_switch_retry(m);
    logind_clear_current_session(m, TRUE);
}

static gboolean
logind_read_boolean_property(GDBusProxy *proxy,
                             const char *iface,
                             const char *prop,
                             gboolean *out)
{
    g_autoptr(GError) error = NULL;
    g_autoptr(GVariant) result = NULL;
    g_autoptr(GVariant) val = NULL;

    if (out)
        *out = FALSE;

    if (!proxy)
        return FALSE;

    result = g_dbus_proxy_call_sync(proxy,
                                    "Get",
                                    g_variant_new("(ss)", iface, prop),
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (error)
        return FALSE;

    g_variant_get(result, "(v)", &val);

    if (!g_variant_is_of_type(val, G_VARIANT_TYPE_BOOLEAN))
        return FALSE;

    if (out)
        *out = g_variant_get_boolean(val);

    return TRUE;
}

static gboolean
logind_read_string_property_dup(GDBusProxy *proxy,
                                const char *iface,
                                const char *prop,
                                char **out)
{
    g_autoptr(GError) error = NULL;
    g_autoptr(GVariant) result = NULL;
    g_autoptr(GVariant) val = NULL;
    const char *tmp;

    if (out)
        *out = NULL;

    if (!proxy)
        return FALSE;

    result = g_dbus_proxy_call_sync(proxy,
                                    "Get",
                                    g_variant_new("(ss)", iface, prop),
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (error)
        return FALSE;

    g_variant_get(result, "(v)", &val);

    if (!g_variant_is_of_type(val, G_VARIANT_TYPE_STRING))
        return FALSE;

    tmp = g_variant_get_string(val, NULL);

    if (out)
        *out = g_strdup(tmp);

    return TRUE;
}

static gboolean
logind_read_uint32_property(GDBusProxy *proxy,
                            const char *iface,
                            const char *prop,
                            guint32 *out)
{
    g_autoptr(GError) error = NULL;
    g_autoptr(GVariant) result = NULL;
    g_autoptr(GVariant) val = NULL;

    if (out)
        *out = 0;

    if (!proxy)
        return FALSE;

    result = g_dbus_proxy_call_sync(proxy,
                                    "Get",
                                    g_variant_new("(ss)", iface, prop),
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if (error)
        return FALSE;

    g_variant_get(result, "(v)", &val);

    if (!g_variant_is_of_type(val, G_VARIANT_TYPE_UINT32))
        return FALSE;

    if (out)
        *out = g_variant_get_uint32(val);

    return TRUE;
}

static gboolean
logind_read_idle_hint(LogindMonitor *m, gboolean *idle_hint_out)
{
    return logind_read_boolean_property(m ? m->session_props_proxy : NULL,
                                        LOGIN1_SESSION_IFACE,
                                        "IdleHint",
                                        idle_hint_out);
}

static gboolean
logind_find_best_session(LogindMonitor *m,
                         char **sid_out,
                         char **spath_out)
{
    g_autoptr(GVariant) reply = NULL;
    g_autoptr(GVariantIter) iter = NULL;
    char *best_id = NULL;
    char *best_path = NULL;
    gboolean best_active = FALSE;

    if (sid_out)
        *sid_out = NULL;
    if (spath_out)
        *spath_out = NULL;

    if (!m || !m->manager_proxy)
        return FALSE;

    reply = g_dbus_proxy_call_sync(m->manager_proxy,
                                   "ListSessions",
                                   NULL,
                                   0,
                                   -1,
                                   NULL,
                                   NULL);
    if (!reply)
        return FALSE;

    g_variant_get(reply, "(a(susso))", &iter);

    while (TRUE) {
        char *id = NULL;
        char *user = NULL;
        char *seat = NULL;
        char *path = NULL;
        guint32 uid = 0;
        g_autoptr(GDBusProxy) props = NULL;
        gboolean active = FALSE;
        guint32 vtnr = 0;
        g_autofree char *class_name = NULL;

        if (!g_variant_iter_next(iter, "(susso)", &id, &uid, &user, &seat, &path))
            break;

        (void)uid;

        if (g_strcmp0(seat, "seat0") != 0)
            goto next;

        props = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                              0,
                                              NULL,
                                              LOGIN1_BUS_NAME,
                                              path,
                                              DBUS_PROPS_IFACE,
                                              NULL,
                                              NULL);
        if (!props)
            goto next;

        logind_read_boolean_property(props, LOGIN1_SESSION_IFACE, "Active", &active);
        logind_read_uint32_property(props, LOGIN1_SESSION_IFACE, "VTNr", &vtnr);
        logind_read_string_property_dup(props, LOGIN1_SESSION_IFACE, "Class", &class_name);

        g_debug("Candidate session id=%s path=%s seat=%s class=%s vtnr=%u active=%s",
                id ? id : "(null)",
                path ? path : "(null)",
                seat ? seat : "(null)",
                class_name ? class_name : "(null)",
                vtnr,
                active ? "true" : "false");

        if (g_strcmp0(class_name, "user") != 0 || vtnr != TARGET_VTNR)
            goto next;

        if (!best_id || (!best_active && active)) {
            g_free(best_id);
            g_free(best_path);
            best_id = g_strdup(id);
            best_path = g_strdup(path);
            best_active = active;
        }

next:
        g_free(id);
        g_free(user);
        g_free(seat);
        g_free(path);

        if (best_active)
            break;
    }

    if (!best_id || !best_path) {
        g_free(best_id);
        g_free(best_path);
        return FALSE;
    }

    if (sid_out)
        *sid_out = g_strdup(best_id);
    if (spath_out)
        *spath_out = g_strdup(best_path);

    g_free(best_id);
    g_free(best_path);
    return TRUE;
}

static gboolean
logind_attach_to_session(LogindMonitor *m,
                         const char *sid,
                         const char *spath)
{
    gboolean idle;

    if (!m || !sid || !spath || !m->connection)
        return FALSE;

    logind_clear_current_session(m, TRUE);

    m->session_id = g_strdup(sid);
    m->session_path = g_strdup(spath);

    m->session_props_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                           0,
                                                           NULL,
                                                           LOGIN1_BUS_NAME,
                                                           spath,
                                                           DBUS_PROPS_IFACE,
                                                           NULL,
                                                           NULL);

    if (!m->session_props_proxy) {
        logind_clear_current_session(m, TRUE);
        return FALSE;
    }

    m->properties_changed_id = g_dbus_connection_signal_subscribe(m->connection,
                                                                  LOGIN1_BUS_NAME,
                                                                  DBUS_PROPS_IFACE,
                                                                  "PropertiesChanged",
                                                                  spath,
                                                                  NULL,
                                                                  0,
                                                                  on_properties_changed,
                                                                  m,
                                                                  NULL);

    if (m->properties_changed_id == 0) {
        logind_clear_current_session(m, TRUE);
        return FALSE;
    }

    if (logind_read_idle_hint(m, &idle))
        logind_emit_screen_state(m, idle ? LOGIND_SCREEN_OFF : LOGIND_SCREEN_ON);

    g_debug("Attached to session id=%s path=%s",
            m->session_id ? m->session_id : "(null)",
            m->session_path ? m->session_path : "(null)");

    return TRUE;
}

static gboolean
logind_switch_retry_cb(gpointer user_data)
{
    LogindMonitor *m = user_data;
    g_autofree char *sid = NULL;
    g_autofree char *spath = NULL;

    if (!m)
        return G_SOURCE_REMOVE;

    m->switch_retry_attempts++;

    g_debug("Session reprobe attempt %u/%u",
            m->switch_retry_attempts,
            SWITCH_RETRY_MAX_ATTEMPTS);

    if (logind_find_best_session(m, &sid, &spath)) {
        if (!m->session_id || g_strcmp0(sid, m->session_id) != 0) {
            g_debug("Switching session: %s -> %s",
                    m->session_id ? m->session_id : "(none)",
                    sid ? sid : "(null)");

            if (logind_attach_to_session(m, sid, spath)) {
                m->switch_retry_source_id = 0;
                m->switch_retry_attempts = 0;
                return G_SOURCE_REMOVE;
            }
        } else {
            g_debug("Session reprobe found same session %s, retrying in %u seconds",
                    sid ? sid : "(null)",
                    SWITCH_RETRY_DELAY_SECONDS);
        }
    } else {
        g_debug("No suitable user session on seat0 VT%u found yet, retrying in %u seconds",
                TARGET_VTNR,
                SWITCH_RETRY_DELAY_SECONDS);
    }

    if (m->switch_retry_attempts >= SWITCH_RETRY_MAX_ATTEMPTS) {
        m->switch_retry_source_id = 0;
        return G_SOURCE_REMOVE;
    }

    return G_SOURCE_CONTINUE;
}

static void
logind_schedule_switch_retry(LogindMonitor *m)
{
    if (!m || m->switch_retry_source_id)
        return;

    m->switch_retry_attempts = 0;

    g_debug("Session switch detected, scheduling reprobe in %u seconds",
            SWITCH_RETRY_DELAY_SECONDS);

    m->switch_retry_source_id = g_timeout_add_seconds(SWITCH_RETRY_DELAY_SECONDS,
                                                      logind_switch_retry_cb,
                                                      m);
}

static void
on_properties_changed(GDBusConnection *connection,
                      const gchar *sender_name,
                      const gchar *object_path,
                      const gchar *interface_name,
                      const gchar *signal_name,
                      GVariant *parameters,
                      gpointer user_data)
{
    LogindMonitor *m = user_data;
    const gchar *changed_iface = NULL;
    GVariant *props = NULL;
    GVariantIter iter;
    const gchar *key = NULL;
    GVariant *val = NULL;

    (void)connection;
    (void)sender_name;
    (void)object_path;
    (void)interface_name;
    (void)signal_name;

    if (!m)
        return;

    g_variant_get(parameters, "(&s@a{sv}^a&s)", &changed_iface, &props, NULL);

    if (g_strcmp0(changed_iface, LOGIN1_SESSION_IFACE) != 0) {
        if (props)
            g_variant_unref(props);
        return;
    }

    g_variant_iter_init(&iter, props);

    while (g_variant_iter_next(&iter, "{&sv}", &key, &val)) {
        if (g_strcmp0(key, "IdleHint") == 0) {
            gboolean idle = g_variant_get_boolean(val);
            logind_emit_screen_state(m, idle ? LOGIND_SCREEN_OFF : LOGIND_SCREEN_ON);
        } else if (g_strcmp0(key, "Active") == 0) {
            gboolean active = g_variant_get_boolean(val);

            g_debug("Session %s Active changed to %s",
                    m->session_id ? m->session_id : "(none)",
                    active ? "true" : "false");

            if (!active)
                logind_schedule_switch_retry(m);
            else
                logind_cancel_switch_retry(m);
        }

        g_variant_unref(val);
    }

    g_variant_unref(props);
}

static void
on_manager_session_removed(GDBusConnection *connection,
                           const gchar *sender_name,
                           const gchar *object_path,
                           const gchar *interface_name,
                           const gchar *signal_name,
                           GVariant *parameters,
                           gpointer user_data)
{
    LogindMonitor *m = user_data;
    const gchar *sid = NULL;
    const gchar *spath = NULL;

    (void)connection;
    (void)sender_name;
    (void)object_path;
    (void)interface_name;
    (void)signal_name;

    if (!m)
        return;

    g_variant_get(parameters, "(&s&o)", &sid, &spath);

    g_debug("Manager reported SessionRemoved id=%s path=%s",
            sid ? sid : "(null)",
            spath ? spath : "(null)");

    if ((m->session_id && g_strcmp0(sid, m->session_id) == 0) ||
        (m->session_path && g_strcmp0(spath, m->session_path) == 0)) {
        g_debug("Tracked session disappeared: id=%s path=%s",
                m->session_id ? m->session_id : "(none)",
                m->session_path ? m->session_path : "(none)");

        logind_clear_current_session(m, TRUE);
        logind_schedule_switch_retry(m);
    }
}

static gboolean
logind_retry_setup(gpointer user_data)
{
    LogindMonitor *m = user_data;
    g_autofree char *sid = NULL;
    g_autofree char *spath = NULL;

    if (!m)
        return G_SOURCE_CONTINUE;

    if (!m->connection)
        m->connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);

    if (!m->connection)
        return G_SOURCE_CONTINUE;

    if (!m->manager_proxy)
        m->manager_proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
                                                         0,
                                                         NULL,
                                                         LOGIN1_BUS_NAME,
                                                         LOGIN1_OBJ_PATH,
                                                         LOGIN1_MGR_IFACE,
                                                         NULL,
                                                         NULL);

    if (!m->manager_proxy)
        return G_SOURCE_CONTINUE;

    if (m->manager_session_removed_id == 0)
        m->manager_session_removed_id = g_dbus_connection_signal_subscribe(m->connection,
                                                                           LOGIN1_BUS_NAME,
                                                                           LOGIN1_MGR_IFACE,
                                                                           "SessionRemoved",
                                                                           LOGIN1_OBJ_PATH,
                                                                           NULL,
                                                                           0,
                                                                           on_manager_session_removed,
                                                                           m,
                                                                           NULL);

    if (!logind_find_best_session(m, &sid, &spath))
        return G_SOURCE_CONTINUE;

    if (!logind_attach_to_session(m, sid, spath))
        return G_SOURCE_CONTINUE;

    m->retry_source_id = 0;
    return G_SOURCE_REMOVE;
}

LogindMonitor *
logind_monitor_new(void (*cb)(LogindScreenState, void *), void *user_data)
{
    LogindMonitor *m = g_new0(LogindMonitor, 1);

    m->on_screen_changed = cb;
    m->user_data = user_data;
    m->screen_state = LOGIND_SCREEN_UNKNOWN;

    m->retry_source_id = g_timeout_add_seconds(1, logind_retry_setup, m);

    return m;
}

void
logind_monitor_free(LogindMonitor *m)
{
    if (!m)
        return;

    if (m->retry_source_id)
        g_source_remove(m->retry_source_id);

    logind_reset_session(m);

    if (m->connection && m->manager_session_removed_id)
        g_dbus_connection_signal_unsubscribe(m->connection,
                                             m->manager_session_removed_id);

    g_clear_object(&m->manager_proxy);
    g_clear_object(&m->connection);

    g_free(m);
}

LogindScreenState
logind_monitor_get_screen_state(LogindMonitor *m)
{
    return m ? m->screen_state : LOGIND_SCREEN_UNKNOWN;
}
