#ifndef FDIS_MAP_H
#define FDIS_MAP_H

#include "fmap.h"

namespace fdis {
template<class K, class V>
class Map {
public:

    Map(fmap_key_type key_type, fmap_dup_type dup_mask) {
        fmap_init(&_map, key_type, dup_mask);
    }

    size_t size() {
        return fmap_size(&_map);
    }

    int add(const K& key, V& value) {
        return fmap_add(&_map, &key, &value);
    }

    int set(const K& key, V& value) {
        return fmap_set(&_map, &key, &value);
    }

    int remove(const K& key) {
        return fmap_remove(&_map, &key);
    }

    bool find(const K& key) {
        return fmap_get(&_map, &key) != NULL;
    }

    V* get(const K& key) {
        fmap_node* node = fmap_get(&_map, &key);
        if (node == NULL) return NULL;
        return (V*) node->value;
    }

private:
    struct fmap _map;
};
}

#endif // FDIS_MAP_H
