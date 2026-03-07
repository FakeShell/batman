/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2026 Bardia Moshiri <bardia@furilabs.com>
 */

#include <string.h>

#include "pulse.h"

typedef struct {
    PulseContext *pulse;
    int active_count;
} PulseRefreshState;

static gboolean
pulse_sink_input_is_active(const pa_sink_input_info *info)
{
    if (info == NULL)
        return FALSE;

    /*
     * treat only non-corked streams as actively playing.
     *
     * this avoids false positives from paused players that keep a sink
     * input around but have it corked.
     */
    if (info->corked)
        return FALSE;

    return TRUE;
}

static void
pulse_emit_state_if_changed(PulseContext *pulse, gboolean audio_playing)
{
    if (pulse == NULL)
        return;

    if (pulse->audio_playing == audio_playing)
        return;

    pulse->audio_playing = audio_playing;

    g_debug("pulse: audio_playing=%d active_sink_inputs=%d",
            pulse->audio_playing ? 1 : 0,
            pulse->active_sink_inputs);

    if (pulse->state_changed_cb != NULL)
        pulse->state_changed_cb(pulse, pulse->audio_playing, pulse->userdata);
}

static void
pulse_update_cached_state(PulseContext *pulse)
{
    if (pulse == NULL)
        return;

    pulse_emit_state_if_changed(pulse,
                                (pulse->active_sink_inputs > 0) ? TRUE : FALSE);
}

static void
pulse_refresh_done(PulseRefreshState *state)
{
    if (state == NULL || state->pulse == NULL) {
        g_free(state);
        return;
    }

    state->pulse->refresh_pending = FALSE;
    state->pulse->active_sink_inputs = state->active_count;
    pulse_update_cached_state(state->pulse);
    g_free(state);
}

static void
pulse_sink_input_info_list_cb(pa_context *context,
                              const pa_sink_input_info *info,
                              int eol,
                              void *userdata)
{
    PulseRefreshState *state = userdata;

    (void)context;

    if (state == NULL || state->pulse == NULL)
        return;

    if (eol < 0) {
        g_debug("pulse: sink-input list callback error");
        pulse_refresh_done(state);
        return;
    }

    if (eol > 0) {
        pulse_refresh_done(state);
        return;
    }

    if (pulse_sink_input_is_active(info))
        state->active_count++;
}

static void
pulse_refresh_sink_inputs(PulseContext *pulse)
{
    pa_operation *op;
    PulseRefreshState *state;

    if (pulse == NULL)
        return;

    if (!pulse->connected || pulse->context == NULL)
        return;

    if (pulse->refresh_pending)
        return;

    state = g_new0(PulseRefreshState, 1);
    state->pulse = pulse;
    state->active_count = 0;

    pulse->refresh_pending = TRUE;

    op = pa_context_get_sink_input_info_list(pulse->context,
                                             pulse_sink_input_info_list_cb,
                                             state);
    if (op == NULL) {
        g_debug("pulse: pa_context_get_sink_input_info_list failed: %s",
                pa_strerror(pa_context_errno(pulse->context)));
        pulse->refresh_pending = FALSE;
        g_free(state);
        return;
    }

    pa_operation_unref(op);
}

static void
pulse_sink_input_info_single_cb(pa_context *context,
                                const pa_sink_input_info *info,
                                int eol,
                                void *userdata)
{
    PulseContext *pulse = userdata;

    (void)context;
    (void)info;

    if (pulse == NULL)
        return;

    if (eol < 0) {
        g_debug("pulse: sink-input single lookup error");
        pulse_refresh_sink_inputs(pulse);
        return;
    }

    if (eol > 0)
        return;

    pulse_refresh_sink_inputs(pulse);
}

static void
pulse_subscribe_cb(pa_context *context,
                   pa_subscription_event_type_t type,
                   uint32_t idx,
                   void *userdata)
{
    PulseContext *pulse = userdata;
    pa_subscription_event_type_t facility;
    pa_subscription_event_type_t operation;
    pa_operation *op;

    (void)context;

    if (pulse == NULL)
        return;

    facility = type & PA_SUBSCRIPTION_EVENT_FACILITY_MASK;
    operation = type & PA_SUBSCRIPTION_EVENT_TYPE_MASK;

    if (facility != PA_SUBSCRIPTION_EVENT_SINK_INPUT)
        return;

    switch (operation) {
    case PA_SUBSCRIPTION_EVENT_NEW:
    case PA_SUBSCRIPTION_EVENT_CHANGE:
        op = pa_context_get_sink_input_info(pulse->context,
                                            idx,
                                            pulse_sink_input_info_single_cb,
                                            pulse);
        if (op == NULL) {
            g_debug("pulse: pa_context_get_sink_input_info(%u) failed: %s",
                    idx,
                    pa_strerror(pa_context_errno(pulse->context)));
            pulse_refresh_sink_inputs(pulse);
            return;
        }
        pa_operation_unref(op);
        break;

    case PA_SUBSCRIPTION_EVENT_REMOVE:
        /* we don't know whether the removed stream was corked or active, refresh the full count */
        pulse_refresh_sink_inputs(pulse);
        break;

    default:
        break;
    }
}

static void
pulse_subscribe_success_cb(pa_context *context,
                           int success,
                           void *userdata)
{
    PulseContext *pulse = userdata;

    (void)context;

    if (pulse == NULL)
        return;

    if (!success) {
        g_debug("pulse: subscription setup failed");
        return;
    }

    g_debug("pulse: subscribed to sink-input events");

    pulse_refresh_sink_inputs(pulse);
}

static void
pulse_context_state_cb(pa_context *context, void *userdata)
{
    PulseContext *pulse = userdata;
    pa_context_state_t state;

    if (pulse == NULL)
        return;

    state = pa_context_get_state(context);

    switch (state) {
    case PA_CONTEXT_READY: {
        pa_operation *op;

        pulse->connected = TRUE;
        g_debug("pulse: context ready");

        pa_context_set_subscribe_callback(context, pulse_subscribe_cb, pulse);

        op = pa_context_subscribe(context,
                                  PA_SUBSCRIPTION_MASK_SINK_INPUT,
                                  pulse_subscribe_success_cb,
                                  pulse);
        if (op == NULL) {
            g_debug("pulse: pa_context_subscribe failed: %s",
                    pa_strerror(pa_context_errno(context)));
            return;
        }

        pa_operation_unref(op);
        break;
    }

    case PA_CONTEXT_FAILED:
        pulse->connected = FALSE;
        pulse->refresh_pending = FALSE;
        pulse->active_sink_inputs = 0;
        g_debug("pulse: context failed: %s",
                pa_strerror(pa_context_errno(context)));
        pulse_emit_state_if_changed(pulse, FALSE);
        break;

    case PA_CONTEXT_TERMINATED:
        pulse->connected = FALSE;
        pulse->refresh_pending = FALSE;
        pulse->active_sink_inputs = 0;
        g_debug("pulse: context terminated");
        pulse_emit_state_if_changed(pulse, FALSE);
        break;

    case PA_CONTEXT_CONNECTING:
    case PA_CONTEXT_AUTHORIZING:
    case PA_CONTEXT_SETTING_NAME:
        break;

    case PA_CONTEXT_UNCONNECTED:
    default:
        break;
    }
}

void
pulse_init(PulseContext *pulse,
           PulseAudioStateChangedFunc state_changed_cb,
           void *userdata)
{
    pa_mainloop_api *api;

    if (pulse == NULL)
        return;

    memset(pulse, 0, sizeof(*pulse));

    pulse->available = FALSE;
    pulse->connected = FALSE;
    pulse->audio_playing = FALSE;
    pulse->refresh_pending = FALSE;
    pulse->active_sink_inputs = 0;
    pulse->state_changed_cb = state_changed_cb;
    pulse->userdata = userdata;

    pulse->mainloop = pa_glib_mainloop_new(g_main_context_default());
    if (pulse->mainloop == NULL) {
        g_debug("pulse: pa_glib_mainloop_new failed");
        return;
    }

    api = pa_glib_mainloop_get_api(pulse->mainloop);
    if (api == NULL) {
        g_debug("pulse: pa_glib_mainloop_get_api failed");
        pa_glib_mainloop_free(pulse->mainloop);
        pulse->mainloop = NULL;
        return;
    }

    pulse->context = pa_context_new(api, "batman");
    if (pulse->context == NULL) {
        g_debug("pulse: pa_context_new failed");
        pa_glib_mainloop_free(pulse->mainloop);
        pulse->mainloop = NULL;
        return;
    }

    pa_context_set_state_callback(pulse->context, pulse_context_state_cb, pulse);

    if (pa_context_connect(pulse->context, NULL, PA_CONTEXT_NOFLAGS, NULL) < 0) {
        g_debug("pulse: pa_context_connect failed: %s",
                pa_strerror(pa_context_errno(pulse->context)));
        pa_context_unref(pulse->context);
        pulse->context = NULL;
        pa_glib_mainloop_free(pulse->mainloop);
        pulse->mainloop = NULL;
        return;
    }

    pulse->available = TRUE;
    g_debug("pulse: monitoring initialized");
}

void
pulse_cleanup(PulseContext *pulse)
{
    if (pulse == NULL)
        return;

    if (pulse->context != NULL) {
        pa_context_set_state_callback(pulse->context, NULL, NULL);
        pa_context_set_subscribe_callback(pulse->context, NULL, NULL);
        pa_context_disconnect(pulse->context);
        pa_context_unref(pulse->context);
        pulse->context = NULL;
    }

    if (pulse->mainloop != NULL) {
        pa_glib_mainloop_free(pulse->mainloop);
        pulse->mainloop = NULL;
    }

    pulse->available = FALSE;
    pulse->connected = FALSE;
    pulse->audio_playing = FALSE;
    pulse->refresh_pending = FALSE;
    pulse->active_sink_inputs = 0;
    pulse->state_changed_cb = NULL;
    pulse->userdata = NULL;
}

gboolean
pulse_is_audio_playing(const PulseContext *pulse)
{
    if (pulse == NULL)
        return FALSE;

    return pulse->audio_playing ? TRUE : FALSE;
}
