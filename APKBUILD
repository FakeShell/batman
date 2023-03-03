# Contributor: Bardia Moshiri <fakeshell@bardia.tech>
# Maintainer: Bardia Moshiri <fakeshell@bardia.tech>

pkgname=batman
pkgver=0.37
pkgrel=0
pkgdesc="A battery management service and program for Linux phones"
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
d6d79f1299c9e29e5376946b93860fc7caea50ff48522230b62019c5af297f4c283edd89a3df27f5795ac95adea027515a01280bd00b8adf8dd6bde30eaf0441 batman.tar.gz
"
