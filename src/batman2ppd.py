#!/usr/bin/python3
# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2025 Bardia Moshiri <fakeshell@bardia.tech>

import configparser
import subprocess
import asyncio
import gbinder
import time
import os
import re

from dbus_fast.aio import MessageBus
from dbus_fast.service import ServiceInterface, method, dbus_property, signal
from dbus_fast.constants import PropertyAccess
from dbus_fast import Variant, BusType

THERMAL_SYSFS_PATH = "/sys/class/thermal"
GPUFREQ_OPP_DUMP_PATH = "/proc/gpufreq/gpufreq_opp_dump"
GPUFREQ_OPP_FREQ_PATH = "/proc/gpufreq/gpufreq_opp_freq"

class PPDInterface(ServiceInterface):
    def __init__(self, loop, bus):
        super().__init__('net.hadess.PowerProfiles')
        self.loop = loop
        self.bus = bus
        self.cookie = 0
        self.cores = os.cpu_count()

        self.overdrive_enabled = False

        self.props = {
            'ActiveProfile': Variant('s', 'balanced'),
            'PerformanceInhibited': Variant('s', ''),
            'PerformanceDegraded': Variant('s', ''),
            'Profiles': Variant('aa{sv}', [
                {
                    'Profile': Variant('s', 'power-saver'),
                    'Driver': Variant('s', 'batman')
                },
                {
                    'Profile': Variant('s', 'balanced'),
                    'Driver': Variant('s', 'batman')
                },
                {
                    'Profile': Variant('s', 'performance'),
                    'Driver': Variant('s', 'batman')
                }
             ]),
            'Actions': Variant('as', ['trickle_charge']),
            'ActionsInfo': Variant('aa{sv}', [
                {
                    'Name': Variant('s', 'trickle_charge'),
                    'Description': Variant('s', 'Configure power supply to trickle charge'),
                    'Enabled': Variant('b', True)
                }
            ]),
            'ActiveProfileHolds': Variant('aa{sv}', []),
            'Version': Variant('s', '0.30'),
            'BatteryAware': Variant('b', True),
            'Overdrive': Variant('b', False),
        }

        self.loop.create_task(self.initialize_profile())

    async def thermal_check(self):
        while True:
            await self.update_performance_degraded()
            await asyncio.sleep(5)

    async def initialize_profile(self):
        self.create_state_file()
        profile = self.get_profile()
        if profile:
            await self.set_active_profile(profile)

        self.loop.create_task(self.thermal_check())

    @dbus_property(access=PropertyAccess.READWRITE)
    async def ActiveProfile(self) -> 's':
        return self.props['ActiveProfile'].value

    @ActiveProfile.setter
    async def ActiveProfile(self, profile: 's'):
        await self.set_active_profile(profile)

    async def set_overdrive(self, enabled: bool):
        enabled = bool(enabled)

        if enabled and not self.overdrive_enabled:
            self.overdrive_enabled = True
            self.props['Overdrive'] = Variant('b', True)

            await self.set_active_profile(
                'performance',
                write_state=False,
                force=True,
                treat_as_overdrive=True,
                update_active=False
            )
            return True

        if (not enabled) and self.overdrive_enabled:
            self.overdrive_enabled = False
            self.props['Overdrive'] = Variant('b', False)

            # Restore backend to user's saved profile
            restore_profile = self.get_profile() or 'balanced'
            await self.set_active_profile(
                restore_profile,
                write_state=False,
                force=True,
                treat_as_overdrive=False,
                update_active=True
            )
            return True
        return True

    async def set_active_profile(self, profile, write_state=True, force=False, treat_as_overdrive=False, update_active=True):
        if os.path.exists("/var/lib/batman/default_cpu_governor"):
            with open("/var/lib/batman/default_cpu_governor", "r") as default_governor_file:
                default_governor = default_governor_file.read()
        else:
            default_governor = ""

        if self.overdrive_enabled and not treat_as_overdrive:
            if write_state:
                self.set_profile(profile)

            if update_active:
                self.props['ActiveProfile'] = Variant('s', profile)

            return

        allow_performance = False
        if self.props['PerformanceDegraded'].value == "" or force:
            allow_performance = True

        if profile == "performance" and allow_performance:
            set_vr(True)
            set_mtkpower(45)

            if os.path.exists("/var/lib/batman/CUSTOM_FIRSTPOLCORE"):
                online_half(self.cores)
                os.remove("/var/lib/batman/CUSTOM_FIRSTPOLCORE")

            if default_governor:
                with open("/var/lib/batman/CUSTOM_DEFAULT_GOVERNOR", "w+") as f:
                    f.write("performance\n")

            # lock frequency to highest available value provided by the driver
            low, high = parse_gpufreq_opp_dump()
            write_gpufreq_opp_freq(high)

            await restart_service('batman')
        elif profile == "balanced":
            set_vr(False)
            set_mtkpower(21) # PROCESS_CREATE, much less intensive than UX_MOVE_SCROLLING (45) which pushes everything up to 100

            if os.path.exists("/var/lib/batman/CUSTOM_FIRSTPOLCORE"):
                online_half(self.cores)
                os.remove("/var/lib/batman/CUSTOM_FIRSTPOLCORE")

            if default_governor:
                with open("/var/lib/batman/CUSTOM_DEFAULT_GOVERNOR", "w+") as f:
                    f.write(default_governor)

            # return to dynamic frequency managed by dvfs
            write_gpufreq_opp_freq(0)

            await restart_service('batman')

        elif profile == "power-saver":
            set_vr(False)
            set_mtkpower(21) # PROCESS_CREATE, much less intensive than UX_MOVE_SCROLLING (45) which pushes everything up to 100

            if default_governor:
                with open("/var/lib/batman/CUSTOM_FIRSTPOLCORE", "w+") as f:
                    half_cores = self.cores // 2
                    f.write(f'{half_cores}')

            # lock frequency to lowest available value provided by the driver
            low, high = parse_gpufreq_opp_dump()
            write_gpufreq_opp_freq(low)

            await restart_service('batman')
            offline_half(self.cores)

        if update_active:
            self.props['ActiveProfile'] = Variant('s', profile)

        if write_state and (not treat_as_overdrive):
            self.set_profile(profile)

    @dbus_property(access=PropertyAccess.READ)
    def PerformanceInhibited(self) -> 's':
        return self.props['PerformanceInhibited'].value

    @dbus_property(access=PropertyAccess.READ)
    def PerformanceDegraded(self) -> 's':
        return self.props['PerformanceDegraded'].value

    @dbus_property(access=PropertyAccess.READ)
    def Profiles(self) -> 'aa{sv}':
        return self.props['Profiles'].value

    @dbus_property(access=PropertyAccess.READ)
    def Actions(self) -> 'as':
        return self.props['Actions'].value

    @dbus_property(access=PropertyAccess.READ)
    def ActionsInfo(self) -> 'aa{sv}':
        return self.props['ActionsInfo'].value

    @dbus_property(access=PropertyAccess.READ)
    def ActiveProfileHolds(self) -> 'aa{sv}':
        return self.props['ActiveProfileHolds'].value

    def hold_profile(self, profile, reason, application_id):
        self.cookie += 1
        hold = {
            'Profile': Variant('s', profile),
            'Reason': Variant('s', reason),
            'ApplicationId': Variant('s', application_id),
            'Cookie': Variant('u', self.cookie)
        }

        current_holds = self.props['ActiveProfileHolds'].value
        current_holds.append(hold)
        self.props['ActiveProfileHolds'] = Variant('aa{sv}', current_holds)

        return self.cookie

    @method()
    def HoldProfile(self, profile: 's', reason: 's', application_id: 's') -> 'u':
        return self.hold_profile(profile, reason, application_id)

    @method()
    def ReleaseProfile(self, cookie: 'u'):
        self.release_profile(cookie)
        self.ProfileReleased()

    @method()
    def SetActionEnabled(self, action: 's', enabled: 'b'):
        self.set_action_enabled(action, enabled)

    def set_action_enabled(action, enabled):
        pass

    @signal()
    def ProfileReleased(self) -> 'u':
        return self.cookie

    @dbus_property(access=PropertyAccess.READ)
    def Version(self) -> 's':
        return self.props['Version'].value

    @dbus_property(access=PropertyAccess.READWRITE)
    def BatteryAware(self) -> 'b':
        return self.props['BatteryAware'].value

    @BatteryAware.setter
    def BatteryAware(self, battery_aware: 'b'):
        self.set_battery_aware(battery_aware)

    def set_battery_aware(self, battery_aware):
        self.props['BatteryAware'] = Variant('b', battery_aware)

    async def update_performance_degraded(self):
        temp_avg = await get_thermal()
        if temp_avg > 50:
            self.props['PerformanceDegraded'] = Variant('s', 'high-operating-temperature')
        else:
            self.props['PerformanceDegraded'] = Variant('s', '')

    def create_state_file(self):
        directory = "/var/lib/power-profiles-daemon/"
        file_path = os.path.join(directory, "state.ini")

        if not os.path.exists(directory):
            os.makedirs(directory)

        config = configparser.ConfigParser()

        if not os.path.isfile(file_path):
            config['State'] = {'Driver': 'batman', 'Profile': 'balanced'}

            with open(file_path, 'w') as state_file:
                config.write(state_file)
        else:
            config.read(file_path)
            if not config.has_option('State', 'Driver') or not config.has_option('State', 'Profile'):
                config['State'] = {'Driver': 'batman', 'Profile': 'balanced'}
                with open(file_path, 'w') as state_file:
                    config.write(state_file)

    def set_profile(self, profile):
        directory = "/var/lib/power-profiles-daemon/"
        file_path = os.path.join(directory, "state.ini")
        config = configparser.ConfigParser()
        if not os.path.isfile(file_path):
            config["State"] = {'Profile': profile}
            with open(file_path, 'w') as state_file:
                config.write(state_file)
        else:
            config.read(file_path)
            config.set('State', 'Profile', profile)
            with open(file_path, 'w') as state_file:
                config.write(state_file)

    def get_profile(self):
        directory = "/var/lib/power-profiles-daemon/"
        file_path = os.path.join(directory, "state.ini")
        config = configparser.ConfigParser()
        if os.path.isfile(file_path):
            try:
                config.read(file_path)
                profile = config.get('State', 'Profile')
            except (configparser.NoSectionError, configparser.NoOptionError):
                profile = "balanced"
            return profile
        return "balanced"

class UPowerPPDInterface(ServiceInterface):
    def __init__(self, ppd_interface):
        super().__init__('org.freedesktop.UPower.PowerProfiles')
        self.ppd_interface = ppd_interface

    @dbus_property(access=PropertyAccess.READWRITE)
    async def ActiveProfile(self) -> 's':
        return self.ppd_interface.props['ActiveProfile'].value

    @ActiveProfile.setter
    async def ActiveProfile(self, profile: 's'):
        await self.ppd_interface.set_active_profile(profile)

    @dbus_property(access=PropertyAccess.READ)
    def PerformanceInhibited(self) -> 's':
        return self.ppd_interface.props['PerformanceInhibited'].value

    @dbus_property(access=PropertyAccess.READ)
    def PerformanceDegraded(self) -> 's':
        return self.ppd_interface.props['PerformanceDegraded'].value

    @dbus_property(access=PropertyAccess.READ)
    def Profiles(self) -> 'aa{sv}':
        return self.ppd_interface.props['Profiles'].value

    @dbus_property(access=PropertyAccess.READ)
    def Actions(self) -> 'as':
        return self.ppd_interface.props['Actions'].value

    @dbus_property(access=PropertyAccess.READ)
    def ActionsInfo(self) -> 'aa{sv}':
        return self.ppd_interface.props['ActionsInfo'].value

    @dbus_property(access=PropertyAccess.READ)
    def ActiveProfileHolds(self) -> 'aa{sv}':
        return self.ppd_interface.props['ActiveProfileHolds'].value

    @method()
    def HoldProfile(self, profile: 's', reason: 's', application_id: 's') -> 'u':
        return self.ppd_interface.hold_profile(profile, reason, application_id)

    @method()
    def ReleaseProfile(self, cookie: 'u'):
        self.ProfileReleased()

    @method()
    def SetActionEnabled(self, action: 's', enabled: 'b'):
        self.ppd_interface.set_action_enabled(action, enabled)

    @method()
    async def EnableOverdrive(self, enabled: 'b'):
        await self.ppd_interface.set_overdrive(enabled)

    @dbus_property(access=PropertyAccess.READ)
    def Overdrive(self) -> 'b':
        return self.ppd_interface.props['Overdrive'].value

    @signal()
    def ProfileReleased(self) -> 'u':
        return self.ppd_interface.cookie

    @dbus_property(access=PropertyAccess.READ)
    def Version(self) -> 's':
        return self.ppd_interface.props['Version'].value

    @dbus_property(access=PropertyAccess.READWRITE)
    def BatteryAware(self) -> 'b':
        return self.ppd_interface.props['BatteryAware'].value

    @BatteryAware.setter
    def BatteryAware(self, battery_aware: 'b'):
        self.ppd_interface.set_battery_aware(battery_aware)

async def restart_service(service_name: str) -> bool:
    try:
        if not service_name.endswith('.service'):
            service_name = f"{service_name}.service"

        bus = await MessageBus(bus_type=BusType.SYSTEM).connect()

        introspection = await bus.introspect(
            'org.freedesktop.systemd1',
            '/org/freedesktop/systemd1'
        )

        proxy_object = bus.get_proxy_object(
            'org.freedesktop.systemd1',
            '/org/freedesktop/systemd1',
            introspection
        )

        systemd = proxy_object.get_interface('org.freedesktop.systemd1.Manager')

        await systemd.call_restart_unit(service_name, 'replace')

        bus.disconnect()

        return True
    except Exception as e:
        print(f"Error restarting {service_name}: {e}")
        return False

def parse_gpufreq_opp_dump(path=GPUFREQ_OPP_DUMP_PATH):
    if not os.path.exists(path):
        return 0, 0

    try:
        freqs = []
        freq_pattern = re.compile(r'freq\s*=\s*(\d+)')

        with open(path, "r") as f:
            for line in f:
                m = freq_pattern.search(line)
                if m:
                    freqs.append(int(m.group(1)))

        if not freqs:
            return 0, 0
        return min(freqs), max(freqs)
    except Exception:
        return 0, 0

def write_gpufreq_opp_freq(freq, path=GPUFREQ_OPP_FREQ_PATH):
    if not os.path.exists(path):
        return False

    try:
        freq_str = str(int(freq)).strip()
        with open(path, "w") as f:
            f.write(freq_str)
        return True
    except Exception:
        return False

def offline_half(cpu_count):
    half_cpus = cpu_count // 2

    for i in range(half_cpus):
        cpu_path = f"/sys/devices/system/cpu/cpu{i}/online"
        try:
            with open(cpu_path, 'w') as f:
                f.write('0')
        except Exception as e:
            print(f"Error disabling CPU {i}: {e}")

def online_half(cpu_count):
    half_cpus = cpu_count // 2

    for i in range(half_cpus):
        cpu_path = f"/sys/devices/system/cpu/cpu{i}/online"
        try:
            with open(cpu_path, 'w') as f:
                f.write('1')
        except Exception as e:
            print(f"Error enabling CPU {i}: {e}")

def set_vr(enabled):
    available = find_hidl("android.hardware.vr@1.0::IVr/default")
    if available:
        vr_state = "1" if enabled else "0"
        result = subprocess.run(f"batman-hybris vr {vr_state}", shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        return result.returncode == 0
    return False

def set_mtkpower(state):
    available = find_hidl("vendor.mediatek.hardware.mtkpower@1.0::IMtkPower/default")
    if available:
        result = subprocess.run(f"batman-hybris mtkpower {state}", shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        return result.returncode == 0
    return False

def find_hidl(intf):
    try:
        sm = gbinder.ServiceManager("/dev/hwbinder")
        return intf in sm.list_sync()
    except:
        return False

### GTherm equivalent implementation ###

def get_thermal_zones():
    zones = []
    for item in os.listdir(THERMAL_SYSFS_PATH):
        if item.startswith("thermal_zone"):
            zones.append(item)
    return zones

def read_sysfs_file(path):
    try:
        with open(path, 'r') as file:
            return file.read().strip()
    except IOError as e:
        return None

def get_zone_temp(zone):
    temp_path = os.path.join(THERMAL_SYSFS_PATH, zone, "temp")
    return read_sysfs_file(temp_path)

def get_zone_type(zone):
    type_path = os.path.join(THERMAL_SYSFS_PATH, zone, "type")
    return read_sysfs_file(type_path)

async def connect_to_dbus():
    return await MessageBus(bus_type=BusType.SYSTEM).connect()

async def get_thermal():
    i = 0
    temp_total = 0
    temp_avg = 0

    zones = get_thermal_zones()
    for zone in zones:
        raw_temperature = get_zone_temp(zone)
        if raw_temperature is not None:
            temperature_celsius = int(raw_temperature) / 1000.0
            # most devices (exynos chipsets being one of them) show a bunch of useless nodes under 10c
            # the temperature reading here is incorrect. instead of going through each kernel one by one
            # we know that anything below 5-10C is basically impossible under normal usage and environment.
            # this is not a perfect solution but should be good enough for now.
            if temperature_celsius > 10:
                temp_total += temperature_celsius
                i += 1

    if i > 0:
        temp_avg = temp_total / i

    return temp_avg

async def main():
    hadess_bus = await connect_to_dbus()
    upower_bus = await connect_to_dbus()

    loop = asyncio.get_running_loop()

    ppd_interface = PPDInterface(loop, hadess_bus)
    hadess_bus.export('/net/hadess/PowerProfiles', ppd_interface)
    await hadess_bus.request_name('net.hadess.PowerProfiles')

    upower_interface = UPowerPPDInterface(ppd_interface)
    upower_bus.export('/org/freedesktop/UPower/PowerProfiles', upower_interface)
    await upower_bus.request_name('org.freedesktop.UPower.PowerProfiles')

    await asyncio.gather(
        hadess_bus.wait_for_disconnect(),
        upower_bus.wait_for_disconnect()
    )

if __name__ == "__main__":
    asyncio.run(main())
