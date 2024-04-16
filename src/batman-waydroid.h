// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2024 Bardia Moshiri <fakeshell@bardia.tech>

#ifndef BATMAN_WAYDROID_H
#define BATMAN_WAYDROID_H

#include <gio/gio.h>

gchar*
waydroid_get_state ();

/**
 * Sends a D-Bus request to Waydroid to either freeze or unfreeze.
 *
 * @param freeze TRUE to freeze, FALSE to unfreeze.
 */
void
waydroid_freezer (gboolean state);

void
waydroid_screen_toggle ();

gboolean
waydroid_screen_status ();

/**
 * Sends a D-Bus request to Waydroid to check if it is available
 * if it is running then turn the screen on or off
 *
 * @param freeze TRUE to turn the screen on, FALSE to turn the screen off.
 */
void
waydroid_screen (gboolean state);

#endif // BATMAN_WAYDROID_H
