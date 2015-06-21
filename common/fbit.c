#include "common.h"
#include "fbit.h"
#include "fmem.h"

/* LITTLE END support only */

static int char_bit_len = sizeof(char *) * 8;

fbits_t *fbit_create(size_t size) {
    if (size <= 0) return NULL;

    size_t real_size = (size - 1) / 8 + 1;
    struct fbits *bits = fmalloc(sizeof(struct fbits) + real_size);
    bits->size = real_size;

    return bits;
}


void fbit_free(fbits_t *bits) {
    ffree(bits);
}

void fbit_set_all(struct fbits *bits, int val) {
    fmemset(bits->bits, val == 0 ? 0 : 0xff, bits->size);
}

void fbit_set(struct fbits *bits, size_t offset, int bit) {
    int idx = offset / 8;
    char section = bits->bits[idx];
    offset %= 8;

    if (bit)
        section |= (1 << offset);
    else
        section &= ~(1 << offset);

    printf("section : %d, offset:%d\n", section, offset);

    bits->bits[idx] = section;
}

//void fbit_move_left(struct fbits *bits, int len) {
//    char section;
//    int tobe_move = len;
//    for (int i = bits->size - 1; i >= 0; --i) {
//        if (tobe_move < char_bit_len) {
//            section = (char) ~(~0 << tobe_move);
//            bits->bits[i] = section;
//            break;
//        } else {
//            section = (char) ~(~0 << char_bit_len);
//            bits->bits[i] = section;
//            tobe_move -= char_bit_len;
//        }
//    }
//}

char *fbit_info(struct fbits *bits, int is_test) {
    char *str;
    if (bits == NULL) {
        str = "null\n";
        return str;
    }

    size_t compute_size = bits->size * 8;
    char *base = str = malloc(sizeof(char) * 1000);

    if (!is_test)
        str += sprintf(str, "size: %zd, compute bit size: %zd\n", bits->size, compute_size);

    for (int i = 0; i < bits->size; ++i) {
        for (int j = 0; j < 8; ++j) {
            str += sprintf(str, "%s", (bits->bits[i] & (1 << j)) == 0 ? "0" : "1");
        }
        if (i != bits->size - 1)
            str += sprintf(str, " ");
    }
    if (!is_test)
        sprintf(str, "\n");

    return base;
}

#define DEBUG_FBIT
#ifdef DEBUG_FBIT

int main(void) {
    size_t bit_len = 16;
    char *run_info;

    fbits_t *bits = fbit_create(bit_len);
    fbit_set_all(bits, 1);
    run_info = fbit_info(bits, 1);
    test_equal_str("11111111 11111111", run_info, 0);
    free(run_info);

    fbit_set_all(bits, 0);
    run_info = fbit_info(bits, 1);
    test_equal_str("00000000 00000000", run_info, 0);
    free(run_info);

    char buffer[100] = "\0";
    char *idx = buffer;
    for (size_t i = 0; i < bit_len; ++i) {
        fbit_set(bits, i, 1);
        run_info = fbit_info(bits, 1);

        for (size_t j = 0; j < 8 * bits->size; ++j) {
            if (j >=8 && j % 8 == 0) idx += sprintf(idx, " ");
            if (i == j)
                idx += sprintf(idx, "1");
            else
                idx += sprintf(idx, "0");
        }
        test_equal_str(buffer, run_info, 0);
        free(run_info);

        idx = buffer;
        fbit_set(bits, i, 0);
    }

    return 0;
}

#endif /* DEBUG_FBIT */