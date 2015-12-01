#ifndef WEBC_WOKER_H
#define WEBC_WOKER_H

#include <stdint.h>
#include <sstream>
#include "proto/modelService.pb.h"

#include "common/flog.h"
#include "RpcServer.h"

namespace fankux {

class ModelServiceImpl : public ModelService {

public:
    ModelServiceImpl() : ModelService() {}

    static ModelServiceImpl& instance() {
        static ModelServiceImpl model_service;
        return model_service;
    }

    void to_string(::google::protobuf::RpcController* controller,
            const ::fankux::ModelRequest* request,
            ::fankux::ModelResponse* response,
            ::google::protobuf::Closure* done) {
        info("to_string achieved: %s", request->SerializeAsString().c_str());

        response->set_error(0);

        std::stringstream ss;
        ss << "seq : " << request->seq() << ", key:" << request->key() << std::endl;
        response->set_msg(ss.str());

        if (done) {
            done->Run();
        }
    }

    void hello(::google::protobuf::RpcController* controller,
            const ::fankux::ModelRequest* request,
            ::fankux::ModelResponse* response,
            ::google::protobuf::Closure* done) {
        info("hello achieved: %s", request->SerializeAsString().c_str());
    }

};


class Woker {

public:
    Woker() : _id(0), _server(RpcServerFactory::instance()) {
    }

    void init() {
        info("woker init, id: %d", _id);

        _server.reg_provider(ModelServiceImpl::descriptor()->index(), ModelServiceImpl::instance());
        _server.init(7234);
    }

    void run() {
        _server.run();
    }

private:
    uint32_t _id;
    RpcServer& _server;
};
}

#endif //WEBC_WOKER_H


using fankux::Woker;

#define DEBUG_WORKER
#ifdef DEBUG_WORKER
#define DEBUG_WORKER

int main(void) {
    Woker woker;
    woker.init();
    woker.run();

    return 0;
}

#endif
