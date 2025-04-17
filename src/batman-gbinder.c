/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2025 Bardia Moshiri <bardia@furilabs.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "batman-gbinder.h"

#define MTKPOWER_HIDL_IFACE "vendor.mediatek.hardware.mtkpower@1.0::IMtkPower"
#define MTKPOWER_HIDL_NAME "default"
#define POWER_HIDL_IFACE "android.hardware.vibrator@1.0::IVibrator"
#define POWER_HIDL_NAME "default"
#define POWER_AIDL_IFACE "android.hardware.power.IPower"
#define POWER_AIDL_NAME "default"
#define VR_HIDL_IFACE "android.hardware.vr@1.0::IVr"
#define VR_HIDL_NAME "default"

BatmanGBinder *
batman_init_power_aidl(void)
{
    BatmanGBinder *ctx = malloc(sizeof(BatmanGBinder));
    if (!ctx)
        return NULL;

    ctx->sm = gbinder_servicemanager_new(GBINDER_DEFAULT_BINDER);
    if (!ctx->sm) {
        free(ctx);
        return NULL;
    }

    ctx->remote = gbinder_servicemanager_get_service_sync(ctx->sm, POWER_AIDL_IFACE "/" POWER_AIDL_NAME, NULL);
    if (!ctx->remote) {
        gbinder_servicemanager_unref(ctx->sm);
        free(ctx);
        return NULL;
    }

    ctx->client = gbinder_client_new(ctx->remote, POWER_AIDL_IFACE);
    if (!ctx->client) {
        gbinder_remote_object_unref(ctx->remote);
        gbinder_servicemanager_unref(ctx->sm);
        free(ctx);
        return NULL;
    }

    return ctx;
}

BatmanGBinder *
batman_init_power_hidl(void)
{
    BatmanGBinder *ctx = malloc(sizeof(BatmanGBinder));
    if (!ctx)
        return NULL;

    ctx->sm = gbinder_servicemanager_new(GBINDER_DEFAULT_HWBINDER);
    if (!ctx->sm) {
        free(ctx);
        return NULL;
    }

    ctx->remote = gbinder_servicemanager_get_service_sync(ctx->sm, POWER_HIDL_IFACE "/" POWER_HIDL_NAME, NULL);
    if (!ctx->remote) {
        gbinder_servicemanager_unref(ctx->sm);
        free(ctx);
        return NULL;
    }

    ctx->client = gbinder_client_new(ctx->remote, POWER_HIDL_IFACE);
    if (!ctx->client) {
        gbinder_remote_object_unref(ctx->remote);
        gbinder_servicemanager_unref(ctx->sm);
        free(ctx);
        return NULL;
    }

    return ctx;
}

BatmanGBinder *
batman_init_vr_hidl(void)
{
    BatmanGBinder *ctx = malloc(sizeof(BatmanGBinder));
    if (!ctx)
        return NULL;

    ctx->sm = gbinder_servicemanager_new(GBINDER_DEFAULT_HWBINDER);
    if (!ctx->sm) {
        free(ctx);
        return NULL;
    }

    ctx->remote = gbinder_servicemanager_get_service_sync(ctx->sm, VR_HIDL_IFACE "/" VR_HIDL_NAME, NULL);
    if (!ctx->remote) {
        gbinder_servicemanager_unref(ctx->sm);
        free(ctx);
        return NULL;
    }

    ctx->client = gbinder_client_new(ctx->remote, VR_HIDL_IFACE);
    if (!ctx->client) {
        gbinder_remote_object_unref(ctx->remote);
        gbinder_servicemanager_unref(ctx->sm);
        free(ctx);
        return NULL;
    }

    return ctx;
}

BatmanGBinder *
batman_init_mtkpower_hidl(void)
{
    BatmanGBinder *ctx = malloc(sizeof(BatmanGBinder));
    if (!ctx)
        return NULL;

    ctx->sm = gbinder_servicemanager_new(GBINDER_DEFAULT_HWBINDER);
    if (!ctx->sm) {
        free(ctx);
        return NULL;
    }

    ctx->remote = gbinder_servicemanager_get_service_sync(ctx->sm, MTKPOWER_HIDL_IFACE "/" MTKPOWER_HIDL_NAME, NULL);
    if (!ctx->remote) {
        gbinder_servicemanager_unref(ctx->sm);
        free(ctx);
        return NULL;
    }

    ctx->client = gbinder_client_new(ctx->remote, MTKPOWER_HIDL_IFACE);
    if (!ctx->client) {
        gbinder_remote_object_unref(ctx->remote);
        gbinder_servicemanager_unref(ctx->sm);
        free(ctx);
        return NULL;
    }

    return ctx;
}

void
batman_cleanup(BatmanGBinder *ctx)
{
    if (!ctx)
        return;
    if (ctx->client)
        gbinder_client_unref(ctx->client);
    if (ctx->remote)
        gbinder_remote_object_unref(ctx->remote);

    free(ctx);
}

int
batman_set_mode_aidl(BatmanGBinder *ctx, PowerMode mode)
{
    if (!ctx || !ctx->client)
        return -1;

    int status;
    GBinderLocalRequest* req;
    GBinderWriter writer;

    req = gbinder_client_new_request(ctx->client);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_int32(&writer, mode);
    gbinder_writer_append_int32(&writer, 1);
    gbinder_client_transact_sync_reply(ctx->client, SET_MODE, req, &status);
    gbinder_local_request_unref(req);

    return status;
}

int
batman_set_boost_aidl(BatmanGBinder *ctx, PowerBoost boost, int durationMs)
{
    if (!ctx || !ctx->client)
        return -1;

    int status;
    GBinderLocalRequest* req;
    GBinderWriter writer;

    req = gbinder_client_new_request(ctx->client);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_bool(&writer, boost);
    gbinder_writer_append_int32(&writer, durationMs);
    gbinder_client_transact_sync_reply(ctx->client, SET_BOOST, req, &status);
    gbinder_local_request_unref(req);

    return status;
}

int
batman_apply_performance_boost_aidl(BatmanGBinder *ctx, PowerBoost boost)
{
    if (!ctx || !ctx->client)
        return -1;

    /* Apply interaction boost when in sustained performance mode */
    return batman_set_boost_aidl(ctx, boost, 300000); /* boost for 5 minutes */
}

int
batman_set_interactive_hidl(BatmanGBinder *ctx, gboolean interactive)
{
    if (!ctx || !ctx->client)
        return -1;

    int status;
    GBinderLocalRequest* req;
    GBinderWriter writer;

    req = gbinder_client_new_request(ctx->client);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_bool(&writer, interactive);
    gbinder_client_transact_sync_reply(ctx->client, SET_INTERACTIVE, req, &status);
    gbinder_local_request_unref(req);

    return status;
}

int
batman_power_hint_hidl(BatmanGBinder *ctx, PowerHint hint, gboolean data)
{
    if (!ctx || !ctx->client)
        return -1;

    int status;
    GBinderLocalRequest* req;
    GBinderWriter writer;

    req = gbinder_client_new_request(ctx->client);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_int32(&writer, hint);
    gbinder_writer_append_int32(&writer, data);
    gbinder_client_transact_sync_reply(ctx->client, POWER_HINT, req, &status);
    gbinder_local_request_unref(req);

    return status;
}

int
batman_set_power_aidl(BatmanGBinder *ctx, PowerBoost boost, PowerMode mode)
{
    if (!ctx || !ctx->client)
        return -1;

    int status = batman_set_mode_aidl(ctx, mode);
    if (status != 0)
        return status;

    /* Apply interaction boost when in sustained performance mode */
    if (mode == SUSTAINED_PERFORMANCE_AIDL)
        status = batman_apply_performance_boost_aidl(ctx, boost);

    return status;
}

int
batman_set_power_hidl(BatmanGBinder *ctx, gboolean interactive, PowerHint hint)
{
    if (!ctx || !ctx->client)
        return -1;

    int status = batman_set_interactive_hidl(ctx, interactive);
    if (status != 0)
        return status;

    status = batman_power_hint_hidl(ctx, hint, interactive);
    return status;
}

int
batman_set_vr_hidl(BatmanGBinder *ctx, gboolean enabled)
{
    if (!ctx || !ctx->client)
        return -1;

    int status;
    GBinderLocalRequest* req;
    GBinderWriter writer;

    req = gbinder_client_new_request(ctx->client);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_client_transact_sync_reply(ctx->client, INIT, req, &status);
    gbinder_local_request_unref(req);

    req = gbinder_client_new_request(ctx->client);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_bool(&writer, enabled);
    gbinder_client_transact_sync_reply(ctx->client, SET_VR_MODE, req, &status);
    gbinder_local_request_unref(req);

    return 0;
}

int
batman_set_mtkpower_hint_hidl(BatmanGBinder *ctx, MtkPowerHint hint)
{
    if (!ctx || !ctx->client)
        return -1;

    int status;
    GBinderLocalRequest* req;
    GBinderWriter writer;

    req = gbinder_client_new_request(ctx->client);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_int32(&writer, hint);
    gbinder_writer_append_int32(&writer, 1);
    gbinder_client_transact_sync_reply(ctx->client, MTK_POWER_HINT, req, &status);
    gbinder_local_request_unref(req);

    return 0;
}

int
batman_set_power_saver(void)
{
    int ret = -1;
    BatmanGBinder *ctx = batman_init_power_aidl();

    if (ctx) {
        ret = batman_set_power_aidl(ctx, INTERACTION_AIDL, LOW_POWER_AIDL);
        batman_cleanup(ctx);
    } else {
        ctx = batman_init_power_hidl();
        if (ctx) {
            ret = batman_set_power_hidl(ctx, 0, LOW_POWER);
            batman_cleanup(ctx);
        }
    }

    return ret;
}

int
batman_set_performance(void)
{
    int ret = -1;
    BatmanGBinder *ctx = batman_init_power_aidl();

    if (ctx) {
        ret = batman_set_power_aidl(ctx, INTERACTION_AIDL, SUSTAINED_PERFORMANCE_AIDL);
        batman_cleanup(ctx);
    } else {
        ctx = batman_init_power_hidl();
        if (ctx) {
            ret = batman_set_power_hidl(ctx, 1, SUSTAINED_PERFORMANCE);
            batman_cleanup(ctx);
        }
    }

    return ret;
}

int
batman_set_vr_mode(gboolean enabled)
{
    int ret = -1;
    BatmanGBinder *ctx = batman_init_vr_hidl();

    if (ctx) {
        ret = batman_set_vr_hidl(ctx, enabled);
        batman_cleanup(ctx);
    }

    return ret;
}

int
batman_apply_mtkpower_hint(MtkPowerHint hint)
{
    int ret = -1;
    BatmanGBinder *ctx = batman_init_mtkpower_hidl();

    if (ctx) {
        ret = batman_set_mtkpower_hint_hidl(ctx, hint);
        batman_cleanup(ctx);
    }

    return ret;
}
