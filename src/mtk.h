#ifndef MTK_H
#define MTK_H

enum {
    // MTK pre-defined hint
    MTKPOWER_HINT_BASE                              = 20,

    MTKPOWER_HINT_PROCESS_CREATE                    = 21,
    MTKPOWER_HINT_PACK_SWITCH                       = 22,
    MTKPOWER_HINT_ACT_SWITCH                        = 23,
    MTKPOWER_HINT_APP_ROTATE                        = 24,
    MTKPOWER_HINT_APP_TOUCH                         = 25,
    MTKPOWER_HINT_GALLERY_BOOST                     = 26,
    MTKPOWER_HINT_GALLERY_STEREO_BOOST              = 27,
    MTKPOWER_HINT_WFD                               = 28,
    MTKPOWER_HINT_PMS_INSTALL                       = 29,
    MTKPOWER_HINT_EXT_LAUNCH                        = 30,
    MTKPOWER_HINT_WHITELIST_LAUNCH                  = 31,
    MTKPOWER_HINT_WIPHY_SPEED_DL                    = 32,
    MTKPOWER_HINT_SDN                               = 33,
    MTKPOWER_HINT_WHITELIST_ACT_SWITCH              = 34,
    MTKPOWER_HINT_BOOT                              = 35,
    MTKPOWER_HINT_AUDIO_LATENCY_DL                  = 36,
    MTKPOWER_HINT_AUDIO_LATENCY_UL                  = 37,
    MTKPOWER_HINT_AUDIO_POWER_DL                    = 38,
    MTKPOWER_HINT_AUDIO_DISABLE_WIFI_POWER_SAVE     = 39,
    MTKPOWER_HINT_MULTI_DISPLAY_WITH_GPU_FPS_60     = 40,
    MTKPOWER_HINT_MULTI_DISPLAY_WITH_GPU_FPS_90     = 41,
    MTKPOWER_HINT_MULTI_DISPLAY_WITH_GPU_FPS_120    = 42,
    MTKPOWER_HINT_UX_SCROLLING                      = 43,
    MTKPOWER_HINT_AUDIO_POWER_UL                    = 44,
    MTKPOWER_HINT_UX_MOVE_SCROLLING                 = 45,
    MTKPOWER_HINT_AUDIO_POWER_HI_RES                = 46,

    MTKPOWER_HINT_NUM,
};

#endif // MTK_H
