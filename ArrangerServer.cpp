#include "common/common.h"
#include "common/check.h"
#include "ArrangerServer.h"

namespace fankux {

void ArrangerServer::init() {
    // TODO, port config
    int service_id = ArrangerService::descriptor()->index();

    Provider* heartbeat = new(std::nothrow) Provider;
    check_null_oom(heartbeat, exit(EXIT_FAILURE), "init heartbeat provider");
    heartbeat->service = this;
    for (int i = 0; i < ArrangerService::descriptor()->method_count(); ++i) {
        const Method* method = ArrangerService::descriptor()->method(i);
        int method_id = method->index();

        provider_id_t provider_id = build_provider_id(service_id, method_id);
        heartbeat->succ_dones.add(provider_id, *RpcServer::default_succ);
        heartbeat->fail_dones.add(provider_id, *RpcServer::default_fail);
    }
    _server.reg_provider(service_id, *heartbeat);
    _server.init(7234);
    _server.run(true);
}

/**
 * worker send heartbeat to arranger peroidly
 */
void ArrangerServer::echo(::google::protobuf::RpcController* controller,
        const HeartbeatRequest* request, HeartbeatResponse* response,
        ::google::protobuf::Closure* done) {
    info("heart beat achieved");

    int64_t timestamp = request->timestamp();
    int64_t timemills = request->timemills();

//    int32_t id = request->id();
//    struct Node* node = (Node*) malloc(sizeof(node));
//    if (node == NULL) {
//        warn("get node faild, id : %d", id);
//        return;
//    }
//    g_ager->handle_node(&g_ager->_nodes[id]);

    struct timeval now;
    gettimeofday(&now, NULL);
    response->set_timestamp(now.tv_sec);
    response->set_timemills(now.tv_usec / 1000);
    response->set_type("rrrrtype");

    info("in time: %lld.%lld, now time: %lld.%lld", timestamp, timemills, now.tv_sec,
            now.tv_usec / 1000);
}
}
