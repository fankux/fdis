//
// Created by fankux on 16-11-30.
//

#ifndef FDIS_FILE_ACCESS_H
#define FDIS_FILE_ACCESS_H

#include <stdlib.h>
#include <sys/stat.h>
#include <src/common/common.h>
#include "common/fstr.hpp"
#include "common/flist.hpp"
#include "common/fmap.hpp"
#include "common/flog.h"
#include "common/check.h"

namespace fdis {
class LocalFile {
public:
    LocalFile(const str& str = "", const str& mode = "r") : _fp(NULL) {
        if (!str.isempty()) {
            open(str, mode);
        }
    }

    ~LocalFile() {
        if (_fp != NULL) {
            close();
        }
    }

    bool good() const {
        return _fp != NULL;
    }

    int open(const str& str, const str& mode = "r") {
        if (_fp != NULL) {
            close();
        }

        _name = str;
        _fp = fopen(str.buf(), mode.buf());
        if (_fp == NULL) {
            return -1;
        }
        return 0;
    }

    bool read(char* buf, const size_t size) {
        return fread(buf, size, 1, _fp);
    }

    bool readint8(int8_t& val) {
        return read((char*) &val, 1) > 0;
    }

    bool readuint8(uint8_t& val) {
        return read((char*) &val, 1) > 0;
    }

    bool readint16(int16_t& val) {
        return read((char*) &val, 2) > 0;
    }

    bool readuint16(uint16_t& val) {
        return read((char*) &val, 2) > 0;
    }

    bool readint(int32_t& val) {
        return read((char*) &val, 4) > 0;
    }

    bool readuint(uint32_t& val) {
        return read((char*) &val, 4) > 0;
    }

    bool readint64(int64_t& val) {
        return read((char*) &val, 8) > 0;
    }

    bool readuint64(uint64_t& val) {
        return read((char*) &val, 8) > 0;
    }

    bool readstr(str& val, const size_t size) {
        val.reserve(size);
        return read(val.data(), size) > 0;
    }

    /**
     * @brief range from 0 to UINT64_MAX - 1, UINT64_MAX mean null length
     * @param packint
     * @return
     */
    bool read_packint(packint_t& packint) {
        /**
         * from mysql source, but max size is 9-bytes
         * An integer that consumes 1, 3, 4, or 9 bytes, depending on its numeric value
         * To convert a number value into a length-encoded integer:
         * If the value is < 251, it is stored as a 1-byte integer.
         * If the value is ≥ 252 and < (253), it is stored as fc + 2-byte integer.
         * If the value is ≥ (253) and < (254), it is stored as fd + 3-byte integer.
         * If the value is ≥ (254) and < (255) it is stored as fe + 8-byte integer.
         */
        if (!this->read((char*) &packint.i.i8, 1)) {
            return false;
        }
        if (packint.i.i8 < 251) {
            return true;
        }
        if (packint.i.i8 == 251) {
            packint.i.i64 = (uint64_t) (~0); //NULL_LENGTH;
            return true;
        }
        if (packint.i.i8 == 252) {
            if (!read((char*) &packint.i.i16, 2)) {
                return false;
            }
            packint.i.i16 = le32toh(packint.i.i16);
            packint.i.i16 += 252;
            return true;
        }
        if (packint.i.i24 == 253) {
            if (!read((char*) &packint.i.i24, 3)) {
                return false;
            }
            packint.i.i24 = le32toh(packint.i.i24);
            packint.i.i24 += 253;
            return true;
        }

        /* Must be 254 when here */
        if (!read((char*) &packint.i.i64, 8)) {
            return false;
        }
        packint.i.i64 = le32toh(packint.i.i64);
        packint.i.i64 += 254;
        return true;
    }

    int write(const char* buf, const size_t size, int direct = 0) {
        fwrite(buf, size, 1, _fp);
        if (ferror(_fp)) {
            return -1;
        }
        return 0;
    }

    void close() {
        if (_fp != NULL) {
            fclose(_fp);
        }
        _fp = NULL;
    }

private:
    FILE* _fp;
    str _name;
};


class Chunk {
private:
    uint64_t _pos;
    uint64_t _size;

    LocalFile _fd;
};

#define LFS_R_INT_RETB(__type__, __val__) {      \
    if (!_persist.read##_type_(__val__)) {      \
        return false;                           \
    }                                           \
}

#define LFS_R_STR_RETB(__val__, _size_) {                \
    if (!_persist.readstr(__val__, (__size__))) {       \
        return false;                                   \
    }                                                   \
}

typedef enum {
    FILE_T_DEL = 0,
    FILE_T_ITEM,
    FILE_T_DIR,
    FILE_T_LINK,
};

#define META_PERSIST_HEADER_SIZE 8

class FileArranger {
public:

    /**

     meta_persist_header:

     header_size                4
     ====================================================
     meta_persist_payload_size    4
     ====================================================


     payloads:

     file_meta_number           packed_num
     ====================================================
     file 1 payload
     ====================================================
     file 2 payload
     ====================================================
     ...
     ====================================================
     file N payload:
     ====================================================


     file payload:

     file_type                  1
     -----------------------------------------
     mode                       4
     -----------------------------------------
     uid                        4
     -----------------------------------------
     gid                        4
     -----------------------------------------
     create_time                4
     -----------------------------------------
     access_time                4
     -----------------------------------------
     change_time                4
     -----------------------------------------
     modify_time                4
     -----------------------------------------
     file_name                  packed_str
     -----------------------------------------
     file_size                  packed_num
     -----------------------------------------
     chunk_number               packed_num
     -----------------------------------------
     chunk_file_path            packed_str
     -----------------------------------------
     chunk0_pos      4 | chunk0_size    4
     -----------------------------------------
     chunk1_pos      4 | chunk1_size    4
     -----------------------------------------
     ...               | ...
     -----------------------------------------
     chunkN_pos      4 | chunkN_size    4
     -----------------------------------------

     */
    bool init(const str& str) {
        if (_persist.open(str) != 0) {
            return false;
        }

        struct stat st;
        if (stat(str.buf(), &st) != 0) {
            fatal("meta file %s stat failed, errno : %d, error : %s" ERRHOLD, str.buf(), ERRSIGN);
            return false;
        }
        check_cond_fatal(st.st_size > META_PERSIST_HEADER_SIZE, return false,
                "meta file too short, size: %d", st.st_size);

        char persist_head_buf[META_PERSIST_HEADER_SIZE];
        int ret = _persist.read(persist_head_buf, sizeof(persist_head_buf));
        check_cond_fatal(ret == 0, return false, "read header failed" ERRPAD);

        uint32_t header_size;
        uint32_t payload_size;


        return true;
    }

private:
    LocalFile _persist;
};

struct File {
    uint8_t _status;
    struct stat _st;
    struct timespec _create_time;
    str _name;

    List<Chunk> _chunks;
};

class LocalFilePack {
public:
    LocalFilePack() : _metas(FMAP_T_STR, FMAP_DUP_VAL) {

    }

private:
    Map<str, Chunk> _metas;
};

typedef enum {
    FS_LOCAL = 0,
    FS_DFS
} FileStoreType;

class FileStore {
public:
    FileStore(const FileStoreType type) {
        _type = type;
    }

    LocalFile* create_file() {
    }

private:


    FileStoreType _type;
};
}

#endif // FDIS_FILE_ACCESS_H
