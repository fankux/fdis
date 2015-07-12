#ifndef FLOG_H
#define FLOG_H

#include <stdio.h>

#ifdef LOG_CLI
#define debug(format, ...) do { \
    log("debug:", ##__VA_ARGS__);   \
} while(0)
#define log(format, ...) do { \
    printf("[%s:%d]"format"\n", __FILE__, __LINE__, ##__VA_ARGS__); \
} while(0)
#define warn(format, ...) do { \
    printf("[%s:%d]warning:"format"\n", __FILE__, __LINE__, ##__VA_ARGS__); \
} while(0)
#define error(format, ...) do { \
    log("error:", ##__VA_ARGS__);   \
} while(0)
#define fatal(format, ...) do { \
    log("!!!fatal error:", ##__VA_ARGS__);  \
    exit(EXIT_FAILURE); \
} while(0)
#endif

#ifdef LOG_BOTH
#define debug(format, ...) do { \
    log("debug:", ##__VA_ARGS__);   \
} while(0)
#define log(format, ...) do { \
    printf("[%s:%d]"format"\n", __FILE__, __LINE__, ##__VA_ARGS__); \
} while(0)
#define warn(format, ...) do { \
    printf("[%s:%d]warning:"format"\n", __FILE__, __LINE__, ##__VA_ARGS__); \
} while(0)
#define error(format, ...) do { \
    log("error:", ##__VA_ARGS__);   \
} while(0)
#define fatal(format, ...) do { \
    log("!!!fatal error:", ##__VA_ARGS__);  \
    exit(EXIT_FAILURE); \
} while(0)
#endif

#ifndef debug
#define debug(format, ...)
#endif

#ifndef log
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

#endif /* FLOG_H */
