#include <sstream>
#include <errno.h>

#include "proto/meta.pb.h"

#include "common/flog.h"
#include "common/check.h"
#include "net.h"
#include "rpc/RpcServer.h"

namespace fankux {

RpcServer::InvokeParam RpcServer::_invoke_param;

Closure* RpcServer::default_succ = google::protobuf::NewCallback(RpcServer::default_succ_func);

Closure* RpcServer::default_fail = google::protobuf::NewCallback(RpcServer::default_fail_func);

pthread_t RpcServer::_routine_pid;

RpcServer::~RpcServer() {
    event_stop(_netinf);
    netinfo_free(_netinf);
}

void* RpcServer::poll_routine(void* arg) {
    struct netinf* netinf = (struct netinf*) arg;
    event_loop(netinf);
    return (void*) 0;
}

void RpcServer::run(bool background) {
    if (background) {
        pthread_create(&_routine_pid, NULL, poll_routine, _netinf);
    } else {
        poll_routine(_netinf);
    }
}

void RpcServer::init(const uint16_t port) {
    _port = port;

    if (_providers.size() == 0) {
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

    InvokeParam* param = &RpcServer::_invoke_param;
    RpcServer* server = param->_server;

    int meta_len = 0;
    memcpy(&meta_len, buffer + 4, 4);
    RequestMeta meta;
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
    Provider* provider = server->_providers.get(service_id);
    if (provider == NULL) {
        param->_status = -1;
        param->_msg = "invalid service_id:" + service_id;
        error("invalid service_id: %d", service_id);
        return 0;
    }
    Service* service = provider->service;
    param->_method = const_cast<Method*>(
            service->GetDescriptor()->method(method_id));
    if (param->_method == NULL) {
        param->_status = -1;
        param->_msg = "invalid methoid: " + meta.method_id();
        error("invalid methoid: %d", meta.method_id());
        return 0;
    }

    provider_id_t provider_id = build_provider_id(service_id, method_id);
    InternalData* in_data = param->_internal_data.get(provider_id);
    if (in_data == NULL) {
        error("incorrect provider, service id: %d, method id: %d", service_id, method_id);
        ev->status = EVENT_STATUS_ERR;
        return -1;
    }

    Message* request = in_data->first;
    Message* response = in_data->second;
    request->ParseFromArray(buffer + 8 + meta_len, package_len - 8 - meta_len);

    if (ev->async) {
        // TODO..actuall call in a queue, or other thread, but do not block event thread
    } else {
        debug("start Invoke method, id: %d, full name: %s", param->_method->index(),
                param->_method->full_name().c_str());
        struct timeval start;
        struct timeval end;
        struct timeval delta;
        gettimeofday(&start, NULL);
        // TODO..controller to detect calling status
        service->CallMethod(service->GetDescriptor()->method(meta.method_id()), NULL,
                request, response, provider->succ_dones.get(provider_id));
        gettimeofday(&end, NULL);
        timeval_subtract(&delta, &start, &end);
        debug("end Invoke method, id: %d, full name: %s, time: %ldus", param->_method->index(),
                param->_method->full_name().c_str(), delta.tv_usec);
    }

    param->_status = 0;
    ev->flags = EVENT_WRITE;
    ev->send_func = response_handler;

    return 0;
}

int RpcServer::response_handler(struct event* ev) {
    debug("response_handler fired");

    InvokeParam* param = &RpcServer::_invoke_param;

    char ssend[1024];

    ResponseStatus status;
    status.set_status(param->_status);
    status.set_msg(param->_msg);

    int service_id = param->_method->service()->index();
    int method_id = param->_method->index();
    provider_id_t provider_id = build_provider_id(service_id, method_id);

    Message* response = param->_internal_data.get(provider_id)->second;

    int status_len = status.ByteSize();
    int response_len = response->ByteSize();
    int package_len = 8 + status_len + response_len;
    memcpy(ssend, &package_len, 4);
    memcpy(ssend + 4, &status_len, 4);
    int ret = response->SerializeToArray(ssend + 8 + status_len, response_len);
    check_cond(ret, return -1, "serialize response faild");

    ssize_t write_len = write_tcp(ev->fd, ssend, (size_t) package_len);
    if (write_len == -1) {
        info("write_tcp faild, fd: %d, errno:%d, error:%s", ev->fd, errno, strerror(errno));
        ev->status = EVENT_STATUS_ERR;
        return -1;
    }

    ev->flags = EVENT_READ;
    ev->send_func = NULL;
    return 0;
}

void RpcServer::reg_provider(const int id, Provider& provider) {
    Service* service = provider.service;
    int service_id = service->GetDescriptor()->index();

    for (int i = 0; i < service->GetDescriptor()->method_count(); ++i) {
        const Method* method = service->GetDescriptor()->method(i);
        int method_id = method->index();

        provider_id_t provider_id = build_provider_id(service_id, method_id);
        info("id:%d, provider register: %s(%d)==>%s(%d)", id,
                service->GetDescriptor()->full_name().c_str(), service_id,
                method->full_name().c_str(), method_id);

        InternalData* in_data = _invoke_param._internal_data.get(provider_id);
        if (in_data != NULL) {
            continue;
        }

        in_data = new(std::nothrow) InternalData;
        if (in_data == NULL) {
            fatal("server internal data alloc faild");
            exit(EXIT_FAILURE);
        }
        Message* request = service->GetRequestPrototype(method).New();
        if (request == NULL) {
            fatal("server request alloc faild");
            delete in_data;
            exit(EXIT_FAILURE);
        }
        Message* response = service->GetResponsePrototype(method).New();
        if (response == NULL) {
            fatal("server response alloc faild");
            delete in_data;
            delete request;
            exit(EXIT_FAILURE);
        }

        in_data->first = request;
        in_data->second = response;
        _invoke_param._internal_data.add(provider_id, *in_data);
    }
    _providers.add(id, provider);
}
}
