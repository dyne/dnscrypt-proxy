
#include <dnscrypt/plugin.h>

int dcplugin_init(DCPlugin * const dcplugin, int argc, char argv[])
{
    (void) dcplugin;
    (void) argc;
    (void) argv;

    return 0;
}

int dcplugin_destroy(DCPlugin * const dcplugin)
{
    (void) dcplugin;

    return 0;
}
