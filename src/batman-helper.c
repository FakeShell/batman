// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2024 Bardia Moshiri <fakeshell@bardia.tech>

#include "batman-wrappers.h"
#include "batman-waydroid.h"
#include <stdio.h>
#include "wlrdisplay.h"
#include "getinfo.h"

static const char* USAGE = "[cpu|mem|wlrdisplay|battery|battery_percentage|battery_state|batman_active|batman_enabled|waydroid_state|waydroid_screen]";

static void print_usage(const char* program_name) {
    printf("Usage: %s %s\n", program_name, USAGE);
}

static const char* get_battery_state_str(batman_state_t state) {
    switch (state) {
        case BATMAN_NO_BATTERY:
            return "no battery";
        case BATMAN_CHARGING:
            return "charging";
        case BATMAN_DISCHARGING:
            return "discharging";
        case BATMAN_FULLY_CHARGED:
            return "fully-charged";
        case BATMAN_UNKNOWN:
            return "unknown";
        default: return "invalid state";
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "cpu") == 0) {
        printf("%.lf\n", get_cpu_usage());
        return EXIT_SUCCESS;
    }

    if (strcmp(argv[1], "mem") == 0) {
        printf("%.Lf\n", mem_usage());
        return EXIT_SUCCESS;
    }

    if (strcmp(argv[1], "wlrdisplay") == 0) {
        int result = wlrdisplay(argc, argv);
        printf(result == 0 ? "yes\n" : "no\n");
        return EXIT_SUCCESS;
    }

    if (strncmp(argv[1], "battery", 7) == 0) {
        UpClient *upower = up_client_new();
        if (upower == NULL) {
            g_debug("Could not connect to upower");
            return 2;
        }

        int ret = EXIT_SUCCESS;

        if (strcmp(argv[1], "battery") == 0) {
            const gchar* status = get_battery_all(upower, NULL);
            if (status != NULL)
                g_print("%s\n", status);
        } else if (strcmp(argv[1], "battery_percentage") == 0) {
            gdouble percentage = get_battery_percentage(upower);
            g_print("%.2f\n", percentage);
        } else if (strcmp(argv[1], "battery_state") == 0) {
            batman_state_t state = get_battery_state(upower);
            g_print("%s\n", get_battery_state_str(state));
        }

        g_object_unref(upower);
        return ret;
    }

    if (strcmp(argv[1], "batman_active") == 0) {
        if (check_batman_active() != -1) {
            printf("Batman active status: %s\n", bm_state.active ? "active" : "inactive");
            return EXIT_SUCCESS;
        }
        printf("Failed to check Batman active status\n");
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "batman_enabled") == 0) {
        if (check_batman_enabled() != -1) {
            printf("Batman enabled status: %s\n", bm_state.enabled ? "enabled" : "disabled");
            return EXIT_SUCCESS;
        }
        printf("Failed to check Batman enabled status\n");
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "waydroid_state") == 0) {
        gchar *state = waydroid_get_state();
        if (state) {
            g_print("%s\n", state);
            g_free(state);
            return EXIT_SUCCESS;
        }
        g_print("Failed to get Waydroid state.\n");
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "waydroid_screen") == 0) {
        gboolean is_asleep = waydroid_screen_status();
        g_print("%s\n", is_asleep ? "False" : "True");
        return EXIT_SUCCESS;
    }

    print_usage(argv[0]);
    return EXIT_FAILURE;
}
