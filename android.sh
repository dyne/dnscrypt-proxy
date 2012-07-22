#! /bin/sh

TARGET=arm-linux-androideabi
TARGET_TOOLCHAIN_VERSION=4.4.3
DROID_HOST=darwin-x86
NDK_TARGET="arm-linux-androideabi-${TARGET_TOOLCHAIN_VERSION}"
NDK_ROOT=/usr/local/Cellar/android-ndk/r8
NDK_PLATFORM=14

./configure --host=arm-linux-androideabi && make -j3
