## Battery Management

This program should work on all devices. needs wlr-randr and yad to be installed.

batman-gui is based on yad so requires yad to be installed. if the apt repo is used then there is no need to install any dependencies manually as it will install all the dependencies itself.

Tested with Phosh, sxmo and Plasma Mobile. It should work with any wlroots or Kwin based environment.

It should be noted at this program has been tested on pmOS (a mainline OS), Droidian. (a Halium OS) and Manjaro Libhybris and AlpHybris.

upower should be in working condition and should return the correct battery and charging status. (which is the reason there is a special branch for Galaxy A5 because battery status reporting is broken, it has some missing drivers in kernel.)

It is currently available as a deb package, PKGBUILD and APKBUILD.

Not tested on Lomiri as it has integration for repowered.


# Dependencies

The following packages have to be installed as dependencies to build batman:

For Debian based systems:

`sudo apt install gcc make libupower-glib-dev pkg-config libwayland-dev libgtk-4-dev debhelper fakeroot`

For Fedora based systems:

`sudo dnf install upower-devel pkg-config wayland-devel gtk4-devel 'Development Tools'`

For Arch based systems:

`sudo pacman -S upower pkgconf wayland gcc make gtk4`

For Alpine based systems:

`sudo apk add upower-dev pkgconfig wayland-dev gcc make gtk4.0-dev`

Everything can be compiled using the Makefile:


### Building

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
sudo systemctl enable --now batman
```


### Manual compilation

To compile batman-helper manually you can use gcc:

```
gcc -DWITH_UPOWER -DWITH_WLRDISPLAY src/batman-helper.c src/wlrdisplay.c -o batman-helper -lwayland-client `pkg-config --cflags --libs upower-glib`
```

If you don't want upower status support and want the script to use the old upower implementation use:

```
gcc -DWITH_WLRDISPLAY src/batman-helper.c src/wlrdisplay.c -o batman-helper -lwayland-client `pkg-config --cflags --libs upower-glib`
```

If you don't want wlrdisplay status support and want batman to use the old wlr-randr implementation use:

```
gcc -DWITH_UPOWER src/batman-helper.c src/wlrdisplay.c -o batman-helper -lwayland-client `pkg-config --cflags --libs upower-glib`
```

older wlr-randr and upower implementations are slower so it is recommended to compile batman-helper with upower and wlrdisplay support


To compile batman-gui manually you can use gcc:

```
gcc src/batman-gui.c src/configcontrol.c src/getinfo.c -o batman-gui `pkg-config --cflags --libs gtk4`
```

To compile governor manually:

```
gcc src/governor.c -o governor
```

### Configuration

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


Make sure to restart the service on changes.
