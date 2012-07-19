
#include <config.h>

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#include <dnscrypt/plugin.h>
#include <ltdl.h>

#include "logger.h"
#include "plugin_support.h"
#include "plugin_support_p.h"
#include "queue.h"

int
plugin_support_add_option(DCPluginSupport * const dcps, char * const arg)
{
    char **argv;

    if (*arg == 0) {
        return 0;
    }
    if (dcps->argc >= INT_MAX) {
        return -1;
    }
    if (SIZE_MAX / sizeof *argv <= (unsigned int) (dcps->argc + 2U)) {
        return -1;
    }
    if ((argv = realloc(dcps->argv, (unsigned int) (dcps->argc + 2U) *
                        sizeof *argv)) == NULL) {
        return -1;
    }
    argv[dcps->argc++] = arg;
    argv[dcps->argc] = NULL;
    dcps->argv = argv;

    return 0;
}

static int
plugin_support_load(DCPluginSupport * const dcps)
{
    lt_dladvise advise;
    lt_dlhandle handle;

    assert(dcps != NULL && dcps->plugin_file != NULL);
    assert(dcps->handle == NULL);
    if (lt_dladvise_init(&advise) != 0) {
        return -1;
    }
    lt_dladvise_local(&advise);
    lt_dladvise_ext(&advise);
    logger(NULL, LOG_INFO, "Loading plugin [%s]", dcps->plugin_file);
    if ((handle = lt_dlopenadvise(dcps->plugin_file, advise)) == NULL) {
        logger(NULL, LOG_ERR, "Unable to load [%s]: [%s]",
               dcps->plugin_file, lt_dlerror());
        lt_dladvise_destroy(&advise);
        return -1;
    }
    lt_dladvise_destroy(&advise);

    return 0;
}

static int
plugin_support_unload(DCPluginSupport * const dcps)
{
    if (dcps->handle == NULL) {
        return 0;
    }
    if (lt_dlclose(dcps->handle) != 0) {
        return -1;
    }
    dcps->handle = NULL;

    return 0;
}

void
plugin_support_free(DCPluginSupport * const dcps)
{
    plugin_support_unload(dcps);
    assert(dcps->plugin_file != NULL && *dcps->plugin_file != 0);
    free(dcps->argv);
    dcps->argv = NULL;
    free(dcps);
}

DCPluginSupport *
plugin_support_new(const char * const plugin_file)
{
    DCPluginSupport *dcps;

    if ((dcps = calloc((size_t) 1U, sizeof *dcps)) == NULL) {
        return NULL;
    }
    assert(plugin_file != NULL && *plugin_file != 0);
    dcps->plugin_file = plugin_file;
    dcps->argv = NULL;
    dcps->handle = NULL;

    return dcps;
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

int
plugin_support_context_load(DCPluginSupportContext * const dcps_context)
{
    DCPluginSupport *dcps;
    _Bool            failed = 0;

    assert(dcps_context != NULL);
    if (lt_dlinit() != 0) {
        return -1;
    }
    SLIST_FOREACH(dcps, &dcps_context->dcps_list, next) {
        if (plugin_support_load(dcps) != 0) {
            failed = 1;
        }
    }
    lt_dlexit();
    if (failed != 0) {
        return -1;
    }
    return 0;
}
