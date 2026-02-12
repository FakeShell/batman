/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2026 Bardia Moshiri <bardia@furilabs.com>
 */

#include "bluetooth.h"
#include "utils.h"

static gboolean
bluetooth_find_adapter_path(BluetoothContext *bt)
{
    g_autoptr(GError) error = NULL;
    g_autoptr(GVariant) reply = NULL;
    g_autoptr(GVariantIter) iter = NULL;

    const char *obj_path = NULL;
    GVariant *ifaces = NULL;
    gboolean found = FALSE;

    if (!bt || !bt->bus)
        return FALSE;

    bt->adapter_path[0] = '\0';

    reply = g_dbus_connection_call_sync(
        bt->bus,
        "org.bluez",
        "/",
        "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects",
        NULL,
        G_VARIANT_TYPE("(a{oa{sa{sv}}})"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (!reply) {
        if (error)
            g_warning("GetManagedObjects failed: %s", error->message);
        return FALSE;
    }

    g_variant_get(reply, "(a{oa{sa{sv}}})", &iter);

    while (g_variant_iter_loop(iter, "{&o@a{sa{sv}}}", &obj_path, &ifaces)) {
        g_autoptr(GVariant) ifaces_local = ifaces;
        g_autoptr(GVariant) adapter_iface;
        ifaces = NULL;

        adapter_iface = g_variant_lookup_value(ifaces_local,
                                               "org.bluez.Adapter1",
                                               G_VARIANT_TYPE("a{sv}"));

        if (adapter_iface) {
            g_strlcpy(bt->adapter_path, obj_path, sizeof(bt->adapter_path));
            found = TRUE;
            break;
        }
    }

    return found;
}

static gboolean
bluetooth_get_adapter_powered(BluetoothContext *bt, gboolean *powered)
{
    g_autoptr(GError) error = NULL;
    g_autoptr(GVariant) reply = NULL;
    g_autoptr(GVariant) v = NULL;

    if (!bt || !powered || !bt->bus || bt->adapter_path[0] == '\0')
        return FALSE;

    *powered = FALSE;

    reply = g_dbus_connection_call_sync(
        bt->bus,
        "org.bluez",
        bt->adapter_path,
        "org.freedesktop.DBus.Properties",
        "Get",
        g_variant_new("(ss)", "org.bluez.Adapter1", "Powered"),
        G_VARIANT_TYPE("(v)"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (!reply) {
        if (error)
            g_debug("Adapter Powered Get failed: %s", error->message);
        return FALSE;
    }

    g_variant_get(reply, "(v)", &v);
    *powered = g_variant_get_boolean(v);

    return TRUE;
}

static void
bluetooth_set_adapter_powered(BluetoothContext *bt, gboolean powered)
{
    g_autoptr(GVariant) reply = NULL;
    g_autoptr(GError) error = NULL;

    if (!bt || !bt->bus || bt->adapter_path[0] == '\0')
        return;

    g_debug("bluetooth: setting adapter Powered=%d on %s",
            powered ? 1 : 0, bt->adapter_path);

    reply = g_dbus_connection_call_sync(
        bt->bus,
        "org.bluez",
        bt->adapter_path,
        "org.freedesktop.DBus.Properties",
        "Set",
        g_variant_new("(ssv)",
                      "org.bluez.Adapter1",
                      "Powered",
                      g_variant_new_boolean(powered)),
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (!reply) {
        if (error)
            g_debug("Adapter Powered Set failed: %s", error->message);
        return;
    }

    g_debug("bluetooth: adapter Powered set to %d successfully", powered ? 1 : 0);
}

gboolean
bluetooth_init(BluetoothContext *bt)
{
    g_autoptr(GError) error = NULL;
    gboolean powered = FALSE;

    if (!bt)
        return FALSE;

    memset(bt, 0, sizeof(*bt));

    bt->forced_off = FALSE;
    bt->adapter_initially_powered = FALSE;
    bt->ready = FALSE;

    bt->bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (!bt->bus) {
        if (error)
            g_warning("failed to connect to system bus: %s", error->message);
        return FALSE;
    }

    bt->ready = TRUE;

    if (!bluetooth_find_adapter_path(bt)) {
        g_debug("no bluetooth adapter found");
        return TRUE;
    }

    if (bluetooth_get_adapter_powered(bt, &powered))
        bt->adapter_initially_powered = powered;

    return TRUE;
}

void
bluetooth_cleanup(BluetoothContext *bt)
{
    if (!bt)
        return;

    if (bt->bus) {
        g_object_unref(bt->bus);
        bt->bus = NULL;
    }

    bt->adapter_path[0] = '\0';
    bt->forced_off = FALSE;
    bt->adapter_initially_powered = FALSE;
    bt->ready = FALSE;
}

gboolean
bluetooth_any_device_connected(BluetoothContext *bt)
{
    g_autoptr(GError) error = NULL;
    g_autoptr(GVariant) reply = NULL;
    g_autoptr(GVariantIter) iter = NULL;

    const char *obj_path = NULL;
    GVariant *ifaces = NULL;

    if (!bt || !bt->bus)
        return FALSE;

    reply = g_dbus_connection_call_sync(
        bt->bus,
        "org.bluez",
        "/",
        "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects",
        NULL,
        G_VARIANT_TYPE("(a{oa{sa{sv}}})"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (!reply) {
        if (error)
            g_debug("GetManagedObjects failed: %s", error->message);
        g_debug("bluetooth: failed to query devices (treating as not connected)");
        return FALSE;
    }

    g_variant_get(reply, "(a{oa{sa{sv}}})", &iter);

    while (g_variant_iter_loop(iter, "{&o@a{sa{sv}}}", &obj_path, &ifaces)) {
        g_autoptr(GVariant) ifaces_local = ifaces;
        ifaces = NULL;

        g_autoptr(GVariant) dev_props = g_variant_lookup_value(ifaces_local,
                                                               "org.bluez.Device1",
                                                               G_VARIANT_TYPE("a{sv}"));

        if (!dev_props)
            continue;

        gboolean connected = FALSE;

        g_autoptr(GVariant) connected_variant = g_variant_lookup_value(dev_props,
                                                                       "Connected",
                                                                       G_VARIANT_TYPE("b"));

        if (connected_variant)
            connected = g_variant_get_boolean(connected_variant);

        if (connected) {
            g_debug("bluetooth: found connected device at %s", obj_path);
            return TRUE;
        }
    }

    g_debug("bluetooth: no connected devices found");
    return FALSE;
}

void
bluetooth_apply_powersave(BluetoothContext *bt,
                          const BatmanConfig *cfg)
{
    gboolean powered = FALSE;

    if (!bt || !cfg)
        return;

    if (!cfg->bluetooth_powersave_enabled)
        return;

    if (!bt->bus || bt->adapter_path[0] == '\0')
        return;

    if (!bluetooth_get_adapter_powered(bt, &powered)) {
        g_debug("bluetooth: can't read adapter powered state");
        return;
    }

    if (!powered) {
        g_debug("bluetooth: adapter already powered off");
        return;
    }

    if (bluetooth_any_device_connected(bt)) {
        g_debug("bluetooth: device connected, not powering off adapter");
        return;
    }

    bluetooth_set_adapter_powered(bt, FALSE);
    bt->forced_off = TRUE;
    g_debug("bluetooth: adapter powered off successfully");
}

void
bluetooth_apply_default(BluetoothContext *bt,
                        const BatmanConfig *cfg)
{
    gboolean powered = FALSE;

    if (!bt || !cfg)
        return;

    if (!cfg->bluetooth_powersave_enabled)
        return;

    if (!bt->bus || bt->adapter_path[0] == '\0')
        return;

    if (!bt->forced_off) {
        g_debug("bluetooth: not forced off, skipping restore");
        return;
    }

    if (!bluetooth_get_adapter_powered(bt, &powered)) {
        g_debug("bluetooth: can't read adapter powered state");
        return;
    }

    if (!powered) {
        bluetooth_set_adapter_powered(bt, TRUE);
        g_debug("bluetooth: adapter powered on successfully");
    } else {
        g_debug("bluetooth: adapter already powered on");
    }

    bt->forced_off = FALSE;
}
