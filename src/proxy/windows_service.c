
#include <config.h>

#include "app.h"
#include "windows_service.h"

#ifndef _WIN32

int
main(int argc, char *argv[])
{
    return dnscrypt_proxy_main(argc, argv);
}

#else

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#ifndef SERVICE_NAME
# define SERVICE_NAME "dnscrypt-proxy"
#endif

static SERVICE_STATUS        service_status;
static SERVICE_STATUS_HANDLE service_status_handle;

static void WINAPI
control_handler(const DWORD wanted_state)
{
    if (wanted_state == SERVICE_CONTROL_STOP &&
        dnscrypt_proxy_loop_break() == 0) {
        service_status.dwCurrentState = SERVICE_STOPPED;
    }
    SetServiceStatus(service_status_handle, &service_status);
}

static void WINAPI
service_main(DWORD argc_, LPTSTR *argv_)
{
    memset(&service_status, 0, sizeof service_status);
    service_status.dwServiceType = SERVICE_WIN32;
    service_status.dwCurrentState = SERVICE_START_PENDING;
    service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    service_status_handle =
        RegisterServiceCtrlHandler(SERVICE_NAME, control_handler);
    if (service_status_handle == 0) {
        return;
    }
    service_status.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(service_status_handle, &service_status);

    dnscrypt_proxy_main((int) argc_, (char **) argv_);
}

static int
windows_main(int argc, char *argv[])
{
    static SERVICE_TABLE_ENTRY service_table[2];
    char                      *service_name;

    if ((service_name = strdup(SERVICE_NAME)) == NULL) {
        perror("strdup");
        return 1;
    }
    memcpy(service_table, (SERVICE_TABLE_ENTRY[2]) {
        { .lpServiceName = service_name, .lpServiceProc = service_main },
        { .lpServiceName = NULL,         .lpServiceProc = (void *) NULL }
    }, sizeof service_table);
    if (StartServiceCtrlDispatcher(service_table) == 0) {
        free(service_name);
        return dnscrypt_proxy_main(argc, argv);
    }
    return 0;
}

int
main(int argc, char *argv[])
{
    return windows_main(argc, argv);
}

#endif
