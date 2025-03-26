#!/usr/bin/python3

import argparse
import os
import signal
import subprocess
import sys
from gi.repository import Gio, GLib

PP_NAME = "org.freedesktop.UPower.PowerProfiles"
PP_PATH = "/org/freedesktop/UPower/PowerProfiles"
PP_IFACE = "org.freedesktop.UPower.PowerProfiles"
PROPERTIES_IFACE = "org.freedesktop.DBus.Properties"


def get_proxy():
    bus = Gio.bus_get_sync(Gio.BusType.SYSTEM, None)
    return Gio.DBusProxy.new_sync(
        bus, Gio.DBusProxyFlags.NONE, None, PP_NAME, PP_PATH, PROPERTIES_IFACE, None
    )


def command(func):
    def wrapper(*args, **kwargs):
        try:
            func(*args, **kwargs)
        except GLib.Error as error:
            sys.stderr.write(
                f"Failed to communicate with power-profiles-daemon: {error}\n"
            )
            sys.exit(1)
        except ValueError as error:
            sys.stderr.write(f"Error: {error}\n")
            sys.exit(1)

    return wrapper


@command
def _version(_args):
    client_version = "0.30"
    try:
        proxy = get_proxy()
        daemon_ver = proxy.Get("(ss)", PP_IFACE, "Version")
    except GLib.Error:
        daemon_ver = "unknown"
    print(f"client: {client_version}\ndaemon: {daemon_ver}")


@command
def _set_profile(args):
    proxy = get_proxy()
    proxy.Set(
        "(ssv)", PP_IFACE, "ActiveProfile", GLib.Variant.new_string(args.profile[0])
    )


@command
def _get(_args):
    proxy = get_proxy()
    profile = proxy.Get("(ss)", PP_IFACE, "ActiveProfile")
    print(profile)


@command
def _set_battery_aware(args):
    enable = args.enable
    disable = args.disable
    if enable is False and disable is True:
        raise ValueError("enable or disable is required")
    if enable is True and disable is False:
        raise ValueError("can't set both enable and disable")
    enable = enable if enable is not None else not disable
    proxy = get_proxy()
    proxy.Set("(ssv)", PP_IFACE, "BatteryAware", GLib.Variant.new_boolean(enable))


def get_profiles_property(prop):
    proxy = get_proxy()
    return proxy.Get("(ss)", PP_IFACE, prop)


def get_profile_choices():
    try:
        return [profile["Profile"] for profile in get_profiles_property("Profiles")]
    except GLib.Error:
        return []


@command
def _list(_args):
    profiles = get_profiles_property("Profiles")
    reason = get_proxy().Get("(ss)", PP_IFACE, "PerformanceDegraded")
    degraded = reason != ""
    active = get_proxy().Get("(ss)", PP_IFACE, "ActiveProfile")

    index = 0
    for profile in reversed(profiles):
        if index > 0:
            print("")
        marker = "*" if profile["Profile"] == active else " "
        print(f'{marker} {profile["Profile"]}:')
        for driver in ["CpuDriver", "PlatformDriver"]:
            if driver not in profile:
                continue
            value = profile[driver]
            print(f"    {driver}:\t{value}")
        if profile["Profile"] == "performance":
            print("    Degraded:  ", f"yes ({reason})" if degraded else "no")
        index += 1


@command
def _list_holds(_args):
    holds = get_profiles_property("ActiveProfileHolds")

    index = 0
    for hold in holds:
        if index > 0:
            print("")
        print("Hold:")
        print("  Profile:        ", hold["Profile"])
        print("  Application ID: ", hold["ApplicationId"])
        print("  Reason:         ", hold["Reason"])
        index += 1


@command
def _launch(args):
    reason = args.reason
    profile = args.profile
    appid = args.appid
    if not args.arguments:
        raise ValueError("No command to launch")
    if not args.appid:
        appid = args.arguments[0]
    if not profile:
        profile = "performance"
    if not reason:
        reason = f"Running {args.appid}"
    ret = 0
    bus = Gio.bus_get_sync(Gio.BusType.SYSTEM, None)
    proxy = Gio.DBusProxy.new_sync(
        bus, Gio.DBusProxyFlags.NONE, None, PP_NAME, PP_PATH, PP_IFACE, None
    )
    cookie = proxy.HoldProfile("(sss)", profile, reason, appid)

    # print (f'Got {cookie} for {profile} hold')
    with subprocess.Popen(args.arguments) as launched_app:
        # Redirect the same signal to the child
        def receive_signal(signum, _stack):
            launched_app.send_signal(signum)

        redirected_signals = [
            signal.SIGTERM,
            signal.SIGINT,
            signal.SIGABRT,
        ]

        for sig in redirected_signals:
            signal.signal(sig, receive_signal)

        try:
            launched_app.wait()
            ret = launched_app.returncode
        except KeyboardInterrupt:
            ret = launched_app.returncode

        for sig in redirected_signals:
            signal.signal(sig, signal.SIG_DFL)

    proxy.ReleaseProfile("(u)", cookie)

    if ret < 0:
        # Use standard POSIX signal exit code.
        os.kill(os.getpid(), -ret)
        return

    sys.exit(ret)


@command
def _query_battery_aware(_args):
    result = get_profiles_property("BatteryAware")
    print(f"Dynamic changes from charger and battery events: {result}")


@command
def _list_actions(_args):
    actions = get_profiles_property("ActionsInfo")
    for action in actions:
        for key in action:
            print(f"{key}: {action[key]}")
        if action != actions[-1]:
            print("")


@command
def _configure_action(args):
    action = args.action[0]
    enable = args.enable
    disable = args.disable
    if enable is False and disable is True:
        raise argparse.ArgumentError(
            argument="action", message="enable or disable is required"
        )
    if enable is True and disable is False:
        raise argparse.ArgumentError(
            argument="action", message="can't set both enable and disable"
        )
    print(f"action: {action}, enable: {enable}")
    bus = Gio.bus_get_sync(Gio.BusType.SYSTEM, None)
    proxy = Gio.DBusProxy.new_sync(
        bus, Gio.DBusProxyFlags.NONE, None, PP_NAME, PP_PATH, PP_IFACE, None
    )
    proxy.SetActionEnabled("(sb)", action, enable)


def get_parser():
    parser = argparse.ArgumentParser(
        epilog="Use “powerprofilesctl COMMAND --help” to get detailed help for individual commands",
    )
    subparsers = parser.add_subparsers(help="Individual command help", dest="command")
    parser_list = subparsers.add_parser("list", help="List available power profiles")
    parser_list.set_defaults(func=_list)
    parser_list_holds = subparsers.add_parser(
        "list-holds", help="List current power profile holds"
    )
    parser_list_holds.set_defaults(func=_list_holds)
    parser_list_actions = subparsers.add_parser(
        "list-actions", help="List available power profile actions"
    )
    parser_list_actions.set_defaults(func=_list_actions)
    parser_get = subparsers.add_parser(
        "get", help="Print the currently active power profile"
    )
    parser_get.set_defaults(func=_get)
    parser_set = subparsers.add_parser(
        "set", help="Set the currently active power profile"
    )
    parser_set.add_argument(
        "profile",
        nargs=1,
        help="Profile to use for set command",
        choices=get_profile_choices(),
    )
    parser_set.set_defaults(func=_set_profile)
    parser_set_action = subparsers.add_parser(
        "configure-action", help="Configure the action to be taken for the profile"
    )
    parser_set_action.add_argument(
        "action",
        nargs=1,
        help="action to change for configure-action",
    )
    parser_set_action.add_argument(
        "--enable",
        action="store_true",
        help="enable action",
    )
    parser_set_action.add_argument(
        "--disable",
        action="store_false",
        help="disable action",
    )
    parser_set_action.set_defaults(func=_configure_action)
    parser_set_battery_aware = subparsers.add_parser(
        "configure-battery-aware",
        help="Turn on or off dynamic changes from battery level or power adapter",
    )
    parser_set_battery_aware.add_argument(
        "--enable",
        action="store_true",
        help="enable battery aware",
    )
    parser_set_battery_aware.add_argument(
        "--disable",
        action="store_false",
        help="disable battery aware",
    )
    parser_set_battery_aware.set_defaults(func=_set_battery_aware)
    parser_query_battery_aware = subparsers.add_parser(
        "query-battery-aware",
        help="Query if dynamic changes from battery level or power adapter are enabled",
    )
    parser_query_battery_aware.set_defaults(func=_query_battery_aware)
    parser_launch = subparsers.add_parser(
        "launch",
        help="Launch a command while holding a power profile",
        description="Launch the command while holding a power profile, "
        "either performance, or power-saver. By default, the profile hold "
        "is for the performance profile, but it might not be available on "
        "all systems. See the list command for a list of available profiles.",
    )
    parser_launch.add_argument(
        "arguments",
        nargs="*",
        help="Command to launch",
    )
    parser_launch.add_argument(
        "--profile", "-p", required=False, help="Profile to use for launch command"
    )
    parser_launch.add_argument(
        "--reason", "-r", required=False, help="Reason to use for launch command"
    )
    parser_launch.add_argument(
        "--appid", "-i", required=False, help="AppId to use for launch command"
    )
    parser_launch.set_defaults(func=_launch)
    parser_version = subparsers.add_parser(
        "version", help="Print version information and exit"
    )
    parser_version.set_defaults(func=_version)

    if not os.getenv("PPD_COMPLETIONS_GENERATION"):
        return parser

    try:
        import shtab  # pylint: disable=import-outside-toplevel

        shtab.add_argument_to(parser, ["--print-completion"])  # magic!
    except ImportError:
        pass

    return parser


def check_unknown_args(args, unknown_args, cmd):
    if cmd != "launch":
        return False

    for idx, unknown_arg in enumerate(unknown_args):
        arg = args[idx]
        if arg == cmd:
            return True
        if unknown_arg == arg:
            return False

    return True


def main():
    parser = get_parser()
    args, unknown = parser.parse_known_args()
    # default behavior is to run list if no command is given
    if not args.command:
        args.func = _list

    if check_unknown_args(sys.argv[1:], unknown, args.command):
        args.arguments += unknown
        unknown = []

    if unknown:
        msg = argparse._("unrecognized arguments: %s")
        parser.error(msg % " ".join(unknown))

    args.func(args)


if __name__ == "__main__":
    main()
