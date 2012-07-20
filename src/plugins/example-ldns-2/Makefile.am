
pkglib_LTLIBRARIES = \
	libdcplugin_example_ldns_2.la

libdcplugin_example_ldns_2_la_LIBTOOLFLAGS = --tag=disable-static

libdcplugin_example_ldns_2_la_SOURCES = \
	example-ldns-2.c

libdcplugin_example_ldns_2_la_LIBADD = @LDNS_LIBS@

libdcplugin_example_ldns_2_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-avoid-version \
	-export-dynamic \
	-module \
	-no-undefined

libdcplugin_example_ldns_2_la_CPPFLAGS = \
	$(LTDLINCL) \
	-I../../include
