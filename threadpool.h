#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include <time.h>
#include "fqueue.h"

typedef pthread_mutex_t mutex_lock;

struct thread_pool {
    int idle;
    int min;
    int max;
    int extra;

    mutex_lock * lock;
    struct fqueue * tasks;
};

#define TASK_STATUS_READY  0
#define TASK_STATUS_RUNNING     1
#define TASK_STATUS_DONE 2

struct task{
    int id;
    int status;
    time_t start_time;
    time_t end_time;

    void * (*run)(void * arg1, void * arg2, void * arg3);
    void * (*success)(void * arg1, void * arg2, void * arg3);
    void * (*faild)(void * arg1, void * arg2, void * arg3);
};

#endif /* THREADPOOL_H */
