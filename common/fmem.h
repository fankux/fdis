#ifndef FMEM_H
#define FMEM_H

#include <stdlib.h>
#include <string.h>


#define fmalloc(bytes) malloc(bytes)

#define ffree(p) free(p)

#define fmemset memset

#define fmemcpy memcpy


#define cpystr(dst, org, bytes) do{ \
    fmemcpy((dst), (org), (bytes)); \
    ((char *)dst)[bytes] = '\0';    \
}while(0)

#endif /* FMEM_H */
