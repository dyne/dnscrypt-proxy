
#include <config.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "dl.h"

#if !defined(_WIN32) && !defined(HAVE_DLOPEN)

DL *dl_open(const char * const lib_file)
{
    (void) lib_file;
    errno = ENOSYS;
    
    return NULL;
}

int dl_close(DL * const dl)
{
    if (dl == NULL) {
        return 0;
    }
    errno = ENOSYS;

    return -1;
}

const char *dl_error(const DL * const dl)
{
    (void) dl;

    return strerror(errno);
}

void *dl_sym(DL * const dl, const char * const symbol)
{
    (void) dl;
    (void) symbol;

    return NULL;
}

#elif !defined(_WIN32)

#include <dlfcn.h>

#ifndef RTLD_NOW
# define RTLD_NOW 0
#endif
#ifndef RTLD_LOCAL
# define RTLD_LOCAL 0
#endif
#ifndef RTLD_FIRST
# define RTLD_FIRST 0
#endif

struct DL_ {
    void *handle;
};

DL *dl_open(const char * const lib_file)
{
    DL   *dl;

    if ((dl = calloc((size_t) 1U, sizeof *dl)) == NULL) {
        return NULL;
    }
    dl->handle = dlopen(lib_file, RTLD_NOW | RTLD_LOCAL | RTLD_FIRST);
    if (dl->handle == NULL) {
        free(dl);
        return NULL;
    }
    return dl;
}

int dl_close(DL * const dl)
{
    if (dl == NULL) {
        return 0;
    }
    if (dl->handle != NULL) {
        if (dlclose(dl->handle) != 0) {
            return -1;
        }
        dl->handle = NULL;        
    }
    free(dl);
    
    return 0;
}

const char *dl_error(const DL * const dl)
{
    (void) dl;

    return dlerror();
}

void *dl_sym(DL * const dl, const char * const symbol)
{
    if (dl == NULL) {
        return NULL;
    }
    return dlsym(dl->handle, symbol);
}

#else /* _WIN32 */

#ifdef HAVE_LIBLOADERAPI_H
# include <LibLoaderApi.h>
#elif defined(HAVE_WINBASE_H)
# include <WinBase.h>
#else
# include <Windows.h>
#endif

struct DL_ {
    HMODULE  handle;
};

static int err;

DL *dl_open(const char * const lib_file)
{
    wchar_t  lib_file_w[32768U];    
    DL      *dl;

    if (MultiByteToWideChar(CP_UTF8, 0, lib_file, -1, lib_file_w,
                            sizeof lib_file_w / sizeof lib_file_w[0]) == 0) {
        return NULL;
    }
    if ((dl = calloc((size_t) 1U, sizeof *dl)) == NULL) {
        err = errno;
        return NULL;
    }
    
    dl->handle = LoadLibraryExW(lib_file_w, NULL,
                                LOAD_WITH_ALTERED_SEARCH_PATH);
    if (dl->handle == NULL) {
        err = GetlastError();
        free(dl);
        return NULL;
    }
    return dl;
}

int dl_close(DL * const dl)
{
    if (dl == NULL) {
        return 0;
    }
    if (dl->handle != NULL) {
        if (FreeLibrary(dl->handle) == 0) {
            err = GetlastError();
            return -1;
        }
        dl->handle = NULL;        
    }
    free(dl);
    err = 0;

    return 0;
}

const char *dl_error(const DL * const dl)
{
    static char errstr[1000U];

    errstr[0] = '-';
    errstr[1] = 0;
    if (dl == NULL || err == 0) {
        return errstr;
    }
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM |
                   FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err,
                   MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                   (LPSTR) (void *) errstr, sizeof errstr, NULL);

    return errstr;
}

void *dl_sym(DL * const dl, const char * const symbol)
{
    if (dl == NULL) {
        return NULL;
    }
    return (void *) GetProcAddress(dl->handle, symbol);
}

#endif
