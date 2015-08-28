#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

#include "common/common.h"
#include "common/fmem.h"
#include "common/flog.h"
#include "common/fbit.h"
#include "common/fqueue.h"
#include "common/check.h"
#include "webc.h"
#include "threadpool.h"

// FIXME..repeat creating thread

//TODO..futex replace mutex


/* TODO..thread detach */
threadpool_t *threadpool_create() {
    struct threadpool *pool = fmalloc(sizeof(struct threadpool));
    check_null_ret_oom(pool, NULL, "thread pool");

    fmemset(pool, 0, sizeof(struct threadpool));

    pool->min = THREAD_POOL_MIN;
    pool->max = THREAD_POOL_MAX;
    pool->act_num = pool->min;
    pool->task_num = 0;
    pthread_mutex_init(&pool->lock, NULL);

    pool->bits = fbit_create(THREAD_POOL_MAX);
    check_null_goto_oom(pool->bits, faild, "threadpool bitmap");

    pool->threads = fcalloc(pool->max, sizeof(struct threaditem));
    check_null_goto_oom(pool->threads, faild, "threadpool thread list");
    for (int i = 0; i < THREAD_POOL_MAX; ++i) {
        pool->threads[i].no = i;
    }

    pool->args = fcalloc(pool->max, sizeof(struct threadarg));
    check_null_goto_oom(pool->args, faild, "threadpool args list");

    pool->tasks = fcalloc(pool->max, sizeof(struct fqueue *));
    check_null_goto_oom(pool->tasks, faild, "threadpool task queue list");
    for (int i = 0; i < THREAD_POOL_MAX; ++i) {
        pool->tasks[i] = fqueue_create(1);
        check_null_goto_oom(pool->tasks[i], faild, "threadpool task queue");
    }

    return pool;

    faild:
    for (int i = 0; i < THREAD_POOL_MAX; ++i) {
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

threadtask_t *threadtask_create(thread_func_t func, void *arg, int run_type,
                                void *extra) {
    struct threadtask *task = fmalloc(sizeof(struct threadtask));
    if (task == NULL) return NULL;

    task->run_type = run_type;
    task->call = func;
    task->start_time = time(NULL);
    task->arg = arg;
    task->extra = extra;
    return task;
}

static void *thread_proc(void *arg) {
    struct threadpool *pool = ((struct threadarg *) arg)->pool;
    struct threaditem *thread = ((struct threadarg *) arg)->thread;

    int tno = thread->no;

    pthread_mutex_lock(&pool->lock);
//    ++pool->act_num;
    fbit_set(pool->bits, (size_t) thread->no, 1);
    char *bit_info = fbit_info(pool->bits, 1);
    log("pool bits:%s", bit_info);
    ffree(bit_info);
    pthread_mutex_unlock(&pool->lock);

    thread->status = THREAD_RUN;

    /* TODO..exit after max idle time  */
    log("thread[%d] thread create", tno);
    while (thread->status == THREAD_RUN) {
//        sleep(1);
        /* fetch task */
        struct threadtask *task;
//        debug("thread[%d] try to accquire lock...", tno);
        pthread_mutex_lock(&thread->task_lock);
//        debug("thread[%d] success accquire lock...", tno);
        while ((task = fqueue_pop(thread->task_list)) == NULL) {
            debug("thread[%d] pedding", tno);
            pthread_cond_wait(&thread->task_cond, &thread->task_lock);
            debug("thread[%d] wakeup", tno);

//            debug("thread[%d] try to unlock...", tno);
            pthread_mutex_unlock(&thread->task_lock);
//            debug("thread[%d] success unlock...", tno);
        }

//        debug("thread[%d] try to unlock...", tno);
        pthread_mutex_unlock(&thread->task_lock);
//        debug("thread[%d] success unlock...", tno);

        log("thread[%d] executing", tno);
        task->call(task->arg);
        ffree(task);

        pthread_mutex_lock(&pool->lock);
        --pool->task_num;
        pthread_mutex_unlock(&pool->lock);
    }

    pthread_mutex_lock(&pool->lock);
    __sync_add_and_fetch(&pool->act_num, -1);
    fbit_set(pool->bits, (size_t) thread->no, 0);
    pthread_mutex_unlock(&pool->lock);

    //TODO..free threaditem mem
    thread->tid = 0;
    thread->status = THREAD_READY;

    return (void *) 0;
}

/* initiate new thread which memory had allocated already */
static struct threaditem *_thread_init(threadpool_t *pool, int tno) {
    struct threaditem *thread = &pool->threads[tno];
    int status;

    pool->args[tno].pool = pool;
    pool->args[tno].thread = thread;

    thread->no = tno;
    thread->routine = thread_proc;
    thread->task_list = pool->tasks[tno];
    thread->status = THREAD_READY;
    status = pthread_mutex_init(&thread->task_lock, NULL);
    check_cond_goto(status == 0, faild,
                    "faild to init task lock, tno:%d, errno:%d, error:%s",
                    tno, errno, strerror(errno));
    status = pthread_cond_init(&thread->task_cond, NULL);
    check_cond_goto(status == 0, faild,
                    "faild to init task cond, tno:%d, errno:%d, error:%s",
                    tno, errno, strerror(errno));
    status = pthread_create(&thread->tid, NULL, thread->routine,
                            &pool->args[tno]);
    check_cond_goto(status == 0, faild,
                    "faild to create thread, tno:%d, errno:%d, error:%s",
                    tno, errno, strerror(errno));

    fbit_set(pool->bits, tno, 1);

    return thread;

    faild:
    thread->tid = 0;
    return NULL;
}

int threadpool_init(threadpool_t *pool) {
    /* init minimum number of threads */
    fbit_set_all(pool->bits, 0);
    for (int i = 0; i < pool->min; ++i) {
        _thread_init(pool, i);
    }
    return 0;
}

/**
 * there only one thread to add task to thread pool
 * if there still thread creating spaces available, create a new thread to execute,
 * else using round robin strategy to chose a thread,
 * then queuing task to that thread
 *
 * TODO..lock free, atomic
 */
int threadpool_add_task(threadpool_t *pool, threadtask_t *task) {
    size_t worker_idx;
    struct threaditem *thread;

    pthread_mutex_lock(&pool->lock);

    int task_num = ++(pool->task_num);
    debug("task number: %d", task_num);
    if (task_num > pool->min && task_num <= pool->max) { /* init new thread */

        fbit_get_val_at_round(pool->bits, 0, 1, &worker_idx);
        fbit_set(pool->bits, worker_idx, 1);

        debug("would create new thread, tno: %d", worker_idx);
        thread = _thread_init(pool, worker_idx);
//        if (thread == NULL) {
//            __sync_add_and_fetch(&pool->task_num, -1);
//        }
    } else { /* round robin strategy */

        fbit_get_val_at_round(pool->bits, 1, task_num, &worker_idx);

        debug("thread[%d] round robin strategy", worker_idx);
        thread = &pool->threads[worker_idx];
    }

    pthread_mutex_unlock(&pool->lock);

    pthread_mutex_lock(&thread->task_lock);
    fqueue_add(thread->task_list, task);
    char *list_info = flist_info(thread->task_list->data, 1);
//    debug("thread[%zu] list_info:%s", worker_idx, list_info);
    ffree(list_info);
//    debug("thread[%zu] task added", worker_idx);
    pthread_cond_signal(&thread->task_cond);
    pthread_mutex_unlock(&thread->task_lock);

    return 1;
}

#ifdef DEBUG_THREADPOOL

void *test_run(void *arg) {
    log("task...%d", (int) arg);
    return (void *) 0;
}

void *queue_consumer(void *arg) {
    struct fqueue *queue = (struct fqueue *) arg;
    pthread_t tid = pthread_self();

    struct timeval start, stop, delta;
    log("[%ld]fetching from queue, pedding...", tid);
    gettimeofday(&start, NULL);


    fqueue_node_t *node = fqueue_pop(queue);

    gettimeofday(&stop, NULL);
    timeval_subtract(&delta, &start, &stop);
    log("[%ld]got a queue node, time(us): %ld", tid, delta.tv_usec);

    return (void *) 0;
}

void *queue_provider(void *arg) {
    struct fqueue *queue = (struct fqueue *) arg;
    pthread_t tid = pthread_self();

    struct timeval start, stop, delta;
    log("[%ld]adding queue, pedding...", tid);
    gettimeofday(&start, NULL);

    fqueue_add(queue, NULL);

    gettimeofday(&stop, NULL);
    timeval_subtract(&delta, &start, &stop);
    log("[%ld]added a queue node, time(us): %ld", tid, delta.tv_usec);

    return (void *) 0;
}

#define thread_num 3

int main(void) {

    /*threadpool_t *pool = threadpool_create();
    threadpool_init(pool);

    sleep(2);

    for (int i = 0; i < 10; ++i) {
        threadtask_t *task = threadtask_create(test_run, (void *) i,
                                               TASK_RUN_IMMEDIATELY, NULL);
        threadpool_add_task(pool, task);
    }*/

    struct fqueue *queue = fqueue_create_fixed(1, 1);

    for (int i = 0; i < thread_num; ++i) {
        pthread_t tid;
        pthread_create(&tid, NULL, queue_provider, queue);
        pthread_create(&tid, NULL, queue_consumer, queue);
    }

    /*sleep(5);
    fqueue_pop(queue);
    sleep(5);
    fqueue_pop(queue);*/

    while (1) { sleep(3); };
    return 0;
}

#endif /* DEBUG_THREADPOOL */