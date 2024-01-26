#!/usr/bin/python3

import signal
import subprocess
import sys
from gi.repository import Gio, GLib

VERSION = '0.13'

def usage_main():
    print('Usage:')
    print('  powerprofilesctl COMMAND [ARGS…]')
    print('')
    print('Commands:')
    print('  help       Print help')
    print('  version    Print version')
    print('  get        Print the currently active power profile')
    print('  set        Set the currently active power profile')
    print('  list       List available power profiles')
    print('  list-holds List current power profile holds')
    print('  launch     Launch a command while holding a power profile')
    print('')
    print('Use “powerprofilesctl help COMMAND” to get detailed help.')

def usage_version():
    print('Usage:')
    print('  powerprofilesctl version')
    print('')
    print('Print version information and exit.')

def usage_get():
    print('Usage:')
    print('  powerprofilesctl get')
    print('')
    print('Print the currently active power profile.')

def usage_set():
    print('Usage:')
    print('  powerprofilesctl set PROFILE')
    print('')
    print('Set the currently active power profile. Must be one of the ')
    print('available profiles.')

def usage_list():
    print('Usage:')
    print('  powerprofilesctl list')
    print('')
    print('List available power profiles.')

def usage_list_holds():
    print('Usage:')
    print('  powerprofilesctl list-holds')
    print('')
    print('List current power profile holds.')

def usage_launch():
    print('Usage:')
    print('  powerprofilesctl launch [COMMAND…]')
    print('')
    print('Launch a command while holding a power profile.')
    print('')
    print('Options:')
    print('  -p, --profile=PROFILE           The power profile to hold')
    print('  -r, --reason=REASON             The reason for the profile hold')
    print('  -i, --appid=APP-ID              The application ID for the profile hold')
    print('')
    print('Launch the command while holding a power profile, either performance, ')
    print('or power-saver. By default, the profile hold is for the performance ')
    print('profile, but it might not be available on all systems. See the list ')
    print('command for a list of available profiles.')

def usage(_command=None):
    if not _command:
        usage_main()
    elif _command == 'get':
        usage_get()
    elif _command == 'set':
        usage_set()
    elif _command == 'list':
        usage_list()
    elif _command == 'list-holds':
        usage_list_holds()
    elif _command == 'launch':
        usage_launch()
    elif _command == 'version':
        usage_version()
    else:
        usage_main()

def version():
    print (VERSION)

def get_proxy():
    try:
        bus = Gio.bus_get_sync(Gio.BusType.SYSTEM, None)
        proxy = Gio.DBusProxy.new_sync(bus, Gio.DBusProxyFlags.NONE, None,
                                       'net.hadess.PowerProfiles',
                                       '/net/hadess/PowerProfiles',
                                       'org.freedesktop.DBus.Properties', None)
    except:
        raise
    return proxy

def _get():
    proxy = get_proxy()
    profile = proxy.Get('(ss)', 'net.hadess.PowerProfiles', 'ActiveProfile')
    print(profile)

def _set(profile):
    try:
        proxy = get_proxy()
        proxy.Set('(ssv)',
            'net.hadess.PowerProfiles',
            'ActiveProfile',
            GLib.Variant.new_string(profile))
    except:
        raise

def get_profiles_property(prop):
    try:
        proxy = get_proxy()
    except:
        raise

    profiles = None
    try:
        profiles = proxy.Get('(ss)', 'net.hadess.PowerProfiles', prop)
    except:
        raise
    return profiles

def _list():
    try:
        profiles = get_profiles_property('Profiles')
        reason = get_proxy().Get('(ss)', 'net.hadess.PowerProfiles', 'PerformanceDegraded')
        degraded = reason != ''
        active = get_proxy().Get('(ss)', 'net.hadess.PowerProfiles', 'ActiveProfile')
    except:
        raise

    index = 0
    for profile in reversed(profiles):
        if index > 0:
            print('')
        marker = '*' if profile['Profile'] == active else ' '
        print(f'{marker} {profile["Profile"]}:')
        print('    Driver:    ', profile['Driver'])
        if profile['Profile'] == 'performance':
            print('    Degraded:  ', f'yes ({reason})' if degraded else 'no')
        index += 1

def _list_holds():
    try:
        holds = get_profiles_property('ActiveProfileHolds')
    except:
        raise

    index = 0
    for hold in holds:
        if index > 0:
            print('')
        print('Hold:')
        print('  Profile:        ', hold['Profile'])
        print('  Application ID: ', hold['ApplicationId'])
        print('  Reason:         ', hold['Reason'])
        index += 1

def _launch(args, profile, appid, reason):
    try:
        bus = Gio.bus_get_sync(Gio.BusType.SYSTEM, None)
        proxy = Gio.DBusProxy.new_sync(bus, Gio.DBusProxyFlags.NONE, None,
                                       'net.hadess.PowerProfiles',
                                       '/net/hadess/PowerProfiles',
                                       'net.hadess.PowerProfiles', None)
    except:
        raise

    cookie = proxy.HoldProfile('(sss)', profile, reason, appid)

    # Kill child when we go away
    def receive_signal(_signum, _stack):
        launched_app.terminate()
    signal.signal(signal.SIGTERM, receive_signal)

    # print (f'Got {cookie} for {profile} hold')
    with subprocess.Popen(args) as launched_app:
        launched_app.wait()
    proxy.ReleaseProfile('(u)', cookie)

def main(): # pylint: disable=too-many-branches, disable=too-many-statements
    args = None
    if len(sys.argv) == 1:
        command = 'list'
    elif len(sys.argv) >= 2:
        command = sys.argv[1]
        if command == '--help':
            command = 'help'
        if command == '--version':
            command = 'version'
        else:
            args = sys.argv[2:]

    if command == 'help':
        if len(args) > 0:
            usage(args[0])
        else:
            usage(None)
    elif command == 'version':
        version()
    elif command == 'get':
        try:
            _get()
        except GLib.Error as error:
            sys.stderr.write(f'Failed to communicate with power-profiles-daemon: {format(error)}\n')
            sys.exit(1)
    elif command == 'set':
        if len(args) != 1:
            usage_set()
            sys.exit(1)
        try:
            _set(args[0])
        except GLib.Error as error:
            sys.stderr.write(f'Failed to communicate with power-profiles-daemon: {format(error)}\n')
            sys.exit(1)
    elif command == 'list':
        try:
            _list()
        except GLib.Error as error:
            sys.stderr.write(f'Failed to communicate with power-profiles-daemon: {format(error)}\n')
            sys.exit(1)
    elif command == 'list-holds':
        try:
            _list_holds()
        except GLib.Error as error:
            sys.stderr.write(f'Failed to communicate with power-profiles-daemon: {format(error)}\n')
            sys.exit(1)
    elif command == 'launch':
        if len(args) == 0:
            sys.exit(0)
        profile = None
        reason = None
        appid = None
        while True:
            if args[0] == '--':
                args = args[1:]
                break
            if args[0][:9] == '--profile' or args[0] == '-p':
                if args[0][:10] == '--profile=':
                    args = args[0].split('=') + args[1:]
                profile = args[1]
                args = args[2:]
                continue
            if args[0][:8] == '--reason' or args[0] == '-r':
                if args[0][:9] == '--reason=':
                    args = args[0].split('=') + args[1:]
                reason = args[1]
                args = args[2:]
                continue
            if args[0][:7] == '--appid' or args[0] == '-i':
                if args[0][:8] == '--appid=':
                    args = args[0].split('=') + args[1:]
                appid = args[1]
                args = args[2:]
                continue
            break

        if len(args) < 1:
            sys.exit(0)
        if not appid:
            appid = args[0]
        if not reason:
            reason = 'Running ' + appid
        if not profile:
            profile = 'performance'
        try:
            _launch(args, profile, appid, reason)
        except GLib.Error as error:
            sys.stderr.write(f'Failed to communicate with power-profiles-daemon: {format(error)}\n')
            sys.exit(1)

if __name__ == '__main__':
    main()
