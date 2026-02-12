/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2026 Bardia Moshiri <bardia@furilabs.com>
 */

#include "thermal.h"
#include "utils.h"

#define THERMAL_SYSFS_PATH "/sys/class/thermal"

static gboolean
is_thermal_zone_name(const char *name)
{
    if (!name)
        return FALSE;
    return g_str_has_prefix(name, "thermal_zone");
}

static gboolean
read_double_from_file_scaled(const char *path, double scale, double *out)
{
    char buf[128];

    if (!path || !out)
        return FALSE;

    buf[0] = '\0';
    if (!read_str(path, buf, sizeof(buf)))
        return FALSE;

    char *endp = NULL;
    errno = 0;
    long long v = strtoll(buf, &endp, 10);
    if (errno != 0 || endp == buf)
        return FALSE;

    *out = ((double)v) / scale;
    return TRUE;
}

gboolean
thermal_get_average_celsius(double *out_avg)
{
    DIR *d = opendir(THERMAL_SYSFS_PATH);
    if (!d)
        return FALSE;

    struct dirent *de = NULL;
    int count = 0;
    double total = 0.0;

    while ((de = readdir(d)) != NULL) {
        if (!is_thermal_zone_name(de->d_name))
            continue;

        char path[512];
        g_snprintf(path, sizeof(path), "%s/%s/temp", THERMAL_SYSFS_PATH, de->d_name);

        double c = 0.0;
        if (!read_double_from_file_scaled(path, 1000.0, &c))
            continue;

        if (c <= 10.0)
            continue;

        total += c;
        count++;
    }

    closedir(d);

    if (count <= 0)
        return FALSE;

    if (out_avg)
        *out_avg = total / (double)count;

    return TRUE;
}
