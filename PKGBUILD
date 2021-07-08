# Maintainer: Arch Pony <archpony@rambler.ru>
# Contributor: Arch Pony <archpony@rambler.ru>
pkgname=ponywall
pkgver=0.2
pkgrel=1
pkgdesc="Use daily Bing wallpaper with your favourite WM"
arch=('x86_64')
url="https://github.com/archpony/ponywall"
license=('GPL')
depends=('curl' 'json-c' )
makedepends=('gcc' 'make')
optdepends=('feh: default wallpaper setter')
source=("ponywall.c"
        "Makefile")
noextract=("ponywall.c" "Makefile")
md5sums=("SKIP" "SKIP")

build() {
	make
}

package() {
	install -D $pkgname $pkgdir/usr/bin/$pkgname
}
