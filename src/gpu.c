/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2026 Bardia Moshiri <bardia@furilabs.com>
 */

#include "gpu.h"
#include "utils.h"

static gboolean
gpu_read_available_governors(const char *governor_path, char *out, size_t out_len)
{
    char dir[4096];
    char path1[4096];
    char path2[4096];
    char *slash;

    if (governor_path == NULL || out == NULL || out_len == 0)
        return FALSE;

    g_strlcpy(dir, governor_path, sizeof(dir));

    slash = strrchr(dir, '/');
    if (slash == NULL)
        return FALSE;

    *slash = '\0';

    g_snprintf(path1, sizeof(path1), "%s/available_governors", dir);
    g_snprintf(path2, sizeof(path2), "%s/scaling_available_governors", dir);

    if (read_str(path1, out, out_len))
        return TRUE;
    if (read_str(path2, out, out_len))
        return TRUE;

    return FALSE;
}

void
gpu_init(GpuContext *gpu, const BatmanConfig *cfg)
{
    const char *candidates[] = {
        "/sys/class/kgsl/kgsl-3d0/devfreq/governor",
        "/sys/class/kgsl/kgsl-3d0/governor",
        "/sys/class/devfreq/1c00000.qcom,kgsl-3d0/governor",
        "/sys/class/devfreq/5000000.qcom,kgsl-3d0/governor",
        "/sys/class/devfreq/ddr_devfreq/governor",
        "/sys/class/devfreq/graphics/governor",
        "/sys/kernel/gpu/gpu_governor",
        NULL
    };
    int i;

    if (gpu == NULL)
        return;

    memset(gpu, 0, sizeof(*gpu));
    gpu->available = FALSE;
    gpu->powersave_supported = FALSE;
    gpu->governor_path[0] = '\0';
    gpu->default_governor[0] = '\0';

    if (cfg != NULL) {
        if (cfg->devfreq_gpu_path[0] != '\0') {
            char override_path[4096];

            g_snprintf(override_path, sizeof(override_path), "%s/governor", cfg->devfreq_gpu_path);

            if (exists(override_path)) {
                gpu->available = TRUE;
                g_strlcpy(gpu->governor_path, override_path, sizeof(gpu->governor_path));
            }
        }
    }

    if (!gpu->available) {
        for (i = 0; candidates[i] != NULL; i++) {
            if (exists(candidates[i])) {
                gpu->available = TRUE;
                g_strlcpy(gpu->governor_path, candidates[i], sizeof(gpu->governor_path));
                break;
            }
        }
    }

    if (!gpu->available)
        return;

    if (!read_str(gpu->governor_path, gpu->default_governor, sizeof(gpu->default_governor))) {
        g_debug("failed to read default gpu governor: %s", gpu->governor_path);
        gpu->default_governor[0] = '\0';
    }

    char avail[1024];

    if (gpu_read_available_governors(gpu->governor_path, avail, sizeof(avail))) {
        if (string_contains_token(avail, "powersave"))
            gpu->powersave_supported = TRUE;
        else
            gpu->powersave_supported = FALSE;
    } else {
        /* some drivers don't expose available_governors. we will try writing anyway. */
        gpu->powersave_supported = TRUE;
    }
}

void
gpu_set_powersave(const GpuContext *gpu, const BatmanConfig *cfg)
{
    if (gpu == NULL || cfg == NULL)
        return;
    if (!cfg->gpu_powersave_enabled)
        return;
    if (!gpu->available)
        return;
    if (!gpu->powersave_supported)
        return;

    write_str(gpu->governor_path, "powersave");
}

void
gpu_set_default(const GpuContext *gpu, const BatmanConfig *cfg)
{
    if (gpu == NULL || cfg == NULL)
        return;
    if (!cfg->gpu_powersave_enabled)
        return;
    if (!gpu->available)
        return;
    if (gpu->default_governor[0] == '\0')
        return;

    write_str(gpu->governor_path, gpu->default_governor);
}
