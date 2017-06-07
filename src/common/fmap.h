#ifndef FMAP_H
#define FMAP_H

#include <inttypes.h>
#include <sys/types.h>
#include <stdlib.h>
#include <pthread.h>
#include "fmem.h"

#ifdef __cplusplus
extern "C" {
namespace fdis {
#endif

typedef struct fmap_type {
    unsigned int (* hash_func)(const void* key);

    int (* cmpkey)(const void* key1, const void* key2);

    void* (* dupkey)(const void* key);

    void (* dupval)(struct fmap_node* n, void* value);

    void (* deskey)(void* key);

    void (* desval)(void* value);
} hash_type_t;

struct fmap_node {
    void* key;
    void* value;
    volatile int ref;
    struct fmap_node* next;
};

struct fmap_slot {
    struct fmap_node entry;
    struct fmap_node* tail;
    pthread_mutex_t lock;
    size_t size;
};

struct fmap_section {
    struct fmap_slot* map;
    size_t size;
    size_t size_mask;
    volatile size_t used;
};

/* max size of hash is (SIZE_MAX-1),
 * because of SIZE_MAX(-1) means memory faild */
typedef struct fmap {
    struct fmap_section section[2];
    struct fmap_section trash;
    volatile int rehash_flag;
    size_t rehash_idx;     // if rehash needed */
    hash_type_t type;
    pthread_mutex_t lock;
} fmap_t;

typedef struct fmap_iter {
    struct fmap_node* current;
    size_t no;              // current node's rank
    size_t iter_idx;
} hash_iter_t;

typedef enum {
    FMAP_DUP_NONE = 0,
    FMAP_DUP_KEY = 1,
    FMAP_DUP_VAL = 2,
    FMAP_DUP_BOTH = FMAP_DUP_KEY | FMAP_DUP_VAL
} fmap_dup_type;

typedef enum {
    FMAP_T_INT32 = 1,
    FMAP_T_INT64,
    FMAP_T_STR,
    FMAP_T_STR_INCASE,
} fmap_key_type;
/****** error code *****/
#define FMAP_FAILD -1       /* memory faild */
#define FMAP_NONE 0         /* no result */
#define FMAP_OK 1
#define FMAP_EXIST 2        /* already exist */

/******************** constants **************************/
#define FMAP_SECTION_INITIAL_SIZE 4
/************************macros***************************/
/* if dupkey is not null,
 * then means string should be duplicated,
 * either the string just linked */
void* fmap_setkey(struct fmap* map, struct fmap_node* node, void* key);

void* fmap_setval(struct fmap* map, struct fmap_node* node, void* value);

/* if deskey is not null,
 * means string had been duplicated,
 * then memeory should be freed,
 * either set the pointer NULL */
#define fmap_freekey(map, node)  do{         \
    if((map)->type.deskey) (map)->type.deskey((node)->key); else (node)->key = NULL;          \
} while (0)

#define fmap_freeval(map, node)  do{         \
    if((map)->type.desval) (map)->type.desval((node)->value); else (node)->value = NULL;      \
} while (0)

#define fmap_cmpkey(map, key1, key2) ((map)->type.cmpkey(key1, key2))
#define fmap_hash(map, key) ((map)->type.hash_func(key))
#define fmap_isempty(map) ((map)->header[0].used + (map)->header[1].used == 0)
#define fmap_size(map) ((map)->section[0].used + (map)->section[1].used)

#define fmap_node_ref(node) ((node) != NULL ? (INC((node)->ref), (node)) : NULL)

#define fmap_node_free(map, node) do {                  \
    fmap_freekey(map, node);                            \
    fmap_freeval(map, node);                            \
    ffree(node);                                        \
} while(0)

#define fmap_node_des(map, node) do {                   \
    printf("ref: %d\n", node->ref);                     \
    if ((node) != NULL && DEC((node)->ref) == 0) {      \
        fmap_node_free(map, node);                      \
        (node) = NULL;                                  \
    }                                                   \
} while (0)
//        printf("key:%s free\n", (char*)(node)->key);    \

/******************** API ****************************/
int fmap_isrehash(struct fmap* map);

void fmap_init(struct fmap* map, fmap_key_type key_type, fmap_dup_type dup_mask);

struct fmap* fmap_create(fmap_key_type key_type, fmap_dup_type dup_mask);

void fmap_empty(struct fmap*);

void fmap_clear(struct fmap*);

void fmap_free(struct fmap*);

/*
 * Add a key-value node at index of 'hash to index'.
 * Be sure that key's hash must be equal to p->key's
 */
int fmap_addraw(struct fmap* map, const void* key, void* value, int rep);

int fmap_add(struct fmap* map, const void* key, void* value);

int fmap_set(struct fmap* map, const void* key, void* value);

int fmap_remove(struct fmap* map, const void* key);

struct fmap_node* fmap_pop(struct fmap* map, const void* key);

struct fmap_node* fmap_get(struct fmap* map, const void* key);

void* fmap_getvalue(struct fmap* map, const void* key);

struct fmap_slot* fmap_getslot(struct fmap* map, const void* key);

hash_iter_t fmap_iter_create(struct fmap* map);

hash_iter_t* fmap_iter_next(struct fmap* map, hash_iter_t* iter);

void fmap_info_str(struct fmap* map);

void fmap_info_int(struct fmap* map);

/******************* hash func ************************/
extern unsigned int int32_hash_func(const void* x);

extern unsigned int int64_hash_func(const void* x);

extern unsigned int str_hash_func(const void* x);

extern unsigned int strcase_hash_func(const void* x);

#ifdef __cplusplus
}
};
#endif

#endif
