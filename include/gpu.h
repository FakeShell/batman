/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2026 Bardia Moshiri <bardia@furilabs.com>
 */

#ifndef GPU_H
#define GPU_H

#include "config.h"

/**
 * GPU governor context.
 */
typedef struct {
    gboolean available;           /** True if a GPU governor node was found */
    gboolean powersave_supported; /** True if "powersave" is supported/detectable */

    char governor_path[4096];     /** governor node path */
    char default_governor[128];   /** value read at init */
} GpuContext;

/**
 * Probe GPU governor nodes and read default governor.
 *
 * @param gpu context to fill
 * @param cfg config for optional override path
 */
void
gpu_init(GpuContext *gpu, const BatmanConfig *cfg);

/**
 * Set GPU governor to "powersave" if supported/enabled.
 *
 * @param gpu gpu context
 * @param cfg config
 */
void
gpu_set_powersave(const GpuContext *gpu, const BatmanConfig *cfg);

/**
 * Restore GPU governor to the saved default.
 *
 * @param gpu gpu context
 * @param cfg config
 */
void
gpu_set_default(const GpuContext *gpu, const BatmanConfig *cfg);

#endif /* GPU_H */
