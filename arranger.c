#include "arranger.h"
#include "fmap.h"
#include "fmem.h"

struct fmap *request_mapping;

/**
 * center schelder for requests
 */
struct arranger {
    char *conf_path;
    void *request_mapping;

    void *thread_pool; /* TODO..be a specify struct */
};


struct arranger *schelder_init(char *conf_path) {
    struct arranger *ager = fmalloc(sizeof(struct arranger));
    if (ager == NULL) return NULL;

    ager->conf_path = conf_path;
    ager->request_mapping = fmap_create();


    return ager;
}