#ifndef COMMON_H
#define COMMON_H

#define DEBUG_THREADPOOL

#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
namespace fankux{
#endif

#define typeof __typeof__
#define min(x, y) ({                                        \
            typeof(x) _min1 = (x);                          \
            typeof(y) _min2 = (y);                          \
            (void) (&_min1 == &_min2);                      \
            _min1 < _min2 ? _min1 : _min2; })

#define max(x, y) ({                                        \
            typeof(x) _max1 = (x);                          \
            typeof(y) _max2 = (y);                          \
            (void) (&_max1 == &_max2);                      \
            _max1 > _max2 ? _max1 : _max2; })


int keysplit(char* buf, size_t* sec_len, char** start, char** next);

int valuesplit(char* buf, size_t* sec_len, char** start, char** next);

int timeval_subtract(struct timeval* re, struct timeval* start, struct timeval* end);

char* time_formate(char* buffer, time_t* ptm);

#ifdef __cplusplus
}
};
#endif

#endif /* COMMON_H */
