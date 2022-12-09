## batman
This program should work on all devices. needs wlr-randr and yad to be installed.

batman-gui is based on yad so requires yad to be installed. if the deb package is used then there is no need to install anything manuall. it will install the repo that gets batman updates and it will install all the dependencies itself.

Tested with phosh, sxmo and plasma mobile.

It should be noted at this program has been tested on pmOS (a mainline OS), Droidian. (a Halium OS) and Manjaro ARM (another Halium OS).

upower should be in working condition and should return the correct battery and charging status. (which is the reason there is a special branch for Galaxy A5 because battery status reporting is broken, it has some missing drivers in kernel.)

It is currently available as a deb package, PKGBUILD and APKBUILD.

Not tested on Ubuntu Touch that has deep sleep and suspends things by itself.

