#ifndef UTEST_H
#define UTEST_H

#include "flog.h"

#define typeof __typeof__
#define check_null_ret(var, ret, format, ...)               \
if((var) == (typeof(var))NULL) {                            \
    log(#format, ##__VA_ARGS__);                            \
    return (ret);                                           \
}

#define check_null_goto(var, to, format, ...)               \
if((var) == (typeof(var))NULL) {                            \
    log(#format, ##__VA_ARGS__);                            \
    goto to;                                                \
}

#define check_null_ret_oom(var, ret, format, ...)           \
if((var) == (typeof(var))NULL) {                            \
    log("!!!OUT OF MEMORY!!!"#format"memory allocate faild!", ##__VA_ARGS__);  \
    return (ret);                                           \
}

#define check_null_goto_oom(var, to, format, ...)           \
if((var) == (typeof(var))NULL) {                            \
    log("!!!OUT OF MEMORY!!!"#format"memory allocate faild!", ##__VA_ARGS__);  \
    goto to;                                                \
}

#define check_cond_ret(condition, ret, format, ...)         \
if(!(condition)) {                                          \
    log(#format, ##__VA_ARGS__);                            \
    return (ret);                                           \
}                                                           \

#define check_cond_goto(condition, to, format, ...)         \
if(!(condition)) {                                          \
    log(#format, ##__VA_ARGS__);                            \
    goto to;                                                \
}

/* !!!DO NOT use function call as args!!! */

#define test_equal_int_arg(expect, test, skip, format, ...)     \
    if ((expect) != (test)) {                       \
        log("!!!test faild!!! expect :%d , but occur: %d"format, (expect), (test), ##__VA_ARGS__);  \
        if (!(skip)) exit(0);                       \
    } else {                                        \
        log("passed, result:%d"format, (test), ##__VA_ARGS__);  \
    }

#define test_equal_str_arg(expect, test, skip, format, ...)     \
    if (strcmp((expect), (test)) != 0) {            \
        log("!!!test faild!!! expect :%s , but occur: %s"format, (expect), (test), ##__VA_ARGS__);  \
        if (!(skip)) exit(0);                       \
    } else {                                        \
        log("passed, result:%s"format, (test), ##__VA_ARGS__);  \
    }

#define test_equal_int(expect, test, skip) test_equal_int_arg((expect), (test), (skip), "")
#define test_equal_str(expect, test, skip) test_equal_str_arg((expect), (test), (skip), "")

#endif //UTEST_H
