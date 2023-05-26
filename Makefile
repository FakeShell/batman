CC = gcc
CFLAGS = -DWITH_UPOWER -DWITH_WLRDISPLAY `pkg-config --cflags upower-glib`
LDFLAGS = -lwayland-client `pkg-config --libs upower-glib`
TARGET = batman-helper
SRC = batman-helper.c wlrdisplay.c
DESTDIR = /usr/bin
LIBDIR = /var/lib/batman
SYSTEMD_DIR = /usr/lib/systemd/system
OPENRC_DIR = /etc/init.d
APT_DIR = /etc/apt/sources.list.d
KEYRING_DIR = /usr/share/keyrings

.PHONY: all
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o $(TARGET)

.PHONY: install
install: $(TARGET)
	cp batman $(DESTDIR)
	cp governor $(DESTDIR)
	mkdir -p $(LIBDIR)
	cp config $(LIBDIR)
	cp $(TARGET) $(DESTDIR)
	chmod +x $(DESTDIR)/batman $(DESTDIR)/governor $(DESTDIR)/$(TARGET)

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
