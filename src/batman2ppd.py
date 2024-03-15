#!/usr/bin/python3
# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2024 Bardia Moshiri <fakeshell@bardia.tech>

from dbus_next.aio import MessageBus
from dbus_next.service import (ServiceInterface,
                               method, dbus_property, signal)
from dbus_next.constants import PropertyAccess
from dbus_next import Variant, DBusError, BusType

import subprocess
import asyncio
import time
import os
import configparser

THERMAL_SYSFS_PATH = "/sys/class/thermal"

class PPDInterface(ServiceInterface):
    def __init__(self, loop, bus):
        super().__init__('net.hadess.PowerProfiles')
        self.loop = loop
        self.bus = bus
        self.props = {
            'ActiveProfile': Variant('s', 'balanced'),
            'PerformanceInhibited': Variant('s', ''),
            'PerformanceDegraded': Variant('s', ''),
            'Profiles': Variant('aa{sv}', [{'Profile': Variant('s', 'power-saver'), 'Driver': Variant('s', 'batman')}, {'Profile': Variant('s', 'balanced'), 'Driver': Variant('s', 'batman')}, {'Profile': Variant('s', 'performance'), 'Driver': Variant('s', 'batman')}]),
            'Actions': Variant('as', ['trickle_charge']),
            'ActiveProfileHolds': Variant('aa{sv}', [])
        }

    @dbus_property(access=PropertyAccess.READWRITE)
    async def ActiveProfile(self) -> 's':
        return self.props['ActiveProfile'].value

    @ActiveProfile.setter
    async def ActiveProfile(self, profile: 's'):
        if os.path.exists("/var/lib/batman/default_cpu_governor"):
            with open("/var/lib/batman/default_cpu_governor", "r") as default_governor_file:
                default_governor = default_governor_file.read()
        else:
            default_governor = ""

        if profile == "performance" and self.props['PerformanceDegraded'].value == "":
            subprocess.Popen("batman-hybris vr 1", shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)

            if default_governor:
                with open("/var/lib/batman/CUSTOM_DEFAULT_GOVERNOR", "w+") as f:
                    f.write("performance\n")

                    time.sleep(2)
                    subprocess.Popen("systemctl restart batman", shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
        elif profile == "balanced":
            subprocess.Popen("batman-hybris vr 0", shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)

            if default_governor:
                with open("/var/lib/batman/CUSTOM_DEFAULT_GOVERNOR", "w+") as f:
                    f.write(default_governor)

                    time.sleep(2)
                    subprocess.Popen("systemctl restart batman", shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
        elif profile == "power-saver":
            subprocess.Popen("batman-hybris vr 0", shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)

            if default_governor:
                with open("/var/lib/batman/CUSTOM_DEFAULT_GOVERNOR", "w+") as f:
                    f.write(default_governor)

                    time.sleep(2)
                    subprocess.Popen("systemctl restart batman", shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)

        self.props['ActiveProfile'] = Variant('s', profile)
        self.SetProfile(profile)


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
    def ActiveProfileHolds(self) -> 'aa{sv}':
        return self.props['ActiveProfileHolds'].value

    @method()
    def HoldProfile(self, profile: 's', reason: 's', application_id: 's'):
        pass

    @method()
    def ReleaseProfile(self, cookie: 'u'):
        pass

    @method()
    def InhibitDevice(self, uid: 's', inhibit: 'b'):
        pass

    def UpdatePerformanceDegraded(self, temp_avg):
        if temp_avg > 50:
            self.props['PerformanceDegraded'] = Variant('s', 'high-operating-temperature')
        else:
            self.props['PerformanceDegraded'] = Variant('s', '')

    def CreateStateFile(self):
        directory = "/var/lib/power-profiles-daemon/"
        file_path = os.path.join(directory, "state.ini")

        if not os.path.exists(directory):
            os.makedirs(directory)

        config = configparser.ConfigParser()

        if not os.path.isfile(file_path):
            config['State'] = {'Driver': 'batman', 'Profile': 'balanced'}

            with open(file_path, 'w') as stateFile:
                config.write(stateFile)
        else:
            config.read(file_path)
            if not config.has_option('State', 'Driver') or not config.has_option('State', 'Profile'):
                config['State'] = {'Driver': 'batman', 'Profile': 'balanced'}
                with open(file_path, 'w') as stateFile:
                    config.write(stateFile)

    def StatesExist(self, file_path):
        return os.path.isfile(file_path)

    def SetState(self, driver, profile):
        directory = "/var/lib/power-profiles-daemon/"
        file_path = os.path.join(directory, "state.ini")
        config = configparser.ConfigParser()
        if not self.StatesExist(file_path):
            config["State"] = {'Driver': driver, 'Profile': profile}
            with open(file_path, 'w') as stateFile:
                config.write(stateFile)
        else:
            config.read(file_path)
            config.set('State', 'Driver', driver)
            config.set('State', 'Profile', profile)
            with open(file_path, 'w') as stateFile:
                config.write(stateFile)

    def SetProfile(self, profile):
        directory = "/var/lib/power-profiles-daemon/"
        file_path = os.path.join(directory, "state.ini")
        config = configparser.ConfigParser()
        if not self.StatesExist(file_path):
            config["State"] = {'Profile': profile}
            with open(file_path, 'w') as stateFile:
                config.write(stateFile)
        else:
            config.read(file_path)
            config.set('State', 'Profile', profile)
            with open(file_path, 'w') as stateFile:
                config.write(stateFile)

    def GetProfile(self):
        directory = "/var/lib/power-profiles-daemon/"
        file_path = os.path.join(directory, "state.ini")
        config = configparser.ConfigParser()
        if self.StatesExist(file_path):
            try:
                profile = config.get('State', 'Profile')
            except configparser.NoSectionError:
                profile = None
            return profile

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

async def GetThermal():
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
                #print(f"{zone}: {temperature_celsius:.1f}C")
                temp_total += temperature_celsius
                i += 1

    if i > 0:
        temp_avg = temp_total / i

    #print(f"Average sys temp: {temp_avg:.1f}C")
    return temp_avg

async def main():
    bus = await MessageBus(bus_type=BusType.SYSTEM).connect()
    loop = asyncio.get_running_loop()
    ppd_interface = PPDInterface(loop, bus)
    bus.export('/net/hadess/PowerProfiles', ppd_interface)
    await bus.request_name('net.hadess.PowerProfiles')

    async def thermal_check():
        while True:
            temp_avg = await GetThermal()
            ppd_interface.UpdatePerformanceDegraded(temp_avg)
            await asyncio.sleep(5)

    ppd_interface.CreateStateFile()

    if ppd_interface.GetProfile():
        profile = ppd_interface.GetProfile()
        ppd_interface.ActiveProfile(profile)

    loop.create_task(thermal_check())

    await bus.wait_for_disconnect()

asyncio.run(main())
