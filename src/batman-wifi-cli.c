/**
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2018 Jolla Ltd
 * Copyright (C) 2024 Bardia Moshiri <fakeshell@bardia.tech>
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "batman-wifi.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s [suspend|resume]\n", argv[0]);
        return 1;
    }

    if (wifi_init() < 0)
        return -1;

    if (strcmp(argv[1], "suspend") == 0) {
        wifi_set_powersave("wlan0", true);
        wifi_set_wmtwifi("wlan0", WMTWIFI_SUSPEND_VALUE);
        wifi_set_setcam(true);
        //printf("Suspend and power save set for wlan0\n");
    } else if (strcmp(argv[1], "resume") == 0) {
        wifi_set_powersave("wlan0", false);
        wifi_set_wmtwifi("wlan0", WMTWIFI_RESUME_VALUE);
        wifi_set_setcam(false);
        //printf("Resume and power save unset for wlan0\n");
    } else {
        fprintf(stderr, "Invalid argument. Use 'suspend' or 'resume'\n");
        wifi_cleanup();
        return EXIT_FAILURE;
    }

    wifi_cleanup();
    return EXIT_SUCCESS;
}
