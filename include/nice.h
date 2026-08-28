/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2026 Bardia Moshiri <bardia@furilabs.com>
 */

#ifndef NICE_H
#define NICE_H

#include <glib.h>

typedef struct NiceContext NiceContext;

/**
 * Initialize the niceness manager.
 *
 * @return NiceContext or NULL on failure
 */
NiceContext *
nice_init(void);

/**
 * Enable or disable niceness handling.
 *
 * @param nice NiceContext
 * @param enabled TRUE to enable, otherwise FALSE
 * @return TRUE on success, otherwise FALSE
 */
gboolean
nice_enable(NiceContext *nice, gboolean enabled);

/**
 * Check whether niceness handling is enabled.
 *
 * @param nice NiceContext
 * @return TRUE if enabled, otherwise FALSE
 */
gboolean
nice_is_enabled(const NiceContext *nice);

/**
 * Disable niceness handling and free resources.
 *
 * @param nice NiceContext
 */
void
nice_cleanup(NiceContext *nice);

#endif /* NICE_H */
