/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2026 Bardia Moshiri <bardia@furilabs.com>
 */

#include "logind.h"

#define LOGIN1_BUS_NAME "org.freedesktop.login1"
#define LOGIN1_OBJ_PATH "/org/freedesktop/login1"
#define LOGIN1_MGR_IFACE "org.freedesktop.login1.Manager"
#define DBUS_PROPS_IFACE "org.freedesktop.DBus.Properties"

static void
logind_reset_session(LogindMonitor *m)
{
    if (m->connection != NULL) {
        if (m->properties_changed_id > 0) {
            g_dbus_connection_signal_unsubscribe(m->connection, m->properties_changed_id);
            m->properties_changed_id = 0;
        }
    }

    if (m->session_props_proxy != NULL) {
        g_object_unref(m->session_props_proxy);
        m->session_props_proxy = NULL;
    }

    if (m->session_id != NULL) {
        g_free(m->session_id);
        m->session_id = NULL;
    }

    m->screen_state = LOGIND_SCREEN_UNKNOWN;
}

static char *
logind_get_session_id_tty7(LogindMonitor *m)
{
    g_autoptr(GError) error = NULL;
    g_autoptr(GVariant) reply = NULL;
    g_autoptr(GVariantIter) iter = NULL;

    char *session_id = NULL;
    guint32 uid = 0;
    char *username = NULL;
    char *seat = NULL;
    char *path = NULL;

    if (m->manager_proxy == NULL)
        return NULL;

    reply = g_dbus_proxy_call_sync(
        m->manager_proxy,
        "ListSessions",
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (error != NULL) {
        g_warning("ListSessions failed: %s", error->message);
        return NULL;
    }

    g_variant_get(reply, "(a(susso))", &iter);

    while (g_variant_iter_next(iter, "(susso)", &session_id, &uid, &username, &seat, &path)) {
        g_autoptr(GError) perr = NULL;
        g_autoptr(GDBusProxy) props = NULL;
        g_autoptr(GVariant) tty_prop = NULL;
        g_autoptr(GVariant) tty_variant = NULL;
        const gchar *tty = NULL;

        props = g_dbus_proxy_new_for_bus_sync(
            G_BUS_TYPE_SYSTEM,
            G_DBUS_PROXY_FLAGS_NONE,
            NULL,
            LOGIN1_BUS_NAME,
            path,
            DBUS_PROPS_IFACE,
            NULL,
            &perr
        );

        if (perr != NULL) {
            g_free(session_id);
            g_free(username);
            g_free(seat);
            g_free(path);
            continue;
        }

        tty_prop = g_dbus_proxy_call_sync(
            props,
            "Get",
            g_variant_new("(ss)", "org.freedesktop.login1.Session", "TTY"),
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            NULL,
            &perr
        );

        if (perr != NULL) {
            g_free(session_id);
            g_free(username);
            g_free(seat);
            g_free(path);
            continue;
        }

        g_variant_get(tty_prop, "(v)", &tty_variant);
        tty = g_variant_get_string(tty_variant, NULL);

        if (g_strcmp0(tty, "tty7") == 0) {
            char *found = g_strdup(session_id);
            g_free(session_id);
            g_free(username);
            g_free(seat);
            g_free(path);
            return found;
        }

        g_free(session_id);
        g_free(username);
        g_free(seat);
        g_free(path);
    }

    return NULL;
}

static gboolean
logind_read_idle_hint(LogindMonitor *m, gboolean *idle_hint_out)
{
    g_autoptr(GError) error = NULL;
    g_autoptr(GVariant) result = NULL;
    g_autoptr(GVariant) value_variant = NULL;

    if (idle_hint_out != NULL)
        *idle_hint_out = FALSE;
    if (m->session_props_proxy == NULL)
        return FALSE;

    result = g_dbus_proxy_call_sync(
        m->session_props_proxy,
        "Get",
        g_variant_new("(ss)", "org.freedesktop.login1.Session", "IdleHint"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (error != NULL) {
        g_warning("Failed to get IdleHint: %s", error->message);
        return FALSE;
    }

    g_variant_get(result, "(v)", &value_variant);

    if (idle_hint_out != NULL)
        *idle_hint_out = (g_variant_get_boolean(value_variant) != FALSE) ? TRUE : FALSE;

    return TRUE;
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
    LogindMonitor *m;
    const gchar *changed_interface;
    GVariant *changed_properties;
    g_autofree const gchar **invalidated_properties = NULL;
    GVariantIter iter;
    const gchar *key;
    GVariant *value;

    (void)connection;
    (void)sender_name;
    (void)object_path;
    (void)interface_name;
    (void)signal_name;

    m = (LogindMonitor *)user_data;
    if (m == NULL)
        return;

    g_variant_get(parameters, "(&s@a{sv}^a&s)",
                  &changed_interface,
                  &changed_properties,
                  &invalidated_properties);

    if (g_strcmp0(changed_interface, "org.freedesktop.login1.Session") != 0) {
        g_variant_unref(changed_properties);
        return;
    }

    g_variant_iter_init(&iter, changed_properties);

    while (g_variant_iter_next(&iter, "{&sv}", &key, &value)) {
        if (g_strcmp0(key, "IdleHint") == 0) {
            gboolean idle_hint = g_variant_get_boolean(value);
            LogindScreenState new_state;

            if (idle_hint)
                new_state = LOGIND_SCREEN_OFF;
            else
                new_state = LOGIND_SCREEN_ON;

            if (new_state != m->screen_state) {
                m->screen_state = new_state;

                if (m->on_screen_changed != NULL)
                    m->on_screen_changed(m->screen_state, m->user_data);
            }
        } else if (g_strcmp0(key, "Active") == 0) {
            gboolean active = g_variant_get_boolean(value);

            if (!active) {
                g_autofree char *new_id = NULL;

                /* Re-probe tty7 when session becomes inactive */
                sleep(10);

                while (TRUE) {
                    new_id = logind_get_session_id_tty7(m);
                    if (new_id == NULL) {
                        sleep(1);
                        continue;
                    }

                    if (m->session_id == NULL)
                        break;
                    if (g_strcmp0(new_id, m->session_id) != 0)
                        break;

                    g_free(new_id);
                    new_id = NULL;
                    sleep(1);
                }

                if (new_id != NULL) {
                    g_debug("Switching session: %s -> %s", m->session_id, new_id);

                    logind_reset_session(m);
                    m->session_id = g_strdup(new_id);

                    g_autofree char *session_path = g_strdup_printf("/org/freedesktop/login1/session/%s", m->session_id);

                    m->session_props_proxy = g_dbus_proxy_new_for_bus_sync(
                        G_BUS_TYPE_SYSTEM,
                        G_DBUS_PROXY_FLAGS_NONE,
                        NULL,
                        LOGIN1_BUS_NAME,
                        session_path,
                        DBUS_PROPS_IFACE,
                        NULL,
                        NULL
                    );

                    if (m->session_props_proxy != NULL) {
                        m->properties_changed_id = g_dbus_connection_signal_subscribe(
                            m->connection,
                            LOGIN1_BUS_NAME,
                            DBUS_PROPS_IFACE,
                            "PropertiesChanged",
                            session_path,
                            NULL,
                            G_DBUS_SIGNAL_FLAGS_NONE,
                            on_properties_changed,
                            m,
                            NULL
                        );

                        gboolean idle_hint_now;
                        if (logind_read_idle_hint(m, &idle_hint_now)) {
                            m->screen_state = idle_hint_now ? LOGIND_SCREEN_OFF : LOGIND_SCREEN_ON;
                            if (m->on_screen_changed != NULL)
                                m->on_screen_changed(m->screen_state, m->user_data);
                        }
                    }
                }
            }
        }

        g_variant_unref(value);
    }

    g_variant_unref(changed_properties);
}

static gboolean
logind_retry_setup(gpointer user_data)
{
    LogindMonitor *m;
    g_autoptr(GError) error = NULL;
    g_autofree char *sid = NULL;

    m = (LogindMonitor *)user_data;
    if (m == NULL)
        return G_SOURCE_CONTINUE;

    if (m->connection == NULL) {
        m->connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
        if (error != NULL) {
            g_warning("Could not connect to system bus (logind), retrying: %s", error->message);
            return G_SOURCE_CONTINUE;
        }
    }

    if (m->manager_proxy == NULL) {
        m->manager_proxy = g_dbus_proxy_new_for_bus_sync(
            G_BUS_TYPE_SYSTEM,
            G_DBUS_PROXY_FLAGS_NONE,
            NULL,
            LOGIN1_BUS_NAME,
            LOGIN1_OBJ_PATH,
            LOGIN1_MGR_IFACE,
            NULL,
            &error
        );

        if (error != NULL) {
            g_warning("Login1 manager not available, retrying: %s", error->message);
            if (m->manager_proxy != NULL) {
                g_object_unref(m->manager_proxy);
                m->manager_proxy = NULL;
            }
            return G_SOURCE_CONTINUE;
        }
    }

    sid = logind_get_session_id_tty7(m);
    if (sid == NULL) {
        g_debug("No tty7 session yet, retrying...");
        return G_SOURCE_CONTINUE;
    }

    m->session_id = g_strdup(sid);

    g_autofree char *session_path = g_strdup_printf("/org/freedesktop/login1/session/%s", m->session_id);

    m->session_props_proxy = g_dbus_proxy_new_for_bus_sync(
        G_BUS_TYPE_SYSTEM,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        LOGIN1_BUS_NAME,
        session_path,
        DBUS_PROPS_IFACE,
        NULL,
        &error
    );

    if (error != NULL) {
        g_warning("Failed to create session props proxy, retrying: %s", error->message);
        logind_reset_session(m);
        return G_SOURCE_CONTINUE;
    }

    m->properties_changed_id = g_dbus_connection_signal_subscribe(
        m->connection,
        LOGIN1_BUS_NAME,
        DBUS_PROPS_IFACE,
        "PropertiesChanged",
        session_path,
        NULL,
        G_DBUS_SIGNAL_FLAGS_NONE,
        on_properties_changed,
        m,
        NULL
    );

    gboolean idle_hint;
    if (logind_read_idle_hint(m, &idle_hint))
        m->screen_state = idle_hint ? LOGIND_SCREEN_OFF : LOGIND_SCREEN_ON;
    else
        m->screen_state = LOGIND_SCREEN_UNKNOWN;

    if (m->on_screen_changed != NULL && m->screen_state != LOGIND_SCREEN_UNKNOWN)
        m->on_screen_changed(m->screen_state, m->user_data);

    m->retry_source_id = 0;
    return G_SOURCE_REMOVE;
}

LogindMonitor *
logind_monitor_new(void (*cb)(LogindScreenState state, void *user_data), void *user_data)
{
    LogindMonitor *m;

    m = g_new0(LogindMonitor, 1);
    m->on_screen_changed = cb;
    m->user_data = user_data;
    m->screen_state = LOGIND_SCREEN_UNKNOWN;

    m->retry_source_id = g_timeout_add_seconds(1, logind_retry_setup, m);

    return m;
}

void
logind_monitor_free(LogindMonitor *m)
{
    if (m == NULL)
        return;

    if (m->retry_source_id != 0) {
        g_source_remove(m->retry_source_id);
        m->retry_source_id = 0;
    }

    logind_reset_session(m);

    if (m->manager_proxy != NULL) {
        g_object_unref(m->manager_proxy);
        m->manager_proxy = NULL;
    }

    if (m->connection != NULL) {
        g_object_unref(m->connection);
        m->connection = NULL;
    }

    g_free(m);
}

LogindScreenState
logind_monitor_get_screen_state(LogindMonitor *m)
{
    if (m == NULL)
        return LOGIND_SCREEN_UNKNOWN;

    return m->screen_state;
}
