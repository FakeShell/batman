## Battery Management

Batman is a battery management service which tweaks different nodes and pieces of hardware in real time.

batman-gui is based on GTK4 and can tweak batman configuration file in real time.

Tested with Phosh, sxmo and Plasma Mobile. It should work with any wlroots or Kwin based environment.

It should be noted at this program has been tested on pmOS, Mobian, Droidian, Manjaro Libhybris, AlpHybris and FuriOS.

upower must provide a proper battery status on `/org/freedesktop/UPower/devices/DisplayDevice`

It is currently available as a deb package (PKGBUILD and APKBUILD are outdated)

# Building

```
make
```

And to install everything onto the system

```
sudo make install
```

And on Debian based systems to build and package batman

```
dpkg-buildpackage
```

And finally, to enable the service

```
sudo systemctl daemon-reload
sudo systemctl enable --now batman
```

# Configuration

`POWERSAVE`
Powersave allows CPU go into powersaving.


`OFFLINE`
Offlining is the process of CPU cores shutting down. These two only activate when device is idling. They also save a lot of battery.


`GPUSAVE`
GPU save allows GPU go into powersaving. This feature is still experimental.


`CHARGESAVE`
Chargesave indicates whether or not put device to powersave when its charging up.


`BUSSAVE`
This option allows devfreq bus nodes to be set to powersave. They also save a lot of battery.


`MAX_CPU_USAGE`
This option tells batman at what CPU usage it should leave powersaving. as an example when listening to music in the background this item can be set to a lower value to make the device not set it to powersave to not make audio choppy


`BTSAVE`
This option will allow batman to moderate bluetooth power management and switch states accordingly. it will check if it is connected and if something is using it and changes states accordingly
