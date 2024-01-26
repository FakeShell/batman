// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2024 Bardia Moshiri <fakeshell@bardia.tech>
// Copyright (C) 2023 Erik Inkinen <erik.inkinen@erikinkinen.fi>

#ifndef GETINFO_H
#define GETINFO_H

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <glob.h>
#include <glib.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/sysinfo.h>

typedef struct {
    gboolean active;
    gboolean enabled;
} BatmanState;

extern BatmanState bm_state;

int check_batman_active();
int check_batman_enabled();

#endif /* GETINFO_H */
