#ifndef __EXAMPLE_CACHE_H__
#define __EXAMPLE_CACHE_H__ 1

#define DEFAULT_CACHE_ENTRIES_MAX 50
#define DEFAULT_MIN_TTL 60
#define MAX_TTL 86400

#define DNS_MAX_HOSTNAME_LEN 256U

#include <stdint.h>
#include <stdlib.h>

typedef struct CacheEntry_ {
    struct CacheEntry_ *next;
    uint8_t            *response;
    uint16_t            response_len;
    char                qname[DNS_MAX_HOSTNAME_LEN];
    uint16_t            qtype;
    time_t              deadline;
} CacheEntry;

typedef struct Cache_ {
    CacheEntry *cache_entries;
    CacheEntry *cache_entries_l2;
    size_t      cache_entries_max;
    time_t      now;
    time_t      min_ttl;
} Cache;

#ifndef putc_unlocked
# define putc_unlocked(c, stream) putc((c), (stream))
#endif

#endif
