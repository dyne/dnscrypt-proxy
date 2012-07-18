
#ifndef __PLUGIN_SUPPORT_H__
#define __PLUGIN_SUPPORT_H__ 1

typedef struct DCPluginSupport_ DCPluginSupport;

DCPluginSupport *plugin_support_new(void);
void plugin_support_free(DCPluginSupport * const dcps);

#endif
