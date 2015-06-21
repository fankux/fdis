#include <time.h>
#include <unistd.h>

#include "common/common.h"
#include "common/fmem.h"
#include "common/flog.h"
#include "common/fbit.h"
#include "webc.h"
#include "threadpool.h"

//hash_type_t task_hash_type = {str_hash_func, str_cmp_func, str_dup_func, str_set_func, str_free_func, str_free_func};

static void *thread_proc(void *arg) {
    struct thread_pool *pool = ((struct thread_arg *) arg)->pool;
    struct thread_item *thread = ((struct thread_arg *) arg)->thread;

    int tid = (int) thread->tid;

    fbit_set(pool->bits, (size_t) thread->id, 1);


    log("thread[%d] thread create", tid);
    while (thread->status == THREAD_RUN) {
        /* fetch task */
        struct thread_task *task;
//        log("thread[%d] try to accquire lock...", tid);
        pthread_mutex_lock(&thread->task_lock);
//        log("thread[%d] success accquire lock...", tid);
        task = fqueue_pop(thread->task_list);
        if (task == NULL) {
            log("thread[%d] pedding", tid);
            pthread_cond_wait(&thread->task_cond, &thread->task_lock);
            log("thread[%d] wakeup", tid);

//            log("thread[%d] try to unlock...", tid);
            pthread_mutex_unlock(&thread->task_lock);
//            log("thread[%d] success unlock...", tid);
            continue;
        }

        log("thread[%d] executing", tid);
        task->routine(task->arg);
        ffree(task);
        __sync_add_and_fetch(&pool->task_num, -1);
    }
    __sync_add_and_fetch(&pool->act_thread_num, -1);
    fbit_set(pool->bits, (size_t) thread->id, 0);

    ffree(arg);

    return (void *) 0;
}

/* TODO..thread detach */
thread_pool_t *thread_pool_create() {
    struct thread_pool *pool = fmalloc(sizeof(struct thread_pool));
    if (pool == NULL) return NULL;

    fmemset(pool, 0, sizeof(struct thread_pool));

    pool->min = THREAD_POOL_MIN;
    pool->max = THREAD_POOL_MAX;
    pool->act_thread_num = pool->min;
    pool->task_num = 0;
    if ((pool->bits = fbit_create(THREAD_POOL_MAX)) == NULL)
        goto faild;
    if ((pool->threads = fcalloc(pool->max, sizeof(struct thread_item))) == NULL)
        goto faild;
    for (int i = 0; i < THREAD_POOL_MAX; ++i) {
        pool->threads[i].id = i;
    }
    if ((pool->tasks = fcalloc(pool->max, sizeof(struct fqueue *))) == NULL)
        goto faild;
    if ((pool->args = fcalloc(pool->max, sizeof(struct thread_arg))) == NULL)
        goto faild;
    for (int i = 0; i < THREAD_POOL_MAX; ++i) {
        if ((pool->tasks[i] = fqueue_create()) == NULL)
            goto faild;
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

int thread_pool_init(thread_pool_t *pool) {
    struct thread_item *threads = pool->threads;

    /* init minimum number of threads */
    for (int i = 0; i < pool->min; ++i) {
        struct thread_item *thread = &threads[i];
        pthread_mutex_init(&thread->task_lock, NULL);
        pthread_cond_init(&thread->task_cond, NULL);

        struct thread_arg *arg = &pool->args[i];
        arg->pool = pool;
        arg->thread = thread;

        thread->rountine = thread_proc;
        thread->task_list = pool->tasks[i];
        thread->status = THREAD_RUN;
        thread->handle = pthread_create(&threads[i].tid, NULL, thread->rountine, &pool->args[i]);
    }

    fbit_set_all(pool->bits, 0);
    fbit_move_left(pool->bits, THREAD_POOL_MAX);
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

/* initiate new thread which memory had allocated already */
struct thread_item *_init_thread(thread_pool_t *pool) {
    int active_num = __sync_fetch_and_add(&pool->act_thread_num, 1);

    struct thread_item *thread = &pool->threads[active_num];
    pool->args[active_num].pool = pool;
    pool->args[active_num].thread = thread;

    thread->rountine = thread_proc;
    thread->task_list = pool->tasks[active_num];
    thread->status = THREAD_RUN;
    if (pthread_mutex_init(&thread->task_lock, NULL) == -1) goto faild;
    if (pthread_cond_init(&thread->task_cond, NULL) == -1) goto faild;
    thread->handle = pthread_create(&thread->tid, NULL, thread->rountine, &pool->args[active_num]);

    return thread;

    faild:
    return NULL;
}

/**
 * there only one thread to add task to thread pool
 */
int thread_pool_add_task(thread_pool_t *pool, thread_task_t *task) {
    int task_num = __sync_add_and_fetch(&pool->task_num, 1);

    int worker_idx = 0;
    struct thread_item *thread;
    if (task_num > pool->min && task_num < pool->max) { /* init new thread */
        if ((thread = _init_thread(pool)) == NULL) {
            __sync_add_and_fetch(&pool->act_thread_num, -1);
            __sync_add_and_fetch(&pool->task_num, -1);
        }
    } else {
        worker_idx = task_num % pool->act_thread_num;
        /* round robin strategy */
        log("round robin strategy, %d", worker_idx);
        thread = &pool->threads[worker_idx];
    }

    pthread_mutex_lock(&thread->task_lock);
    fqueue_add(thread->task_list, task);
    flist_info(thread->task_list->data);
    log("task added, signal thread : %d", worker_idx);
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

    sleep(2);

    for (int i = 0; i < 15; ++i) {
        thread_task_t *task = thread_task_create(test_run, (void *) i, TASK_RUN_IMMEDIATELY, NULL);
        thread_pool_add_task(pool, task);
    }

    sleep(3);

    return 0;
}

#endif /* DEBUG_THREAD_POOL */