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
            'ActiveProfilesHolds': Variant('aa{sv}', [])
        }

    @dbus_property(access=PropertyAccess.READWRITE)
    def ActiveProfile(self) -> 's':
        return self.props['ActiveProfile'].value

    @ActiveProfile.setter
    def ActiveProfile(self, val: 's'):
        # this is mostly a placeholder until we come up with a proper "performance" and "power-saver" mode
        if val == "performance":
            subprocess.Popen("batman-hybris vr 1", shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
        elif val == "balanced":
            subprocess.Popen("batman-hybris vr 0", shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
        elif val == "power-saver":
            subprocess.Popen("batman-hybris vr 0", shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)

        self.props['ActiveProfile'] = Variant('s', val)

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
        return []

    @method()
    def HoldProfile(self, profile: 's', reason: 's', application_id: 's'):
        pass

    @method()
    def ReleaseProfile(self, cookie: 'u'):
        pass

    @method()
    def InhibitDevice(self, uid: 's', inhibit: 'b'):
        pass

async def main():
    bus = await MessageBus(bus_type=BusType.SYSTEM).connect()
    loop = asyncio.get_running_loop()
    ppd_interface = PPDInterface(loop, bus)
    bus.export('/net/hadess/PowerProfiles', ppd_interface)
    await bus.request_name('net.hadess.PowerProfiles')
    await bus.wait_for_disconnect()

asyncio.run(main())
