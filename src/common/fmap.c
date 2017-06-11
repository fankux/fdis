#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <ctype.h>

#include "common.h"
#include "fmap.h"

#ifdef __cplusplus
namespace fdis{
#endif

static const unsigned int hash_seed = 5381;

//private function

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

    return (unsigned int) key;
}

/* djb hash */
inline unsigned int str_hash_func(const void* buf) {
    unsigned int hash = hash_seed;
    unsigned char* p = (unsigned char*) buf;

    while (*p) { /* hash = hash * 33 + c */
        hash = ((hash << 5) + hash) + *p++;
    }
    return hash;
}

/* case insensitive hash function (based on djb hash) */
inline unsigned int strcase_hash_func(const void* buf) {
    unsigned int hash = hash_seed;
    unsigned char* p = (unsigned char*) buf;

    while (*p) { /* hash * 33 + c */
        hash = ((hash << 5) + hash) + (tolower(*p++));
    }
    return hash;
}

/****************** API Implementations ********************/
static void _reset_section(struct fmap_section* section) {
    memset(section, 0, sizeof(struct fmap_section));
}

static void _release_section(struct fmap_section* section) {
    ffree(section->map);
    _reset_section(section);
}

void fmap_init(struct fmap* map, fmap_key_type key_type, fmap_dup_type dup_mask) {
    _reset_section(&map->section[0]);
    _reset_section(&map->section[1]);
    map->rehash_flag = 0;
    map->rehash_idx = 0;
    map->lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
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

    struct fmap_section* section = &map->section[0];
    if ((section->map = fcalloc(FMAP_SECTION_INITIAL_SIZE, sizeof(struct fmap_slot))) == NULL) {
        return;
    }

    for (int i = 0; i < FMAP_SECTION_INITIAL_SIZE; ++i) {
        memset(&section->map[i].entry, 0, sizeof(struct fmap_node));
        section->map[i].lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
        section->map[i].size = 0;
        section->map[i].tail = NULL;
    }

    section->used = 0;
    section->size = FMAP_SECTION_INITIAL_SIZE;
    section->size_mask = section->size - 1;
}

/*create a fmap which value linked */
struct fmap* fmap_create(fmap_key_type key_type, fmap_dup_type dup_mask) {
    struct fmap* map;
    if ((map = fmalloc(sizeof(struct fmap))) == NULL) {
        return NULL;
    }

    fmap_init(map, key_type, dup_mask);

    if (map->section[0].map == NULL) {
        ffree(map);
        return NULL;
    }
    return map;
}

void fmap_empty(struct fmap* map) {
    struct fmap_node* p, * q;
    for (int j = 0; j < 2; ++j) {
        struct fmap_section* h = &map->section[j];
        for (size_t i = 0; i < h->size; ++i) {
            p = h->map[i].entry.next;
            while (p) {
                q = p;
                p = p->next;
                fmap_node_des(map, q);
                DEC(h->used);
            }
            h->map[i].entry.next = NULL;
        }
    }
}

void fmap_clear(struct fmap* map) {
    if (map == NULL) return;

    struct fmap_node* p, * q;
    for (int j = 0; j < 2; ++j) {
        struct fmap_section* h = &map->section[j];
        for (size_t i = 0; i < h->size; ++i) {
            p = h->map[i].entry.next;
            while (p) {
                q = p;
                p = p->next;
                fmap_node_des(map, q);
                DEC(h->used);
            }
        }
        ffree(h->map);
    }
}

void fmap_free(struct fmap* map) {
    if (map == NULL) return;

    struct fmap_node* p, * q;
    for (int j = 0; j < 2; ++j) {
        struct fmap_section* h = &map->section[j];
        for (size_t i = 0; i < h->size; ++i) {
            p = h->map[i].entry.next;
            while (p) {
                q = p;
                p = p->next;
                fmap_node_des(map, q);
                DEC(h->used);
            }
        }
        ffree(h->map);
    }
    ffree(map);
}

void* fmap_setkey(struct fmap* map, struct fmap_node* node, void* key) {
    if (map->type.dupkey) {
        node->key = map->type.dupkey(key);
    } else {
        node->key = key;
    }
    return node->key;
}

void* fmap_setval(struct fmap* map, struct fmap_node* node, void* value) {
    if (map->type.dupval) {
        map->type.dupval(node, value);
    } else {
        node->value = value;
    }
    return node->value;
}

static void _rehash_step(struct fmap* map, const int step) {
    return;
    struct fmap_section* org = &map->section[0];
    struct fmap_section* new = &map->section[1];
    struct fmap_node* p = NULL;
    size_t index;
    int i = 0;

    if (!fmap_isrehash(map) || step == 0) return;

    while (i < step) {
        p = org->map[map->rehash_idx].entry.next;
        while (p && i < step) {
            org->map[map->rehash_idx].entry.next = p->next;
            index = new->size_mask & fmap_hash(map, p->key);
            p->next = new->map[index].entry.next;
            new->map[index].entry.next = p;
            p = org->map[map->rehash_idx].entry.next;

            --org->used;
            ++new->used;
            ++i;
        }
        if (org->map[map->rehash_idx].entry.next) continue;

        do {
            /* rehash complete */
            if (++map->rehash_idx >= org->size) {
                map->rehash_idx = -1;

                _release_section(&map->section[0]);
                map->section[0] = map->section[1];
                _reset_section(&map->section[1]);

                return;
            }
        } while (!org->map[map->rehash_idx].entry.next);
    }
}

static int _expand(struct fmap* map, const size_t len) {
    return FMAP_OK;
    //is rehash, expand action denied
    if (fmap_isrehash(map)) return FMAP_OK;

    struct fmap_section* section = &map->section[1];
    if ((section->map = fcalloc(len, sizeof(struct fmap_slot))) == NULL) {
        return FMAP_FAILD;
    }

    for (int i = 0; i < len; ++i) {
        memset(&section->map[i].entry, 0, sizeof(struct fmap_node));
        section->map[i].lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
        section->map[i].size = 0;
        section->map[i].tail = NULL;
    }

    section->used = 0;
    section->size = len;
    section->size_mask = section->size - 1;

    map->rehash_idx = 0;

    return FMAP_OK;
}

inline int fmap_isrehash(struct fmap* map) {
    return map->rehash_flag;
}

int fmap_addraw(struct fmap* map, const void* key, void* value, int rep) {
    struct fmap_section* h;
    if (fmap_isrehash(map)) {
        h = &map->section[1];
    } else {
        h = &map->section[0];
    }

    unsigned int hash = fmap_hash(map, key);
    size_t slot = hash & h->size_mask;
    struct fmap_node* p = fmap_node_ref(h->map[slot].entry.next);

    pthread_mutex_lock(&h->map[slot].lock);
    while (p) {
        if (fmap_cmpkey(map, p->key, key) == 0) {
            if (rep == 0) {
                fmap_node_des(map, p);
                pthread_mutex_unlock(&h->map[slot].lock);
                return FMAP_EXIST;
            }
            fmap_freeval(map, p);
            fmap_setval(map, p, value);
            pthread_mutex_unlock(&h->map[slot].lock);
            return FMAP_OK;
        }
        struct fmap_node* q = p;
        p = fmap_node_ref(p->next);
        fmap_node_des(map, q);
    }

    struct fmap_node* node;
    if ((node = fmalloc(sizeof(struct fmap_node))) == NULL)
        return FMAP_FAILD;/* allocate one */
    node->next = 0;
    node->ref = 1;

    fmap_setkey(map, node, (void*) key);
    fmap_setval(map, node, value);

    if (h->map[slot].tail != NULL) {
        h->map[slot].tail->next = node;
    } else {
        h->map[slot].entry.next = node;
    }
    h->map[slot].tail = node;

    if (INC(h->used) >= h->size) {
        _expand(map, h->size << 1);
    }

    if (fmap_isrehash(map)) {
        // in here, h->size may be reset to 0
        _rehash_step(map, 1);
    }

    pthread_mutex_unlock(&h->map[slot].lock);
    return FMAP_OK;
}

int fmap_add(struct fmap* map, const void* key, void* value) {
    return fmap_addraw(map, key, value, 0);
}

int fmap_set(struct fmap* map, const void* key, void* value) {
    return fmap_addraw(map, key, value, 1);
}

struct fmap_node* fmap_get(struct fmap* map, const void* key) {
    struct fmap_section* h = &map->section[0];
    struct fmap_slot* slot;
    struct fmap_node* p;
    unsigned int hash_code;
    int flag;
    int hflag = 1;

    hash_code = fmap_hash(map, key);

    do {
        slot = &h->map[h->size_mask & hash_code];
        p = fmap_node_ref(slot->entry.next);

        pthread_mutex_lock(&slot->lock);
        while (p) {
            if (fmap_cmpkey(map, p->key, key) == 0) {
                pthread_mutex_unlock(&slot->lock);
                return p;
            }
            struct fmap_node* q = p;
            p = fmap_node_ref(p->next);
            fmap_node_des(map, q);
        }
        flag = 0;
        if (fmap_isrehash(map) && hflag) {
            h = &map->section[1];
            flag = 1;
            hflag = 0;
        }
    } while (flag);

    pthread_mutex_unlock(&slot->lock);

    return NULL;
}

void* fmap_getvalue(struct fmap* map, const void* key) {
    struct fmap_node* node = fmap_get(map, key);
    if (node == NULL) {
        return NULL;
    }

    void* val = node->value;
    fmap_node_des(map, node);
    return val;
}

struct fmap_slot* fmap_getslot(struct fmap* map, const void* key) {
    struct fmap_section* h = &map->section[0];

    unsigned int hash_code = fmap_hash(map, key);
    struct fmap_slot* slot = &h->map[h->size_mask & hash_code];

    pthread_mutex_lock(&slot->lock);
    struct fmap_node* p = fmap_node_ref(slot->entry.next);
    while (p) {
        p = fmap_node_ref(p->next);
    }
    pthread_mutex_unlock(&slot->lock);

    return slot;
}

int fmap_remove(struct fmap* map, const void* key) {
    struct fmap_node* node = fmap_pop(map, key);
    if (node == NULL) {
        return FMAP_NONE;
    }
    fmap_node_des(map, node);
    return FMAP_OK;
}

/* pop a node,
** the memory should be controlled by caller manually */
struct fmap_node* fmap_pop(struct fmap* map, const void* key) {
    struct fmap_section* h = NULL;
    if (fmap_isrehash(map)) {
        h = &map->section[1];
    } else {
        h = &map->section[0];
    }

    size_t index = h->size_mask & fmap_hash(map, key);
    struct fmap_slot* slot = &h->map[index];

    pthread_mutex_lock(&slot->lock);
    struct fmap_node* p = fmap_node_ref(slot->entry.next);
    struct fmap_node* q = p;
    while (p) {
        if (!fmap_cmpkey(map, p->key, key)) {
            break;
        }

        q = p;
        p = fmap_node_ref(p->next);
        fmap_node_des(map, q);
    }

    // not exist
    if (p == NULL) {
        pthread_mutex_unlock(&slot->lock);
        return NULL;
    }

    if (q == p) {
        h->map[index].entry.next = p->next;
    } else {
        q->next = p->next;
        fmap_node_des(map, q);
    }
    if (p->next == NULL) {
        h->map[index].tail = NULL;
    }
    fmap_node_des(map, p);

    DEC(h->used);
    pthread_mutex_unlock(&slot->lock);

    return p;
}

/**************** iterator implement ******************/
/* pos is index of node where starting iter */
hash_iter_t fmap_iter_create(struct fmap* dict) {
    size_t i, level = 0;
    hash_iter_t iter;
    struct fmap_section* h = &dict->section[0];

    for (i = 0; i < h->size; ++i) {
        if (h->map[i].entry.next != NULL) {
            break;
        }
    }

    iter.no = 0;
    iter.iter_idx = i + level;
    iter.current = h->map[i].entry.next;

    return iter;
}

hash_iter_t* fmap_iter_next(struct fmap* hash, hash_iter_t* iter) {
    struct fmap_section* h;
    int flag = 0;
    int hflag = 1;
    size_t i;
    size_t header_size = 0;

    if (!iter) return NULL;
    /* no more */
    if (iter->no >= hash->section[0].used + hash->section[1].used - 1) {
        return NULL;
    }

    h = &hash->section[0];
    if (iter->iter_idx >= h->size) {
        header_size = h->size;
        h = &hash->section[1];
        hflag = 0;
    }

    for (i = iter->iter_idx - header_size; i < h->size;) {
        if (h->map[i].entry.next == NULL) {
            ++i;
            continue;
        }

        if (flag == 1) {
            iter->current = h->map[i].entry.next;
            iter->iter_idx = i + header_size;
            ++iter->no;
            return iter;
        }

        if (iter->current->next) {
            iter->current = iter->current->next;
            iter->iter_idx = i + header_size;
            ++iter->no;
            return iter;
        } else {
            ++i;
            flag = 1;
        }

        if (i == h->size && fmap_isrehash(hash) && hflag) {
            h = &hash->section[1];
            hflag = 0;
            i = 0;
            header_size = hash->section[0].size;
        }
    }

    return NULL;
}

/****************Test use ******************/
void fmap_info_str(struct fmap* map) {
    printf("rehash_idx:%li;\n", map->rehash_idx);
    struct fmap_section* h;
    for (int j = 0; j < 2; ++j) {
        h = &map->section[j];
        printf("header:%d: size:%lu; used:%lu\n", j, h->size, h->used);

        for (int i = 0; i < h->size; ++i) {
            struct fmap_node* p = h->map[i].entry.next;
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
    printf("rehash_idx:%li;\n", map->rehash_idx);
    struct fmap_section* h;
    for (int j = 0; j < 2; ++j) {
        h = &map->section[j];
        printf("h%d: size:%lu; used:%lu\n", j, h->size, h->used);
        for (int i = 0; i < h->size; ++i) {
            struct fmap_node* p = h->map[i].entry.next;
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
