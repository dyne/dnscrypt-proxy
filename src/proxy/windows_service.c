
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <windows.h>

#include "logger.h"
#include "utils.h"

#ifndef WINDOWS_SERVICE_NAME
# define WINDOWS_SERVICE_NAME "dnscrypt-proxy"
#endif
#ifndef WINDOWS_SERVICE_REGISTRY_PARAMETERS_KEY
# define  WINDOWS_SERVICE_REGISTRY_PARAMETERS_KEY \
    "SYSTEM\\CurrentControlSet\\Services\\" \
    WINDOWS_SERVICE_NAME "\\Parameters"
#endif

static SERVICE_STATUS        service_status;
static SERVICE_STATUS_HANDLE service_status_handle;
static _Bool                 app_is_running_as_a_service;

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
    app_is_running_as_a_service = 1;

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
append_tstring_to_tstring(TCHAR ** const str1_p, size_t * const str1_len_p,
                          const TCHAR * const str2)
{
    TCHAR  *str1_tmp;
    size_t  required_len;
    size_t  str2_len;

    if (str2 == NULL) {
        return 0;
    }
    str2_len = _tcslen(str2);
    if (SIZE_MAX / sizeof (sizeof *str1_tmp) - str2_len <= *str1_len_p) {
        return -1;
    }
    required_len = *str1_len_p + str2_len + 1U;
    if ((str1_tmp = realloc(*str1_p,
                            required_len * sizeof (*str1_tmp))) == NULL) {
        return -1;
    }
    memcpy(str1_tmp + *str1_len_p, str2, str2_len * sizeof (*str2));
    *str1_p = str1_tmp;
    *str1_len_p = *str1_len_p + str2_len;
    (*str1_p)[*str1_len_p] = 0;

    return 0;
}

static int
append_string_to_tstring(TCHAR ** const str1_p, size_t * const str1_len_p,
                         const char * const str2)
{
    TCHAR  *str2_tchar;
    size_t  str2_len;
    int     ret;

    if (sizeof (TCHAR) == sizeof(char)) {
        return append_tstring_to_tstring(str1_p, str1_len_p,
                                         (const TCHAR *) str2);
    }
    str2_len = strlen(str2);
    if (str2_len >= SIZE_MAX / sizeof *str2_tchar) {
        return -1;
    }
    if ((str2_tchar = calloc(str2_len + 1U, sizeof *str2_tchar)) == NULL) {
        return -1;
    }
    ret = -1;
    if (MultiByteToWideChar(CP_UTF8, 0, str2, -1,
                            (LPWSTR) str2_tchar, str2_len + 1U) > 0) {
        ret = append_tstring_to_tstring(str1_p, str1_len_p, str2_tchar);
    }
    free(str2_tchar);

    return ret;
}

static char **
cmdline_clone_options(const int argc, char ** const argv)
{
    char **argv_new;

    if (argc >= INT_MAX || (size_t) argc >= SIZE_MAX / sizeof *argv_new ||
        (argv_new = calloc((unsigned int) argc + 1U,
                           sizeof *argv_new)) == NULL) {
        return NULL;
    }
    memcpy(argv_new, argv, (unsigned int) (argc + 1U) * sizeof *argv_new);

    return argv_new;
}

static int
cmdline_add_option(int * const argc_p, char *** const argv_p,
                   const char * const arg)
{
    char  *arg_dup;
    char **argv_new;

    if (*argc_p >= INT_MAX ||
        SIZE_MAX / sizeof *argv_new <= (unsigned int) (*argc_p + 2U)) {
        return -1;
    }
    if ((argv_new = realloc(*argv_p, (unsigned int) (*argc_p + 2U) *
                            sizeof *argv_new)) == NULL) {
        return -1;
    }
    if ((arg_dup = strdup(arg)) == NULL) {
        free(argv_new);
        return -1;
    }
    argv_new[(*argc_p)++] = arg_dup;
    argv_new[*argc_p] = NULL;
    *argv_p = argv_new;

    return 0;
}

static char *
windows_service_registry_read_parameter(const char * const key)
{
    BYTE   *value = NULL;
    HKEY    hk = NULL;
    DWORD   value_len;
    DWORD   value_type;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     WINDOWS_SERVICE_REGISTRY_PARAMETERS_KEY,
                     (DWORD) 0, KEY_READ, &hk) != ERROR_SUCCESS) {
        return NULL;
    }
    if (RegQueryValueEx(hk, key, 0,
                        &value_type, NULL, &value_len) == ERROR_SUCCESS &&
        value_type == (DWORD) REG_SZ && value_len <= SIZE_MAX &&
        (value = malloc((size_t) value_len)) != NULL) {
        if (RegQueryValueEx(hk, key, 0,
                            &value_type, value, &value_len) != ERROR_SUCCESS ||
            value_type != (DWORD) REG_SZ) {
            free(value);
            value = NULL;
        }
        assert(value == NULL || value_len == 0 ||
               (value_len > 0 && value[value_len - 1] == 0));
    }
    RegCloseKey(hk);

    return (char *) value;
}

static int
windows_service_install(const int argc, const char * const argv[])
{
    TCHAR      self_path[MAX_PATH];
    TCHAR     *cmd_line = NULL;
    SC_HANDLE  scm_handle;
    SC_HANDLE  service_handle;
    size_t     cmd_line_size = (size_t) 0U;
    int        i;

    if (GetModuleFileName(NULL, self_path, MAX_PATH) <= (DWORD) 0) {
        return -1;
    }
    scm_handle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (scm_handle == NULL) {
        return -1;
    }
    append_tstring_to_tstring(&cmd_line, &cmd_line_size, self_path);
    i = 1;
    while (i < argc) {
        append_tstring_to_tstring(&cmd_line, &cmd_line_size, _T(" "));
        append_string_to_tstring(&cmd_line, &cmd_line_size, argv[i]);
        i++;
    }
    service_handle = CreateService
        (scm_handle, WINDOWS_SERVICE_NAME,
         WINDOWS_SERVICE_NAME, SERVICE_ALL_ACCESS,
         SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START,
         SERVICE_ERROR_NORMAL, cmd_line, NULL, NULL, NULL, NULL, NULL);
    free(cmd_line);
    if (service_handle == NULL) {
        CloseServiceHandle(scm_handle);
        return -1;
    }
    CloseServiceHandle(service_handle);
    CloseServiceHandle(scm_handle);

    return 0;
}

int
windows_service_option(const int opt_flag, const int argc,
                       const char *argv[])
{
    if (app_is_running_as_a_service != 0) {
        return 0;
    }
    switch (opt_flag) {
    case WIN_OPTION_INSTALL:
    case WIN_OPTION_REINSTALL:
        windows_service_uninstall();
        if (windows_service_install(argc, argv) != 0) {
            logger_noformat(NULL, LOG_ERR, "Unable to install the service");
            exit(1);
        } else {
            logger_noformat(NULL, LOG_INFO, "The " WINDOWS_SERVICE_NAME
                            " service has been installed and started");
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
