
#ifndef __PLUGIN_SUPPORT_P_H__
#define __PLUGIN_SUPPORT_P_H__ 1

#include "queue.h"

struct DCPluginSupport_ {
    SLIST_ENTRY(DCPluginSupport_) next;
    const char  *plugin_name;
    char       **argv;
    int          argc;
};

struct DCPluginSupportContext_ {
    SLIST_HEAD(DCPluginSupportList_, DCPluginSupport_) dcps_list;
};

#endif
