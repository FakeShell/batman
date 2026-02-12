## Battery Manager

Batman is a battery management service which tweaks different nodes and pieces of hardware in real time.

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
Powersave allows CPU to go into powersaving mode.


`OFFLINE`
Offlining is the process of CPU cores shutting down.


`GPUSAVE`
GPU save allows GPU to go into powersaving.


`CHARGESAVE`
Chargesave indicates whether or not put device to powersave when it is connected to a power source.


`BUSSAVE`
This option allows devfreq bus nodes to be set to powersave.


`BTSAVE`
This option will allow batman to moderate bluetooth power management and switch states accordingly.


`HYBRIS`
This option will use binder IPC to set various pieces of hardware to powersave mode.


`WIFI`
This option will set the WiFi adapter to powersave mode.
