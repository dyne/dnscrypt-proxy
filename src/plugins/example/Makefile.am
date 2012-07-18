
PLUGIN_VERSION = 1:0:0

pkglib_LTLIBRARIES = \
	libdcmod_example.la

libdcmod_example_la_SOURCES = \
	example.c

libdcmod_example_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-export-dynamic \
	-version-info $(PLUGIN_VERSION)
