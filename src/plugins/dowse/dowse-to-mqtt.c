/*  Dowse Spring to MQTT via libmosquitto
 *
 *  (c) Copyright 2016 Dyne.org foundation, Amsterdam
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
 * TODO: trap signals for clean quit, catch more redis errors
 * and most importantly establish an internal data structure for dns query
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>

// libmosquitto
#include <mosquitto.h>

#include <dowse.h>

// static char output[MAX_OUTPUT];
static int quit = 0;


redisContext *redis = NULL;
redisReply   *reply = NULL;

struct mosquitto *mosq = NULL;

extern int optind;

void ctrlc(int sig) {
	act("\nQuit.");
    if(redis) redisFree(redis);
    if(mosq) mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    quit = 1;
}

int main(int argc, char **argv) {
    // settings for
  char *channel_list;   int len_channel_list;

  #define DNS_QUERY_CHANNEL "dns-query-channel"
  len_channel_list=strlen(DNS_QUERY_CHANNEL);

  channel_list=(char*) malloc(len_channel_list+1);

  snprintf(channel_list,len_channel_list+1,DNS_QUERY_CHANNEL);

  char id[25];
    const char *host;
    int port    = 1883;
    int keepalive = 60;

    // char *dns, *ip, *action, *epoch, *domain, *tld, *group;
    // long long int hits;

    int mres;

    int opt;
    char pidfile[MAX_OUTPUT];
    // pid_t pid;

    pidfile[0]=0x0;

    while((opt = getopt(argc, argv, "p:c:")) != -1) {
	    switch(opt) {
		    case 'p':
			    snprintf(pidfile,MAX_OUTPUT,"%s",optarg);
			    break;
		    case 'c':
		      len_channel_list+=(strlen(optarg)+1); /* +1 for the blank */
		      char *new_list=malloc(len_channel_list+1);
		      snprintf(new_list,len_channel_list+1,"%s %s",channel_list,optarg);

		      free(channel_list);
		      channel_list=new_list;

		      break;
	    }
    }



    if(argv[optind] == NULL) {
	    err("usage: %s [-p pidfile] host [port]", argv[0]);
	    exit(1);
    }
    host = argv[optind];
    // TODO: get port from argv[2] when present

    signal(SIGINT, ctrlc);

    redis = connect_redis(db_dynamic);

    mosquitto_lib_init();
    srand(time(NULL));
    snprintf(id,24,"%u",rand());
    mosq = mosquitto_new(id, true, NULL);


    // libmosq_EXPORT int mosquitto_connect(   struct  mosquitto   *   mosq,
    //                                         const   char    *   host,
    //                                         int         port,
    //                                         int         keepalive   )

    mres = mosquitto_connect( mosq, host, port, keepalive);
    if(mres != MOSQ_ERR_SUCCESS) {
	    err("can't connect to mosquitto server %s on port %u",host, port);
	    mosquitto_destroy(mosq);
	    mosquitto_lib_cleanup();
	    return(1);
    } else {
	    notice("connected to mosquitto server %s on port %u", host, port);
    }

    reply = cmd_redis(redis,"SUBSCRIBE %s ",channel_list);
    freeReplyObject(reply);

    if(pidfile[0]) {
	    pid_t pid;
	    FILE *fpid;
	    pid = getpid();
	    fpid = fopen(pidfile,"w");
	    if(!fpid) perror("writing pidfile");
	    else {
		    fprintf(fpid,"%u\n",pid);
		    fclose(fpid);
	    }
    }

    while(redisGetReply(redis,(void**)&reply) == REDIS_OK) {
        if(quit) break;

        // mosquitto_publish(  struct  mosquitto   *mosq,
        //                     int                 *mid,
        //                     const   char        *topic,
        //                     int                 payloadlen,
        //                     const   void        *payload,
        //                     int                 qos,
        //                     bool                retain  )

        // Parameters
	    //     mosq	a valid mosquitto instance.
	    //     mid	pointer to an int.  If not NULL, the function will set this to the message id of this particular message.
		//     payloadlen	the size of the payload (bytes).  Valid values are between 0 and 268,435,455.
		// 	   payload	pointer to the data to send.  If payloadlen > 0 this must be a valid memory location.
		// 	   qos	integer value 0, 1 or 2 indicating the Quality of Service to be used for the message.
		// 	   retain	set to true to make the message retained.

        mres = mosquitto_loop(mosq, -1, 1);
        if(mres) mosquitto_reconnect(mosq);

	if (strcmp(reply->element[0]->str,"message")!=0) {
	  /* only the "message" should be forwarded on mqtt to the "subscribe" */
	  continue;
	}
	/* every message is forwarded on the mqtt using the same name channel (reply->element[1]->str)
	 */
        mres = mosquitto_publish(mosq, NULL, reply->element[1]->str,
                                 reply->element[2]->len, reply->element[2]->str,
                                 1 /*qos*/, false);
	/*
	for (int i=0;i<reply->elements;i++){
	  fprintf(stderr," %d %s\n",i,reply->element[i]->str);
	}
	fprintf(stderr,"\n------------\n");
	*/

        // Returns
        //  MOSQ_ERR_SUCCESS	on success.
        //  MOSQ_ERR_INVAL	if the input parameters were invalid.
        // 	MOSQ_ERR_NOMEM	if an out of memory condition occurred.
        // 	MOSQ_ERR_NO_CONN	if the client isn’t connected to a broker.
        // 	MOSQ_ERR_PROTOCOL	if there is a protocol error communicating with the broker.
        // 	MOSQ_ERR_PAYLOAD_SIZE	if payloadlen is too large.
        if(mres != MOSQ_ERR_SUCCESS)
	        err("error publishing message to mosquitto");

        mosquitto_loop(mosq, -1, 1);

        // TODO: check returned value


        freeReplyObject(reply);
    }
    sleep(1);

    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    exit(0);
}
