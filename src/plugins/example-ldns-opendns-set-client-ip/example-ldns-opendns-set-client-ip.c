
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <dnscrypt/plugin.h>
#include <ldns/ldns.h>

#define EDNS_HEADER    "4f44" "000b" "4f444e5300" "00" "10"
#define EDNS_CLIENT_IP "7f000001"

const char *
dcplugin_description(DCPlugin * const dcplugin)
{
    return "Apply the OpenDNS settings defined for a specific IP address";
}

const char *
dcplugin_long_description(DCPlugin * const dcplugin)
{
    return
        "If you defined a policy for your network, this plugin allows\n"
        "using this policy no matter where you are, and without interfering\n"
        "with the networks you are roaming on.\n"
        "\n"
        "The IP address must be a hex-encoded IPv4 address.\n"
        "\n"
        "Usage:\n"
        "\n"
        "# dnscrypt-proxy --plugin \\\n"
        "  .../libdcplugin_example_ldns_opendns_set_client_ip.la,7f000001";
}

int
dcplugin_init(DCPlugin * const dcplugin, int argc, char *argv[])
{
    char   *edns_hex;
    size_t  edns_hex_size = sizeof EDNS_HEADER EDNS_CLIENT_IP;

    edns_hex = malloc(sizeof EDNS_HEADER EDNS_CLIENT_IP);
    dcplugin_set_user_data(dcplugin, edns_hex);
    if (edns_hex == NULL) {
        return -1;
    }
    memcpy(edns_hex, EDNS_HEADER EDNS_CLIENT_IP, edns_hex_size);
    assert(sizeof EDNS_CLIENT_IP - 1U == (size_t) 8U);
    if (argc > 1 && strlen(argv[1]) == (size_t) 8U) {
        memcpy(edns_hex + sizeof EDNS_HEADER - (size_t) 1U,
               argv[1], sizeof EDNS_CLIENT_IP);
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
