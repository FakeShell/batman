CC = gcc

CFLAGS_BATMAN = `pkg-config --cflags upower-glib libgbinder glib-2.0 libnl-3.0 libnl-genl-3.0 libnl-route-3.0 libpulse libpulse-mainloop-glib` -Iinclude
LDFLAGS_BATMAN = `pkg-config --libs upower-glib libgbinder glib-2.0 libnl-3.0 libnl-genl-3.0 libnl-route-3.0 libpulse libpulse-mainloop-glib` -lwayland-client

CFLAGS_MONITOR = `pkg-config --cflags upower-glib` -Iinclude
LDFLAGS_MONITOR = -lwayland-client `pkg-config --libs upower-glib`

CFLAGS_NFCD = -fPIC -DNFC_PLUGIN_EXTERNAL `pkg-config --cflags nfcd-plugin libglibutil gobject-2.0 glib-2.0`
LDFLAGS_NFCD = -fPIC -shared `pkg-config --libs libglibutil gobject-2.0 glib-2.0`

CFLAGS_NICERDICER = `pkg-config --cflags gio-2.0` -Iinclude
LDFLAGS_NICERDICER = `pkg-config --libs gio-2.0`

SOURCES_BATMAN = src/main.c \
                 src/utils.c \
                 src/binder.c \
                 src/wifi.c \
                 src/wlrdisplay.c \
                 src/bluetooth.c \
                 src/config.c \
                 src/cpu.c \
                 src/gpu.c \
                 src/device_node.c \
                 src/logind.c \
                 src/mtk.c \
                 src/ppd.c \
                 src/thermal.c \
                 src/pulse.c

SOURCES_MONITOR = src/monitor/batman-system-monitor.c \
                  src/wlrdisplay.c \
                  src/utils.c

SOURCES_NFCD = src/nfcd/nfcd-batman-plugin.c
SOURCES_NICERDICER = src/nicerdicer/nicerdicer.c

TARGET_BATMAN = batman
TARGET_MONITOR = batman-system-monitor
TARGET_NICERDICER = batman-nicerdicer
TARGET_NFCD = batman.so
TARGET_PPDCLI = src/cli/powerprofilesctl.py

PREFIX ?= /usr
TRIPLET ?= $(shell $(CC) -dumpmachine)

all: $(TARGET_BATMAN) $(TARGET_MONITOR) $(TARGET_NFCD) $(TARGET_NICERDICER)

$(TARGET_BATMAN):
	$(CC) $(CFLAGS_BATMAN) $(SOURCES_BATMAN) -o $(TARGET_BATMAN) $(LDFLAGS_BATMAN)

$(TARGET_MONITOR):
	$(CC) $(CFLAGS_MONITOR) $(SOURCES_MONITOR) -o $(TARGET_MONITOR) $(LDFLAGS_MONITOR)

$(TARGET_NFCD):
	$(CC) $(CFLAGS_NFCD) $(SOURCES_NFCD) -o $(TARGET_NFCD) $(LDFLAGS_NFCD)

$(TARGET_NICERDICER):
	$(CC) $(CFLAGS_NICERDICER) $(SOURCES_NICERDICER) -o $(TARGET_NICERDICER) $(LDFLAGS_NICERDICER)

install: all
	install -d $(DESTDIR)$(PREFIX)/lib
	install -d $(DESTDIR)$(PREFIX)/bin
	install -d $(DESTDIR)$(PREFIX)/sbin
	install -d $(DESTDIR)/etc
	install -d $(DESTDIR)/var
	install -d $(DESTDIR)/var/lib/batman
	install -d $(DESTDIR)$(PREFIX)/share/polkit-1/actions
	install -d $(DESTDIR)$(PREFIX)/share/dbus-1/system.d
	install -d $(DESTDIR)$(PREFIX)/lib/$(TRIPLET)
	install -d $(DESTDIR)$(PREFIX)/lib/nfcd/plugins

	install -m 0755 $(TARGET_BATMAN) $(DESTDIR)$(PREFIX)/sbin/
	install -m 0755 $(TARGET_MONITOR) $(DESTDIR)$(PREFIX)/bin/
	install -m 0755 $(TARGET_NICERDICER) $(DESTDIR)$(PREFIX)/sbin/

	install -m 0755 $(TARGET_PPDCLI) $(DESTDIR)$(PREFIX)/bin/powerprofilesctl

	install -m 0644 $(TARGET_NFCD) $(DESTDIR)$(PREFIX)/lib/nfcd/plugins/

	install -m 0644 data/config $(DESTDIR)/var/lib/batman/config

	install -m 0644 data/org.freedesktop.UPower.PowerProfiles.policy $(DESTDIR)$(PREFIX)/share/polkit-1/actions/
	install -m 0644 data/org.freedesktop.UPower.PowerProfiles.conf $(DESTDIR)$(PREFIX)/share/dbus-1/system.d/
	install -m 0644 data/io.FuriOS.NicerDicer.conf $(DESTDIR)$(PREFIX)/share/dbus-1/system.d/

ifeq ($(strip $(DESTDIR)),)
	install -d $(DESTDIR)$(PREFIX)/lib/systemd/system
	install -m 0644 data/batman.service $(DESTDIR)$(PREFIX)/lib/systemd/system/
	install -m 0644 data/nicerdicer.service $(DESTDIR)$(PREFIX)/lib/systemd/system/
endif

	install -m 0644 data/nicerdicer.conf $(DESTDIR)/etc/nicerdicer.conf

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/sbin/$(TARGET_BATMAN)
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET_MONITOR)
	rm -f $(DESTDIR)$(PREFIX)/sbin/$(TARGET_NICERDICER)

	rm -f $(DESTDIR)$(PREFIX)/bin/powerprofilesctl

	rm -f $(DESTDIR)$(PREFIX)/lib/nfcd/plugins/$(TARGET_NFCD)

	rm -f $(DESTDIR)/var/lib/batman/config

	rm -f $(DESTDIR)$(PREFIX)/share/polkit-1/actions/org.freedesktop.UPower.PowerProfiles.policy
	rm -f $(DESTDIR)$(PREFIX)/share/dbus-1/system.d/org.freedesktop.UPower.PowerProfiles.conf
	rm -f $(DESTDIR)$(PREFIX)/share/dbus-1/system.d/io.FuriOS.NicerDicer.conf

	rm -f $(DESTDIR)$(PREFIX)/lib/systemd/system/batman.service
	rm -f $(DESTDIR)$(PREFIX)/lib/systemd/system/nicerdicer.service

	rm -f $(DESTDIR)/etc/nicerdicer.conf

clean:
	rm -f $(TARGET_BATMAN) $(TARGET_MONITOR) $(TARGET_NFCD) $(TARGET_NICERDICER)

.PHONY: all clean install uninstall
