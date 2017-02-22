#ifndef FCONFIG_H
#define FCONFIG_H

#include <stdio.h>
#include <sys/types.h>

/* +1 act as null end */
#define FCONFIG_BUFSIZE 4096 + 1

#ifdef __cplusplus
extern "C" {
namespace fdis{
#endif

typedef struct fconf {
    char* path;
    FILE* fp;

    size_t size;
    ssize_t idx;
    struct fmap* data;
} fconf_t;

fconf_t* fconf_create(const char* path);

char* fconf_get(fconf_t* conf, const char* key);

int16_t fconf_get_int16(fconf_t* conf, const char* key);

int16_t fconf_get_int16_dft(fconf_t* conf, const char* key, int16_t dft);

int32_t fconf_get_int32(fconf_t* conf, const char* key);

int32_t fconf_get_int32_dft(fconf_t* conf, const char* key, int32_t dft);

int64_t fconf_get_int64(fconf_t* conf, const char* key);

int64_t fconf_get_int64_dft(fconf_t* conf, const char* key, int64_t dft);

uint16_t fconf_get_uint16(fconf_t* conf, const char* key);

uint16_t fconf_get_uint16_dft(fconf_t* conf, const char* key, uint16_t dft);

uint32_t fconf_get_uint32(fconf_t* conf, const char* key);

uint32_t fconf_get_uint32_dft(fconf_t* conf, const char* key, uint32_t dft);

uint64_t fconf_get_uint64(fconf_t* conf, const char* key);

uint64_t fconf_get_uint64_dft(fconf_t* conf, const char* key, uint64_t dft);

double fconf_get_float(fconf_t* conf, const char* key);

#ifdef __cplusplus
}
};
#endif

#endif /* F_CONFIG_H */
