/* Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "uv.h"
#include "internal.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <TargetConditionals.h>

#if !TARGET_OS_IPHONE
#include <CoreServices/CoreServices.h>
#endif

#include <mach/mach.h>
#include <mach/mach_time.h>
#include <mach-o/dyld.h> /* _NSGetExecutablePath */
#include <sys/resource.h>
#include <sys/sysctl.h>
#include <unistd.h>  /* sysconf */

#if TARGET_OS_IPHONE
/* see: http://developer.apple.com/library/mac/#qa/qa1398/_index.html */
uint64_t uv_hrtime() {
    uint64_t now;
    uint64_t enano;
    static mach_timebase_info_data_t sTimebaseInfo;

    now = mach_absolute_time();

    if (0 == sTimebaseInfo.denom) {
        (void)mach_timebase_info(&sTimebaseInfo);
    }

    enano = now * sTimebaseInfo.numer / sTimebaseInfo.denom;

    return enano;
}
#else
uint64_t uv_hrtime() {
  uint64_t now;
  Nanoseconds enano;
  uint64_t enano64;
  now = mach_absolute_time();
  AbsoluteTime at;
  if (sizeof at < sizeof now) {
    memset(&at, 0, sizeof at);
  }
  (void) sizeof(char[sizeof at <= sizeof now ? 1 : -1]);
  memcpy(&at, &now, sizeof at);
  enano = AbsoluteToNanoseconds(at);
  (void) sizeof(char[sizeof enano <= sizeof enano64 ? 1 : -1]);
  memcpy(&enano64, &enano, sizeof enano);
  return enano64;
}
#endif

int uv_exepath(char* buffer, size_t* size) {
  uint32_t usize;
  int result;
  char* path;
  char* fullpath;

  if (!buffer || !size) {
    return -1;
  }

  usize = *size;
  result = _NSGetExecutablePath(buffer, &usize);
  if (result) return result;

  path = (char*)malloc(2 * PATH_MAX);
  fullpath = realpath(buffer, path);

  if (fullpath == NULL) {
    free(path);
    return -1;
  }

  strncpy(buffer, fullpath, *size);
  free(fullpath);
  *size = strlen(buffer);
  return 0;
}

uint64_t uv_get_free_memory(void) {
  vm_statistics_data_t info;
  mach_msg_type_number_t count = sizeof(info) / sizeof(integer_t);

  if (host_statistics(mach_host_self(), HOST_VM_INFO,
                      (host_info_t)&info, &count) != KERN_SUCCESS) {
    return -1;
  }

  return (uint64_t) info.free_count * sysconf(_SC_PAGESIZE);
}

uint64_t uv_get_total_memory(void) {
  uint64_t info;
  int which[] = {CTL_HW, HW_MEMSIZE};
  size_t size = sizeof(info);

  if (sysctl(which, 2, &info, &size, NULL, 0) < 0) {
    return -1;
  }

  return (uint64_t) info;
}

void uv_loadavg(double avg[3]) {
  struct loadavg info;
  size_t size = sizeof(info);
  int which[] = {CTL_VM, VM_LOADAVG};

  if (sysctl(which, 2, &info, &size, NULL, 0) < 0) return;

  avg[0] = (double) info.ldavg[0] / info.fscale;
  avg[1] = (double) info.ldavg[1] / info.fscale;
  avg[2] = (double) info.ldavg[2] / info.fscale;
}
