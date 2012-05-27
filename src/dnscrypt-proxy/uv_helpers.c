
#include <config.h>
#include <sys/types.h>
#ifdef _WIN32
# if !defined(WINVER) || WINVER < 0x0501
#  undef WINVER
#  define WINVER 0x0501
# endif
# include <winsock2.h>
# include <ws2tcpip.h>
#else
# include <sys/socket.h>
# include <netinet/in.h>
# include <netdb.h>
#endif

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
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

static void
ares_addr_nodes_free(struct ares_addr_node * const addr_nodes)
{
    struct ares_addr_node *addr_node = addr_nodes;
    struct ares_addr_node *addr_node_tmp;

    while (addr_node != NULL) {
        addr_node_tmp = addr_node->next;
        free(addr_node);
        addr_node = addr_node_tmp;
    }
}

int
ares_set_servers_any(ares_channel channel, const ares_ss_node * const ss_nodes)
{
    struct ares_addr_node *addr_node;
    struct ares_addr_node *addr_nodes = NULL;
    struct ares_addr_node *addr_nodes_last = NULL;
    const ares_ss_node    *ss_node = ss_nodes;
    int                    ares_ret;

    while (ss_node != NULL) {
        if ((addr_node = calloc((size_t) 1U, sizeof *addr_node)) == NULL) {
            ares_addr_nodes_free(addr_nodes);
            return ARES_ENOMEM;
        }
        addr_node->family = STORAGE_FAMILY(*(ss_node->ss));
        switch (addr_node->family) {
        case AF_INET:
            addr_node->addr.addr4 =
                ((const struct sockaddr_in *) ss_node->ss)->sin_addr;
            break;
        case AF_INET6:
            memcpy(&addr_node->addr.addr6,
                   &((const struct sockaddr_in6 *) ss_node->ss)->sin6_addr,
                   sizeof addr_node->addr.addr6);
            break;
        default:
            ares_addr_nodes_free(addr_nodes);
            return ARES_EBADFAMILY;
        }
        addr_node->next = NULL;
        if (addr_nodes == NULL) {
            addr_nodes = addr_node;
        }
        if (addr_nodes_last != NULL) {
            addr_nodes_last->next = addr_node;
        }
        addr_nodes_last = addr_node;
        ss_node = ss_node->next;
    }
    ares_ret = ares_set_servers(channel, addr_nodes);
    ares_addr_nodes_free(addr_nodes);

    return ares_ret;
}
