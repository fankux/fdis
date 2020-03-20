#ifdef DEBUG_FLIST

#include <stdio.h>
#include "common/fstr.hpp"
#include "common/sortlist.hpp"

class TestSortValue {
public:
    TestSortValue() {
        _val = 0;
    }

    explicit TestSortValue(const int val) {
        _val = val;
    }

    TestSortValue(const TestSortValue& rhs) {
        _val = rhs._val;
    }

    bool operator<=(const TestSortValue& rhs) const {
        return _val <= rhs._val;
    }

    bool operator==(const TestSortValue& rhs) const {
        return _val == rhs._val;
    }

    fdis::str tostr() const {
        char buf[30];
        snprintf(buf, sizeof(buf) - 1, "%d", _val);
        return fdis::str(buf);
    }

    const uint64_t hash_code() const {
        return (uint64_t) _val;
    }

private:
    int _val;
};

int main() {

    fdis::SortList<TestSortValue> list;
    list.insert(TestSortValue(1));
//    list.info();
    list.insert(TestSortValue(3));
    list.info();
    list.insert(TestSortValue(2));
    list.info();
    list.insert(TestSortValue(5));
    list.insert(TestSortValue(5));
    list.info();
    list.insert(TestSortValue(4));
    list.insert(TestSortValue(4));
    list.info();

    list.remove(TestSortValue(1));
    list.info();
    list.remove(TestSortValue(3));
    list.remove(TestSortValue(5));
    list.info();
    list.insert(TestSortValue(5));
    list.info();
    list.remove(TestSortValue(5));
    list.info();

    return 0;
}

#endif
