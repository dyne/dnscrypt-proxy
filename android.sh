#! /bin/sh

export TARGET=arm-linux-androideabi
export TARGET_TOOLCHAIN_VERSION=4.4.3
export DROID_HOST=darwin-x86
export NDK_TARGET="arm-linux-androideabi-${TARGET_TOOLCHAIN_VERSION}"
export NDK_ROOT=/usr/local/Cellar/android-ndk/r8b
export NDK_PLATFORM=8
export CFLAGS="-Os -mthumb"
export LDFLAGS="-mthumb"

./configure --host=arm-linux-androideabi && make -j3
