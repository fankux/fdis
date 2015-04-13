#ifndef COMMON_H
#define COMMON_H

#define DEBUG_FCONF

#include <stdio.h>

#define ASSERT(exp, ret) do{    \
    if(!(exp)) return ret;      \
}while(0)

#define ASSERT_NULL(val, ret) do {   \
    if(val == NULL) return ret; \
}while(0)

#define typeof __typeof__
#define min(x, y) ({							\
			typeof(x) _min1 = (x);				\
			typeof(y) _min2 = (y);				\
			(void) (&_min1 == &_min2);			\
			_min1 < _min2 ? _min1 : _min2; })

#define max(x, y) ({							\
			typeof(x) _max1 = (x);				\
			typeof(y) _max2 = (y);				\
			(void) (&_max1 == &_max2);			\
			_max1 > _max2 ? _max1 : _max2; })

#define cpystr(dest, org, len) ({				\
			memcpy((dest), (org), (len));		\
			(dest)[(len)] = '\0';				\
		})

int keysplit(char * buf, size_t * sec_len,
		char ** start, char ** next);
int valuesplit(char * buf, size_t * sec_len,
		char ** start, char ** next);

#endif /* COMMON_H */
