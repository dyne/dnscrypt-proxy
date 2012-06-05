
#ifndef __UTILS_H__
#define __UTILS_H__ 1

#define COMPILER_ASSERT(X) (void) sizeof(char[(X) ? 1 : -1])

void dnscrypt_memzero(void * const pnt, const size_t size);
int closedesc_all(const int closestdin);
int do_daemonize(void);

#endif
