// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2024 Bardia Moshiri <fakeshell@bardia.tech>
// Copyright (C) 2023 Erik Inkinen <erik.inkinen@erikinkinen.fi>

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <glob.h>
#include <glib.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/sysinfo.h>
#include "getinfo.h"

BatmanState bm_state;

int check_batman_active() {
    char *line = NULL;
    size_t len = 0;
    int result = 0;
    FILE *file = popen("systemctl is-active batman", "r");
    if (!file) return -1;

    if (getline(&line, &len, file) != -1) {
        if (strcmp(line, "active\n") == 0)
            bm_state.active = TRUE;
        else bm_state.active = FALSE;
    } else result = -1;

    pclose(file);
    free(line);
    return result;
}

int check_batman_enabled() {
    char *line = NULL;
    size_t len = 0;
    int result = 0;
    FILE *file = popen("systemctl is-enabled batman", "r");
    if (!file) return -1;

    if (getline(&line, &len, file) != -1) {
        if (strcmp(line, "enabled\n") == 0)
            bm_state.enabled = TRUE;
        else bm_state.enabled = FALSE;
    } else result = -1;

    pclose(file);
    free(line);
    return result;
}
