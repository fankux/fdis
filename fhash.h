#ifndef FHASH_H
#define FHASH_H

#include <inttypes.h>

struct hash_node {
    void * key;
    void * value;
    struct hash_node * next;
};

struct hash_header {
    struct hash_node ** map;
    size_t size;
    size_t size_mask;
    size_t used;
};

typedef struct hash_type {
    unsigned int (* hash_func)(void * key);
    int    (* cmpkey)(void * key1, void * key2);
    void * (* dupkey)(void * key);
    void   (* setval)(struct hash_node * n, void * value);
    void   (* deskey)(void * key);
    void   (* desval)(void * value);
} hash_type_t;

/* max size of hash is (SIZE_MAX-1),
 * because of SIZE_MAX(-1) means memory faild */
struct fhash {
    struct hash_header header[2];
    ssize_t rehash_idx;   // if rehash needed */
    size_t iter_num;       // iterators' number */
    hash_type_t * type;
};

typedef struct hash_iter {
    struct hash_node * current;
    size_t no;           //current node's rank
    size_t iter_idx;
} hash_iter_t;

/****** error code *****/
#define FHASH_FAILD -1 /* memory faild */
#define FHASH_NONE 0  /* no result */
#define FHASH_OK 1
#define FHASH_EXIST 2 /* already exist */

/******************** constants **************************/
#define FHASH_HEADER_INITIAL_SIZE 4
/************************macros***************************/
/* if dupkey is not null,
 * then means string should be duplicated,
 * either the string just linked */
#define fhash_setkey(h, node, key) do{      \
    if((h)->type->dupkey) (node)->key = (h)->type->dupkey(key); else (node)->key = key;     \
}while(0)

#define fhash_setval(h, node, value) do{    \
    if((h)->type->setval) (h)->type->setval((node), (value)); else (node)->value = (value); \
}while(0)

/* if deskey is not null,
 * means string had been duplicated,
 * then memeory should be freed,
 * either set the pointer NULL */
#define fhash_freekey(h, node)  do{         \
    if((h)->type->deskey) (h)->type->deskey((node)->key); else (node)->key = NULL;          \
} while (0)

#define fhash_freeval(h, node)  do{         \
    if((h)->type->desval) (h)->type->desval((node)->value); else (node)->value = NULL;      \
} while (0)

#define fhash_cmpkey(h, key1, key2) ((h)->type->cmpkey(key1, key2))
#define fhash_hash(h, key) ((h)->type->hash_func(key))
#define fhash_isrehash(h) ((h)->rehash_idx != -1)
#define fhash_isempty(h) ((h)->header[0].used + (h)->header[1].used == 0)
#define fhash_len(h) ((h)->header[0].used + (h)->header[1].used)

/******************** API ****************************/
struct fhash * fhash_create();
struct fhash * fhash_create_dupkey();
struct fhash * fhash_create_dupboth();
void fhash_empty(struct fhash *);
void fhash_free(struct fhash *);
int fhash_addraw(struct fhash * hash, const unsigned int hash_code, void * key, void * value);
int fhash_add(struct fhash * hash, void * key, void * value);
int fhash_remove(struct fhash * hash, void * key);
struct hash_node * fhash_pop(struct fhash * hash, void * key);
struct hash_node * fhash_get(struct fhash * hash, void * value);
void * fhash_getvalue(struct fhash * hash, void * value);
struct hash_node * fhash_getrand(struct fhash * hash);
int fhash_set(struct fhash * hash, void * key, void * value);
int fhash_replace(struct fhash * hash, void * key, void * value);
hash_iter_t * fhash_iter_create(struct fhash * hash, size_t pos);
hash_iter_t * fhash_iter_next(struct fhash * hash, hash_iter_t * iter);
void fhash_iter_cancel(struct fhash * hash, hash_iter_t * iter);
void fhash_info_str(struct fhash * hash);
void fhash_info_int(struct fhash * hash);

/******************* hash type ************************/
extern unsigned int _int_hash_func(void * x);
extern unsigned int _str_hash_func(void * x);
extern unsigned int _strcase_hash_func(void * x);
extern void _str_set(struct hash_node * n, void * str);
extern void * _str_dup(void * str);
extern void _str_free(void * str);

#endif
