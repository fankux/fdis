#ifndef FLOG_H
#define FLOG_H

#include <stdio.h>
#include <time.h>

#include "common/common.h"

#define LOG_CLI
#ifdef LOG_CLI

#ifdef __cplusplus
extern "C" {
namespace fankux{
#endif

extern time_t __now__;
extern char __tm_buffer__[20];

#define debug(format, ...) do { \
    __now__ = time(NULL);       \
    printf("[%s]DEBUG:[%s:%4d]"format"\n", time_formate(__tm_buffer__, &__now__), __FILE__, __LINE__, ##__VA_ARGS__);\
} while(0)
#define info(format, ...) do {   \
    __now__ = time(NULL);       \
    printf("[%s]INFO:[%s:%4d]"format"\n", time_formate(__tm_buffer__, &__now__), __FILE__, __LINE__, ##__VA_ARGS__); \
} while(0)
#define warn(format, ...) do {  \
    __now__ = time(NULL);       \
    printf("[%s]WARN:[%s:%4d]"format"\n", time_formate(__tm_buffer__, &__now__), __FILE__, __LINE__, ##__VA_ARGS__);\
} while(0)
#define error(format, ...) do { \
    __now__ = time(NULL);       \
    printf("[%s]ERROR:[%s:%4d]"format"\n", time_formate(__tm_buffer__, &__now__), __FILE__, __LINE__, ##__VA_ARGS__);\
} while(0)
#define fatal(format, ...) do { \
    __now__ = time(NULL);       \
    printf("[%s]FATAL:[%s:%4d]"format"\n", time_formate(__tm_buffer__, &__now__), __FILE__, __LINE__, ##__VA_ARGS__);\
} while(0)
#endif

#ifdef LOG_BOTH
#define debug(format, ...) do { \
    __now__ = time(NULL);       \
    printf("[%s]DEBUG:[%s:%4d]"format"\n", time_formate(__tm_buffer__, &__now__), __FILE__, __LINE__, ##__VA_ARGS__);\
} while(0)
#define info(format, ...) do {   \
    __now__ = time(NULL);       \
    printf("[%s]INFO:[%s:%4d]"format"\n", time_formate(__tm_buffer__, &__now__), __FILE__, __LINE__, ##__VA_ARGS__); \
} while(0)
#define warn(format, ...) do {  \
    __now__ = time(NULL);       \
    printf("[%s]WARN:[%s:%4d]"format"\n", time_formate(__tm_buffer__, &__now__), __FILE__, __LINE__, ##__VA_ARGS__);\
} while(0)
#define error(format, ...) do { \
    __now__ = time(NULL);       \
    printf("[%s]ERROR:[%s:%4d]"format"\n", time_formate(__tm_buffer__, &__now__), __FILE__, __LINE__, ##__VA_ARGS__);\
} while(0)
#define fatal(format, ...) do { \
    __now__ = time(NULL);       \
    printf("[%s]FATAL:[%s:%4d]"format"\n", time_formate(__tm_buffer__, &__now__), __FILE__, __LINE__, ##__VA_ARGS__);\
} while(0)
#endif

#ifndef debug
#define debug(format, ...)
#endif

#ifndef info
#define log(format, ...)
#endif

#ifndef warn
#define warn(fromat, ...)
#endif

#ifndef error
#define error(fromat, ...)
#endif

#ifndef fatal
#define fatal(fromat, ...)
#endif

#ifdef __cplusplus
}
};
#endif

#endif /* FLOG_H */
