[![Build Status](https://travis-ci.org/jedisct1/dnscrypt-proxy.png?branch=master)](https://travis-ci.org/jedisct1/dnscrypt-proxy?branch=master)

[![DNSCrypt](https://raw.github.com/jedisct1/dnscrypt-proxy/master/dnscrypt-small.png)](https://dnscrypt.org)
============

A protocol for securing communications between a client and a DNS resolver.

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

Signatures can be verified with the
[Minisign](https://jedisct1.github.io/minisign/) tool:

    $ minisign -VP RWQf6LRCGA9i53mlYecO4IzT51TGPpvWucNSCh1CBM0QTaLn73Y7GFO3 -m dnscrypt-proxy-1.9.0.tar.bz2
