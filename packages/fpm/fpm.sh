#! /bin/sh

VERSION="0.7"
TMPDIR=${TMPDIR:-/tmp}
BASE_DIR=$(mktemp -d "$TMPDIR"/dnscrypt.XXXXXX)
INSTALL_DIR="$BASE_DIR/usr"

./configure --prefix="$INSTALL_DIR" && make -j4 install

sudo chown -R 0:0 $BASE_DIR
find $BASE_DIR -type d -exec chmod 755 {} \;

for t in deb rpm; do
  fpm -s dir -t "$t" -n dnscrypt-proxy -v "$VERSION" -C "$BASE_DIR"
done
