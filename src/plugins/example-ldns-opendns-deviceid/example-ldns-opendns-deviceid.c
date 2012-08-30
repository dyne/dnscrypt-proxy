
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <dnscrypt/plugin.h>
#include <ldns/ldns.h>

#define EDNS_HEADER "0004000f4f70656e444e53"
#define EDNS_DEV_ID "cafebabedeadbeef"

int
dcplugin_init(DCPlugin * const dcplugin, int argc, char *argv[])
{
    char   *edns_hex;
    size_t  edns_hex_size = sizeof EDNS_HEADER EDNS_DEV_ID;
    
    edns_hex = malloc(sizeof EDNS_HEADER EDNS_DEV_ID);
    dcplugin_set_user_data(dcplugin, edns_hex);
    if (edns_hex == NULL) {
        return -1;
    }
    memcpy(edns_hex, EDNS_HEADER EDNS_DEV_ID, edns_hex_size);
    assert(sizeof EDNS_DEV_ID - 1U == (size_t) 16U);
    if (argc > 1 && strlen(argv[1]) == (size_t) 16U) {
        memcpy(edns_hex + sizeof EDNS_HEADER - (size_t) 1U,
               argv[1], sizeof EDNS_DEV_ID);
    }
    return 0;
}

int
dcplugin_destroy(DCPlugin *dcplugin)
{
    free(dcplugin_get_user_data(dcplugin));    
    
    return 0;
}

DCPluginSyncFilterResult
dcplugin_sync_pre_filter(DCPlugin *dcplugin, DCPluginDNSPacket *dcp_packet)
{
    uint8_t  *new_packet;
    ldns_rdf *edns_data;
    ldns_pkt *packet;
    size_t    new_packet_size;

    ldns_wire2pkt(&packet, dcplugin_get_wire_data(dcp_packet),
                  dcplugin_get_wire_data_len(dcp_packet));

    edns_data = ldns_rdf_new_frm_str(LDNS_RDF_TYPE_HEX,
                                     dcplugin_get_user_data(dcplugin));
    ldns_pkt_set_edns_data(packet, edns_data);

    ldns_pkt2wire(&new_packet, packet, &new_packet_size);
    if (dcplugin_get_wire_data_max_len(dcp_packet) >= new_packet_size) {
        dcplugin_set_wire_data(dcp_packet, new_packet, new_packet_size);
    }

    free(new_packet);
    ldns_pkt_free(packet);

    return DCP_SYNC_FILTER_RESULT_OK;
}
