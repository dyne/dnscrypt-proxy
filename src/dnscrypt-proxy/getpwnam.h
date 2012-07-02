
#ifndef __GETPWNAM_H__
#define __GETPWNAM_H__ 1

#include <config.h>
#include <sys/types.h>

#ifdef HAVE_PWD_H
# include <pwd.h>
# ifdef HAVE_UUID_UUID_H
#  include <uuid/uuid.h>
# endif

#elif defined(HAVE_GETPWNAM)

# include <time.h>
# ifdef HAVE_UUID_UUID_H
#  include <uuid/uuid.h>
# endif

struct passwd {
    char    *pw_name;       /* user name */
    char    *pw_passwd;     /* encrypted password */
    uid_t    pw_uid;        /* user uid */
    gid_t    pw_gid;        /* user gid */
    time_t   pw_change;     /* password change time */
    char    *pw_class;      /* user access class */
    char    *pw_gecos;      /* Honeywell login info */
    char    *pw_dir;        /* home directory */
    char    *pw_shell;      /* default shell */
    time_t   pw_expire;     /* account expiration */
};
struct passwd *getpwnam(const char *);

#endif

#endif
