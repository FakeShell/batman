/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2026 Bardia Moshiri <bardia@furilabs.com>
 */

#ifndef THERMAL_H
#define THERMAL_H

#include <gio/gio.h>

/**
 * Read thermal zones under /sys/class/thermal and compute an average temperature.
 *
 * @param out_avg receives average temperature on success
 * @return TRUE if at least one zone contributed to average, FALSE otherwise
 */
gboolean
thermal_get_average_celsius(double *out_avg);

#endif /* THERMAL_H */
