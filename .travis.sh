#! /bin/sh

set -e

cd /workdir

apk --update upgrade
apk add alpine-sdk
apk add coreutils
apk add ldns-dev

cd libsodium
./configure --disable-dependency-tracking --prefix=/usr
make clean
make -j$(nproc) check
make -j$(nproc) install ||:
/sbin/ldconfig ||:

cd ..
./configure --disable-dependency-tracking --enable-debug
make clean
make -j$(nproc) check
