#include "fmap.h"

namespace fankux {
template<class K, class V>
class Map {
public:
    Map() {
        fmap_init_int(&_map);
    }

    size_t size() {
        return fmap_size(&_map);
    }

    int add(const K& key, V& value) {
        return fmap_add(&_map, key, &value);
    }

    int remove(const K& key) {
        return fmap_remove(&_map, key);
    }

    V* get(const K& key) {
        fmap_node* node = fmap_get(&_map, key);
        if (node == NULL) return NULL;
        return (V*) node->value;
    }

private:
    struct fmap _map;
};
}