/*  Dowse - DNSCrypt proxy plugin for DNS management
 *
 *  (c) Copyright 2016-202430 Dyne.org foundation, Amsterdam
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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <inttypes.h>
#include <netdb.h>

#include <dnscrypt/plugin.h>
#include <ldns/ldns.h>

#include <hiredis/hiredis.h>
// #include <jemalloc/jemalloc.h>

// #include <epoch.h>

#include <dnscrypt-dowse.h>

// 24 hours
#define CACHE_EXPIRY 5

#ifndef DEBUG
#define DEBUG 1
#endif

DCPLUGIN_MAIN(__FILE__);

int publish_query(plugin_data_t *data);

// returns the answer in a wire format buffer (allocated in this function)
// fills in wire packet lenght in *asize
// returned buffer must be freed with LDNS_FREE
uint8_t *answer_to_question(uint16_t pktid, ldns_rr *question_rr, char *answer, size_t *asize);

// ip2mac address translation tool
int ip4_derive_mac(plugin_data_t *data);

const char * dcplugin_description(DCPlugin * const dcplugin) {
	return "Dowse plugin to filter dnscrypt queries";
}


const char * dcplugin_long_description(DCPlugin * const dcplugin) {
	return
		"This plugin checks all settings in Dowse and operates filtering on dnscrypt queries accordingly\n";
}
int dcplugin_init(DCPlugin * const dcplugin, int argc, char *argv[]) {
	int i;


	func("compile time : %s", __TIME__);


	plugin_data_t *data = malloc(sizeof(plugin_data_t));
	memset(data, 0x0, sizeof(plugin_data_t));


	// TODO: check succesful result or refuse to init

	data->cache = NULL;
	data->offline = 0;
	data->debug = DEBUG;
	if(data->debug) act("BUILD TIME DEBUG ON");

	for(i=0; i<argc; i++) {
		func("%u arg: %s", i, argv[i]);

		if( strncmp(argv[i], "debug", 5) == 0) {
			data->debug = 1;
			act("RUN TIME DEBUG ON");
			// TODO: check error
		}

		if( strncmp(argv[i], "offline", 7) == 0) data->offline=1;

	}

	data->listpath = getenv("DOWSE_DOMAINLIST");
	if(!data->listpath) {
		warn("environmental variable DOWSE_DOMAINLIST not set");
		data->listpath = NULL;
	} else {
		act("Loading domainlist from: %s", data->listpath);
		load_domainlist(data);
	}

	{
		char *stmp = getenv("DOWSE_LAN_ADDRESS_IP4");
		if(!stmp) {
			warn("own IP4 on LAN not known, variable DOWSE_LAN_ADDRESS_IP4 not set");
			data->ownip4[0] = 0x0;
		} else {
			strncpy(data->ownip4, stmp, NI_MAXHOST);
			act("Own IPv4 address on LAN: %s", data->ownip4);
		}
	}


	{
		char *stmp = getenv("DOWSE_LAN_NETMASK_IP4");
		if(!stmp) {
			warn("IP4 netmask not known, variable DOWSE_LAN_NETMASK_IP4 not set");
			data->netmask_ip4[0] = 0x0;
		} else {
			strncpy(data->netmask_ip4, stmp, NI_MAXHOST);
			act("Own IPv4 netmask for LAN: %s", data->netmask_ip4);
		}
		// // cheap trick to verify if ip4
		// if( sscanf(ipaddr, "%d.%d.%d.%d", &a, &b, &c, &d) != 4)
		// 	return 1;
	}

	// data->interface = getenv("interface");
	// if(!data->interface) {
	// 	err("Error: no interface configured in Dowse");
	// 	return DCP_SYNC_FILTER_RESULT_ERROR;
	// }

	// Get an internet domain socket.
	data->sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(data->sock == -1) {
		err("Cannot open an internet domain socket for mac resolution: %s",
		    strerror(errno));
		return DCP_SYNC_FILTER_RESULT_ERROR;
	}

	data->redis = connect_redis(db_dynamic);
	if(!data->redis) return 1;

	data->redis_stor = connect_redis(db_storage);
	if(!data->redis_stor) return 1;

	// data->cache = connect_redis(REDIS_HOST, REDIS_PORT, db_runtime);
	// if(!data->cache) return 1;

	// // save the cache connection to runtime db as logger
	// log_redis = data->cache;

	dcplugin_set_user_data(dcplugin, data);
	notice("Dowse plugin initialisation succesfull");
	return 0;
}

int dcplugin_destroy(DCPlugin * const dcplugin) {

	plugin_data_t *data = dcplugin_get_user_data(dcplugin);

	if(data->debug) {
		act("dnscrypt dowse plugin quit");
	}

	free_domainlist(data);
	redisFree(data->redis);
	redisFree(data->redis_stor);
	if(data->cache) redisFree(data->cache);
	close(data->sock);
	free(data);

	return 0;
}

DCPluginSyncFilterResult dcplugin_sync_pre_filter(DCPlugin *dcplugin, DCPluginDNSPacket *dcp_packet) {


	uint8_t* wire;
	size_t wirelen;
	uint16_t packet_id;

	ldns_pkt *packet = NULL;
	ldns_rr_list *question_rr_list;
	ldns_rr  *question_rr;
	ldns_rdf *question_rdf;

	char     question_str[MAX_QUERY];
	int      question_len;

	// to get the source ip
	struct sockaddr_storage *from_sa;
	// socklen_t from_len;

	size_t answer_size = 0;
	uint8_t *outbuf = NULL;

	char rr_to_redirect[2048];
	int party_mode=0;
	int enable_to_browse = 0;

	plugin_data_t *data = dcplugin_get_user_data(dcplugin);


	wire = dcplugin_get_wire_data(dcp_packet);


    wirelen = dcplugin_get_wire_data_len(dcp_packet);


    // enforce max dns query size
	if(wirelen > MAX_DNS) // (RFC 6891)
		return DCP_SYNC_FILTER_RESULT_KILL;

	// TODO: throttling to something like 200 calls per second
	// FUNCTION LIMIT_API_CALL(ip)
	// 	ts = CURRENT_UNIX_TIME()
	// 	keyname = ip+":"+ts
	// 	current = GET(keyname)
	// 	IF current != NULL AND current > 10 THEN
	// 	ERROR "too many requests per second"
	// 	ELSE
	// 	MULTI
	// 	INCR(keyname,1)
	// 	EXPIRE(keyname,10)
	// 	EXEC
	// 	PERFORM_API_CALL()
	// 	END


	// import the wire packet to ldns format
	if (ldns_wire2pkt(&packet, wire, wirelen) != LDNS_STATUS_OK)
		return DCP_SYNC_FILTER_RESULT_KILL;


	packet_id = ldns_pkt_id(packet);

	// debug
	if(data->debug) {
		func("-- packet received with id %u:\n", packet_id);
		ldns_pkt_print(stderr, packet);
		func("-- \n");
	}


	// retrieve the actual question string
	question_rr_list  = ldns_pkt_question(packet);


    question_rr       = ldns_rr_list_rr(question_rr_list, 0U); // first in list)


    question_rdf      = ldns_rr_owner(question_rr);

    {
	    char *tmp;
	    tmp = ldns_rdf2str(question_rdf);
	    if(tmp) {
		    snprintf(question_str, MAX_QUERY, "%s", tmp);
		    free(tmp);
	    }
    }

	question_len      = strlen(question_str);
	// TODO: check if we need to query for more questions here

	if (question_len == 0) {
		ldns_pkt_free(packet);
		return DCP_SYNC_FILTER_RESULT_KILL;
	}

	// debug
	if(data->debug)
		func("%s (%u, last: '%c')\n", question_str, question_len,
		        question_str[question_len-1]);

	data->reverse = 0;
	/////////////////////////////////////
	// resolve reverse ip to domain (PTR)
	///
	if(question_str[question_len-1] == '.') { // .arpa. query?
		if(question_str[question_len-5] == 'a' &&
		   question_str[question_len-4] == 'r' &&
		   question_str[question_len-3] == 'p' &&
		   question_str[question_len-2] == 'a') {
			// TODO: shall we check for .in.addr? (RFC?)
			// this is now considered a reverse call
			func("reverse resolution call: %s", question_str);
			ldns_rdf    *tmpname, *reverse;
			char reverse_str[MAX_QUERY];

			data->reverse = 1;

			// import in ldns dname format
			tmpname = ldns_dname_new_frm_str(question_str);
			if (ldns_rdf_get_type(tmpname) != LDNS_RDF_TYPE_DNAME) {
				ldns_rdf_deep_free(tmpname);
				ldns_pkt_free(packet);
				func("dropped packet: .arpa address is not dname");
				return DCP_SYNC_FILTER_RESULT_KILL;
			}

			// reverse the domain
			reverse = ldns_dname_reverse(tmpname);
			// chop left twice (lazy with ldns, TODO: optimise)
			ldns_rdf_deep_free(tmpname);
			tmpname = ldns_dname_left_chop((const ldns_rdf*)reverse);
			ldns_rdf_deep_free(reverse);
			reverse = ldns_dname_left_chop(tmpname);
			ldns_rdf_deep_free(tmpname);

			snprintf(reverse_str, MAX_QUERY, "%s", ldns_rdf2str(reverse));
			ldns_rdf_deep_free(reverse);
			reverse_str[ strlen(reverse_str)-1]='\0';

			if(data->debug)
				func("reverse: %s\n", reverse_str);

			// resolve locally leased hostnames with a O(1) operation on redis
			data->reply = cmd_redis(data->redis_stor,
			                        "GET dns-reverse-%s",
			                        reverse_str);
			if(data->reply)
				if(data->reply->len) { // it exists, return that
					char tmprr[2048];

					if(data->debug)
						func("found local reverse: %s", data->reply->str);

					snprintf(tmprr, 2048, "%s 0 IN PTR %s",
					         question_str, data->reply->str);
					freeReplyObject(data->reply);
					// render the packet into wire format (outbuf is saved with a memcpy)
					outbuf = answer_to_question(packet_id, question_rr,
					                            tmprr, &answer_size);

					// free all the packet structure here, outbuf is the
					// wire format result
					ldns_pkt_free(packet);


					if(!outbuf) return DCP_SYNC_FILTER_RESULT_KILL;

					dcplugin_set_wire_data(dcp_packet, outbuf, answer_size);

					if(outbuf) LDNS_FREE(outbuf);
					return DCP_SYNC_FILTER_RESULT_DIRECT;
				}

			freeReplyObject(data->reply);

			// check if the ip is part of the LAN, if yes avoid forwarding it and return not-found
			// {
			// 	struct in_addr query_ip4_ia;
			// 	inet_pton(AF_INET, reverse_str, &query_ip4_ia);
			// 	inet_pton(AF_INET, reverse_str, &query_ip4_ia);

			// 	inet_pton(AF_INET, data->netmask_ip4, &data->netmask_ip4_ia);
			// 	inet_pton(AF_INET, data->network_ip4, &data->network_ip4_ia);
			// 	if (
			// 	    (
			// 	     ((int32_t)query_ip4_ia) & ((int32_t)data->netmask_ip4_ia)
			// 	     )
			// 	    ==
			// 	    (
			// 	     ((int32_t)data->network_ip4_ia) & ((int32_t)data->netmask_ip4_ia)
			// 	     )
			// 	     ) {
			// 		// request about LAN. TODO: Return here, don't go further
			// 	}
			// }
		}
	}
	// end of reverse resolution (PTR)
	/////////////////////////////////

	// save the query string in the user plugin_data structure
	strncpy(data->query, question_str, MAX_QUERY);
	data->query_len = strlen(data->query);
	data->query[data->query_len-1] = '\0'; // eliminate terminating dot

	// sockaddr
	from_sa = dcplugin_get_client_address(dcp_packet);
	//	from_len = dcplugin_get_client_address_len(dcp_packet);

	// TODO: check if convenient to resolve also the hostname in this way
	// if( getnameinfo((struct sockaddr *)from_sa, from_len,
	//                 data->from, sizeof(data->from), NULL, 0, 0x0) != 0) {
	// 	getnameinfo((struct sockaddr *)from_sa, from_len,
	// 	            data->from, sizeof(data->from), NULL, 0, NI_NUMERICHOST);

    // TODO: check on return value if error
	data->ip4_addr = ((struct sockaddr_in *)from_sa)->sin_addr;
    data->ip4 = inet_ntoa(data->ip4_addr);
    if(data->ip4 == 0) {
	    error("Query from invalid address, error inet_ntoa(sockaddr_in)");
	    ldns_pkt_free(packet);
	    return DCP_SYNC_FILTER_RESULT_KILL;
    }

    snprintf(data->from, NI_MAXHOST, "%s", data->ip4);

    // publish info to redis channel
    publish_query(data);

    // skip checks if query comes from localhost
    if(strcmp(data->ip4, "127.0.0.1") != 0) {

	    // retrieve mac_address of client and writes it into data->mac
	    if ( ip4_derive_mac(data) != 0) {

		    warn("can't resolv mac address of IP: %s", data->ip4);
		    warn("redirect on captive portal due to ip2mac() internal error");
		    if(data->query && data->ownip4)
			    snprintf(rr_to_redirect, 2048, "%s 0 IN A %s", data->query, data->ownip4);
		    // double free?!
		    // if(data->reply)
			//     freeReplyObject(data->reply);

		    func("return a wire packet immediately");
		    outbuf = answer_to_question(packet_id, question_rr, rr_to_redirect, &answer_size);
		    if (!outbuf) {
			    ldns_pkt_free(packet);
			    return DCP_SYNC_FILTER_RESULT_KILL;
		    }

		    dcplugin_set_wire_data(dcp_packet, outbuf, answer_size);
		    LDNS_FREE(outbuf);
		    ldns_pkt_free(packet);
		    return DCP_SYNC_FILTER_RESULT_DIRECT;

	    }

	    // check if party_mode is on then no need to control authorization to browse
	    data->reply = cmd_redis(data->redis_stor,"GET party_mode");
	    if(data->reply) {
		    if(data->reply->str)
			    if( strncmp(data->reply->str,"yes",3) == 0)
				    party_mode = 1;
		    freeReplyObject(data->reply);
	    }

	    if(!party_mode) {
		    // check if the mac address is authorized
		    data->reply = cmd_redis(data->redis_stor, "HGET thing_%s enable_to_browse", data->mac);
		    if(data->reply) {
			    if(data->reply->str)
				    if( strncmp(data->reply->str, "yes", 3) == 0)
					    enable_to_browse = 1;
			    freeReplyObject(data->reply);
		    }

		    if(!enable_to_browse) {
			    // redirect to dowse if it is not authorized
			    func("redirect on captive portal for ip %s mac %s", data->ip4, data->mac);
			    snprintf(rr_to_redirect, 2048, "%s 0 IN A %s", data->query, data->ownip4);

			    // return a wire packet immediately
			    outbuf = answer_to_question(packet_id, question_rr, rr_to_redirect, &answer_size);
			    if (!outbuf) {
				    ldns_pkt_free(packet);
				    return DCP_SYNC_FILTER_RESULT_KILL;
			    }

			    dcplugin_set_wire_data(dcp_packet, outbuf, answer_size);
			    LDNS_FREE(outbuf);

			    ldns_pkt_free(packet);
			    return DCP_SYNC_FILTER_RESULT_DIRECT;

		    }
	    }
    } // check if not localhost

	// DIRECT ENDPOINT
	// resolve locally leased hostnames with a O(1) operation on redis
	data->reply = cmd_redis(data->redis, "GET dns-lease-%s", data->query);
	if(data->reply) {
		if(data->reply->len) { // it exists, return that
			size_t answer_size = 0;
			uint8_t *outbuf = NULL;
			char tmprr[2048];

			if(data->debug)
				func("local lease found: %s", data->reply->str);

			snprintf(tmprr, 2048, "%s 0 IN A %s", data->query, data->reply->str);
			freeReplyObject(data->reply);

			outbuf = answer_to_question(packet_id, question_rr,
			                            tmprr, &answer_size);

			if(!outbuf) {
				ldns_pkt_free(packet);
				return DCP_SYNC_FILTER_RESULT_KILL;
			}

			dcplugin_set_wire_data(dcp_packet, outbuf, answer_size);

			if(outbuf) LDNS_FREE(outbuf);
			ldns_pkt_free(packet);
			return DCP_SYNC_FILTER_RESULT_DIRECT;
		}
		////////////////
		freeReplyObject(data->reply);
	}

	if(data->cache) {
		// check if the answer is cached (the key is the domain string)
		data->reply = cmd_redis(data->cache, "GET dns-cache-%s", data->query);
		if(data->reply)
			if(data->reply->len) { // it exists in cache, return that

				if(data->debug)
					func("found in cache wire packet of %u bytes", data->reply->len);


				// a bit dangerous, but veeery fast: working directly on the wire packet
				// copy message ID (first 16 bits)
				data->reply->str[0] = wire[0];
				data->reply->str[1] = wire[1];

				dcplugin_set_wire_data(dcp_packet, data->reply->str, data->reply->len);
				freeReplyObject(data->reply);

				ldns_pkt_free(packet);
				return DCP_SYNC_FILTER_RESULT_DIRECT;
			}
	}

	// if(from_sa->ss_family == AF_PACKET) { // if contains mac address
	//  char *p = ((struct sockaddr_ll*) from_sa)->sll_addr;
	//  snprintf(data->mac, 32, "%02x:%02x:%02x:%02x:%02x:%02x",
	//           p[0], p[1], p[2], p[3], p[4], p[5]);
	//  // this is here as a pro-memoria, since we never get AF_P

	if(data->offline) {
		size_t answer_size = 0;
		uint8_t *outbuf = NULL;
		char tmprr[2048];
		if(data->reverse)
			snprintf(tmprr, 2048, "%s 0 IN PTR localhost", question_str);
		else
			snprintf(tmprr, 2048, "%s 0 IN A 127.0.0.1", data->query);

		outbuf = answer_to_question(packet_id, question_rr,
		                            tmprr, &answer_size);
		if(!outbuf)
			ldns_pkt_free(packet);
			return DCP_SYNC_FILTER_RESULT_KILL;

		dcplugin_set_wire_data(dcp_packet, outbuf, answer_size);

		if(outbuf) LDNS_FREE(outbuf);
		ldns_pkt_free(packet);
		return DCP_SYNC_FILTER_RESULT_DIRECT;
	}


	dcplugin_set_user_data(dcplugin, data);
	if(data->debug)
		func("%s (forwarding to dnscrypt)", data->query);
	ldns_pkt_free(packet);
	return DCP_SYNC_FILTER_RESULT_OK;
	// return DCP_SYNC_FILTER_RESULT_OK;
}

DCPluginSyncFilterResult dcplugin_sync_post_filter(DCPlugin *dcplugin, DCPluginDNSPacket *dcp_packet) {

	return DCP_SYNC_FILTER_RESULT_OK;

#if 0

	plugin_data_t *data;
	uint8_t *wire;
	size_t wirelen;

	ldns_pkt *packet = NULL;
	ldns_rr_list *answers;
	ldns_rr      *answer;
	size_t        answers_count;


	data = dcplugin_get_user_data(dcplugin);
	wire = dcplugin_get_wire_data(dcp_packet);
	wirelen = dcplugin_get_wire_data_len(dcp_packet);

	if (ldns_wire2pkt(&packet, wire, wirelen) != LDNS_STATUS_OK)
		return DCP_SYNC_FILTER_RESULT_KILL;

	// just cache if return packet contains an answer
	answers = ldns_pkt_answer(packet);
	answers_count = ldns_rr_list_rr_count(answers);
	if(answers_count && data->cache) {

		if(data->debug) {
			func("-- caching reply for: %s", data->query);
			ldns_pkt_print(stderr, packet);
			func("-- \n");
		}

		// check if the query is cached
		func("SETEX dns-cache-%s %u (wire of %u bytes)",
		     data->query, CACHE_EXPIRY, wirelen);
		data->reply = redisCommand(data->cache, "SETEX dns-cache-%s %u %b",
		                           data->query, CACHE_EXPIRY, wire, wirelen);
		okredis(data->cache, data->reply);
	}
	ldns_pkt_free(packet);
	return DCP_SYNC_FILTER_RESULT_OK;


	// test print out ip from packet
	{
		ldns_rr_list *answers;
		ldns_rr      *answer;
		char         *answer_str;
		ldns_rr_type  type;
		size_t        answers_count;
		size_t        i;

		answers = ldns_pkt_answer(packet);
		answers_count = ldns_rr_list_rr_count(answers);
		for (i = (size_t) 0U; i < answers_count; i++) {
			answer = ldns_rr_list_rr(answers, i);
			type = ldns_rr_get_type(answer);
			if (type != LDNS_RR_TYPE_A && type != LDNS_RR_TYPE_AAAA) {
				continue;
			}
			if ((answer_str = ldns_rdf2str(ldns_rr_a_address(answer))) == NULL) {
				return DCP_SYNC_FILTER_RESULT_KILL;
			}
			func("ip: %s", answer_str);
			free(answer_str);
		}
	}
#endif


}


uint8_t *answer_to_question(uint16_t pktid, ldns_rr *question_rr, char *answer, size_t *asize) {
	ldns_pkt *answer_pkt = NULL;
	ldns_status status;
	ldns_rr_list *answer_qr;
	ldns_rr_list *answer_an;
	ldns_rr *answer_an_rr;
	uint8_t *outbuf = NULL;

	// clone the question_rr in answer_qr
	answer_qr = ldns_rr_list_new();
	ldns_rr_list_push_rr(answer_qr, ldns_rr_clone(question_rr));

	// create the answer_an rr_list
	answer_an = ldns_rr_list_new();
	// answer_an_rdf = ldns_rdf_new_frm_str // makes a copy of data buffer
	//  (LDNS_RDF_TYPE_A, data->reply->str);

	// TODO: optimise by avoiding the string parsing and creating the rdf manually
	//  i.e.    ldns_rr_set_rdf(answer_an_rr, answer_an_rdf, 0U);
	// this may need the creation of more functions rather than remove this one
	ldns_rr_new_frm_str(&answer_an_rr, answer, 0, NULL, NULL);

	// after this push will free both with free(answer_an)
	ldns_rr_list_push_rr(answer_an, answer_an_rr);

	// create the packet and empty fields
	answer_pkt = ldns_pkt_new();

	ldns_pkt_set_qr(answer_pkt, 1);
	ldns_pkt_set_aa(answer_pkt, 1);
	ldns_pkt_set_ad(answer_pkt, 0);

	ldns_pkt_set_id(answer_pkt, pktid);

	ldns_pkt_push_rr_list(answer_pkt, LDNS_SECTION_QUESTION,   answer_qr);
	ldns_pkt_push_rr_list(answer_pkt, LDNS_SECTION_ANSWER,     answer_an);

	// render the packet into wire format (outbuf is saved with a memcpy)
	status = ldns_pkt2wire(&outbuf, answer_pkt, asize);

	// free all the packet structure here, outbuf is the wire format result
	ldns_pkt_free(answer_pkt);
	// ldns_rr_free(answer_an_rr);
	ldns_rr_list_free(answer_an);
	ldns_rr_list_free(answer_qr);

	if (status != LDNS_STATUS_OK) {
		err("Error in answer_to_question : %s",
		        ldns_get_errorstr_by_id(status));
		if(outbuf) LDNS_FREE(outbuf);
		outbuf = NULL; // NULL on error
	}

	return outbuf;

}


/* a debug tool */
void print_data_redis( redisContext *redis,char*prefix) {
    #define PRINT_POINTER(p) {\
    FILE *fp=fopen("log.txt","a+");\
    fprintf(fp,"%s %s %p\n",prefix,#p,p);\
    fclose(fp);\
    }
    PRINT_POINTER(redis);
    PRINT_POINTER(redis->obuf);
    PRINT_POINTER(redis->reader);
    PRINT_POINTER(redis->reader->fn);
    PRINT_POINTER(redis->reader->rstack);
}


int publish_query(plugin_data_t *data) {
	char *extracted;
	int val = 1;
	int res;
	char *sval;

	time_t epoch_t;
	char outnew[MAX_OUTPUT];

	// domain hit count
	extracted = extract_domain(data);
	data->reply = cmd_redis(data->redis, "INCR dns-query-%s", extracted);
	if(data->reply) {
		val = data->reply->integer;
		freeReplyObject(data->reply);
	}

	data->reply = cmd_redis(data->redis, "EXPIRE dns-query-%s %u", extracted, DNS_HIT_EXPIRE); // DNS_HIT_EXPIRE
	if(data->reply) freeReplyObject(data->reply);

	// timestamp
	time(&epoch_t);

	// retrieve thing's name from redis
	data->reply = cmd_redis(data->redis_stor, "HGET thing_%s name", data->mac);
	if(data->reply) {
		if(data->reply->str) { // we have the name
			// compose the path of the detected query
			snprintf(outnew, MAX_OUTPUT,
			         "DNS,%s,%d,%lu,%s,%s",
			         data->reply->str, val,
			         epoch_t, extracted, data->tld);
		} else {
			snprintf(outnew, MAX_OUTPUT,
			         "DNS,%s,%d,%lu,%s,%s",
			         data->from, val,
			         epoch_t, extracted, data->tld);
		}
		freeReplyObject(data->reply);
	}

	// add domainlist group if found
	if(data->listpath) {
		res = hashmap_get(data->domainlist, extracted, (void**)(&sval));
		if(res==MAP_OK) {// add domain group
			strncat(outnew,",",2);
			strncat(outnew,sval,MAX_OUTPUT-128);
			//snprintf(outnew,MAX_OUTPUT,"%s,%s",outnew,sval);
		}
	}

	data->reply = cmd_redis(data->redis, "PUBLISH dns-query-channel %s", outnew);
	if(data->reply) freeReplyObject(data->reply);



	return 0;
}
