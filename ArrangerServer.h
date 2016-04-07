#ifndef FANKUX_ARRANGERSERVER_H
#define FANKUX_ARRANGERSERVER_H

#include "rpc/RpcServer.h"
#include "proto/ArrangerService.pb.h"

namespace fankux {
class ArrangerServer : public ArrangerService {
public:
    ArrangerServer() : _server() {}

    void init();

    void echo(::google::protobuf::RpcController* controller,
            const HeartbeatRequest* request, HeartbeatResponse* response, Closure* done);

private:
    RpcServer _server;
};
}

#endif //WEBC_ARRANGERSERVER_H
