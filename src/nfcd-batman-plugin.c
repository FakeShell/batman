/*
 * Copyright (C) 2020-2022 Jolla Ltd.
 * Copyright (C) 2020-2022 Slava Monich <slava.monich@jolla.com>
 * Copyright (C) 2023 Droidian Project
 * Copyright (C) 2023 Bardia Moshiri <fakeshell@bardia.tech>
 *
 * You may use this file under the terms of BSD license as follows:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *   3. Neither the names of the copyright holders nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <nfc_plugin_impl.h>
#include <nfc_manager.h>
#include <gutil_log.h>
#include <glib.h>
#include "wlrdisplay.h"

enum manager_events {
    MANAGER_ENABLED,
    MANAGER_EVENT_COUNT
};

typedef NfcPluginClass BatmanPluginClass;
typedef struct batman_plugin {
    NfcPlugin parent;
    NfcManager* manager;
    gulong manager_event_id[MANAGER_EVENT_COUNT];
    guint timer_id;
    gboolean screen_on;
    gboolean always_on;
} BatmanPlugin;

G_DEFINE_TYPE(BatmanPlugin, batman_plugin, NFC_TYPE_PLUGIN)
#define THIS_TYPE (batman_plugin_get_type())
#define THIS(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), THIS_TYPE, BatmanPlugin))

#define SETTINGS_STORAGE_PATH   "/var/lib/nfcd/settings"
#define SETTINGS_GROUP          "Settings"
#define SETTINGS_KEY_ALWAYS_ON  "AlwaysOn"

int wlrdisplay_status() {
    putenv("XDG_RUNTIME_DIR=/run/user/32011");
    int result = wlrdisplay(0, NULL);
    return result != 0;
}

static gboolean batman_plugin_check_wlrdisplay_status(gpointer user_data) {
    BatmanPlugin* self = THIS(user_data);
    gboolean new_screen_status = wlrdisplay_status() == 0;

    if (new_screen_status != self->screen_on) {
        self->screen_on = new_screen_status;

        if (new_screen_status) {
            nfc_manager_request_power(self->manager, TRUE); // Turn ON
        } else {
            nfc_manager_request_power(self->manager, FALSE); // Turn OFF
        }
    }

    return TRUE;
}

static gboolean batman_plugin_start(NfcPlugin* plugin, NfcManager* manager) {
    BatmanPlugin* self = THIS(plugin);

    self->manager = nfc_manager_ref(manager);

    nfc_manager_request_power(self->manager, TRUE); // Turn ON

    self->screen_on = wlrdisplay_status() == 0;

    if (!self->timer_id) {
        self->timer_id = g_timeout_add(5000, batman_plugin_check_wlrdisplay_status, self);
    }

    return TRUE;
}

static void batman_plugin_stop(NfcPlugin* plugin) {
    BatmanPlugin* self = THIS(plugin);

    nfc_manager_request_power(self->manager, FALSE); // Turn OFF

    if (self->timer_id) {
        g_source_remove(self->timer_id);
        self->timer_id = 0;
    }

    nfc_manager_remove_all_handlers(self->manager, self->manager_event_id);
    nfc_manager_unref(self->manager);
    self->manager = NULL;
}

static void batman_plugin_init(BatmanPlugin* self) {
    GKeyFile* config = g_key_file_new();

    if (g_key_file_load_from_file(config, SETTINGS_STORAGE_PATH, 0, NULL)) {
        self->always_on = g_key_file_get_boolean(config, SETTINGS_GROUP, SETTINGS_KEY_ALWAYS_ON, NULL);
    }

    g_key_file_unref(config);
}

static void batman_plugin_class_init(BatmanPluginClass* klass) {
    klass->start = batman_plugin_start;
    klass->stop = batman_plugin_stop;
}

static NfcPlugin* batman_plugin_create(void) {
    return g_object_new(THIS_TYPE, NULL);
}

NFC_PLUGIN_DEFINE2(batman, "batman state switcher", batman_plugin_create, 0, 0)
