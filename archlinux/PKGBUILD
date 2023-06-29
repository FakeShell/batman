# Maintainer: Bardia Moshiri <fakeshell@bardia.tech>

pkgname='batman'
pkgver=1.41
pkgrel=1
_commit=70efa135771a18f431d91c8341af717831699eab
pkgdesc="A battery management service for Linux phones"
arch=(any)
url="https://github.com/FakeShell/batman"
license=('GPLv2')
depends=('gtk4' 'upower' 'bash' 'which' 'bluez-utils')
makedepends=('git' 'tar' 'gtk4' 'libadwaita' 'pkgconf' 'wayland' 'gcc')
provides=('batman')
source=("git+https://github.com/FakeShell/batman#commit=${_commit}")
md5sums=('SKIP')

build() {
  cd "$srcdir/batman"

  gcc -DWITH_UPOWER -DWITH_WLRDISPLAY src/batman-helper.c src/wlrdisplay.c -o batman-helper -lwayland-client `pkg-config --cflags --libs upower-glib`
  gcc src/batman-gui.c src/configcontrol.c src/getinfo.c -o batman-gui `pkg-config --cflags --libs gtk4 libadwaita-1`
  gcc src/governor.c -o governor
}

package() {
  cd "$srcdir/batman"

  install -Dm755 src/batman "${pkgdir}/usr/bin/batman"
  install -Dm755 batman-helper "${pkgdir}/usr/bin/batman-helper"
  install -Dm755 governor "${pkgdir}/usr/bin/governor"
  install -Dm755 batman-gui "${pkgdir}/usr/bin/batman-gui"
  install -Dm644 data/config "${pkgdir}/var/lib/batman/config"
  install -Dm644 data/batman-gui.desktop "${pkgdir}/usr/share/applications/batman-gui.desktop"
  install -Dm644 data/batman.png "${pkgdir}/usr/share/icons/batman.png"
}
