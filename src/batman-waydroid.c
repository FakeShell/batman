// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2025 Bardia Moshiri <fakeshell@bardia.tech>

#include "batman-waydroid.h"
#include <stdio.h>
#include <stdlib.h>

#define WAYDROID_DBUS_NAME          "id.waydro.Container"
#define WAYDROID_DBUS_PATH          "/ContainerManager"
#define WAYDROID_DBUS_INTERFACE     "id.waydro.ContainerManager"

gchar *
waydroid_get_state ()
{
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
        g_debug ("Error creating proxy: %s", error->message);
        g_clear_error (&error);
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
        g_debug ("Error calling GetSession: %s", error->message);
        g_clear_error (&error);
    } else {
        GVariant *inner_dict;
        GVariantIter iter;
        gchar *key, *value;

        inner_dict = g_variant_get_child_value (result, 0);

        g_variant_iter_init (&iter, inner_dict);

        while (g_variant_iter_next (&iter, "{ss}", &key, &value)) {
            if (g_strcmp0 (key, "state") == 0) {
                state = g_strdup (value);
                g_free (key);
                g_free (value);
                break;
            }
            g_free (key);
            g_free (value);
        }

        g_variant_unref (inner_dict);
        g_variant_unref (result);
    }

    g_object_unref (waydroid_proxy);

    return state;
}

void
waydroid_freezer (gboolean state)
{
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
        g_print ("Error creating proxy: %s", error->message);
        g_clear_error (&error);
        return;
    }

    const char *method = state ? "Freeze" : "Unfreeze";

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
        g_debug ("Error calling %s: %s", method, error->message);
        g_clear_error (&error);
    } else {
        g_debug ("Successfully called %s on Waydroid.", method);
    }

    g_object_unref (waydroid_proxy);
}

void
waydroid_screen_toggle ()
{
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
        g_print ("Error creating proxy: %s", error->message);
        g_clear_error (&error);
        return;
    }

    g_dbus_proxy_call_sync(
        waydroid_proxy,
        "Screen",
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (error) {
        g_debug ("Error calling Screen: %s", error->message);
        g_clear_error (&error);
    } else {
        g_debug ("Successfully called Screen on Waydroid.");
    }

    g_object_unref (waydroid_proxy);
}

gboolean
waydroid_screen_status ()
{
    GDBusProxy *waydroid_proxy;
    GError *error = NULL;
    GVariant *result;
    gboolean is_asleep = FALSE;

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
        g_print ("Error creating proxy: %s", error->message);
        g_clear_error (&error);
        g_object_unref (waydroid_proxy);
        return is_asleep;
    }

    result = g_dbus_proxy_call_sync(
        waydroid_proxy,
        "isAsleep",
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (error) {
        g_debug ("Error calling isAsleep on Waydroid: %s", error->message);
        g_clear_error (&error);
    } else {
        g_variant_get (result, "(b)", &is_asleep);
        g_variant_unref (result);
    }

    g_object_unref (waydroid_proxy);
    return is_asleep;
}

void
waydroid_setprop (const gchar* propname, const gchar* propvalue)
{
    GDBusProxy *waydroid_proxy;
    GError *error = NULL;
    GVariant *parameters;

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
        g_debug ("Error creating proxy: %s", error->message);
        g_clear_error (&error);
        return;
    }

    parameters = g_variant_new ("(ss)", propname, propvalue);

    g_dbus_proxy_call_sync(
        waydroid_proxy,
        "Setprop",
        parameters,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (error) {
        g_debug ("Error calling Setprop: %s", error->message);
        g_clear_error (&error);
    }

    g_object_unref (waydroid_proxy);
}

void
waydroid_screen (gboolean state)
{
    gchar *current_state = waydroid_get_state ();
    gboolean is_asleep = waydroid_screen_status ();

    if (current_state != NULL) {
        if (g_strcmp0 (current_state, "RUNNING") == 0) {
            g_debug ("Waydroid is currently running.");
            if ((state && is_asleep) || (!state && !is_asleep)) {
                g_debug ("Screen state transition: %s -> %s",
                         is_asleep ? "asleep" : "awake",
                         state ? "awake" : "asleep");
                if (!state) { // Screen going off
                    waydroid_setprop ("furios.screen_off", "true");
                    waydroid_screen_toggle ();
                } else { // Screen going on
                    waydroid_screen_toggle ();
                    waydroid_setprop ("furios.screen_off", "false");
                }
            }
        }
        g_free (current_state);
    } else {
        g_debug ("Failed to get Waydroid state.");
    }
}

gboolean
waydroid_app_open ()
{
    GDBusProxy *waydroid_proxy;
    GError *error = NULL;
    GVariant *result;
    gboolean app_open = FALSE;

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
        g_debug ("Error creating proxy: %s", error->message);
        g_clear_error (&error);
        g_object_unref (waydroid_proxy);
        return app_open;
    }

    result = g_dbus_proxy_call_sync(
        waydroid_proxy,
        "OpenAppPresent",
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error
    );

    if (error) {
        g_debug ("Error calling OpenAppPresent on Waydroid: %s", error->message);
        g_clear_error (&error);
    } else {
        g_variant_get (result, "(b)", &app_open);
        g_variant_unref (result);
    }

    g_object_unref (waydroid_proxy);
    return app_open;
}
