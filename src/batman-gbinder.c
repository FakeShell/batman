// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2024 Bardia Moshiri <fakeshell@bardia.tech>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "batman-gbinder.h"

void power_aidl(GBinderClient* client, const int interactive, const enum hints hint) {
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

void power_hidl(GBinderClient* client, const int interactive, const enum hints hint) {
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

int init_power_aidl(const int mode) {
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
        power_aidl(client, INTERACTION_AIDL, LOW_POWER);
    } else if (mode == 1) {
        power_aidl(client, INTERACTION_AIDL, SUSTAINED_PERFORMANCE);
    }

    gbinder_client_unref(client);
    gbinder_remote_object_unref(remote);

    return 0;
}

// mode: 0 -> POWERSAVE, 1 -> INTERACTIVE
int init_power_hidl(const int mode) {
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
        power_hidl(client, 0, POWERSAVE);
    } else if (mode == 1) {
        power_hidl(client, 1, PERFORMANCE);
    }

    gbinder_client_unref(client);
    gbinder_remote_object_unref(remote);

    return 0;
}

void vr_hidl(GBinderClient* client, const int enabled) {
    int status;
    GBinderLocalRequest* req = gbinder_client_new_request(client);
    GBinderWriter writer;

    gbinder_local_request_init_writer(req, &writer);
    gbinder_client_transact_sync_reply(client, 1, req, &status); // init
    gbinder_local_request_unref(req);

    req = gbinder_client_new_request(client);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_bool(&writer, enabled);
    gbinder_client_transact_sync_reply(client, 2, req, &status); // setVrMode
    gbinder_local_request_unref(req);
}

int init_vr_hidl(const int mode) {
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

    vr_hidl(client, mode);

    gbinder_client_unref(client);
    gbinder_remote_object_unref(remote);

    return 0;
}

void radio_hidl(GBinderClient* client, const int type, const int enabled) {
    int status;
    GBinderLocalRequest* req = gbinder_client_new_request(client);
    GBinderWriter writer;

    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_int32(&writer, 1); // serial
    gbinder_writer_append_int32(&writer, type);
    gbinder_writer_append_bool(&writer, enabled);
    gbinder_client_transact_sync_reply(client, 128, req, &status); // sendDeviceState
    gbinder_local_request_unref(req);
}

void radio_aidl(GBinderClient* client, const int type, const int enabled) {
    int status;
    GBinderLocalRequest* req = gbinder_client_new_request(client);
    GBinderWriter writer;

    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_int32(&writer, 1); // serial
    gbinder_writer_append_int32(&writer, type);
    gbinder_writer_append_bool(&writer, enabled);
    gbinder_client_transact_sync_reply(client, 14, req, &status); // sendDeviceState
    gbinder_local_request_unref(req);
}

int init_radio_hidl(const int type, const int mode) {
    GBinderServiceManager* sm = gbinder_servicemanager_new("/dev/hwbinder");
    if (!sm) return 1;

    GBinderRemoteObject* remote = gbinder_servicemanager_get_service_sync(sm, "android.hardware.radio@1.0::IRadio/slot1", NULL);
    if (!remote) {
        gbinder_servicemanager_unref(sm);
        return 1;
    }

    GBinderClient* client = gbinder_client_new(remote, "android.hardware.radio@1.0::IRadio");
    if (!client) {
        gbinder_remote_object_unref(remote);
        gbinder_servicemanager_unref(sm);
        return 1;
    }

    radio_hidl(client, type, mode);

    gbinder_client_unref(client);
    gbinder_remote_object_unref(remote);

    return 0;
}

int init_radio_aidl(const int type, const int mode) {
    GBinderServiceManager* sm = gbinder_servicemanager_new("/dev/binder");
    if (!sm) return 1;

    GBinderRemoteObject* remote = gbinder_servicemanager_get_service_sync(sm, "android.hardware.radio.modem.IRadioModem/default", NULL);
    if (!remote) {
        gbinder_servicemanager_unref(sm);
        return 1;
    }

    GBinderClient* client = gbinder_client_new(remote, "android.hardware.radio.modem.IRadioModem");
    if (!client) {
        gbinder_remote_object_unref(remote);
        gbinder_servicemanager_unref(sm);
        return 1;
    }

    radio_aidl(client, type, mode);

    gbinder_client_unref(client);
    gbinder_remote_object_unref(remote);

    return 0;
}

static GBinderLocalReply *
tetheroffload_hidl_callback (GBinderLocalObject   *obj,
                                       GBinderRemoteRequest *req,
                                       guint                 code,
                                       guint                 flags,
                                       int                  *status,
                                       void                 *user_data)
{
  /* Unused for now */

  return NULL;
}

void tetheroffload_hidl(GBinderClient* client, const int enabled, GBinderLocalObject* callback_object) {
    int status;
    GBinderLocalRequest* req = gbinder_client_new_request(client);
    GBinderWriter writer;

    if (enabled == 1) {
        gbinder_local_request_init_writer(req, &writer);
        gbinder_local_request_append_local_object(req, callback_object); // required to call this method. unused but the function signature has it either way
        gbinder_client_transact_sync_reply(client, 1, req, &status); // initOffload
        gbinder_local_request_unref(req);
    } else if (enabled == 0) {
        gbinder_local_request_init_writer(req, &writer);
        gbinder_client_transact_sync_reply(client, 2, req, &status); // stopOffload
        gbinder_local_request_unref(req);
    }
}

int init_tetheroffload_hidl(const int mode) {
    GBinderServiceManager* sm = gbinder_servicemanager_new("/dev/hwbinder");
    if (!sm) return 1;

    GBinderRemoteObject* remote = gbinder_servicemanager_get_service_sync(sm, "android.hardware.tetheroffload.control@1.0::IOffloadControl/default", NULL);
    if (!remote) {
        gbinder_servicemanager_unref(sm);
        return 1;
    }

    GBinderClient* client = gbinder_client_new(remote, "android.hardware.tetheroffload.control@1.0::IOffloadControl");
    if (!client) {
        gbinder_remote_object_unref(remote);
        gbinder_servicemanager_unref(sm);
        return 1;
    }

    GBinderLocalObject* callback_object = gbinder_servicemanager_new_local_object (sm,
                                           "android.hardware.tetheroffload.control@1.0::ITetheringOffloadCallback",
                                           tetheroffload_hidl_callback,
                                           NULL);

    tetheroffload_hidl(client, mode, callback_object);

    gbinder_client_unref(client);
    gbinder_remote_object_unref(remote);

    return 0;
}
