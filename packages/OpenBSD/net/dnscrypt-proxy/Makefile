# $OpenBSD$

COMMENT-main=		secure communications between a DNS client and resolver
COMMENT-plugins=	example plugins for dnscrypt-proxy

DISTNAME=		dnscrypt-proxy-1.1.0
CATEGORIES=		net
MASTER_SITES=		https://github.com/downloads/opendns/dnscrypt-proxy/

HOMEPAGE=		http://dnscrypt.org

MAINTAINER=		Frank Denis <dnscrypt@pureftpd.org>

PERMIT_PACKAGE_CDROM=	No
PERMIT_PACKAGE_FTP=	No
PERMIT_DISTFILES_CDROM=	No
PERMIT_DISTFILES_FTP=	No

MULTI_PACKAGES=		-main -plugins

WANTLIB-main=		c execinfo kvm ltdl m
WANTLIB-plugins=	c ldns m

LIB_DEPENDS-main=	devel/libexecinfo \
			devel/libtool,-ltdl

BUILD_DEPENDS-plugins=	devel/libtool
LIB_DEPENDS-plugins=	net/ldns/libldns

BUILD_DEPENDS=		archivers/gtar
TAR=			${LOCALBASE}/bin/gtar

CONFIGURE_STYLE=	gnu

CONFIGURE_ARGS+=	--enable-plugins --enable-plugins-root

DOCS=			README-PLUGINS.markdown \
			README.markdown \
			TECHNOTES

post-install:
	${INSTALL_DATA_DIR} ${PREFIX}/share/doc/dnscrypt-proxy
	cd ${WRKSRC}; ${INSTALL_DATA} ${DOCS} ${PREFIX}/share/doc/dnscrypt-proxy

.include <bsd.port.mk>
