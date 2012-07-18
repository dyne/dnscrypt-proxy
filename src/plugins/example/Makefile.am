
PLUGIN_VERSION =        1:0:0
#                       | | |
#                +------+ | +---+
#                |        |     |
#             current:revision:age
#                |        |     |
#                |        |     +- increment if interfaces have been added
#                |        |        set to zero if interfaces have been removed
#                |        |        or changed
#                |        +- increment if source code has changed
#                |           set to zero if current is incremented
#                +- increment if interfaces have been added, removed or changed

pkglib_LTLIBRARIES = \
	libdcplugin_example.la

libdcplugin_example_la_SOURCES = \
	example.c

libdcplugin_example_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-export-dynamic \
	-module \
	-version-info $(PLUGIN_VERSION)

libdcplugin_example_la_CPPFLAGS = \
	$(LTDLINCL) \
	-I../../include
