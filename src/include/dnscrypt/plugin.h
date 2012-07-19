
#ifndef __DNSCRYPT_PLUGIN_H__
#define __DNSCRYPT_PLUGIN_H__ 1

#include <dnscrypt/private.h>
#include <dnscrypt/version.h>

typedef struct DCPlugin_ DCPlugin;
typedef struct DCPluginDNSPacket_ DCPluginDNSPacket;

typedef enum DCPluginSyncFilterResult_ {
    DCP_SYNC_FILTER_RESULT_OK,
    DCP_SYNC_FILTER_RESULT_KILL,
    DCP_SYNC_FILTER_RESULT_ERROR,
    DCP_SYNC_FILTER_RESULT_FATAL
} DCPluginSyncFilterResult;

int dcplugin_init(DCPlugin *dcplugin, int argc, char *argv[]);
int dcplugin_destroy(DCPlugin *dcplugin);

DCPluginSyncFilterResult
dcplugin_sync_pre_filter(DCPlugin *dcplugin, DCPluginDNSPacket *dcp_packet);

DCPluginSyncFilterResult
dcplugin_sync_post_filter(DCPlugin *dcplugin, DCPluginDNSPacket *dcp_packet);

#define dcplugin_get_user_data(P) ((P)->user_data)
#define dcplugin_set_user_data(P, V) ((P)->user_data = (V))

#endif
