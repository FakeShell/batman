// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2024 Bardia Moshiri <fakeshell@bardia.tech>

#ifndef BATMAN_GBINDER_H
#define BATMAN_GBINDER_H

#include <gbinder.h>

// Power HIDL hints type
enum hints {
    INTERACTION = 0x00000002,
    POWERSAVE = 0x00000005,
    PERFORMANCE = 0x00000006
};

// Power AIDL mode type
enum mode {
    LOW_POWER = 1,
    SUSTAINED_PERFORMANCE = 2
};

// Power AIDL boost type
enum boost {
    INTERACTION_AIDL = 0,
};

void power_aidl(GBinderClient* client, const int interactive, const enum hints hint);
void power_hidl(GBinderClient* client, const int interactive, const enum hints hint);
int init_power_aidl(const int mode);
int init_power_hidl(const int mode);
void vr_hidl(GBinderClient* client, const int enabled);
int init_vr_hidl(const int mode);

#endif // BATMAN_GBINDER_H
