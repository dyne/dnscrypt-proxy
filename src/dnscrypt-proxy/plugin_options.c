
#include <config.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "plugin_options.h"
#include "plugin_support.h"

int
plugin_option_parse_str(DCPluginSupportContext * const dcps_context, char *str)
{
    DCPluginSupport *dcps;
    char            *plugin_name;
    char            *tmp;

    assert(dcps_context != NULL);
    assert(str != NULL);
    if (*(plugin_name = str) == 0) {
        errno = EINVAL;
        return -1;
    }
    if ((tmp = strchr(str, ',')) != NULL) {
        *tmp++ = 0;
    }
    if ((dcps = plugin_support_new(plugin_name)) == NULL) {
        return -1;
    }
    plugin_support_context_insert(dcps_context, dcps);

    return 0;
}
