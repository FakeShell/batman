// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2023 Bardia Moshiri <fakeshell@bardia.tech>

#include <stdio.h>
#include <gbinder.h>
#include <stdlib.h>
#include <stdint.h>

// HIDL
enum hints {
    INTERACTION = 0x00000002,
    POWERSAVE = 0x00000005,
    PERFORMANCE = 0x00000006
};

// AIDL
enum mode {
    LOW_POWER = 1,
    SUSTAINED_PERFORMANCE = 2
};

// AIDL
enum boost {
    INTERACTION_AIDL = 0,
};

void set_modes_aidl(GBinderClient* client, const int interactive, const enum hints hint) {
    int status;
    GBinderLocalRequest* req = gbinder_client_new_request(client);
    GBinderWriter writer;

    if (hint == SUSTAINED_PERFORMANCE) {
        // interactive mode
        gbinder_local_request_init_writer(req, &writer);
        gbinder_writer_append_bool(&writer, interactive);
        gbinder_writer_append_int32(&writer, 300000); // boost for 5 minutes
        gbinder_client_transact_sync_reply(client, 3, req, &status); // setBoost
        gbinder_local_request_unref(req);
    }

    // disable other modes
    enum hints other_mode = (hint == LOW_POWER) ? SUSTAINED_PERFORMANCE : LOW_POWER;
    req = gbinder_client_new_request(client);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_int32(&writer, (uint32_t) other_mode);
    gbinder_writer_append_int32(&writer, 0);
    gbinder_client_transact_sync_reply(client, 1, req, &status); // setMode
    gbinder_local_request_unref(req);

    // set requested mode
    req = gbinder_client_new_request(client);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_int32(&writer, (uint32_t) hint);
    gbinder_writer_append_int32(&writer, 1);
    gbinder_client_transact_sync_reply(client, 1, req, &status); // setMode
    gbinder_local_request_unref(req);
}

void set_modes_hidl(GBinderClient* client, const int interactive, const enum hints hint) {
    int status;
    GBinderLocalRequest* req = gbinder_client_new_request(client);
    GBinderWriter writer;

    // interactive mode
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_bool(&writer, interactive);
    gbinder_client_transact_sync_reply(client, 1, req, &status); // setInteractive
    gbinder_local_request_unref(req);

    // hints
    req = gbinder_client_new_request(client);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_int32(&writer, (uint32_t) hint);
    gbinder_writer_append_int32(&writer, interactive);
    gbinder_client_transact_sync_reply(client, 2, req, &status); // powerHint
    gbinder_local_request_unref(req);
}

int init_aidl(const int mode) {
    GBinderServiceManager* sm = gbinder_servicemanager_new("/dev/binder");
    if (!sm) return 1;

    GBinderRemoteObject* remote = gbinder_servicemanager_get_service_sync(sm, "android.hardware.power.IPower/default", NULL);
    if (!remote) {
        gbinder_servicemanager_unref(sm);
        return 1;
    }

    GBinderClient* client = gbinder_client_new(remote, "android.hardware.power.IPower");
    if (!client) {
        gbinder_remote_object_unref(remote);
        gbinder_servicemanager_unref(sm);
        return 1;
    }

    if (mode == 0) {
        set_modes_aidl(client, INTERACTION_AIDL, LOW_POWER);
    } else if (mode == 1) {
        set_modes_aidl(client, INTERACTION_AIDL, SUSTAINED_PERFORMANCE);
    }

    gbinder_client_unref(client);
    gbinder_remote_object_unref(remote);

    return 0;
}

// mode: 0 -> POWERSAVE, 1 -> INTERACTIVE
int init_hidl(const int mode) {
    GBinderServiceManager* sm = gbinder_servicemanager_new("/dev/hwbinder");
    if (!sm) return 1;

    GBinderRemoteObject* remote = gbinder_servicemanager_get_service_sync(sm, "android.hardware.power@1.0::IPower/default", NULL);
    if (!remote) {
        gbinder_servicemanager_unref(sm);
        return 1;
    }

    GBinderClient* client = gbinder_client_new(remote, "android.hardware.power@1.0::IPower");
    if (!client) {
        gbinder_remote_object_unref(remote);
        gbinder_servicemanager_unref(sm);
        return 1;
    }

    if (mode == 0) {
        set_modes_hidl(client, 0, POWERSAVE);
    } else if (mode == 1) {
        set_modes_hidl(client, 1, PERFORMANCE);
    }

    gbinder_client_unref(client);
    gbinder_remote_object_unref(remote);

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <0|1> (0 for non-interactive + powersave, 1 for interactive + performance)\n", argv[0]);
        return 1;
    }

    int choice = atoi(argv[1]);

    if (choice == 0 || choice == 1) {
        int ret = init_aidl(choice);
        if (ret != 0) {
            int ret = init_hidl(choice);

            if (ret != 0) {
                printf("None of the backends are available. exiting\n");
                return 1;
            } else {
                printf("Using HIDL backend\n");
            }
        } else {
            printf("Using AIDL backend\n");
        }
    } else {
        printf("Invalid argument. Use 0 for non-interactive + powersave or 1 for interactive + performance.\n");
        return 1;
    }

    return 0;
}
