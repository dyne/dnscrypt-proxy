# Copyright 1999-2014 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $
EAPI=5

inherit autotools-utils user

DESCRIPTION="A tool for securing communications between a client and a DNS resolver"
HOMEPAGE="http://dnscrypt.org"
SRC_URI="http://download.dnscrypt.org/dnscrypt-proxy/${P}.tar.gz"

LICENSE="BSD"
SLOT="0"
KEYWORDS="~amd64 ~x86"

DEPEND="
	>=dev-libs/libsodium-1.0.0"
IUSE="-plugins"

AUTOTOOLS_IN_SOURCE_BUILD=1

DOCS=(AUTHORS COPYING INSTALL NEWS README README.markdown TECHNOTES THANKS)

AUTOTOOLS_AUTORECONF=1

pkg_setup() {
	enewgroup dnscrypt
	enewuser dnscrypt -1 -1 /var/empty dnscrypt
}

src_configure() {
	local myeconfargs=(
		$(use_enable plugins)
	)
	autotools-utils_src_configure
}

src_install() {
	autotools-utils_src_install

	newinitd "${FILESDIR}/dnscrypt-proxy_1_4_0.initd" dnscrypt-proxy || die "newinitd failed"
	newconfd "${FILESDIR}/dnscrypt-proxy_1_4_0.confd" dnscrypt-proxy || die "newconfd failed"
}
