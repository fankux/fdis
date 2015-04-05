/* fstr */

#include <string.h>
#include "common.h"

#define BBUF_SIZE 256

static inline void _good(ssize_t * bbuf, char * pattern, const size_t psize)
{
    memset(bbuf, -1, BBUF_SIZE);
    for(ssize_t i = 0; i < psize; ++i)
        bbuf[pattern[i]] = i;
}

int bad(char * gbuf, const size_t g_size)
{

}

/* substring, boyer-moore implementation */
int idxof(char * str, const size_t strsize, char * pattern, const size_t psize )
{
    if(psize == 0 || strsize < psize) return -1;

    ssize_t bbuf[BBUF_SIZE];
    char gbuf[];
    _good(bbuf, pattern, psize);

    size_t i = psize - 1, last_pos = 0;
    for(; i >= last_pos; --i){
        if(str[i] == pattern[i]) continue;

        size_t shift = psize - bbuf[pattern[i]];
        i += shift;
        last_pos += shift;
    }
}

int main()
{
    char str[] = "HERE IS A SIMPLE EXAMPLE";
    char pattern[] = "EXAMPLE";

    LOG("result:%d", idxof(str, sizeof(str) - 1, pattern, sizeof(pattern) - 1));
}