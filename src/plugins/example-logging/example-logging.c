
#include <dnscrypt/plugin.h>

#include <ctype.h>
#include <stdio.h>
#include <time.h>

#ifdef _WIN32
# include <ws2tcpip.h>
#else
# include <sys/socket.h>
# include <arpa/inet.h>
#endif

DCPLUGIN_MAIN(__FILE__);

#ifndef putc_unlocked
# define putc_unlocked(c, stream) putc((c), (stream))
#endif

const char *
dcplugin_description(DCPlugin * const dcplugin)
{
    return "Log client queries";
}

const char *
dcplugin_long_description(DCPlugin * const dcplugin)
{
    return
        "Log client queries\n"
        "\n"
        "This plugin logs the client queries to the standard output (default)\n"
        "or to a file.\n"
        "\n"
        "# dnscrypt-proxy --plugin libdcplugin_example_logging,/var/log/dns.log";
}

int
dcplugin_init(DCPlugin * const dcplugin, int argc, char *argv[])
{
    FILE *fp;

    if (argc != 2U) {
        fp = stdout;
    } else {
        if ((fp = fopen(argv[1], "a")) == NULL) {
            return -1;
        }
    }
    dcplugin_set_user_data(dcplugin, fp);

    return 0;
}

int
dcplugin_destroy(DCPlugin * const dcplugin)
{
    FILE * const fp = dcplugin_get_user_data(dcplugin);

    if (fp != stdout) {
        fclose(fp);
    }
    return 0;
}

static int
string_fprint(FILE * const fp, const unsigned char *str,
              const size_t size, _Bool lower)
{
    int    c;
    size_t i = (size_t) 0U;

    while (i < size) {
        c = (int) str[i++];
        if (!isprint(c)) {
            fprintf(fp, "\\x%02x", (unsigned int) c);
        } else if (c == '\\') {
            if (lower) {
                c = tolower(c);
            }
            putc_unlocked(c, fp);
        }
        putc_unlocked(c, fp);
    }
    return 0;
}

static int
timestamp_fprint(FILE * const fp, _Bool unix_ts)
{
    char now_s[128];

    time_t     now;
    struct tm *tm;

    if (time(&now) == (time_t) -1) {
        putc_unlocked('-', fp);
        return -1;
    }
    if (unix_ts) {
        fprintf(fp, "%lu", (unsigned long) now);
    } else {
        tm = localtime(&now);
        strftime(now_s, sizeof now_s, "%c", tm);
        fprintf(fp, "%s", now_s);
    }
    return 0;
}

static int
ip_fprint(FILE * const fp,
          const struct sockaddr_storage * const client_addr,
          const size_t client_addr_len)
{
    if (client_addr->ss_family == AF_INET) {
        struct sockaddr_in in;
        uint32_t           a;

        memcpy(&in, client_addr, sizeof in);
        a = ntohl(in.sin_addr.s_addr);
        fprintf(fp, "%u.%u.%u.%u",
                (a >> 24) & 0xff, (a >> 16) & 0xff,
                (a >> 8) & 0xff, a  & 0xff);
    } else if (client_addr->ss_family == AF_INET6) {
        struct sockaddr_in6  in6;
        const unsigned char *a;
        int                  i;
        uint16_t             w;
        _Bool                blanks;

        memcpy(&in6, client_addr, sizeof in6);
        a = in6.sin6_addr.s6_addr;
        blanks = (a[0] | a[1]) == 0;
        putc_unlocked('[', fp);
        for (i = 0; i < 16; i += 2) {
            w = ((uint16_t) a[i] << 8) | (uint16_t) a[i + 1];
            if (blanks) {
                if (w == 0U) {
                    continue;
                }
                putc_unlocked(':', fp);
                blanks = 0;
            }
            if (i != 0) {
                putc_unlocked(':', fp);
            }
            if (blanks == 0) {
                fprintf(fp, "%x", (unsigned int) w);
            }
        }
        putc_unlocked(']', fp);
    } else {
        putc_unlocked('-', fp);
    }
    return 0;
}

DCPluginSyncFilterResult
dcplugin_sync_pre_filter(DCPlugin *dcplugin, DCPluginDNSPacket *dcp_packet)
{
    FILE                *fp = dcplugin_get_user_data(dcplugin);
    const unsigned char *wire_data = dcplugin_get_wire_data(dcp_packet);
    size_t               wire_data_len = dcplugin_get_wire_data_len(dcp_packet);
    size_t               i = (size_t) 12U;
    size_t               csize = (size_t) 0U;
    unsigned short       type;
    unsigned char        c;
    _Bool                first = 1;

    if (wire_data_len < 15U || wire_data[4] != 0U || wire_data[5] != 1U) {
        return DCP_SYNC_FILTER_RESULT_ERROR;
    }
    timestamp_fprint(fp, 0);
    putc_unlocked('\t', fp);
    ip_fprint(fp, dcplugin_get_client_address(dcp_packet),
              dcplugin_get_client_address_len(dcp_packet));
    putc_unlocked('\t', fp);
    if (wire_data[i] == 0U) {
        putc_unlocked('.', fp);
    }
    while (i < wire_data_len && (csize = wire_data[i]) != 0U &&
           csize < wire_data_len - i) {
        i++;
        if (first != 0) {
            first = 0;
        } else {
            putc_unlocked('.', fp);
        }
        string_fprint(fp, &wire_data[i], csize, 1);
        i += csize;
    }
    type = 0U;
    if (i < wire_data_len - 2U) {
        type = (wire_data[i + 1U] << 8) + wire_data[i + 2U];
    }
    switch (type) {
    case 0x01:
        fprintf(fp, "\tA\n"); break;
    case 0x02:
        fprintf(fp, "\tNS\n"); break;
    case 0x0f:
        fprintf(fp, "\tMX\n"); break;
    case 0x1c:
        fprintf(fp, "\tAAAA\n"); break;
    default:
        fprintf(fp, "\t0x%02hX\n", type);
    }
    fflush(fp);

    return DCP_SYNC_FILTER_RESULT_OK;
}

DCPluginSyncFilterResult
dcplugin_sync_post_filter(DCPlugin *dcplugin, DCPluginDNSPacket *dcp_packet)
{
    return DCP_SYNC_FILTER_RESULT_OK;
}
