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

#ifdef DEBUG_FMAP

using namespace fdis;

#include "common.h"

int main() {
    struct fmap* map = fmap_create(FMAP_T_INT, FMAP_DUP_NONE);
    int n = 65536 * 2 * 2 * 2 - 1;

    struct timeval start;
    struct timeval stop;
    struct timeval diff;

    gettimeofday(&start, NULL);
    const int k = 1;
    const int k1 = 1;
    fmap_add(map, &k, (void*) "111");
    char* val = (char*) fmap_getvalue(map, &k1);
    printf("values : %s\n", val);

//    for (int i = 0; i < n; ++i) {
//        char key[100] = "\0";
//        char val[100] = "\0";
//        sprintf(key, "k");
//        sprintf(val, "v");
//        fmap_add(map, key, val);
//
//        fmap_info_str(map);
//    }
//    fmap_add(map, "k7", "v7");
    gettimeofday(&stop, NULL);

    timeval_subtract(&diff, &start, &stop);
    fmap_info_int(map);

    printf("time:%ldms\n", diff.tv_sec * 1000 + diff.tv_usec / 1000);

//    fmap_remove(map, "k4");
//    fmap_info_str(map);
//
//    fmap_add(map, "k5", "v5");
//    fmap_info_str(map);

    return 0;
}

#endif /* DEBUG_FMAP */