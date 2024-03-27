
pkglib_LTLIBRARIES = \
	libdcplugin_dowse.la

libdcplugin_dowse_la_LIBTOOLFLAGS = --tag=disable-static

libdcplugin_dowse_la_SOURCES = \
	dnscrypt-dowse.c domainlist.c ip2mac.c database.h dnscrypt-dowse.h

libdcplugin_dowse_la_LIBADD = @LDNS_LIBS@ -lhiredis

libdcplugin_dowse_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-avoid-version \
	-export-dynamic \
	-module \
	-no-undefined

libdcplugin_dowse_la_CPPFLAGS = \
	$(LTDLINCL) \
	-I../../include
