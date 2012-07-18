
#ifndef __DL_H__
#define __DL_H__ 1

typedef struct DL_ DL;

DL *dl_open(const char * const lib_file);
int dl_close(DL * const dl);
const char *dl_error(const DL * const dl);
void *dl_sym(DL * const dl, const char * const symbol);

#endif
