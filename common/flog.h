#ifndef LOG_H
#define LOG_H

#include <stdio.h>

#ifdef LOG_CLI
#define log(format, ...) do { \
    printf("[%s:%d]"format"\n", __FILE__, __LINE__, ##__VA_ARGS__); \
} while(0)
#define warn(format, ...) do { \
    printf("[%s:%d]warning:"format"\n", __FILE__, __LINE__, ##__VA_ARGS__); \
} while(0)
#define fatal(format, ...) do { \
    log("!!!fatal error:", ##__VA_ARGS__);  \
    exit(EXIT_FAILURE); \
} while(0)
#endif

#ifdef LOG_BOTH
#define log(format, ...) do { \
    printf("[%s:%d]"format"\n", __FILE__, __LINE__, ##__VA_ARGS__); \
} while(0)
#define warn(format, ...) do { \
    printf("[%s:%d]warning:"format"\n", __FILE__, __LINE__, ##__VA_ARGS__); \
} while(0)
#define fatal(format, ...) do { \
    log("!!!fatal error:", ##__VA_ARGS__);  \
    exit(EXIT_FAILURE); \
} while(0)
#endif

#ifndef log
#define log(format, ...)
#endif

#ifndef warn
#define warn(fromat, ...)
#endif

#ifndef fatal
#define fatal(fromat, ...)
#endif

#endif /* LOG_H */
