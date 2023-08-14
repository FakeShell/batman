## Examples

`batman-functions.c` includes a set of functions which can be taken as reference for integrating apps with batman.

to build batman-functions:

```
gcc batman-functions.c `pkg-config --cflags --libs upower-glib` -lbatman-wrappers
```
