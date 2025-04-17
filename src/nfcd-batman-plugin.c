/*
 * Copyright (C) 2020-2022 Jolla Ltd.
 * Copyright (C) 2020-2022 Slava Monich <slava.monich@jolla.com>
 * Copyright (C) 2025 Furi Labs
 * Copyright (C) 2025 Bardia Moshiri <bardia@furilabs.com>
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
#include <glib.h>
#include <sys/inotify.h>
#include <unistd.h>

#define SCREEN_STATE_PATH "/var/lib/batman/screen"
#define EVENT_BUF_LEN     (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))

enum manager_events {
    MANAGER_ENABLED,
    MANAGER_EVENT_COUNT
};

typedef NfcPluginClass BatmanPluginClass;
typedef struct batman_plugin {
    NfcPlugin parent;
    NfcManager *manager;
    gulong manager_event_id[MANAGER_EVENT_COUNT];
    GIOChannel *inotify_channel;
    guint inotify_watch_id;
    int inotify_fd;
    int inotify_wd;
    gboolean screen_on;
} BatmanPlugin;

G_DEFINE_TYPE(BatmanPlugin, batman_plugin, NFC_TYPE_PLUGIN)
#define THIS_TYPE (batman_plugin_get_type())
#define THIS(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), THIS_TYPE, BatmanPlugin))

static gboolean
batman_plugin_read_screen_state(BatmanPlugin *self)
{
    char buf[4];
    FILE *f = fopen(SCREEN_STATE_PATH, "r");
    if (!f) {
        g_debug("Failed to open %s: %s", SCREEN_STATE_PATH, strerror(errno));
        return FALSE;
    }

    size_t len = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);

    if (len > 0) {
        buf[len] = '\0';
        return strncmp(buf, "yes", 3) == 0;
    }

    return FALSE;
}

static gboolean
batman_plugin_inotify_callback(GIOChannel *source, GIOCondition condition, gpointer user_data)
{
    BatmanPlugin *self = THIS(user_data);
    char buffer[EVENT_BUF_LEN];
    gsize bytes_read;
    GError* error = NULL;

    if (condition & (G_IO_HUP | G_IO_ERR | G_IO_NVAL)) {
        g_debug("inotify watch failed, condition: %d", condition);
        self->inotify_watch_id = 0;
        return G_SOURCE_REMOVE;
    }

    GIOStatus status = g_io_channel_read_chars(source, buffer,
        EVENT_BUF_LEN, &bytes_read, &error);

    if (status == G_IO_STATUS_ERROR) {
        if (error) {
            g_debug("Error reading inotify event: %s", error->message);
            g_error_free(error);
        }
        self->inotify_watch_id = 0;
        return G_SOURCE_REMOVE;
    }

    if (bytes_read > 0) {
        gboolean screen_on = batman_plugin_read_screen_state(self);
        if (screen_on != self->screen_on) {
            g_debug("Screen state changed from %s to %s",
                    self->screen_on ? "ON" : "OFF",
                    screen_on ? "ON" : "OFF");
            self->screen_on = screen_on;
            nfc_manager_request_power(self->manager, screen_on);
        }
    }

    return G_SOURCE_CONTINUE;
}

static void
batman_plugin_setup_inotify(BatmanPlugin *self)
{
    if (access(SCREEN_STATE_PATH, F_OK) != 0) {
        g_debug("Screen state file %s does not exist", SCREEN_STATE_PATH);
        return;
    }

    self->inotify_fd = inotify_init();
    if (self->inotify_fd < 0) {
        g_debug("Failed to initialize inotify: %s", strerror(errno));
        return;
    }

    self->inotify_wd = inotify_add_watch(self->inotify_fd, SCREEN_STATE_PATH,
        IN_MODIFY | IN_CLOSE_WRITE);
    if (self->inotify_wd < 0) {
        g_debug("Failed to add inotify watch: %s", strerror(errno));
        close(self->inotify_fd);
        self->inotify_fd = -1;
        return;
    }

    self->inotify_channel = g_io_channel_unix_new(self->inotify_fd);
    g_io_channel_set_encoding(self->inotify_channel, NULL, NULL);
    g_io_channel_set_flags(self->inotify_channel,
        G_IO_FLAG_NONBLOCK | g_io_channel_get_flags(self->inotify_channel), NULL);
    g_io_channel_set_buffered(self->inotify_channel, FALSE);

    self->inotify_watch_id = g_io_add_watch(self->inotify_channel,
        G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL, batman_plugin_inotify_callback, self);

    self->screen_on = batman_plugin_read_screen_state(self);
    g_debug("Initial screen state: %s", self->screen_on ? "ON" : "OFF");
}

static void
batman_plugin_cleanup_inotify(BatmanPlugin *self)
{
    if (self->inotify_watch_id) {
        g_source_remove(self->inotify_watch_id);
        self->inotify_watch_id = 0;
    }

    if (self->inotify_channel) {
        g_io_channel_unref(self->inotify_channel);
        self->inotify_channel = NULL;
    }

    if (self->inotify_wd >= 0) {
        inotify_rm_watch(self->inotify_fd, self->inotify_wd);
        self->inotify_wd = -1;
    }

    if (self->inotify_fd >= 0) {
        close(self->inotify_fd);
        self->inotify_fd = -1;
    }
}

static gboolean
batman_plugin_start(NfcPlugin *plugin, NfcManager *manager)
{
    BatmanPlugin* self = THIS(plugin);
    self->manager = nfc_manager_ref(manager);
    batman_plugin_setup_inotify(self);
    return TRUE;
}

static void
batman_plugin_stop(NfcPlugin *plugin)
{
    BatmanPlugin* self = THIS(plugin);
    batman_plugin_cleanup_inotify(self);
    nfc_manager_remove_all_handlers(self->manager, self->manager_event_id);
    nfc_manager_unref(self->manager);
    self->manager = NULL;
}

static void
batman_plugin_init(BatmanPlugin *self)
{
    self->inotify_fd = -1;
    self->inotify_wd = -1;
}

static void
batman_plugin_class_init(BatmanPluginClass *klass)
{
    klass->start = batman_plugin_start;
    klass->stop = batman_plugin_stop;
}

static NfcPlugin*
batman_plugin_create(void)
{
    return g_object_new(THIS_TYPE, NULL);
}

NFC_PLUGIN_DEFINE2(batman, "batman state switcher", batman_plugin_create, 0, 0)
