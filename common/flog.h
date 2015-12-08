#ifndef FLOG_H
#define FLOG_H

#include <stdio.h>
#include <time.h>
#include <pthread.h>

#include "common/common.h"

#ifdef __cplusplus
extern "C" {
namespace fankux{
#endif

#ifndef LOG_LEVEL
#define LOG_LEVEL 4
#endif

extern time_t __now__;
extern char __tm_buffer__[20];

#if LOG_LEVEL > 4
#define debug(format, ...) do { \
    __now__ = time(NULL);       \
    printf("(%5d)[%s]DEBUG:[%s:%4d]"format"\n", pthread_self(), time_formate(__tm_buffer__, &__now__), __FILE__, __LINE__, ##__VA_ARGS__);\
} while(0)
#else
#define debug(format, ...)
#endif

#if LOG_LEVEL > 3
#define info(format, ...) do {   \
    __now__ = time(NULL);       \
    printf("(%5d)[%s]INFO:[%s:%4d]"format"\n", pthread_self(), time_formate(__tm_buffer__, &__now__), __FILE__, __LINE__, ##__VA_ARGS__); \
} while(0)
#else
#define info(format, ...)
#endif

#if LOG_LEVEL > 2
#define warn(format, ...) do {  \
    __now__ = time(NULL);       \
    printf("(%5d)[%s]WARN:[%s:%4d]"format"\n", pthread_self(), time_formate(__tm_buffer__, &__now__), __FILE__, __LINE__, ##__VA_ARGS__);\
} while(0)
#else
#define warn(format, ...)
#endif

#define error(format, ...) do { \
    __now__ = time(NULL);       \
    printf("(%5d)[%s]ERROR:[%s:%4d]"format"\n", pthread_self(), time_formate(__tm_buffer__, &__now__), __FILE__, __LINE__, ##__VA_ARGS__);\
} while(0)

#define fatal(format, ...) do { \
    __now__ = time(NULL);       \
    printf("(%5d)[%s]FATAL:[%s:%4d]"format"\n", pthread_self(), time_formate(__tm_buffer__, &__now__), __FILE__, __LINE__, ##__VA_ARGS__);\
} while(0)

#ifdef __cplusplus
}
};
#endif

#endif /* FLOG_H */
