#! /bin/sh

export CFLAGS="-Os -march=pentium2 -mtune=nocona"
export PREFIX="$(pwd)/dnscrypt-proxy-win32"

./configure --prefix="$PREFIX" --exec-prefix="$PREFIX" \
  --enable-plugins \
  --with-included-ltdl && \
make install-strip

upx --best --ultra-brute /usr/local/sbin/dnscrypt-proxy.exe &
upx --best --ultra-brute /usr/local/bin/hostip.exe

wait
