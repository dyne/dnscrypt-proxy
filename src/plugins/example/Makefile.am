
PLUGIN_VERSION = 1:0:0

pkglib_LTLIBRARIES = \
	libdcplugin_example.la

libdcplugin_example_la_SOURCES = \
	example.c

libdcplugin_example_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-export-dynamic \
	-version-info $(PLUGIN_VERSION)

libdcplugin_example_la_CPPFLAGS = \
	$(LTDLINCL) \
	-I../../include
