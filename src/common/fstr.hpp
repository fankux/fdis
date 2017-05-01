//
// Created by fankux on 16-11-30.
//

#ifndef FDIS_FSTR_HPP
#define FDIS_FSTR_HPP

#include <string>
#include "fstr.h"

namespace fdis {

class str {
public:
    str() {
        _fstr = fstr_createlen(NULL, 32);
    };

    ~str() {
        fstr_free(_fstr);
    }

    str(const std::string& std_string) {
        _fstr = fstr_createlen(std_string.c_str(), std_string.size());
    }

    str(const char* buf) {
        _fstr = fstr_create(buf);
    }

    str& operator=(const std::string& std_string) {
        fstr_setlen(_fstr, std_string.c_str(), 0, std_string.size());
        return *this;
    }

    str& operator=(const char* buf) {
        fstr_set(_fstr, buf, 0);
        return *this;
    }

    bool operator==(const str& rhs) const {
        return _fstr == rhs._fstr;
    }

    bool operator!=(const str& rhs) const {
        return !(rhs == *this);
    }

    str& operator+(const std::string std_string) {
        _fstr = fstr_catlen(_fstr, std_string.c_str(), std_string.size());
        return *this;
    }

    str& operator+(const char* buf) {
        fstr_cat(_fstr, buf);
        return *this;
    }

    char operator[](size_t idx) {
        return _fstr->buf[idx];
    }

    const size_t size() const {
        return (const size_t) _fstr->len;
    }

    const char* buf() const {
        return _fstr->buf;
    }

    char* data() {
        return _fstr->buf;
    }

    bool isempty() const {
        return size() == 0;
    }

    void reserve(const size_t size) {
        _fstr = fstr_reserve(_fstr, size);
    }

    void assign(const char* buf, size_t size) {
        _fstr = fstr_setlen(_fstr, buf, 0, size);
    }

private:
    struct fstr* _fstr;
};
}

#endif // FDIS_FSTR_HPP
