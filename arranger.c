#include "arranger.h"

#include "common/common.h"
#include "common/flog.h"
#include "common/fmap.h"
#include "common/fmem.h"
#include "common/flist.h"

#include "cmd.h"

#define ARRANGER_DEFAULT_TOKEN_NUM 32
#define ARRANGER_DEFAULT_WORKER_NUM 3

#define ARRANGER_HANDLE_RETRY 3

struct webc_worker;

struct webc_token {
    int id;
    void *key;

    struct webc_worker *worker;
};

#define WOKER_OFFLINE   0
#define WOKER_LOADING   1
#define WOKER_OK        2
struct webc_worker {
    void *id;
    struct flist *tokens;

    // server related
    int status;
    size_t checkpoint;

    unsigned int ip;
    unsigned short port;
    int sockfd;
};

/**
 * center schelder for requests
 */
struct webc_arranger {
    struct webc_token *tokens;
    size_t token_num; //not changed at runtime

    struct webc_worker *workers;
    size_t worker_num;
};

struct webc_worker *worker_create_array(size_t len) {
    struct webc_worker *worker = fcalloc(len, sizeof(struct webc_worker));
    // TODO.. field
    return worker;
}

struct webc_token *token_create_array(size_t len) {
    struct webc_token *token = fcalloc(len, sizeof(struct webc_token));
    // TODO.. field
    return token;
}

struct webc_arranger *arranger_create() {
    struct webc_arranger *ager = fmalloc(sizeof(struct webc_arranger));
    if (ager == NULL) return NULL;

    // TODO.. null check
    ager->token_num = ARRANGER_DEFAULT_TOKEN_NUM;
    ager->tokens = token_create_array(ager->token_num);
    ager->worker_num = ARRANGER_DEFAULT_WORKER_NUM;
    ager->workers = worker_create_array(ager->worker_num);

    return ager;
}


///////////////// Implement ///////////////////////////////////////////////////////////////////
static struct webc_token *match_token(struct webc_arranger *ager, const char *key) {
    unsigned int hash = str_hash_func(key);
    return &ager->tokens[hash % ager->token_num];
}

static void sort_worker(struct webc_worker *workers, size_t len) {
    struct webc_worker tmp;
    int sweap;
    for (size_t i = 0; i < len - 1; ++i) {
        sweap = 0;
        for (size_t j = i + 1; j < len - 1; ++j) {
            if (flist_len(workers[j].tokens) > flist_len(workers[j + i].tokens)) {
                fmemcpy(&tmp, &workers[j], sizeof(struct webc_worker));
                fmemcpy(&workers[j], &workers[j + 1], sizeof(struct webc_worker));
                fmemcpy(&workers[j + 1], &tmp, sizeof(struct webc_worker));
                sweap = 1;
            }
        }
        if (!sweap) break;
    }
}

static void load_balance(struct webc_arranger *ager, struct flist *tokens) {
    if (tokens == NULL) return;

    sort_worker(ager->workers, ager->worker_num);

    int i = 0;
    struct flist_node *node = NULL;
    while ((node = flist_pop_head(tokens))) {
        int delta = flist_len(ager->workers[ager->worker_num - 1].tokens) -
                    flist_len(ager->workers[i].tokens);
        if (delta == 0) delta = 1;

        for (int k = 0; k < delta; ++k) {
            struct webc_worker *worker = &ager->workers[i % ager->worker_num];
            if (worker->status != WOKER_OK) {
                //TODO..while there're no workers in good status....
                ++i;
                continue;
            }

            ((struct webc_token *) node->data)->worker = worker;
            flist_add_head(worker->tokens, node->data);

            node = flist_pop_head(tokens);
            if (node == NULL) break;
        }
        if (node == NULL) break;

        ++i;
    }
}

void arranger_init(struct webc_arranger *ager) {
    // TODO...local var token_list mem release
    struct flist *token_list = flist_create();

    for (size_t i = 0; i < ager->token_num; ++i) {
        flist_add_head(token_list, (void *) &ager->tokens[i]);
    }
    load_balance(ager, token_list);
}

static void failover(struct webc_arranger *ager, struct webc_worker *worker) {
    //TODO.. find the best worker

    struct flist *token_list = worker->tokens;
    worker->tokens = NULL;

    load_balance(ager, token_list);
}

int arrager_handle(struct webc_arranger *ager, struct webc_cmd * cmd) {
    unsigned int hash = str_hash_func(cmd->key);
    log("key:[%s] to hash value: %u", cmd->key, hash);
    struct webc_token *token = &ager->tokens[hash % ager->token_num];

    int retry = 0;
    while (retry < ARRANGER_HANDLE_RETRY) {
        // after failover, worker could be changed.
        struct webc_worker *worker = token->worker;

        //节点加载状态
        if (worker->status == WOKER_LOADING) {
            // TODO..还没想好怎么实现这个状态的failover
            ++retry;
        }

        // 节点状态下线, 进行故障转移, token迁移
        if (worker->status == WOKER_OFFLINE) {
            failover(ager, worker);
            ++retry;
        }




        break;
    }

}

#ifdef DEBUG_ARRANGER

int main(void) {
    struct webc_arranger *ager = arranger_create();
    arranger_init(ager);

    return 0;
}

#endif /* DEBUG_ARRANGER */