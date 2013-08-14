EAPI="3"

inherit eutils flag-o-matic

DESCRIPTION="a new easy-to-use high-speed software library for network communication, encryption, decryption, signatures, etc."
HOMEPAGE="http://download.libsodium.org/libsodium/releases/"
SRC_URI="http://download.libsodium.org/libsodium/releases/${P}.tar.gz"

LICENSE="BSD"
SLOT="0"
KEYWORDS="amd64 i386"

src_configure() {
	append-ldflags -Wl,-z,noexecstack || die
	econf || die "econf failed"
}

src_install() {
	emake DESTDIR="${D}" install || die "emake install failed"

	dodoc {AUTHORS,ChangeLog,COPYING,NEWS,README,INSTALL,THANKS} || die "dodoc failed"
}
