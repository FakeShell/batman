CC = gcc
CFLAGS = -DWITH_UPOWER -DWITH_WLRDISPLAY `pkg-config --cflags upower-glib gtk4`
LDFLAGS = -lwayland-client `pkg-config --libs upower-glib gtk4`
TARGET = batman-helper
TARGET_GUI = batman-gui
TARGET_GOVERNOR = governor
SRC = batman-helper.c wlrdisplay.c
SRC_GUI = batman-gui.c
SRC_GOVERNOR = governor.c
BINDIR = /usr/bin
LIBDIR = /var/lib/batman
SYSTEMD_DIR = /usr/lib/systemd/system
OPENRC_DIR = /etc/init.d
APT_DIR = /etc/apt/sources.list.d
KEYRING_DIR = /usr/share/keyrings
DESKTOP_DIR = /usr/share/applications
ICON_DIR = /usr/share/icons

.PHONY: all
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o $(TARGET)
	$(CC) $(CFLAGS) $(SRC_GUI) $(LDFLAGS) -o $(TARGET_GUI)
	$(CC) $(CFLAGS) $(SRC_GOVERNOR) $(LDFLAGS) -o $(TARGET_GOVERNOR)

.PHONY: install
install: $(TARGET)
	cp batman $(BINDIR)
	cp batman-gui $(BINDIR)
	cp governor $(BINDIR)
	cp batman-gui.desktop $(DESKTOP_DIR)
	cp batman.png $(ICON_DIR)
	mkdir -p $(LIBDIR)
	cp config $(LIBDIR)
	cp $(TARGET) $(BINDIR)
	chmod +x $(BINDIR)/batman $(BINDIR)/governor $(BINDIR)/batman-gui $(BINDIR)/$(TARGET)

ifeq ($(shell test -d $(SYSTEMD_DIR) && echo 1),1)
	cp batman.service $(SYSTEMD_DIR)
else ifeq ($(shell test -e /sbin/openrc && echo 1),1)
	cp batman.rc $(OPENRC_DIR)/batman
else
	cp batman-init $(OPENRC_DIR)/batman
endif

ifeq ($(shell test -x /usr/bin/apt-get && echo 1),1)
	cp batman.list $(APT_DIR)
	cp batman.gpg $(KEYRING_DIR)/batman.gpg
endif

.PHONY: clean
clean:
	rm -f $(TARGET)
	rm -f $(TARGET_GUI)
	rm -f $(TARGET_GOVERNOR)
