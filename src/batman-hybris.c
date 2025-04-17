/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2025 Bardia Moshiri <fakeshell@bardia.tech>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "batman-gbinder.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <feature> <state>\n", argv[0]);
        printf("Features:\n");
        printf("  power    - Power management (0: power saver, 1: performance)\n");
        printf("  vr       - VR mode (0: off, 1: on)\n");
        printf("  mtkpower - MTK power hint (value between 20 and 46)\n");
        return 1;
    }

    const char *feature = argv[1];
    int state = atoi(argv[2]);
    int ret = -1;

    if (strcmp(feature, "vr") == 0) {
        if (state == 0 || state == 1) {
            ret = batman_set_vr_mode(state);
            if (ret != 0) {
                printf("Failed to set VR mode. VR service might not be available.\n");
                return 1;
            }
        } else {
            printf("Invalid VR state. Use 0 for VR mode off or 1 for VR mode on.\n");
            return 1;
        }
    } else if (strcmp(feature, "power") == 0) {
        if (state == 0) {
            ret = batman_set_power_saver();
            if (ret != 0) {
                printf("Failed to set power saver mode. Power service might not be available.\n");
                return 1;
            }
        }  else if (state == 1) {
            ret = batman_set_performance();
            if (ret != 0) {
                printf("Failed to set performance mode. Power service might not be available.\n");
                return 1;
            }
        } else {
            printf("Invalid power state. Use 0 for power saver or 1 for performance.\n");
            return 1;
        }
    } else if (strcmp(feature, "mtkpower") == 0) {
        if (state >= MTK_POWER_HINT_PROCESS_CREATE && state <= MTK_POWER_HINT_THERMAL_LIMIT) {
            ret = batman_apply_mtkpower_hint((MtkPowerHint)state);
            if (ret != 0) {
                printf("Failed to apply MTK power hint. MTK Power service might not be available.\n");
                return 1;
            }
        } else {
            printf("Invalid MTK power hint. State must be between 20 and 46.\n");
            return 1;
        }
    } else {
        printf("Invalid feature. Use 'vr', 'power', or 'mtkpower'.\n");
        return 1;
    }

    return 0;
}
