#include <stddef.h>

#include "fmem.h"
#include "fqueue.h"

inline void fqueue_free(struct fqueue *queue) {
    if (queue == NULL) return;
    flist_free(queue->data);
    ffree(queue);
}

inline struct fqueue *fqueue_create(int blocked) {
    return fqueue_create_fixed(0, blocked);
}

inline struct fqueue *fqueue_create_fixed(size_t size, int blocked) {
    struct fqueue *queue = fmalloc(sizeof(struct fqueue));
    if (queue == NULL) return NULL;

    if ((queue->data = flist_create()) == NULL) return NULL;

    queue->max_size = size == 0 ? SIZE_MAX : size;
    queue->blocked = blocked;
    if (queue->blocked) {
        pthread_mutex_init(&queue->mutex, NULL);
        pthread_cond_init(&queue->cond, NULL);
    }

    return queue;
}

static int fqueue_add_block(struct fqueue *queue, void *val) {
    pthread_mutex_lock(&queue->mutex);

    while (fqueue_isfull(queue)) {
        pthread_cond_wait(&queue->cond, &queue->mutex);
    }
    int result = flist_add_tail(queue->data, val);
    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);

    return result;
}

static fqueue_node_t *fqueue_pop_block(struct fqueue *queue) {
    pthread_mutex_lock(&queue->mutex);

    while (fqueue_isempty(queue)) {
        pthread_cond_wait(&queue->cond, &queue->mutex);
    }
    fqueue_node_t *n = flist_pop_head(queue->data);
    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
    return n;
}

inline int fqueue_add(struct fqueue *queue, void *val) {
    if (queue->blocked) {
        return fqueue_add_block(queue, val);
    }
    if (fqueue_isfull(queue)) return FQUEUE_FIALD;
    return flist_add_tail(queue->data, val);
}

inline void *fqueue_pop(struct fqueue *queue) {
    if (queue->blocked) {
        return fqueue_pop_block(queue)->data;
    }
    fqueue_node_t *n = flist_pop_head(queue->data);
    if (n) return n->data;
    return NULL;
}