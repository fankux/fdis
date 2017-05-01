/** 
 * Created by Fankux (fankux@gmail.com)
 */

#include "common/bytes.h"
#include "bytes.h"

namespace fdis {

int8_t Bytes::readint8(char** buf) {
    int8_t val;
    fmemcpy(&val, *buf, 1);
    (*buf) += 1;
    return val;
}

uint8_t Bytes::readuint8(char** buf) {
    uint8_t val;
    fmemcpy(&val, *buf, 1);
    (*buf) += 1;
    return val;
}

int16_t Bytes::readint16(char** buf) {
    int16_t val;
    fmemcpy(&val, *buf, 2);
    (*buf) += 2;
    return val;
}

uint16_t Bytes::readuint16(char** buf) {
    uint16_t val;
    fmemcpy(&val, *buf, 2);
    (*buf) += 2;
    return val;
}

int32_t Bytes::readint32(char** buf) {
    int32_t val;
    fmemcpy(&val, *buf, 4);
    (*buf) += 4;
    return val;
}

uint32_t Bytes::readuint32(char** buf) {
    uint32_t val;
    fmemcpy(&val, *buf, 4);
    (*buf) += 4;
    return val;
}

int64_t Bytes::readint64(char** buf) {
    int64_t val;
    fmemcpy(&val, *buf, 8);
    (*buf) += 8;
    return val;
}

uint64_t Bytes::readuint64(char** buf) {
    uint64_t val;
    fmemcpy(&val, *buf, 8);
    (*buf) += 8;
    return val;
}

struct packint Bytes::readpackint(char** buf) {
    char* pos = *buf;
    packint val;
    if (*pos < 251) {
        (*buf)++;
        val.i.i8 = (uint8_t) *pos;
    } else if (*pos == 251) {
        (*buf)++;
        val.i.i8 = 251;
    } else if (*pos == 252) {
        (*buf) += 3;
        memcpy(&val.i.i16, pos + 1, 2);
        val.i.i16 = le32toh(val.i.i16);
    } else if (*pos == 253) {
        (*buf) += 4;
        memcpy(&val.i.i24, pos + 1, 3);
        val.i.i24 = le32toh(val.i.i24);
    } else if (*pos == 254) {
        (*buf) += 8;
        memcpy(&val.i.i64, pos + 1, 8);
        val.i.i64 = le64toh(val.i.i64);
    } else { /* Must be 255 when here */
        (*buf)++;
        val.i.i8 = 255;
    }
    return val;
}

static void Bytes::readpackstr(char** buf, packstr& pstr) {
    struct packint size = readpackint(buf);
    pstr.assign(*buf, size.i.i64);
    (*buf) += size.i.i64;
}

static struct timespec Bytes::readtime(char** buf) {
    struct timespec time;
    time.tv_nsec = readuint32(buf);
    time.tv_sec = readuint32(buf);
    return time;
}

}