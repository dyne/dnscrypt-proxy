[![Build Status](https://travis-ci.org/jedisct1/dnscrypt-proxy.png?branch=master)](https://travis-ci.org/jedisct1/dnscrypt-proxy?branch=master)

[![DNSCrypt](https://raw.github.com/jedisct1/dnscrypt-proxy/master/dnscrypt-small.png)](https://dnscrypt.org)
============

A protocol for securing communications between a client and a DNS resolver.

Disclaimer
----------

`dnscrypt-proxy` verifies that responses you get from a DNS provider
have been actually sent by that provider, and haven't been tampered
with.

This is not a VPN. It doesn't mask your IP address, and if you are
using it with a public DNS service, be aware that it will (and has to)
decrypt your queries.

Description
-----------

`dnscrypt-proxy` provides local service which can be used directly as your
local resolver or as a DNS forwarder, authenticating requests using the
DNSCrypt protocol and passing them to an upstream server.

The DNSCrypt protocol uses high-speed high-security elliptic-curve
cryptography and is very similar to [DNSCurve](https://dnscurve.org/), but
focuses on securing communications between a client and its first-level
resolver.

While not providing end-to-end security, it protects the local network, which
is often the weakest point of the chain, against man-in-the-middle attacks.

`dnscrypt-proxy` is only a client-implementation of the protocol. It requires
a [DNSCrypt server](https://www.dnscrypt.org/#dnscrypt-server) on the other
end.

Online documentation
--------------------

For complete and up-to-date documentation, please refer to the
[dnscrypt-proxy wiki](https://github.com/jedisct1/dnscrypt-proxy/wiki/).

For the most recent news about the project, please refer to the
[dnscrypt website](https://dnscrypt.org).

Download and integrity check
----------------------------

dnscrypt-proxy can be downloaded here:
[dnscrypt-proxy download](https://download.dnscrypt.org/dnscrypt-proxy/)

Note: dnscrypt.org is now blocked by the Great Firewall of China. But
the source code can also be downloaded on Github, in the "releases"
section.

Signatures can be verified with the
[Minisign](https://jedisct1.github.io/minisign/) tool:

    $ minisign -VP RWQf6LRCGA9i53mlYecO4IzT51TGPpvWucNSCh1CBM0QTaLn73Y7GFO3 -m dnscrypt-proxy-1.9.0.tar.bz2

Installation
------------

Compiling dnscrypt-proxy from source requires the
[libsodium](https://libsodium.org) crypto library.

The [ldns](https://www.nlnetlabs.nl/projects/ldns/) library is
optional, but highly recommended.

Running dnscrypt-proxy using systemd
------------------------------------

On a system using systemd, and when compiled with `--with-systemd`,
the proxy can take advantage of systemd's socket activation instead of
creating the sockets itself. The proxy will also notify systemd on
successful startup.

Two sockets need to be configured: a UDP socket (`ListenStream`) and a
TCP socket (`ListenDatagram`) sharing the same port.

The source distribution includes the `dnscrypt-proxy.socket` and
`dnscrypt-proxy.service` files that can be used as a starting point.

Installation as a service (Windows only)
----------------------------------------

The proxy can be installed as a Windows service.

See the [DNSCrypt Wiki](https://github.com/jedisct1/dnscrypt-proxy/wiki/)
for more information on using DNSCrypt on Windows.

Configuration file
------------------

Starting with version 1.8.0, a configuration file can be used instead
of supplying command-line switches.

The distribution includes a sample configuration file named
`dnscrypt-proxy.conf`.

In order to start the server with a configuration file, provide the
name of that file without any additional switches:

    # dnscrypt-proxy /etc/dnscrypt-proxy.conf

Using DNSCrypt in combination with a DNS cache
----------------------------------------------

For optimal performance, the recommended way of running DNSCrypt is to run it
as a forwarder for a local DNS cache, such as:
* [unbound](https://www.unbound.net/)
* [powerdns-recursor](https://www.powerdns.com/recursor.html)
* [edgedns](https://github.com/jedisct1/edgedns)
* [acrylic DNS proxy](http://mayakron.altervista.org/wikibase/show.php?id=AcrylicHome)

These DNS caches can safely run on the same machine as long as they are
listening to different IP addresses (preferred) or different ports.

If your DNS cache is `unbound`, all you need is to edit the `unbound.conf`
file and add the following lines at the end of the `server` section:

    do-not-query-localhost: no

    forward-zone:
      name: "."
      forward-addr: 127.0.0.1@40

The first line is not required if you are using different IP addresses
instead of different ports.

Then start `dnscrypt-proxy`, telling it to use a specific port (`40`,
in this example):

    # dnscrypt-proxy --local-address=127.0.0.1:40 --daemonize

IPv6 support
------------

IPv6 is fully supported. IPv6 addresses with a port number should be
specified as `[ip]:port`.

Queries using nonstandard ports / over TCP
------------------------------------------

By default, `dnscrypt-proxy` sends outgoing queries to UDP port 443.

However, the DNSCrypt proxy can force outgoing queries to be sent over
TCP. For example, TCP port 443, which is commonly used for
communication over HTTPS, may not be filtered at places where UDP is.

The `TcpOnly` parameter forces this behavior. When an incoming query
is received, the daemon immediately replies with a "response
truncated" message, forcing the client to retry over TCP. The daemon
then authenticates the query and forwards it over TCP to the resolver.

The `TcpOnly` mode is slower than UDP because multiple queries over a
single TCP connections aren't supported yet, and this workaround
should never be used except when bypassing a filter is actually
required.

The `hostip` utility
--------------------

The DNSCrypt proxy ships with a simple tool named `hostip` that
resolves a name to IPv4 or IPv6 addresses.

This tool can be useful for starting some services before
`dnscrypt-proxy`.

Queries made by `hostip` are not authenticated.

Plugins
-------

`dnscrypt-proxy` can be extended with plugins. A plugin acts as a
filter that can locally inspect and modify queries and responses.

The plugin API is documented in the `README-PLUGINS.markdown` file.
