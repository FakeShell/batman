/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2026 Bardia Moshiri <bardia@furilabs.com>
 */

#ifndef CPU_H
#define CPU_H

#include "config.h"

#define PROC_CPUINFO "/proc/cpuinfo"
#define PROC_DT_MODEL "/proc/device-tree/model"

#define SYS_CPU_BASE "/sys/devices/system/cpu"
#define SYS_CPUFREQ_BASE "/sys/devices/system/cpu/cpufreq"

#define SYS_CPUFREQ_ALL_TIME_IN_STATE SYS_CPU_BASE "/cpufreq/all_time_in_state"
#define SYS_POWER_CPUFREQ_MIN "/sys/power/cpufreq_min_limit"
#define SYS_POWER_CPUFREQ_MAX "/sys/power/cpufreq_max_limit"
#define SYS_POWER_CPUFREQ_MAX_WRITE "/sys/power/cpufreq_max_limit"
#define SYS_POWER_CPUFREQ_MIN_WRITE "/sys/power/cpufreq_min_limit"

#define EXYNOS_CPU_LIMIT "/var/lib/batman/exynos_cpu_limit"
#define DEFAULT_CPU_GOVERNOR_FILE "/var/lib/batman/default_cpu_governor"
#define DEFAULT_CPU_GOVERNOR_SAVED "/var/lib/batman/saved"

#define CUSTOM_DEFAULT_GOVERNOR_FILE "/var/lib/batman/CUSTOM_DEFAULT_GOVERNOR"
#define CUSTOM_LASTPOLCORE_FILE "/var/lib/batman/CUSTOM_LASTPOLCORE"
#define CUSTOM_FIRSTPOLCORE_FILE "/var/lib/batman/CUSTOM_FIRSTPOLCORE"

/**
 * CPU save governor backend selection.
 */
typedef enum {
    CPU_SAVE_GOVERNOR_NONE = 0,        /** No supported save governor */
    CPU_SAVE_GOVERNOR_POWERSAVE = 1,   /** Use "powersave" governor */
    CPU_SAVE_GOVERNOR_USERSPACE = 2,   /** Use "userspace" governor + setspeed */
    CPU_SAVE_GOVERNOR_EXYNOS_LIMIT = 3 /** Use exynos cpu limit nodes */
} CpuSaveGovernor;

/**
 * CPU context
 */
typedef struct {
    gboolean is_x86;                   /** True for x86_64/i686 */
    gboolean is_exynos;                /** True if device-tree model contains EXYNOS token */

    int core_first;                    /** First CPU index (usually 0) */
    int core_last;                     /** Last CPU index */

    gboolean legacy;                   /** Legacy cpufreq layout (no policy dirs) */
    int available_policies;            /** Number of cpufreq policy dirs */

    int first_pol_core;                /** First core of the LITTLE/policy group */
    int last_pol_core;                 /** Last core of the LITTLE/policy group */
    int offline_count;                 /** Saved original last_pol_core for restore */

    CpuSaveGovernor save_backend;      /** Which backend to use for powersave/default */

    char default_governor[64];         /** Current default governor */
    char runtime_cur_governor[64];     /** Runtime override */

    long long exynos_min_limit;        /** /sys/power/cpufreq_min_limit */
    long long exynos_max_limit;        /** /sys/power/cpufreq_max_limit */

    int base_first_pol_core;           /** Base first_pol_core detected at init (before CUSTOM_* overrides) */
    int base_last_pol_core;            /** Base last_pol_core detected at init (before CUSTOM_* overrides) */
    int base_offline_count;            /** Base offline_count detected at init (before CUSTOM_* overrides) */
    char base_default_governor[64];    /** Base default governor detected at init (before CUSTOM_* overrides) */
} CpuContext;

/**
 * Initialize CPU context.
 *
 * @param cpu CpuContext to initialize
 * @return true on success, false on failure
 */
gboolean
cpu_init(CpuContext *cpu);

/**
 * Apply powersave mode for CPU.
 *
 * @param cpu CpuContext
 */
void
cpu_apply_powersave(CpuContext *cpu);

/**
 * Apply default mode for CPU.
 *
 * @param cpu CpuContext
 */
void
cpu_apply_default(CpuContext *cpu);

/**
 * Offline policy cores as configured by cpu->first_pol_core..cpu->last_pol_core.
 *
 * @param cpu CpuContext
 */
void
cpu_apply_offline(CpuContext *cpu);

/**
 * Online policy cores as configured by cpu->first_pol_core..cpu->last_pol_core.
 *
 * @param cpu CpuContext
 */
void
cpu_apply_online(CpuContext *cpu);

/**
 * Restore last_pol_core/offline_count back to the original value.
 *
 * @param cpu CpuContext
 * @param cfg BatmanConfig
 */
void
cpu_restore_offline_limit(CpuContext *cpu, const BatmanConfig *cfg);

/**
 * Refresh CPU runtime overrides.
 *
 * @param cpu CpuContext
 */
void
cpu_refresh_overrides(CpuContext *cpu);

/**
 * Return total CPU count.
 *
 * @param cpu CpuContext
 * @return CPU count
 */
int
cpu_get_core_count(const CpuContext *cpu);

/**
 * Get default CPU governor.
 *
 * @return governor name on success, empty string on failure.
 */
gchar *
cpu_get_default_governor(void);

#endif /* CPU_H */
