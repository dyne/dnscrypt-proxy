#! /bin/sh

export LDFLAGS='-Wl,-static -static -static-libgcc -s -Wl,--gc-sections'
export CFLAGS='-Os -fomit-frame-pointer'

./configure --host=mipsel-linux && \
  make -j3
