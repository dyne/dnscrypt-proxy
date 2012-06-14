
#ifndef __UDP_REQUEST_H_P__
#define __UDP_REQUEST_H_P__ 1

#include <sys/types.h>

#include <stdint.h>

#include <event2/event.h>

#include "dnscrypt.h"
#include "queue.h"

typedef struct UDPRequestStatus_ {
    _Bool is_dying : 1;
} UDPRequestStatus;

typedef struct UDPRequest_ {
    uint8_t                  client_nonce[crypto_box_HALF_NONCEBYTES];
    TAILQ_ENTRY(UDPRequest_) queue;
    struct sockaddr_storage  client_sockaddr;
    ProxyContext            *proxy_context;
    struct event            *timeout_timer;
    evutil_socket_t          client_proxy_handle;
    ev_socklen_t             client_sockaddr_len;
    UDPRequestStatus         status;
} UDPRequest;

#endif
