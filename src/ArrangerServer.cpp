#include "common/common.h"
#include "common/check.h"
#include "ArrangerServer.h"
#include "arranger.h"

namespace fdis {

void ArrangerServer::init(uint16_t port) {
    // TODO, port config
    int service_id = ArrangerService::descriptor()->index();

    Provider* heartbeat = new(std::nothrow) Provider;
    check_null_oom(heartbeat, exit(EXIT_FAILURE), "init heartbeat provider");
    heartbeat->service = this;
    for (int i = 0; i < ArrangerService::descriptor()->method_count(); ++i) {
        const Method* method = ArrangerService::descriptor()->method(i);
        int method_id = method->index();

        procedure_id_t procedure_id = build_procedure_id(service_id, method_id);

        heartbeat->succ_dones.add(procedure_id, *RpcServer::default_succ);
        heartbeat->fail_dones.add(procedure_id, *RpcServer::default_fail);
    }
    _server.reg_provider(service_id, *heartbeat);
    _server.init(port);
    _server.run(true);
}

void ArrangerServer::stop(uint32_t mills) {
    _server.stop(mills);
}

/**
 * worker send heartbeat to arranger peroidly
 */
void ArrangerServer::echo(::google::protobuf::RpcController* controller,
        const HeartbeatRequest* request, HeartbeatResponse* response,
        ::google::protobuf::Closure* done) {
    info("heartbeat achieved");

    int64_t timestamp = request->timestamp();
    int64_t timemills = request->timemills();

    struct timeval now;
    gettimeofday(&now, NULL);
    response->set_timestamp(now.tv_sec);
    response->set_timemills(now.tv_usec / 1000);
    response->set_type("rrrrtype");

    info("in time: %ld.%ld, now time: %ld.%ld", timestamp, timemills, now.tv_sec,
            now.tv_usec / 1000);

    Provider* provider = _server.get_provider(this->descriptor()->index());

    Node node;
    node.id = request->id();
    node.last_report.tv_sec = timestamp;
    node.last_report.tv_usec = timemills * 1000;
    node.status = NODE_OK;
    node.sockaddr = provider->net_info.sockaddr;
    node.sockaddr.sin_port = (in_port_t) request->port();

    int re = timeval_subtract(&node.delay, &node.last_report, &now);
    if (re == -1) {
        // TODO.. node time exceed arranger time
    }

    g_ager.handle_node(&node);
}
}
