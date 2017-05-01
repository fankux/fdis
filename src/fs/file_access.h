//
// Created by fankux on 16-11-30.
//

#ifndef FDIS_FILE_ACCESS_H
#define FDIS_FILE_ACCESS_H

#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <src/common/bytes.h>
#include "common/common.h"
#include "common/bytes.h"
#include "common/fstr.hpp"
#include "common/flist.hpp"
#include "common/fmap.hpp"
#include "common/flog.h"
#include "common/check.h"
#include "common/bytes.h"

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
        return fread(buf, size, 1, _fp) >= 0;
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
     * @brief range from 0 to UINT64_MAX
     * @param packint
     * @return
     */
    bool read_packint(packint& packint) {
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
            return true;
        }
        if (packint.i.i8 == 252) {
            if (!read((char*) &packint.i.i16, 2)) {
                return false;
            }
            packint.i.i16 = le32toh(packint.i.i16);
            return true;
        }
        if (packint.i.i8 == 253) {
            if (!read((char*) &packint.i.i24, 3)) {
                return false;
            }
            packint.i.i24 = le32toh(packint.i.i24);
            return true;
        }
        if (packint.i.i8 == 254) {
            if (!read((char*) &packint.i.i64, 8)) {
                return false;
            }
            packint.i.i64 = le32toh(packint.i.i64);
            return true;
        }
        /* Must be 255 when here */
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
public:
    /**
        chunk0_file_path            packed_str
        -----------------------------------------
        chunk0_pos      4 | chunk0_size    4
    */
    bool parse_from_stream(char** buf) {
        Bytes::readpackstr(buf, _name);
        _pos = Bytes::readuint32(buf);
        _size = Bytes::readuint32(buf);
        return true;
    }

private:
    str _name;
    uint32_t _pos;
    uint32_t _size;

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

/**
 * @brief minium length of a file meta buffer
 *
 * file_type(1) + mode(4) + uid(4) + gid(4) + create_time(8) + access_time(8) + change_time(8) +
 * modify_time(8) + min(file_name) + min(file_size) + min(chunk_number)
 */
#define FILE_META_PAYLOAD_MIN_SIZE (1 + 4 + 4 + 4 + 8 + 8 + 8 + 8 + PACKSTR_MIN_SIZE + \
        PACKINT_MIN_SIZE + PACKINT_MIN_SIZE)

/**
 * @brief minium length of a meta persist payload buffer
 *
 * min(file_meta_number) + FILE_META_PAYLOAD_MIN_SIZE
 */
#define META_PERSIST_PAYLOAD_MIN_SIZE (PACKINT_MIN_SIZE + FILE_META_PAYLOAD_MIN_SIZE)

class FileMeta;

class FileArranger {
public:

    /**

     meta_persist_header:

     header_size                  4
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
     */
    bool init(const str& str) {
        if (_persist.open(str) != 0) {
            return false;
        }

        struct stat st;
        if (stat(str.buf(), &st) != 0) {
            fatal("meta file %s stat failed, errno : %d, error : %s"
                    ERRHOLD, str.buf(), ERRSIGN);
            return false;
        }
        check_cond_fatal(st.st_size > META_PERSIST_HEADER_SIZE, return false,
                "meta file too short, size: %d", st.st_size);

        char persist_head_buf[META_PERSIST_HEADER_SIZE];
        int ret = _persist.read(persist_head_buf, sizeof(persist_head_buf));
        check_cond_fatal(ret == 0, return false, "read header failed"
                ERRPAD);


        char** buf = (char**) &persist_head_buf;
        uint32_t header_size = Bytes::readuint32(buf);
        uint32_t meta_persist_payload_size = Bytes::readuint32(buf);

        check_cond_fatal(meta_persist_payload_size >= META_PERSIST_PAYLOAD_MIN_SIZE, return false,
                "meta_persist_payload_size : %u to short, META_PERSIST_PAYLOAD_MIN_SIZE is %u",
                meta_persist_payload_size, META_PERSIST_PAYLOAD_MIN_SIZE);
        info("read meta_persist_payload_size : %u", meta_persist_payload_size);

        char* payload_buf = (char*) fmalloc(meta_persist_payload_size);
        check_null_oom(payload_buf, return false, "payload buf");
        ret = _persist.read(payload_buf, meta_persist_payload_size);
        check_cond_fatal(ret == 0, return false, "read payload failed"
                ERRPAD);
        buf = &payload_buf;

        struct packint file_meta_num = Bytes::readpackint(buf);
        for (uint64_t i = 0; i < file_meta_num.i.i64; ++i) {
            uint32_t current_offset = (uint32_t) ((*buf) - payload_buf);
            if (meta_persist_payload_size - current_offset < FILE_META_PAYLOAD_MIN_SIZE) {
                fatal("meta persist file rest size too short, file corrupt, "
                        "could not read file meta info continue, current payload offset : %u",
                        current_offset);
                return true;
            }

            FileMeta* file_meta = (FileMeta*) fmalloc(sizeof(FileMeta));
            if (!file_meta->parse_from_stream(buf)) {
                return false;
            }

            _file_metas.add(file_meta->get_name(), *file_meta);
        }

        return true;
    }

private:
    Map<str, FileMeta> _file_metas;

    LocalFile _persist;
};

class FileMeta {
public:
    /**
     * @brief

     file payload:

     file_type                  1
     -----------------------------------------
     mode                       4
     -----------------------------------------
     uid                        4
     -----------------------------------------
     gid                        4
     -----------------------------------------
     create_time                8
     -----------------------------------------
     access_time                8
     -----------------------------------------
     change_time                8
     -----------------------------------------
     modify_time                8
     -----------------------------------------
     file_name                  packed_str
     -----------------------------------------
     file_size                  packed_num
     -----------------------------------------
     chunk_number               packed_num
     -----------------------------------------
     chunk0_file_path            packed_str
     -----------------------------------------
     chunk0_pos      4 | chunk0_size    4
     -----------------------------------------
     chunk1_file_path            packed_str
     -----------------------------------------
     chunk1_pos      4 | chunk1_size    4
     -----------------------------------------
     ...               | ...
     -----------------------------------------
     chunkN_file_path            packed_str
     -----------------------------------------
     chunkN_pos      4 | chunkN_size    4
     -----------------------------------------

     */
    bool parse_from_stream(char** buf) {
        _type = Bytes::readuint8(buf);
        _mode = Bytes::readuint32(buf);
        _uid = Bytes::readuint32(buf);
        _gid = Bytes::readuint32(buf);
        _create_time = Bytes::readtime(buf);
        _access_time = Bytes::readtime(buf);
        _change_time = Bytes::readtime(buf);
        _modify_time = Bytes::readtime(buf);
        Bytes::readpackstr(buf, _name);
        _file_size = Bytes::readpackint(buf).i.i64;

        if (_file_size > 0) {
            // read chunks meta
            _chunk_number = Bytes::readpackint(buf).i.i64;

            for (uint64_t i = 0; i < _chunk_number; ++i) {
                Chunk* chunk = (Chunk*) fmalloc(sizeof(Chunk));
                check_null_oom(chunk, return false, "file chunk meta");

                if (!chunk->parse_from_stream(buf)) {
                    // TODO dump bin
                    fatal("init chunk meta fild");
                    ffree(chunk);
                    return false;
                }

                _chunks.add_tail(*chunk);
            }
        }

        return true;
    }

    const packstr& get_name() const {
        return _name;
    }

private:
    uint8_t _type;
    uint32_t _mode;
    uint32_t _uid;
    uint32_t _gid;
    struct timespec _create_time;
    struct timespec _access_time;
    struct timespec _change_time;
    struct timespec _modify_time;
    packstr _name;
    uint64_t _file_size;
    uint64_t _chunk_number;

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
