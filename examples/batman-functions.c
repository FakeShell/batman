#include <batman/batman-wrappers.h>
#include <batman/wlrdisplay.h>
#include <batman/getinfo.h>
#include <batman/batman-waydroid.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    UpClient *upower = up_client_new();
    gdouble battery_percentage;

    if (upower == NULL) {
        g_print("Could not connect to upower");
        return 2;
    }

    const gchar *statelabel = findBattery(upower, &battery_percentage);

    g_object_unref(upower);

    int cpu_usage = cpuUsage();
    printf("CPU Usage: %d%%\n", cpu_usage);

    int mem_usage = memUsage();
    printf("Memory Usage: %d%%\n", mem_usage);

    if (check_batman_active() != -1) {
        printf("Batman active status: %s\n", bm_state.active ? "active" : "inactive");
    } else {
        printf("Failed to check Batman active status\n");
    }

    if (check_batman_enabled() != -1) {
        printf("Batman enabled status: %s\n", bm_state.enabled ? "enabled" : "disabled");
    } else {
        printf("Failed to check Batman enabled status\n");
    }

    int result = wlrdisplay(argc, argv);
    printf("wlroots screen status: %s", result == 0 ? "yes\n" : "no\n");

    if (statelabel != NULL) {
        g_print("Battery status: %s\nBattery percentage: %.2f%%\n", statelabel, battery_percentage);
    } else {
        g_print("No battery found\n");
    }

    gchar *state = waydroid_get_state();

    if (state) {
        g_print("Waydroid State: %s\n", state);
        g_free(state);
    } else {
        g_print("Failed to get Waydroid state.\n");
    }

    return 0;
}
