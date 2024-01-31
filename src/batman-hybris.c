// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2024 Bardia Moshiri <fakeshell@bardia.tech>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "batman-gbinder.h"

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
                        printf("None of the backends are available for power. Exiting.\n");
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
    } else if (argc == 4) {
        int feature = atoi(argv[2]);
        int state = atoi(argv[1]);
        int ret;

        if ((state == 0 || state == 1) && (feature == 1 || feature == 2 || feature == 3)) {
            ret = init_radio_aidl(feature, state);

            if (ret != 0) {
                ret = init_radio_hidl(feature, state);

                if (ret != 0) {
                    printf("None of the backends are available for radio. Exiting.\n");
                    return 1;
                } else {
                    printf("Using Radio HIDL backend\n");
                }
            } else {
                printf("Using Radio AIDL backend\n");
            }
        } else {
            printf("Invalid argument. Use <feature> (1: power save mode, 2: charging state, 3: low data expected) <state> (1 for on, 0 for off)\n");
            return 1;
        }
    } else {
        printf("Usage: %s <feature> <state> for VR and Power OR %s <feature> (1: power save mode, 2: charging state, 3: low data expected) <state> for Radio\n", argv[0], argv[0]);
        return 1;
    }

    return 0;
}
