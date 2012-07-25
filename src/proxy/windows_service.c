
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

#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "logger.h"
#include "utils.h"

#ifndef WINDOWS_SERVICE_NAME
# define WINDOWS_SERVICE_NAME "dnscrypt-proxy"
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
        RegisterServiceCtrlHandler(WINDOWS_SERVICE_NAME, control_handler);
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

    if ((service_name = strdup(WINDOWS_SERVICE_NAME)) == NULL) {
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

static int
windows_service_uninstall(void)
{
    SC_HANDLE scm_handle;
    SC_HANDLE service_handle;
    int       ret = 0;

    scm_handle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (scm_handle == NULL) {
        return -1;
    }
    service_handle = OpenService(scm_handle, WINDOWS_SERVICE_NAME, DELETE);
    if (service_handle == NULL) {
        CloseServiceHandle(scm_handle);
        return 0;
    }
    if (DeleteService(service_handle) == 0) {
        ret = -1;
    }
    CloseServiceHandle(service_handle);
    CloseServiceHandle(scm_handle);

    return ret;
}

static int
windows_service_install(void)
{
    TCHAR     self_path[MAX_PATH];
    SC_HANDLE scm_handle;
    SC_HANDLE service_handle;

    windows_service_uninstall();
    if (GetModuleFileName(NULL, self_path, MAX_PATH) <= (DWORD) 0) {
        return -1;
    }
    scm_handle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (scm_handle == NULL) {
        return -1;
    }
    service_handle = CreateService
        (scm_handle, WINDOWS_SERVICE_NAME,
         WINDOWS_SERVICE_NAME, SERVICE_ALL_ACCESS,
         SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START,
         SERVICE_ERROR_NORMAL, self_path, NULL, NULL, NULL, NULL, NULL);

    if (service_handle == NULL) {
        CloseServiceHandle(scm_handle);
        return -1;
    }
    CloseServiceHandle(service_handle);
    CloseServiceHandle(scm_handle);

    return 0;
}

int
windows_service_option(const int opt_flag)
{
    switch (opt_flag) {
    case WIN_OPTION_INSTALL:
    case WIN_OPTION_REINSTALL:
        if (windows_service_install() != 0) {
            logger_noformat(NULL, LOG_ERR, "Unable to install the service");
            exit(1);
        } else {
            logger_noformat(NULL, LOG_INFO, "The " WINDOWS_SERVICE_NAME
                            " service has been installed (but not started)");
            exit(0);
        }
        break;
    case WIN_OPTION_UNINSTALL:
        if (windows_service_uninstall() != 0) {
            logger_noformat(NULL, LOG_ERR, "Unable to uninstall the service");
            exit(1);
        } else {
            logger_noformat(NULL, LOG_INFO, "The " WINDOWS_SERVICE_NAME
                            " service has been removed from this system");
            exit(0);
        }
        break;
    default:
        return -1;
    }
    return 0;
}

int
main(int argc, char *argv[])
{
    return windows_main(argc, argv);
}

#endif
