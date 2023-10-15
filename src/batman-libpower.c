// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2023 Bardia Moshiri <fakeshell@bardia.tech>

#include <stdio.h>
#include <gbinder.h>
#include <stdlib.h>
#include <stdint.h>

enum hints {
    INTERACTION = 0x00000002,
    POWERSAVE = 0x00000005,
    PERFORMANCE = 0x00000006
};

void set_modes(GBinderClient* client, const int interactive, const enum hints hint) {
    int status;
    GBinderLocalRequest* req = gbinder_client_new_request(client);
    GBinderWriter writer;

    // interactive mode
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_bool(&writer, interactive);
    gbinder_client_transact_sync_reply(client, 1, req, &status);
    gbinder_local_request_unref(req);

    // hints
    req = gbinder_client_new_request(client);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_int32(&writer, (uint32_t) hint);
    gbinder_writer_append_int32(&writer, interactive);
    gbinder_client_transact_sync_reply(client, 2, req, &status);
    gbinder_local_request_unref(req);
}

int main(int argc, char *argv[]) {
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

    if (argc != 2) {
        printf("Usage: %s <0|1> (0 for non-interactive + powersave, 1 for interactive + performance)\n", argv[0]);
        return 1;
    }

    int choice = atoi(argv[1]);

    if (choice == 0) {
        set_modes(client, 0, POWERSAVE);
    } else if (choice == 1) {
        set_modes(client, 1, PERFORMANCE);
    } else {
        printf("Invalid argument. Use 0 for non-interactive + powersave or 1 for interactive + performance.\n");
        return 1;
    }

    gbinder_client_unref(client);
    gbinder_remote_object_unref(remote);

    return 0;
}
