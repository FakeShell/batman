// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2024 Bardia Moshiri <fakeshell@bardia.tech>

#ifndef GETINFO_H
#define GETINFO_H

#include <glib.h>

typedef struct {
    gboolean active;
    gboolean enabled;
} BatmanState;

extern BatmanState bm_state;

/**
 * Check if the batman service is currently active using systemd D-Bus interface
 * @return 0 on success, -1 on failure
 */
int check_batman_active(void);

/**
 * Check if the batman service is enabled at boot using systemd D-Bus interface
 * @return 0 on success, -1 on failure
 */
int check_batman_enabled(void);

#endif // GETINFO_H
