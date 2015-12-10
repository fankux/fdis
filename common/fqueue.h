#ifndef FQUEUE_H
#define FQUEUE_H

#include "flist.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
namespace fankux{
#endif


#define FQUEUE_OK 0
#define FQUEUE_NONE -1
#define FQUEUE_FIALD -2

struct fqueue {
    struct flist data;
    size_t max_size;
    /* 0 means no limit */

    int blocked;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

typedef struct flist_node fqueue_node_t;

/************* macros *************/
#define fqueue_isempty(queue) (flist_len((&((queue)->data))) == 0)
#define fqueue_isfull(queue) (flist_len((&((queue)->data)))>= (queue)->max_size)

/************* API *************/
void fqueue_free(struct fqueue* queue);

void fqueue_clear(struct fqueue* queue);

void fqueue_init_fixed(struct fqueue* queue, const size_t size, const int blocking);

struct fqueue* fqueue_create(const int blocking);

struct fqueue* fqueue_create_fixed(const size_t size, const int blocking);

size_t fqueue_size(struct fqueue* queue);

int fqueue_add(struct fqueue* queue, void* val);

void* fqueue_pop(struct fqueue* queue);

void* fqueue_peek(struct fqueue* queue);

#ifdef __cplusplus
}
};
#endif

#endif /* FQUEUE_H */
