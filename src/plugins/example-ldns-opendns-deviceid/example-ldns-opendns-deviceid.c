
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <dnscrypt/plugin.h>
#include <ldns/ldns.h>

#define EDNS_HEADER "0004000f4f70656e444e53"
#define EDNS_DEV_ID "cafebabedeadbeef"

const char *
dcplugin_description(DCPlugin * const dcplugin)
{
    return "Add an OpenDNS device identifier to outgoing queries";
}

const char *
dcplugin_long_description(DCPlugin * const dcplugin)
{
    return
        "Some routers provide advanced support for OpenDNS, and allow using\n"
        "a specific set of settings no matter what their IP address is.\n"
        "\n"
        "Since they don't support dnscrypt, outgoing encrypted queries can\n"
        "not be rewritten by these routers, and OpenDNS will not recognize\n"
        "the router settings.\n"
        "\n"
        "This plugin adds a device identifier to outgoing packets, so that\n"
        "router settings can be used, even when using dnscrypt.\n"
        "\n"
        "When connected without dnscrypt through such a router, the\n"
        "following command will print your device id:\n"
        "\n"
        "$ dig TXT debug.opendns.com\n"
        "\n"
        "Just pass this device id as an argument to this plugin:\n"
        "\n"
        "# dnscrypt-proxy --plugin \\\n"
        "  .../libdcplugin_example_ldns_opendns_deviceid.la,XXXXXXXXXXXXXXXX";
}

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
