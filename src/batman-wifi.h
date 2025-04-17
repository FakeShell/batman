/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2018 Jolla Ltd
 * Copyright (C) 2025 Bardia Moshiri <bardia@furilabs.com>
 */

#ifndef BATMAN_WIFI_H
#define BATMAN_WIFI_H

#include <stdbool.h>
#include <stdint.h>

#define WMTWIFI_SUSPEND_VALUE   (1)
#define WMTWIFI_RESUME_VALUE    (0)

/**
 * Initialize WiFi control system
 *
 * @return 0 on success, negative on error
 */
int
wifi_init(void);

/**
 * Cleanup WiFi control system
 */
void
wifi_cleanup(void);

/**
 * Set powersave state for a WiFi interface
 *
 * @param ifname interface name (e.g. wlan0)
 * @param is_enable enable/disable state
 */
void
wifi_set_powersave(const char *ifname, bool is_enable);

/**
 * Set WMT WiFi state for a WiFi interface
 *
 * @param ifname interface name (e.g. wlan0)
 * @param suspend_value suspend value as uint8_t (usually 1 to suspend, 0 to resume)
 */
void
wifi_set_wmtwifi(const char *ifname, uint8_t suspend_value);

/**
 * Set Mediatek CAM to powersave
 *
 * @param is_enable enable/disable state
 */
void
wifi_set_setcam(bool is_enable);

#endif /* BATMAN_WIFI_H */
