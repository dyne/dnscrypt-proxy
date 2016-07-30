#! /bin/sh

export CFLAGS="-Os -m64 -mtune=westmere"
export LDFLAGS="-mtune=westmere"
export PREFIX="$(pwd)/dnscrypt-proxy-win64"
export MINGW_PREFIX='/mingw64'
export SODIUM_PREFIX='/tmp/libsodium-win64'

export CPPFLAGS="${CPPFLAGS} -I${SODIUM_PREFIX}/include"
export LDFLAGS="${LDFLAGS} -L${SODIUM_PREFIX}/lib"

./configure --prefix="$PREFIX" --exec-prefix="$PREFIX" \
  --host=x86_64-w64-mingw32 \
  --bindir="$PREFIX" \
  --sbindir="$PREFIX" \
  --with-included-ltdl && \
make install

rm -fr "${PREFIX}/share"
rm -fr "${PREFIX}/lib/pkgconfig"
mv "${PREFIX}/lib/dnscrypt-proxy/"*.dll "${PREFIX}/"
mv "${PREFIX}/lib/dnscrypt-proxy/"*.la "${PREFIX}/"
rm -fr "${PREFIX}/lib"
cp "${MINGW_PREFIX}/bin/libwinpthread-1.dll" "${PREFIX}/"
cp "${MINGW_PREFIX}/bin/libgcc_s_dw2-1.dll" "${PREFIX}/"
cp "${MINGW_PREFIX}/bin/libldns-1.dll" "${PREFIX}/"
cp "${SODIUM_PREFIX}/bin/libsodium-18.dll" "${PREFIX}/"

nm "${MINGW_PREFIX}/bin/libldns-1.dll" | \
  fgrep -i libeay &&
  cp "${MINGW_PREFIX}/bin/"libeay*.dll "${PREFIX}/"

cp dnscrypt-resolvers.csv "${PREFIX}/"
