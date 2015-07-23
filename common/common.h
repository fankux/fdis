#ifndef COMMON_H
#define COMMON_H

#define DEBUG_THREADPOOL
#define LOG_CLI

#include <stdio.h>

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

#define check_null_ret(var, ret, log, ...)                  \
if((var) == NULL) {                                         \
    printf((log), ##__VA_ARGS__);                           \
    return (ret);                                           \
}

#define check_null_goto(var, to, log, ...)                  \
if((var) == NULL) {                                         \
    printf((log), ##__VA_ARGS__);                           \
    goto to;                                                \
}

#define check_null_ret_oom(var, ret, log, ...)              \
if((var) == NULL) {                                         \
    printf("!!!OUT OF MEMORY!!!"#log, ##__VA_ARGS__);       \
    return (ret);                                           \
}

#define check_null_goto_oom(var, to, log, ...)              \
if((var) == NULL) {                                         \
    printf("!!!OUT OF MEMORY!!!"#log"memory allocate faild!", ##__VA_ARGS__);  \
    goto to;                                                \
}

#define check_cond_ret(condition, ret, log, ...)            \
    if(!(condition)) {                                      \
        printf((log), ##__VA_ARGS__);                       \
        return (ret);                                       \
}                                                           \

#define check_cond_goto(condition, to, log, ...)            \
    if(!(condition)) {                                      \
        printf((log), ##__VA_ARGS__);                       \
        return to;                                          \
}                                                           \


int keysplit(char *buf, size_t *sec_len, char **start, char **next);

int valuesplit(char *buf, size_t *sec_len, char **start, char **next);

/* UNIT test */
void test_equal_str(char *expect, char *test, int skip);

#endif /* COMMON_H */
