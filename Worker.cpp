#include <sstream>
#include "proto/modelService.pb.h"
#include "common/flog.h"
#include "Worker.h"

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
        info("to_string achieved: seq: %d, key: %s", request->seq(), request->key().c_str());

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
        info("hello achieved: seq:%d, key: %s", request->seq(), request->key().c_str());

        std::stringstream ss;
        ss << "seq : " << request->seq() << ", key:" << request->key() << std::endl;
//        response->set_msg(ss.str());

        if (done) {
            done->Run();
        }
    }

};

void Worker::init() {
    info("worker init, id: %d", _id);

    _server.reg_provider(ModelServiceImpl::descriptor()->index(), ModelServiceImpl::instance());
    _server.init(7234);
}

void Worker::run() {
    _server.run();
}
}
