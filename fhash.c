#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <ctype.h>

#include "fhash.h"
#include "fmem.h"
#include "common.h"

static unsigned int hash_seed = 5381;
static unsigned int hash_rand_seed = 5731;

//private function
static void _reset_header(struct hash_header *);

static void _release_header(struct hash_header *);

static struct hash_node * _find(struct fhash *, void *, unsigned int *);

static int _expand(struct fhash *, const size_t);

static void _rehash_step(struct fhash *, const unsigned int);

/****************Type Functions *****************/
static int _int_cmpkey(void * key, void * value)
{
    int a = (int) key;
    int b = *(int *) value;
    return a > b ? 1 : (a < b ? -1 : 0);
}

static int _intcmp(void * av, void * bv)
{
    int a = *(int *) av;
    int b = *(int *) bv;
    return a > b ? 1 : (a < b ? -1 : 0);
}

static int _strcmp(void * str1, void * str2)
{
    return strcmp((char *) str1, (char *) str2);
}

static int _strcasecmp(void * str1, void * str2)
{
    return strcasecmp((char *) str1, (char *) str2);
}


void _str_set(struct hash_node * node, void * str)
{
    size_t len = strlen((char *) str);
    if (!(node->value = fmalloc(sizeof(len) + 1))) return;
    cpystr(node->value, str, len);
}

void * _str_dup(void * str)
{
    size_t len = strlen((char *) str);
    char * p = NULL;
    if (!(p = fmalloc(len + 1))) return NULL;
    cpystr(p, str, len);

    return p;
}

inline void _str_free(void * str)
{
    ffree(str);
}

static inline void * _int_dup(void * str)
{
    return (void *) (*(int *) str);
}

hash_type_t hash_type_lnkboth = {_str_hash_func, _strcmp, NULL, NULL, NULL, NULL};
hash_type_t hash_type_dupboth = {_str_hash_func, _strcmp, _str_dup, _str_set, _str_free, _str_free};
hash_type_t hash_type_dupkey = {_str_hash_func, _strcmp, _str_dup, NULL, _str_free, NULL};

/*************** Hash *********************/
/* Thomas Wang's 32 bit Mix Function */
unsigned int _int_hash_func(void * x)
{
    unsigned int key = (unsigned int) x;

    key += ~(key << 15);
    key ^= (key >> 10);
    key += (key << 3);
    key ^= (key >> 6);
    key += ~(key << 11);
    key ^= (key >> 16);

    return key;
}

/* djb hash */
unsigned int _str_hash_func(void * buf)
{
    unsigned int hash = hash_seed;
    unsigned char * p = (unsigned char *) buf;

    while (*p) /* hash = hash * 33 + c */
        hash = ((hash << 5) + hash) + *p++;

    return hash;
}

/* case insensitive hash function (based on djb hash) */
unsigned int _strcase_hash_func(void * buf)
{
    unsigned int hash = hash_seed;
    unsigned char * p = (unsigned char *) buf;

    while (*p) /* hash * 33 + c */
        hash = ((hash << 5) + hash) + (tolower(*p++));
    return hash;
}

/****************** API Implementations ********************/
static void _reset_header(struct hash_header * header)
{
    header->map = NULL;
    header->size = 0;
    header->size_mask = 0;
    header->used = 0;
}

static void _release_header(struct hash_header * header)
{
    ffree(header->map);
    _reset_header(header);
}

/*create a fhash which value linked */
struct fhash * fhash_create()
{
    struct fhash * hash;
    if (!(hash = fmalloc(sizeof(struct fhash)))) return NULL;

    _reset_header(&hash->header[1]);
    _reset_header(&hash->header[0]);
    hash->rehash_idx = -1;
    hash->iter_num = 0;
    hash->type = &hash_type_lnkboth;

    return hash;
}

/* create a fhash which key duplicated, value linked */
struct fhash * fhash_create_dupkey(){
    struct fhash * hash;
    if (!(hash = fmalloc(sizeof(struct fhash)))) return NULL;

    _reset_header(&hash->header[1]);
    _reset_header(&hash->header[0]);
    hash->rehash_idx = -1;
    hash->iter_num = 0;
    hash->type = &hash_type_dupkey;

    return hash;
}


/* create a fhash which key & value both duplicated */
struct fhash * fhash_create_dupboth(){
    struct fhash * hash;
    if (!(hash = fmalloc(sizeof(struct fhash)))) return NULL;

    _reset_header(&hash->header[1]);
    _reset_header(&hash->header[0]);
    hash->rehash_idx = -1;
    hash->iter_num = 0;
    hash->type = &hash_type_dupboth;

    return hash;
}

void fhash_empty(struct fhash * hash)
{
    struct hash_node * p, * q;
    for (int j = 0; j < 2; ++j) {
        struct hash_header * h = &hash->header[j];
        for (size_t i = 0; i < h->size; ++i) {
            p = h->map[i];
            while (p) {
                q = p;
                p = p->next;
                fhash_freekey(hash, q);
                fhash_freeval(hash, q);
                --h->used;
            }
            h->map[i] = NULL;
        }
    }
}

void fhash_free(struct fhash * hash)
{
    struct hash_node * p, * q;
    for (int j = 0; j < 2; ++j) {
        struct hash_header * h = &hash->header[j];
        for (size_t i = 0; i < h->size; ++i) {
            p = h->map[i];
            while (p) {
                q = p;
                p = p->next;
                fhash_freekey(hash, q);
                fhash_freeval(hash, q);
                --h->used;
            }
        }
        ffree(h->map);
    }
    ffree(hash);
}

static void _rehash_step(struct fhash * dict, const unsigned int step)
{
    struct hash_header * org = &dict->header[0];
    struct hash_header * new = &dict->header[1];
    struct hash_node * p;
    size_t index;
    int i = 0;

    if (!fhash_isrehash(dict) || step == 0) return;

    while (i < step) {
        p = org->map[dict->rehash_idx];
        while (p && i < step) {
            org->map[dict->rehash_idx] = p->next;
            index = new->size_mask & fhash_hash(dict, p->key);
            p->next = new->map[index];
            new->map[index] = p;
            p = org->map[dict->rehash_idx];

            --org->used;
            ++new->used;
            ++i;
        }
        if (org->map[dict->rehash_idx]) continue;

        do {
            /* rehash complete */
            if (++dict->rehash_idx >= org->size) {
                dict->rehash_idx = -1;

                _release_header(&dict->header[0]);
                dict->header[0] = dict->header[1];
                _reset_header(&dict->header[1]);
                //h0 <= h1, release h1

                return;
            }
        } while (!org->map[dict->rehash_idx]);
    }
}

static int _expand(struct fhash * dict, const size_t len)
{
    //is rehash, expand action denied
    if (fhash_isrehash(dict)) return 2;

    struct hash_header * h = &dict->header[1];
    struct hash_node ** new = NULL;
    if (!(new = fmalloc(len * sizeof(struct hash_node *)))) return 0;
    fmemset(new, 0, len * sizeof(struct hash_node *));

    h->map = new;
    h->size = len;
    h->size_mask = len - 1;
    h->used = 0;

    dict->rehash_idx = 0;
    _rehash_step(dict, 1);

    return FHASH_OK;
}

/* find the key, the "hash" stores the key hash value */
static struct hash_node * _find(struct fhash * dict, void * key, unsigned int * hash)
{
    struct hash_header * h = &dict->header[0];
    struct hash_node * p;
    unsigned int result;
    int flag, hflag = 1;

    /* fisrt memeroy allocation */
    if (h->size <= 0) {
        if (!(h->map = fmalloc(FHASH_HEADER_INITIAL_SIZE * sizeof(struct hash_node *))))
            return 0;
        fmemset(h->map, 0, FHASH_HEADER_INITIAL_SIZE * sizeof(struct hash_node *));
        h->size = FHASH_HEADER_INITIAL_SIZE;
        h->size_mask = h->size - 1;
    }

    result = fhash_hash(dict, key);
    if (hash) *hash = result;

    do {
        p = h->map[(unsigned int) (h->size_mask & result)];
        while (p) {
            if (0 == fhash_cmpkey(dict, p->key, key))
                return p;
            p = p->next;
        }
        flag = 0;
        if (fhash_isrehash(dict) && hflag) {
            h = &dict->header[1];
            flag = 1;
            hflag = 0;
        }
    } while (flag);

    return NULL;
}

/* Add a key-value node at index of 'hash to index'.
** Be sure that key's hash must be equal to p->key's */
int fhash_addraw(struct fhash * dict, const unsigned int hash, void * key, void * value)
{
    struct hash_header * h;
    if (fhash_isrehash(dict))
        h = &dict->header[1];
    else h = &dict->header[0];

    /* fisrt memeroy allocation */
    if (fhash_isempty(dict)) {
        if (!(h->map = fmalloc(FHASH_HEADER_INITIAL_SIZE * sizeof(struct hash_node *))))
            return FHASH_FAILD;
        fmemset(h->map, 0, FHASH_HEADER_INITIAL_SIZE * sizeof(struct hash_node *));
        h->size = FHASH_HEADER_INITIAL_SIZE;
        h->size_mask = h->size - 1;
    }

    unsigned int index = (unsigned int) (hash & h->size_mask);
    struct hash_node * new;
    if (!(new = fmalloc(sizeof(struct hash_node))))
        return FHASH_FAILD;/* allocate one */

    fhash_setkey(dict, new, key);
    /* key never be null if key is string */
    if (dict->type->dupkey == _str_dup && !new->key) {
        ffree(new);
        return FHASH_FAILD;
    }

    fhash_setval(dict, new, value);

    new->next = h->map[index];
    h->map[index] = new;

    if (fhash_isrehash(dict)) _rehash_step(dict, 1);

    if (++h->used >= h->size);
    /* _expand(dict, h->size << 1); */

    return FHASH_OK;
}

int fhash_add(struct fhash * dict, void * key, void * value)
{
    unsigned int hash;
    struct hash_node * p = _find(dict, key, &hash);
    if (p) return FHASH_EXIST;//this key already exist, add faild

    return fhash_addraw(dict, hash, key, value);
}

inline struct hash_node * fhash_get(struct fhash * hash, void * key)
{
    return _find(hash, key, NULL);
}

struct hash_node * fhash_getrand(struct fhash * hash)
{
    srand(hash_rand_seed);
    size_t real_size = hash->header[0].used;
    if (fhash_isrehash(hash)) real_size += hash->header[1].used;

    return fhash_getat(hash, rand() % real_size);
}

void * fhash_getvalue(struct fhash * hash, void * key)
{
    struct hash_node * node = _find(hash, key, NULL);
    if(!node) return NULL;
    return node->value;
}


int fhash_replace(struct fhash * dict, void * key, void * value)
{
    struct hash_node * p = NULL;
    p = _find(dict, key, NULL);

    if (!p) return 0;

    fhash_freeval(dict, p);
    fhash_setval(dict, p, value);

    return 1;
}

/* change value,if not exist, add new one */
/* !!!!!!!bug!!!!!!! */
int fhash_set(struct fhash * dict, void * key, void * value)
{
    unsigned int hash;
    struct hash_node * p = _find(dict, key, &hash);

    //not exist,then add it to dict
    if (!p) return fhash_addraw(dict, hash, key, value);

    fhash_freeval(dict, p);
    fhash_setval(dict, p, value);

    return FHASH_OK;
}

/* pop a node,
** the memory should be controlled by caller manually */
struct hash_node * fhash_pop(struct fhash * hash, void * key)
{
    struct hash_header * h = NULL;
    if (fhash_isrehash(hash)) h = &hash->header[1];
    else h = &hash->header[0];

    unsigned int index = h->size_mask & fhash_hash(hash, key);
    struct hash_node * p = h->map[index];
    struct hash_node * q = p;
    while (p) {
        if (!fhash_cmpkey(hash, p->key, key)) break;

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

int fhash_remove(struct fhash * hash, void * key)
{
    struct hash_node * p;
    if (!(p = fhash_pop(hash, key))) return FHASH_NONE;

    fhash_freeval(hash, p);
    fhash_freekey(hash, p);

    return FHASH_OK;
}

/**************** iterator implement ******************/
/* pos is index of node where starting iter */
hash_iter_t * fhash_iter_create(struct fhash * dict, size_t pos)
{
    size_t i, level = 0;
    hash_iter_t * iter;
    struct hash_header * h = &dict->header[0];

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

hash_iter_t * fhash_iter_next(struct fhash * hash, hash_iter_t * iter)
{
    struct hash_header * h;
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

        if (i == h->size && fhash_isrehash(hash) && hflag) {
            h = &hash->header[1];
            hflag = 0;
            i = 0;
            level = hash->header[0].size;
        }
    }

    /* iteration complete */
    fhash_iter_cancel(hash, iter);
    return NULL;
}

void fhash_iter_cancel(struct fhash * hash, hash_iter_t * iter)
{
    __sync_sub_and_fetch(&hash->iter_num, 1);
    ffree(iter);
}

/****************Test use ******************/
void fhash_info_str(struct fhash * hash)
{
    printf("rehash_idx:%d; iter_num:%d\n", hash->rehash_idx, hash->iter_num);
    struct hash_header * h;
    for (int j = 0; j < 2; ++j) {
        h = &hash->header[j];
        printf("header:%d: size:%zu; used:%zu\n", j, h->size, h->used);

        for (int i = 0; i < h->size; ++i) {
            struct hash_node * p = h->map[i];
            printf("%4d(p:%9d):", i, (int) p);
            while (p) {
                printf("%s:%s->", (char *) p->key, (char *) p->value);
                p = p->next;
            }
            printf("\n");
        }
    }
}

void fhash_info_int(struct fhash * hash)
{
    printf("rehash_idx:%zd; iter_num:%zu\n", hash->rehash_idx, hash->iter_num);
    struct hash_header * h;
    for (int j = 0; j < 2; ++j) {
        h = &hash->header[j];
        printf("h%d: size:%zu; used:%zu\n", j, h->size, h->used);
        for (int i = 0; i < h->size; ++i) {
            struct hash_node * p = h->map[i];
            printf("%4d(p:%9d):", i, (int) p);
            while (p) {
                printf("%d:%d->", (int) p->key, (int) &p->value);
                p = p->next;
            }
            printf("\n");
        }
    }
}

#ifdef DEBUG_FHASH

int main()
{
    struct fhash * hash = fhash_create();

    fhash_add(hash, "k1", "v1");
    fhash_info_str(hash);


    return 0;
}

#endif