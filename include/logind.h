/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2026 Bardia Moshiri <bardia@furilabs.com>
 */

#ifndef LOGIND_H
#define LOGIND_H

#include <gio/gio.h>

/**
 * Logind screen state.
 * - IdleHint == TRUE  -> screen OFF
 * - IdleHint == FALSE -> screen ON
 */
typedef enum {
    LOGIND_SCREEN_UNKNOWN = 0,
    LOGIND_SCREEN_OFF = 1,
    LOGIND_SCREEN_ON = 2
} LogindScreenState;

/**
 * Logind monitor object.
 */
typedef struct {
    GDBusConnection *connection;      /** System bus connection */
    GDBusProxy *manager_proxy;        /** org.freedesktop.login1.Manager */
    GDBusProxy *session_props_proxy;  /** org.freedesktop.DBus.Properties */

    guint properties_changed_id;      /** Signal subscription id */
    guint retry_source_id;            /** Retry timer id */

    char *session_id;                 /** Current tty7 session id */
    LogindScreenState screen_state;   /** Cached screen state */

    void (*on_screen_changed)(LogindScreenState state, void *user_data); /** Callback */
    void *user_data;                  /** User pointer */
} LogindMonitor;

/**
 * Create and start a logind monitor.
 *
 * @param cb Callback invoked when screen state changes
 * @param user_data User pointer passed to callback
 * @return Allocated LogindMonitor
 */
LogindMonitor *
logind_monitor_new(void (*cb)(LogindScreenState state, void *user_data), void *user_data);

/**
 * Free logind monitor and unsubscribe from signals and timers.
 *
 * @param m LogindMonitor instance (can be NULL)
 */
void
logind_monitor_free(LogindMonitor *m);

/**
 * Get the current cached screen state.
 *
 * @param m LogindMonitor instance
 * @return Cached state (UNKNOWN/ON/OFF)
 */
LogindScreenState
logind_monitor_get_screen_state(LogindMonitor *m);

#endif /* LOGIND_H */
