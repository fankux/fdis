#ifndef FDIS_ARRANGERCONF_H
#define FDIS_ARRANGERCONF_H

#include <stdint.h>
#include "common/fstr.hpp"
#include "config.h"

namespace fdis {

class ArrangerConf {
    friend class Arranger;

public:
    static ArrangerConf* load(const str& path);

    static void release(ArrangerConf* conf);


    const str& get_persist_path() const {
        return _persist_path;
    }

    const str& get_persist_prefix() const {
        return _persist_prefix;
    }

private:
    ArrangerConf() {};

private:
    fconf_t* _conf;

    uint16_t _server_port;
    uint16_t _chunk_port;
    uint32_t _default_token_num;
    uint32_t _default_worker_num;
    uint32_t _handle_retry;
    uint32_t _lease_time;
    uint32_t _threadpool_min;
    uint32_t _threadpool_max;

    str _persist_path;
    str _persist_prefix;

};

}

#endif //FDIS_ARRANGERCONF_H
