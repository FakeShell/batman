// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2024 Bardia Moshiri <fakeshell@bardia.tech>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "batman-gbinder.h"
#include "mtk.h"

int main(int argc, char *argv[]) {
    // This is a weird way to handle it, we should do something about it
    // but I don't have any good solution in mind right now ¯\_(ツ)_/¯
    if (argc == 3) {
        char *feature = argv[1];
        int state = atoi(argv[2]);

        if (strcmp(feature, "vr") == 0) {
            if (state == 0 || state == 1) {
                int ret = init_vr_hidl(state);

                if (ret != 0) {
                    printf("None of the backends are available for VR. Exiting.\n");
                    return 1;
                } else {
                    //printf("Using VR HIDL backend\n");
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
                        printf("None of the backends are available for power. Exiting.\n");
                        return 1;
                    } else {
                        //printf("Using Power HIDL backend\n");
                    }
                } else {
                    //printf("Using Power AIDL backend\n");
                }
            } else {
                printf("Invalid Power state argument. Use 0 for non-interactive + powersave or 1 for interactive + performance.\n");
                return 1;
            }
        } else if (strcmp(feature, "mtkpower") == 0) {
            if (state >= 20 && state <= 46) {
                int ret = init_mtkpower_hidl(state);

                if (ret != 0) {
                    printf("None of the backends are available for MTK Power. Exiting.\n");
                    return 1;
                } else {
                    //printf("Using MTK Power HIDL backend\n");
                }
            } else {
                printf("Invalid MTK Power state argument. State must be between 20 and 46\n");
                return 1;
            }
        } else {
            printf("Invalid feature argument. Use 'vr' or 'power' or 'mtkpower'.\n");
            return 1;
        }
    } else {
        printf("Usage: %s <feature> <state> for features 'vr' and 'power' and 'mtkpower'\n", argv[0]);
        return 1;
    }

    return 0;
}
