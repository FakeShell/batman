// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2024 Bardia Moshiri <fakeshell@bardia.tech>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "batman-gbinder.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <feature> <state> (1 for on, 0 for off)\n", argv[0]);
        printf("feature: 'vr' or 'power'\n");
        return 1;
    }

    char *feature = argv[1];
    int state = atoi(argv[2]);

    if (strcmp(feature, "vr") == 0) {
        if (state == 0 || state == 1) {
            int ret = init_vr_hidl(state);
            if (ret != 0) {
                printf("None of the VR backends are available. Exiting.\n");
                return 1;
            } else {
                printf("Using VR HIDL backend\n");
            }
        } else {
            printf("Invalid VR state argument. Use 0 for VR mode off or 1 for VR mode on\n");
            return 1;
        }
    } else if (strcmp(feature, "power") == 0) {
        if (state == 0 || state == 1) {
            int ret = init_power_aidl(state);
            if (ret != 0) {
                ret = init_power_hidl(state);
                if (ret != 0) {
                    printf("None of the Power backends are available. Exiting.\n");
                    return 1;
                } else {
                    printf("Using Power HIDL backend\n");
                }
            } else {
                printf("Using Power AIDL backend\n");
            }
        } else {
            printf("Invalid Power state argument. Use 0 for non-interactive + powersave or 1 for interactive + performance.\n");
            return 1;
        }
    } else {
        printf("Invalid feature argument. Use 'vr' or 'power'.\n");
        return 1;
    }

    return 0;
}
