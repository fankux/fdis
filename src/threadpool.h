#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include <time.h>
#include "common/fqueue.h"
#include "common/fbit.h"

#ifdef __cplusplus
extern "C" {
namespace fdis{
#endif

typedef pthread_mutex_t mutex;
typedef pthread_cond_t cond_lock;
typedef pthread_attr_t thread_attr;

typedef void* (* thread_routine)(void*);

typedef enum {
    THREAD_READY = 1,
    THREAD_RUN = 2,
    THREAD_END = 3
} ThreadStatus;

typedef enum {
    TASK_RUN_IMMEDIATELY = 0,
    TASK_RUN_DELAY = 1
} ThreadRunType;

struct threaditem {
    int no;
    int status;
    pthread_t tid;
    thread_routine routine;
    struct fqueue* task_queue;
};

struct threadarg {
    struct threadpool* pool;
    struct threaditem* thread;
};

typedef struct threadpool {
    size_t min; // unused
    size_t max;
    volatile int act_num;
    volatile int task_num;

    mutex lock;

    /* left to right active thread bitmap */
    fbits_t* bits;
    /* queue array */
    struct fqueue** tasks;
    struct threadarg* args;
    struct threaditem* threads;

} threadpool_t;

typedef void* (* thread_func_t)(void* args);

typedef struct threadtask {
    int id;
    int run_type;
    __useconds_t run_delay;
    struct timeval start_time;
    struct timeval end_time;

    thread_func_t call;
    void* arg;

} threadtask_t;

threadpool_t* threadpool_create(size_t min_size, size_t max_size);

threadtask_t* threadtask_create(thread_func_t func, void* arg, ThreadRunType run_type,
        __useconds_t delay);

int threadpool_init(threadpool_t* pool);

int threadpool_add_task(threadpool_t* pool, threadtask_t* task);

#ifdef __cplusplus
}
};
#endif

#endif /* THREADPOOL_H */
