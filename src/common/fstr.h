#ifndef FSTR_H
#define FSTR_H

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
namespace fdis {
#endif

struct fstr { //string
    int len;
    int free;
    char buf[];
};

#define FSTR_TRIMBOTH 0
#define FSTR_TRIMLEFT 1
#define FSTR_TRIMRIGHT 2

/* API */
#define fstr_getpt(buf) ((struct fstr *)((buf) - sizeof(struct fstr)))

struct fstr* fstr_createlen(const char* str, const size_t len);

struct fstr* fstr_create(const char* str);

struct fstr* fstr_createint(const int64_t value);

void fstr_empty(struct fstr* str);

void fstr_free(struct fstr* str);

void fstr_freestr(char* buf);

struct fstr* fstr_reserve(struct fstr* a, const size_t len);

struct fstr* fstr_catlen(struct fstr* a, const char* b, const size_t len);

struct fstr* fstr_cat(struct fstr* a, const char* b);

struct fstr* fstr_trim(struct fstr* s, const int FLAG);

struct fstr* fstr_setlen(struct fstr* a, const char* b, const size_t pos, const size_t len);

struct fstr* fstr_set(struct fstr* a, const char* b, const size_t pos);

struct fstr* fstr_removelen(struct fstr* s, const size_t start, const size_t len);

struct fstr* fstr_duplen(struct fstr* a, const size_t start, const size_t len);

struct fstr* fstr_dup(struct fstr* a);

struct fstr* fstr_replace(struct fstr* a, char* b, const size_t pos);

struct fstr* fstr_replacelen(struct fstr* a, char* b, const size_t pos, const size_t len);

struct fstr* fstr_dup_endpoint(struct fstr* b, const char c);

struct fstr* fstr_insertlen(struct fstr* a, char* b, const size_t pos, const size_t len);

struct fstr* fstr_insert(struct fstr* a, char* b, const size_t pos);

void fstr_squeeze(struct fstr* a);

int fstr_compare(struct fstr* a, struct fstr* b);

void fstr_info(struct fstr* str);

#endif

#ifdef __cplusplus
namespace fdis{
#endif
