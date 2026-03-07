/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2026 Bardia Moshiri <bardia@furilabs.com>
 */

#ifndef PULSE_H
#define PULSE_H

#include <glib.h>
#include <pulse/glib-mainloop.h>
#include <pulse/pulseaudio.h>

typedef struct _PulseContext PulseContext;

/**
 * PulseAudio playback state callback.
 *
 * @param pulse pulse context
 * @param audio_playing TRUE if at least one non-corked sink input exists
 * @param userdata user data passed to pulse_init()
 */
typedef void (*PulseAudioStateChangedFunc)(const PulseContext *pulse,
                                           gboolean audio_playing,
                                           void *userdata);

/**
 * PulseAudio monitoring context.
 */
struct _PulseContext {
    gboolean available;         /** True if PulseAudio context was created */
    gboolean connected;         /** True if connected to PulseAudio */
    gboolean audio_playing;     /** Cached playback state */
    gboolean refresh_pending;   /** True while a sink-input refresh is in flight */

    int active_sink_inputs;     /** Count of non-corked sink inputs */

    pa_glib_mainloop *mainloop;
    pa_context *context;

    PulseAudioStateChangedFunc state_changed_cb;
    void *userdata;
};

/**
 * Initialize PulseAudio monitoring.
 *
 * @param pulse context to fill
 * @param state_changed_cb callback invoked when playback state changes
 * @param userdata callback user data
 */
void
pulse_init(PulseContext *pulse,
           PulseAudioStateChangedFunc state_changed_cb,
           void *userdata);

/**
 * Free PulseAudio monitoring resources.
 *
 * @param pulse pulse context
 */
void
pulse_cleanup(PulseContext *pulse);

/**
 * Return cached PulseAudio playback state.
 *
 * @param pulse pulse context
 * @return TRUE if playback is currently active, otherwise FALSE
 */
gboolean
pulse_is_audio_playing(const PulseContext *pulse);

#endif /* PULSE_H */
