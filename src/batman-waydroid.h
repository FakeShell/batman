// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2024 Bardia Moshiri <fakeshell@bardia.tech>

#ifndef BATMAN_WAYDROID_H
#define BATMAN_WAYDROID_H

#include <gio/gio.h>

/**
 * Sends a D-Bus request to Waydroid to either freeze or unfreeze.
 *
 * @param freeze TRUE to freeze, FALSE to unfreeze.
 */
void waydroid_freezer(gboolean freeze);

#endif // BATMAN_WAYDROID_H
