#include <iostream>
#include <unistd.h>
#include <sstream>
#include <errno.h>
#include "google/protobuf/service.h"
#include "proto/modelService.pb.h"

#include "common/flog.h"
#include "event.h"
#include "net.h"

using google::protobuf::Message;

namespace fankux {
class ModelServiceImpl : public ModelService {

public:
    ModelServiceImpl() : ModelService() { }

    static ModelServiceImpl& instance() {
        static ModelServiceImpl model_service;
        return model_service;
    }

    void to_string(::google::protobuf::RpcController* controller,
                   const ::fankux::ModelRequest* request,
                   ::fankux::ModelResponse* response,
                   ::google::protobuf::Closure* done) {
        info("to_string achieved: %s", request->SerializeAsString().c_str());

        response->set_status(200);
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

class FServer {
public:
    FServer(const uint16_t port) {
        _port = port;
        init();
    }

    virtual ~FServer() {
        event_stop(_netinf);
        netinfo_free(_netinf);
    }

    void run() {
        event_loop(_netinf);
    }

private:
    void init() {
        _netinf = event_create(EVENT_LIST_MAX);
        _netinf->listenfd = create_tcpsocket_listen(_port);

        _ev = event_info_create(_netinf->listenfd);
        _ev->flags = EVENT_ACCEPT | EVENT_READ;
        _ev->recv_func = request_handler;
        event_add(_netinf, _ev);
    }

    static int request_handler(struct event* ev) {
        debug("request_handler fired\n");

        char buffer[1024] = "\0";
        ssize_t readlen = read_tcp(ev->fd, buffer, sizeof(buffer));
        if (readlen == -1) {
            info("read_tcp faild, fd: %d, errno:%d, error:%s\n", ev->fd, errno, strerror(errno));
            ev->status = EVENT_STATUS_ERR;
            return -1;
        }

        ModelRequest request;
        ModelResponse* response = new ModelResponse();
        request.ParseFromArray(buffer, sizeof(buffer));

        debug("buffer len:%d, buffer:, msg len:, content:", readlen, buffer,
            strlen(response->msg().c_str()), response->msg());

        // TODO..actuall call in a queue, or other thread, but do not block event thread

        debug("msg len: %s", request.descriptor()->full_name());
        ModelServiceImpl::instance().to_string(NULL, &request, response, NULL);

        ev->flags = EVENT_WRITE;
        ev->send_func = response_handler;
        ev->send_param = response;

        return 0;
    }

    static int response_handler(struct event* ev) {
        debug("response_handler fired\n");

        ModelRequest* response = (ModelRequest*) ev->send_param;
        std::string ssend = response->SerializeAsString();

        ssize_t write_len = write_tcp(ev->fd, ssend.c_str(), ssend.size());
        if (write_len == -1) {
            info("write_tcp faild, fd: %d, errno:%d, error:%s\n", ev->fd, errno, strerror(errno));
            ev->status = EVENT_STATUS_ERR;
            return -1;
        }

        ev->flags = EVENT_READ;
        debug("write finish, modify to read\n");
        ev->send_func = NULL;
        ev->send_param = NULL;
        delete response;
        return 0;
    }

private:
    uint16_t _port;
    struct event* _ev;
    struct netinf* _netinf;
};
}

int main() {
    fankux::FServer server(7234);
    server.run();
}
