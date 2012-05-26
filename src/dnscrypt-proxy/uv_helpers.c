
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
uv_addr_any(struct sockaddr_storage * const sa, const char * const host,
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
        if (ai->ai_addrlen <= (socklen_t) sizeof *sa &&
            (ai->ai_family == AF_INET || ai->ai_family == AF_INET6)) {
            memcpy(sa, ai->ai_addr, ai->ai_addrlen);
            ret = 0;
            break;
        }
        ai_cur = ai->ai_next;
    }
    freeaddrinfo(ai);

    return ret;
}

in_port_t *
storage_port4(struct sockaddr_storage * const ss)
{
    struct sockaddr_in * const si = (struct sockaddr_in *) ss;

    return &si->sin_port;
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
        return storage_port4(ss);
    case AF_INET6:
        return storage_port6(ss);
    }
    errno = EINVAL;

    return NULL;
}

int
uv_udp_bind_any(uv_udp_t *handle, struct sockaddr_storage ss, unsigned flags)
{
    switch (STORAGE_FAMILY(ss)) {
    case AF_INET: {
        struct sockaddr_in si;
        COMPILER_ASSERT(sizeof ss >= sizeof si);
        memcpy(&si, &ss, sizeof si);
        return uv_udp_bind(handle, si, flags);
    }
    case AF_INET6: {
        struct sockaddr_in6 si6;
        COMPILER_ASSERT(sizeof ss >= sizeof si6);
        memcpy(&si6, &ss, sizeof si6);
        return uv_udp_bind6(handle, si6, flags);
    }
    }
    errno = EINVAL;

    return -1;
}

int
uv_tcp_bind_any(uv_tcp_t *handle, struct sockaddr_storage ss)
{
    switch (STORAGE_FAMILY(ss)) {
    case AF_INET: {
        struct sockaddr_in si;
        COMPILER_ASSERT(sizeof ss >= sizeof si);
        memcpy(&si, &ss, sizeof si);
        return uv_tcp_bind(handle, si);
    }
    case AF_INET6: {
        struct sockaddr_in6 si6;
        COMPILER_ASSERT(sizeof ss >= sizeof si6);
        memcpy(&si6, &ss, sizeof si6);
        return uv_tcp_bind6(handle, si6);
    }
    }
    errno = EINVAL;

    return -1;
}
