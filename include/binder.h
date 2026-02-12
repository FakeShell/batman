/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2026 Bardia Moshiri <bardia@furilabs.com>
 */

#ifndef BINDER_H
#define BINDER_H

#include <gbinder.h>

/**
 * Power HIDL 1.0 function indices
 */
typedef enum {
    SET_INTERACTIVE = 1,
    POWER_HINT = 2,
    SET_FEATURE = 3,
    GET_PLATFORM_LOW_POWER_STATS = 4
} PowerHalFunctions1_0;

/**
 * Power AIDL function indices
 */
typedef enum {
    SET_MODE = 1,
    IS_MODE_SUPPORTED = 2,
    SET_BOOST = 3,
    IS_BOOST_SUPPORTED = 4,
    CREATE_HINT_SESSION = 5,
    GET_HINT_SESSION_PREFERRED_RATE = 6,
    CREATE_HINT_SESSION_WITH_CONFIG = 7,
    GET_SESSION_CHANNEL = 8,
    CLOSE_SESSION_CHANNEL = 9
} PowerHalFunctionsAidl;

/**
 * VR HIDL 1.0 function indices
 */
typedef enum {
    INIT = 1,
    SET_VR_MODE = 2
} VrFunctions1_0;

/**
 * MTK Power HIDL function indices
 */
typedef enum {
    MTK_POWER_HINT = 1,
    MTK_CUS_POWER_HINT = 2,
    NOTIFY_APP_STATE = 3,
    QUERY_SYS_INFO = 4,
    SET_SYS_INFO = 5,
    SET_SYS_INFO_ASYNC = 6
} MtkPowerFunctions1_0;

/**
 * IDL type for binder context
 */
typedef enum {
    IDL_HIDL = 0,
    IDL_AIDL = 1
} IdlType;

/**
 * Power HIDL hints type
 */
typedef enum {
    /**
     * Foreground app has started or stopped requesting a VSYNC pulse
     * from SurfaceFlinger. If the app has started requesting VSYNC
     * then CPU and GPU load is expected soon, and it may be appropriate
     * to raise speeds of CPU, memory bus, etc. The data parameter is
     * non-zero to indicate VSYNC pulse is now requested, or zero for
     * VSYNC pulse no longer requested.
     */
    VSYNC = 0x00000001,

    /**
     * User is interacting with the device, for example, touchscreen
     * events are incoming. CPU and GPU load may be expected soon,
     * and it may be appropriate to raise speeds of CPU, memory bus,
     * etc. The data parameter is the estimated length of the interaction
     * in milliseconds, or 0 if unknown.
     */
    INTERACTION = 0x00000002,

    /**
     * Video encode is happening. The data parameter is non-zero if
     * video encode is active, and zero otherwise.
     * @deprecated This hint is deprecated in newer HAL versions.
     */
    VIDEO_ENCODE = 0x00000003,

    /**
     * Video decode is happening. The data parameter is non-zero if
     * video decode is active, and zero otherwise.
     * @deprecated This hint is deprecated in newer HAL versions.
     */
    VIDEO_DECODE = 0x00000004,

    /**
     * Low power mode is activated or deactivated. Low power mode
     * is intended to save battery at the cost of performance. The data
     * parameter is non-zero when low power mode is activated, and zero
     * when deactivated.
     */
    LOW_POWER = 0x00000005,

    /**
     * Sustained Performance mode is actived or deactivated. Sustained
     * performance mode is intended to provide a consistent level of
     * performance for a prolonged amount of time. The data parameter is
     * non-zero when sustained performance mode is activated, and zero
     * when deactivated.
     */
    SUSTAINED_PERFORMANCE = 0x00000006,

    /**
     * VR Mode is activated or deactivated. VR mode is intended to
     * provide minimum guarantee for performance for the amount of time the
     * device can sustain it. The data parameter is non-zero when the mode
     * is activated and zero when deactivated.
     */
    VR_MODE = 0x00000007,

    /**
     * This hint indicates that an application has been launched. Can be used
     * for device specific optimizations during application launch. The data
     * parameter is non-zero when the application starts to launch and zero when
     * it has been launched.
     */
    LAUNCH = 0x00000008
} PowerHint;

/**
 * Power AIDL mode types as defined in Android AIDL interface
 */
typedef enum {
    /**
     * Double tap to wake mode
     */
    DOUBLE_TAP_TO_WAKE_AIDL = 0,

    /**
     * Low power mode - saves battery at the cost of performance
     */
    LOW_POWER_AIDL = 1,

    /**
     * Sustained performance mode - provides consistent performance for prolonged periods
     */
    SUSTAINED_PERFORMANCE_AIDL = 2,

    /**
     * Fixed performance mode - locks CPU/GPU frequencies
     */
    FIXED_PERFORMANCE_AIDL = 3,

    /**
     * VR mode - ensures minimum performance guarantees for VR applications
     */
    VR_AIDL = 4,

    /**
     * Launch mode - optimizes for app launches
     */
    LAUNCH_AIDL = 5,

    /**
     * Expensive rendering mode - indicates app is doing expensive rendering
     */
    EXPENSIVE_RENDERING_AIDL = 6,

    /**
     * Interactive mode - device is actively being used
     */
    INTERACTIVE_AIDL = 7,

    /**
     * Device idle mode - device is in doze/idle state
     */
    DEVICE_IDLE_AIDL = 8,

    /**
     * Display inactive mode - device display is off
     */
    DISPLAY_INACTIVE_AIDL = 9,

    /**
     * Low latency audio streaming mode
     */
    AUDIO_STREAMING_LOW_LATENCY_AIDL = 10,

    /**
     * Secure camera streaming mode
     */
    CAMERA_STREAMING_SECURE_AIDL = 11,

    /**
     * Low quality camera streaming mode
     */
    CAMERA_STREAMING_LOW_AIDL = 12,

    /**
     * Medium quality camera streaming mode
     */
    CAMERA_STREAMING_MID_AIDL = 13,

    /**
     * High quality camera streaming mode
     */
    CAMERA_STREAMING_HIGH_AIDL = 14,

    /**
     * Game mode - optimizes for gaming workloads
     */
    GAME_AIDL = 15,

    /**
     * Game loading mode - optimizes for game loading screens
     */
    GAME_LOADING_AIDL = 16,

    /**
     * Display change mode - triggered during display state transitions
     */
    DISPLAY_CHANGE_AIDL = 17,

    /**
     * Automotive projection mode - optimized for automotive displays
     */
    AUTOMOTIVE_PROJECTION_AIDL = 18
} PowerMode;

/**
 * Power AIDL boost types
 */
typedef enum {
    /**
     * User interaction boost for short interactions inside an app,
     * such as button presses or starting an animation on button press.
     */
    INTERACTION_AIDL = 0,

    /**
     * Boost to display update when it's imminent - this signal is
     * used to signal a boost needed when the display will be updated soon.
     */
    DISPLAY_UPDATE_IMMINENT_AIDL = 1,

    /**
     * Boost for ML (Machine Learning) accelerator usage.
     */
    ML_ACC_AIDL = 2,

    /**
     * Boost for audio launch - indicates audio playback is about to start.
     */
    AUDIO_LAUNCH_AIDL = 3,

    /**
     * Boost for camera launch operations.
     */
    CAMERA_LAUNCH_AIDL = 4,

    /**
     * Boost for camera shot operations.
     */
    CAMERA_SHOT_AIDL = 5
} PowerBoost;

/**
 * MTK Power hint types
 * Values range from 20-46 representing different power profiles
 */
typedef enum {
    MTK_POWER_HINT_PROCESS_CREATE = 20,
    MTK_POWER_HINT_PACK_SWITCH = 21,
    MTK_POWER_HINT_ACT_SWITCH = 22,
    MTK_POWER_HINT_GAME_LAUNCH = 23,
    MTK_POWER_HINT_APP_ROTATE = 24,
    MTK_POWER_HINT_GAME_LOADING = 25,
    MTK_POWER_HINT_GALLERY_BOOST = 26,
    MTK_POWER_HINT_GALLERY_STEREO_BOOST = 27,
    MTK_POWER_HINT_AUDIO_POWER = 28,
    MTK_POWER_HINT_GALLERY_TOUCH = 29,
    MTK_POWER_HINT_SPORTS = 30,
    MTK_POWER_HINT_TEST_MODE = 31,
    MTK_POWER_HINT_GAMING = 32,
    MTK_POWER_HINT_CAMERA_LAUNCH = 33,
    MTK_POWER_HINT_CAMERA_SHOT = 34,
    MTK_POWER_HINT_VIDEO = 35,
    MTK_POWER_HINT_VIDEO_4K = 36,
    MTK_POWER_HINT_GAMING_ULTRA = 37,
    MTK_POWER_HINT_BROWSER_SCROLL = 38,
    MTK_POWER_HINT_BOOST_RESUME = 39,
    MTK_POWER_HINT_APP_TOUCH = 40,
    MTK_POWER_HINT_APP_LAUNCH = 41,
    MTK_POWER_HINT_FPS_CHANGE = 42,
    MTK_POWER_HINT_UX_SCROLLING = 43,
    MTK_POWER_HINT_INTERACTION = 44,
    MTK_POWER_HINT_UX_FOCUS = 45,
    MTK_POWER_HINT_THERMAL_LIMIT = 46
} MtkPowerHint;

/**
 * Binder context structure
 */
typedef struct {
    GBinderServiceManager *sm;
    GBinderRemoteObject *remote;
    GBinderClient *client;
    IdlType idl_type;
} Binder;

/**
 * Initialize Binder context for Power AIDL.
 *
 * @return Binder pointer on success, NULL on failure
 */
Binder *
binder_init_power_aidl(void);

/**
 * Initialize Binder context for Power HIDL.
 *
 * @return Binder pointer on success, NULL on failure
 */
Binder *
binder_init_power_hidl(void);

/**
 * Initialize Binder context for VR HIDL.
 *
 * @return Binder pointer on success, NULL on failure
 */
Binder *
binder_init_vr_hidl(void);

/**
 * Initialize Binder context for MTK Power HIDL.
 *
 * @return Binder pointer on success, NULL on failure
 */
Binder *
binder_init_mtkpower_hidl(void);

/**
 * Cleanup and free Binder context.
 *
 * @param ctx Binder to clean up
 */
void
binder_cleanup(Binder *ctx);

/**
 * Initialize Binder context for IPower.
 *
 * @return Binder instance on success, NULL on failure
 */
Binder *
binder_init(void);

/**
 * Apply specific power mode using AIDL.
 *
 * @param ctx Binder instance
 * @param mode Power mode to set (LOW_POWER, SUSTAINED_PERFORMANCE, etc.)
 * @return 0 on success, non-zero on failure
 */
int
binder_set_mode_aidl(Binder *ctx, PowerMode mode);

/**
 * Apply power boost using AIDL.
 *
 * @param ctx Binder instance
 * @param boost Boost type to apply (INTERACTION_AIDL, etc.)
 * @param durationMs Duration of the boost in milliseconds
 * @return 0 on success, non-zero on failure
 */
int
binder_set_boost_aidl(Binder *ctx, PowerBoost boost, int durationMs);

/**
 * Apply interactive state using HIDL.
 *
 * @param ctx Binder instance
 * @param interactive Interactive state (0 or 1)
 * @return 0 on success, non-zero on failure
 */
int
binder_set_interactive_hidl(Binder *ctx, gboolean interactive);

/**
 * Apply power hint using HIDL.
 *
 * @param ctx Binder instance
 * @param hint Power hint to set (LOW_POWER, SUSTAINED_PERFORMANCE, etc.)
 * @param data Additional data for the hint (0 or 1)
 * @return 0 on success, non-zero on failure
 */
int
binder_power_hint_hidl(Binder *ctx, PowerHint hint, gboolean data);

/**
 * Apply AIDL power settings with both mode and boost.
 *
 * @param ctx Binder instance
 * @param boost Boost type to apply
 * @param mode Power mode to set
 * @return 0 on success, non-zero on failure
 */
int
binder_set_power_aidl(Binder *ctx, PowerBoost boost, PowerMode mode);

/**
 * Apply HIDL power settings with both interactive state and hint.
 *
 * @param ctx Binder instance
 * @param interactive Interactive state (0 or 1)
 * @param hint Power hint to set
 * @return 0 on success, non-zero on failure
 */
int
binder_set_power_hidl(Binder *ctx, gboolean interactive, PowerHint hint);

/**
 * Apply performance boost for sustained performance mode in AIDL.
 *
 * @param ctx Binder instance
 * @param boost Boost type to apply
 * @return 0 on success, non-zero on failure
 */
int
binder_apply_performance_boost_aidl(Binder *ctx, PowerBoost boost);

/**
 * Apply VR HIDL settings.
 *
 * @param ctx Binder instance
 * @param enabled VR mode state (0 or 1)
 * @return 0 on success, non-zero on failure
 */
int
binder_set_vr_hidl(Binder *ctx, gboolean enabled);

/**
 * Apply MTK Power HIDL hint.
 *
 * @param ctx Binder instance
 * @param hint MTK Power hint to apply
 * @return 0 on success, non-zero on failure
 */
int
binder_set_mtkpower_hint_hidl(Binder *ctx, MtkPowerHint hint);

/**
 * Set power saver mode.
 *
 * @param ctx Binder instance
 * @return 0 on success, non-zero on failure
 */
int
binder_set_power_saver(Binder *ctx);

/**
 * Set sustained performance mode.
 *
 * @param ctx Binder instance
 * @return 0 on success, non-zero on failure
 */
int
binder_set_performance(Binder *ctx);

#endif /* BINDER_H */
