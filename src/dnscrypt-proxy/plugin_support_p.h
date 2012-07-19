
#ifndef __PLUGIN_SUPPORT_P_H__
#define __PLUGIN_SUPPORT_P_H__ 1

#include <ltdl.h>

#include "queue.h"

struct DCPluginSupport_ {
    SLIST_ENTRY(DCPluginSupport_) next;
    lt_dlhandle  handle;
    DCPlugin    *plugin;
    const char  *plugin_file;
    char       **argv;
    int          argc;
};

struct DCPluginSupportContext_ {
    SLIST_HEAD(DCPluginSupportList_, DCPluginSupport_) dcps_list;
};

typedef int (*DCPluginInit)(DCPlugin * const dcplugin, int argc, char argv[]);

#endif
