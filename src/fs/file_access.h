//
// Created by fankux on 16-11-30.
//

#ifndef FDIS_FILE_ACCESS_H
#define FDIS_FILE_ACCESS_H

#include <stdlib.h>
#include "common/fstr.hpp"

namespace fdis {
class FileLocal {
public:
    FileLocal() : _fp(NULL) {}

    int open(const str& str, const str& mode) {
        _fp = fopen(str.buf(), mode.buf());
        if (_fp == NULL) {
            return -1;
        }
        return 0;
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

    FileLocal* create_file() {

    }

private:
    FileStoreType _type;
};
}

#endif // FDIS_FILE_ACCESS_H
