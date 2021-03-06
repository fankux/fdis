#include <strings.h>
#include "flist.h"
#include "fmem.h"
#include "flog.h"

#ifdef __cplusplus
namespace fdis{
#endif

struct flist* flist_create() {
    struct flist* list;
    if (!(list = (struct flist*) fmalloc(sizeof(struct flist))))
        return NULL;

    fmemset(list, 0, sizeof(struct flist));
    return list;
}

void flist_init(struct flist* list) {
    if (list == NULL) return;
    fmemset(list, 0, sizeof(struct flist));
    return;
}

void flist_free(struct flist* list) {
    if (list == NULL) return;

    struct flist_node* p = list->head;
    struct flist_node* q = p;
    for (size_t i = list->len; i > 0; --i) {
        flist_free_val(list, q);
        p = p->next;
        q = p;
    }
    ffree(list);
}

void flist_clear(struct flist* list) {
    if (list == NULL) return;

    struct flist_node* p = list->head;
    struct flist_node* q = p;
    for (size_t i = list->len; i > 0; --i) {
        flist_free_val(list, q);
        p = p->next;
        q = p;
    }
    fmemset(list, 0, sizeof(struct flist));
}

/* insert a value, aorb means 'after or before',
** aorb 1, insert value before pivot, its default
** aorb 2, insert value after pivot ,
** return value:
** FLIST_FAILD, usually caused by mem error,
** FLIST_NONE, pivot not exist,
** FLIST_OK, action done */
int flist_insert(struct flist* list, void* value, void* pivot,
                 const uint8_t aorb) {
    int flag = 0;
    struct flist_node* p = list->head;
    struct flist_node* new;

    while (p) {
        if (0 == flist_cmp_val(list, p->data, pivot)) {
            flag = 1;
            break;
        }
        p = p->next;
    }
    if (!flag) return FLIST_NONE;

    if (!(new = fmalloc(sizeof(struct flist_node)))) return FLIST_FAILD;
    new->data = value;

    if (list->head == p && 2 != aorb) {/* p first, before */
        list->head = new;
        new->prev = NULL;
        new->next = p;
        p->prev = new;
    } else if (list->tail == p && aorb == 2) {/* p tail, after */
        list->tail = new;
        new->next = NULL;
        new->prev = p;
        p->next = new;
    } else if (aorb == 2) {/* after */
        p->next->prev = new;
        new->next = p->next;
        new->prev = p;
        p->next = new;
    } else {/* before */
        p->prev->next = new;
        new->prev = p->prev;
        new->next = p;
        p->prev = new;
    }

    return FLIST_OK;
}

/* get a node by 'value', if not exist, return FLIST_NONE
** else set p to it */
int flist_get(struct flist* list, void* value, struct flist_node** p) {
    struct flist_node* head = list->head;

    while (head) {
        if (flist_cmp_val(list, head->data, value) == 0) {
            if (p) *p = head;
            return FLIST_OK;
        }
        head = head->next;
    }

    return FLIST_NONE;
}

/* add node at header */
int flist_add_head(struct flist* p, void* data) {
    if (flist_isfull(p)) return FLIST_FAILD;

    struct flist_node* new;
    if (NULL ==
        (new = (struct flist_node*) fmalloc(sizeof(struct flist_node))))
        return FLIST_FAILD;

    new->data = flist_dup_val(p, data);

    if (flist_isempty(p)) {//list empty, head, tail pointing new point
        new->prev = new->next = NULL;
        p->head = p->tail = new;
    } else {
        new->prev = NULL;
        new->next = p->head;
        p->head = p->head->prev = new;
    }
    ++p->len;

    return FLIST_OK;
}

/* add node at tail */
int flist_add_tail(struct flist* p, void* data) {
    if (flist_isfull(p)) return FLIST_FAILD;

    struct flist_node* new;
    if (!(new = (struct flist_node*) fmalloc(sizeof(struct flist_node))))
        return FLIST_FAILD;

    new->data = flist_dup_val(p, data);

    if (flist_isempty(p)) {
        new->prev = new->next = NULL;
        p->head = p->tail = new;
    } else {
        new->next = NULL;
        new->prev = p->tail;
        p->tail = p->tail->next = new;
    }
    ++p->len;

    return FLIST_OK;
}

/* delete node at header */
struct flist_node* flist_pop_head(struct flist* list) {
    struct flist_node* p = NULL;
    if (flist_isempty(list)) return NULL;

    if (list->len == 1) { /* last one node */
        p = list->head;
        list->head = list->tail = NULL;
    } else {
        p = list->head;
        list->head = list->head->next;
        list->head->prev = NULL;

    }
    --list->len;
    return p;
}

/* delete node at tail */
struct flist_node* flist_pop_tail(struct flist* list) {
    struct flist_node* p = NULL;
    if (flist_isempty(list)) return NULL;

    if (list->len == 1) {
        p = list->tail;
        list->head = list->tail = NULL;
    } else {
        p = list->tail;
        list->tail = list->tail->prev;
        list->tail->next = NULL;
    }
    --list->len;
    return p;
}

int flist_set(struct flist* list, const size_t index, void* value) {
    if (index >= flist_len(list))
        return FLIST_OUTRANGE;

    struct flist_node* p = list->head;
    int i = 0;

    while (i < index) {
        p = p->next;
        ++i;
    }

    flist_free_val(list, p);
    p->data = flist_dup_val(list, value);

    return FLIST_OK;
}

/* get the 'left to right' rank of first node which value equal 'value',
** if no result , result -1 */
int flist_indexof(struct flist* list, void* value) {
    struct flist_node* p = list->head;

    int i = 0;
    while (p) {
        if (p->data && flist_cmp_val(list, p->data, value) == 0)
            return i;
        else
            ++i;
    }

    return -1;
}

/* unlink the p from list */
#define riditof(list, p)                            \
    if(list->len == 1){                             \
        list->head = list->tail = NULL;             \
    }else{                                          \
        if((p)->prev == NULL){                      \
            list->head = (p)->next;                 \
            (p)->next->prev = NULL;                 \
        }else if((p)->next == NULL){                \
            list->tail = (p)->prev;                 \
            (p)->prev->next = NULL;                 \
        }else{                                      \
            (p)->next->prev = (p)->prev;            \
            (p)->prev->next = (p)->next;            \
        }                                           \
    }

/* remove number of absolute value of 'count' node from 'pos',
** it's value equal to 'value'
** count > 0, search from left to right,
** count < 0, search from right to left,
** count = 0, remove all, 'pos' direction depend on value sign,
** return value is the actually number of deleted nodes */
int flist_remove(struct flist* list, void* value, const size_t pos, int count) {
    struct flist_node* p, * q;
    size_t i = 0;
    int n = 0;

    if (pos >= flist_len(list))
        return FLIST_NONE;

    if (count < 0) {/* start scan from tail */
        count = 0 - count;
        p = list->tail;
        while (i++ < pos) {/* go forward to 'pos' */
            p = p->prev;
        }
        while (p && n < count) {
            /* printf("n:%d, count:%d \n", n, count); */
            q = p;
            p = p->prev;
            if (0 == flist_cmp_val(list, q->data, value)) {
                riditof(list, q);
                flist_free_val(list, q);
                ffree(q);
                ++n;
                --list->len;
            }
        }
    } else {/* count >= 0 */
        p = list->head;
        while (i++ < pos) {/* go forward to 'pos' */
            p = p->next;
        }
        while (p && (n < count || 0 == count)) {
            q = p;
            p = p->next;
            if (0 == flist_cmp_val(list, q->data, value)) {
                riditof(list, q);
                flist_free_val(list, q);
                ffree(q);
                ++n;
                --list->len;
            }
        }
    }

    return n;
}

/* remove a node at position of 'pos', pos is 0 base,
** if 'out_val' is NULL, the value at 'pos' would be freed,
** else value at 'pos' would be linked to 'out_val' */
int flist_remove_at(struct flist* list, const size_t pos, void** out_val) {
    struct flist_node* p, * q;
    size_t i;

    if (pos >= flist_len(list))/* pos type of unsigned, never minus */
        return FLIST_OUTRANGE;

    p = list->head;
    i = -1;
    while (++i < pos) {
        p = p->next;
    }
    q = p;

    /* ffree or store out the p->value */
    if (out_val == NULL) {
        flist_free_val(list, p);
    } else {
        *out_val = p->data;
    }

    riditof(list, p);

    ffree(q);
    --list->len;

    return FLIST_OK;
}

/* remove first node which value equals to 'value',
** 'null' can't equal anything
** 'value' just send to CmpFunc, different implement of CmpFunc
** may define different value type */
int flist_remove_value(struct flist* list, void* value, void** out_val) {
    struct flist_node* p, * q = NULL;

    p = list->head;
    while (p) {
        q = p;
        if (p->data && 0 == flist_cmp_val(list, p->data, value))
            break;
        p = p->next;
    }
    /* no result */
    if (p == NULL) {
        if (out_val)
            *out_val = NULL;
        return FLIST_NONE;
    }
    /* ffree or store out the p->value */
    if (out_val == NULL) {
        flist_free_val(list, p);
    } else {
        *out_val = p->data;
    }

    riditof(list, p);

    ffree(q);
    --list->len;

    return FLIST_OK;
}

char* flist_info(struct flist* p, int simplify) {
    char* buf, * s;
    s = buf = fmalloc(1000);

    if (p == NULL) {
        sprintf(buf, "null");
        return buf;
    }
    struct flist_node* iter = p->head;
    if (!simplify) buf += sprintf(buf, "next:");
    while (iter) {
        buf += sprintf(buf, "%d->", *(int*) iter->data);
        iter = iter->next;
    }
    buf += sprintf(buf, "null");
    if (!simplify) {
        buf += sprintf(buf, "prev:");
        iter = p->tail;
        while (iter) {
            buf += sprintf(buf, "%d->", *((int*) (iter->data)));
            iter = iter->prev;
        }
        buf += sprintf(buf, "null;length:%lu\n", p->len);
    }
    return s;
}

/********** iterator implements, pos is 0 base **********/
struct flist_iter flist_iter_start(struct flist* list, const uint8_t direct, const size_t pos) {

    struct flist_iter iter = FLIST_ITER_INITIALIZER;
    if (flist_isempty(list) || pos >= flist_len(list)) {
        return iter;
    }

    iter.direct = direct;
    if (direct == FLIST_START_HEAD) {
        iter.next = list->head;
    } else if (direct == FLIST_START_TAIL) {
        iter.next = list->tail;
    }

    for (size_t i = 0; i < pos; ++i) {
        if (flist_iter_next(&iter) == NULL) {
            break;
        }
    }

    return iter;
}

struct flist_node* flist_iter_next(struct flist_iter* iter) {
    if (!iter->next) {
        return NULL;
    }

    struct flist_node* p = iter->next;
    iter->current = p;
    if (iter->direct == FLIST_START_HEAD)
        iter->next = p->next;
    else if (iter->direct == FLIST_START_TAIL)
        iter->next = p->prev;

    return p;
}

struct flist_node* flist_get_index(struct flist* list, const size_t index) {
    if (index >= flist_len(list)) return NULL;

    struct flist_iter iter = flist_iter_start(list, FLIST_START_HEAD, index);
    struct flist_node* p = flist_iter_next(&iter);
    return p;
}

/********** type function **********/
inline int flist_cmp_int_func(void* a, void* b) {
    size_t m = *(size_t*) a;
    size_t n = *(size_t*) b;
    return m > n ? 1 : (m < n ? -1 : 0);
}

inline int flist_cmp_str_func(void* a, void* b) {
    return strcmp((char*) a, (char*) b);
}

inline int flist_cmp_casestr_func(void* a, void* b) {
    return strcasecmp((char*) a, (char*) b);
}

#ifdef __cplusplus
}
#endif
