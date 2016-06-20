#include "arranger.h"

namespace fankux {

ArrangerConf& ArrangerConf::load(const std::string& path) {
    ArrangerConf* conf = new(std::nothrow) ArrangerConf;
    check_null_oom(conf, exit(EXIT_FAILURE), "arranger config");

    conf->_conf = fconf_create(path.c_str());
    check_null_oom(conf->_conf, exit(EXIT_FAILURE), "arranger config inner");

    conf->_default_token_num = fconf_get_uint32(conf->_conf, "default_token_num");
    conf->_default_worker_num = fconf_get_uint32(conf->_conf, "default_worker_num ");
    conf->_handle_retry = fconf_get_uint32(conf->_conf, "handle_retry ");
    conf->_lease_time = fconf_get_uint32(conf->_conf, "lease_time ");
    conf->_threadpool_min = fconf_get_uint32(conf->_conf, "threadpool_min ");
    conf->_threadpool_max = fconf_get_uint32(conf->_conf, "threadpool_max ");

    return *conf;
}

}
