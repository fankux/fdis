#include <time.h>
#include <pthread.h>
#include <errno.h>

#define __USE_XOPEN_EXTENDED

#include <unistd.h>

#include "common/common.h"
#include "common/fmem.h"
#include "common/flog.h"
#include "common/fbit.h"
#include "common/fqueue.h"
#include "common/check.h"
#include "threadpool.h"

#ifdef __cplusplus
namespace fdis{
#endif

/* TODO..futex replace mutex
 * TODO..thread detach
 * TODO..delay task no blocking
 */
threadpool_t* threadpool_create(size_t min_size, size_t max_size) {
    struct threadpool* pool = fmalloc(sizeof(struct threadpool));
    check_null_oom(pool, return NULL, "thread pool");

    fmemset(pool, 0, sizeof(struct threadpool));

    pool->min = min_size;
    pool->max = max_size;
    pool->act_num = pool->min;
    pool->task_num = 0;
    pthread_mutex_init(&pool->lock, NULL);

    pool->bits = fbit_create(max_size);
    check_null_oom(pool->bits, goto faild, "threadpool bitmap");

    pool->threads = fcalloc(pool->max, sizeof(struct threaditem));
    check_null_oom(pool->threads, goto faild, "threadpool thread list");
    for (int i = 0; i < max_size; ++i) {
        pool->threads[i].no = i;
    }

    pool->args = fcalloc(pool->max, sizeof(struct threadarg));
    check_null_oom(pool->args, goto faild, "threadpool args list");

    pool->tasks = fcalloc(pool->max, sizeof(struct fqueue*));
    check_null_oom(pool->tasks, goto faild, "threadpool task queue list");
    for (int i = 0; i < max_size; ++i) {
        pool->tasks[i] = fqueue_create(1);
        check_null_oom(pool->tasks[i], goto faild, "threadpool task queue");
    }

    return pool;

    faild:
    for (int i = 0; i < max_size; ++i) {
        if (pool->tasks[i]) {
            fqueue_free(pool->tasks[i]);
        }
    }
    fbit_free(pool->bits);
    ffree(pool->tasks);
    ffree(pool->threads);
    ffree(pool->args);
    ffree(pool);
    return NULL;
}

threadtask_t* threadtask_create(thread_func_t func, void* arg, ThreadRunType run_type,
        __useconds_t delay) {
    struct threadtask* task = fmalloc(sizeof(struct threadtask));
    if (task == NULL) return NULL;

    task->run_type = run_type;
    task->run_delay = delay;
    task->call = func;
    task->arg = arg;
    gettimeofday(&task->start_time, NULL);
    return task;
}

static void* thread_proc(void* arg) {
    struct threadpool* pool = ((struct threadarg*) arg)->pool;
    struct threaditem* thread = ((struct threadarg*) arg)->thread;

    int tno = thread->no;

    pthread_mutex_lock(&pool->lock);
    __sync_add_and_fetch(&pool->act_num, 1);
    fbit_set(pool->bits, (size_t) thread->no, 1);
    pthread_mutex_unlock(&pool->lock);

    thread->status = THREAD_RUN;

    /* TODO..exit after max idle time  */
    info("thread[%d] thread create", tno);
    while (thread->status == THREAD_RUN) {
        debug("thread[%d] pedding", tno);
        struct threadtask* task = fqueue_pop(thread->task_queue);
        debug("thread[%d] got task", tno);

        info("thread[%d] executing", tno);

        switch (task->run_type) {
            case TASK_RUN_IMMEDIATELY:
                task->call(task->arg);
                break;
            case TASK_RUN_DELAY:
                usleep(task->run_delay);
                task->call(task->arg);
                break;
            default:
                break;
        }
        ffree(task);

        pthread_mutex_lock(&pool->lock);
        --pool->task_num;
        pthread_mutex_unlock(&pool->lock);
    }

    pthread_mutex_lock(&pool->lock);
    thread->tid = 0;
    thread->status = THREAD_END;
    __sync_add_and_fetch(&pool->act_num, -1);
    fbit_set(pool->bits, (size_t) thread->no, 0);
    pthread_mutex_unlock(&pool->lock);


    return (void*) 0;
}

/* initiate new thread which memory had allocated already */
static inline struct threaditem* _thread_init(threadpool_t* pool, int tno) {
    int status;
    struct threaditem* thread = &pool->threads[tno];

    pool->args[tno].pool = pool;
    pool->args[tno].thread = thread;

    thread->no = tno;
    thread->routine = thread_proc;
    thread->task_queue = pool->tasks[tno];
    thread->status = THREAD_READY;
    status = pthread_create(&thread->tid, NULL, thread->routine, &pool->args[tno]);
    check_cond(status == 0, goto faild, "faild to create thread, tno:%d, errno:%d, error:%s",
            tno, errno, strerror(errno));

    return thread;

    faild:
    thread->tid = 0;
    return NULL;
}

int threadpool_init(threadpool_t* pool) {
    /* init minimum number of threads */
    fbit_set_all(pool->bits, 0);
    for (int i = 0; i < pool->min; ++i) {
        if (_thread_init(pool, i) == NULL) {
            goto faild;
        }
    }
    return 0;

    faild:
    for (int i = 0; i < pool->min; ++i) {
        struct threaditem* thread = &pool->threads[i];
        thread->status = THREAD_END;
    }
    return -1;
}

/**
 * there only one thread to add task to thread pool
 * if there still thread creating spaces available, create a new thread to execute,
 * else using round robin strategy to chose a thread,
 * then queuing task to that thread
 */
int threadpool_add_task(threadpool_t* pool, threadtask_t* task) {
    size_t worker_idx;
    struct threaditem* thread;

    pthread_mutex_lock(&pool->lock);

    int task_num = ++(pool->task_num);
    debug("task number: %d", task_num);
    if (task_num > pool->min && task_num <= pool->max) { /* init new thread */

        fbit_get_val_at_round(pool->bits, 0, 1, &worker_idx);
        fbit_set(pool->bits, worker_idx, 1);

        debug("would create new thread, tno: %d", worker_idx);
        thread = _thread_init(pool, worker_idx);

    } else { /* round robin strategy */

        fbit_get_val_at_round(pool->bits, 1, (size_t) task_num, &worker_idx);

        debug("thread[%d] round robin strategy", worker_idx);
        thread = &pool->threads[worker_idx];
    }

    pthread_mutex_unlock(&pool->lock);

    fqueue_add(thread->task_queue, task);

    return 1;
}

#ifdef DEBUG_THREADPOOL

void* test_run(void* arg) {
    info("task...%d", (int) arg);
    return (void*) 0;
}

void* queue_consumer(void* arg) {
    struct fqueue* queue = (struct fqueue*) arg;
    pthread_t tid = pthread_self();

    struct timeval start, stop, delta;
    info("[%ld]fetching from queue, pedding...", tid);
    gettimeofday(&start, NULL);


    fqueue_node_t* node = fqueue_pop(queue);

    gettimeofday(&stop, NULL);
    timeval_subtract(&delta, &start, &stop);
    info("[%ld]got a queue node, time(us): %ld", tid, delta.tv_usec);

    return (void*) 0;
}

void* queue_provider(void* arg) {
    struct fqueue* queue = (struct fqueue*) arg;
    pthread_t tid = pthread_self();

    struct timeval start, stop, delta;
    info("[%ld]adding queue, pedding...", tid);
    gettimeofday(&start, NULL);

    fqueue_add(queue, NULL);

    gettimeofday(&stop, NULL);
    timeval_subtract(&delta, &start, &stop);
    info("[%ld]added a queue node, time(us): %ld", tid, delta.tv_usec);

    return (void*) 0;
}

#ifdef __cplusplus
};
#endif

int main(void) {

    threadpool_t* pool = threadpool_create(2, 10);
    threadpool_init(pool);

    sleep(2);

    for (int i = 0; i < 4; ++i) {
        threadtask_t* task;

        task = threadtask_create(test_run, (void*) i,
                (i % 3) ? TASK_RUN_IMMEDIATELY
                        : TASK_RUN_DELAY, 3000000);
        threadpool_add_task(pool, task);
    }

    /*struct fqueue *queue = fqueue_create_fixed(1, 1);

    for (int i = 0; i < thread_num; ++i) {
        pthread_t tid;
        pthread_create(&tid, NULL, queue_provider, queue);
        pthread_create(&tid, NULL, queue_consumer, queue);
    }*/

    /*sleep(5);
    fqueue_pop(queue);
    sleep(5);
    fqueue_pop(queue);*/

    while (1) {sleep(3);};
    return 0;
}

#endif /* DEBUG_THREADPOOL */