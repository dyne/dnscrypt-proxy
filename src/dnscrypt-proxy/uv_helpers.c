
#include <config.h>
#include <sys/types.h>
#ifdef _WIN32
# include <winsock2.h>
#else
# include <sys/socket.h>
# include <netinet/in.h>
#endif

#include <errno.h>
#include <netdb.h>
#include <stdint.h>
#include <string.h>

#include "utils.h"
#include "uv.h"
#include "uv_helpers.h"

int
uv_addr_any(struct sockaddr_storage * const ss, const char * const host,
            const char * const port, const int socktype, const int protocol,
            const _Bool passive)
{
    struct addrinfo  hints;
    struct addrinfo *ai;
    struct addrinfo *ai_cur;
    int              gai_err;
    int              ret = -1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = passive ? AI_PASSIVE : 0;
    hints.ai_socktype = socktype;
    hints.ai_protocol = protocol;
    gai_err = getaddrinfo(host, port, &hints, &ai);
    if (gai_err != 0) {
        return -1;
    }
    ai_cur = ai;
    while (ai_cur != NULL) {
        if (ai->ai_addrlen <= (socklen_t) sizeof *ss &&
            (ai->ai_family == AF_INET || ai->ai_family == AF_INET6)) {
            memcpy(ss, ai->ai_addr, ai->ai_addrlen);
            ret = 0;
            break;
        }
        ai_cur = ai->ai_next;
    }
    freeaddrinfo(ai);

    return ret;
}

in_port_t *
storage_port6(struct sockaddr_storage * const ss)
{
    struct sockaddr_in6 * const si = (struct sockaddr_in6 *) ss;

    return &si->sin6_port;
}

in_port_t *
storage_port_any(struct sockaddr_storage * const ss)
{
    switch (STORAGE_FAMILY(*ss)) {
    case AF_INET:
        return & (((struct sockaddr_in *) (void *) ss)->sin_port);
    case AF_INET6:
        return & (((struct sockaddr_in6 *) (void *) ss)->sin6_port);
    }
    errno = EINVAL;

    return NULL;
}

int
uv_udp_bind_any(uv_udp_t *handle, struct sockaddr_storage * const ss,
                unsigned flags)
{
    switch (STORAGE_FAMILY(*ss)) {
    case AF_INET: {
        return uv_udp_bind(handle,
                           * (struct sockaddr_in *) (void *) ss, flags);
    }
    case AF_INET6: {
        return uv_udp_bind6(handle,
                            * (struct sockaddr_in6 *) (void *) ss, flags);
    }
    }
    errno = EINVAL;

    return -1;
}

int
uv_tcp_bind_any(uv_tcp_t *handle, struct sockaddr_storage * const ss)
{
    switch (STORAGE_FAMILY(*ss)) {
    case AF_INET: {
        return uv_tcp_bind(handle,
                          * (struct sockaddr_in *) (void *) ss);
    }
    case AF_INET6: {
        return uv_tcp_bind6(handle,
                           * (struct sockaddr_in6 *) (void *) ss);
    }
    }
    errno = EINVAL;

    return -1;
}

int
uv_udp_send_any(uv_udp_send_t *req, uv_udp_t *handle, uv_buf_t bufs[],
                int bufcnt, struct sockaddr_storage * const ss,
                uv_udp_send_cb send_cb)
{
    switch (STORAGE_FAMILY(*ss)) {
    case AF_INET: {
        return uv_udp_send(req, handle, bufs, bufcnt,
                           * (struct sockaddr_in *) (void *) ss, send_cb);
    }
    case AF_INET6: {
        return uv_udp_send6(req, handle, bufs, bufcnt,
                            * (struct sockaddr_in6 *) (void *) ss, send_cb);
    }
    }
    errno = EINVAL;

    return -1;
}

int
uv_tcp_connect_any(uv_connect_t *req, uv_tcp_t *handle,
                   struct sockaddr_storage * const ss, uv_connect_cb cb)
{
    switch (STORAGE_FAMILY(*ss)) {
    case AF_INET: {
        return uv_tcp_connect(req, handle,
                              * (struct sockaddr_in *) (void *) ss, cb);
    }
    case AF_INET6: {
        return uv_tcp_connect6(req, handle,
                               * (struct sockaddr_in6 *) (void *) ss, cb);
    }
    }
    errno = EINVAL;

    return -1;
}
