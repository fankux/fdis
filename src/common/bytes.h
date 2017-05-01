/** 
 * Created by Fankux (fankux@gmail.com)
 */

#ifndef FDIS_BYTES_H
#define FDIS_BYTES_H

#include <stdint.h>
#include <time.h>
#include "common/fmem.h"
#include "common/fstr.hpp"

namespace fdis {

struct packint {
    union {
        uint8_t i8;
        uint16_t i16;
        uint32_t i24;
        uint64_t i64;
    } i;
} packint;

typedef struct str packstr;

#define PACKINT_MIN_SIZE 1
#define PACKINT_MAX_SIZE sizeof(struct packint)

#define PACKSTR_MIN_SIZE (PACKINT_MIN_SIZE + 1)   // PACKINT_MIN_SIZE and end of string

class Bytes {
public:
    static int8_t readint8(char** buf);

    static uint8_t readuint8(char** buf);

    static int16_t readint16(char** buf);

    static uint16_t readuint16(char** buf);

    static int32_t readint32(char** buf);

    static uint32_t readuint32(char** buf);

    static int64_t readint64(char** buf);

    static uint64_t readuint64(char** buf);

    static struct packint readpackint(char** buf);

    static void readpackstr(char** buf, packstr& pstr);

    static struct timespec readtime(char** buf);
};
}

#endif // FDIS_BYTES_H
