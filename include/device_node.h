/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2026 Bardia Moshiri <bardia@furilabs.com>
 */

#ifndef DEVICE_NODE_H
#define DEVICE_NODE_H

#include "config.h"

/**
 * Device governor node.
 */
typedef struct {
    gboolean present;             /** True if node exists */
    char path[4096];              /** Node path */
    char default_value[256];      /** Default value */
} DeviceGovernorNode;

/**
 * Device node context.
 */
typedef struct {
    DeviceGovernorNode governors[96];  /** Discovered devfreq governor nodes */
    int governor_count;               /** Count of governors[] used */
} DeviceNodeContext;

/**
 * Initialize device node context.
 *
 * @param dn context to fill
 */
void
device_node_init(DeviceNodeContext *dn);

/**
 * Apply device-node power saving.
 *
 * @param dn device node context
 * @param cfg configuration toggles
 */
void
device_node_apply_powersave(const DeviceNodeContext *dn, const BatmanConfig *cfg);

/**
 * Apply device-node defaults/performance.
 *
 * @param dn device node context
 * @param cfg configuration toggles
 */
void
device_node_apply_default(const DeviceNodeContext *dn, const BatmanConfig *cfg);

#endif /* DEVICE_NODE_H */
