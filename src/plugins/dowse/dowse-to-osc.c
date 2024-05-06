/*  Dowse - Open Sound Control listener for DNS query events
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

// liblo
#include <lo/lo.h>

#include <dowse.h>

redisContext *redis = NULL;
redisReply   *reply = NULL;

static int quit = 0;
void ctrlc(int sig) {
    act("\nQuit.");
    if(redis) redisFree(redis);
    quit = 1;
}

int main(int argc, char **argv) {

    int res;
    lo_address osc;
    char *dns, *ip, *action, *epoch, *domain, *tld;
    unsigned int hits;

    if(argv[1] == NULL) {
        err("usage: %s osc.URL (i.e: osc.udp://localhost:666/pd)", argv[0]);
        exit(1);
    }

    osc = lo_address_new_from_url( argv[1] );
    lo_address_set_ttl(osc, 1); // subnet scope

    redis = connect_redis(db_dynamic);

    reply = cmd_redis(redis,"SUBSCRIBE dns-query-channel");
    freeReplyObject(reply);

    signal(SIGINT, ctrlc);

    while(redisGetReply(redis,(void**)&reply) == REDIS_OK) {

        // reusable block to parse redis into variables
        dns = strtok(reply->element[2]->str,",");
        if(!dns) continue;
        ip = strtok(NULL,",");
        if(!ip) continue;
        action = strtok(NULL,",");
        if(!action) continue;
        epoch = strtok(NULL,",");
        if(!epoch) continue;
        domain = strtok(NULL,",");
        if(!domain) continue;
        tld = strtok(NULL,",");
        if(!tld) continue;
        // --

        hits = atoll(action);

        // TODO: use a more refined lo_send with low-latency flags
        res = lo_send(osc, "/dowse/dns", "siss",
                      ip, hits, domain, tld);
        if(res == -1)
            err("OSC send error: %s",lo_address_errstr(osc));
        // just for console debugging
        else
            func("/dowse/dns %s %u %s %s",
                 ip, hits, domain, tld);

        fflush(stderr);

        freeReplyObject(reply);
        if(quit) break;
    }

    lo_address_free(osc);

    exit(0);
}
