/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2026 Bardia Moshiri <bardia@furilabs.com>
 */

#include "cpu.h"
#include "utils.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>

static gboolean
is_x86_arch(void)
{
    struct utsname u;
    if (uname(&u) != 0)
        return FALSE;
    return (strcmp(u.machine, "x86_64") == 0 || strcmp(u.machine, "i686") == 0) ? TRUE : FALSE;
}

static gboolean
detect_exynos(void)
{
    if (!exists(PROC_DT_MODEL))
        return FALSE;
    return (file_contains_token(PROC_DT_MODEL, "EXYNOS") ||
            file_contains_token(PROC_DT_MODEL, "Exynos") ||
            file_contains_token(PROC_DT_MODEL, "exynos")) ? TRUE : FALSE;
}

static gboolean
parse_cpu_range(const char *buf, int *out_first, int *out_last)
{
    if (!out_first || !out_last)
        return FALSE;

    *out_first = 0;
    *out_last = 0;

    if (!buf || buf[0] == '\0')
        return FALSE;

    /* supports:
     *   "0-7"
     *   "0-3,8-11"
     *   "0"
     */
    int first = -1;
    int last = -1;

    const char *p = buf;

    while (*p) {
        while (*p && (isspace((unsigned char)*p) || *p == ',')) p++;
        if (!*p)
            break;

        char *endp = NULL;
        long a = strtol(p, &endp, 10);
        if (endp == p)
            break;

        long b = a;

        p = endp;
        if (*p == '-') {
            p++;
            endp = NULL;
            b = strtol(p, &endp, 10);
            if (endp == p)
                b = a;
            p = endp;
        }

        if (first < 0)
            first = (int)a;
        if ((int)a < first)
            first = (int)a;
        if ((int)b > last)
            last = (int)b;

        while (*p && *p != ',')
            p++;
    }

    if (first < 0 || last < 0)
        return FALSE;

    *out_first = first;
    *out_last = last;
    return TRUE;
}

static gboolean
read_cpu_range_sysfs(int *out_first, int *out_last)
{
    if (!out_first || !out_last)
        return FALSE;

    char buf[256];

    /* prefer present */
    buf[0] = '\0';
    if (read_str(SYS_CPU_BASE "/present", buf, sizeof(buf))) {
        if (buf[0] != '\0' && parse_cpu_range(buf, out_first, out_last))
            return TRUE;
    }

    /* fallback to possible */
    buf[0] = '\0';
    if (read_str(SYS_CPU_BASE "/possible", buf, sizeof(buf))) {
        if (buf[0] != '\0' && parse_cpu_range(buf, out_first, out_last))
            return TRUE;
    }

    return FALSE;
}

static int
read_first_core_cpuinfo(void)
{
    FILE *fp = fopen(PROC_CPUINFO, "r");
    if (!fp) {
        g_warning("Failed to open %s: %s", PROC_CPUINFO, g_strerror(errno));
        return 0;
    }

    char line[256];
    int core = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "processor", 9) == 0) {
            char *colon = strchr(line, ':');
            if (colon) {
                core = atoi(colon + 1);
                break;
            }
        }
    }

    fclose(fp);
    return core;
}

static int
read_last_core_cpuinfo(void)
{
    FILE *fp = fopen(PROC_CPUINFO, "r");
    if (!fp) {
        g_warning("Failed to open %s: %s", PROC_CPUINFO, g_strerror(errno));
        return 0;
    }

    char line[256];
    int last = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "processor", 9) == 0) {
            char *colon = strchr(line, ':');
            if (colon)
                last = atoi(colon + 1);
        }
    }

    fclose(fp);
    return last;
}

static int
read_first_core(void)
{
    int first = 0, last = 0;
    if (read_cpu_range_sysfs(&first, &last))
        return first;
    return read_first_core_cpuinfo();
}

static int
read_last_core(void)
{
    int first = 0, last = 0;
    if (read_cpu_range_sysfs(&first, &last))
        return last;
    return read_last_core_cpuinfo();
}

static gboolean
policy_parse_index(const char *name, int *out_idx)
{
    if (!name || !out_idx)
        return FALSE;
    if (strncmp(name, "policy", 6) != 0)
        return FALSE;

    const char *p = name + 6;
    if (*p == '\0')
        return FALSE;
    for (const char *q = p; *q; q++) {
        if (!isdigit((unsigned char)*q))
            return FALSE;
    }

    *out_idx = atoi(p);
    return TRUE;
}

static int
count_policies(void)
{
    DIR *d = opendir(SYS_CPUFREQ_BASE);
    if (!d)
        return 0;

    int count = 0;
    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (de->d_type != DT_DIR && de->d_type != DT_LNK && de->d_type != DT_UNKNOWN)
            continue;
        if (strncmp(de->d_name, "policy", 6) == 0) {
            int idx = -1;
            if (policy_parse_index(de->d_name, &idx))
                count++;
        }
    }

    closedir(d);
    return count;
}

static gboolean
get_lowest_policy_name(char *out, size_t out_len)
{
    if (!out || out_len == 0)
        return FALSE;

    out[0] = '\0';

    DIR *d = opendir(SYS_CPUFREQ_BASE);
    if (!d)
        return FALSE;

    int best_idx = -1;
    char best_name[64] = {0};

    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (de->d_type != DT_DIR && de->d_type != DT_LNK && de->d_type != DT_UNKNOWN)
            continue;
        int idx = -1;
        if (!policy_parse_index(de->d_name, &idx))
            continue;
        if (best_idx < 0 || idx < best_idx) {
            best_idx = idx;
            g_strlcpy(best_name, de->d_name, sizeof(best_name));
        }
    }

    closedir(d);

    if (best_idx < 0 || best_name[0] == '\0')
        return FALSE;

    g_strlcpy(out, best_name, out_len);
    return TRUE;
}

static void
parse_related_cpus_range(const char *buf, int *out_first, int *out_last)
{
    if (!out_first || !out_last)
        return;

    *out_first = -1;
    *out_last = -1;

    if (!buf || buf[0] == '\0')
        return;

    const char *p = buf;
    while (*p) {
        while (*p && isspace((unsigned char)*p)) {
            p++;
        }
        if (!*p)
            break;
        int v = atoi(p);
        if (*out_first < 0)
            *out_first = v;
        *out_last = v;
        while (*p && !isspace((unsigned char)*p)) {
            p++;
        }
    }
}

static void
save_default_cpu_governor(CpuContext *cpu)
{
    if (!cpu)
        return;

    if (!exists(DEFAULT_CPU_GOVERNOR_SAVED)) {
        if (!cpu->is_x86 && strcmp(cpu->default_governor, "powersave") == 0) {
            g_warning("bad starting governor 'powersave'");
            return;
        }

        write_str(DEFAULT_CPU_GOVERNOR_FILE, cpu->default_governor);
        write_str(DEFAULT_CPU_GOVERNOR_SAVED, "");

        g_debug("saved default CPU governor '%s' to %s",
                cpu->default_governor, DEFAULT_CPU_GOVERNOR_FILE);
        return;
    }

    if (!cpu->is_x86 && strcmp(cpu->default_governor, "powersave") == 0) {
        char saved[128];
        saved[0] = '\0';
        if (!read_str(DEFAULT_CPU_GOVERNOR_FILE, saved, sizeof(saved)))
            saved[0] = '\0';

        if (saved[0] == '\0' || strcmp(saved, "powersave") == 0) {
            g_warning("bad starting governor and saved governor 'powersave'");
            return;
        }

        g_debug("detected governor was 'powersave'. overriding with saved '%s'", saved);
        g_strlcpy(cpu->default_governor, saved, sizeof(cpu->default_governor));
    }
}

static void
detect_policy_cores(CpuContext *cpu)
{
    if (!cpu)
        return;

    cpu->available_policies = count_policies();

    if (cpu->available_policies != 0) {
        if (cpu->is_x86) {
            char first_policy[64];
            get_lowest_policy_name(first_policy, sizeof(first_policy));

            cpu->legacy = FALSE;
            cpu->first_pol_core = 1;
            cpu->last_pol_core = cpu->core_last;
            cpu->offline_count = cpu->last_pol_core;

            g_debug("cpufreq policies present (x86): offline range %d..%d",
                    cpu->first_pol_core, cpu->last_pol_core);
            return;
        }

        /* pick deterministic lowest policy */
        char first_policy[64];
        if (!get_lowest_policy_name(first_policy, sizeof(first_policy))) {
            cpu->legacy = FALSE;
            cpu->first_pol_core = cpu->core_first;
            cpu->last_pol_core = cpu->core_last;
            cpu->offline_count = cpu->last_pol_core;
            g_debug("cpufreq: no policy found, fallback offline range %d..%d",
                    cpu->first_pol_core, cpu->last_pol_core);
            return;
        }

        char rel_path[256];
        char rel_buf[256];

        g_snprintf(rel_path, sizeof(rel_path),
                   SYS_CPUFREQ_BASE "/%s/related_cpus", first_policy);

        rel_buf[0] = '\0';
        if (!read_str(rel_path, rel_buf, sizeof(rel_buf)))
            rel_buf[0] = '\0';

        int first = -1, last = -1;
        parse_related_cpus_range(rel_buf, &first, &last);

        if (first < 0 || last < 0) {
            cpu->first_pol_core = cpu->core_first;
            cpu->last_pol_core = cpu->core_last;
        } else {
            cpu->first_pol_core = first;
            cpu->last_pol_core = last;
        }

        cpu->legacy = FALSE;
        cpu->offline_count = cpu->last_pol_core;

        g_debug("cpufreq: picked %s related_cpus='%s'. offline range %d..%d",
                first_policy, rel_buf, cpu->first_pol_core, cpu->last_pol_core);
        return;
    }

    /* legacy ARM (no policy*) */
    if (exists(SYS_CPUFREQ_ALL_TIME_IN_STATE)) {
        cpu->legacy = TRUE;
        cpu->first_pol_core = cpu->core_first;
        cpu->last_pol_core = cpu->core_last / 2;
        cpu->offline_count = cpu->last_pol_core;

        g_debug("legacy cpufreq: offline range %d..%d",
                cpu->first_pol_core, cpu->last_pol_core);
        return;
    }

    /* no policy and no legacy node: fallback to all cores */
    cpu->legacy = FALSE;
    cpu->first_pol_core = cpu->core_first;
    cpu->last_pol_core = cpu->core_last;
    cpu->offline_count = cpu->last_pol_core;

    g_debug("no cpufreq policy/legacy: offline range %d..%d",
            cpu->first_pol_core, cpu->last_pol_core);
}

static void
set_default_governor(CpuContext *cpu)
{
    if (!cpu)
        return;

    if (cpu->is_x86) {
        char avail_path[256];
        char gov_path[256];

        g_snprintf(avail_path, sizeof(avail_path),
                   SYS_CPU_BASE "/cpu%d/cpufreq/scaling_available_governors",
                   cpu->core_first);
        g_snprintf(gov_path, sizeof(gov_path),
                   SYS_CPU_BASE "/cpu%d/cpufreq/scaling_governor",
                   cpu->core_first);

        if (!exists(avail_path) || !exists(gov_path)) {
            g_warning("cpufreq nodes missing for x86 default governor detection");
            g_strlcpy(cpu->default_governor, "powersave", sizeof(cpu->default_governor));
            return;
        }

        if (file_contains_token(avail_path, "performance")) {
            if (file_contains_token(gov_path, "powersave"))
                g_strlcpy(cpu->default_governor, "powersave", sizeof(cpu->default_governor));
            else
                g_strlcpy(cpu->default_governor, "performance", sizeof(cpu->default_governor));
        } else {
            g_warning("can't find x86 performance governor, defaulting to powersave");
            g_strlcpy(cpu->default_governor, "powersave", sizeof(cpu->default_governor));
        }
        return;
    }

    /* read current scaling_governor */
    char gov_path[256];
    g_snprintf(gov_path, sizeof(gov_path),
               SYS_CPU_BASE "/cpu%d/cpufreq/scaling_governor",
               cpu->core_first);

    cpu->default_governor[0] = '\0';
    if (!read_str(gov_path, cpu->default_governor, sizeof(cpu->default_governor)))
        cpu->default_governor[0] = '\0';

    if (cpu->default_governor[0] == '\0' ||
        strcmp(cpu->default_governor, "powersave") == 0) {

        char saved[128];
        saved[0] = '\0';
        if (!read_str(DEFAULT_CPU_GOVERNOR_FILE, saved, sizeof(saved)))
            saved[0] = '\0';

        if (saved[0] != '\0' && strcmp(saved, "powersave") != 0) {
            g_debug("default governor fallback: using saved '%s' (detected was '%s')",
                    saved, cpu->default_governor[0] ? cpu->default_governor : "(empty)");
            g_strlcpy(cpu->default_governor, saved, sizeof(cpu->default_governor));
        }
    }

    if (cpu->default_governor[0] == '\0')
        g_strlcpy(cpu->default_governor, "schedutil", sizeof(cpu->default_governor));
}

static void
select_save_backend(CpuContext *cpu)
{
    if (!cpu)
        return;

    cpu->save_backend = CPU_SAVE_GOVERNOR_NONE;

    /* Exynos special path */
    if (exists(EXYNOS_CPU_LIMIT) &&
        exists(SYS_POWER_CPUFREQ_MIN) &&
        exists(SYS_POWER_CPUFREQ_MAX)) {
        char tmp[128];

        tmp[0] = '\0';
        if (!read_str(SYS_POWER_CPUFREQ_MIN, tmp, sizeof(tmp)))
            tmp[0] = '\0';
        cpu->exynos_min_limit = g_ascii_strtoll(tmp, NULL, 10);

        tmp[0] = '\0';
        if (!read_str(SYS_POWER_CPUFREQ_MAX, tmp, sizeof(tmp)))
            tmp[0] = '\0';
        cpu->exynos_max_limit = g_ascii_strtoll(tmp, NULL, 10);

        cpu->save_backend = CPU_SAVE_GOVERNOR_EXYNOS_LIMIT;
        return;
    }

    /* legacy path checks cpuX/cpufreq available governors */
    if (cpu->legacy) {
        char avail_path[256];
        g_snprintf(avail_path, sizeof(avail_path),
                   SYS_CPU_BASE "/cpu%d/cpufreq/scaling_available_governors",
                   cpu->core_first);

        if (file_contains_token(avail_path, "powersave"))
            cpu->save_backend = CPU_SAVE_GOVERNOR_POWERSAVE;
        else if (file_contains_token(avail_path, "userspace"))
            cpu->save_backend = CPU_SAVE_GOVERNOR_USERSPACE;
        else
            cpu->save_backend = CPU_SAVE_GOVERNOR_NONE;
        return;
    }

    char first_policy[64];
    if (!get_lowest_policy_name(first_policy, sizeof(first_policy))) {
        cpu->save_backend = CPU_SAVE_GOVERNOR_NONE;
        return;
    }

    char avail_path[256];
    g_snprintf(avail_path, sizeof(avail_path),
               SYS_CPUFREQ_BASE "/%s/scaling_available_governors", first_policy);

    if (file_contains_token(avail_path, "powersave"))
        cpu->save_backend = CPU_SAVE_GOVERNOR_POWERSAVE;
    else if (file_contains_token(avail_path, "userspace"))
        cpu->save_backend = CPU_SAVE_GOVERNOR_USERSPACE;
    else
        cpu->save_backend = CPU_SAVE_GOVERNOR_NONE;
}

void
cpu_refresh_overrides(CpuContext *cpu)
{
    if (!cpu)
        return;

    int base_start = cpu->base_first_pol_core;
    int base_end = cpu->base_last_pol_core;

    if (base_start < cpu->core_first)
        base_start = cpu->core_first;
    if (base_end > cpu->core_last)
        base_end = cpu->core_last;
    if (base_start > base_end)
        base_start = base_end;

    cpu->first_pol_core = base_start;
    cpu->last_pol_core = base_end;

    cpu->offline_count = cpu->last_pol_core;

    g_strlcpy(cpu->default_governor,
              cpu->base_default_governor,
              sizeof(cpu->default_governor));

    /* CUSTOM_DEFAULT_GOVERNOR overrides default_governor */
    if (exists(CUSTOM_DEFAULT_GOVERNOR_FILE)) {
        char custom[128] = {0};
        if (read_str(CUSTOM_DEFAULT_GOVERNOR_FILE, custom, sizeof(custom)) &&
            custom[0] != '\0') {

            g_debug("cpu_refresh_overrides: default governor override '%s'", custom);
            g_strlcpy(cpu->default_governor, custom, sizeof(cpu->default_governor));
        }
    }

    /* CUSTOM_LASTPOLCORE forces end of offline range */
    if (exists(CUSTOM_LASTPOLCORE_FILE)) {
        char tmp[64] = {0};
        if (read_str(CUSTOM_LASTPOLCORE_FILE, tmp, sizeof(tmp)) && tmp[0] != '\0') {
            int v = atoi(tmp);

            if (v < cpu->core_first)
                v = cpu->core_first;
            if (v > cpu->core_last)
                v = cpu->core_last;

            cpu->last_pol_core = v;

            g_debug("cpu_refresh_overrides: last_pol_core override %d",
                    cpu->last_pol_core);
        }
    }

    /*
     * CUSTOM_FIRSTPOLCORE forces start of offline range
     *   N=1 => offline 1..end
     *   N=4 => offline 4..end
     */
    if (exists(CUSTOM_FIRSTPOLCORE_FILE)) {
        char tmp[64] = {0};
        if (read_str(CUSTOM_FIRSTPOLCORE_FILE, tmp, sizeof(tmp)) && tmp[0] != '\0') {
            int v = atoi(tmp);

            if (v < cpu->core_first)
                v = cpu->core_first;

            if (v > cpu->core_last)
                v = cpu->core_last + 1;

            cpu->first_pol_core = v;

            g_debug("cpu_refresh_overrides: first_pol_core override %d",
                    cpu->first_pol_core);
        }
    }

    if (cpu->first_pol_core < cpu->core_first)
        cpu->first_pol_core = cpu->core_first;

    if (cpu->last_pol_core > cpu->core_last)
        cpu->last_pol_core = cpu->core_last;

    if (cpu->first_pol_core > cpu->last_pol_core)
        g_debug("cpu_refresh_overrides: empty offline range %d..%d (no-op)",
                cpu->first_pol_core, cpu->last_pol_core);

    cpu->offline_count = cpu->last_pol_core;

    g_debug("cpu_refresh_overrides: range %d..%d offline_count=%d default_gov=%s",
            cpu->first_pol_core,
            cpu->last_pol_core,
            cpu->offline_count,
            cpu->default_governor);
}

gboolean
cpu_init(CpuContext *cpu)
{
    if (!cpu)
        return FALSE;

    memset(cpu, 0, sizeof(*cpu));

    cpu->is_x86 = is_x86_arch();
    cpu->is_exynos = detect_exynos();

    cpu->core_first = read_first_core();
    cpu->core_last = read_last_core();

    detect_policy_cores(cpu);
    set_default_governor(cpu);
    save_default_cpu_governor(cpu);

    cpu->base_first_pol_core = cpu->first_pol_core;
    cpu->base_last_pol_core = cpu->last_pol_core;
    cpu->base_offline_count = cpu->offline_count;
    g_strlcpy(cpu->base_default_governor, cpu->default_governor, sizeof(cpu->base_default_governor));

    select_save_backend(cpu);

    cpu->runtime_cur_governor[0] = '\0';

    cpu_refresh_overrides(cpu);

    g_debug("cpu_init: core_first=%d core_last=%d first_pol_core=%d last_pol_core=%d offline_count=%d backend=%d default_gov=%s",
            cpu->core_first, cpu->core_last,
            cpu->first_pol_core, cpu->last_pol_core, cpu->offline_count,
            (int)cpu->save_backend, cpu->default_governor);

    return TRUE;
}

void
cpu_apply_powersave(CpuContext *cpu)
{
    if (!cpu)
        return;

    if (cpu->save_backend == CPU_SAVE_GOVERNOR_POWERSAVE) {
        for (int i = cpu->core_first; i <= cpu->core_last; i++) {
            char path[256];
            g_snprintf(path, sizeof(path),
                       SYS_CPU_BASE "/cpu%d/cpufreq/scaling_governor", i);
            write_str(path, "powersave");
        }
        g_strlcpy(cpu->runtime_cur_governor, "powersave", sizeof(cpu->runtime_cur_governor));
        return;
    }

    if (cpu->save_backend == CPU_SAVE_GOVERNOR_USERSPACE) {
        for (int i = cpu->core_first; i <= cpu->core_last; i++) {
            char gov_path[256];
            char min_path[256];
            char set_path[256];
            char min_buf[64];

            g_snprintf(gov_path, sizeof(gov_path),
                       SYS_CPU_BASE "/cpu%d/cpufreq/scaling_governor", i);
            g_snprintf(min_path, sizeof(min_path),
                       SYS_CPU_BASE "/cpu%d/cpufreq/scaling_min_freq", i);
            g_snprintf(set_path, sizeof(set_path),
                       SYS_CPU_BASE "/cpu%d/cpufreq/scaling_setspeed", i);

            write_str(gov_path, "userspace");

            min_buf[0] = '\0';
            if (!read_str(min_path, min_buf, sizeof(min_buf)))
                min_buf[0] = '\0';

            if (min_buf[0] != '\0')
                write_str(set_path, min_buf);
        }
        g_strlcpy(cpu->runtime_cur_governor, "powersave", sizeof(cpu->runtime_cur_governor));
        return;
    }

    if (cpu->save_backend == CPU_SAVE_GOVERNOR_EXYNOS_LIMIT) {
        write_int(SYS_POWER_CPUFREQ_MAX_WRITE, cpu->exynos_min_limit);
        g_strlcpy(cpu->runtime_cur_governor, "powersave", sizeof(cpu->runtime_cur_governor));
        return;
    }

    g_debug("No CPU powersave backend available");
}

void
cpu_apply_default(CpuContext *cpu)
{
    if (!cpu)
        return;

    if (cpu->save_backend == CPU_SAVE_GOVERNOR_POWERSAVE) {
        for (int i = cpu->core_first; i <= cpu->core_last; i++) {
            char path[256];
            g_snprintf(path, sizeof(path),
                       SYS_CPU_BASE "/cpu%d/cpufreq/scaling_governor", i);
            write_str(path, cpu->default_governor);
        }
        g_strlcpy(cpu->runtime_cur_governor, cpu->default_governor, sizeof(cpu->runtime_cur_governor));
        return;
    }

    if (cpu->save_backend == CPU_SAVE_GOVERNOR_USERSPACE) {
        for (int i = cpu->core_first; i <= cpu->core_last; i++) {
            char gov_path[256];
            char max_path[256];
            char set_path[256];
            char max_buf[64];

            g_snprintf(gov_path, sizeof(gov_path),
                       SYS_CPU_BASE "/cpu%d/cpufreq/scaling_governor", i);
            g_snprintf(max_path, sizeof(max_path),
                       SYS_CPU_BASE "/cpu%d/cpufreq/scaling_max_freq", i);
            g_snprintf(set_path, sizeof(set_path),
                       SYS_CPU_BASE "/cpu%d/cpufreq/scaling_setspeed", i);

            write_str(gov_path, "userspace");

            max_buf[0] = '\0';
            if (!read_str(max_path, max_buf, sizeof(max_buf)))
                max_buf[0] = '\0';

            if (max_buf[0] != '\0')
                write_str(set_path, max_buf);
        }
        g_strlcpy(cpu->runtime_cur_governor, cpu->default_governor, sizeof(cpu->runtime_cur_governor));
        return;
    }

    if (cpu->save_backend == CPU_SAVE_GOVERNOR_EXYNOS_LIMIT) {
        write_int(SYS_POWER_CPUFREQ_MAX_WRITE, cpu->exynos_max_limit);
        write_int(SYS_POWER_CPUFREQ_MIN_WRITE, cpu->exynos_max_limit);
        g_strlcpy(cpu->runtime_cur_governor, cpu->default_governor, sizeof(cpu->runtime_cur_governor));
        return;
    }

    g_debug("No CPU default backend available");
}

void
cpu_apply_offline(CpuContext *cpu)
{
    if (!cpu)
        return;

    int start = cpu->first_pol_core;
    int end = cpu->last_pol_core;

    if (start > end) {
        g_debug("cpu_apply_offline: empty range %d..%d (no-op)", start, end);
        return;
    }

    /* exynos hotplug instability: never offline cpu0 */
    if (cpu->save_backend == CPU_SAVE_GOVERNOR_EXYNOS_LIMIT && start == 0)
        start = 1;

    g_debug("cpu_apply_offline: range %d..%d", start, end);

    for (int i = start; i <= end; i++) {
        char path[256];
        g_snprintf(path, sizeof(path),
                   SYS_CPU_BASE "/cpu%d/online", i);
        write_str(path, "0");
    }
}

void
cpu_apply_online(CpuContext *cpu)
{
    if (!cpu)
        return;

    int start = cpu->first_pol_core;
    int end = cpu->last_pol_core;

    if (start > end) {
        g_debug("cpu_apply_online: empty range %d..%d (no-op)", start, end);
        return;
    }

    if (cpu->save_backend == CPU_SAVE_GOVERNOR_EXYNOS_LIMIT && start == 0)
        start = 1;

    g_debug("cpu_apply_online: range %d..%d", start, end);

    for (int i = start; i <= end; i++) {
        char path[256];
        g_snprintf(path, sizeof(path),
                   SYS_CPU_BASE "/cpu%d/online", i);
        write_str(path, "1");
    }
}

void
cpu_restore_offline_limit(CpuContext *cpu, const BatmanConfig *cfg)
{
    if (!cpu || !cfg)
        return;

    cpu->last_pol_core = cpu->offline_count;
}

int
cpu_get_core_count(const CpuContext *cpu)
{
    if (!cpu)
        return 0;

    int count = (cpu->core_last - cpu->core_first) + 1;
    if (count < 0)
        return 0;

    return count;
}

gchar *
cpu_get_default_governor(void)
{
    char buf[256];
    buf[0] = '\0';

    if (!exists(DEFAULT_CPU_GOVERNOR_FILE))
        return g_strdup("");

    if (!read_str(DEFAULT_CPU_GOVERNOR_FILE, buf, sizeof(buf)))
        return g_strdup("");

    if (buf[0] == '\0')
        return g_strdup("");

    return make_valid_utf8(buf);
}
