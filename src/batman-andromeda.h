// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2025 Bardia Moshiri <fakeshell@bardia.tech>

#ifndef BATMAN_ANDROMEDA_H
#define BATMAN_ANDROMEDA_H

#include <gio/gio.h>

/**
 * Retrieves the current state of the Andromeda container.
 *
 * @return A string containing the current state of Andromeda.
 *         Caller is responsible for freeing the returned string.
 */
gchar*
andromeda_get_state ();

/**
 * Sends a D-Bus request to Andromeda to either freeze or unfreeze.
 *
 * @param freeze TRUE to freeze, FALSE to unfreeze.
 */
void
andromeda_freezer (gboolean state);

/**
 * Toggles the Andromeda container's screen state.
 * If the screen is on, turns it off, and vice versa.
 */
void
andromeda_screen_toggle ();

/**
 * Queries the current screen status of the Andromeda container.
 *
 * @return TRUE if the screen is on, FALSE if the screen is off.
 */
gboolean
andromeda_screen_status ();

/**
 * Sends a D-Bus request to Andromeda to check if it is available
 * if it is running then turn the screen on or off.
 *
 * @param state TRUE to turn the screen on, FALSE to turn the screen off.
 */
void
andromeda_screen (gboolean state);

/**
 * Checks the Andromeda container for any currently running applications
 * by sending a D-Bus request to query the container's state.
 *
 * @return TRUE if there are any applications running in the Andromeda container,
 *         FALSE if no applications are currently active.
 */
gboolean
andromeda_app_open ();

/**
 * Sets a system property in the Andromeda container via D-Bus.
 *
 * @param propname The name of the property to set.
 * @param propvalue The value to set the property to.
 */
void
andromeda_setprop (const gchar* propname, const gchar* propvalue);

#endif // BATMAN_ANDROMEDA_H
