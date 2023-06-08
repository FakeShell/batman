CC = gcc
CFLAGS = -DWITH_UPOWER -DWITH_WLRDISPLAY `pkg-config --cflags upower-glib gtk4`
LDFLAGS = -lwayland-client `pkg-config --libs upower-glib gtk4`
TARGET = batman
TARGET_HELPER = batman-helper
TARGET_GUI = batman-gui
TARGET_GOVERNOR = governor
SRC_HELPER = src/batman-helper.c src/wlrdisplay.c
SRC_GUI = src/batman-gui.c src/configcontrol.c src/getinfo.c
SRC_GOVERNOR = src/governor.c
BINDIR = /usr/bin
CONFIGDIR = /var/lib/batman
SYSTEMD_DIR = /usr/lib/systemd/system
OPENRC_DIR = /etc/init.d
APT_DIR = /etc/apt/sources.list.d
KEYRING_DIR = /usr/share/keyrings
DESKTOP_DIR = /usr/share/applications
ICON_DIR = /usr/share/icons

.PHONY: all
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC_HELPER) $(LDFLAGS) -o $(TARGET_HELPER)
	$(CC) $(CFLAGS) $(SRC_GUI) $(LDFLAGS) -o $(TARGET_GUI)
	$(CC) $(CFLAGS) $(SRC_GOVERNOR) $(LDFLAGS) -o $(TARGET_GOVERNOR)

.PHONY: install
install: $(TARGET)
	cp src/$(TARGET) $(BINDIR)
	cp $(TARGET_HELPER) $(BINDIR)
	cp $(TARGET_GUI) $(BINDIR)
	cp $(TARGET_GOVERNOR) $(BINDIR)
	cp data/batman-gui.desktop $(DESKTOP_DIR)
	cp data/batman.png $(ICON_DIR)
	mkdir -p $(CONFIGDIR)
	cp data/config $(CONFIGDIR)
	chmod +x $(BINDIR)/$(TARGET) $(BINDIR)/$(TARGET_HELPER) $(BINDIR)/$(TARGET_GUI) $(BINDIR)/$(TARGET_GOVERNOR)

ifeq ($(shell test -d $(SYSTEMD_DIR) && echo 1),1)
	cp data/batman.service $(SYSTEMD_DIR)
else ifeq ($(shell test -e /sbin/openrc && echo 1),1)
	cp data/batman.rc $(OPENRC_DIR)/batman
else
	cp data/batman-init $(OPENRC_DIR)/batman
endif

ifeq ($(shell test -x /usr/bin/apt-get && echo 1),1)
	cp data/batman.list $(APT_DIR)
	cp data/batman.gpg $(KEYRING_DIR)/batman.gpg
endif

.PHONY: clean
clean:
	rm -f $(TARGET_HELPER)
	rm -f $(TARGET_GUI)
	rm -f $(TARGET_GOVERNOR)
