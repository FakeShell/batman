CC = gcc
CFLAGS_HELPER = `pkg-config --cflags upower-glib gtk4 libadwaita-1 gio-2.0`
LDFLAGS_HELPER = -lwayland-client `pkg-config --libs upower-glib gtk4 libadwaita-1 gio-2.0`
CFLAGS_WRAPPERS = `pkg-config --cflags upower-glib`
LDFLAGS_WRAPPERS = `pkg-config --libs upower-glib` -lwayland-client
CFLAGS_GOVERNOR = `pkg-config --cflags upower-glib`
LDFLAGS_GOVERNOR = -lwayland-client `pkg-config --libs upower-glib`
CFLAGS_GUI = `pkg-config --cflags gtk4 libadwaita-1`
LDFLAGS_GUI = `pkg-config --libs gtk4 libadwaita-1`
CFLAGS_NFCD = -fPIC -DNFC_PLUGIN_EXTERNAL `pkg-config --cflags nfcd-plugin libglibutil gobject-2.0 glib-2.0`
LDFLAGS_NFCD = -fPIC -shared `pkg-config --libs libglibutil gobject-2.0 glib-2.0` -lwayland-client
CFLAGS_GBINDER = `pkg-config --cflags libgbinder`
LDFLAGS_GBINDER = `pkg-config --libs libgbinder`
CFLAGS_HYBRIS = `pkg-config --cflags libgbinder`
LDFLAGS_HYBRIS = `pkg-config --libs libgbinder`
CFLAGS_WIFI = `pkg-config --cflags glib-2.0 libnl-3.0 libnl-genl-3.0 libnl-route-3.0`
LDFLAGS_WIFI = `pkg-config --libs glib-2.0 libnl-3.0 libnl-genl-3.0 libnl-route-3.0`
CFLAGS_ANDROMEDA = `pkg-config --cflags gio-2.0`
LDFLAGS_ANDROMEDA = `pkg-config --libs gio-2.0`
CFLAGS_NICERDICER = `pkg-config --cflags gio-2.0`
LDFLAGS_NICERDICER = `pkg-config --libs gio-2.0`
CFLAGS_POWERCONFIG = `pkg-config --cflags gio-2.0`
LDFLAGS_POWERCONFIG = `pkg-config --libs gio-2.0`
CFLAGS_EXAMPLES = `pkg-config --cflags upower-glib`
LDFLAGS_EXAMPLES = `pkg-config --libs upower-glib` -lbatman-wrappers -lbatman-andromeda -lwayland-client

TARGET = batman
TARGET_HELPER = batman-helper
TARGET_GUI = batman-gui
TARGET_GOVERNOR = governor
TARGET_WRAPPERS = libbatman-wrappers.so
TARGET_GBINDER = libbatman-gbinder.so
TARGET_HYBRIS = batman-hybris
TARGET_NFCD = batman.so
TARGET_WIFI = libbatman-wifi.so
TARGET_WIFI_CLI = batman-wifi
TARGET_BATMAN2PPD = src/batman2ppd.py
TARGET_PPDCLI = src/powerprofilesctl.py
TARGET_ANDROMEDA = libbatman-andromeda.so
TARGET_ANDROMEDA_CLI = batman-andromeda
TARGET_NICERDICER = batman-nicerdicer
TARGET_POWERCONFIG = batman-powerconfig
TARGET_EXAMPLES = batman-examples

SRC_HELPER = src/batman-helper.c src/wlrdisplay.c src/batman-wrappers.c src/getinfo.c src/batman-andromeda.c
SRC_GUI = src/batman-gui.c src/configcontrol.c src/getinfo.c
SRC_GOVERNOR = src/governor.c src/wlrdisplay.c src/batman-wrappers.c
SRC_WRAPPERS = src/batman-wrappers.c src/wlrdisplay.c src/getinfo.c
SRC_GBINDER = src/batman-gbinder.c
SRC_HYBRIS = src/batman-hybris.c src/batman-gbinder.c
SRC_NFCD = src/nfcd-batman-plugin.c src/wlrdisplay.c
SRC_WIFI = src/batman-wifi.c
SRC_WIFI_CLI = src/batman-wifi-cli.c src/batman-wifi.c
SRC_ANDROMEDA = src/batman-andromeda.c
SRC_ANDROMEDA_CLI = src/batman-andromeda-cli.c src/batman-andromeda.c
SRC_NICERDICER = src/nicerdicer.c
SRC_POWERCONFIG = src/powerconfig.c
SRC_EXAMPLES = examples/batman-functions.c
HEADERS = src/batman-wrappers.h src/getinfo.h src/wlrdisplay.h src/batman-gbinder.h src/batman-andromeda.h src/batman-wifi.h

PREFIX ?= /usr
LIBDIR ?= $(DESTDIR)$(PREFIX)/lib
BINDIR ?= $(DESTDIR)$(PREFIX)/bin
SBINDIR ?= $(DESTDIR)$(PREFIX)/sbin
ETCDIR ?= $(DESTDIR)/etc
VARDIR ?= $(DESTDIR)/var
CONFIGDIR ?= $(VARDIR)/lib/batman
SYSTEMDDIR ?= $(LIBDIR)/systemd/system
INITDIR ?= $(ETCDIR)/init.d
DESKTOPDIR ?= $(DESTDIR)$(PREFIX)/share/applications
ICONDIR ?= $(DESTDIR)$(PREFIX)/share/icons
INCLUDEDIR ?= $(DESTDIR)$(PREFIX)/include/batman
POLKITDIR ?= $(DESTDIR)$(PREFIX)/share/polkit-1/actions
DBUSDIR ?= $(DESTDIR)$(PREFIX)/share/dbus-1/system.d
NFCDDIR ?= $(LIBDIR)/nfcd/plugins
TRIPLET ?= $(shell $(CC) -dumpmachine)

.PHONY: all
all: helper wrappers governor gui gbinder hybris wifi nfcd andromeda nicerdicer powerconfig

helper: $(TARGET_HELPER)
$(TARGET_HELPER):
	$(CC) $(SRC_HELPER) $(CFLAGS_HELPER) $(LDFLAGS_HELPER) -o $(TARGET_HELPER)

wrappers: $(TARGET_WRAPPERS)
$(TARGET_WRAPPERS):
	$(CC) -fPIC -shared $(SRC_WRAPPERS) $(CFLAGS_WRAPPERS) $(LDFLAGS_WRAPPERS) -o $(TARGET_WRAPPERS)

governor: $(TARGET_GOVERNOR)
$(TARGET_GOVERNOR):
	$(CC) $(SRC_GOVERNOR) $(LDFLAGS_GOVERNOR) $(CFLAGS_GOVERNOR) -o $(TARGET_GOVERNOR)

gui: $(TARGET_GUI)
$(TARGET_GUI):
	$(CC) $(SRC_GUI) $(CFLAGS_GUI) $(LDFLAGS_GUI) -o $(TARGET_GUI)

gbinder: $(TARGET_GBINDER)
$(TARGET_GBINDER):
	$(CC) -fPIC -shared $(SRC_GBINDER) $(CFLAGS_GBINDER) $(LDFLAGS_GBINDER) -o $(TARGET_GBINDER)

hybris: $(TARGET_HYBRIS)
$(TARGET_HYBRIS):
	$(CC) $(SRC_HYBRIS) $(CFLAGS_HYBRIS) $(LDFLAGS_HYBRIS) -o $(TARGET_HYBRIS)

wifi: $(TARGET_WIFI)
$(TARGET_WIFI):
	$(CC) -fPIC -shared $(SRC_WIFI) $(CFLAGS_WIFI) $(LDFLAGS_WIFI) -o $(TARGET_WIFI)
	$(CC) $(SRC_WIFI_CLI) $(CFLAGS_WIFI) -L. -lbatman-wifi $(LDFLAGS_WIFI) -o $(TARGET_WIFI_CLI)

nfcd: $(TARGET_NFCD)
$(TARGET_NFCD): nfcd-batman-plugin.o wlrdisplay.o
	$(CC) $^ $(LDFLAGS_NFCD) -o $@

andromeda: $(TARGET_ANDROMEDA)
$(TARGET_ANDROMEDA):
	$(CC) -fPIC -shared $(SRC_ANDROMEDA) $(CFLAGS_ANDROMEDA) $(LDFLAGS_ANDROMEDA) -o $(TARGET_ANDROMEDA)
	$(CC) $(SRC_ANDROMEDA_CLI) $(CFLAGS_ANDROMEDA) $(LDFLAGS_ANDROMEDA) -o $(TARGET_ANDROMEDA_CLI)

nicerdicer: $(TARGET_NICERDICER)
$(TARGET_NICERDICER):
	$(CC) $(SRC_NICERDICER) $(CFLAGS_NICERDICER) $(LDFLAGS_NICERDICER) -o $(TARGET_NICERDICER)

powerconfig: $(TARGET_POWERCONFIG)
$(TARGET_POWERCONFIG):
	$(CC) $(SRC_POWERCONFIG) $(CFLAGS_POWERCONFIG) $(LDFLAGS_POWERCONFIG) -o $(TARGET_POWERCONFIG)

examples: $(TARGET_EXAMPLES)
$(TARGET_EXAMPLES):
	$(CC) $(SRC_EXAMPLES) $(CFLAGS_EXAMPLES) $(LDFLAGS_EXAMPLES) -o $(TARGET_EXAMPLES)

nfcd-batman-plugin.o: src/nfcd-batman-plugin.c
	$(CC) -c $< $(CFLAGS_NFCD) -O2 -o $@

wlrdisplay.o: src/wlrdisplay.c
	$(CC) -c $< $(CFLAGS_NFCD) -O2 -o $@

.PHONY: install
install: all
	install -d $(LIBDIR) $(BINDIR) $(SBINDIR) $(ETCDIR) $(VARDIR) $(CONFIGDIR) $(SYSTEMDDIR) $(INITDIR) $(DESKTOPDIR)
	install -d $(ICONDIR) $(INCLUDEDIR) $(POLKITDIR) $(DBUSDIR) $(LIBDIR)/$(TRIPLET) $(NFCDDIR)

	cp src/$(TARGET) $(SBINDIR)/
	cp $(TARGET_HELPER) $(BINDIR)
	cp $(TARGET_GUI) $(BINDIR)
	cp $(TARGET_GOVERNOR) $(BINDIR)
	cp $(TARGET_WRAPPERS) $(LIBDIR)/$(TRIPLET)
	cp $(TARGET_GBINDER) $(LIBDIR)/$(TRIPLET)
	cp $(TARGET_HYBRIS) $(BINDIR)
	cp $(TARGET_WIFI) $(LIBDIR)/$(TRIPLET)
	cp $(TARGET_WIFI_CLI) $(BINDIR)
	cp $(TARGET_BATMAN2PPD) $(SBINDIR)/batman2ppd
	cp $(TARGET_PPDCLI) $(BINDIR)/powerprofilesctl
	cp $(TARGET_ANDROMEDA) $(LIBDIR)/$(TRIPLET)
	cp $(TARGET_ANDROMEDA_CLI) $(BINDIR)
	cp $(TARGET_NICERDICER) $(SBINDIR)
	cp $(TARGET_POWERCONFIG) $(SBINDIR)
	cp $(TARGET_NFCD) $(NFCDDIR)

	cp $(HEADERS) $(INCLUDEDIR)

	cp data/batman-gui.desktop $(DESKTOPDIR)
	cp data/batman.png $(ICONDIR)
	cp data/config $(CONFIGDIR)

	cp data/net.hadess.PowerProfiles.policy $(POLKITDIR)
	cp data/net.hadess.PowerProfiles.conf $(DBUSDIR)

	cp data/org.freedesktop.UPower.PowerProfiles.policy $(POLKITDIR)
	cp data/org.freedesktop.UPower.PowerProfiles.conf $(DBUSDIR)

	cp data/io.FuriOS.NicerDicer.conf $(DBUSDIR)
	cp data/io.FuriOS.BatmanPowerConfig.conf $(DBUSDIR)

ifeq ($(shell test -d $(SYSTEMDDIR) && echo 1),1)
	cp data/batman.service $(SYSTEMDDIR)
	cp data/batman2ppd.service $(SYSTEMDDIR)
	cp data/nicerdicer.service $(SYSTEMDDIR)
	cp data/powerconfig.service $(SYSTEMDDIR)
else ifeq ($(shell test -e /sbin/openrc && echo 1),1)
	cp data/batman.rc $(INITDIR)/batman
else
	cp data/batman-init $(INITDIR)/batman
endif

	cp data/nicerdicer.conf $(ETCDIR)/nicerdicer.conf

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
	rm -f $(TARGET_WIFI_CLI)
	rm -f $(TARGET_ANDROMEDA)
	rm -f $(TARGET_ANDROMEDA_CLI)
	rm -f $(TARGET_NICERDICER)
	rm -f $(TARGET_EXAMPLES)
	rm -f $(TARGET_POWERCONFIG)
	rm -f nfcd-batman-plugin.o wlrdisplay.o
