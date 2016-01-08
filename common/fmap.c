#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <ctype.h>

#include "fmap.h"
#include "fmem.h"

#ifdef __cplusplus
namespace fankux{
#endif

static const unsigned int hash_seed = 5381;
static const unsigned int hash_rand_seed = 5731;

//private function
static void _reset_header(struct fmap_header*);

static void _release_header(struct fmap_header*);

static struct fmap_node* _find(struct fmap*, const void*, unsigned int*);

static int _expand(struct fmap*, const size_t);

static void _rehash_step(struct fmap*, const int);

/****************Type Functions *****************/
static inline int int64_cmp_func(const void* av, const void* bv) {
    int64_t a = *((int64_t*) av);
    int64_t b = *((int64_t*) bv);
    return a > b ? 1 : (a < b ? -1 : 0);
}

static inline void int64_set_func(struct fmap_node* node, void* integer) {
    if (!(node->value = fmalloc(sizeof(int64_t)))) return;
    memcpy(node->value, integer, sizeof(int64_t));
}

static inline void* int64_dup_func(const void* integer) {
    char* p = NULL;
    if (!(p = fmalloc(sizeof(int64_t)))) return NULL;
    memcpy(p, integer, sizeof(int64_t));

    return p;
}

static inline int int32_cmp_func(const void* av, const void* bv) {
    int32_t a = *((int32_t*) av);
    int32_t b = *((int32_t*) bv);
    return a > b ? 1 : (a < b ? -1 : 0);
}

static inline void int32_set_func(struct fmap_node* node, void* integer) {
    if (!(node->value = fmalloc(sizeof(int32_t)))) return;
    memcpy(node->value, integer, sizeof(int32_t));
}

static inline void* int32_dup_func(const void* integer) {
    char* p = NULL;
    if (!(p = fmalloc(sizeof(int32_t)))) return NULL;
    memcpy(p, integer, sizeof(int32_t));

    return p;
}

static inline void int_free_func(void* integer) {
    ffree(integer);
}

static inline int str_cmp_func(const void* str1, const void* str2) {
    return strcmp((char*) str1, (char*) str2);
}

static inline int strcase_cmp_func(const void* str1, const void* str2) {
    return strcasecmp((char*) str1, (char*) str2);
}

static inline void str_set_func(struct fmap_node* node, void* str) {
    size_t len = strlen((char*) str);
    if (!(node->value = fmalloc(sizeof(len) + 1))) return;
    cpystr(node->value, str, len);
}

static inline void* str_dup_func(const void* str) {
    size_t len = strlen((char*) str);
    char* p = NULL;
    if (!(p = fmalloc(len + 1))) return NULL;
    cpystr(p, str, len);

    return p;
}

static inline void str_free_func(void* str) {
    ffree(str);
}

/*************** Hash *********************/
/* Thomas Wang's 32 bit Mix Function */
inline unsigned int int32_hash_func(const void* x) {
    uint32_t key = *((uint32_t*) x);

    key += ~(key << 15);
    key ^= (key >> 10);
    key += (key << 3);
    key ^= (key >> 6);
    key += ~(key << 11);
    key ^= (key >> 16);

    return key;
}

/* Thomas Wang's 64 bit Mix Function: http://www.cris.com/~Ttwang/tech/inthash.htm */
inline unsigned int int64_hash_func(const void* x) {
    uint64_t key = *((uint64_t*) x);

    key += ~(key << 32);
    key ^= (key >> 22);
    key += ~(key << 13);
    key ^= (key >> 8);
    key += (key << 3);
    key ^= (key >> 15);
    key += ~(key << 27);
    key ^= (key >> 31);

    return (unsigned int)key;
}

/* djb hash */
inline unsigned int str_hash_func(const void* buf) {
    unsigned int hash = hash_seed;
    unsigned char* p = (unsigned char*) buf;

    while (*p) /* hash = hash * 33 + c */
        hash = ((hash << 5) + hash) + *p++;

    return hash;
}

/* case insensitive hash function (based on djb hash) */
inline unsigned int strcase_hash_func(const void* buf) {
    unsigned int hash = hash_seed;
    unsigned char* p = (unsigned char*) buf;

    while (*p) /* hash * 33 + c */
        hash = ((hash << 5) + hash) + (tolower(*p++));
    return hash;
}

/****************** API Implementations ********************/
static void _reset_header(struct fmap_header* header) {
    header->map = NULL;
    header->size = 0;
    header->size_mask = 0;
    header->used = 0;
}

static void _release_header(struct fmap_header* header) {
    ffree(header->map);
    _reset_header(header);
}

inline void fmap_init(struct fmap* map, fmap_key_type key_type, fmap_dup_type dup_mask) {
    _reset_header(&map->header[0]);
    _reset_header(&map->header[1]);
    map->rehash_idx = -1;
    map->iter_num = 0;
    memset(&map->type, 0, sizeof(map->type));

    switch (key_type) {
    case FMAP_T_INT32:
        map->type.hash_func = int32_hash_func;
        map->type.cmpkey = int32_cmp_func;
        if (dup_mask & FMAP_DUP_KEY) {
            map->type.dupkey = int32_dup_func;
            map->type.deskey = int_free_func;
        }
        break;
    case FMAP_T_INT64:
        map->type.hash_func = int64_hash_func;
        map->type.cmpkey = int64_cmp_func;
        if (dup_mask & FMAP_DUP_KEY) {
            map->type.dupkey = int64_dup_func;
            map->type.deskey = int_free_func;
        }
        break;
    case FMAP_T_STR:
        map->type.hash_func = str_hash_func;
        map->type.cmpkey = str_cmp_func;
        if (dup_mask & FMAP_DUP_KEY) {
            map->type.dupkey = str_dup_func;
            map->type.deskey = str_free_func;
        }
        break;
    case FMAP_T_STR_INCASE:
        map->type.hash_func = strcase_hash_func;
        map->type.cmpkey = strcase_cmp_func;
        if (dup_mask & FMAP_DUP_KEY) {
            map->type.dupkey = str_dup_func;
            map->type.deskey = str_free_func;
        }
        break;
    default:
        break;
    }

    if (dup_mask & FMAP_DUP_VAL) {
        map->type.dupval = str_set_func;
        map->type.desval = str_free_func;
    }

    struct fmap_header* h = &map->header[0];
    if ((h->map = fmalloc(FMAP_HEADER_INITIAL_SIZE * sizeof(struct fmap_node*))) == NULL) {
        return;
    }

    fmemset(h->map, 0, FMAP_HEADER_INITIAL_SIZE * sizeof(struct fmap_node*));
    h->size = FMAP_HEADER_INITIAL_SIZE;
    h->size_mask = h->size - 1;

}

/*create a fmap which value linked */
struct fmap* fmap_create(fmap_key_type key_type, fmap_dup_type dup_mask) {
    struct fmap* map;
    if (!(map = fmalloc(sizeof(struct fmap)))) return NULL;

    fmap_init(map, key_type, dup_mask);

    if (map->header[0].map == NULL) {
        ffree(map);
        return NULL;
    }
    return map;
}

void fmap_empty(struct fmap* map) {
    struct fmap_node* p, * q;
    for (int j = 0; j < 2; ++j) {
        struct fmap_header* h = &map->header[j];
        for (size_t i = 0; i < h->size; ++i) {
            p = h->map[i];
            while (p) {
                q = p;
                p = p->next;
                fmap_freekey(map, q);
                fmap_freeval(map, q);
                --h->used;
            }
            h->map[i] = NULL;
        }
    }
}

void fmap_free(struct fmap* map) {
    if (map == NULL) return;

    struct fmap_node* p, * q;
    for (int j = 0; j < 2; ++j) {
        struct fmap_header* h = &map->header[j];
        for (size_t i = 0; i < h->size; ++i) {
            p = h->map[i];
            while (p) {
                q = p;
                p = p->next;
                fmap_freekey(map, q);
                fmap_freeval(map, q);
                --h->used;
            }
        }
        ffree(h->map);
    }
    ffree(map);
}

static void _rehash_step(struct fmap* map, const int step) {
    struct fmap_header* org = &map->header[0];
    struct fmap_header* new = &map->header[1];
    struct fmap_node* p = NULL;
    size_t index;
    int i = 0;

    if (!fmap_isrehash(map) || step == 0) return;

    while (i < step) {
        p = org->map[map->rehash_idx];
        while (p && i < step) {
            org->map[map->rehash_idx] = p->next;
            index = new->size_mask & fmap_hash(map, p->key);
            p->next = new->map[index];
            new->map[index] = p;
            p = org->map[map->rehash_idx];

            --org->used;
            ++new->used;
            ++i;
        }
        if (org->map[map->rehash_idx]) continue;

        do {
            /* rehash complete */
            if (++map->rehash_idx >= org->size) {
                map->rehash_idx = -1;

                _release_header(&map->header[0]);
                map->header[0] = map->header[1];
                _reset_header(&map->header[1]);

                return;
            }
        } while (!org->map[map->rehash_idx]);
    }
}

static int _expand(struct fmap* map, const size_t len) {
    //is rehash, expand action denied
    if (fmap_isrehash(map)) return FMAP_OK;

    struct fmap_node** new = NULL;
    if (!(new = fmalloc(len * sizeof(struct fmap_node*)))) return 0;
    fmemset(new, 0, len * sizeof(struct fmap_node*));

    struct fmap_header* h = &map->header[1];
    h->map = new;
    h->size = len;
    h->size_mask = len - 1;
    h->used = 0;

    map->rehash_idx = 0;
    return FMAP_OK;
}

/* find the key, the "hash" stores the key hash value */
static struct fmap_node* _find(struct fmap* map, const void* key, unsigned int* hash) {
    struct fmap_header* h = &map->header[0];
    struct fmap_node* p;
    unsigned int hash_code;
    int flag, hflag = 1;

    hash_code = fmap_hash(map, key);
    if (hash) *hash = hash_code;

    do {
        p = h->map[(unsigned int) (h->size_mask & hash_code)];
        while (p) {
            if (0 == fmap_cmpkey(map, p->key, key))
                return p;
            p = p->next;
        }
        flag = 0;
        if (fmap_isrehash(map) && hflag) {
            h = &map->header[1];
            flag = 1;
            hflag = 0;
        }
    } while (flag);

    return NULL;
}

/*
 * Add a key-value node at index of 'hash to index'.
 * Be sure that key's hash must be equal to p->key's
 */
int fmap_addraw(struct fmap* map, const unsigned int hash, const void* key, void* value) {
    struct fmap_header* h;
    if (fmap_isrehash(map)) {
        h = &map->header[1];
    } else {
        h = &map->header[0];
    }

    size_t index = hash & h->size_mask;
    struct fmap_node* new;
    if (!(new = fmalloc(sizeof(struct fmap_node))))
        return FMAP_FAILD;/* allocate one */

    fmap_setkey(map, new, key);
    if (map->type.dupkey != NULL && new->key == NULL) {
        /* key never be null if key is string */
        ffree(new);
        return FMAP_FAILD;
    }
    fmap_setval(map, new, value);

    new->next = h->map[index];
    h->map[index] = new;
    ++h->used;

    if (h->used >= h->size) {
        _expand(map, h->size << 1);
    }

    if (fmap_isrehash(map)) {
        // in here, h->size may be reset to 0
        _rehash_step(map, 1);
    }

    return FMAP_OK;
}

int fmap_add(struct fmap* map, const void* key, void* value) {
    unsigned int hash;
    struct fmap_node* p = _find(map, key, &hash);
    if (p) return FMAP_EXIST;// this key already exist, add faild

    return fmap_addraw(map, hash, key, value);
}

int fmap_set(struct fmap* map, const void* key, void* value) {
    unsigned int hash;
    struct fmap_node* p = _find(map, key, &hash);
    if (p) {
        // this key already exist, free first
        fmap_freeval(map, p);
        fmap_setval(map, p, value);
        return FMAP_OK;
    }

    return fmap_addraw(map, hash, key, value);
}

inline struct fmap_node* fmap_get(struct fmap* hash, const void* key) {
    return _find(hash, key, NULL);
}

void* fmap_getvalue(struct fmap* map, const void* key) {
    struct fmap_node* node = _find(map, key, NULL);
    if (!node) return NULL;
    return node->value;
}

struct fmap_node* fmap_getslot(struct fmap* map, const void* key) {
    struct fmap_header* h = &map->header[0];

    unsigned int hash_code = fmap_hash(map, key);

    return h->map[h->size_mask & hash_code];
}

/* pop a node,
** the memory should be controlled by caller manually */
struct fmap_node* fmap_pop(struct fmap* hash, const void* key) {
    struct fmap_header* h = NULL;
    if (fmap_isrehash(hash)) h = &hash->header[1];
    else h = &hash->header[0];

    size_t index = h->size_mask & fmap_hash(hash, key);
    struct fmap_node* p = h->map[index];
    struct fmap_node* q = p;
    while (p) {
        if (!fmap_cmpkey(hash, p->key, key)) break;

        q = p;
        p = p->next;
    }

    //not exist
    if (!p) return NULL;

    if (q != p) q->next = p->next;
    else h->map[index] = p->next;

    --h->used;

    return p;
}

int fmap_remove(struct fmap* hash, const void* key) {
    struct fmap_node* p;
    if (!(p = fmap_pop(hash, key))) return FMAP_NONE;

    fmap_freeval(hash, p);
    fmap_freekey(hash, p);

    return FMAP_OK;
}

/**************** iterator implement ******************/
/* pos is index of node where starting iter */
hash_iter_t* fmap_iter_create(struct fmap* dict, size_t pos) {
    size_t i, level = 0;
    hash_iter_t* iter;
    struct fmap_header* h = &dict->header[0];

    if (pos >= h->used) {
        level = h->size;
        pos -= h->used;
        h = &dict->header[1];
        if (pos >= h->used)
            return NULL; /* out of range */
    }
    if (!(iter = fmalloc(sizeof(hash_iter_t)))) return NULL;

    for (i = 0; i < h->size; ++i) {
        if (!h->map[i]) continue;
        else break;
    }

    iter->no = 0;
    iter->iter_idx = i + level;
    iter->current = h->map[i];

    /* for(i = 0; i < pos; ++i) */
    /* 	iter = fdict_iter_next(dict, iter); */

    return iter;
}

hash_iter_t* fmap_iter_next(struct fmap* hash, hash_iter_t* iter) {
    struct fmap_header* h;
    int flag = 0, hflag = 1;
    size_t i, level = 0;

    if (!iter) return NULL;
    /* no more */
    if (iter->no >= hash->header[0].used + hash->header[1].used - 1) {
        ffree(iter);
        return NULL;
    }

    h = &hash->header[0];
    if (iter->iter_idx >= h->size) {
        level = h->size;
        h = &hash->header[1];
        hflag = 0;
    }

    for (i = iter->iter_idx - level; i < h->size;) {
        if (!h->map[i]) {
            ++i;
            continue;
        }

        if (flag == 1) {
            iter->current = h->map[i];
            iter->iter_idx = i + level;
            ++iter->no;
            return iter;
        }

        if (iter->current->next) {
            iter->current = iter->current->next;
            iter->iter_idx = i + level;
            ++iter->no;
            return iter;
        } else {
            ++i;
            flag = 1;
        }

        if (i == h->size && fmap_isrehash(hash) && hflag) {
            h = &hash->header[1];
            hflag = 0;
            i = 0;
            level = hash->header[0].size;
        }
    }

    /* iteration complete */
    fmap_iter_cancel(hash, iter);
    return NULL;
}

void fmap_iter_cancel(struct fmap* hash, hash_iter_t* iter) {
    __sync_sub_and_fetch(&hash->iter_num, 1);
    ffree(iter);
}

/****************Test use ******************/
void fmap_info_str(struct fmap* map) {
    printf("rehash_idx:%zd; iter_num:%zu\n", map->rehash_idx, map->iter_num);
    struct fmap_header* h;
    for (int j = 0; j < 2; ++j) {
        h = &map->header[j];
        printf("header:%d: size:%zu; used:%zu\n", j, h->size, h->used);

        for (int i = 0; i < h->size; ++i) {
            struct fmap_node* p = h->map[i];
            printf("%4d(p:%9d):", i, (int) p);
            while (p) {
                printf("%s:%s->", (char*) p->key, (char*) p->value);
                p = p->next;
            }
            printf("\n");
        }
    }
}

void fmap_info_int(struct fmap* map) {
    printf("rehash_idx:%zd; iter_num:%zu\n", map->rehash_idx, map->iter_num);
    struct fmap_header* h;
    for (int j = 0; j < 2; ++j) {
        h = &map->header[j];
        printf("h%d: size:%zu; used:%zu\n", j, h->size, h->used);
        for (int i = 0; i < h->size; ++i) {
            struct fmap_node* p = h->map[i];
            printf("%4d(p:%9d):", i, (int) p);
            while (p) {
                printf("%d:%d->", *((int*) p->key), (int) &p->value);
                p = p->next;
            }
            printf("\n");
        }
    }
}

#ifdef __cplusplus
}
#endif
