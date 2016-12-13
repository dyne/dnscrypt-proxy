
#ifndef __WINDOWS_SERVICE_H__
#define __WINDOWS_SERVICE_H__ 1

#ifndef WINDOWS_SERVICE_NAME
# define WINDOWS_SERVICE_NAME "dnscrypt-proxy"
#endif

#define WINDOWS_SERVICE_REGISTRY_PARAMETERS_PATH \
    "SYSTEM\\CurrentControlSet\\Services\\"
#define WINDOWS_SERVICE_REGISTRY_PARAMETERS_NAME \
    "\\Parameters"

typedef enum WinOption_ {
    WIN_OPTION_INSTALL = 256,
    WIN_OPTION_REINSTALL,
    WIN_OPTION_UNINSTALL,
    WIN_OPTION_SERVICE_NAME
} WinOption;

char *get_windows_service_name(void);
char *windows_service_registry_parameters_key(void);
int windows_service_install(ProxyContext * const proxy_context);
int windows_service_uninstall(void);

#endif
