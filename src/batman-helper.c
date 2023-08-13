// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2023 Bardia Moshiri <fakeshell@bardia.tech>

#include "batman-wrappers.h"
#include <stdio.h>

#ifdef WITH_WLRDISPLAY
int wlrdisplay(int argc, char *argv[]);
#endif

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s [cpu|mem|wlrdisplay|battery]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "cpu") == 0) {
        return cpuUsage();
    } else if (strcmp(argv[1], "mem") == 0) {
        return memUsage();
    } else if (strcmp(argv[1], "wlrdisplay") == 0) {
        #ifdef WITH_WLRDISPLAY
        return wlrdisplay(argc, argv);
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

        UpDevice *battery = findBattery(upower);

        g_object_unref(upower);

        return 0;
        #else
        printf("Upower support is not enabled. Recompile with -DWITH_UPOWER to enable it.\n");
        return EXIT_FAILURE;
        #endif
    } else {
        printf("Invalid option. Usage: %s [cpu|mem|wlrdisplay|battery]\n", argv[0]);
        return EXIT_FAILURE;
    }
}
