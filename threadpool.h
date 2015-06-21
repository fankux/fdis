#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include <time.h>
#include "common/fqueue.h"

typedef pthread_mutex_t mutex;
typedef pthread_cond_t cond_lock;
typedef pthread_attr_t thread_attr;

typedef void *(*thread_rountine)(void *);

#define THREAD_RUN 1
#define THREAD_END 2

#define TASK_RUN_IMMEDIATELY    0
#define TASK_RUN_DELAY          1
#define TASK_STATUS_READY       0
#define TASK_STATUS_RUNNING     1
#define TASK_STATUS_DONE        2

struct thread_item {
    int id;
    int status;
    int handle;
    pthread_t tid;

    mutex task_lock;
    cond_lock task_cond;
    struct fqueue *task_list;

    thread_rountine rountine;
};

struct thread_arg {
    struct thread_pool *pool;
    struct thread_item *thread;
};

typedef struct thread_pool {
    int min;
    size_t max;
    int act_thread_num;
    int task_num;

    /* left to right active thread bitmap */
    fbits_t * bits;
    /* queue array */
    struct fqueue **tasks;
    struct thread_arg *args;
    struct thread_item *threads;

} thread_pool_t;

typedef void *(*thread_func_t)(void *args);

typedef struct thread_task {
    int id;
    int run_type;
    time_t start_time;
    time_t end_time;

    thread_func_t routine;
    void *arg;
    void *extra;

} thread_task_t;

#endif /* THREADPOOL_H */
