#ifndef FCONFIG_H
#define FCONFIG_H

#include <stdio.h>
#include <sys/types.h>

/* +1 act as null end */
#define FCONFIG_BUFSIZE 4096 + 1

typedef struct fconf {
    char * path;
    FILE * fp;

    size_t size;
    ssize_t idx;
    struct fmap * data;
} fconf_t;


fconf_t * fconf_create(char * path);
char * fconf_get(fconf_t * conf, char * key);
int fconf_get_int(fconf_t * conf, char * key);
double fconf_get_float(fconf_t * conf, char * key);

#endif /* F_CONFIG_H */
