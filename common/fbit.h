#ifndef FBIT_H
#define FBIT_H

#include <stddef.h>

typedef struct fbits {
    size_t size; /* number of char in bits array */
    char bits[];
} fbits_t;

fbits_t *fbit_create(size_t size);

void fbit_free(fbits_t *bit);

void fbit_set(struct fbits *bits, size_t offset, int bit);

void fbit_set_all(fbits_t *bits, int val);

//void fbit_move_left(fbits_t *bits, int len);


#endif //FBIT_H
