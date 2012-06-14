#! /bin/sh

if glibtoolize --version > /dev/null 2>&1; then
  LIBTOOLIZE='glibtoolize'
else
  LIBTOOLIZE='libtoolize'
fi
$LIBTOOLIZE && \
aclocal -I m4 && \
autoheader && \
automake --gnu --add-missing --include-deps && \
autoconf -I m4
