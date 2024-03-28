![image](https://github.com/dyne/dnscrypt-proxy/assets/148059/4b242442-e1cf-4d0e-9ff7-d599ba3b6e81)

## Status of the project

The DNScrypt v2 C++ implementation was taken offline by its creator and maintainer Frank Denis on the 6th December 2017, after announcing in November 2017 that [the project needs a new maintainer](https://twitter.com/jedisct1/status/928942292202860544).

The [dnscrypt.org](https://dnscrypt.org) webpage lists a good number of end-user resources built from a new implementation written in Go.  

At Dyne.org we rely on the v2 of the DNScrypt protocol and this older but still working C++ implementation of dnscrypt-proxy for our [Dowse.eu](https://dyne.org/software/dowse) project and we keep maintaining the C++ implementation of dnscrypt-proxy.

## What is DNSCrypt

DNSCrypt is a protocol for securing communications between a client
and a DNS resolver, using high-speed high-security elliptic-curve
cryptography.

While not providing end-to-end security, it protects the local network, which
is often the weakest point of the chain, against man-in-the-middle attacks.

`dnscrypt-proxy` is a client-implementation of the protocol. It
requires a DNS server made available by the [DNSCrypt](https://github.com/DNSCrypt/) project.

Plugins
-------

Aside from implementing the DNSCrypt v2 protocol, the C++ dnscrypt-proxy can be extended
with plug-ins, and gives a lot of control on the local DNS traffic:

- Provide nifty real-time traffic visualization using the Dowse plugin.
- Review the DNS traffic originating from your network in real time,
and detect compromised hosts and applications phoning home.
- Locally block ads, trackers, malware, spam, and any website whose
domain names or IP addresses match a set of rules you define.
- Prevent queries for local zones from being leaked.
- Reduce latency by caching resposes and avoiding requesting IPv6
addresses on IPv4-only networks.
- Force traffic to use TCP, to route it through TCP-only tunnels or
Tor.
