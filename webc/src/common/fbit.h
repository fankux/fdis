#ifndef FBIT_H
#define FBIT_H

#include <stddef.h>

typedef struct fbits {
    /* number of char in bits array */
    size_t size;
    size_t len;
    char bits[];
} fbits_t;

fbits_t *fbit_create(size_t len);

void fbit_free(fbits_t *bit);

void fbit_set(struct fbits *bits, size_t offset, int bit);

void fbit_set_all(fbits_t *bits, int val);

int fbit_get_val_at_round(struct fbits *bits, int val, size_t npos,
                          size_t *offset);

char *fbit_info(struct fbits *bits, int is_test);
//void fbit_move_left(fbits_t *bits, int len);


#endif //FBIT_H
