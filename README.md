## batman
This program works on all devices but requires wlr-randr to be installed.

batman-gui is based on yad so requires yad to be installed. if the deb package is used then there is no need to install anything manuall. it will install the repo that gets batman updates and it will install all the dependencies itself.

Tested with phosh and kde plasma.

It should be noted at this script has been tested on both pmOS (a mainline OS), Droidian. (a halium OS) and Manjaro ARM (another halium OS).

upower should be in working condition and should return the correct battery and charging status. (which is the reason there is a special branch for Galaxy A5 because battery status reporting is broken, missing drivers in kernel.)

It is currently available as a deb package, PKGBUILD and APKBUILD.

Not tested on Ubuntu Touch that has deep sleep and suspends things by itself.

