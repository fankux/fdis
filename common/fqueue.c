#include <stddef.h>

#include "fmem.h"
#include "fqueue.h"

inline struct fqueue *fqueue_create() {
    return fqueue_create_fixed(0);
}

inline struct fqueue *fqueue_create_fixed(const size_t size) {
    struct fqueue *queue = fmalloc(sizeof(struct fqueue));
    if (queue == NULL) return NULL;

    if ((queue->data = flist_create()) == NULL) return NULL;
    queue->max_size = size;

    return queue;
}

inline int fqueue_add(struct fqueue *queue, void *val) {
    if (fqueue_isfull(queue)) return FQUEUE_FIALD;
    return flist_add_tail(queue->data, val);
}

inline void *fqueue_pop(struct fqueue *queue) {
    struct flist_node *n = flist_pop_head(queue->data);
    if (n) return n->data;
    return NULL;
}