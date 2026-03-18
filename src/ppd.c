/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2026 Bardia Moshiri <bardia@furilabs.com>
 */

#include "ppd.h"
#include "binder.h"
#include "cpu.h"
#include "thermal.h"
#include "utils.h"

#define PPD_BUS_NAME "org.freedesktop.UPower.PowerProfiles"
#define PPD_OBJ_PATH "/org/freedesktop/UPower/PowerProfiles"
#define PPD_IFACE_NAME "org.freedesktop.UPower.PowerProfiles"

#define STATE_DIR "/var/lib/power-profiles-daemon"
#define STATE_FILE "/var/lib/power-profiles-daemon/state.ini"

#define GPUFREQ_OPP_DUMP_PATH "/proc/gpufreq/gpufreq_opp_dump"
#define GPUFREQ_OPP_FREQ_PATH "/proc/gpufreq/gpufreq_opp_freq"

typedef struct {
    guint32 cookie;
    char *profile;
    char *reason;
    char *application_id;
} PpdHold;

struct PpdContext {
    CpuContext *cpu; /* not owned */

    Binder *vr;
    Binder *mtkpower;

    GDBusConnection *bus;
    guint owner_id;
    guint reg_id;

    gboolean battery_aware;

    char *active_profile;
    char *performance_inhibited;
    char *performance_degraded;
    gboolean overdrive_enabled;

    guint32 next_cookie;
    GPtrArray *holds;

    guint thermal_timer_id;
};

static const gchar introspection_xml[] =
    "<node>"
    "  <interface name='org.freedesktop.UPower.PowerProfiles'>"
    "    <method name='HoldProfile'>"
    "      <arg name='profile' type='s' direction='in'/>"
    "      <arg name='reason' type='s' direction='in'/>"
    "      <arg name='application_id' type='s' direction='in'/>"
    "      <arg name='cookie' type='u' direction='out'/>"
    "    </method>"
    "    <method name='ReleaseProfile'>"
    "      <arg name='cookie' type='u' direction='in'/>"
    "    </method>"
    "    <method name='SetActionEnabled'>"
    "      <arg name='action' type='s' direction='in'/>"
    "      <arg name='enabled' type='b' direction='in'/>"
    "    </method>"
    "    <method name='EnableOverdrive'>"
    "      <arg name='enabled' type='b' direction='in'/>"
    "    </method>"
    "    <signal name='ProfileReleased'>"
    "      <arg name='cookie' type='u'/>"
    "    </signal>"
    "    <property name='ActiveProfile' type='s' access='readwrite'/>"
    "    <property name='PerformanceInhibited' type='s' access='read'/>"
    "    <property name='PerformanceDegraded' type='s' access='read'/>"
    "    <property name='Profiles' type='aa{sv}' access='read'/>"
    "    <property name='Actions' type='as' access='read'/>"
    "    <property name='ActionsInfo' type='aa{sv}' access='read'/>"
    "    <property name='ActiveProfileHolds' type='aa{sv}' access='read'/>"
    "    <property name='Version' type='s' access='read'/>"
    "    <property name='BatteryAware' type='b' access='readwrite'/>"
    "    <property name='Overdrive' type='b' access='read'/>"
    "  </interface>"
    "</node>";

static GDBusNodeInfo *introspection = NULL;

static gchar *
ppd_normalize_profile(const gchar *profile)
{
    g_autofree gchar *p = make_valid_utf8(profile);

    if (g_strcmp0(p, "power-saver") == 0)
        return g_strdup("power-saver");
    if (g_strcmp0(p, "balanced") == 0)
        return g_strdup("balanced");
    if (g_strcmp0(p, "performance") == 0)
        return g_strdup("performance");

    return g_strdup("balanced");
}

static void
emit_properties_changed(PpdContext *ppd, GVariantBuilder *changed_props)
{
    if (!ppd || !ppd->bus || !changed_props)
        return;

    GVariantBuilder invalidated;
    g_variant_builder_init(&invalidated, G_VARIANT_TYPE("as"));

    g_dbus_connection_emit_signal(ppd->bus,
                                  NULL,
                                  PPD_OBJ_PATH,
                                  "org.freedesktop.DBus.Properties",
                                  "PropertiesChanged",
                                  g_variant_new("(sa{sv}as)",
                                                PPD_IFACE_NAME,
                                                changed_props,
                                                &invalidated),
                                  NULL);
}

static void
ensure_state_file(void)
{
    ensure_dir(STATE_DIR, 0755);

    if (exists(STATE_FILE))
        return;

    GKeyFile *kf = g_key_file_new();
    g_key_file_set_string(kf, "State", "Driver", "batman");
    g_key_file_set_string(kf, "State", "Profile", "balanced");
    g_key_file_set_string(kf, "State", "CpuDriver", "batman");
    g_key_file_set_string(kf, "State", "PlatformDriver", "batman");

    gsize len = 0;
    g_autofree gchar *data = g_key_file_to_data(kf, &len, NULL);
    if (data && len > 0)
        write_str(STATE_FILE, data);

    g_key_file_unref(kf);
}

static gchar *
state_get_profile(void)
{
    ensure_state_file();

    GKeyFile *kf = g_key_file_new();
    g_autoptr(GError) error = NULL;

    if (!g_key_file_load_from_file(kf, STATE_FILE, G_KEY_FILE_NONE, &error)) {
        g_key_file_unref(kf);
        return g_strdup("balanced");
    }

    g_autofree gchar *profile = g_key_file_get_string(kf, "State", "Profile", NULL);
    if (!profile || profile[0] == '\0') {
        g_free(profile);
        profile = g_key_file_get_string(kf, "State", "profile", NULL);
    }

    g_key_file_unref(kf);
    return ppd_normalize_profile(profile);
}

static void
state_set_profile(const char *profile)
{
    ensure_state_file();

    g_autofree gchar *norm = ppd_normalize_profile(profile);

    GKeyFile *kf = g_key_file_new();
    g_autoptr(GError) error = NULL;

    if (g_key_file_load_from_file(kf, STATE_FILE, G_KEY_FILE_NONE, &error))
        g_key_file_remove_group(kf, "State", NULL);

    g_key_file_set_string(kf, "State", "Driver", "batman");
    g_key_file_set_string(kf, "State", "Profile", (norm && norm[0]) ? norm : "balanced");
    g_key_file_set_string(kf, "State", "CpuDriver", "batman");
    g_key_file_set_string(kf, "State", "PlatformDriver", "batman");

    gsize len = 0;
    g_autofree gchar *data = g_key_file_to_data(kf, &len, NULL);
    if (data && len > 0)
        write_str(STATE_FILE, data);

    g_key_file_unref(kf);
}

static void
parse_gpufreq_opp_dump(long long *out_low, long long *out_high)
{
    if (out_low)
        *out_low = 0;
    if (out_high)
        *out_high = 0;

    if (!exists(GPUFREQ_OPP_DUMP_PATH))
        return;

    FILE *fp = fopen(GPUFREQ_OPP_DUMP_PATH, "r");
    if (!fp)
        return;

    long long low = 0;
    long long high = 0;

    char line[512];
    while (fgets(line, sizeof(line), fp) != NULL) {
        const char *p = strstr(line, "freq");
        if (!p)
            continue;

        const char *eq = strchr(p, '=');
        if (!eq)
            continue;

        eq++;
        while (*eq == ' ' || *eq == '\t')
            eq++;

        errno = 0;
        char *endp = NULL;
        long long v = strtoll(eq, &endp, 10);
        if (errno != 0 || endp == eq)
            continue;

        if (low == 0 || v < low)
            low = v;
        if (v > high)
            high = v;
    }

    fclose(fp);

    if (out_low)
        *out_low = low;
    if (out_high)
        *out_high = high;
}

static void
write_gpufreq_opp_freq(long long freq)
{
    if (!exists(GPUFREQ_OPP_FREQ_PATH))
        return;

    char buf[64];
    g_snprintf(buf, sizeof(buf), "%lld", freq);
    write_str(GPUFREQ_OPP_FREQ_PATH, buf);
}

static void
hold_free(PpdHold *h)
{
    if (!h)
        return;
    g_free(h->profile);
    g_free(h->reason);
    g_free(h->application_id);
    g_free(h);
}

static GVariant *
build_holds_variant(PpdContext *ppd)
{
    GVariantBuilder arr;
    g_variant_builder_init(&arr, G_VARIANT_TYPE("aa{sv}"));

    if (!ppd || !ppd->holds)
        return g_variant_builder_end(&arr);

    for (guint i = 0; i < ppd->holds->len; i++) {
        PpdHold *h = g_ptr_array_index(ppd->holds, i);
        if (!h)
            continue;

        g_autofree gchar *p = ppd_normalize_profile(h->profile);
        g_autofree gchar *r = make_valid_utf8(h->reason);
        g_autofree gchar *a = make_valid_utf8(h->application_id);

        GVariantBuilder dict;
        g_variant_builder_init(&dict, G_VARIANT_TYPE("a{sv}"));

        g_variant_builder_add(&dict, "{sv}", "Profile", g_variant_new_string(p));
        g_variant_builder_add(&dict, "{sv}", "Reason", g_variant_new_string(r));
        g_variant_builder_add(&dict, "{sv}", "ApplicationId", g_variant_new_string(a));
        g_variant_builder_add(&dict, "{sv}", "Cookie", g_variant_new_uint32(h->cookie));

        g_variant_builder_add_value(&arr, g_variant_builder_end(&dict));
    }

    return g_variant_builder_end(&arr);
}

static guint32
holds_add(PpdContext *ppd, const char *profile, const char *reason, const char *app_id)
{
    if (!ppd)
        return 0;

    ppd->next_cookie++;

    PpdHold *h = g_new0(PpdHold, 1);
    h->cookie = ppd->next_cookie;

    h->profile = ppd_normalize_profile(profile);
    h->reason = make_valid_utf8(reason);
    h->application_id = make_valid_utf8(app_id);

    g_ptr_array_add(ppd->holds, h);
    return h->cookie;
}

static gboolean
holds_remove(PpdContext *ppd, guint32 cookie)
{
    if (!ppd || !ppd->holds)
        return FALSE;

    for (guint i = 0; i < ppd->holds->len; i++) {
        PpdHold *h = g_ptr_array_index(ppd->holds, i);
        if (h && h->cookie == cookie) {
            g_ptr_array_remove_index(ppd->holds, i);
            hold_free(h);
            return TRUE;
        }
    }
    return FALSE;
}

static GVariant *
build_profiles_variant(void)
{
    GVariantBuilder arr;
    g_variant_builder_init(&arr, G_VARIANT_TYPE("aa{sv}"));

    const char *names[] = { "power-saver", "balanced", "performance" };

    for (guint i = 0; i < G_N_ELEMENTS(names); i++) {
        GVariantBuilder dict;
        g_variant_builder_init(&dict, G_VARIANT_TYPE("a{sv}"));
        g_variant_builder_add(&dict, "{sv}", "Profile", g_variant_new_string(names[i]));
        g_variant_builder_add(&dict, "{sv}", "Driver", g_variant_new_string("batman"));
        g_variant_builder_add_value(&arr, g_variant_builder_end(&dict));
    }

    return g_variant_builder_end(&arr);
}

static GVariant *
build_actions_variant(void)
{
    GVariantBuilder b;
    g_variant_builder_init(&b, G_VARIANT_TYPE("as"));
    g_variant_builder_add(&b, "s", "trickle_charge");
    return g_variant_builder_end(&b);
}

static GVariant *
build_actions_info_variant(void)
{
    GVariantBuilder arr;
    g_variant_builder_init(&arr, G_VARIANT_TYPE("aa{sv}"));

    GVariantBuilder dict;
    g_variant_builder_init(&dict, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&dict, "{sv}", "Name", g_variant_new_string("trickle_charge"));
    g_variant_builder_add(&dict, "{sv}", "Description",
                          g_variant_new_string("Configure power supply to trickle charge"));
    g_variant_builder_add(&dict, "{sv}", "Enabled", g_variant_new_boolean(TRUE));
    g_variant_builder_add_value(&arr, g_variant_builder_end(&dict));

    return g_variant_builder_end(&arr);
}

static void
apply_backend_profile(PpdContext *ppd,
                      const char *profile,
                      gboolean force_allow_performance)
{
    if (!ppd || !profile)
        return;

    CpuContext *cpu = ppd->cpu;
    g_autofree gchar *default_gov = cpu_get_default_governor();

    gboolean allow_perf = TRUE;
    if (!force_allow_performance) {
        if (ppd->performance_degraded && ppd->performance_degraded[0] != '\0')
            allow_perf = FALSE;
    }

    if (g_strcmp0(profile, "performance") == 0 && allow_perf) {
        if (ppd->vr)
            binder_set_vr_hidl(ppd->vr, TRUE);
        if (ppd->mtkpower)
            binder_set_mtkpower_hint_hidl(ppd->mtkpower, MTK_POWER_HINT_UX_FOCUS);

        if (exists(CUSTOM_FIRSTPOLCORE_FILE))
            unlink(CUSTOM_FIRSTPOLCORE_FILE);

        write_str(CUSTOM_DEFAULT_GOVERNOR_FILE, "performance");

        long long low = 0, high = 0;
        parse_gpufreq_opp_dump(&low, &high);
        if (high > 0)
            write_gpufreq_opp_freq(high);

        if (cpu) {
            cpu_refresh_overrides(cpu);
            cpu_apply_online(cpu);
            cpu_apply_default(cpu);
        }
        return;
    }

    if (g_strcmp0(profile, "balanced") == 0) {
        if (ppd->vr)
            binder_set_vr_hidl(ppd->vr, FALSE);
        if (ppd->mtkpower)
            binder_set_mtkpower_hint_hidl(ppd->mtkpower, MTK_POWER_HINT_PACK_SWITCH);

        if (exists(CUSTOM_FIRSTPOLCORE_FILE))
            unlink(CUSTOM_FIRSTPOLCORE_FILE);

        if (default_gov && default_gov[0] != '\0')
            write_str(CUSTOM_DEFAULT_GOVERNOR_FILE, default_gov);
        else if (exists(CUSTOM_DEFAULT_GOVERNOR_FILE))
            unlink(CUSTOM_DEFAULT_GOVERNOR_FILE);

        write_gpufreq_opp_freq(0);

        if (cpu) {
            cpu_refresh_overrides(cpu);
            cpu_apply_online(cpu);
            cpu_apply_default(cpu);
        }
        return;
    }

    if (g_strcmp0(profile, "power-saver") == 0) {
        if (ppd->vr)
            binder_set_vr_hidl(ppd->vr, FALSE);
        if (ppd->mtkpower)
            binder_set_mtkpower_hint_hidl(ppd->mtkpower, MTK_POWER_HINT_PACK_SWITCH);

        if (exists(CUSTOM_FIRSTPOLCORE_FILE))
            unlink(CUSTOM_FIRSTPOLCORE_FILE);

        if (default_gov && default_gov[0] != '\0')
            write_str(CUSTOM_DEFAULT_GOVERNOR_FILE, default_gov);
        else if (exists(CUSTOM_DEFAULT_GOVERNOR_FILE))
            unlink(CUSTOM_DEFAULT_GOVERNOR_FILE);

        write_str(CUSTOM_DEFAULT_GOVERNOR_FILE, "powersave");

        long long low = 0, high = 0;
        parse_gpufreq_opp_dump(&low, &high);
        if (low > 0)
            write_gpufreq_opp_freq(low);

        if (cpu) {
            cpu_refresh_overrides(cpu);
            cpu_apply_online(cpu);
            cpu_apply_default(cpu);
        }
        return;
    }
}

static void
ppd_set_active_profile_internal(PpdContext *ppd,
                                const char *profile,
                                gboolean write_state,
                                gboolean force,
                                gboolean treat_as_overdrive,
                                gboolean update_active)
{
    if (!ppd || !profile)
        return;

    g_autofree gchar *norm_profile = ppd_normalize_profile(profile);

    if (ppd->overdrive_enabled && !treat_as_overdrive) {
        if (write_state)
            state_set_profile(norm_profile);

        if (update_active) {
            g_free(ppd->active_profile);
            ppd->active_profile = g_strdup(norm_profile);

            GVariantBuilder changed;
            g_variant_builder_init(&changed, G_VARIANT_TYPE("a{sv}"));
            g_variant_builder_add(&changed, "{sv}", "ActiveProfile",
                                  g_variant_new_string(ppd->active_profile));
            emit_properties_changed(ppd, &changed);
        }
        return;
    }

    apply_backend_profile(ppd, norm_profile, force ? TRUE : FALSE);

    if (update_active) {
        g_free(ppd->active_profile);
        ppd->active_profile = g_strdup(norm_profile);

        GVariantBuilder changed;
        g_variant_builder_init(&changed, G_VARIANT_TYPE("a{sv}"));
        g_variant_builder_add(&changed, "{sv}", "ActiveProfile",
                              g_variant_new_string(ppd->active_profile));
        emit_properties_changed(ppd, &changed);
    }

    if (write_state && !treat_as_overdrive)
        state_set_profile(norm_profile);
}

static void
ppd_set_overdrive(PpdContext *ppd, gboolean enabled)
{
    if (!ppd)
        return;

    enabled = enabled ? TRUE : FALSE;

    if (enabled && !ppd->overdrive_enabled) {
        ppd->overdrive_enabled = TRUE;

        GVariantBuilder changed;
        g_variant_builder_init(&changed, G_VARIANT_TYPE("a{sv}"));
        g_variant_builder_add(&changed, "{sv}", "Overdrive", g_variant_new_boolean(TRUE));
        emit_properties_changed(ppd, &changed);

        ppd_set_active_profile_internal(ppd,
                                        "performance",
                                        FALSE,
                                        TRUE,
                                        TRUE,
                                        FALSE);
        return;
    }

    if (!enabled && ppd->overdrive_enabled) {
        ppd->overdrive_enabled = FALSE;

        GVariantBuilder changed;
        g_variant_builder_init(&changed, G_VARIANT_TYPE("a{sv}"));
        g_variant_builder_add(&changed, "{sv}", "Overdrive", g_variant_new_boolean(FALSE));
        emit_properties_changed(ppd, &changed);

        g_autofree gchar *restore = state_get_profile();
        if (!restore || restore[0] == '\0')
            restore = g_strdup("balanced");

        ppd_set_active_profile_internal(ppd,
                                        restore,
                                        FALSE,
                                        TRUE,
                                        FALSE,
                                        TRUE);
        return;
    }
}

gboolean
ppd_is_overdrive_enabled(const PpdContext *ppd)
{
    if (!ppd)
        return FALSE;

    return ppd->overdrive_enabled ? TRUE : FALSE;
}

static gboolean
thermal_poll_cb(gpointer userdata)
{
    PpdContext *ppd = userdata;
    if (!ppd)
        return G_SOURCE_CONTINUE;

    double avg = 0.0;
    gboolean ok = thermal_get_average_celsius(&avg);
    if (ok)
        g_debug("thermal: average = %.2f C", avg);
    else
        g_debug("thermal: average read failed");

    const char *new_val = "";
    if (ok && avg > 50.0)
        new_val = "high-operating-temperature";

    if (g_strcmp0(ppd->performance_degraded, new_val) != 0) {
        g_free(ppd->performance_degraded);
        ppd->performance_degraded = g_strdup(new_val);

        GVariantBuilder changed;
        g_variant_builder_init(&changed, G_VARIANT_TYPE("a{sv}"));
        g_variant_builder_add(&changed, "{sv}", "PerformanceDegraded",
                              g_variant_new_string(ppd->performance_degraded));
        emit_properties_changed(ppd, &changed);
    }

    return G_SOURCE_CONTINUE;
}

static GVariant *
ppd_get_property(GDBusConnection *connection,
                 const gchar *sender,
                 const gchar *object_path,
                 const gchar *interface_name,
                 const gchar *property_name,
                 GError **error,
                 gpointer user_data)
{
    (void)connection;
    (void)sender;
    (void)object_path;
    (void)interface_name;
    (void)error;

    PpdContext *ppd = user_data;
    if (!ppd || !property_name)
        return NULL;

    if (g_strcmp0(property_name, "ActiveProfile") == 0) {
        g_autofree gchar *p = ppd_normalize_profile(ppd->active_profile);
        return g_variant_new_string(p);
    }

    if (g_strcmp0(property_name, "PerformanceInhibited") == 0) {
        g_autofree gchar *s = make_valid_utf8(ppd->performance_inhibited);
        return g_variant_new_string(s);
    }

    if (g_strcmp0(property_name, "PerformanceDegraded") == 0) {
        g_autofree gchar *s = make_valid_utf8(ppd->performance_degraded);
        return g_variant_new_string(s);
    }

    if (g_strcmp0(property_name, "Profiles") == 0)
        return build_profiles_variant();

    if (g_strcmp0(property_name, "Actions") == 0)
        return build_actions_variant();

    if (g_strcmp0(property_name, "ActionsInfo") == 0)
        return build_actions_info_variant();

    if (g_strcmp0(property_name, "ActiveProfileHolds") == 0)
        return build_holds_variant(ppd);

    if (g_strcmp0(property_name, "Version") == 0)
        return g_variant_new_string("0.30");

    if (g_strcmp0(property_name, "BatteryAware") == 0)
        return g_variant_new_boolean(ppd->battery_aware);

    if (g_strcmp0(property_name, "Overdrive") == 0)
        return g_variant_new_boolean(ppd->overdrive_enabled);

    return NULL;
}

static gboolean
ppd_set_property(GDBusConnection *connection,
                 const gchar *sender,
                 const gchar *object_path,
                 const gchar *interface_name,
                 const gchar *property_name,
                 GVariant *value,
                 GError **error,
                 gpointer user_data)
{
    (void)connection;
    (void)sender;
    (void)object_path;
    (void)interface_name;

    PpdContext *ppd = user_data;
    if (!ppd || !property_name || !value)
        return FALSE;

    if (g_strcmp0(property_name, "ActiveProfile") == 0) {
        const char *in = g_variant_get_string(value, NULL);
        g_autofree gchar *profile = ppd_normalize_profile(in);

        ppd_set_active_profile_internal(ppd,
                                        profile,
                                        TRUE,
                                        FALSE,
                                        FALSE,
                                        TRUE);
        return TRUE;
    }

    if (g_strcmp0(property_name, "BatteryAware") == 0) {
        gboolean b = g_variant_get_boolean(value);
        ppd->battery_aware = b ? TRUE : FALSE;

        GVariantBuilder changed;
        g_variant_builder_init(&changed, G_VARIANT_TYPE("a{sv}"));
        g_variant_builder_add(&changed, "{sv}", "BatteryAware",
                              g_variant_new_boolean(ppd->battery_aware));
        emit_properties_changed(ppd, &changed);
        return TRUE;
    }

    g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "Property not writable");
    return FALSE;
}

static void
ppd_emit_profile_released(PpdContext *ppd, guint32 cookie)
{
    if (!ppd || !ppd->bus)
        return;

    g_dbus_connection_emit_signal(ppd->bus,
                                  NULL,
                                  PPD_OBJ_PATH,
                                  PPD_IFACE_NAME,
                                  "ProfileReleased",
                                  g_variant_new("(u)", cookie),
                                  NULL);
}

static void
ppd_method_call(GDBusConnection *connection,
                const gchar *sender,
                const gchar *object_path,
                const gchar *interface_name,
                const gchar *method_name,
                GVariant *parameters,
                GDBusMethodInvocation *invocation,
                gpointer user_data)
{
    (void)connection;
    (void)sender;
    (void)object_path;
    (void)interface_name;

    PpdContext *ppd = user_data;

    if (g_strcmp0(method_name, "HoldProfile") == 0) {
        const char *profile = NULL;
        const char *reason = NULL;
        const char *app_id = NULL;

        g_variant_get(parameters, "(&s&s&s)", &profile, &reason, &app_id);

        g_debug("ppd: HoldProfile(profile=%s, reason=%s, application_id=%s)",
                profile ? profile : "",
                reason ? reason : "",
                app_id ? app_id : "");

        guint32 cookie = holds_add(ppd, profile, reason, app_id);

        GVariantBuilder changed;
        g_variant_builder_init(&changed, G_VARIANT_TYPE("a{sv}"));
        g_variant_builder_add(&changed, "{sv}", "ActiveProfileHolds", build_holds_variant(ppd));
        emit_properties_changed(ppd, &changed);

        g_dbus_method_invocation_return_value(invocation, g_variant_new("(u)", cookie));
        return;
    }

    if (g_strcmp0(method_name, "ReleaseProfile") == 0) {
        guint32 cookie = 0;
        g_variant_get(parameters, "(u)", &cookie);

        g_debug("ppd: ReleaseProfile(cookie=%u)", cookie);

        holds_remove(ppd, cookie);

        GVariantBuilder changed;
        g_variant_builder_init(&changed, G_VARIANT_TYPE("a{sv}"));
        g_variant_builder_add(&changed, "{sv}", "ActiveProfileHolds", build_holds_variant(ppd));
        emit_properties_changed(ppd, &changed);

        ppd_emit_profile_released(ppd, cookie);
        g_dbus_method_invocation_return_value(invocation, NULL);
        return;
    }

    if (g_strcmp0(method_name, "SetActionEnabled") == 0) {
        const char *action = NULL;
        gboolean enabled = FALSE;
        g_variant_get(parameters, "(&sb)", &action, &enabled);

        g_debug("ppd: SetActionEnabled(action=%s, enabled=%s)",
                action ? action : "",
                enabled ? "true" : "false");

        g_dbus_method_invocation_return_value(invocation, NULL);
        return;
    }

    if (g_strcmp0(method_name, "EnableOverdrive") == 0) {
        gboolean enabled = FALSE;
        g_variant_get(parameters, "(b)", &enabled);

        g_debug("ppd: EnableOverdrive(enabled=%s)", enabled ? "true" : "false");

        ppd_set_overdrive(ppd, enabled);
        g_dbus_method_invocation_return_value(invocation, NULL);
        return;
    }

    g_debug("ppd: unknown method call %s", method_name ? method_name : "(null)");

    g_dbus_method_invocation_return_error(invocation,
                                          G_IO_ERROR,
                                          G_IO_ERROR_NOT_SUPPORTED,
                                          "Unknown method %s",
                                          method_name);
}

static const GDBusInterfaceVTable interface_vtable = {
    .method_call = ppd_method_call,
    .get_property = ppd_get_property,
    .set_property = ppd_set_property
};

static void
on_bus_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
    (void)name;

    PpdContext *ppd = user_data;
    ppd->bus = g_object_ref(connection);

    g_autoptr(GError) error = NULL;

    ppd->reg_id = g_dbus_connection_register_object(connection,
                                                    PPD_OBJ_PATH,
                                                    introspection->interfaces[0],
                                                    &interface_vtable,
                                                    ppd,
                                                    NULL,
                                                    &error);
    if (ppd->reg_id == 0) {
        g_warning("ppd: failed to register object: %s", error ? error->message : "unknown error");
        return;
    }

    ensure_state_file();
    g_autofree gchar *saved = state_get_profile();
    if (!saved || saved[0] == '\0')
        saved = g_strdup("balanced");

    g_free(ppd->active_profile);
    ppd->active_profile = g_strdup(saved);

    ppd_set_active_profile_internal(ppd,
                                    ppd->active_profile,
                                    FALSE,
                                    TRUE,
                                    FALSE,
                                    TRUE);

    if (ppd->thermal_timer_id == 0)
        ppd->thermal_timer_id = g_timeout_add_seconds(30, thermal_poll_cb, ppd);

    thermal_poll_cb(ppd);

    g_debug("ppd: exported %s at %s", PPD_IFACE_NAME, PPD_OBJ_PATH);
}

static void
on_name_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
    (void)connection;
    (void)name;
    (void)user_data;

    g_debug("ppd: name acquired");
}

static void
on_name_lost(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
    (void)connection;
    (void)name;

    PpdContext *ppd = user_data;
    if (!ppd)
        return;

    g_warning("ppd: lost bus name %s", PPD_BUS_NAME);

    if (ppd->thermal_timer_id) {
        g_source_remove(ppd->thermal_timer_id);
        ppd->thermal_timer_id = 0;
    }

    if (ppd->bus && ppd->reg_id) {
        g_dbus_connection_unregister_object(ppd->bus, ppd->reg_id);
        ppd->reg_id = 0;
    }

    if (ppd->bus)
        g_clear_object(&ppd->bus);
}

PpdContext *
ppd_init(CpuContext *cpu)
{
    if (!introspection) {
        g_autoptr(GError) error = NULL;
        introspection = g_dbus_node_info_new_for_xml(introspection_xml, &error);
        if (!introspection) {
            g_warning("ppd: introspection parse failed: %s", error ? error->message : "unknown error");
            return NULL;
        }
    }

    PpdContext *ppd = g_new0(PpdContext, 1);
    ppd->cpu = cpu;

    ppd->vr = binder_init_vr_hidl();
    ppd->mtkpower = binder_init_mtkpower_hidl();

    ppd->battery_aware = TRUE;
    ppd->active_profile = g_strdup("balanced");
    ppd->performance_inhibited = g_strdup("");
    ppd->performance_degraded = g_strdup("");
    ppd->overdrive_enabled = FALSE;
    ppd->next_cookie = 0;
    ppd->holds = g_ptr_array_new_with_free_func((GDestroyNotify)hold_free);

    ppd->owner_id = g_bus_own_name(G_BUS_TYPE_SYSTEM,
                                  PPD_BUS_NAME,
                                  G_BUS_NAME_OWNER_FLAGS_NONE,
                                  on_bus_acquired,
                                  on_name_acquired,
                                  on_name_lost,
                                  ppd,
                                  NULL);

    if (ppd->owner_id == 0) {
        g_warning("ppd: failed to own name %s", PPD_BUS_NAME);
        ppd_cleanup(ppd);
        return NULL;
    }

    return ppd;
}

void
ppd_cleanup(PpdContext *ppd)
{
    if (!ppd)
        return;

    if (ppd->thermal_timer_id) {
        g_source_remove(ppd->thermal_timer_id);
        ppd->thermal_timer_id = 0;
    }

    if (ppd->owner_id) {
        g_bus_unown_name(ppd->owner_id);
        ppd->owner_id = 0;
    }

    if (ppd->bus && ppd->reg_id) {
        g_dbus_connection_unregister_object(ppd->bus, ppd->reg_id);
        ppd->reg_id = 0;
    }

    g_clear_object(&ppd->bus);

    if (ppd->holds) {
        g_ptr_array_free(ppd->holds, TRUE);
        ppd->holds = NULL;
    }

    if (ppd->vr) {
        binder_cleanup(ppd->vr);
        ppd->vr = NULL;
    }

    if (ppd->mtkpower) {
        binder_cleanup(ppd->mtkpower);
        ppd->mtkpower = NULL;
    }

    g_clear_pointer(&ppd->active_profile, g_free);
    g_clear_pointer(&ppd->performance_inhibited, g_free);
    g_clear_pointer(&ppd->performance_degraded, g_free);

    g_free(ppd);
}
