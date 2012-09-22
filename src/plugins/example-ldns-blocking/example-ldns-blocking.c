
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <dnscrypt/plugin.h>
#include <ldns/ldns.h>

typedef struct StrList_ {
    struct StrList_ *next;
    char            *str;
} StrList;

typedef struct Blocking_ {
    StrList *domains;
    StrList *ips;
} Blocking;

static struct option getopt_long_options[] = {
    { "domains", 1, NULL, 'd' },
    { "ips", 1, NULL, 'i' },
    { NULL, 0, NULL, 0 }
};
static const char *getopt_options = "di";

static void
str_list_free(StrList * const str_list)
{
    StrList *next;
    StrList *scanned = str_list;

    while (scanned != NULL) {
        next = scanned->next;
        free(scanned->str);
        scanned->next = NULL;
        scanned->str = NULL;
        free(scanned);
        scanned = next;
    }
}

static StrList *
parse_str_list(const char * const file)
{
    char     line[300U];
    FILE    *fp;
    char    *ptr;
    StrList *str_list = NULL;
    StrList *str_list_item;
    StrList *str_list_last = NULL;

    if ((fp = fopen(file, "r")) == NULL) {
        return NULL;
    }
    while (fgets(line, (int) sizeof line, fp) != NULL) {
        while ((ptr = strchr(line, '\n')) != NULL ||
               (ptr = strchr(line, '\r')) != NULL) {
            *ptr = 0;
        }
        if (*line == 0 || *line == '#') {
            continue;
        }
        if ((str_list_item = calloc(1U, sizeof *str_list_item)) == NULL ||
            (str_list_item->str = strdup(line)) == NULL) {
            break;
        }
        str_list_item->next = NULL;
        *(str_list == NULL ? &str_list : &str_list_last->next) = str_list_item;
        str_list_last = str_list_item;
    }
    if (!feof(fp)) {
        str_list_free(str_list);
        str_list = NULL;
    }
    fclose(fp);

    return str_list;
}

const char *
dcplugin_description(DCPlugin * const dcplugin)
{
    return "Block specific domains and IP addresses";
}

const char *
dcplugin_long_description(DCPlugin * const dcplugin)
{
    return
        "This plugin returns a REFUSED response if the query name is in a\n"
        "list of blacklisted names, or if at least one of the returned IP\n"
        "addresses happens to be in a list of blacklisted IPs.\n"
        "\n"
        "Recognized switches are:\n"
        "--domains=<file>\n"
        "--ips=<file>\n"
        "\n"
        "A file should list one entry per line.\n"
        "\n"
        "IPv4 and IPv6 addresses are supported.\n"
        "For names, wildcards (*) are also supported.\n"
        "\n"
        "# dnscrypt-proxy --plugin \\\n"
        "  libdcplugin_example,--ips=/etc/blk-ips,--domains=/etc/blk-names\n";
}

int
dcplugin_init(DCPlugin * const dcplugin, int argc, char *argv[])
{
    Blocking *blocking;
    int       opt_flag;
    int       option_index = 0;

    if ((blocking = calloc((size_t) 1U, sizeof *blocking)) == NULL) {
        return -1;
    }
    dcplugin_set_user_data(dcplugin, blocking);
    if (blocking == NULL) {
        return -1;
    }
    blocking->domains = blocking->ips = NULL;
    optind = 0;
#ifdef _OPTRESET
    optreset = 1;
#endif
    while ((opt_flag = getopt_long(argc, argv,
                                   getopt_options, getopt_long_options,
                                   &option_index)) != -1) {
        switch (opt_flag) {
        case 'd':
            if ((blocking->domains = parse_str_list(optarg)) == NULL) {
                return -1;
            }
            break;
        case 'i':
            if ((blocking->ips = parse_str_list(optarg)) == NULL) {
                return -1;
            }
            break;
        default:
            return -1;
        }
    }
    if (blocking->domains == NULL && blocking->ips == NULL) {
        return -1;
    }
    return 0;
}

int
dcplugin_destroy(DCPlugin * const dcplugin)
{
    Blocking *blocking = dcplugin_get_user_data(dcplugin);

    if (blocking == NULL) {
        return 0;
    }
    str_list_free(blocking->domains);
    blocking->domains = NULL;
    str_list_free(blocking->ips);
    blocking->ips = NULL;
    free(blocking);

    return 0;
}

DCPluginSyncFilterResult
dcplugin_sync_post_filter(DCPlugin *dcplugin, DCPluginDNSPacket *dcp_packet)
{
    Blocking *blocking = dcplugin_get_user_data(dcplugin);
    ldns_pkt *packet;

    if (blocking->domains == NULL && blocking->ips == NULL) {
        return DCP_SYNC_FILTER_RESULT_OK;
    }
    ldns_wire2pkt(&packet, dcplugin_get_wire_data(dcp_packet),
                  dcplugin_get_wire_data_len(dcp_packet));
    if (packet == NULL) {
        return DCP_SYNC_FILTER_RESULT_ERROR;
    }
    if (blocking->domains != NULL) {
        StrList  *scanned;
        ldns_rdf *owner;
        ldns_rdf *scanned_rdf;
        ldns_rr  *question;

        scanned = blocking->domains;
        question = ldns_rr_list_rr(ldns_pkt_question(packet), 0U);
        owner = ldns_rr_owner(question);
        do {
            if ((scanned_rdf = ldns_dname_new_frm_str(scanned->str)) == NULL) {
                return DCP_SYNC_FILTER_RESULT_FATAL;
            }
            if (ldns_dname_match_wildcard(owner, scanned_rdf) != 0) {
                LDNS_RCODE_SET(dcplugin_get_wire_data(dcp_packet),
                               LDNS_RCODE_REFUSED);
                ldns_rdf_free(scanned_rdf);
                break;
            }
            ldns_rdf_free(scanned_rdf);
        } while ((scanned = scanned->next) != NULL);
    }

    if (blocking->ips != NULL) {
        StrList      *scanned;
        ldns_rr_list *answers;
        ldns_rr      *answer;
        char         *answer_str;
        ldns_rr_type  type;
        size_t        answers_count;
        size_t        i;

        answers = ldns_pkt_answer(packet);
        answers_count = ldns_rr_list_rr_count(answers);
        for (i = (size_t) 0U; i < answers_count; i++) {
            answer = ldns_rr_list_rr(answers, i);
            type = ldns_rr_get_type(answer);
            if (type != LDNS_RR_TYPE_A && type != LDNS_RR_TYPE_AAAA) {
                continue;
            }
            if ((answer_str = ldns_rdf2str(ldns_rr_a_address(answer))) == NULL) {
                return DCP_SYNC_FILTER_RESULT_FATAL;
            }
            scanned = blocking->ips;
            do {
                if (strcasecmp(scanned->str, answer_str) == 0) {
                    LDNS_RCODE_SET(dcplugin_get_wire_data(dcp_packet),
                                   LDNS_RCODE_REFUSED);
                    break;
                }
            } while ((scanned = scanned->next) != NULL);
            free(answer_str);
        }
    }

    ldns_pkt_free(packet);

    return DCP_SYNC_FILTER_RESULT_OK;
}
