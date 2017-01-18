#! /bin/sh

set -e

cd /workdir

apk --update upgrade
apk add build-base
apk add coreutils
apk add ldns-dev

cd libsodium
./configure --disable-dependency-tracking --prefix=/usr
make clean
make -j$(nproc) check
make -j$(nproc) install
/sbin/ldconfig ||:

cd ..
./configure --disable-dependency-tracking --enable-debug
make clean
make -j$(nproc) check
make -j$(nproc) install

/usr/local/sbin/dnscrypt-proxy -t 60 -R dnscrypt.org-fr \
  --plugin=libdcplugin_example.so \
  --plugin=libdcplugin_example_cache.so \
  --plugin=/usr/local/lib/dnscrypt-proxy/libdcplugin_example_ldns_aaaa_blocking.so

echo 'ResolverName dnscrypt.org-fr' > /tmp/dnscrypt-proxy.conf
echo 'LocalCache yes' >> /tmp/dnscrypt-proxy.conf
echo 'BlockIPv6 yes' >> /tmp/dnscrypt-proxy.conf
echo 'Plugin libdcplugin_example.so' >> /tmp/dnscrypt-proxy.conf
echo 'Test 60' >> /tmp/dnscrypt-proxy.conf

/usr/local/sbin/dnscrypt-proxy /tmp/dnscrypt-proxy.conf
