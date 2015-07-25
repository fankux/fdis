#include <time.h>
#include <pthread.h>

#include "common/common.h"
#include "common/fmem.h"
#include "common/flog.h"
#include "common/fbit.h"
#include "common/fqueue.h"
#include "common/check.h"
#include "webc.h"
#include "threadpool.h"

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
        pool->tasks[i] = fqueue_create();
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

    int tid = (int) thread->tid;

    pthread_mutex_lock(&pool->lock);
    __sync_add_and_fetch(&pool->act_num, -1);
    fbit_set(pool->bits, (size_t) thread->no, 1);
    pthread_mutex_unlock(&pool->lock);

    /* TODO..exit after max idle time  */
    log("thread[%d] thread create", tid);
    while (thread->status == THREAD_RUN) {
        /* fetch task */
        struct threadtask *task;
        debug("thread[%d] try to accquire lock...", tid);
        pthread_mutex_lock(&thread->task_lock);
        debug("thread[%d] success accquire lock...", tid);
        task = fqueue_pop(thread->task_list);
        if (task == NULL) {
            debug("thread[%d] pedding", tid);
            pthread_cond_wait(&thread->task_cond, &thread->task_lock);
            debug("thread[%d] wakeup", tid);

            debug("thread[%d] try to unlock...", tid);
            pthread_mutex_unlock(&thread->task_lock);
            debug("thread[%d] success unlock...", tid);
            continue;
        }

        log("thread[%d] executing", tid);
        task->call(task->arg);
        ffree(task);
        __sync_add_and_fetch(&pool->task_num, -1);
    }

    pthread_mutex_lock(&pool->lock);
    __sync_add_and_fetch(&pool->act_num, -1);
    fbit_set(pool->bits, (size_t) thread->no, 0);
    pthread_mutex_unlock(&pool->lock);

    //TODO..free threaditem mem
    ffree(arg);

    return (void *) 0;
}

/* initiate new thread which memory had allocated already */
static struct threaditem *_thread_init(threadpool_t *pool, int tno) {
    struct threaditem *thread = &pool->threads[tno];
    pool->args[tno].pool = pool;
    pool->args[tno].thread = thread;

    thread->routine = thread_proc;
    thread->task_list = pool->tasks[tno];
    pthread_mutex_init(&thread->task_lock, NULL);
    pthread_cond_init(&thread->task_cond, NULL);
    thread->status = THREAD_READY;

    thread->handle = pthread_create(&thread->tid, NULL, thread->routine,
                                    &pool->args[tno]);
    check_cond_goto(thread->handle == -1, faild,
                    "faild to create thread, tno:%d", tno);

    return thread;

    faild:
    return NULL;
}

int threadpool_init(threadpool_t *pool) {
    /* init minimum number of threads */
    for (int i = 0; i < pool->min; ++i) {
        _thread_init(pool, i);
    }
    fbit_set_all(pool->bits, 0);
    return 0;
}

/**
 * there only one thread to add task to thread pool
 * if there still thread creating spaces available, create a new thread to execute,
 * else using round robin strategy to chose a thread,
 * then queuing task to that thread
 */
int threadpool_add_task(threadpool_t *pool, threadtask_t *task) {
    int task_num = __sync_add_and_fetch(&pool->task_num, 1);

    int worker_idx = 0;
    struct threaditem *thread;

    if (task_num > pool->min && task_num < pool->max) { /* init new thread */
        if ((thread = _thread_init(pool, task_num - 1)) == NULL) {
            __sync_add_and_fetch(&pool->task_num, -1);
        }
    } else {
        /* TODO..round robin strategy base on bitmap,
         * TODO..find next available thread) */
        worker_idx = task_num % pool->act_num;
        debug("round robin strategy, %d", worker_idx);
        thread = &pool->threads[worker_idx];
    }

    pthread_mutex_lock(&thread->task_lock);
    fqueue_add(thread->task_list, task);
    flist_info(thread->task_list->data);
    debug("task added, signal thread : %d", worker_idx);
    pthread_cond_signal(&thread->task_cond);
    pthread_mutex_unlock(&thread->task_lock);

    return 1;
}

#ifdef DEBUG_THREADPOOL

void *test_run(void *arg) {
    log("task...%d", (int) arg);

    return (void *) 0;
}

int main(void) {
    threadpool_t *pool = threadpool_create();
    threadpool_init(pool);

    sleep(2);

    for (int i = 0; i < 1; ++i) {
        threadtask_t *task = threadtask_create(test_run, (void *) i,
                                               TASK_RUN_IMMEDIATELY, NULL);
        threadpool_add_task(pool, task);
    }

    sleep(3);

    return 0;
}

#endif /* DEBUG_THREADPOOL */