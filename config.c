#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>

#include "common.h"
#include "config.h"
#include "fmem.h"

#define LOG_CLI

#include "flog.h"
#include "fmap.h"

static int _parse_file(struct fconf * conf, char * buf, size_t size);

static inline struct fconf * _fconf_init()
{
    struct fconf * conf = fmalloc(sizeof(struct fconf));
    ASSERT_NULL(conf, NULL);

    if (!(conf->data = fmap_create_dupboth())) {
        ffree(conf);
        return NULL;
    }
    return conf;
}

/**
* parse config file content
* key can't repeat which would cause failure.
* syntax like:
* a = b
* c = d
* 'aa' = b
* cc = 'bb'
*/
#define STAT_BF_KEY 0
#define STAT_AT_KEY 1
#define STAT_AF_KEY 2
#define STAT_BF_VAL 3
#define STAT_AT_VAL 4
#define STAT_AF_VAL 5
static int _parse_file(struct fconf * conf, char * buf, size_t size)
{
    size_t buf_idx = 0;
    int stat = STAT_BF_KEY;
    char end = ' ';
    char key[FCONFIG_BUFSIZE] = "\0";
    char val[FCONFIG_BUFSIZE] = "\0";

    /* status machine for parse config syntax
     * TODO. backslash...
     */
    while (*buf != '\0') {
        if (stat == STAT_BF_KEY) {
            end = ' ';
            if (isspace(*buf)) { /* skip spaces before key */
                ++buf;
            } else if (*buf == '\'' || *buf == '"') { /* value included by quotes */
                stat = STAT_AT_KEY;
                end = *buf;
                ++buf;
            } else {
                stat = STAT_AT_KEY;
                buf_idx = 0;
            }
            continue;
        }

        if (stat == STAT_AT_KEY) {
            /* value included by quotes */
            if (end != ' ' && *buf == '=') {
                log("syntax error : key must end with %c", end);
                goto error;
            } else if (end == ' ' && *buf == '=') {
                stat = STAT_BF_VAL;
                key[buf_idx] = '\0';
                ++buf;
            } else if (end != ' ' && *buf == end) {
                stat = STAT_AF_KEY;
                key[buf_idx] = '\0';
                ++buf;
            } else if (end == ' ' && isspace(*buf)) {
                stat = STAT_AF_KEY;
                key[buf_idx] = '\0';
            } else {
                key[buf_idx++] = *buf++;
            }
            continue;
        }


        if (stat == STAT_AF_KEY) { /* content after key must be = or spaces, otherwise syntax error */
            if (*buf == '=') {
                stat = STAT_BF_VAL;
                ++buf;
            } else if (isspace(*buf)) {
                ++buf;
            } else {
                log("syntax error : error charactors between key and value");
                goto error;
            }
            continue;
        }

        if (stat == STAT_BF_VAL) {
            end = ' ';
            if (isspace(*buf)) { /* skip spaces before value */
                ++buf;
            } else if (*buf == '\'' || *buf == '"') {
                stat = STAT_AT_VAL;
                end = *buf;
                ++buf;
            } else {
                stat = STAT_AT_VAL;
                buf_idx = 0;
            }
            continue;
        }

        if (stat == STAT_AT_VAL) {
            if (end != ' ' && (*buf == '\n' || *buf == '\r')) {
                log("syntax error : value must end with %c", end);
                goto error;
            } else if (end == ' ' && (*buf == '\n' || *buf == '\r')) {
                stat = STAT_BF_KEY;
                val[buf_idx] = '\0';
                ++buf;

                fmap_add(conf->data, key, val);
                log("%s => %s \n", key, val);
            } else if (end != ' ' && *buf == end) {
                stat = STAT_AF_VAL;
                val[buf_idx] = '\0';
                ++buf;
            } else if (end == ' ' && isspace(*buf)) {
                stat = STAT_AF_VAL;
                val[buf_idx] = '\0';
            } else {
                val[buf_idx++] = *buf++;
            }
            continue;
        }

        /* if code ran here, must be STAT_AF_VAL */
        if (stat == STAT_AF_VAL) {
            if (*buf == '\n' || *buf == '\r') {
                stat = STAT_BF_KEY;
                ++buf;

                fmap_add(conf->data, key, val);
            } else if (isspace(*buf)) {
                ++buf;
            } else {
                log("syntax error : error charactors between key and value");
                goto error;
            }
            continue;
        }
    }

/* at most condition, last value will follow by '\0'  */
    log("stat:%d", stat);
    if (stat == STAT_AF_VAL) {
        val[buf_idx] = '\0';
        fmap_add(conf->data, key, val);
    } else if (stat != STAT_BF_KEY) {
        log("Syntax error : status error: %d", stat);
        goto error;
    }

    return 0;

    error:
    fmap_empty(conf->data);
    return -1;
}

fconf_t * fconf_create(char * path)
{
    struct fconf * conf = _fconf_init();
    if (!conf) {
        log("fconf init faild, mem error");
        return NULL;
    }

    struct stat file_stat;
    if (-1 == stat(path, &file_stat)) {
        log("get file(%s) status faild, %d, %s", path, errno, strerror(errno));
        return NULL;
    }
    if (file_stat.st_size >= FCONFIG_BUFSIZE) { /* over size */
        log("file too large to read, file path:%s , size: %ld", path, file_stat.st_size);
        return NULL;
    }

    FILE * fp = fopen(path, "r");
    if (!fp) {
        log("file(%s) open faild : %d, %s", path, errno, strerror(errno));
        return NULL;
    }


    char buf[FCONFIG_BUFSIZE] = "\0";
    size_t n = fread(buf, FCONFIG_BUFSIZE, 1, fp);

    log("file(%s) read success, size: %zu, content:%s", path, n, buf);

    _parse_file(conf, buf, n);

    fclose(fp);

    return conf;
}

char * fconf_get(fconf_t * conf, char * key)
{
    struct fmap_node * node = fmap_get(conf->data, key);
    if (!node) return NULL;
    return node->value;
}

int fconf_get_int(fconf_t * conf, char * key)
{
    char * value = fmap_getvalue(conf->data, key);
    if (!value) return 0;
    return atoi(value);
}

double fconf_get_float(fconf_t * conf, char * key)
{
    char * value = fmap_getvalue(conf->data, key);
    if (!value) return 0;
    return atof(value);
}

#ifdef DEBUG_FCONF

int main()
{
    fconf_t * conf = fconf_create("/home/fankux/projects/step/flog/testconf.conf");
    fmap_info_str(conf->data);

    return 0;
}

#endif
