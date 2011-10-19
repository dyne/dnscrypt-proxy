#include <time.h>
#include <sys/time.h>
#include "nanoseconds.h"

/* XXX: Y2036 problems; should upgrade to a 128-bit type for this */
/* XXX: nanosecond granularity limits users to 1 terabyte per second */

long long nanoseconds(void)
{
#ifdef __APPLE__
  struct timeval t;
  if (gettimeofday(&t, NULL) != 0) return -1;
  return t.tv_sec * 1000000000LL + t.tv_usec * 1000LL;
#else
  struct timespec t;
  if (clock_gettime(CLOCK_REALTIME,&t) != 0) return -1;
  return t.tv_sec * 1000000000LL + t.tv_nsec;
#endif
}
