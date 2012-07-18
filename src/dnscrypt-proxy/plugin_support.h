
#ifndef __PLUGIN_SUPPORT_H__
#define __PLUGIN_SUPPORT_H__ 1

typedef struct DCPluginSupportContext_ DCPluginSupportContext;

DCPluginSupportContext *plugin_support_context_new(void);
void plugin_support_context_free(DCPluginSupportContext * const dcps_context);

#endif
