#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "common.h"
#include "fstr.h"

#ifdef __cplusplus
extern "C" {
namespace fdis{
#endif

struct fstr* fstr_createlen(const char* str, const size_t len) {
    struct fstr* p;
    if (!(p = (struct fstr*) malloc(sizeof(struct fstr) + len + 1)))
        return NULL;

    if (str) {
        size_t str_len = strlen(str);
        p->len = str_len > len ? len : str_len;
        p->free = len - p->len;

        memcpy(p->buf, str, p->len);
        p->buf[p->len] = '\0';
    } else {
        p->len = 0;
        p->free = len;
    }

    return p;
}

struct fstr* fstr_create(const char* str) {
    return fstr_createlen(str, (str == NULL) ? 0 : strlen(str));
}

struct fstr* fstr_createint(const int64_t value) {
    struct fstr* p;
    if (!(p = (struct fstr*) malloc(sizeof(struct fstr) + sizeof(int64_t))))
        return NULL;

    memcpy(p->buf, &value, sizeof(value));

    return p;
}

void fstr_empty(struct fstr* str) {
    str->free += str->len;
    str->len = 0;
    if (str->buf)
        str->buf[0] = '\0';
}

inline void fstr_free(struct fstr* str) {
    free(str);
}

inline void fstr_freestr(char* buf) {
    if (buf == NULL) return;
    free(fstr_getpt(buf));
}

struct fstr* fstr_reserve(struct fstr* a, const size_t len) {
    struct fstr* p = a;
    int newfree = a->free - len;
    int flag = 0;

    /* space not enough,realloc double space of which needed */
    if (a->free < len) {
        if (!(p = malloc(sizeof(struct fstr) + ((p->len + len) << 2) + 1)))
            return NULL;
        memcpy(p->buf, a->buf, a->len);
        newfree = a->len + len;
        flag = 1;
    }

    p->len = a->len + len;
    p->free = newfree;

    if (flag) free(a);

    return p;
}

struct fstr* fstr_catlen(struct fstr* a, const char* b, const size_t len) {
    struct fstr* p = a;
    int newfree = a->free - len;
    int flag = 0;

    /* space not enough,realloc double space of which needed */
    if (a->free < len) {
        if (!(p = malloc(sizeof(struct fstr) + ((p->len + len) << 2) + 1)))
            return NULL;
        memcpy(p->buf, a->buf, a->len);
        newfree = a->len + len;
        flag = 1;
    }
    /* complete cat */
    memcpy(p->buf + a->len, b, len);
    p->buf[a->len + len] = '\0';

    p->len = a->len + len;
    p->free = newfree;

    if (flag) free(a);

    return p;
}

struct fstr* fstr_cat(struct fstr* a, const char* b) {
    return fstr_catlen(a, b, strlen(b));
}

struct fstr* fstr_trim(struct fstr* p, const int FLAG) {
    int i = 0, j = 0, front_blank_num = 0, tail_blank_num = 0;

    while (i < p->len) {
        int f = 0;
        if (p->buf[i] == ' ' &&
            (FLAG == 0 || FLAG == FSTR_TRIMLEFT)) {
            ++front_blank_num;
            f = 1;
        }
        if (p->buf[p->len - i - 1] == ' ' &&
            (FLAG == 0 || FLAG == FSTR_TRIMRIGHT)) {
            ++tail_blank_num;
            f = 1;
        }
        if (f == 0) {
            break;
        }
        ++i;
    }
    for (i = front_blank_num; i < p->len - tail_blank_num && i != j;
         ++i, ++j) {
        p->buf[j] = p->buf[i];
    }
    p->len -= front_blank_num + tail_blank_num;
    p->buf[p->len] = '\0';

    return p;
}

/* replace chars which length len */
struct fstr* fstr_setlen(struct fstr* s, const char* str, const size_t start, const size_t len) {
    if (s == NULL) {
        return NULL;
    }

    struct fstr* p = s;

    if (start > p->len)/* out of bound */
        return NULL;

    if (p->len + p->free < len + start) {/* space need realloc */
        if (!(p = (struct fstr*) malloc(sizeof(struct fstr) +
                                        ((len + start) << 1) + 1)))
            return NULL;

        memcpy(p->buf, s->buf, start);
        p->len = p->free = len + start;
        fstr_free(s);
    } else {
        p->free = p->free + p->len - len - start;
        p->len = len + start;
    }

    memcpy(p->buf + start, str, len);
    *(p->buf + start + len) = '\0';

    return p;
}

struct fstr* fstr_set(struct fstr* p, const char* str, const size_t start) {
    return fstr_setlen(p, str, start, strlen(str));
}

struct fstr* fstr_removelen(struct fstr* p, const size_t start, const size_t len) {
    int i, j;
    i = min(start, start + len);
    j = max(start, start + len);
    if (i >= p->len || i < 0 || j < 0 || j > p->len) return NULL;
    for (; j < p->len; ++i, ++j) {
        p->buf[i] = p->buf[j];
    }
    p->len -= j - i;

    return p;
}

struct fstr* fstr_duplen(struct fstr* a, const size_t start, const size_t len) {
    int i, j;
    i = min(start, start + len);
    j = max(start, start + len);
    if (i >= a->len || i < 0 || j < 0 || j > a->len)
        return NULL;

    struct fstr* p;
    if (!(p = malloc(sizeof(struct fstr) + len + 1)))
        return NULL;
    p->len = j - i;
    p->free = 0;

    memcpy(p->buf, a->buf + i, j - i);
    p->buf[p->len] = '\0';

    return p;
}

struct fstr* fstr_dup(struct fstr* a) {
    return fstr_duplen(a, 0, a->len);
}

/* copy 'b' to 'a' till 'c' appear in 'b',
** original content of 'a' will be covered and cleared */
struct fstr* fstr_dup_endpoint(struct fstr* b, const char c) {
    struct fstr* p;
    int i = 0;

    while (i < b->len && b->buf[i] != c)
        ++i;
    if (i == b->len) return NULL; /* no 'c' appear */

    if (!(p = (struct fstr*) malloc(
            sizeof(struct fstr) + ((i + 2) << 1) * sizeof(char))))
        return NULL;

    p->free = p->len = i + 1;

    memcpy(p->buf, b->buf, i + 1);
    p->buf[p->len] = '\0';

    return p;
}

struct fstr* fstr_replacelen(struct fstr* a, char* b,
                             const size_t pos, const size_t len) {
    if (pos >= a->len) return NULL;

    size_t str_len = strlen(b);
    if (str_len > len) str_len = len;
    if (pos + str_len >= a->len) return NULL;

    memcpy(a->buf + pos, b, str_len);

    return a;
}

struct fstr* fstr_replace(struct fstr* a, char* b, const size_t pos) {
    return fstr_replacelen(a, b, pos, strlen(b));
}

struct fstr* fstr_insertlen(struct fstr* a, char* b, const size_t pos,
                            const size_t len) {
    size_t str_len;
    struct fstr* p;

    if (pos > a->len) return NULL;

    str_len = strlen(b);
    if (str_len > len) str_len = len;

    p = a;

    if (str_len > a->free) {
        if (!(p = malloc(sizeof(struct fstr) +
                         (a->len + str_len << 1) + 1)))
            return NULL;

        p->free = p->len = a->len + str_len;
        memcpy(p->buf, a->buf, pos);  /* original buf before pos */
        memcpy(p->buf + pos + str_len, a->buf + pos, a->len - pos);

        free(a);
    } else {
        memmove(p->buf + pos + str_len, p->buf + pos, p->len); /* move */

        p->free -= str_len;
        p->len += str_len;
    }
    memcpy(p->buf + pos, b, str_len); /* string to be inserted */
    p->buf[p->len] = '\0';

    return p;
}

struct fstr* fstr_insert(struct fstr* a, char* b, const size_t pos) {
    return fstr_insertlen(a, b, pos, strlen(b));
}

/* clear invisiblily characters in the string */
void fstr_squeeze(struct fstr* a) {
    size_t i, s = 0, l = 0;
    int re, flag = 0;

    for (i = 0; i < a->len; ++i) {
        re = isspace(a->buf[i]);
        if (re && 0 == flag) {/* start count space characters */
            s = i;
            ++l;

            flag = 1;
        } else if (re && 1 == flag) {/* counting */
            ++l;

            if (i == a->len - 1) {/* end of string */
                a->len -= l;
                a->free += l;

                return;
            }

        } else if (!re && 1 == flag) {
            /* counting complete */
            /* clear invisiblity characters by memcpy */
            memcpy(a->buf + s, a->buf + s + l, a->len - s - l);
            a->len -= l;
            a->free += l;
            i -= l;
            l = 0;

            flag = 0;
        } else {/* nothing */

        }
    }
}

int fstr_compare(struct fstr* a, struct fstr* b) {
    int i = 0;
    if (a->len != b->len) {
        return a->len < b->len ? -1 : (a->len == b->len ? 0 : 1);
    }
    while (i < a->len) {
        if (a->buf[i] != b->buf[i]) {
            return a->buf[i] < b->buf[i] ? -1 :
                   (a->buf[i] == b->buf[i] ? 0 : 1);
        }
        ++i;
    }
    return 0;
}

void fstr_info(struct fstr* p) {
    if (p == NULL) {
        printf("null\n");
        return;
    }
    for (int i = 0; i < p->len; i++) {
        printf("%c", p->buf[i]);
    }
    printf(";length=%d,free=%d\n", p->len, p->free);
}

#ifdef __cplusplus
}
};
#endif

/* int main(){ */
/*     char str_key[] = "     qwert  yuiop[]|   "; */
/*     char str_value[] = "qqqqqwwwww"; */
/*     char str_value1[] = "qweuuurtyuuudu"; */
/*     char str_value2[] = "ssssss1"; */

/*     struct fstr * p = fstr_create(str_value); */
/*     struct fstr * p1 = fstr_create(str_value1); */
/*     struct fstr * p2 = fstr_create(str_value2); */
/*     fstr_info(p); */
/* 	p = fstr_setlen(p, "sssss", 10, 1); */
/* 	fstr_info(p); */

/* 	/\* fstr_squeeze(p1); *\/ */
/* 	/\* fstr_info(p1); *\/ */

/*     /\* printf("%d %d %d\n", fstr_compare(p, p), fstrCompare(p1, p2), *\/ */
/*     /\*                 fstr_compare(p1, p)); *\/ */

/* 	/\* p = fstr_insertlen(p, "bbbbbdafdaf", 10, 5); *\/ */
/* 	/\* fstr_info(p); *\/ */
/*     /\* p = fstr_insertlen(p, "ccccc", 0, 3); *\/ */
/* 	/\* fstr_info(p); *\/ */
/* 	/\* struct fstr * re = fstr_dup_endpoint(p1, '6'); *\/ */
/* 	/\* fstr_info(re); *\/ */

/* 	/\* re = fstr_dup_endpoint(p1, 'd'); *\/ */
/* 	/\* fstr_info(re); *\/ */

/* 	/\* re = fstrCopyEndPoint(p, p1, 'u'); *\/ */
/* 	/\* printf("copy re:%d\n", re); *\/ */
/* 	/\* fstr_info(p); *\/ */

/* 	/\* re = fstrCopyEndPoint(p, p1, 'r'); *\/ */
/* 	/\* printf("copy re:%d\n", re); *\/ */
/* 	/\* fstr_info(p); *\/ */
/* 	/\* fstr_free(p1); *\/ */

/* 	return 0; */
/* } */


