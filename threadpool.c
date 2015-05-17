#include <time.h>
#include <unistd.h>

#include "common/common.h"
#include "common/fmem.h"
#include "common/flog.h"
#include "webc.h"
#include "threadpool.h"

//hash_type_t task_hash_type = {str_hash_func, str_cmp_func, str_dup_func, str_set_func, str_free_func, str_free_func};

static void *thread_proc(void *arg) {
    struct thread_pool *pool = ((struct thread_arg *) arg)->pool;
    struct thread_item *thread = ((struct thread_arg *) arg)->thread;

    log("thread create");
    while (thread->status == THREAD_RUN) {
        /* fetch task */
        struct thread_task *task;
        pthread_mutex_lock(&thread->task_lock);
        do {
            task = fqueue_pop(thread->task_list);
            log("thread[%d] pedding", thread->tid);
            pthread_cond_wait(&thread->task_cond, &thread->task_lock);
            log("thread[%d] wakeup", thread->tid);
        } while (task == NULL);
        pthread_mutex_unlock(&thread->task_lock);

        log("thread[%d] executing", thread->tid);
        task->routine(task->arg);
    }

//    ffree(arg);

    return (void *) 0;
}

/* TODO..thread detach */
thread_pool_t *thread_pool_create() {
    struct thread_pool *pool = fmalloc(sizeof(struct thread_pool));
    if (pool == NULL) return NULL;

    fmemset(pool, 0, sizeof(struct thread_pool));

    pool->min = THREAD_POOL_MIN;
    pool->max = THREAD_POOL_MAX;
    pool->active = pool->min;
    pool->task_num = 0;
    if ((pool->tasks = fcalloc(THREAD_POOL_MAX, sizeof(struct fqueue))) == NULL)
        goto faild;
    if ((pool->args = fcalloc((size_t) pool->max, sizeof(struct thread_arg))) == NULL)
        goto faild;
    if ((pool->threads = fcalloc((size_t) pool->max, sizeof(struct thread_item))) == NULL)
        goto faild;

    return pool;

    faild:
    ffree(pool->tasks);
    ffree(pool->threads);
    ffree(pool->args);
    ffree(pool);
    return NULL;
}

int thread_pool_init(thread_pool_t *pool) {
    struct thread_item *threads = pool->threads;

    /* init minimum number of threads */
    for (int i = 0; i < pool->min; ++i) {
        struct thread_arg *arg = &pool->args[i];
        arg->pool = pool;
        arg->thread = &threads[i];

        struct thread_item *thread = &threads[i];
        thread->rountine = thread_proc;
        thread->handle = pthread_create(&threads[i].tid, NULL, thread->rountine, &pool->args[i]);
        thread->task_list = &pool->tasks[i];
        /* TODO...create faild check */
        thread->task_list = fqueue_create();

        pthread_mutex_init(&thread->task_lock, NULL);
        pthread_cond_init(&thread->task_cond, NULL);
    }
    return 0;
}

thread_task_t *thread_task_create(thread_func_t func, void *arg, int run_type, void *extra) {
    struct thread_task *task = fmalloc(sizeof(struct thread_task));
    if (task == NULL) return NULL;

    task->run_type = run_type;
    task->routine = func;
    task->start_time = time(NULL);
    task->arg = arg;
    task->extra = extra;
    return task;
}

/**
 * there only one thread to add task to thread pool
 */
int thread_pool_add_task(thread_pool_t *pool, thread_task_t *task) {
    __sync_add_and_fetch(&pool->task_num, 1);

    int task_num = pool->task_num;
    struct thread_item *thread;
    if (task_num > pool->min && task_num < pool->max) { /* init new thread */
        __sync_add_and_fetch(&pool->active, 1);

        thread = &pool->threads[task_num];
        pool->args[task_num].pool = pool;
        pool->args[task_num].thread = thread;

        /* new thread created */
        thread->rountine = thread_proc;
        thread->task_list = &pool->tasks[pool->active];
        thread->status = THREAD_RUN;
        thread->task_list = fqueue_create();
        if (pthread_mutex_init(&thread->task_lock, NULL) == -1) {
            __sync_add_and_fetch(&pool->task_num, -1);
            __sync_add_and_fetch(&pool->active, -1);
            return -1;
        }
        if (pthread_cond_init(&thread->task_cond, NULL) == -1) {
            __sync_add_and_fetch(&pool->task_num, -1);
            __sync_add_and_fetch(&pool->active, -1);
            return -1;
        }

        thread->handle = pthread_create(&thread->tid, NULL, thread->rountine, &pool->args[task_num]);

    } else {
        /* round robin strategy */
        log("round robin strategy, %d", task_num % pool->active);
        thread = &pool->threads[task_num % pool->active];
    }

    pthread_mutex_lock(&thread->task_lock);
    fqueue_add(thread->task_list, task);
    flist_info(thread->task_list->data);
    log("task added, signal thread : %d", task_num % pool->active);
    pthread_cond_signal(&thread->task_cond);
    pthread_mutex_unlock(&thread->task_lock);

    return 1;
}

#ifdef DEBUG_THREAD_POOL

void *test_run(void *arg) {
    log("task...%d", (int) arg);

    return (void *) 0;
}

int main(void) {
    thread_pool_t *pool = thread_pool_create();
    thread_pool_init(pool);

    for (int i = 0; i < 1; ++i) {
        thread_task_t *task = thread_task_create(test_run, (void *) i, TASK_RUN_IMMEDIATELY, NULL);
        thread_pool_add_task(pool, task);
    }

    sleep(3);

    return 0;
}

#endif /* DEBUG_THREAD_POOL */