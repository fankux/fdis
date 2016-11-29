//
// Created by fankux on 16-11-30.
//

#ifndef WEBC_FSTR_HPP
#define WEBC_FSTR_HPP

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
        fstr_catlen(_fstr, std_string.c_str(), std_string.size());
        return *this;
    }

    str& operator+(const char* buf) {
        fstr_cat(_fstr, buf);
        return *this;
    }

    const size_t size() const {
        return (const size_t) _fstr->len;
    }

    const char* buf() const {
        return _fstr->buf;
    }

    char operator[](size_t idx) {
        return _fstr->buf[idx];
    }

private:
    struct fstr* _fstr;
};

};

#endif //WEBC_FSTR_HPP
