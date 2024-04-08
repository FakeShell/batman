// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2024 Bardia Moshiri <fakeshell@bardia.tech>

#include <stdio.h>
#include "batman-waydroid.h"

#define WAYDROID_DBUS_NAME          "id.waydro.Container"
#define WAYDROID_DBUS_PATH          "/ContainerManager"
#define WAYDROID_DBUS_INTERFACE     "id.waydro.ContainerManager"

// yes this is stupid, once batman is rewritten this will be dropped
int main(int argc, char *argv[]) {
    if (argc != 2) {
        g_print("Usage: %s <0|1>\n0 to unfreeze, 1 to freeze\n", argv[0]);
        return 1;
    }

    gboolean freeze = g_strcmp0(argv[1], "1") == 0;
    waydroid_freezer(freeze);

    return 0;
}
