# New ports collection makefile for:	dnscrypt-proxy
# Date created:				27 November 2011
# Whom:					OpenDNS Inc.
#

PORTNAME=	dnscrypt-proxy
PORTVERSION=	0.7
CATEGORIES=	dns
MASTER_SITES=	https://github.com/downloads/opendns/dnscrypt-proxy/
LICENSE=	BSD

MAINTAINER=	dnscrypt@opendns.com
COMMENT=	secure communications between a client and a DNS resolver

USE_BZIP2=	yes
USE_RC_SUBR=	dnscrypt-proxy.sh
GNU_CONFIGURE=	yes
MAKE_JOBS_SAFE=	yes
CONFIGURE_ARGS=	--sysconfdir=${PREFIX}/etc \
		--localstatedir=/var

MAN8=		dnscrypt-proxy.8

.include <bsd.port.pre.mk>

PORTDOCS=	AUTHORS COPYING README.markdown

post-install:
.if !defined(NOPORTDOCS)
	@${MKDIR} ${DOCSDIR}
. for doc in ${PORTDOCS}
	${INSTALL_DATA} ${WRKSRC}/${doc} ${DOCSDIR}
. endfor
.endif
	@${CAT} ${PKGMESSAGE}

.include <bsd.port.post.mk>
