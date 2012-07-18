
#include <config.h>

#include <stdlib.h>
#include <dnscrypt/plugin.h>

#include "plugin_support.h"
#include "plugin_support_p.h"

DCPluginSupport *
plugin_support_new(void)
{
    DCPluginSupport *dcps;

    if ((dcps = calloc((size_t) 1U, sizeof *dcps)) == NULL) {
        return NULL;
    }
    return dcps;
}

void
plugin_support_free(DCPluginSupport * const dcps)
{
    free(dcps);
}
