//
// Created by fankux on 16-12-11.
//

#define DEBUG_FMAP
#ifdef DEBUG_FMAP

#include "common/common.h"
#include "common/fmap.h"

using namespace fdis;

int main() {
    struct fmap* map = fmap_create(FMAP_T_STR, FMAP_DUP_BOTH);
    int n = 32;

    struct timeval start;
    struct timeval stop;
    struct timeval diff;

    gettimeofday(&start, NULL);
    const char* k = "1";
    const char* k1 = "1";
    fmap_add(map, k, (void*) "111");
    struct fmap_node* node = fmap_get(map, k);
    fmap_node_des(map, node);
    int ret = fmap_add(map, k, (void*) "222");
    printf("add ret : %d\n", ret);
    char* val = (char*) fmap_getvalue(map, k1);
    printf("get value : %s\n", val);
    node = fmap_pop(map, k);
    fmap_node_des(map, node);

    fmap_add(map, k1, (void*) "333");
    fmap_remove(map, k);
//
//    fmap_set(map, &k, (void*) "222");
//    val = (char*) fmap_getvalue(map, &k1);
//    printf("value : %s\n", val);

//    for (int i = 0; i < n; ++i) {
//        char key[100] = "\0";
//        char val[100] = "\0";
//        sprintf(key, "k%d", i);
//        sprintf(val, "v%d", i);
//        fmap_add(map, key, val);
//
//        fmap_info_str(map);
//    }

//    fmap_free(map);
//    fmap_add(map, "k7", (void*) "v7");
//    gettimeofday(&stop, NULL);

//    timeval_subtract(&diff, &start, &stop);
//    fmap_info_int(map);

//    printf("time:%ldms\n", diff.tv_sec * 1000 + diff.tv_usec / 1000);

//    fmap_remove(map, "k4");
//    fmap_info_str(map);
//
//    fmap_add(map, "k5", (void*) "v5");
//    fmap_info_str(map);

    return 0;
}

#endif /* DEBUG_FMAP */