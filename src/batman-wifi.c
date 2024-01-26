// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2018 Jolla Ltd
// Copyright (C) 2024 Bardia Moshiri <fakeshell@bardia.tech>

#include <net/if.h>
#include <stdint.h>
#include <netlink/errno.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <linux/nl80211.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define QUOTE(x) #x
#define STRINGIFY(x) QUOTE(x)

#define SET_WOWLAN(iface)   do { \
                                if (!wowlan_##iface##_enabled) { \
                                    wowlan_##iface##_enabled = (suspend_set_wowlan(STRINGIFY(iface)) == 0); \
                                } \
                            } while(false)

#define WMTWIFI_DEVICE "/dev/wmtWifi"
#define TESTMODE_CMD_ID_SUSPEND 101
#define WMTWIFI_SUSPEND_VALUE   (1)
#define WMTWIFI_RESUME_VALUE    (0)

#define PRIV_CMD_SIZE 512
typedef struct android_wifi_priv_cmd {
    char buf[PRIV_CMD_SIZE];
    int used_len;
    int total_len;
} android_wifi_priv_cmd;

struct testmode_cmd_hdr {
    uint32_t idx;
    uint32_t buflen;
};

struct testmode_cmd_suspend {
    struct testmode_cmd_hdr header;
    uint8_t suspend;
};

static struct nl_sock *nl_socket = NULL;
static int driver_id = -1;

static
int
handle_nl_command_valid(
    struct nl_msg *msg,
    void *arg)
{
    int *ret = arg;
    *ret = 0;
    //printf("%d", *ret);
    return NL_SKIP;
}

static
int
handle_nl_command_error(
    struct sockaddr_nl *nla,
    struct nlmsgerr *err,
    void *arg)
{
    int *ret = arg;
    *ret = err->error;
    //printf("%s: error: %d", __func__, *ret);
    return NL_SKIP;
}

static
int
handle_nl_command_finished(
    struct nl_msg *msg,
    void *arg)
{
    int *ret = arg;
    *ret = 0;
    //printf("%d", *ret);
    return NL_SKIP;
}

static
int
handle_nl_command_ack(
    struct nl_msg *msg,
    void *arg)
{
    int *ret = arg;
    *ret = 0;
    //printf("%d", *ret);
    return NL_STOP;
}

static
int
handle_nl_seq_check(
    struct nl_msg *msg,
    void *arg)
{
    return NL_OK;
}

static
int
suspend_plugin_netlink_handler()
{
    struct nl_cb *cb;
    int res = 0;
    int err = 0;

    cb = nl_cb_alloc(NL_CB_VERBOSE);
    if (!cb) {
        //printf("%s: failed to allocate netlink callbacks", __func__);
        return 1;
    }

    err = 1;
    nl_cb_err(cb, NL_CB_CUSTOM, handle_nl_command_error, &err);
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, handle_nl_command_valid, &err);
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, handle_nl_command_finished, &err);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, handle_nl_command_ack, &err);
    nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, handle_nl_seq_check, &err);

    while (err == 1) {
        //printf("waiting until nl testmode command has been processed\n");
        res = nl_recvmsgs(nl_socket, cb);
        if (res < 0) {
            //printf("nl_recvmsgs failed - wmtWifi %s:%d\n", __func__, res);
            break;
        }
    }

    if (err == 0) {
        //printf("suspend on/off successfully done");
    }

    nl_cb_put(cb);

    return err;
}

static
int
suspend_set_wowlan(
    const char *ifname)
{
    int err = 0;
    struct nl_msg *msg;

    struct nlattr *wowlan_triggers;

    int ifindex = 0;

    ifindex = if_nametoindex(ifname);

    if (ifindex == 0) {
        if (!strcmp(ifname, "wlan0")) {
            //printf("iface %s is not active/present (set_wowlan).", ifname);
        } else {
            //printf("iface %s is not active/present (set_wowlan).", ifname);
        }
        return -1;
    }

    //printf("iface %s, setting wowlan.", ifname);

    msg = nlmsg_alloc();

    genlmsg_put(msg, 0, 0, driver_id, 0, 0, NL80211_CMD_SET_WOWLAN, 0);

    nla_put_u32(msg, NL80211_ATTR_IFINDEX, ifindex);

    wowlan_triggers = nla_nest_start(msg, NL80211_ATTR_WOWLAN_TRIGGERS);

    nla_put_flag(msg, NL80211_WOWLAN_TRIG_ANY);

    nla_nest_end(msg, wowlan_triggers);

    if ((err = nl_send_auto(nl_socket, msg)) < 0) {
        //printf("Failed to send wowlan command.\n");
    } else {
        if ((err = suspend_plugin_netlink_handler()) != 0) {
            //printf("%s: setting wowlan failed for %s with error %d\n", __func__, ifname, err);
        }
    }

    nlmsg_free(msg);
    return err;
}

/**
 * @brief Set powersave state (on/off), same as calling `iw dev %ifname% set power_save %is_enable%`
 *
 * @param ifname interface name (e.g. wlan0)
 * @param is_enable enable/disable state
 */
static
void
suspend_set_powersave(
    const char *ifname, bool is_enable)
{
    int err = 0;
    struct nl_msg *msg;

    enum nl80211_ps_state ps_state;

    int ifindex = 0;

    ifindex = if_nametoindex(ifname);

    if (ifindex == 0) {
        if (!strcmp(ifname, "wlan0")) {
            //printf("iface %s is not active/present (set_powersave).", ifname);
        } else {
            //printf("iface %s is not active/present (set_powersave).", ifname);
        }
        return;
    }

    //printf("iface %s, setting powersave.", ifname);

    msg = nlmsg_alloc();

    genlmsg_put(msg, 0, 0, driver_id, 0, 0, NL80211_CMD_SET_POWER_SAVE, 0);

    nla_put_u32(msg, NL80211_ATTR_IFINDEX, ifindex);

    if (is_enable)
        ps_state = NL80211_PS_ENABLED;
    else
        ps_state = NL80211_PS_DISABLED;

    nla_put_u32(msg, NL80211_ATTR_PS_STATE, ps_state);

    if ((err = nl_send_auto(nl_socket, msg)) < 0) {
        //printf("Failed to send powersave command.\n");
    } else {
        if ((err = suspend_plugin_netlink_handler()) != 0) {
            //printf("%s: setting powersave failed for %s with error %d\n", __func__, ifname, err);
        }
    }

    nlmsg_free(msg);
}

/**
 * @brief Suspend or resume wmtWifi device with gen2 or gen3 driver
 *
 * @param ifname interface name (e.g. wlan0)
 * @param suspend_value suspend value as uint8_t (usually 1 to suspend, 0 to resume)
 */
static
void
suspend_set_wmtwifi(
    const char *ifname,
    uint8_t suspend_value)
{
    // first try the vendor specific TESTMODE command
    struct nl_msg *msg = NULL;
    int ifindex = 0;
    struct testmode_cmd_suspend susp_cmd;

    int success = 0;

    ifindex = if_nametoindex(ifname);

    if (ifindex == 0) {
        //printf("iface %s is not active/present (handle on_off).", ifname);
        return;
    }

    //printf("iface: %s suspend value: %d\n", ifname, (int)suspend_value);

    msg = nlmsg_alloc();

    genlmsg_put(msg, 0, 0, driver_id, 0, 0, NL80211_CMD_TESTMODE, 0);

    memset(&susp_cmd, 0, sizeof susp_cmd);
    susp_cmd.header.idx = TESTMODE_CMD_ID_SUSPEND;
    susp_cmd.header.buflen = 0; // unused
    susp_cmd.suspend = suspend_value;

    nla_put_u32(msg, NL80211_ATTR_IFINDEX, ifindex);
    nla_put(
        msg,
        NL80211_ATTR_TESTDATA,
        sizeof susp_cmd,
        (void*)&susp_cmd);

    if (nl_send_auto(nl_socket, msg) < 0) {
        //printf("Failed to send testmode command.\n");
    } else {
        if (suspend_plugin_netlink_handler() != 0) {
            // the driver returned an error or doesn't support this command
            // could be a driver which uses "SETSUSPENDMODE 1/0" priv cmds
            //printf("%s: TESTMODE command failed."
            //    "Ignore if the kernel is using a gen3 wmtWifi driver.\n",
            //    __func__);
        } else {
            success = 1;
        }
    }

    nlmsg_free(msg);

    // also send SETSUSPENDMODE private commands for gen3 drivers:
    int cmd_len = 0;
    struct ifreq ifr;
    android_wifi_priv_cmd priv_cmd;

    int ret;
    int ioctl_sock;

    ioctl_sock = socket(PF_INET, SOCK_DGRAM, 0);

    memset(&ifr, 0, sizeof(ifr));
    memset(&priv_cmd, 0, sizeof(priv_cmd));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

    cmd_len = snprintf(
        priv_cmd.buf,
        sizeof(priv_cmd.buf),
        "SETSUSPENDMODE %d",
        (int)suspend_value);

    priv_cmd.used_len = cmd_len + 1;
    priv_cmd.total_len = PRIV_CMD_SIZE;
    ifr.ifr_data = (void*)&priv_cmd;

    ret = ioctl(ioctl_sock, SIOCDEVPRIVATE + 1, &ifr);

    if (ret != 0) {
        //printf("%s: SETSUSPENDMODE private command failed: %d,"
        //    "ignore if the kernel is using a gen2 wmtWifi driver.",
        //    __func__,
        //    errno);
    } else {
        success = 1;
    }

    close(ioctl_sock);

    if (!success) {
        //printf("%s: could not enter suspend mode, both methods failed",
        //    __func__);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s [suspend|resume]\n", argv[0]);
        return 1;
    }

    // Initialize netlink
    nl_socket = nl_socket_alloc();
    if (!nl_socket) {
        //fprintf(stderr, "Failed to allocate netlink socket\n");
        return -1;
    }

    if (genl_connect(nl_socket)) {
        //fprintf(stderr, "Failed to connect to generic netlink\n");
        nl_socket_free(nl_socket);
        return -1;
    }

    driver_id = genl_ctrl_resolve(nl_socket, "nl80211");
    if (driver_id < 0) {
        //fprintf(stderr, "Could not resolve nl80211 driver id\n");
        nl_socket_free(nl_socket);
        return -1;
    }

    if (strcmp(argv[1], "suspend") == 0) {
        suspend_set_powersave("wlan0", true);
        suspend_set_wmtwifi("wlan0", WMTWIFI_SUSPEND_VALUE);
        //printf("Suspend and power save set for wlan0\n");
    } else if (strcmp(argv[1], "resume") == 0) {
        suspend_set_powersave("wlan0", false);
        suspend_set_wmtwifi("wlan0", WMTWIFI_RESUME_VALUE);
        //printf("Resume and power save unset for wlan0\n");
    } else {
        //fprintf(stderr, "Invalid argument. Use 'suspend' or 'resume'\n");
        nl_socket_free(nl_socket);
        return -1;
    }

    nl_socket_free(nl_socket);
    return 0;
}
