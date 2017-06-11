//
// Created by fankux on 17-6-5.
//

#ifndef FDIS_SORTLIST_HPP
#define FDIS_SORTLIST_HPP

#include "common/common.h"
#include "common/flist.h"
#include "common/fmap.h"

namespace fdis {

template<class E>
class SortList {
private:
    class Node {
    public:
        Node() {
            prev = NULL;
            next = NULL;
        }

    public:
        Node* prev;
        Node* next;
        E val;
    };

public:
    SortList() : _len(0) {
        _head = NULL;
        _tail = NULL;
        fmap_init(&_entry, FMAP_T_INT64, FMAP_DUP_KEY);
    }

    ~SortList() {
        Node* p = _head;
        Node* q = p;
        for (size_t i = _len; i > 0; --i) {
            delete q;
            --_len;
            p = p->next;
            q = p;
        }

        fmap_clear(&_entry);
    }

    size_t size() {
        return _len;
    }

    int insert(const E& val) {
        Node* n = new(std::nothrow) Node();
        if (n == NULL) {
            return FLIST_FAILD;
        }
        n->val = val;

        uint64_t key = _key(val);
        if (size() == 0) {//list empty, head, tail pointing new point
            n->prev = NULL;
            n->next = NULL;
            _head = n;
            _tail = n;
            fmap_add(&_entry, &key, n);
            INC(_len);
            return true;
        }

        Node* p = _head;
        while (p) {
            if (val <= p->val) {
                break;
            }
            p = p->next;
        }

        if (p == NULL) {
            n->prev = _tail;
            n->next = NULL;
            _tail->next = n;
            _tail = n;
            fmap_add(&_entry, &key, n);
            INC(_len);
            return true;
        }

        // insert before p
        if (p == _head) {
            _head = n;
            n->prev = NULL;
            n->next = p;
            p->prev = n;
        } else {/* after */
            p->prev->next = n;
            n->prev = p->prev;
            n->next = p;
            p->prev = n;
        }
        fmap_add(&_entry, &key, n);
        INC(_len);
        return FLIST_OK;
    }

    void remove(const E& val) {
        Node* p = _find(val);
        if (p == NULL) {
            return;
        }

        Node* q = NULL;
        Node* l = p->prev;
        while (l != NULL && l->val == val) {
            q = l;
            l = l->prev;
            detach(q);
            ffree(q);
        }

        do {
            q = p;
            p = p->next;
            detach(q);
            ffree(q);
        } while (p != NULL && p->val == val);
    }

    void detach(const Node* node) {
        if (size() == 1) {
            _head = NULL;
            _tail = NULL;
        } else {
            if ((node)->prev == NULL) {
                _head = (node)->next;
                (node)->next->prev = NULL;
            } else if ((node)->next == NULL) {
                _tail = (node)->prev;
                (node)->prev->next = NULL;
            } else {
                (node)->next->prev = (node)->prev;
                (node)->prev->next = (node)->next;
            }
        }

        uint64_t key = _key(node->val);
        fmap_remove(&_entry, &key);
        DEC(_len);
    }

#ifdef DEBUG_FLIST

    void info() {
        printf("size:%zu, head:%zu, tail:%zu\n", size(), (size_t) _head, (size_t) _tail);
        for (E& item : *this) {
            printf("%s=>", item.tostr().buf());
        }
        printf("NULL\n");
    }

#endif

    /************** iterator **************************/
    class iterator : public E {
    public:
        iterator(Node* node, bool end_flag) : E(node->val) {
            if (end_flag) {
                _next = NULL;
                _current = NULL;
                _end_flag = true;
            } else {
                _next = node->next;
                _current = node;
                _end_flag = false;
            }
        }

        bool operator!=(const iterator& rhs) const {
            return (_end_flag != rhs._end_flag || _current != rhs._current || _next != rhs._next);
        }

        iterator& operator++() {
            if (_next == NULL) {
                _next = NULL;
                _current = NULL;
                _end_flag = true;
            } else {
                _current = _next;
                _next = _next->next;
            }
            return *this;
        }

        E& operator*() {
            return _current->val;
        }

        E* operator->() {
            return &_current->val;
        }

    private:
        struct Node* _current;
        struct Node* _next;
        bool _end_flag;
    };

    iterator begin() const {
        return iterator(_head, false);
    }

    iterator end() const {
        return iterator(_tail, true);
    }

private:
    Node* _find(const E& val) {
        uint64_t key = _key(val);
        return (Node*) fmap_getvalue(&_entry, &key);
    }

    uint64_t _key(const E& val) {
        return val.hash_code();
    }

private:
    volatile size_t _len;
    Node* _head;
    Node* _tail;

    struct fmap _entry;
};
}

#endif // FDIS_SORTLIST_HPP
