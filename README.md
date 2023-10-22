## Battery Management

Batman is a battery management service which tweaks different nodes and pieces of hardware in real time.

batman-gui is based on GTK4 and can tweak batman configuration file in real time.

Tested with Phosh, sxmo and Plasma Mobile. It should work with any wlroots or Kwin based environment.

It should be noted at this program has been tested on pmOS and Mobian (mainline) as well as Droidian, Manjaro Libhybris and AlpHybris (hybris).

upower must provide a proper battery status on `/org/freedesktop/UPower/devices/DisplayDevice`

It is currently available as a deb package, PKGBUILD and APKBUILD.

Not tested on Lomiri as it has integration for repowered.

# Dependencies

The following packages have to be installed as dependencies to build batman:

For Debian based systems:

```
sudo apt install gcc make libupower-glib-dev pkg-config libwayland-dev libgtk-4-dev debhelper fakeroot libadwaita-1-dev
```

For Fedora based systems:

```
sudo dnf install upower-devel pkg-config wayland-devel gtk4-devel libadwaita-devel 'Development Tools'
```

For Arch based systems:

```
sudo pacman -S upower pkgconf wayland gcc make gtk4 libadwaita
```

For Alpine based systems:

```
sudo apk add upower-dev pkgconfig wayland-dev gcc make gtk4.0-dev libadwaita-dev
```

Everything can be compiled using the Makefile:


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


# Manual compilation

To build batman-helper manually you can use gcc:

```
gcc -DWITH_UPOWER -DWITH_WLRDISPLAY -DWITH_GETINFO src/batman-helper.c src/batman-wrappers.c src/wlrdisplay.c src/getinfo.c -o batman-helper -lwayland-client `pkg-config --cflags --libs upower-glib`
```

If you don't want wlrdisplay status support and want batman to use the old wlr-randr implementation, remove `-DWITH_WLRDISPLAY` and to use the older upower implementation, remove `-DWITH_UPOWER`. To not build the software with systemd batman status, remove `-DWITH_GETINFO`.

older wlr-randr and upower implementations are slower so it is recommended to compile batman-helper with upower and wlrdisplay support

To build batman-gui manually you can use gcc:

```
gcc src/batman-gui.c src/configcontrol.c src/getinfo.c -o batman-gui `pkg-config --cflags --libs gtk4`
```

To build governor manually:

```
gcc src/governor.c -o governor
```

To build libbatman-wrappers.so:

```
gcc -DWITH_UPOWER -DWITH_WLRDISPLAY -DWITH_GETINFO -fPIC -shared src/batman-wrappers.c src/wlrdisplay.c src/getinfo.c -o libbatman-wrappers.so -lwayland-client `pkg-config --cflags --libs upower-glib`
```

Example usage of these functions are available at [examples/](./examples/README.md)


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
