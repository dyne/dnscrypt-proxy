
COMMENT=	secure communications between a client and a DNS resolver

DISTNAME=	dnscrypt-proxy-0.7
CATEGORIES=	net
MASTER_SITES=	https://github.com/downloads/opendns/dnscrypt-proxy/

HOMEPAGE=	http://www.opendns.com/technology/dnscrypt/

MAINTAINER=	OpenDNS Inc. <dnscrypt@opendns.com>

PERMIT_PACKAGE_CDROM=	No
PERMIT_PACKAGE_FTP=	No
PERMIT_DISTFILES_CDROM=	No
PERMIT_DISTFILES_FTP=	No
WANTLIB=		c m

CONFIGURE_STYLE= gnu

DOCS+=	README.markdown

post-install:
	cd ${WRKSRC}; ${INSTALL_DATA} ${DOCS} ${PREFIX}/share/doc/dnscrypt-proxy

.include <bsd.port.mk>
