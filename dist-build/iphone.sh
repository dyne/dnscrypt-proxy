#! /bin/sh

export XCODEDIR="/Applications/Xcode.app/Contents/Developer"
export BASEDIR="${XCODEDIR}/Platforms/iPhoneOS.platform/Developer"
export PATH="${BASEDIR}/usr/bin:$BASEDIR/usr/sbin:$PATH"
export SDK="${BASEDIR}/SDKs/iPhoneOS6.1.sdk"
export CFLAGS="-Oz -mthumb -arch armv7 -isysroot ${SDK}"
export LDFLAGS="-mthumb -arch armv7 -isysroot ${SDK}"
export PREFIX="$(pwd)/dnscrypt-proxy-iphone"

export SODIUM_IPHONE_PREFIX="/tmp/libsodium-ios"
export CPPFLAGS="$CPPFLAGS -I${SODIUM_IPHONE_PREFIX}/include"
export LDFLAGS="$LDFLAGS -L${SODIUM_IPHONE_PREFIX}/lib"

./configure --host=arm-apple-darwin10 \
            --disable-shared \
            --prefix="$PREFIX" && \
make -j3 install && \
echo "dnscrypt-proxy has beein installed into $PREFIX" && \
echo 'Now, using codesign(1) to sign dnscrypt-proxy'
