/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2026 Bardia Moshiri <bardia@furilabs.com>
 */

#include "binder.h"

#define MTKPOWER_HIDL_IFACE "vendor.mediatek.hardware.mtkpower@1.0::IMtkPower"
#define MTKPOWER_HIDL_NAME "default"
#define POWER_HIDL_IFACE "android.hardware.power@1.0::IPower"
#define POWER_HIDL_NAME "default"
#define POWER_AIDL_IFACE "android.hardware.power.IPower"
#define POWER_AIDL_NAME "default"
#define VR_HIDL_IFACE "android.hardware.vr@1.0::IVr"
#define VR_HIDL_NAME "default"

Binder *
binder_init_power_aidl(void)
{
    g_debug("binder: initializing Power AIDL");

    Binder *ctx = malloc(sizeof(Binder));
    if (!ctx)
        return NULL;

    ctx->idl_type = IDL_AIDL;

    ctx->sm = gbinder_servicemanager_new(GBINDER_DEFAULT_BINDER);
    if (!ctx->sm) {
        g_debug("binder: AIDL service manager creation failed");
        free(ctx);
        return NULL;
    }

    ctx->remote = gbinder_servicemanager_get_service_sync(ctx->sm, POWER_AIDL_IFACE "/" POWER_AIDL_NAME, NULL);
    if (!ctx->remote) {
        g_debug("binder: AIDL power service not found");
        gbinder_servicemanager_unref(ctx->sm);
        free(ctx);
        return NULL;
    }

    ctx->client = gbinder_client_new(ctx->remote, POWER_AIDL_IFACE);
    if (!ctx->client) {
        g_debug("binder: AIDL client creation failed");
        gbinder_remote_object_unref(ctx->remote);
        gbinder_servicemanager_unref(ctx->sm);
        free(ctx);
        return NULL;
    }

    g_debug("binder: Power AIDL initialized successfully");

    return ctx;
}

Binder *
binder_init_power_hidl(void)
{
    g_debug("binder: initializing Power HIDL");

    Binder *ctx = malloc(sizeof(Binder));
    if (!ctx)
        return NULL;

    ctx->idl_type = IDL_HIDL;

    ctx->sm = gbinder_servicemanager_new(GBINDER_DEFAULT_HWBINDER);
    if (!ctx->sm) {
        g_debug("binder: HIDL service manager creation failed");
        free(ctx);
        return NULL;
    }

    ctx->remote = gbinder_servicemanager_get_service_sync(ctx->sm, POWER_HIDL_IFACE "/" POWER_HIDL_NAME, NULL);
    if (!ctx->remote) {
        g_debug("binder: HIDL power service not found");
        gbinder_servicemanager_unref(ctx->sm);
        free(ctx);
        return NULL;
    }

    ctx->client = gbinder_client_new(ctx->remote, POWER_HIDL_IFACE);
    if (!ctx->client) {
        g_debug("binder: HIDL client creation failed");
        gbinder_remote_object_unref(ctx->remote);
        gbinder_servicemanager_unref(ctx->sm);
        free(ctx);
        return NULL;
    }

    g_debug("binder: Power HIDL initialized successfully");

    return ctx;
}

Binder *
binder_init_vr_hidl(void)
{
    g_debug("binder: initializing VR HIDL");

    Binder *ctx = malloc(sizeof(Binder));
    if (!ctx)
        return NULL;

    ctx->idl_type = IDL_HIDL;

    ctx->sm = gbinder_servicemanager_new(GBINDER_DEFAULT_HWBINDER);
    if (!ctx->sm) {
        g_debug("binder: VR service manager creation failed");
        free(ctx);
        return NULL;
    }

    ctx->remote = gbinder_servicemanager_get_service_sync(ctx->sm, VR_HIDL_IFACE "/" VR_HIDL_NAME, NULL);
    if (!ctx->remote) {
        g_debug("binder: VR service not found");
        gbinder_servicemanager_unref(ctx->sm);
        free(ctx);
        return NULL;
    }

    ctx->client = gbinder_client_new(ctx->remote, VR_HIDL_IFACE);
    if (!ctx->client) {
        g_debug("binder: VR client creation failed");
        gbinder_remote_object_unref(ctx->remote);
        gbinder_servicemanager_unref(ctx->sm);
        free(ctx);
        return NULL;
    }

    g_debug("binder: VR HIDL initialized successfully");

    return ctx;
}

Binder *
binder_init_mtkpower_hidl(void)
{
    g_debug("binder: initializing MTK Power HIDL");

    Binder *ctx = malloc(sizeof(Binder));
    if (!ctx)
        return NULL;

    ctx->idl_type = IDL_HIDL;

    ctx->sm = gbinder_servicemanager_new(GBINDER_DEFAULT_HWBINDER);
    if (!ctx->sm) {
        g_debug("binder: MTK service manager creation failed");
        free(ctx);
        return NULL;
    }

    ctx->remote = gbinder_servicemanager_get_service_sync(ctx->sm, MTKPOWER_HIDL_IFACE "/" MTKPOWER_HIDL_NAME, NULL);
    if (!ctx->remote) {
        g_debug("binder: MTK power service not found");
        gbinder_servicemanager_unref(ctx->sm);
        free(ctx);
        return NULL;
    }

    ctx->client = gbinder_client_new(ctx->remote, MTKPOWER_HIDL_IFACE);
    if (!ctx->client) {
        g_debug("binder: MTK client creation failed");
        gbinder_remote_object_unref(ctx->remote);
        gbinder_servicemanager_unref(ctx->sm);
        free(ctx);
        return NULL;
    }

    g_debug("binder: MTK Power HIDL initialized successfully");

    return ctx;
}

void
binder_cleanup(Binder *ctx)
{
    if (!ctx)
        return;

    if (ctx->client)
        gbinder_client_unref(ctx->client);
    if (ctx->remote)
        gbinder_remote_object_unref(ctx->remote);
    if (ctx->sm)
        gbinder_servicemanager_unref(ctx->sm);

    free(ctx);
}

Binder *
binder_init(void)
{
    Binder *ctx;

    ctx = binder_init_power_aidl();
    if (ctx)
        return ctx;

    ctx = binder_init_power_hidl();
    if (ctx)
        return ctx;

    return NULL;
}

int
binder_set_mode_aidl(Binder *ctx, PowerMode mode)
{
    if (!ctx || !ctx->client)
        return -1;

    int status = -1;
    GBinderLocalRequest* req = NULL;
    GBinderWriter writer;
    GBinderRemoteReply* reply = NULL;

    req = gbinder_client_new_request(ctx->client);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_int32(&writer, mode);
    gbinder_writer_append_int32(&writer, 1);

    reply = gbinder_client_transact_sync_reply(ctx->client, SET_MODE, req, &status);
    if (reply)
        gbinder_remote_reply_unref(reply);

    gbinder_local_request_unref(req);
    return status;
}

int
binder_set_boost_aidl(Binder *ctx, PowerBoost boost, int durationMs)
{
    if (!ctx || !ctx->client)
        return -1;

    int status = -1;
    GBinderLocalRequest* req = NULL;
    GBinderWriter writer;
    GBinderRemoteReply* reply = NULL;

    req = gbinder_client_new_request(ctx->client);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_int32(&writer, boost);
    gbinder_writer_append_int32(&writer, durationMs);

    reply = gbinder_client_transact_sync_reply(ctx->client, SET_BOOST, req, &status);
    if (reply)
        gbinder_remote_reply_unref(reply);

    gbinder_local_request_unref(req);
    return status;
}

int
binder_apply_performance_boost_aidl(Binder *ctx, PowerBoost boost)
{
    if (!ctx || !ctx->client)
        return -1;

    /* Apply interaction boost when in sustained performance mode */
    return binder_set_boost_aidl(ctx, boost, 300000); /* boost for 5 minutes */
}

int
binder_set_interactive_hidl(Binder *ctx, gboolean interactive)
{
    if (!ctx || !ctx->client)
        return -1;

    int status = -1;
    GBinderLocalRequest* req = NULL;
    GBinderWriter writer;
    GBinderRemoteReply* reply = NULL;

    req = gbinder_client_new_request(ctx->client);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_bool(&writer, interactive);

    reply = gbinder_client_transact_sync_reply(ctx->client, SET_INTERACTIVE, req, &status);
    if (reply)
        gbinder_remote_reply_unref(reply);

    gbinder_local_request_unref(req);
    return status;
}

int
binder_power_hint_hidl(Binder *ctx, PowerHint hint, gboolean data)
{
    if (!ctx || !ctx->client)
        return -1;

    int status = -1;
    GBinderLocalRequest* req = NULL;
    GBinderWriter writer;
    GBinderRemoteReply* reply = NULL;

    req = gbinder_client_new_request(ctx->client);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_int32(&writer, hint);
    gbinder_writer_append_int32(&writer, data);

    reply = gbinder_client_transact_sync_reply(ctx->client, POWER_HINT, req, &status);
    if (reply)
        gbinder_remote_reply_unref(reply);

    gbinder_local_request_unref(req);
    return status;
}

int
binder_set_power_aidl(Binder *ctx, PowerBoost boost, PowerMode mode)
{
    if (!ctx || !ctx->client)
        return -1;

    int status = binder_set_mode_aidl(ctx, mode);
    if (status != 0)
        return status;

    /* Apply interaction boost when in sustained performance mode */
    if (mode == SUSTAINED_PERFORMANCE_AIDL)
        status = binder_apply_performance_boost_aidl(ctx, boost);

    return status;
}

int
binder_set_power_hidl(Binder *ctx, gboolean interactive, PowerHint hint)
{
    if (!ctx || !ctx->client)
        return -1;

    int status = binder_set_interactive_hidl(ctx, interactive);
    if (status != 0)
        return status;

    status = binder_power_hint_hidl(ctx, hint, interactive);
    return status;
}

int
binder_set_vr_hidl(Binder *ctx, gboolean enabled)
{
    if (!ctx || !ctx->client)
        return -1;

    int status = 0;
    GBinderLocalRequest* req = NULL;
    GBinderWriter writer;
    GBinderRemoteReply* reply = NULL;

    req = gbinder_client_new_request(ctx->client);
    gbinder_local_request_init_writer(req, &writer);
    reply = gbinder_client_transact_sync_reply(ctx->client, INIT, req, &status);
    if (reply)
        gbinder_remote_reply_unref(reply);

    gbinder_local_request_unref(req);

    req = gbinder_client_new_request(ctx->client);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_bool(&writer, enabled);
    reply = gbinder_client_transact_sync_reply(ctx->client, SET_VR_MODE, req, &status);
    if (reply)
        gbinder_remote_reply_unref(reply);

    gbinder_local_request_unref(req);

    return status;
}

int
binder_set_mtkpower_hint_hidl(Binder *ctx, MtkPowerHint hint)
{
    if (!ctx || !ctx->client)
        return -1;

    int status = 0;
    GBinderLocalRequest* req = NULL;
    GBinderWriter writer;
    GBinderRemoteReply* reply = NULL;

    req = gbinder_client_new_request(ctx->client);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_int32(&writer, hint);
    gbinder_writer_append_int32(&writer, 1);

    reply = gbinder_client_transact_sync_reply(ctx->client, MTK_POWER_HINT, req, &status);
    if (reply)
        gbinder_remote_reply_unref(reply);

    gbinder_local_request_unref(req);
    return status;
}

int
binder_set_power_saver(Binder *ctx)
{
    if (!ctx)
        return -1;

    if (ctx->idl_type == IDL_AIDL) {
        g_debug("binder: setting power saver via AIDL");
        return binder_set_power_aidl(ctx, INTERACTION_AIDL, LOW_POWER_AIDL);
    } else {
        g_debug("binder: setting power saver via HIDL");
        return binder_set_power_hidl(ctx, 0, LOW_POWER);
    }
}

int
binder_set_performance(Binder *ctx)
{
    if (!ctx)
        return -1;

    if (ctx->idl_type == IDL_AIDL) {
        g_debug("binder: setting performance via AIDL");
        return binder_set_power_aidl(ctx, INTERACTION_AIDL, SUSTAINED_PERFORMANCE_AIDL);
    } else {
        g_debug("binder: setting performance via HIDL");
        return binder_set_power_hidl(ctx, 1, SUSTAINED_PERFORMANCE);
    }
}
