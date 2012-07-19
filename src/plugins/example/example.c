
#include <dnscrypt/plugin.h>

int dcplugin_init(DCPlugin * const dcplugin, int argc, char *argv[])
{
    (void) dcplugin;
    (void) argc;
    (void) argv;

    return 0;
}

int dcplugin_destroy(DCPlugin * const dcplugin)
{
    (void) dcplugin;

    return 0;
}

DCPluginSyncFilterResult
dcplugin_sync_pre_filter(DCPlugin *dcplugin, DCPluginDNSPacket *dcp_packet)
{
    (void) dcplugin;

    return 0;
}
