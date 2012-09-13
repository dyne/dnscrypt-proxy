#! /bin/sh

export CFLAGS="-fdata-sections -ffunction-sections -Os \
               -fomit-frame-pointer -march=pentium4 -mtune=core2"

export LDFLAGS="-Wl,--gc-sections"

./configure --disable-ssp && make -j3 install-strip

upx --best --ultra-brute /usr/local/sbin/dnscrypt-proxy.exe &
upx --best --ultra-brute /usr/local/bin/hostip.exe

wait
