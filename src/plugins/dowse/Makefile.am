
pkglib_LTLIBRARIES = \
	libdcplugin_dowse.la

libdcplugin_dowse_la_LIBTOOLFLAGS = --tag=disable-static

libdcplugin_dowse_la_SOURCES = database.h dnscrypt-dowse.h dowse.h  \
ip2mac.c dnscrypt-dowse.c domainlist.c hashmap.c redis.c log.c

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



bin_PROGRAMS = \
	dowse-to-gource

dowse_to_gource_SOURCES = \
  dowse-to-gource.c log.c redis.c dowse.h

dowse_to_gource_LDADD = -lhiredis

AM_CFLAGS = @CWFLAGS@
