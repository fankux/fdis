#include "common/check.h"
#include "ArrangerConf.h"

namespace fdis {

ArrangerConf* ArrangerConf::load(const str& path) {
    ArrangerConf* conf = new(std::nothrow) ArrangerConf;
    check_null_oom(conf, exit(EXIT_FAILURE), "arranger config");

    conf->_conf = fconf_create(path.buf());
    check_null(conf->_conf, exit(EXIT_FAILURE), "arranger config inner init faild");

    conf->_default_token_num = fconf_get_uint32(conf->_conf, "default_token_num");
    conf->_default_worker_num = fconf_get_uint32(conf->_conf, "default_worker_num");
    conf->_handle_retry = fconf_get_uint32(conf->_conf, "handle_retry");
    conf->_lease_time = fconf_get_uint32(conf->_conf, "lease_time");
    conf->_threadpool_min = fconf_get_uint32(conf->_conf, "threadpool_min");
    conf->_threadpool_max = fconf_get_uint32(conf->_conf, "threadpool_max");

    conf->_server_port = fconf_get_uint16_dft(conf->_conf, "server_port", 7234);
    conf->_chunk_port = fconf_get_uint16_dft(conf->_conf, "chunk_port", 7235);

    conf->_persist_path = fconf_get(conf->_conf, "persist_path");
    conf->_persist_prefix = fconf_get(conf->_conf, "persist_prefix");

    return conf;
}

void ArrangerConf::release(ArrangerConf* conf) {
    delete conf;
}

}
