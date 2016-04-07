#ifndef FANKUX_LIST_H
#define FANKUX_LIST_H

#include "flist.h"

namespace fankux {
template<class E>
class List {
public:
    List() {
        flist_init(&_list);
    };

    int add_head(E& element) {
        return flist_add_head(&_list, &element);
    };

    int add_tail(E& element) {
        return flist_add_tail(&_list, &element);
    }

    E* pop_head() {
        flist_node* node = flist_pop_head(&_list);
        if (node == NULL) return NULL;
        return node->data;
    }

    E* pop_tail() {
        flist_node* node = flist_pop_tail(&_list);
        if (node == NULL) return NULL;
        return node->data;
    }

private:
    struct flist _list;
};
}


#endif // FANKUX_LIST_H
