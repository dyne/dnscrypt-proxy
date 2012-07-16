
#include <config.h>
#include <sys/types.h>
#ifdef HAVE_SETRLIMIT
# include <sys/resource.h>
#endif
#include <sys/time.h>

#ifdef HAVE_SANDBOX_H
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
# include <sandbox.h>
#endif

#include "sandboxes.h"

static int
sandboxes_rlimit(void)
{
#ifdef HAVE_SETRLIMIT
    if (setrlimit(RLIMIT_NPROC,
                  & (struct rlimit) { .rlim_cur = 0, .rlim_max = 0 }) != 0) {
        return -1;
    }
#endif
    return 0;
}

int
sandboxes_app(void)
{
    return sandboxes_rlimit();
}

int
sandboxes_pidproc(void)
{
#ifdef HAVE_SANDBOX_INIT
    char *errmsg;

    if (sandbox_init(kSBXProfileNoNetwork, SANDBOX_NAMED, &errmsg) != 0) {
        return -1;
    }
#endif
    return sandboxes_rlimit();
}
