// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2023 Bardia Moshiri <fakeshell@bardia.tech>

#include <stdio.h>
#include <gbinder.h>
#include <stdlib.h>
#include <stdint.h>

void set_modes_hidl(GBinderClient* client, const int enabled) {
    int status;
    GBinderLocalRequest* req = gbinder_client_new_request(client);
    GBinderWriter writer;

    // init
    gbinder_local_request_init_writer(req, &writer);
    gbinder_client_transact_sync_reply(client, 1, req, &status);
    gbinder_local_request_unref(req);

    // setVrMode
    req = gbinder_client_new_request(client);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_bool(&writer, enabled);
    gbinder_client_transact_sync_reply(client, 2, req, &status);
    gbinder_local_request_unref(req);
}

int init_hidl(const int mode) {
    GBinderServiceManager* sm = gbinder_servicemanager_new("/dev/hwbinder");
    if (!sm) return 1;

    GBinderRemoteObject* remote = gbinder_servicemanager_get_service_sync(sm, "android.hardware.vr@1.0::IVr/default", NULL);
    if (!remote) {
        gbinder_servicemanager_unref(sm);
        return 1;
    }

    GBinderClient* client = gbinder_client_new(remote, "android.hardware.vr@1.0::IVr");
    if (!client) {
        gbinder_remote_object_unref(remote);
        gbinder_servicemanager_unref(sm);
        return 1;
    }

    set_modes_hidl(client, mode);

    gbinder_client_unref(client);
    gbinder_remote_object_unref(remote);

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <0|1> (0 for vr mode off, 1 for vr mode on)\n", argv[0]);
        return 1;
    }

    int choice = atoi(argv[1]);

    if (choice == 0 || choice == 1) {
        int ret = init_hidl(choice);

        if (ret != 0) {
            printf("None of the backends are available. exiting\n");
            return 1;
        } else {
            printf("Using HIDL backend\n");
        }
    } else {
        printf("Invalid argument. Use 0 for vr mode off or 1 for vr mode on\n");
        return 1;
    }

    return 0;
}
