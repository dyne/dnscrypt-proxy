#! /bin/sh

export CFLAGS="-mmacosx-version-min=10.8"
export LDFLAGS="-mmacosx-version-min=10.8"

./configure --with-included-ltdl \
            --enable-plugins \
            --enable-plugins-root && \
make -j3
