DNSCrypt Plugins
================

Starting with version 1.1.0, `dnscrypt-proxy` can be extended with
plugins.

A plugin can implement *pre-filters* and *post-filters*.

A DNS query received by the proxy will pass through all *pre-filters*
before being encrypted, signed, and sent to the upstream DNS resolver.

Once a response has been received by the proxy, this response is
going to be validated and decrypted before passing through all
*post-filters*, and eventually the end result will eventually be
delivered to the client.

Filters are given the packets in wire format. Every filter can inspect and
alter these packets at will. A filter can also tell the proxy to totally
ignore a query.

    query -> pre-plugins -> encryption/authentication -> resolver

    client <- post-plugins <- verification/decryption <- resolver

Technically, a plugin is just a native shared library. A good old `.so` file on
Unix, a `.dylib` file on OSX and a `.DLL` file on Windows.

Support for plugins is disabled by default, and has to be explicitly
enabled at compilation time:

    ./configure --enable-plugins

If the `ltdl` library is found on the system, it will be picked up. If
not, a built-in copy will be used instead.

If you intend to distribute a binary package that should run on
systems without the `ltdl` library (which is probably the case on
Windows), add `--with-included-ltdl`:

    ./configure --enable-plugins --with-included-ltdl

Plugins can inspect and mangle packets in any way, but before
reinventing the wheel, take a look at what the `ldns` library has to
offer. `ldns` makes it really easy to parse and build any kind of DNS
packet, can validate DNSSEC records, and is rock solid.

If `ldns` is available on the current system, additional example
plugins will be compiled.

If the `./configure` isn't given a different prefix, example plugins
are installed in `/usr/local/lib/dnscrypt-proxy`.
