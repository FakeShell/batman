// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2024 Bardia Moshiri <fakeshell@bardia.tech>

#include <stdio.h>
#include "batman-waydroid.h"

#define WAYDROID_DBUS_NAME          "id.waydro.Container"
#define WAYDROID_DBUS_PATH          "/ContainerManager"
#define WAYDROID_DBUS_INTERFACE     "id.waydro.ContainerManager"

gchar* waydroid_get_state() {
    GDBusProxy *waydroid_proxy;
    GError *error = NULL;
    GVariant *result;
    gchar *state = NULL;

    waydroid_proxy = g_dbus_proxy_new_for_bus_sync(
        G_BUS_TYPE_SYSTEM,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        WAYDROID_DBUS_NAME,
        WAYDROID_DBUS_PATH,
        WAYDROID_DBUS_INTERFACE,
        NULL,
        &error
    );

    if (error) {
        g_print("Error creating proxy: %s\n", error->message);
        g_clear_error(&error);
        return NULL;
    }

    result = g_dbus_proxy_call_sync(
        waydroid_proxy,
        "GetSession",
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (error) {
        g_print("Error calling GetSession: %s\n", error->message);
        g_clear_error(&error);
    } else {
        GVariant *inner_dict;
        GVariantIter iter;
        gchar *key, *value;

        inner_dict = g_variant_get_child_value(result, 0);

        g_variant_iter_init(&iter, inner_dict);

        while (g_variant_iter_next(&iter, "{ss}", &key, &value)) {
            if (g_strcmp0(key, "state") == 0) {
                state = g_strdup(value);
                g_free(key);
                g_free(value);
                break;
            }
            g_free(key);
            g_free(value);
        }

        g_variant_unref(inner_dict);
        g_variant_unref(result);
    }

    g_object_unref(waydroid_proxy);

    return state;
}

void waydroid_freezer(gboolean freeze) {
    GDBusProxy *waydroid_proxy;
    GError *error = NULL;

    waydroid_proxy = g_dbus_proxy_new_for_bus_sync(
        G_BUS_TYPE_SYSTEM,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        WAYDROID_DBUS_NAME,
        WAYDROID_DBUS_PATH,
        WAYDROID_DBUS_INTERFACE,
        NULL,
        &error
    );

    if (error) {
        g_print("Error creating proxy: %s\n", error->message);
        g_clear_error(&error);
        return;
    }

    const char *method = freeze ? "Freeze" : "Unfreeze";

    g_dbus_proxy_call_sync(
        waydroid_proxy,
        method,
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (error) {
        g_debug("Error calling %s: %s\n", method, error->message);
        g_clear_error(&error);
    } else {
        g_debug("Successfully called %s on Waydroid.\n", method);
    }

    g_object_unref(waydroid_proxy);
}
