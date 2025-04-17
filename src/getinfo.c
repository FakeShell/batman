/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2025 Bardia Moshiri <bardia@furilabs.com>
 */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <glib.h>
#include <gio/gio.h>
#include "getinfo.h"

BatmanState bm_state;

/**
 * get_system_bus:
 * @error: (nullable): Return location for error
 *
 * Helper function to get system bus connection
 *
 * Returns: (transfer full): New GDBusConnection or NULL on error
 */
static GDBusConnection *
get_system_bus(GError **error)
{
    GDBusConnection *connection;

    connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, error);
    if (!connection) {
        g_prefix_error(error, "Failed connecting to D-Bus system bus: ");
        return NULL;
    }

    return connection;
}

/**
 * get_unit_property:
 * @property_name: The systemd unit property to query
 * @error: (nullable): Return location for error
 *
 * Gets a property value from the batman.service systemd unit.
 * Connects to the system bus and makes the appropriate DBus calls to systemd.
 *
 * Returns: (transfer full): A newly allocated string containing the property value,
 *          or NULL if an error occurred. The string should be freed with g_free().
 */
static gchar *
get_unit_property(const gchar *property_name, GError **error)
{
    g_autoptr(GDBusConnection) connection = NULL;
    g_autoptr(GVariant) unit_path_variant = NULL;
    g_autofree gchar *unit_path = NULL;
    g_autoptr(GVariant) property_value = NULL;
    g_autoptr(GVariant) property_variant = NULL;
    const gchar *value = NULL;

    connection = get_system_bus(error);
    if (!connection)
        return NULL;

    unit_path_variant = g_dbus_connection_call_sync(connection,
                                                    "org.freedesktop.systemd1",
                                                    "/org/freedesktop/systemd1",
                                                    "org.freedesktop.systemd1.Manager",
                                                    "GetUnit",
                                                    g_variant_new("(s)", "batman.service"),
                                                    G_VARIANT_TYPE("(o)"),
                                                    G_DBUS_CALL_FLAGS_NONE,
                                                    -1,
                                                    NULL,
                                                    error);

    if (!unit_path_variant) {
        g_debug("Failed to get unit path");
        return NULL;
    }

    g_variant_get(unit_path_variant, "(o)", &unit_path);

    property_value = g_dbus_connection_call_sync(connection,
                                                 "org.freedesktop.systemd1",
                                                 unit_path,
                                                 "org.freedesktop.DBus.Properties",
                                                 "Get",
                                                 g_variant_new("(ss)",
                                                               "org.freedesktop.systemd1.Unit",
                                                                property_name),
                                                 G_VARIANT_TYPE("(v)"),
                                                 G_DBUS_CALL_FLAGS_NONE,
                                                 -1,
                                                 NULL,
                                                 error);

    if (!property_value) {
        g_debug("Failed to get property %s", property_name);
        return NULL;
    }

    g_variant_get(property_value, "(v)", &property_variant);
    value = g_variant_get_string(property_variant, NULL);

    return g_strdup(value);
}

int
check_batman_active(void)
{
    g_autoptr(GError) error = NULL;
    g_autofree gchar *active_state = NULL;

    active_state = get_unit_property("ActiveState", &error);
    if (!active_state) {
        g_debug("Failed to get ActiveState: %s",
                error ? error->message : "unknown error");
        return -1;
    }

    bm_state.active = (g_strcmp0(active_state, "active") == 0 ||
                       g_strcmp0(active_state, "activating") == 0);
    return 0;
}

int
check_batman_enabled(void)
{
    g_autoptr(GError) error = NULL;
    g_autofree gchar *unit_file_state = NULL;

    unit_file_state = get_unit_property("UnitFileState", &error);
    if (!unit_file_state) {
        g_debug("Failed to get UnitFileState: %s",
                error ? error->message : "unknown error");
        return -1;
    }

    bm_state.enabled = (g_strcmp0(unit_file_state, "enabled") == 0 ||
                        g_strcmp0(unit_file_state, "static") == 0);

    return 0;
}

gboolean
start_batman_service(GError **error)
{
    g_autoptr(GDBusConnection) connection = NULL;
    g_autoptr(GVariant) start_result = NULL;

    connection = get_system_bus(error);
    if (!connection)
        return FALSE;

    start_result = g_dbus_connection_call_sync(connection,
                                               "org.freedesktop.systemd1",
                                               "/org/freedesktop/systemd1",
                                               "org.freedesktop.systemd1.Manager",
                                               "StartUnit",
                                               g_variant_new("(ss)",
                                                             "batman.service",
                                                             "replace"),
                                               G_VARIANT_TYPE("(o)"),
                                               G_DBUS_CALL_FLAGS_NONE,
                                               -1,
                                               NULL,
                                               error);

    if (!start_result) {
        g_prefix_error(error, "Failed to start batman service: ");
        return FALSE;
    }

    return TRUE;
}

gboolean
stop_batman_service(GError **error)
{
    g_autoptr(GDBusConnection) connection = NULL;
    g_autoptr(GVariant) stop_result = NULL;

    connection = get_system_bus(error);
    if (!connection)
        return FALSE;

    stop_result = g_dbus_connection_call_sync(connection,
                                              "org.freedesktop.systemd1",
                                              "/org/freedesktop/systemd1",
                                              "org.freedesktop.systemd1.Manager",
                                              "StopUnit",
                                              g_variant_new("(ss)",
                                                            "batman.service",
                                                            "replace"),
                                              G_VARIANT_TYPE("(o)"),
                                              G_DBUS_CALL_FLAGS_NONE,
                                              -1,
                                              NULL,
                                              error);

    if (!stop_result) {
        g_prefix_error(error, "Failed to stop batman service: ");
        return FALSE;
    }

    return TRUE;
}

gboolean
enable_batman_service(GError **error)
{
    g_autoptr(GDBusConnection) connection = NULL;
    g_autoptr(GVariant) enable_result = NULL;
    const char *service_list[] = { "batman.service", NULL };

    connection = get_system_bus(error);
    if (!connection)
        return FALSE;

    enable_result = g_dbus_connection_call_sync(connection,
                                                "org.freedesktop.systemd1",
                                                "/org/freedesktop/systemd1",
                                                "org.freedesktop.systemd1.Manager",
                                                "EnableUnitFiles",
                                                g_variant_new("(^asbb)",
                                                              service_list,
                                                              FALSE, FALSE),
                                                G_VARIANT_TYPE("(ba(sss))"),
                                                G_DBUS_CALL_FLAGS_NONE,
                                                -1,
                                                NULL,
                                                error);

    if (!enable_result) {
        g_prefix_error(error, "Failed to enable batman service: ");
        return FALSE;
    }

    return TRUE;
}

gboolean
disable_batman_service(GError **error)
{
    g_autoptr(GDBusConnection) connection = NULL;
    g_autoptr(GVariant) disable_result = NULL;
    const char *service_list[] = { "batman.service", NULL };

    connection = get_system_bus(error);
    if (!connection)
        return FALSE;

    disable_result = g_dbus_connection_call_sync(connection,
                                                 "org.freedesktop.systemd1",
                                                 "/org/freedesktop/systemd1",
                                                 "org.freedesktop.systemd1.Manager",
                                                 "DisableUnitFiles",
                                                 g_variant_new("(^asb)",
                                                               service_list,
                                                               FALSE),
                                                 G_VARIANT_TYPE("(a(sss))"),
                                                 G_DBUS_CALL_FLAGS_NONE,
                                                 -1,
                                                 NULL,
                                                 error);

    if (!disable_result) {
        g_prefix_error(error, "Failed to disable batman service: ");
        return FALSE;
    }

    return TRUE;
}
