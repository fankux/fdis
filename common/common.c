#include "common.h"

#ifdef __cplusplus
namespace fankux {
#endif

/* parse key, syntax: key:[num_index]:2nd level key,
** key length limited to 128 charactors
** "sec_len" is section length,
** "start" section start index,
** "next" section start index,
** return 1, string key, there may be still next section
** return 2, number index, there may be still next section
** return -1, syntax error,
** return -2, number index that should be in [ ] is not number */
int keysplit(char* buf, size_t* sec_len,
        char** start, char** next) {
    size_t len = 0;
    char end;

    while (*buf == ' ') ++buf;/* skip spaces */
    if (!*buf) return 0;/* buffer end */
    if ('[' == *buf) {/* number mode */
        end = ']';
        ++buf;
    } else end = ':';
    if (start) *start = buf;
    while (end != *buf) {
        if (!*buf) {
            /* [ no closing */
            if (']' == end) return -1;
            break;
        }
        /* characters between [ and ] must be number */
        if (']' == end && (*buf < '0' || *buf > '9')) {
            return -2;
        }
        ++len;
        ++buf;
    }
    if (*buf) ++buf;
    if (sec_len) *sec_len = len;
    if (next) *next = buf;
    if (']' == end) return 2;

    return 1;
}

/* split command value string like ,
** "abc" "def"\0, abc def\0, abc "def"\0 to
** "abc\0 "def\0, abc\0def\0, abc\0"def\0.
** by spaces, each call of this function while get next section
** divided by spaces, note that buf poniter will be moved after
** each calling, each section end up whth '\0' which overwriting
** the section end space or end ".
** use loop struct to call this function
** section like "123", 32a or a32 is string, but 123 means number
** "" means null type, spaces will be skip, so some converter needed
** "sec_len" output section length,
** "start" section start index,
** "next" next section start index,
** if success, string section, return 1,
** if success, number section, return 2,
** if buf end, return 0;
** if syntax error, return -1 */
int valuesplit(char* buf, size_t* sec_len,
        char** start, char** next) {
    char* s;
    char end;
    size_t len = 0, num_flag = 1;

    while (*buf == ' ') ++buf; /* skip spaces */

    if (!*buf) return 0;
    if ('\"' == *buf) {
        end = '\"';
        ++buf;
    }
    else end = ' ';
    if (start) *start = buf;
    s = buf;
    while (*buf != end) {
        if (!*buf) {
            if ('\"' == end) return -1; /* " must match */
            break;
        }
        if (buf == s && '-' == *buf) num_flag = 1;
        else if (*buf > '9' || *buf < '0' || '\"' == end) num_flag = 0;
        ++buf;
        ++len;
    }
    if ('\"' == end) ++buf;
    if (next) *next = buf;
    if (sec_len) *sec_len = len;
    if (num_flag) return 2;

    return 1;
}

inline int timeval_subtract(struct timeval* result, struct timeval* start, struct timeval* end) {
    if (start->tv_sec > end->tv_sec)
        return -1;

    if ((start->tv_sec == end->tv_sec) && (start->tv_usec > end->tv_usec))
        return -1;

    result->tv_sec = (end->tv_sec - start->tv_sec);
    result->tv_usec = (end->tv_usec - start->tv_usec);

    if (result->tv_usec < 0) {
        --result->tv_sec;
        result->tv_usec += 1000000;
    }

    return 0;
}

inline struct timeval timeval_add(struct timeval a, struct timeval b) {
    struct timeval re;
    re.tv_sec = a.tv_sec + b.tv_sec;
    re.tv_usec = a.tv_usec + a.tv_usec;
    if (re.tv_usec > 1000000) {
        ++re.tv_sec;
        re.tv_usec -= 1000000;
    }
    return re;
}

inline char* time_formate(char* buffer, time_t* ptm) {
    struct tm* stm = localtime(ptm);
    sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d", stm->tm_year + 1900, stm->tm_mon + 1,
            stm->tm_mday, stm->tm_hour, stm->tm_min, stm->tm_sec);
    return buffer;
}

#ifdef __cplusplus
}
#endif
