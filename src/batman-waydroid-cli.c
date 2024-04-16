// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2024 Bardia Moshiri <fakeshell@bardia.tech>

#include <stdio.h>
#include "batman-waydroid.h"

// yes this is stupid, once batman is rewritten this will be dropped
int main(int argc, char *argv[]) {
    if (argc != 3) {
        g_print("Usage: %s <screen|freeze> <0|1>\n0 to unfreeze/screen off, 1 to freeze/screen on\n", argv[0]);
        return 1;
    }

    gboolean state = g_strcmp0(argv[2], "1") == 0;

    if (g_strcmp0(argv[1], "freeze") == 0) {
        waydroid_freezer(state);
    } else if (g_strcmp0(argv[1], "screen") == 0) {
        waydroid_screen(state);
    } else {
        g_print("Unsupported operation\n");
        return 1;
    }

    return 0;
}
