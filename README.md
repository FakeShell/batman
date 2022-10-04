## batman
This progra works on all devices but requires wlr-randr to be installed.
batman-gui is based on yad so requires yad to be installed. if the deb package is used then there is no need to install anything manuall. it will install the repo that gets batman updates and it will install all the dependencies itself.
Currently only tested with wlroots DEs or WMs. As an example Phosh or SXMO. but right now only phosh has been tested tho but i'm pretty sure plasma mobile won't work and i'm not a big fan of trying to make it work either it was not stable anyways. (atleast in my experience)
It should be noted at this script has been tested on both pmOS (a mainline OS) and Droidian. (a halium OS)
upower should be in working condition and should return the correct battery and charging status. (which is the reason there is a special branch for Galaxy A5 because battery status reporting is broken, missing drivers in kernel.)
It is currently only available as a deb package PRs are welcome for APKBUILD (Alpine Linux).
Not tested on Ubuntu Touch that has deep sleep and suspends things by itself.

