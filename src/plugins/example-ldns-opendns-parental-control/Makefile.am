
pkglib_LTLIBRARIES = \
	libdcplugin_example_ldns_opendns_parental_control.la

libdcplugin_example_ldns_opendns_parental_control_la_LIBTOOLFLAGS = --tag=disable-static

libdcplugin_example_ldns_opendns_parental_control_la_SOURCES = \
	example-ldns-opendns-parental-control.c

libdcplugin_example_ldns_opendns_parental_control_la_LIBADD = @LDNS_LIBS@

libdcplugin_example_ldns_opendns_parental_control_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-avoid-version \
	-export-dynamic \
	-module \
	-no-undefined

libdcplugin_example_ldns_opendns_parental_control_la_CPPFLAGS = \
	$(LTDLINCL) \
	-I../../include
