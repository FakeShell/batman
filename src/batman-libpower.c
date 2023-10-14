// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2023 Bardia Moshiri <fakeshell@bardia.tech>

#include <stdio.h>
#include <gbinder.h>
#include <stdlib.h>

// gcc libpower.c `pkg-config --libs --cflags libgbinder`
void mode_setter(GBinderClient* client, const int enabled) {
    int status;
    GBinderLocalRequest* req = gbinder_client_new_request(client);
    GBinderWriter writer;

    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_bool(&writer, enabled);
    gbinder_client_transact_sync_reply(client, 1, req, &status);
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
        printf("Usage: %s <0|1> (0 for disabled, 1 for enabled)\n", argv[0]);
        return 1;
    }

    int choice = atoi(argv[1]);

    if (choice == 0) {
        mode_setter(client, 0);
    } else if (choice == 1) {
        mode_setter(client, 1);
    } else {
        printf("Invalid argument. Use 0 to disable or 1 to enable interactive mode.\n");
    }

    gbinder_client_unref(client);
    gbinder_remote_object_unref(remote);
    return 0;
}
