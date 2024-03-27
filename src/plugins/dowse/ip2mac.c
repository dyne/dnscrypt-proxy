/*  Dowse - ip2mac translation function
 *
 *  (c) Copyright 2016-2017 Dyne.org foundation, Amsterdam
 *  Written by Nicola Rossi <nicola@dyne.org>
 *             Denis Roio <jaromil@dyne.org>
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Public License as published
 * by the Free Software Foundation; either version 3 of the License,
 * or (at your option) any later version.
 *
 * This source code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * Please refer to the GNU Public License for more details.
 *
 * You should have received a copy of the GNU Public License along with
 * this source code; if not, write to:
 * Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <linux/sockios.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "dowse.h"
#include "dnscrypt-dowse.h"

#define IP2MAC_ERROR (1)
#define IP2MAC_RESULT_OK (0)

int convert_from_ipv4(char *ipaddr_value, char *mac_addr);
int convert_from_ipv6(char *ipaddr_value, char *mac_addr);

void ethernet_mactoa(struct sockaddr *addr, char*buff) {

    unsigned char *ptr = (unsigned char *) addr->sa_data;

    sprintf(buff, "%02x:%02x:%02x:%02x:%02x:%02x", (ptr[0] & 0377),
            (ptr[1] & 0377), (ptr[2] & 0377), (ptr[3] & 0377), (ptr[4] & 0377),
            (ptr[5] & 0377));
}

int ip4_derive_mac(plugin_data_t *data) {
	struct arpreq areq;
	struct sockaddr_in *sin;
	struct sockaddr_in * p;
	char buf[256];

	/* Make the ARP request. */
	memset(&areq, 0, sizeof(areq));
	sin = (struct sockaddr_in *) &areq.arp_pa;
	sin->sin_family = AF_INET;

	sin->sin_addr = data->ip4_addr;
	sin = (struct sockaddr_in *) &areq.arp_ha;
	sin->sin_family = ARPHRD_ETHER;

	strncpy(areq.arp_dev, data->interface, 15);

	if (ioctl(data->sock, SIOCGARP, (caddr_t) &areq) == -1) {
		warn("Error requesting ARP for IP [%s] on device [%s]: %s",
		     data->ip4, data->interface, strerror(errno));
		return 1;
	}

	p = (struct sockaddr_in *) &(areq.arp_pa);

	inet_ntop(AF_INET, &(p->sin_addr), buf, sizeof(buf));

	ethernet_mactoa(&areq.arp_ha, data->mac);

	return 0;
}

int convert_from_ipv6(char *ipaddr_value, char *mac_addr) {
    int socket_fd;
    struct arpreq areq;
    struct sockaddr_in6 *sin;
    struct in6_addr ipaddr;

    /* Get an internet domain socket. */
    if ((socket_fd = socket(AF_INET6, SOCK_DGRAM, 0)) == -1) {
        err("Sorry but during IP-ARP conversion I cannot open a socket [%s]",
                strerror(errno));
        return (1);
    }

    /* Make the ARP request. */
    memset(&areq, 0, sizeof(areq));
    sin = (struct sockaddr_in6 *) &areq.arp_pa;
    sin->sin6_family = AF_INET6;

    if (inet_pton(AF_INET6, ipaddr_value, &ipaddr) == 0) {
        err(
                "Sorry but during IP-ARP conversion I cannot execute inet_aton(%s) due to (%s)",
                ipaddr_value, strerror(errno));

        close(socket_fd);
        return (1);
    }

    sin->sin6_addr = ipaddr;
    sin = (struct sockaddr_in6 *) &areq.arp_ha;
    sin->sin6_family = ARPHRD_ETHER;

    /* TODO definizione di device su cui e' attestata webui */
    char *dev = getenv("interface");
    if (dev == NULL) {
        dev = "lo";
    }

    strncpy(areq.arp_dev, dev, 15);

    if (ioctl(socket_fd, SIOCGARP, (caddr_t) &areq) == -1) {
        err(
                "-- Error: unable to make ARP request for IP [%s], error on device [%s] due to [%s]",
                ipaddr_value, dev, strerror(errno));
        return (1);
    }
    char buf[256];
    struct sockaddr_in6 * p;
    p = (struct sockaddr_in6 *) &(areq.arp_pa);
    inet_ntop(AF_INET6, &(p->sin6_addr), buf, sizeof(buf));

    ethernet_mactoa(&areq.arp_ha, mac_addr);

    func("Conversion form %s  -> %s\n", ipaddr_value, mac_addr);
    close(socket_fd);
    return 0;
}
