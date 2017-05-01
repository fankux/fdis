#ifndef UTEST_H
#define UTEST_H

#include "flog.h"

#ifdef __cplusplus
extern "C" {
namespace fdis {
#endif

#define typeof __typeof__
#define check_null(var, action, format, ...)                \
if((var) == (typeof(var))NULL) {                            \
    fatal(format, ##__VA_ARGS__);                          \
    action;                                                 \
}

#define check_null_oom(var, action, format, ...)           \
if((var) == (typeof(var))NULL) {                            \
    fatal("!!!OUT OF MEMORY!!! "#format" memory allocate faild!", ##__VA_ARGS__);  \
    action;                                                \
}

#define check_cond_fatal(condition, action, format, ...)    \
if(!(condition)) {                                          \
    fatal(format, ##__VA_ARGS__);                           \
    action;                                                 \
}                                                           \

/* !!!DO NOT use function call as args!!! */

#define test_equal_int_arg(expect, test, skip, format, ...)         \
    if ((expect) != (test)) {                       \
        error("!!!test faild!!! expect :%d , but occur: %d"format, (expect), (test), ##__VA_ARGS__);  \
        if (!(skip)) exit(0);                       \
    } else {                                        \
        error("passed, result:%d"format, (test), ##__VA_ARGS__);    \
    }

#define test_equal_str_arg(expect, test, skip, format, ...)         \
    if (strcmp((expect), (test)) != 0) {            \
        error("!!!test faild!!! expect :%s , but occur: %s"format, (expect), (test), ##__VA_ARGS__);  \
        if (!(skip)) exit(0);                       \
    } else {                                        \
        error("passed, result:%s"format, (test), ##__VA_ARGS__);    \
    }

#define test_equal_int(expect, test, skip) test_equal_int_arg((expect), (test), (skip), "")
#define test_equal_str(expect, test, skip) test_equal_str_arg((expect), (test), (skip), "")

#ifdef __cplusplus
}
};
#endif

#endif //UTEST_H
