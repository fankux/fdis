#include <stddef.h>

#include "fmem.h"
#include "fqueue.h"

#ifdef __cplusplus
namespace fdis{
#endif

inline void fqueue_free(struct fqueue* queue) {
    if (queue == NULL) return;
    ffree(queue);
}

inline void fqueue_init_fixed(struct fqueue* queue, const size_t size, const int blocking) {
    if (queue == NULL) return;

    flist_init(&queue->data);

    queue->max_size = size == 0 ? SIZE_MAX : size;
    queue->blocked = blocking;
    if (queue->blocked) {
        pthread_mutex_init(&queue->mutex, NULL);
        pthread_cond_init(&queue->cond, NULL);
    }
}

inline struct fqueue* fqueue_create(const int blocking) {
    return fqueue_create_fixed(0, blocking);
}

inline struct fqueue* fqueue_create_fixed(const size_t size, int blocking) {
    struct fqueue* queue = fmalloc(sizeof(struct fqueue));
    if (queue == NULL) return NULL;

    flist_init(&queue->data);

    queue->max_size = size == 0 ? SIZE_MAX : size;
    queue->blocked = blocking;
    if (queue->blocked) {
        pthread_mutex_init(&queue->mutex, NULL);
        pthread_cond_init(&queue->cond, NULL);
    }

    return queue;
}

inline size_t fqueue_size(struct fqueue* queue) {
    return flist_len(&queue->data);
}

static int fqueue_add_block(struct fqueue* queue, void* val) {
    pthread_mutex_lock(&queue->mutex);

    while (fqueue_isfull(queue)) {
        pthread_cond_wait(&queue->cond, &queue->mutex);
    }
    int result = flist_add_tail(&queue->data, val);
    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);

    return result;
}

static fqueue_node_t* fqueue_pop_block(struct fqueue* queue) {
    pthread_mutex_lock(&queue->mutex);

    while (fqueue_isempty(queue)) {
        pthread_cond_wait(&queue->cond, &queue->mutex);
    }
    fqueue_node_t* n = flist_pop_head(&queue->data);
    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
    return n;
}

inline int fqueue_add(struct fqueue* queue, void* val) {
    if (queue->blocked) {
        return fqueue_add_block(queue, val);
    }
    if (fqueue_isfull(queue)) return FQUEUE_FIALD;
    return flist_add_tail(&queue->data, val);
}

inline void* fqueue_pop(struct fqueue* queue) {
    fqueue_node_t* n;
    if (queue->blocked) {
        n = fqueue_pop_block(queue);
    } else {
        n = flist_pop_head(&queue->data);
    }
    if (n) {
        void* data = n->data;
        ffree(n);
        return data;
    }
    return NULL;
}

inline void* fqueue_peek(struct fqueue* queue) {
    fqueue_node_t* n = queue->data.head;
    if (n == NULL) return NULL;
    return n->data;
}

inline void fqueue_clear(struct fqueue* queue) {
    if (queue->blocked) {
        pthread_cond_signal(&queue->cond);
        pthread_mutex_lock(&queue->mutex);
        pthread_cond_signal(&queue->cond);
        pthread_mutex_unlock(&queue->mutex);
    }
    flist_clear(&queue->data);
}

#ifdef __cplusplus
}
#endif
