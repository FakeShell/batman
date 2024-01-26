// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2024 Bardia Moshiri <fakeshell@bardia.tech>

#ifndef BATMAN_WRAPPER_H
#define BATMAN_WRAPPER_H

#include <upower.h>

const gchar *findBattery(UpClient *upower, gdouble *percentage);

double cpuUsage();
long double memUsage();

#endif // BATMAN_WRAPPER_H
