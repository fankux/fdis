#include <sstream>
#include <errno.h>

#include "proto/meta.pb.h"
#include "RpcServer.h"
#include "common/flog.h"
#include "common/check.h"
#include "event.h"
#include "net.h"

using google::protobuf::Message;

namespace fankux {

RpcServer::InvokeParam RpcServer::_invoke_param;

RpcServer::RpcServer() : _port(0), _ev(NULL), _netinf(NULL) {}

RpcServer::~RpcServer() {
    event_stop(_netinf);
    netinfo_free(_netinf);
}

void RpcServer::run() {
    event_loop(_netinf);
}

void RpcServer::init(const uint16_t port) {
    _port = port;

    if (_services.size() == 0) {
        warn("rpc server no service...");
    }

    _netinf = event_create(EVENT_LIST_MAX);
    check_null_oom(_netinf, exit(EXIT_FAILURE), "epoll api info init");
    _netinf->listenfd = create_tcpsocket_listen(_port);
    _ev = event_info_create(_netinf->listenfd);
    check_null_oom(_ev, exit(EXIT_FAILURE), "list event init");
    RpcServer::_invoke_param._server = this;
    _ev->flags = EVENT_ACCEPT | EVENT_READ;
    _ev->recv_func = request_handler;
    _ev->recv_param = &RpcServer::_invoke_param;
    event_add(_netinf, _ev);
}

int RpcServer::request_handler(struct event* ev) {
    debug("request_handler fired");

    char buffer[1024] = "\0";
    ssize_t readlen = read_tcp(ev->fd, buffer, sizeof(buffer));
    if (readlen == -1) {
        info("read_tcp faild, fd: %d, errno:%d, error:%s", ev->fd, errno, strerror(errno));
        ev->status = EVENT_STATUS_ERR;
        return -1;
    }

    int package_len = 0;
    memcpy(&package_len, buffer, 4);
    if (readlen <= 4 || package_len != readlen) {
        info("tcp package length incorrect, drop it, read len: %zu, package len: %d",
                readlen, package_len);
        ev->status = EVENT_STATUS_ERR;
        return -1;
    }

    InvokeParam* param = (InvokeParam*) ev->recv_param;
    RpcServer* server = param->_server;

    int meta_len = 0;
    memcpy(&meta_len, buffer + 4, 4);
    RequestMeta meta;

    // TODO.. check length

    //set service id and method id due to calculate size
    debug("meta len: %d", meta_len);
    int ret = meta.ParseFromArray(buffer + 8, meta_len);
    if (!ret) {
        param->_status = -1;
        param->_msg = "RequestMeta parse faild";
        error("RequestMeta parse faild");
        return 0;
    }

    int service_id = meta.service_id();
    int method_id = meta.method_id();
    debug("service id: %d, method id: %d", service_id, method_id);
    google::protobuf::Service* service = server->_services.get(meta.service_id());
    if (service == NULL) {
        param->_status = -1;
        param->_msg = "invalid service_id:" + service_id;
        error("invalid service_id: %d", method_id);
        return 0;
    }

    param->_method = const_cast<google::protobuf::MethodDescriptor*>(
            service->GetDescriptor()->method(method_id));
    if (param->_method == NULL) {
        param->_status = -1;
        param->_msg = "invalid methoid: " + meta.method_id();
        error("invalid methoid: %d", meta.method_id());
        return 0;
    }

    // TODO  same type _request & _response pool
    if (param->_request) {
        delete param->_request;
        param->_request;
    }
    param->_request = service->GetRequestPrototype(param->_method).New();
    if (param->_request == NULL) {
        param->_status = -1;
        param->_msg = "server request alloc faild";
        error("server request alloc faild");
        return 0;
    }
    param->_request->ParseFromArray(buffer + 8 + meta_len, package_len - 8 - meta_len);

    if (param->_response) {
        delete param->_response;
        param->_response = NULL;
    }
    param->_response = service->GetResponsePrototype(param->_method).New();
    if (param->_response == NULL) {
        param->_status = -1;
        param->_msg = "server response alloc faild";
        error("server response alloc faild");
        return 0;
    }

    if (ev->async) {
        // TODO..actuall call in a queue, or other thread, but do not block event thread
    } else {
        debug("start Invoke method, id: %d, full name: %s", param->_method->index(),
                param->_method->full_name().c_str());
        struct timeval start;
        struct timeval end;
        struct timeval delta;
        gettimeofday(&start, NULL);
        service->CallMethod(service->GetDescriptor()->method(meta.method_id()), NULL,
                param->_request, param->_response, NULL);
        gettimeofday(&end, NULL);
        timeval_subtract(&delta, &start, &end);
        debug("end Invoke method, id: %d, full name: %s, time: %ldus", param->_method->index(),
                param->_method->full_name().c_str(), delta.tv_usec);
    }

    ev->flags = EVENT_WRITE;
    ev->send_func = response_handler;
    ev->send_param = param;

    return 0;
}

int RpcServer::response_handler(struct event* ev) {
    debug("response_handler fired");

    InvokeParam* param = reinterpret_cast<InvokeParam*> (ev->recv_param);

    char ssend[1024];

    ResponseStatus status;
    status.set_status(param->_status);
    status.set_msg(param->_msg);

    int status_len = status.ByteSize();
    int response_len = param->_response->ByteSize();
    int package_len = status_len + response_len;
    memcpy(ssend, &package_len, 4);
    memcpy(ssend + 4, &status_len, 4);
    int ret = param->_response->SerializeToArray(ssend + 8, response_len);
    check_cond(ret, return -1, "serialize response faild");

    ssize_t write_len = write_tcp(ev->fd, ssend, (size_t) package_len);
    if (write_len == -1) {
        info("write_tcp faild, fd: %d, errno:%d, error:%s", ev->fd, errno, strerror(errno));
        ev->status = EVENT_STATUS_ERR;
        return -1;
    }

    ev->flags = EVENT_READ;
    debug("write finish, modify to read");
    ev->send_func = NULL;
    ev->send_param = NULL;
    delete param->_response;
    return 0;
}

void RpcServer::reg_provider(const int id, google::protobuf::Service& service) {
    _services.add(id, service);
}
}

#ifdef DEBUG_RPCSERVER
#include "Woker.h"
int main() {
    fankux::RpcServer server;
    server.reg_provider(ModelServiceImpl::descriptor()->index(), ModelServiceImpl::instance());
    server.init(7234);
    server.run();
}
#endif