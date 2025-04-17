/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2025 Bardia Moshiri <bardia@furilabs.com>
 */

#ifndef GETINFO_H
#define GETINFO_H

#include <glib.h>

/**
 * BatmanState:
 * @active: Whether the batman service is currently active
 * @enabled: Whether the batman service is enabled at boot
 *
 * Structure containing the current state of the batman service
 */
typedef struct {
    gboolean active;
    gboolean enabled;
} BatmanState;

extern BatmanState bm_state;

/**
 * check_batman_active:
 *
 * Checks if the batman service is currently active using systemd D-Bus interface
 *
 * Returns: 0 on success, -1 on failure
 */
int
check_batman_active(void);

/**
 * check_batman_enabled:
 *
 * Checks if the batman service is enabled at boot using systemd D-Bus interface
 *
 * Returns: 0 on success, -1 on failure
 */
int
check_batman_enabled(void);

/**
 * start_batman_service:
 * @error: (nullable): Return location for error
 *
 * Starts the batman systemd service using D-Bus
 *
 * Returns: %TRUE on success, %FALSE if an error occurred
 */
gboolean
start_batman_service(GError **error);

/**
 * stop_batman_service:
 * @error: (nullable): Return location for error
 *
 * Stops the batman systemd service using D-Bus
 *
 * Returns: %TRUE on success, %FALSE if an error occurred
 */
gboolean
stop_batman_service(GError **error);

/**
 * enable_batman_service:
 * @error: (nullable): Return location for error
 *
 * Enables the batman systemd service using D-Bus
 *
 * Returns: %TRUE on success, %FALSE if an error occurred
 */
gboolean
enable_batman_service(GError **error);

/**
 * disable_batman_service:
 * @error: (nullable): Return location for error
 *
 * Disables the batman systemd service using D-Bus
 *
 * Returns: %TRUE on success, %FALSE if an error occurred
 */
gboolean
disable_batman_service(GError **error);

#endif /* GETINFO_H */
