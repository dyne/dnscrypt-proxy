#! /bin/sh

set -e

cd libsodium
./configure --disable-dependency-tracking
sudo make -j$(nproc) install
sudo ldconfig
cd ..
./configure --disable-dependency-tracking --enable-debug
make clean
make -j$(nproc) check
