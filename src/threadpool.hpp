#ifndef WEBC_THREADPOOL_HPP
#define WEBC_THREADPOOL_HPP

#include <stdlib.h>

#include "common/check.h"
#include "threadpool.h"

namespace fdis {
class Threadpool {
public:
    Threadpool() {};

    int init(const size_t min_size, const size_t max_size) {
        _pool = threadpool_create(min_size, max_size);
        check_null_oom(_pool, return -1, "thread pool ");
        if (threadpool_init(_pool) != 0) {
            free(_pool);
            return -1;
        }
        return 0;
    }

    int add(threadtask_t* task) {
        return threadpool_add_task(_pool, task);
    }

private:
    threadpool_t* _pool;
};
};

#endif //WEBC_THREADPOOL_HPP
