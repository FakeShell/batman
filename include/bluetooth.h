/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2026 Bardia Moshiri <bardia@furilabs.com>
 */

#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include "config.h"

/**
 * Bluetooth runtime context
 */
typedef struct {
    GDBusConnection *bus;               /** System bus connection */
    char adapter_path[512];             /** BlueZ adapter object path */
    gboolean forced_off;                /** True if we powered adapter off */
    gboolean adapter_initially_powered; /** Adapter Powered state at init */
    gboolean ready;                     /** True if bus is connected */
} BluetoothContext;

/**
 * Initialize Bluetooth control.
 *
 * @param bt BluetoothContext to initialize
 * @return true on success, false on failure to connect to system bus
 */
gboolean
bluetooth_init(BluetoothContext *bt);

/**
 * Cleanup Bluetooth control resources.
 *
 * If we forced bluetooth off, this does not automatically restore it.
 *
 * @param bt BluetoothContext to clean up
 */
void
bluetooth_cleanup(BluetoothContext *bt);

/**
 * Check whether any bluetooth device is currently connected.
 *
 * @param bt BluetoothContext
 * @return true if any device is connected, false otherwise
 */
gboolean
bluetooth_any_device_connected(BluetoothContext *bt);

/**
 * Apply bluetooth powersave policy.
 *
 * If bluetooth powersave is disabled in config or adapter is missing, does nothing
 * If adapter is powered and no device is connected then power the adapter off
 * If any device is connected do nothing
 *
 * @param bt BluetoothContext
 * @param cfg BatmanConfig
 */
void
bluetooth_apply_powersave(BluetoothContext *bt, const BatmanConfig *cfg);

/**
 * Apply bluetooth default policy.
 *
 * If bluetooth powersave is disabled in config or adapter is missing, does nothing
 * If we previously powered adapter off then power it back on
 *
 * @param bt BluetoothContext
 * @param cfg BatmanConfig
 */
void
bluetooth_apply_default(BluetoothContext *bt, const BatmanConfig *cfg);

#endif /* BLUETOOTH_H */
