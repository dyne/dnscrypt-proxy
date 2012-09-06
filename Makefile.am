ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = gnu

EXTRA_DIST = \
	README.markdown \
	README-WINDOWS.markdown \
	TECHNOTES \
	THANKS \
	android.sh \
	autogen.sh \
	org.dnscrypt.osx.DNSCryptProxy.plist \
	iphone.sh

SUBDIRS = \
	man

if PLUGINS
SUBDIRS += \
	libltdl
endif

SUBDIRS += \
	src \
	test
