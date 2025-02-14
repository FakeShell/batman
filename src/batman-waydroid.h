// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2025 Bardia Moshiri <fakeshell@bardia.tech>

#ifndef BATMAN_WAYDROID_H
#define BATMAN_WAYDROID_H

#include <gio/gio.h>

/**
 * Retrieves the current state of the Waydroid container.
 *
 * @return A string containing the current state of Waydroid.
 *         Caller is responsible for freeing the returned string.
 */
gchar*
waydroid_get_state ();

/**
 * Sends a D-Bus request to Waydroid to either freeze or unfreeze.
 *
 * @param freeze TRUE to freeze, FALSE to unfreeze.
 */
void
waydroid_freezer (gboolean state);

/**
 * Toggles the Waydroid container's screen state.
 * If the screen is on, turns it off, and vice versa.
 */
void
waydroid_screen_toggle ();

/**
 * Queries the current screen status of the Waydroid container.
 *
 * @return TRUE if the screen is on, FALSE if the screen is off.
 */
gboolean
waydroid_screen_status ();

/**
 * Sends a D-Bus request to Waydroid to check if it is available
 * if it is running then turn the screen on or off.
 *
 * @param state TRUE to turn the screen on, FALSE to turn the screen off.
 */
void
waydroid_screen (gboolean state);

/**
 * Checks the Waydroid container for any currently running applications
 * by sending a D-Bus request to query the container's state.
 *
 * @return TRUE if there are any applications running in the Waydroid container,
 *         FALSE if no applications are currently active.
 */
gboolean
waydroid_app_open ();

/**
 * Sets a system property in the Waydroid container via D-Bus.
 *
 * @param propname The name of the property to set.
 * @param propvalue The value to set the property to.
 */
void
waydroid_setprop (const gchar* propname, const gchar* propvalue);

#endif // BATMAN_WAYDROID_H
