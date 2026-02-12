/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2026 Bardia Moshiri <bardia@furilabs.com>
 */

#include "device_node.h"
#include "utils.h"

static void
dn_add_governor(DeviceNodeContext *dn, const char *path)
{
    DeviceGovernorNode *it;

    if (dn == NULL)
        return;
    if (path == NULL)
        return;
    if (dn->governor_count >= (int)(sizeof(dn->governors) / sizeof(dn->governors[0])))
        return;
    if (!exists(path))
        return;

    it = &dn->governors[dn->governor_count];
    memset(it, 0, sizeof(*it));

    it->present = FALSE;
    g_strlcpy(it->path, path, sizeof(it->path));

    if (!read_str(path, it->default_value, sizeof(it->default_value)))
        it->default_value[0] = '\0';

    dn->governor_count++;
}

static void
dn_write_int_if_exists(const char *path, long long value)
{
    if (exists(path))
        write_int(path, value);
}

static void
dn_write_str_if_exists(const char *path, const char *value)
{
    if (exists(path))
        write_str(path, value);
}

void
device_node_init(DeviceNodeContext *dn)
{
    if (dn == NULL)
        return;

    memset(dn, 0, sizeof(*dn));

    /* QCOM */
    dn_add_governor(dn, "/sys/class/devfreq/aa00000.qcom,vidc1:arm9_bus_ddr/governor");
    dn_add_governor(dn, "/sys/class/devfreq/aa00000.qcom,vidc:arm9_bus_ddr/governor");
    dn_add_governor(dn, "/sys/class/devfreq/aa00000.qcom,vidc1:bus_cnoc/governor");
    dn_add_governor(dn, "/sys/class/devfreq/aa00000.qcom,vidc:bus_cnoc/governor");
    dn_add_governor(dn, "/sys/class/devfreq/soc:qcom,cci/governor");
    dn_add_governor(dn, "/sys/class/devfreq/soc:qcom,cpubw/governor");
    dn_add_governor(dn, "/sys/class/devfreq/soc:qcom,gpubw/governor");
    dn_add_governor(dn, "/sys/class/devfreq/soc:qcom,kgsl-busmon/governor");
    dn_add_governor(dn, "/sys/class/devfreq/soc:qcom,mincpubw/governor");
    dn_add_governor(dn, "/sys/class/devfreq/soc:qcom,l3-cdsp/governor");
    dn_add_governor(dn, "/sys/class/devfreq/soc:qcom,memlat-cpu0/governor");
    dn_add_governor(dn, "/sys/class/devfreq/soc:qcom,memlat-cpu4/governor");
    dn_add_governor(dn, "/sys/class/devfreq/soc:qcom,memlat-cpu6/governor");
    dn_add_governor(dn, "/sys/class/devfreq/soc:qcom,mincpu0bw/governor");
    dn_add_governor(dn, "/sys/class/devfreq/soc:qcom,mincpu6bw/governor");
    dn_add_governor(dn, "/sys/class/devfreq/soc:devfreq_spdm_cpu/governor");
    dn_add_governor(dn, "/sys/class/devfreq/soc:qcom,cdsp-cdsp-l3-lat/governor");
    dn_add_governor(dn, "/sys/class/devfreq/soc:qcom,cpu0-cpu-ddr-latfloor/governor");
    dn_add_governor(dn, "/sys/class/devfreq/soc:qcom,cpu0-cpu-l3-lat/governor");
    dn_add_governor(dn, "/sys/class/devfreq/soc:qcom,cpu0-cpu-llcc-lat/governor");
    dn_add_governor(dn, "/sys/class/devfreq/soc:qcom,cpu0-llcc-ddr-lat/governor");
    dn_add_governor(dn, "/sys/class/devfreq/soc:qcom,cpu6-cpu-ddr-latfloor/governor");
    dn_add_governor(dn, "/sys/class/devfreq/soc:qcom,cpu6-cpu-l3-lat/governor");
    dn_add_governor(dn, "/sys/class/devfreq/soc:qcom,cpu6-cpu-llcc-lat/governor");
    dn_add_governor(dn, "/sys/class/devfreq/soc:qcom,cpu6-llcc-ddr-lat/governor");
    dn_add_governor(dn, "/sys/class/devfreq/soc:qcom,cpu-cpu-llcc-bw/governor");
    dn_add_governor(dn, "/sys/class/devfreq/soc:qcom,cpu-llcc-ddr-bw/governor");
    dn_add_governor(dn, "/sys/class/devfreq/soc:qcom,npudsp-npu-ddr-bw/governor");
    dn_add_governor(dn, "/sys/class/devfreq/soc:qcom,npu-npu-ddr-bw/governor");

    /* Exynos/Tensor */
    dn_add_governor(dn, "/sys/class/devfreq/17000010.devfreq_mif/governor");
    dn_add_governor(dn, "/sys/class/devfreq/17000020.devfreq_int/governor");
    dn_add_governor(dn, "/sys/class/devfreq/17000030.devfreq_intcam/governor");
    dn_add_governor(dn, "/sys/class/devfreq/17000040.devfreq_disp/governor");
    dn_add_governor(dn, "/sys/class/devfreq/17000050.devfreq_cam/governor");
    dn_add_governor(dn, "/sys/class/devfreq/17000060.devfreq_tnr/governor");
    dn_add_governor(dn, "/sys/class/devfreq/17000070.devfreq_mfc/governor");
    dn_add_governor(dn, "/sys/class/devfreq/17000080.devfreq_bo/governor");

    /* Google/GS */
    dn_add_governor(dn, "/sys/class/devfreq/gs_memlat_devfreq:devfreq_mif_cpu0_memlat@17000010/governor");
    dn_add_governor(dn, "/sys/class/devfreq/gs_memlat_devfreq:devfreq_mif_cpu1_memlat@17000010/governor");
    dn_add_governor(dn, "/sys/class/devfreq/gs_memlat_devfreq:devfreq_mif_cpu2_memlat@17000010/governor");
    dn_add_governor(dn, "/sys/class/devfreq/gs_memlat_devfreq:devfreq_mif_cpu3_memlat@17000010/governor");
    dn_add_governor(dn, "/sys/class/devfreq/gs_memlat_devfreq:devfreq_mif_cpu4_memlat@17000010/governor");
    dn_add_governor(dn, "/sys/class/devfreq/gs_memlat_devfreq:devfreq_mif_cpu5_memlat@17000010/governor");
    dn_add_governor(dn, "/sys/class/devfreq/gs_memlat_devfreq:devfreq_mif_cpu6_memlat@17000010/governor");
    dn_add_governor(dn, "/sys/class/devfreq/gs_memlat_devfreq:devfreq_mif_cpu7_memlat@17000010/governor");
}

static void
device_node_apply_save_tweaks(void)
{
    /* MediaTek CPUFreq */
    dn_write_int_if_exists("/proc/cpufreq/cpufreq_power_mode", 1);
    dn_write_int_if_exists("/proc/cpufreq/cpufreq_cci_mode", 0);
    dn_write_int_if_exists("/proc/cpufreq/cpufreq_sched_disable", 1);
    dn_write_int_if_exists("/proc/cpufreq/cpufreq_stress_test", 1);
    dn_write_int_if_exists("/proc/cpufreq/MT_CPU_DVFS_CCI/cpufreq_turbo_mode", 0);
    dn_write_int_if_exists("/proc/cpufreq/MT_CPU_DVFS_L/cpufreq_turbo_mode", 0);
    dn_write_int_if_exists("/proc/cpufreq/MT_CPU_DVFS_LL/cpufreq_turbo_mode", 0);

    /* MediaTek */
    dn_write_int_if_exists("/sys/devices/system/cpu/perf/enable", 0);
    dn_write_int_if_exists("/sys/devices/system/cpu/sched/sched_boost", 0);

    /* MediaTek GED */
    dn_write_int_if_exists("/sys/module/ged/parameters/boost_gpu_enable", 0);
    dn_write_int_if_exists("/sys/module/ged/parameters/boost_extra", 0);
    dn_write_int_if_exists("/sys/module/ged/parameters/gx_game_mode", 0);
    dn_write_int_if_exists("/sys/module/ged/parameters/gx_boost_on", 0);
    dn_write_int_if_exists("/sys/module/ged/parameters/ged_force_mdp_enable", 1);
    dn_write_int_if_exists("/sys/module/ged/parameters/boost_amp", 0);

    /* MediaTek GBE */
    dn_write_int_if_exists("/sys/kernel/gbe/gbe_enable1", 0);
    dn_write_int_if_exists("/sys/kernel/gbe/gbe_enable2", 0);
    dn_write_int_if_exists("/sys/kernel/gbe/gbe2_max_boost_cnt", 2);
    dn_write_int_if_exists("/sys/kernel/gbe/gbe2_loading_th", 5);

    /* QCOM */
    dn_write_str_if_exists("/sys/module/lpm_levels/parameters/sleep_disabled", "N");
    dn_write_str_if_exists("/sys/module/lpm_levels/parameters/lpm_prediction", "N");
    dn_write_str_if_exists("/sys/module/lpm_levels/parameters/lpm_ipi_prediction", "N");
    dn_write_int_if_exists("/sys/kernel/debug/msm_vidc/fw_low_power_mode", 1);

    /* UFS */
    dn_write_int_if_exists("/sys/devices/platform/soc/1d84000.ufshc/hibern8_on_idle_enable", 1);
    dn_write_int_if_exists("/sys/devices/platform/soc/1d84000.ufshc/clkgate_enable", 1);
    dn_write_int_if_exists("/sys/devices/platform/soc/1d84000.ufshc/clkscale_enable", 1);

    /* MediaTek FPSGO */
    dn_write_int_if_exists("/sys/kernel/fpsgo/fstb/adopt_low_fps", 1);
    dn_write_int_if_exists("/sys/kernel/fpsgo/fbt/boost_ta", 0);
    dn_write_int_if_exists("/sys/kernel/fpsgo/common/gpu_block_boost", 0);

    /* MediaTek apusys */
    dn_write_int_if_exists("/sys/kernel/apusys/mnoc_apu_qos_boost", 0);

    /* MediaTek perfmgr */
    dn_write_int_if_exists("/proc/perfmgr/boost_ctrl/dram_ctrl/ddr", 0);
    dn_write_int_if_exists("/proc/perfmgr/boost_ctrl/eas_ctrl/sched_boost", 0);
}

static void
device_node_apply_boost_tweaks(void)
{
    /* MediaTek CPUFreq */
    dn_write_int_if_exists("/proc/cpufreq/cpufreq_power_mode", 3);
    dn_write_int_if_exists("/proc/cpufreq/cpufreq_cci_mode", 1);
    dn_write_int_if_exists("/proc/cpufreq/cpufreq_sched_disable", 0);
    dn_write_int_if_exists("/proc/cpufreq/cpufreq_stress_test", 1);
    dn_write_int_if_exists("/proc/cpufreq/MT_CPU_DVFS_CCI/cpufreq_turbo_mode", 1);
    dn_write_int_if_exists("/proc/cpufreq/MT_CPU_DVFS_L/cpufreq_turbo_mode", 1);
    dn_write_int_if_exists("/proc/cpufreq/MT_CPU_DVFS_LL/cpufreq_turbo_mode", 1);

    /* MediaTek */
    dn_write_int_if_exists("/sys/devices/system/cpu/perf/enable", 1);
    dn_write_int_if_exists("/sys/devices/system/cpu/sched/sched_boost", 1);

    /* MediaTek GED */
    dn_write_int_if_exists("/sys/module/ged/parameters/boost_gpu_enable", 1);
    dn_write_int_if_exists("/sys/module/ged/parameters/boost_extra", 1);
    dn_write_int_if_exists("/sys/module/ged/parameters/gx_game_mode", 1);
    dn_write_int_if_exists("/sys/module/ged/parameters/gx_boost_on", 1);
    dn_write_int_if_exists("/sys/module/ged/parameters/ged_force_mdp_enable", 1);
    dn_write_int_if_exists("/sys/module/ged/parameters/boost_amp", 1);

    /* MediaTek GBE */
    dn_write_int_if_exists("/sys/kernel/gbe/gbe_enable1", 1);
    dn_write_int_if_exists("/sys/kernel/gbe/gbe_enable2", 1);
    dn_write_int_if_exists("/sys/kernel/gbe/gbe2_max_boost_cnt", 2);
    dn_write_int_if_exists("/sys/kernel/gbe/gbe2_loading_th", 20);

    /* QCOM */
    dn_write_str_if_exists("/sys/module/lpm_levels/parameters/sleep_disabled", "Y");
    dn_write_str_if_exists("/sys/module/lpm_levels/parameters/lpm_prediction", "N");
    dn_write_str_if_exists("/sys/module/lpm_levels/parameters/lpm_ipi_prediction", "N");
    dn_write_int_if_exists("/sys/kernel/debug/msm_vidc/fw_low_power_mode", 0);

    /* UFS */
    dn_write_int_if_exists("/sys/devices/platform/soc/1d84000.ufshc/hibern8_on_idle_enable", 0);
    dn_write_int_if_exists("/sys/devices/platform/soc/1d84000.ufshc/clkgate_enable", 0);
    dn_write_int_if_exists("/sys/devices/platform/soc/1d84000.ufshc/clkscale_enable", 0);

    /* MediaTek fpsgo */
    dn_write_int_if_exists("/sys/kernel/fpsgo/fstb/adopt_low_fps", 0);
    dn_write_int_if_exists("/sys/kernel/fpsgo/fbt/boost_ta", 1);
    dn_write_int_if_exists("/sys/kernel/fpsgo/common/gpu_block_boost", 101);

    /* MediaTek apusys */
    dn_write_int_if_exists("/sys/kernel/apusys/mnoc_apu_qos_boost", 1);

    /* MediaTek perfmgr */
    dn_write_int_if_exists("/proc/perfmgr/boost_ctrl/dram_ctrl/ddr", 2);
    dn_write_int_if_exists("/proc/perfmgr/boost_ctrl/eas_ctrl/sched_boost", 1);
}

void
device_node_apply_powersave(const DeviceNodeContext *dn, const BatmanConfig *cfg)
{
    int i;

    if (dn == NULL)
        return;
    if (cfg == NULL)
        return;

    if (cfg->bus_powersave_enabled) {
        for (i = 0; i < dn->governor_count; i++) {
            const DeviceGovernorNode *it = &dn->governors[i];

            if (!it->present)
                continue;

            write_str(it->path, "powersave");
        }
    }

    device_node_apply_save_tweaks();
}

void
device_node_apply_default(const DeviceNodeContext *dn, const BatmanConfig *cfg)
{
    int i;

    if (dn == NULL)
        return;
    if (cfg == NULL)
        return;

    if (cfg->bus_powersave_enabled) {
        for (i = 0; i < dn->governor_count; i++) {
            const DeviceGovernorNode *it = &dn->governors[i];

            if (!it->present)
                continue;
            if (it->default_value[0] == '\0')
                continue;

            write_str(it->path, it->default_value);
        }
    }

    device_node_apply_boost_tweaks();
}
