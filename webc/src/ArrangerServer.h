#ifndef FDIS_ARRANGERSERVER_H
#define FDIS_ARRANGERSERVER_H

#include "rpc/RpcServer.h"
#include "proto/ArrangerService.pb.h"

namespace fdis {
class ArrangerServer : public ArrangerService {
public:
    ArrangerServer() : _server() {}

    void init(uint16_t port);

    void stop(uint32_t mills);

    void echo(::google::protobuf::RpcController* controller,
            const HeartbeatRequest* request, HeartbeatResponse* response, Closure* done);

private:
    RpcServer _server;
};
}

#endif //WEBC_ARRANGERSERVER_H
