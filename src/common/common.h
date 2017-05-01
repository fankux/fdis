#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
namespace fdis{
#endif

#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#else
# define UNUSED(x) x
#endif

#define typeof __typeof__
#define min(x, y) ({                                \
    typeof(x) _min1 = (x);                          \
    typeof(y) _min2 = (y);                          \
    (void) (&_min1 == &_min2);                      \
    _min1 < _min2 ? _min1 : _min2; })

#define max(x, y) ({                                \
    typeof(x) _max1 = (x);                          \
    typeof(y) _max2 = (y);                          \
    (void) (&_max1 == &_max2);                      \
    _max1 > _max2 ? _max1 : _max2; })

#define min2level(x) ({                             \
    while(x > 4 && (x & (x - 1))) x = x & (x - 1);  \
    (x <= 4 ? 4 : x << 1);                          \
})

#define AVOID_COPY(__clazz__)                   \
private:                                        \
    __clazz__(const __clazz__&);                \
    __clazz__& operator=(const __clazz__&);     \


#define INC(n) __sync_add_and_fetch(&n, 1)
#define DEC(n) __sync_sub_and_fetch(&n, 1)
#define CAS __sync_bool_compare_and_swap

struct s_time {
    uint64_t stamp;
    uint64_t mills;
};

/**
 * @brief   parse key, syntax: key:[num_index]:2nd level key, key length limited to 128 charactors
 *
 * @param   buf
 * @param   sec_len section length
 * @param   start section start index
 * @param   next next section start index
 * @return  1, string key, there may be still next section
            2, number index, there may be still next section
            -1, syntax error,
            -2, number index that should be in [ ] is not number
 */
int keysplit(char* buf, size_t* sec_len, char** start, char** next);

/**
 @brief split command value string like ,
 "abc" "def"\0, abc def\0, abc "def"\0 to
 "abc\0 "def\0, abc\0def\0, abc\0"def\0.
 by spaces, each call of this function while get next section
 divided by spaces, note that buf poniter will be moved after
 each calling, each section end up whth '\0' which overwriting
 the section end space or end ".
 use loop struct to call this function
 section like "123", 32a or a32 is string, but 123 means number
 "" means null type, spaces will be skip, so some converter needed

 * @param buf
 * @param       sec_len     output section length
 * @param       start       section start index
 * @param       next        next section start index
 * @return      if success, string section, return 1,
                if success, number section, return 2,
                if buf end, return 0;
                if syntax error, return -1
 */
int valuesplit(char* buf, size_t* sec_len, char** start, char** next);

int timeval_subtract(struct timeval* re, struct timeval* start, struct timeval* end);

struct timeval timeval_add(struct timeval a, struct timeval b);

char* time_formate(char* buffer, time_t* ptm);

#ifdef __cplusplus
}
};
#endif

#endif /* COMMON_H */
