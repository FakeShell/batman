// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2023 Bardia Moshiri <fakeshell@bardia.tech>

#include "batman-wrappers.h"
#include <stdio.h>

#ifdef WITH_WLRDISPLAY
#include "wlrdisplay.h"
#endif

#ifdef WITH_GETINFO
#include "getinfo.h"
#endif

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s [cpu|mem|wlrdisplay|battery|batman_active|batman_enabled]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "cpu") == 0) {
        printf("%.lf\n", cpuUsage());
        return EXIT_SUCCESS;
    } else if (strcmp(argv[1], "mem") == 0) {
        printf("%.Lf\n", memUsage());
        return EXIT_SUCCESS;
    } else if (strcmp(argv[1], "wlrdisplay") == 0) {
        #ifdef WITH_WLRDISPLAY
        int result = wlrdisplay(argc, argv);
        printf(result == 0 ? "yes\n" : "no\n");
        return EXIT_SUCCESS;
        #else
        printf("wlrdisplay support is not enabled. Recompile with -DWITH_WLRDISPLAY to enable it.\n");
        return EXIT_FAILURE;
        #endif
    } else if (strcmp(argv[1], "battery") == 0) {
        #ifdef WITH_UPOWER
        UpClient *upower = up_client_new();

        if (upower == NULL) {
            g_print("Could not connect to upower");
            return 2;
        }

        const gchar *batteryStatus = findBattery(upower);
        if (batteryStatus != NULL) {
            g_print("%s\n", batteryStatus);
        }

        g_object_unref(upower);

        return 0;
        #else
        printf("Upower support is not enabled. Recompile with -DWITH_UPOWER to enable it.\n");
        return EXIT_FAILURE;
        #endif
    } else if (strcmp(argv[1], "batman_active") == 0) {
        #ifdef WITH_GETINFO
        if (check_batman_active() != -1) {
            printf("Batman active status: %s\n", bm_state.active ? "active" : "inactive");
            return EXIT_SUCCESS;
        } else {
            printf("Failed to check Batman active status\n");
            return EXIT_FAILURE;
        }
        #else
        printf("getinfo support is not enabled. Recompile with -DWITH_GETINFO to enable it.\n");
        return EXIT_FAILURE
        #endif
    } else if (strcmp(argv[1], "batman_enabled") == 0) {
        #ifdef WITH_GETINFO
        if (check_batman_enabled() != -1) {
            printf("Batman enabled status: %s\n", bm_state.enabled ? "enabled" : "disabled");
            return EXIT_SUCCESS;
        } else {
            printf("Failed to check Batman enabled status\n");
            return EXIT_FAILURE;
        }
        #else
        printf("getinfo support is not enabled. Recompile with -DWITH_GETINFO to enable it.\n");
        return EXIT_FAILURE
        #endif
    } else {
        printf("Invalid option. Usage: %s [cpu|mem|wlrdisplay|battery|batman_active|batman_enabled]\n", argv[0]);
        return EXIT_FAILURE;
    }
}
