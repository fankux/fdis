/**
 * QUEUE base on flist
 */

#ifndef FQUEUE_H
#define FQUEUE_H

#include "flist.h"
#include <pthread.h>

#define FQUEUE_OK 0
#define FQUEUE_NONE -1
#define FQUEUE_FIALD -2

struct fqueue {
    struct flist *data;
    size_t max_size;
    /* 0 means no limit */

    int blocked;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

typedef struct flist_node fqueue_node_t;

/************* macros *************/
#define fqueue_isempty(queue) (flist_len((queue)->data) == 0)
#define fqueue_isfull(queue) (flist_len((queue)->data)>= (queue)->max_size)

/************* API *************/
void fqueue_free(struct fqueue *queue);

struct fqueue *fqueue_create(int blocked);

struct fqueue *fqueue_create_fixed(size_t size, int blocked);

int fqueue_add(struct fqueue *queue, void *val);

void *fqueue_pop(struct fqueue *queue);

#endif /* FQUEUE_H */
