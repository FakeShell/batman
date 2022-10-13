# Contributor: Bardia Moshiri <fakeshell@bardia.tech>
# Maintainer: Bardia Moshiri <fakeshell@bardia.tech>
pkgname=batman
pkgver=0.32
pkgrel=0
pkgdesc="A battery management service and program for linux phones"
url="https://github.com/FakeShell/batman"
arch="noarch"
license="GPLv2"
depends="wlr-randr bash upower yad"
makedepends="git tar"
source="https://github.com/FakeShell/batman/releases/download/batman/batman.tar.gz"
options="!check"

package() {
	install -Dm755 $srcdir/$pkgname/batman "$pkgdir/usr/bin/batman"
	install -Dm755 $srcdir/$pkgname/batman-gui "$pkgdir/usr/bin/batman-gui"
	install -Dm755 $srcdir/$pkgname/governor "$pkgdir/usr/bin/governor"
	install -Dm644 $srcdir/$pkgname/config "$pkgdir/var/lib/$pkgname/config"
	install -Dm644 $srcdir/$pkgname/batman.rc "$pkgdir/etc/init.d/batman"
	install -Dm644 $srcdir/$pkgname/batman.png "$pkgdir/usr/share/icons/batman.png"
	install -Dm644 $srcdir/$pkgname/batman-gui.desktop "$pkgdir/usr/share/applications/batman-gui.desktop"
}

sha512sums="
b749970b4f3c830ead30a81ee3c12790a59d8175da53554d510b913dec0d572269ef4b42ca8eeebffaf41e49c0ebd8c5d3d9cdfcf1276112a046bce89f2469a8  batman.tar.gz
"
