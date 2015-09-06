#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include <time.h>
#include "common/fqueue.h"

typedef pthread_mutex_t mutex;
typedef pthread_cond_t cond_lock;
typedef pthread_attr_t thread_attr;

typedef void *(*thread_routine)(void *);

#define THREAD_IDLE_TIME 60

#define THREAD_READY 1
#define THREAD_RUN 2
#define THREAD_END 3

#define TASK_RUN_IMMEDIATELY    0
#define TASK_RUN_DELAY          1

struct threaditem {
    int no;
    int status;
    pthread_t tid;
    thread_routine routine;
    struct fqueue *task_queue;
};

struct threadarg {
    struct threadpool *pool;
    struct threaditem *thread;
};

typedef struct threadpool {
    size_t min;
    size_t max;
    volatile int act_num;
    volatile int task_num;

    mutex lock;

    /* left to right active thread bitmap */
    fbits_t *bits;
    /* queue array */
    struct fqueue **tasks;
    struct threadarg *args;
    struct threaditem *threads;

} threadpool_t;

typedef void *(*thread_func_t)(void *args);

typedef struct threadtask {
    int id;
    int run_type;
    __useconds_t run_delay;
    struct timeval start_time;
    struct timeval end_time;

    thread_func_t call;
    void *arg;

} threadtask_t;

#endif /* THREADPOOL_H */
