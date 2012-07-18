
#ifndef __DNSCRYPT_PLUGIN_H__
#define __DNSCRYPT_PLUGIN_H__ 1

#include <dnscrypt/version.h>

typedef struct DCPlugin_ DCPlugin;

int dcplugin_init(DCPlugin * const dcplugin, int argc, char argv[]);

#endif
