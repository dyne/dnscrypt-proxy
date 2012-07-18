
#ifndef __APP_H__
#define __APP_H__ 1

#include <config.h>
#ifdef PLUGINS
# include "plugin_support.h"
#endif

typedef struct AppContext_ {
    struct ProxyContext_ *proxy_context;
#ifdef PLUGINS
    DCPluginSupport *dpcs;
#endif
} AppContext;

#endif
