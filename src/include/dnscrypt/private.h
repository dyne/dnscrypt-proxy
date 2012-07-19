
#ifndef __DNSCRYPT_PRIVATE_H__
#define __DNSCRYPT_PRIVATE_H__ 1

#include <stdint.h>

struct DCPlugin_ {
    void *user_data;
};

struct DCPluginDNSPacket_ {
    struct sockaddr_storage *client_sockaddr;
    uint8_t                 *dns_packet;
    size_t                  *dns_packet_len_p;
    size_t                   client_sockaddr_len_s;
    size_t                   dns_packet_max_len;
};

#endif
