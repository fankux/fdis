#ifndef FDIS_LIST_H
#define FDIS_LIST_H

#include "flist.h"

namespace fdis {
class List;

template<class E>
class ListIter {
public:
    ListIter(const List& list, const size_t pos) {
        _iter = flist_iter_start(&list._list, FLIST_START_HEAD, pos);
    };

    bool operator!=(const ListIter& rhs) const {
        return (_iter.direct != rhs._iter.direct || _iter.current != rhs._iter.current ||
                _iter.next != rhs._iter.next);
    }

    const ListIter& operator++() {
        flist_iter_next(&_iter);
        return *this;
    }

    E operator*() const {
        return *((E*) _iter.current->data);
    }

private:
    struct flist_iter _iter;
};

template<class E>
class List {
public:
    friend class ListIter<E>;

    List() {
        flist_init(&_list);
    };

    const size_t size() const {
        return flist_len(&_list);
    }

    int add_head(const E& element) {
        E* val = (E*) malloc(sizeof(E));
        memcpy(val, &element, sizeof(E));
        return flist_add_head(&_list, val);
    };

    int add_tail(const E& element) {
        E* val = (E*) malloc(sizeof(E));
        memcpy(val, &element, sizeof(E));
        return flist_add_tail(&_list, &val);
    }

    bool pop_head(E& element) {
        flist_node* node = flist_pop_head(&_list);
        if (node == NULL) return false;
        element = *node;
        ffree(node);
        return true;
    }

    bool pop_tail(E& element) {
        flist_node* node = flist_pop_tail(&_list);
        if (node == NULL) return false;
        element = *node;
        ffree(node);
        return true;
    }

    E get(size_t index) const {
        flist_node* node = flist_get_index(&_list, index);
        return *((E*) node->data);
    }

    ListIter<E> begin() const {
        return ListIter(*this, 0);
    }

    ListIter<E> end() const {
        return ListIter(*this, size());
    }

    void set(size_t index, E& value) {
        E* val = new(std::nothrow) E;
        flist_set(&_list, index, val);
    }

private:
    struct flist _list;
};
}


#endif // FDIS_LIST_H
