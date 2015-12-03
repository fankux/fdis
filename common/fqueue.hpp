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

    void add(T& value) {
        fqueue_add(&_queue, &value);
    }

    T* pop() {
        return fqueue_pop(&_queue);
    }

    T* peek() {
        return fqueue_peek(&_queue);
    }

private:
    struct fqueue _queue;
};

}

#endif // FANKUX_QUEUE_H
