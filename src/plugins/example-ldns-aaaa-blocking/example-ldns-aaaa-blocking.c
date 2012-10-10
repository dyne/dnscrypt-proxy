
#include <dnscrypt/plugin.h>
#include <ldns/ldns.h>

DCPLUGIN_MAIN(__FILE__);

const char *
dcplugin_description(DCPlugin * const dcplugin)
{
    return "Directly return an empty response to AAAA queries";
}

int
dcplugin_init(DCPlugin * const dcplugin, int argc, char *argv[])
{
    (void) dcplugin;
    (void) argc;
    (void) argv;

    return 0;
}

DCPluginSyncFilterResult
dcplugin_sync_pre_filter(DCPlugin *dcplugin, DCPluginDNSPacket *dcp_packet)
{
    ldns_pkt                 *packet;
    ldns_rr_list             *questions;
    DCPluginSyncFilterResult  result = DCP_SYNC_FILTER_RESULT_OK;

    ldns_wire2pkt(&packet, dcplugin_get_wire_data(dcp_packet),
                  dcplugin_get_wire_data_len(dcp_packet));    
    if (packet == NULL) {
        return DCP_SYNC_FILTER_RESULT_ERROR;
    }
    questions = ldns_pkt_question(packet);
    if (ldns_rr_list_rr_count(questions) == (size_t) 1U &&
        ldns_rr_get_type(ldns_rr_list_rr(questions,
                                         (size_t) 0U)) == LDNS_RR_TYPE_AAAA) {
        result = DCP_SYNC_FILTER_RESULT_DIRECT;
    }
    ldns_pkt_free(packet);

    return result;
}
