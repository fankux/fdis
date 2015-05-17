/**
 * QUEUE base on flist
 */

#ifndef FQUEUE_H
#define FQUEUE_H

#include "flist.h"

#define FQUEUE_OK 0
#define FQUEUE_NONE -1
#define FQUEUE_FIALD -2

struct fqueue {
    struct flist *data;
    size_t max_size; /* 0 means no limit */
};

/************* macros *************/
#define fqueue_isempty(queue) (flist_len((queue)->data) == 0)
#define fqueue_isfull(queue) (flist_len((queue)->data)>= (queue)->max_size)

/************* API *************/
struct fqueue *fqueue_create();

void fqueue_free(struct fqueue *queue);

struct fqueue *fqueue_create_fixed(const size_t size);

int fqueue_add(struct fqueue *queue, void *val);

void *fqueue_pop(struct fqueue *queue);

#endif /* FQUEUE_H */
