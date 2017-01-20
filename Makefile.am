ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = \
	DNSCRYPT-V2-PROTOCOL.txt \
	README-iOS.markdown \
	README-PLUGINS.markdown \
	README-WINDOWS.markdown \
	README.markdown \
	THANKS \
	apparmor.profile.dnscrypt-proxy \
	autogen.sh \
	dnscrypt-proxy.conf \
	dnscrypt-proxy.service.in \
	dnscrypt-proxy.socket \
	dnscrypt-update-resolvers.sh.in \
	org.dnscrypt.osx.DNSCryptProxy.plist \
	resolvers-check.sh

SUBDIRS = \
	dist-build \
	man

if PLUGINS
SUBDIRS += \
	libltdl
endif

SUBDIRS += \
	contrib \
	src \
	test

dist_pkgdata_DATA = \
	dnscrypt-resolvers.csv \
	minisign.pub

doc_DATA = \
	README.markdown \
	COPYING \
	DNSCRYPT-V2-PROTOCOL.txt \
	dnscrypt-proxy.conf

if PLUGINS
doc_DATA += \
	README-PLUGINS.markdown
endif

if HAVE_SYSTEMD
doc_DATA += \
	dnscrypt-proxy.service \
	dnscrypt-proxy.socket
endif

dnscrypt-proxy.service: dnscrypt-proxy.service.in
	$(SED) \
	-e 's|[@]sbindir@|$(sbindir)|g' \
	-e 's|[@]sysconfdir@|$(sysconfdir)|g' \
	< dnscrypt-proxy.service.in > dnscrypt-proxy.service

CLEANFILES = \
	dnscrypt-proxy.service

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(sysconfdir); \
	if [ -f $(DESTDIR)$(sysconfdir)/dnscrypt-proxy.conf ]; then \
		if cmp -s $(srcdir)/dnscrypt-proxy.conf $(docdir)/dnscrypt-proxy.conf; then \
			echo "Configuration file unchanged"; \
		else \
			echo "The example configuration file [$(docdir)/dnscrypt-proxy.conf] has been updated, you may want to compare it with the one at [$(DESTDIR)$(sysconfdir)/dnscrypt-proxy.conf]"; \
		fi; \
	else \
		$(INSTALL_DATA) $(srcdir)/dnscrypt-proxy.conf $(DESTDIR)$(sysconfdir)/dnscrypt-proxy.conf; \
	fi

uninstall-local:
	if cmp -s $(srcdir)/dnscrypt-proxy.conf $(DESTDIR)$(sysconfdir)/dnscrypt-proxy.conf; then \
		echo "$(DESTDIR)$(sysconfdir)/dnscrypt-proxy.conf has not changed and will be removed."; \
		rm -f $(DESTDIR)$(sysconfdir)/dnscrypt-proxy.conf; \
        rmdir $(DESTDIR)$(sysconfdir) 2> /dev/null; \
	fi
