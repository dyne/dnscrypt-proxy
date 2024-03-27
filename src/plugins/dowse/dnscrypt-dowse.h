/*  Dowse - DNSCrypt proxy plugin for DNS management
 *
 *  (c) Copyright 2016 Dyne.org foundation, Amsterdam
 *  Written by Denis Roio aka jaromil <jaromil@dyne.org>
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


#ifndef __DOWSE_H__
#define __DOWSE_H__

// just for NI_MAXHOST
#include <netdb.h>
#include <dowse.h>

// expiration in seconds for the domain hit counter
#define DNS_HIT_EXPIRE 21600

// expiration in seconds for the cache on dns replies
#define DNS_CACHE_EXPIRE 60

#define MAX_QUERY 512
#define MAX_DOMAIN 256
#define MAX_TLD 32
#define MAX_DNS 512  // RFC 6891

#define MAX_LINE 512

typedef struct {
	////////
	// data

	char query[MAX_QUERY]; // incoming query
	size_t query_len; // size of incoming query string

	char domain[MAX_DOMAIN]; // domain part parsed (2nd last dot)
	char tld[MAX_TLD]; // tld (domain extension, 1st dot)

	char from[NI_MAXHOST]; // hostname or ip originating the query
	char mac[32]; // mac address (could be just 12 chars)
	char *ip4; // sin_addr conversion returned by inet_ntoa
	struct in_addr ip4_addr;

	char ownip4[NI_MAXHOST];
	char netmask_ip4[NI_MAXHOST];
	char network_ip4[NI_MAXHOST];

	struct in_addr ownip4_ia;
	struct in_addr netmask_ip4_ia;
	struct in_addr network_ip4_ia;
	char *interface; // from getenv
	int sock;

	// map of known domains
	char *listpath;
	map_t domainlist;

	redisContext *redis;
	redisContext *redis_stor;
	redisReply   *reply;

	// using db_runtime to store cached hits
	redisContext *cache;

	int reverse;

	int debug;
	int offline;
}  plugin_data_t;


// ldns/error.h
void warning(const char *fmt, ...);
void   error(const char *fmt, ...);
void    mesg(const char *fmt, ...);

/// from domainlist.c
size_t trim(char *out, size_t len, const char *str);
void load_domainlist(plugin_data_t *data);
char *extract_domain(plugin_data_t *data);
void free_domainlist(plugin_data_t *data);
///

#endif
