#ifndef FANKUX_QUEUE_H
#define FANKUX_QUEUE_H

#include "fqueue.h"

namespace fankux {
template<class T>
class Queue {
public:
    Queue(const size_t size = 0, bool blocking = false) {
        fqueue_init_fixed(&_queue, size, blocking);
    }

    int add(T& value) {
        return fqueue_add(&_queue, &value);
    }

    T* pop() {
        return fqueue_pop(&_queue);
    }

    T* peek() {
        return fqueue_peek(&_queue);
    }

    void clear() {
        return fqueue_clear(&_queue);
    }

    size_t size() {
        return fqueue_size(&_queue);
    }

private:
    struct fqueue _queue;
};

}

#endif // FANKUX_QUEUE_H
