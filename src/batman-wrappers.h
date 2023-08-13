// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2023 Bardia Moshiri <fakeshell@bardia.tech>

#ifndef BATMAN_WRAPPER_H
#define BATMAN_WRAPPER_H

#include <upower.h>

struct meminfo;

#ifdef WITH_UPOWER
UpDevice *findBattery(UpClient *upower);
#endif

int readMemInfo(struct meminfo *mem);
long long getTotalCPUTime();
long long getIdleCPUTime();
int cpuUsage();
int memUsage();

#endif // BATMAN_WRAPPER_H
