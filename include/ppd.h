/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2026 Bardia Moshiri <bardia@furilabs.com>
 */

#ifndef PPD_H
#define PPD_H

#include "cpu.h"

typedef struct PpdContext PpdContext;

/**
 * Initialize and export org.freedesktop.UPower.PowerProfiles on the system bus.
 *
 * @param cpu CpuContext
 * @return PpdContext or NULL on failure
 */
PpdContext *
ppd_init(CpuContext *cpu);

/**
 * Unexport the service and free resources.
 *
 * @param ppd PpdContext
 */
void
ppd_cleanup(PpdContext *ppd);

#endif /* PPD_H */
