
#include <config.h>

#include <stdlib.h>
#include <dnscrypt/plugin.h>

#include "plugin_support.h"
#include "plugin_support_p.h"

DCPluginSupportContext *
plugin_support_context_new(void)
{
    DCPluginSupportContext *dcps_context;

    if ((dcps_context = calloc((size_t) 1U, sizeof *dcps_context)) == NULL) {
        return NULL;
    }
    return dcps_context;
}

void
plugin_support_context_free(DCPluginSupportContext * const dcps_context)
{
    free(dcps_context);
}
