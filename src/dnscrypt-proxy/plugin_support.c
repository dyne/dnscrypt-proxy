
#include <config.h>

#include <assert.h>
#include <stdlib.h>
#include <dnscrypt/plugin.h>

#include "plugin_support.h"
#include "plugin_support_p.h"
#include "queue.h"

DCPluginSupport *
plugin_support_new(const char * const plugin_name)
{
    DCPluginSupport *dcps;

    if ((dcps = calloc((size_t) 1U, sizeof *dcps)) == NULL) {
        return NULL;
    }
    assert(plugin_name != NULL && *plugin_name != 0);
    dcps->plugin_name = plugin_name;

    return dcps;
}

void
plugin_support_free(DCPluginSupport * const dcps)
{
    assert(dcps->plugin_name != NULL && *dcps->plugin_name != 0);
    free(dcps);
}

DCPluginSupportContext *
plugin_support_context_new(void)
{
    DCPluginSupportContext *dcps_context;

    if ((dcps_context = calloc((size_t) 1U, sizeof *dcps_context)) == NULL) {
        return NULL;
    }
    SLIST_INIT(&dcps_context->dcps_list);

    return dcps_context;
}

int
plugin_support_context_insert(DCPluginSupportContext * const dcps_context,
                              DCPluginSupport * const dcps)
{
    assert(dcps_context != NULL);
    assert(dcps != NULL);
    SLIST_INSERT_HEAD(&dcps_context->dcps_list, dcps, next);

    return 0;
}

int
plugin_support_context_remove(DCPluginSupportContext * const dcps_context,
                              DCPluginSupport * const dcps)
{
    assert(! SLIST_EMPTY(&dcps_context->dcps_list));
    SLIST_REMOVE(&dcps_context->dcps_list, dcps, DCPluginSupport_, next);

    return 0;
}

void
plugin_support_context_free(DCPluginSupportContext * const dcps_context)
{
    DCPluginSupport *dcps;
    DCPluginSupport *dcps_tmp;

    SLIST_FOREACH_SAFE(dcps, &dcps_context->dcps_list, next, dcps_tmp) {
        plugin_support_free(dcps);
    }
    free(dcps_context);
}
