#ifndef FLIST_H
#define FLIST_H

#include <inttypes.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
namespace fankux{
#endif

/* error code */
#define FLIST_OK 0
#define FLIST_NONE -1 /* none target */
#define FLIST_FAILD -2 /* memory faild */
#define FLIST_OUTRANGE -2 /* pos overflow */

/* flist max size is SIZE_MAX - 2,
** because of -1(SIZE_MAX) meaning faild,
** -2(SIZE_MAX - 1) meaning out of range */
struct flist_node {
    struct flist_node* prev;
    struct flist_node* next;
    void* data;
};

struct flist {
    size_t len;
    struct flist_node* head;
    struct flist_node* tail;

    void* (* dup_val_func)(void* value);

    void (* free_val_func)(void* a);

    int (* cmp_val_func)(void* a, void* b);
};

/* iter direction */
#define FDLIST_START_HEAD 0
#define FDLIST_START_TAIL 1

struct flist_iter {
    int direct;
    size_t rank;
    struct flist_node* prev;
    struct flist_node* next;
};


/* macros */
#define flist_isempty(list) (list->len == 0)
#define flist_isfull(list) (list->len == (SIZE_MAX - 2))
#define flist_len(list) (list->len)
#define flist_free_val(list, n) do {                             \
    if(list->free_val_func) list->free_val_func(n->data);        \
    else n->data = NULL; }while(0)
#define flist_cmp_val(list, val1, val2)                          \
    (list->cmp_val_func != NULL? list->cmp_val_func(val1, val2): \
    ((val1) > (val2)? 1: ((val2) > (val1)? -1: 0)))
#define flist_dup_val(list, value)                               \
    (list->dup_val_func? list->dup_val_func(value): value)

/************* API *************/
void flist_init(struct flist* list);

struct flist* flist_create();

void flist_free(struct flist* list);

int flist_set(struct flist* list, const size_t index, void* value);

int flist_insert(struct flist* list, void* value, void* pivot,
        const uint8_t aorb);

int flist_get(struct flist* list, void* value, struct flist_node** p);

int flist_add_head(struct flist* list, void* value);

int flist_add_tail(struct flist* list, void* value);

struct flist_node* flist_pop_head(struct flist* list);

struct flist_node* flist_pop_tail(struct flist* list);

int flist_remove(struct flist* list, void* value, const size_t pos, int count);

int flist_remove_value(struct flist* list, void* value, void** out_val);

int flist_remove_at(struct flist* list, const size_t pos, void** out_val);

struct flist_node* flist_get_index(struct flist* list, const size_t index);

int flist_indexof(struct flist* list, void* value);

struct flist_iter* flist_iter_create(struct flist* p, const uint8_t direct,
        const size_t start_pos);

struct flist_node* flist_iter_next(struct flist_iter* iter);

void flist_iter_cancel(struct flist_iter* iter);

void flist_iter_rewind(struct flist* list, struct flist_iter* iter);

char* flist_info(struct flist* list, int simplify);

/**** function type ****/
extern int flist_cmp_int_func(void* a, void* b);

extern int flist_cmp_str_func(void* a, void* b);

extern int flist_cmp_casestr_func(void* a, void* b);

#ifdef __cplusplus
}
};
#endif

#endif //FQUEUE_H
