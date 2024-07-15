CC = gcc
CFLAGS = `pkg-config --cflags upower-glib gtk4 libadwaita-1 gio-2.0`
LDFLAGS = -lwayland-client `pkg-config --libs upower-glib gtk4 libadwaita-1 gio-2.0`
CFLAGS_NFCD = -fPIC -DNFC_PLUGIN_EXTERNAL `pkg-config --cflags nfcd-plugin libglibutil gobject-2.0 glib-2.0`
LDFLAGS_NFCD = -fPIC -shared `pkg-config --libs libglibutil gobject-2.0 glib-2.0` -lwayland-client
LDFLAGS_GBINDER = `pkg-config --libs --cflags libgbinder`
LDFLAGS_HYBRIS = `pkg-config --libs --cflags libgbinder`
CFLAGS_WIFI = `pkg-config --cflags glib-2.0 libnl-3.0 libnl-genl-3.0 libnl-route-3.0`
LDFLAGS_WIFI = `pkg-config --libs glib-2.0 libnl-3.0 libnl-genl-3.0 libnl-route-3.0`
CFLAGS_WAYDROID = `pkg-config --cflags gio-2.0`
LDFLAGS_WAYDROID = `pkg-config --libs gio-2.0`
CFLAGS_NICERDICER = `pkg-config --cflags gio-2.0`
LDFLAGS_NICERDICER = `pkg-config --libs gio-2.0`
CFLAGS_EXAMPLES = `pkg-config --cflags upower-glib`
LDFLAGS_EXAMPLES = `pkg-config --libs upower-glib` -lbatman-wrappers -lbatman-waydroid

TARGET = batman
TARGET_HELPER = batman-helper
TARGET_GUI = batman-gui
TARGET_GOVERNOR = governor
TARGET_WRAPPERS = libbatman-wrappers.so
TARGET_GBINDER = libbatman-gbinder.so
TARGET_HYBRIS = batman-hybris
TARGET_NFCD = batman.so
TARGET_WIFI = batman-wifi
TARGET_BATMAN2PPD = src/batman2ppd.py
TARGET_PPDCLI = src/powerprofilesctl.py
TARGET_WAYDROID = libbatman-waydroid.so
TARGET_WAYDROID_CLI = batman-waydroid
TARGET_NICERDICER = batman-nicerdicer
TARGET_EXAMPLES = batman-examples

SRC_HELPER = src/batman-helper.c src/wlrdisplay.c src/batman-wrappers.c src/getinfo.c src/batman-waydroid.c
SRC_GUI = src/batman-gui.c src/configcontrol.c src/getinfo.c
SRC_GOVERNOR = src/governor.c src/wlrdisplay.c src/batman-wrappers.c
SRC_WRAPPERS = src/batman-wrappers.c src/wlrdisplay.c src/getinfo.c
SRC_GBINDER = src/batman-gbinder.c
SRC_HYBRIS = src/batman-hybris.c src/batman-gbinder.c
SRC_NFCD = src/nfcd-batman-plugin.c src/wlrdisplay.c
SRC_WIFI = src/batman-wifi.c
SRC_WAYDROID = src/batman-waydroid.c
SRC_WAYDROID_CLI = src/batman-waydroid-cli.c src/batman-waydroid.c
SRC_NICERDICER = src/nicerdicer.c
SRC_EXAMPLES = examples/batman-functions.c
HEADERS = src/batman-wrappers.h src/getinfo.h src/governor.h src/batman-gbinder.h src/batman-waydroid.h

PREFIX ?= /usr
LIBDIR ?= $(PREFIX)/lib
BINDIR ?= $(PREFIX)/bin
SBINDIR ?= $(PREFIX)/sbin
CONFIGDIR ?= /var/lib/batman
SYSTEMD_DIR ?= $(LIBDIR)/systemd/system
OPENRC_DIR ?= /etc/init.d
DESKTOP_DIR ?= $(PREFIX)/share/applications
ICON_DIR ?= $(PREFIX)/share/icons
INCLUDE_DIR ?= $(PREFIX)/include/batman
POLKIT_DIR ?= $(PREFIX)/share/polkit-1/actions
DBUS_DIR ?= $(PREFIX)/share/dbus-1/system.d

.PHONY: all
all: helper wrappers governor gui gbinder hybris wifi nfcd waydroid nicerdicer

helper: $(TARGET_HELPER)
$(TARGET_HELPER):
	$(CC) $(CFLAGS) $(SRC_HELPER) $(LDFLAGS) -o $(TARGET_HELPER)

wrappers: $(TARGET_WRAPPERS)
$(TARGET_WRAPPERS):
	$(CC) -fPIC -shared $(CFLAGS) $(SRC_WRAPPERS) $(LDFLAGS) -o $(TARGET_WRAPPERS)

governor: $(TARGET_GOVERNOR)
$(TARGET_GOVERNOR):
	$(CC) $(CFLAGS) $(SRC_GOVERNOR) $(LDFLAGS) -o $(TARGET_GOVERNOR)

gui: $(TARGET_GUI)
$(TARGET_GUI):
	$(CC) $(CFLAGS) $(SRC_GUI) $(LDFLAGS) -o $(TARGET_GUI)

gbinder: $(TARGET_GBINDER)
$(TARGET_GBINDER):
	$(CC) -fPIC -shared $(SRC_GBINDER) -o $(TARGET_GBINDER) $(LDFLAGS_GBINDER)

hybris: $(TARGET_HYBRIS)
$(TARGET_HYBRIS):
	$(CC) $(SRC_HYBRIS) -o $(TARGET_HYBRIS) $(LDFLAGS_HYBRIS)

wifi: $(TARGET_WIFI)
$(TARGET_WIFI):
	$(CC) $(SRC_WIFI) -o $(TARGET_WIFI) $(CFLAGS_WIFI) $(LDFLAGS_WIFI)

nfcd: $(TARGET_NFCD)
$(TARGET_NFCD): nfcd-batman-plugin.o wlrdisplay.o
	$(CC) $^ $(LDFLAGS_NFCD) -o $@

waydroid: $(TARGET_WAYDROID)
$(TARGET_WAYDROID):
	$(CC) -fPIC -shared $(SRC_WAYDROID) -o $(TARGET_WAYDROID) $(CFLAGS_WAYDROID) $(LDFLAGS_WAYDROID)
	$(CC) $(SRC_WAYDROID_CLI) -o $(TARGET_WAYDROID_CLI) $(CFLAGS_WAYDROID) $(LDFLAGS_WAYDROID)

nicerdicer: $(TARGET_NICERDICER)
$(TARGET_NICERDICER):
	$(CC) $(SRC_NICERDICER) -o $(TARGET_NICERDICER) $(CFLAGS_NICERDICER) $(LDFLAGS_NICERDICER)

examples: $(TARGET_EXAMPLES)
$(TARGET_EXAMPLES):
	$(CC) $(SRC_EXAMPLES) -o $(TARGET_EXAMPLES) $(CFLAGS_EXAMPLES) $(LDFLAGS_EXAMPLES)

nfcd-batman-plugin.o: src/nfcd-batman-plugin.c
	$(CC) -c $< $(CFLAGS_NFCD) -O2 -o $@

wlrdisplay.o: src/wlrdisplay.c
	$(CC) -c $< $(CFLAGS_NFCD) -O2 -o $@

.PHONY: install
install:
	cp src/$(TARGET) $(BINDIR)
	cp $(TARGET_HELPER) $(BINDIR)
	cp $(TARGET_GUI) $(BINDIR)
	cp $(TARGET_GOVERNOR) $(BINDIR)
	cp $(TARGET_WRAPPERS) $(LIBDIR)
	cp $(TARGET_GBINDER) $(LIBDIR)
	cp $(TARGET_HYBRIS) $(BINDIR)
	cp $(TARGET_WIFI) $(BINDIR)
	cp $(TARGET_BATMAN2PPD) $(BINDIR)/batman2ppd
	cp $(TARGET_PPDCLI) $(BINDIR)/powerprofilesctl
	cp $(TARGET_WAYDROID) $(LIBDIR)
	cp $(TARGET_WAYDROID_CLI) $(BINDIR)
	cp $(TARGET_NICERDICER) $(SBINDIR)

	mkdir -p $(DESKTOP_DIR)
	cp data/batman-gui.desktop $(DESKTOP_DIR)

	mkdir -p $(ICON_DIR)
	cp data/batman.png $(ICON_DIR)

	mkdir -p $(CONFIGDIR)
	cp data/config $(CONFIGDIR)

	mkdir -p $(INCLUDE_DIR)
	cp $(HEADERS) $(INCLUDE_DIR)

	mkdir -p $(POLKIT_DIR)
	cp data/net.hadess.PowerProfiles.policy $(POLKIT_DIR)

	mkdir -p $(DBUS_DIR)
	cp data/net.hadess.PowerProfiles.conf $(DBUS_DIR)
	cp data/io.FuriOS.NicerDicer.conf $(DBUS_DIR)

ifeq ($(shell test -d $(SYSTEMD_DIR) && echo 1),1)
	cp data/batman.service $(SYSTEMD_DIR)
	cp data/batman2ppd.service $(SYSTEMD_DIR)
	cp data/nicerdicer.service $(SYSTEMD_DIR)
else ifeq ($(shell test -e /sbin/openrc && echo 1),1)
	cp data/batman.rc $(OPENRC_DIR)/batman
else
	cp data/batman-init $(OPENRC_DIR)/batman
endif

	cp data/nicerdicer.conf /etc/nicerdicer.conf

.PHONY: clean
clean:
	rm -f $(TARGET_HELPER)
	rm -f $(TARGET_GUI)
	rm -f $(TARGET_GOVERNOR)
	rm -f $(TARGET_WRAPPERS)
	rm -f $(TARGET_GBINDER)
	rm -f $(TARGET_HYBRIS)
	rm -f $(TARGET_NFCD)
	rm -f $(TARGET_WIFI)
	rm -f $(TARGET_WAYDROID)
	rm -f $(TARGET_WAYDROID_CLI)
	rm -f $(TARGET_NICERDICER)
	rm -f $(TARGET_EXAMPLES)
	rm -f nfcd-batman-plugin.o wlrdisplay.o
