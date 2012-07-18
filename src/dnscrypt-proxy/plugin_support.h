
#ifndef __PLUGIN_SUPPORT_H__
#define __PLUGIN_SUPPORT_H__ 1

#include "queue.h"

typedef struct DCPluginSupportContext_ DCPluginSupportContext;
typedef struct DCPluginSupport_ DCPluginSupport;

DCPluginSupportContext *plugin_support_context_new(void);
void plugin_support_context_free(DCPluginSupportContext * const dcps_context);
int plugin_support_context_insert(DCPluginSupportContext * const dcps_context,
                                  DCPluginSupport * const dcps);
int plugin_support_context_remove(DCPluginSupportContext * const dcps_context,
                                  DCPluginSupport * const dcps);

DCPluginSupport * plugin_support_new(const char * const plugin_file);
void plugin_support_free(DCPluginSupport *dcps);
int plugin_support_add_option(DCPluginSupport * const dcps, char * const arg);

#endif
