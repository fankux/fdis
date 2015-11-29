#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include "google/protobuf/service.h"
#include "proto/modelService.pb.h"

#include "common/flog.h"
#include "net.h"
#include "event.h"

using google::protobuf::RpcChannel;
using google::protobuf::Service;
using google::protobuf::Closure;
using fankux::ModelService;
using fankux::ModelService_Stub;
using fankux::ModelRequest;
using fankux::ModelResponse;
using google::protobuf::Message;
using google::protobuf::Closure;
using google::protobuf::MethodDescriptor;
using google::protobuf::RpcController;
using google::protobuf::NewCallback;

namespace fankux {
class FChannel : public RpcChannel {
public:
    FChannel(const std::string& addr, uint16_t port) {
        _addr = addr;
        _port = port;
        init();
    }

    virtual ~FChannel() {
        event_stop(_netinf);
        pthread_join(_routine_pid, NULL);
    }

    void run(bool background = false) {
        if (background) {
            pthread_create(&_routine_pid, NULL, poll_routine, _netinf);
        } else {
            poll_routine(_netinf);
        }
    }

    void CallMethod(const MethodDescriptor* method, RpcController* controller,
                    const Message* request, Message* response, Closure* done) {

        debug("method: %s_%d", method->full_name(), method->index());

        _ev->flags = EVENT_WRITE;
        _ev->send_func = invoke_callback;
        _ev->recv_param = response;
        _ev->send_param = const_cast<Message*>(request);

        event_mod(_netinf, _ev);
    }

private:
    void connect() {
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(_port);
        addr.sin_addr.s_addr = inet_addr(_addr.c_str());
        event_connect(_netinf, _ev, &addr);
    }

    static void* poll_routine(void* arg) {
        struct fankux::netinf* netinf = (struct fankux::netinf*) arg;
        event_loop(netinf);
        return (void*) 0;
    }

    static int invoke_callback(struct event* ev) {
        debug("invoke fired");

        ModelRequest* request = (ModelRequest*) ev->send_param;
        std::string ssend = request->SerializeAsString();

        ssize_t write_len = write_tcp(ev->fd, ssend.c_str(), ssend.size());
        if (write_len == -1) {
            error("write_tcp faild, fd: %d", ev->fd);
            ev->status = EVENT_STATUS_ERR;
            return -1;
        }

        ev->flags = EVENT_READ;
        ev->send_func = NULL;
        ev->recv_func = return_callback;
        return 0;
    }

    static int return_callback(struct event* ev) {
        debug("return fired");

        char buffer[1024] = "\0";
        ssize_t readlen = read_tcp(ev->fd, buffer, sizeof(buffer));
        if (readlen == -1) {
            error("read_tcp faild, fd: %d, errno:%d, error:%s", ev->fd, errno, strerror(errno));
            ev->status = EVENT_STATUS_ERR;
            return -1;
        }

        ModelResponse* response = (ModelResponse*) ev->recv_param;
        response->ParseFromArray(buffer, sizeof(buffer));

        ev->flags = 0;
        ev->send_param = NULL;
        ev->send_func = NULL;
        ev->recv_param = NULL;
        ev->recv_func = NULL;
        return 0;
    }

    void init() {
        _netinf = event_create(EVENT_LIST_MAX);
        _ev = event_info_create(create_tcpsocket());
        _ev->flags = 0;
        _ev->keepalive = 1;
        connect();
    }

private:
    std::string _addr;
    uint16_t _port;
    struct event* _ev;
    struct netinf* _netinf;
    pthread_t _routine_pid;

    static bool _is_sync;
    static pthread_mutex_t _invoke_sync;
};

bool FChannel::_is_sync = true;
pthread_mutex_t FChannel::_invoke_sync = PTHREAD_MUTEX_INITIALIZER;

}

using fankux::FChannel;

int main() {
    RpcChannel* channel = new FChannel("127.0.0.1", 7234);
    ((FChannel*) channel)->run(true);
    sleep(1);

    ModelService_Stub* model_service = new ModelService::Stub(channel);
    ModelRequest request;
    ModelResponse response;

    request.set_key("to_string");
    request.set_seq(1);
    model_service->to_string(NULL, &request, &response, NULL);
    sleep(1);
    std::cout << "call succ, response:" << response.SerializeAsString() <<
    std::endl;
    sleep(5);
    request.Clear();
    response.Clear();

    request.set_key("hello");
    request.set_seq(2);
    model_service->hello(NULL, &request, &response, NULL);
    sleep(1);
    std::cout << "call succ, response:" << response.SerializeAsString() <<
    std::endl;

    return 0;
}