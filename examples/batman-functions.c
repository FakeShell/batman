#include <batman/batman-wrappers.h>
#include <batman/wlrdisplay.h>
#include <batman/getinfo.h>
#include <batman/batman-waydroid.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    g_autoptr(UpClient) upower = NULL;

    upower = up_client_new();
    if (upower == NULL) {
        g_debug("Could not connect to upower");
        return 2;
    }

    gdouble battery_percentage = get_battery_percentage(upower);
    batman_state_t battery_state = get_battery_state(upower);
    const gchar *battery_status = get_battery_all(upower, NULL);

    double cpu = get_cpu_usage();
    long double mem = mem_usage();

    printf("CPU Usage: %.1f%%\n", cpu);
    printf("Memory Usage: %.1Lf%%\n", mem);

    if (check_batman_active() != -1)
        printf("Batman active status: %s\n", bm_state.active ? "active" : "inactive");
    else
        printf("Failed to check Batman active status\n");

    if (check_batman_enabled() != -1)
        printf("Batman enabled status: %s\n", bm_state.enabled ? "enabled" : "disabled");
    else
        printf("Failed to check Batman enabled status\n");

    int display_result = wlrdisplay(argc, argv);
    printf("wlroots screen status: %s", display_result == 0 ? "yes\n" : "no\n");

    if (battery_state != BATMAN_NO_BATTERY && battery_state != BATMAN_UNKNOWN)
        g_print("Battery status: %s\nBattery percentage: %.2f%%\n", battery_status, battery_percentage);
    else
        g_print("No battery found\n");

    g_autofree gchar *waydroid_state = waydroid_get_state();
    if (waydroid_state)
        g_print("Waydroid State: %s\n", waydroid_state);
    else
        g_print("Failed to get Waydroid state.\n");

    return 0;
}
