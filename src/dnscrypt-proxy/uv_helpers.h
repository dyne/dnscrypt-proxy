
#ifndef __UV_HELPERS_H__
#define __UV_HELPERS_H__ 1

#include <sys/types.h>
#ifdef _WIN32
# include <winsock2.h>
#else
# include <sys/socket.h>
# include <netinet/in.h>
#endif

#include <stdint.h>

#ifdef HAVE_SS_LEN
# define STORAGE_LEN(X) ((X).ss_len)
#elif defined(HAVE___SS_LEN)
# define STORAGE_LEN(X) ((X).__ss_len)
#else
# define STORAGE_LEN(X) (STORAGE_FAMILY(X) == AF_INET ? \
                         sizeof(struct sockaddr_in) : \
                         sizeof(struct sockaddr_in6))
#endif

#ifdef HAVE___SS_FAMILY
# define STORAGE_FAMILY(X) ((X).__ss_family)
#else
# define STORAGE_FAMILY(X) ((X).ss_family)
#endif

#define STORAGE_PORT4(X) (*storage_port4(&(X)))
#define STORAGE_PORT6(X) (*storage_port6(&(X)))
#define STORAGE_PORT_ANY(X) (*storage_port_any(&(X)))

int uv_addr_any(struct sockaddr_storage * const sa, const char * const host,
                const char * const port, const int socktype,
                const int protocol, const _Bool passive);

int uv_udp_bind_any(uv_udp_t *handle, struct sockaddr_storage ss,
                    unsigned flags);

int uv_tcp_bind_any(uv_tcp_t *handle, struct sockaddr_storage ss);

in_port_t *storage_port4(struct sockaddr_storage * const ss);
in_port_t *storage_port6(struct sockaddr_storage * const ss);
in_port_t *storage_port_any(struct sockaddr_storage * const ss);

#endif
