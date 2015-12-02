#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include "google/protobuf/service.h"
#include "proto/modelService.pb.h"
#include "proto/meta.pb.h"

#include "common/flog.h"
#include "common/check.h"
#include "net.h"
#include "event.h"

using fankux::ModelService;
using fankux::ModelService_Stub;
using fankux::ModelRequest;
using fankux::ModelResponse;

namespace fankux {
class FChannel : public google::protobuf::RpcChannel {
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

    void CallMethod(const google::protobuf::MethodDescriptor* method,
            google::protobuf::RpcController* controller, const google::protobuf::Message* request,
            google::protobuf::Message* response, google::protobuf::Closure* done) {

        debug("service: %s(%d)\n method: %s(%d)", method->service()->full_name().c_str(),
                method->service()->index(), method->full_name().c_str(), method->index());

        RequestMeta meta;
        meta.set_service_id(method->service()->index());
        meta.set_method_id(method->index());

        int meta_len = meta.ByteSize();
        int request_len = request->ByteSize();
        FChannel::_package_len = meta_len + request_len;
        if (FChannel::_package_len >= sizeof(FChannel::_send_buffer)) {
            // TODO send for some batch
            error("too large package, len: %d, max len: %zu", request_len,
                    sizeof(FChannel::_send_buffer));
            return;
        }

        debug("meta len: %d, request_len: %d", meta_len, request_len);
        memcpy(FChannel::_send_buffer, &FChannel::_package_len, 4);
        memcpy(FChannel::_send_buffer + 4, &meta_len, 4);
        int ret = meta.SerializeToArray(FChannel::_send_buffer + 8, meta_len);
        check_cond(ret, return, "serialize meta faild");
        ret = request->SerializeToArray(FChannel::_send_buffer + 8 + meta_len, request_len);
        check_cond(ret, return, "serialize request faild");

        _ev->flags = EVENT_WRITE;
        _ev->send_func = invoke_callback;
        _ev->recv_param = response;

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

        debug("send buffer: %s", FChannel::_send_buffer);
        ssize_t write_len = write_tcp(ev->fd, FChannel::_send_buffer,
                (size_t) FChannel::_package_len);
        if (write_len == -1) {
            error("write_tcp faild, fd: %d", ev->fd);
            ev->status = EVENT_STATUS_ERR;
            return -1;
        }

        ev->flags = EVENT_READ;
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
        if (readlen < 4) {
            error("return package too low, len:%zu", readlen);
            ev->status = EVENT_STATUS_ERR;
            return -1;
        }

        int package_len = 0;
        memcpy(&package_len, buffer, 4);
        if (package_len != readlen) {
            error("package len incorrect, packagelen:%d, readlen: %zu", package_len, readlen);
            ev->status = EVENT_STATUS_ERR;
            return -1;
        }

        int status_len = 0;
        memcpy(&status_len, buffer + 4, 4);

        ResponseStatus status;
        status.ParseFromArray(buffer + 8, status_len);
        if (status.status() != 0) {
            info("return error, status: %d, message: %s", status.status(), status.msg());
            //TODO release
            return 0;
        }

        google::protobuf::Message* response = (google::protobuf::Message*) ev->recv_param;
        response->ParseFromArray(buffer + 8 + status_len, package_len);

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
    uint16_t _port;
    std::string _addr;
    struct event* _ev;
    struct netinf* _netinf;
    pthread_t _routine_pid;

    static int _package_len;
    static char _send_buffer[1024];

    static bool _is_sync;
    static pthread_mutex_t _invoke_sync;
};

int FChannel::_package_len = 0;
char FChannel::_send_buffer[1024] = "\0";
bool FChannel::_is_sync = true;
pthread_mutex_t FChannel::_invoke_sync = PTHREAD_MUTEX_INITIALIZER;

}

using fankux::FChannel;
using namespace fankux;

int main() {
    google::protobuf::RpcChannel* channel = new FChannel("127.0.0.1", 7234);
    ((FChannel*) channel)->run(true);
    sleep(1);

    ModelService_Stub* model_service = new ModelService::Stub(channel);
    ModelRequest request;
    ModelResponse response;

    for (int i = 0; i < 1; ++i) {
        request.set_key("to_string");
        request.set_seq(1);
        model_service->to_string(NULL, &request, &response, NULL);
        sleep(1);
        info("call succ, response, msg: %s", response.msg().c_str());
        sleep(5);
        request.Clear();
        response.Clear();
    }

    return 0;
}