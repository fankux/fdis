#define LOG_LEVEL 10

#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include "proto/modelService.pb.h"
#include "proto/meta.pb.h"

#include "rpc.h"
#include "common/flog.h"
#include "common/check.h"
#include "net.h"

namespace fankux {

Map<provider_id_t, pthread_mutex_t> RpcClient::_provider_locks;
Map<provider_id_t, InvokePackage> RpcClient::_invoke_packages;
Map<provider_id_t, MutexCond> RpcClient::_invoke_signals;

pthread_mutex_t RpcClient::_lock = PTHREAD_MUTEX_INITIALIZER;

RpcClient::RpcClient(const std::string& addr, uint16_t port) {
    _addr = addr;
    _port = port;
    init();
}

RpcClient::~RpcClient() {
    event_stop(_netinf);
    pthread_join(_routine_pid, NULL);
}

void RpcClient::run(bool background) {
    if (background) {
        pthread_create(&_routine_pid, NULL, poll_routine, _netinf);
    } else {
        poll_routine(_netinf);
    }
}

void RpcClient::CallMethod(const google::protobuf::MethodDescriptor* method,
        google::protobuf::RpcController* controller,
        const google::protobuf::Message* request,
        google::protobuf::Message* response, google::protobuf::Closure* done) {
    int service_id = method->service()->index();
    int method_id = method->index();
    debug("service: %s(%d)==>method: %s(%d)", method->service()->full_name().c_str(),
            service_id, method->full_name().c_str(), method_id);

    provider_id_t provider_id = build_provider_id(service_id, method_id);
    pthread_mutex_t* provider_lock = get_provider_lock(service_id, method_id, provider_id);
    if (provider_lock == NULL) {
        return;
    }
    debug("lock accquiring, pid: %ld", provider_id);
    pthread_mutex_lock(provider_lock);
    debug("lock accquired!!!, pid: %ld", provider_id);

    InvokePackage* package = get_invoke_package(service_id, method_id, provider_id);
    if (package == NULL) {
        pthread_mutex_unlock(provider_lock);
        return;
    }
    package->_response = response;
    bool ret = populate_invoke_package(package, service_id, method_id, request);
    if (!ret) {
        pthread_mutex_unlock(provider_lock);
        return;
    }

    MutexCond* invoke_signal = get_invoke_signal(service_id, method_id, provider_id);
    if (invoke_signal == NULL) {
        pthread_mutex_unlock(provider_lock);
        return;
    }

    struct event* ev = RpcClient::_evs.get(provider_id);
    debug("signal lock accquiring");
    pthread_mutex_lock(&invoke_signal->first);

    // FIXME reconnect
    if (ev == NULL) {
        ev = event_info_create(create_tcpsocket());
        if (ev == NULL) {
            error("invoke event, service id:%d, method id:%d", service_id, method_id);
            pthread_mutex_unlock(provider_lock);
            pthread_mutex_unlock(&invoke_signal->first);
            return;
        }

        RpcClient::_evs.add(provider_id, *ev);
        ev->flags = EVENT_REUSE | EVENT_WRITE;
        ev->faild_func = faild_callback;
        ev->faild_param = (void*) method;
        ev->send_func = invoke_callback;
        ev->send_param = (void*) method;

        if (!connect(ev)) {
            error("invoke connect error, service id:%d, method id:%d", service_id, method_id);
            pthread_mutex_unlock(provider_lock);
            pthread_mutex_unlock(&invoke_signal->first);
            return;
        }
    } else {
        if (ev->status == EVENT_STATUS_ERR) {
            if (!connect(ev)) {
                pthread_mutex_unlock(provider_lock);
                pthread_mutex_unlock(&invoke_signal->first);
                return;
            }
        }
        ev->flags = EVENT_REUSE | EVENT_WRITE;
        ev->send_func = invoke_callback;
        ev->send_param = (void*) method;
        event_mod(_netinf, ev);
    }

    // blocking untill invoking finish
    debug("signal lock accquired, waiting!!!");
    pthread_cond_wait(&invoke_signal->second, &invoke_signal->first);
    debug("signal waked!!!");
    pthread_mutex_unlock(&invoke_signal->first);
    debug("signal lock unlocked!!!");

    pthread_mutex_unlock(provider_lock);
    debug("lock unlocked!!!, pid: %ld", provider_id);
}

InvokePackage* RpcClient::get_invoke_package(const int sid, const int mid,
        const provider_id_t pid) {
    InvokePackage* invoke_queue = RpcClient::_invoke_packages.get(pid);
    if (invoke_queue == NULL) {
        invoke_queue = (InvokePackage*) malloc(sizeof(InvokePackage));
        check_null_oom(invoke_queue, return NULL,
                "invoke queue, service id:%d, method id:%d", sid, mid);
        int ret = RpcClient::_invoke_packages.add(pid, *invoke_queue);
        debug("add ret:%d", ret);
    }
    return invoke_queue;
}

pthread_mutex_t* RpcClient::get_provider_lock(const int sid, const int mid,
        const provider_id_t pid) {

    pthread_mutex_t* lock = RpcClient::_provider_locks.get(pid);
    if (lock == NULL) {
        // double check, lock if provider lock not available yet in cause of double new
        pthread_mutex_lock(&RpcClient::_lock);
        if ((lock = RpcClient::_provider_locks.get(pid)) == NULL) {
            lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
            check_null_oom(lock, pthread_mutex_unlock(&RpcClient::_lock), return NULL,
                    "provider lock , service id:%d, method id:%d", sid, mid);
            pthread_mutex_init(lock, NULL);
            RpcClient::_provider_locks.add(pid, *lock);
        }
        pthread_mutex_unlock(&RpcClient::_lock);
    }
    return lock;
}

MutexCond* RpcClient::get_invoke_signal(const int sid, const int mid,
        const provider_id_t pid) {
    MutexCond* pair = RpcClient::_invoke_signals.get(pid);
    if (pair == NULL) {
        // double check, lock if provider lock not available yet in cause of double new
        pthread_mutex_lock(&RpcClient::_lock);
        if ((pair = RpcClient::_invoke_signals.get(pid)) == NULL) {
            pair = new(std::nothrow) std::pair<pthread_mutex_t, pthread_cond_t>();
            check_null_oom(pair, pthread_mutex_unlock(&RpcClient::_lock), return NULL,
                    "signal <lock cond> pair, service id:%d, method id:%d", sid, mid);
            pthread_mutex_t provider_lock = PTHREAD_MUTEX_INITIALIZER;
            pthread_cond_t provider_cond = PTHREAD_COND_INITIALIZER;
            pair->first = provider_lock;
            pair->second = provider_cond;
            RpcClient::_invoke_signals.add(pid, *pair);
        }
        pthread_mutex_unlock(&RpcClient::_lock);
    }
    return pair;
}

bool RpcClient::populate_invoke_package(InvokePackage* package, const int sid, const int mid,
        const google::protobuf::Message* request) {
    RequestMeta meta;
    meta.set_service_id(sid);
    meta.set_method_id(mid);

    int meta_len = meta.ByteSize();
    int request_len = request->ByteSize();
    package->_package_len = 8 + meta_len + request_len;
    if (package->_package_len >= sizeof(package->_recv_buffer)) {
        // TODO send for some batch
        error("too large package, len: %d, max len: %zu", request_len,
                sizeof(package->_recv_buffer));
        return false;
    }

//    debug("meta len: %d, request_len: %d, ModelRequest len: %d", meta_len, request_len,
//            ((ModelRequest*) request)->ByteSize());
    memcpy(package->_recv_buffer, &package->_package_len, 4);
    memcpy(package->_recv_buffer + 4, &meta_len, 4);
    bool ret = meta.SerializeToArray(package->_recv_buffer + 8, meta_len);
    if (!ret) {
        error("serialize meta faild");
        return false;
    }
    ret = request->SerializeToArray(package->_recv_buffer + 8 + meta_len, request_len);
    if (!ret) {
        error("serialize request faild");
        return false;
    }

    return true;
}

bool RpcClient::connect(struct event* ev) {
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(_port);
    addr.sin_addr.s_addr = inet_addr(_addr.c_str());
    return event_connect(_netinf, ev, &addr) == 0;
}

void* RpcClient::poll_routine(void* arg) {
    struct fankux::netinf* netinf = (struct fankux::netinf*) arg;
    event_loop(netinf);
    return (void*) 0;
}

static inline int wakeup_invoke(provider_id_t provider_id, Map<provider_id_t, MutexCond>* signals) {
    MutexCond* pair = signals->get(provider_id);
    if (pair == NULL) {
        return -1;
    }
    debug("signal lock accquiring");
    pthread_mutex_lock(&pair->first);
    debug("signal lock accquired, signal!!!");
    pthread_cond_signal(&pair->second);
    debug("signal signaled!!!");
    pthread_mutex_unlock(&pair->first);
    debug("signal lock unlocked!!!");
    return 0;
}

int RpcClient::faild_callback(struct event* ev) {
    debug("faild fired");

    google::protobuf::MethodDescriptor* method =
            (google::protobuf::MethodDescriptor*) ev->faild_param;
    int service_id = method->service()->index();
    int method_id = method->index();
    provider_id_t provider_id = build_provider_id(service_id, method_id);

    int ret = wakeup_invoke(provider_id, &RpcClient::_invoke_signals);
    check_cond(ret == 0, return 0,
            "provider signal invalid, service id:%d, method id:%d, provider id : %d",
            service_id, method_id, provider_id);
    return 0;
}

int RpcClient::invoke_callback(struct event* ev) {
    debug("invoke fired");

    ev->flags = EVENT_REUSE;
    google::protobuf::MethodDescriptor* method =
            (google::protobuf::MethodDescriptor*) ev->send_param;
    int service_id = method->service()->index();
    int method_id = method->index();
    provider_id_t provider_id = build_provider_id(service_id, method_id);
    InvokePackage* package = RpcClient::_invoke_packages.get(provider_id);
    if (package == NULL) {
        warn("empty package, maybe error occured already, service id:%d, method id:%d",
                service_id, method_id);
        wakeup_invoke(provider_id, &RpcClient::_invoke_signals);
        return 0;
    }

    ssize_t write_len = write_tcp(ev->fd, package->_recv_buffer, (size_t) package->_package_len);
    if (write_len == -1) {
        error("write_tcp faild, fd: %d", ev->fd);
        ev->status = EVENT_STATUS_ERR;
        wakeup_invoke(provider_id, &RpcClient::_invoke_signals);
        return -1;
    }

    ev->flags = EVENT_REUSE | EVENT_READ;
    ev->recv_func = return_callback;
    return 0;
}

int RpcClient::return_callback(struct event* ev) {
    debug("return fired");

    ev->flags = EVENT_REUSE;

    google::protobuf::MethodDescriptor* method =
            (google::protobuf::MethodDescriptor*) ev->send_param;
    int service_id = method->service()->index();
    int method_id = method->index();
    provider_id_t provider_id = build_provider_id(service_id, method_id);
    InvokePackage* package = RpcClient::_invoke_packages.get(provider_id);
    if (package == NULL) {
        warn("empty package, maybe error occured already, service id:%d, method id:%d",
                service_id, method_id);
        goto end;
    }

    ssize_t readlen = read_tcp(ev->fd, package->_recv_buffer, sizeof(package->_recv_buffer));
    if (readlen == -1) {
        error("read_tcp faild, fd: %d, errno:%d, error:%s", ev->fd, errno, strerror(errno));
        ev->status = EVENT_STATUS_ERR;
        wakeup_invoke(provider_id, &RpcClient::_invoke_signals);
        return -1;
    }
    if (readlen < 8) {
        error("return package too low, len:%zu", readlen);
        ev->status = EVENT_STATUS_ERR;
        wakeup_invoke(provider_id, &RpcClient::_invoke_signals);
        return -1;
    }

    int package_len = 0;
    memcpy(&package_len, package->_recv_buffer, 4);
    if (package_len != readlen) {
        error("package len incorrect, package len:%d, read len: %zu", package_len, readlen);
        ev->status = EVENT_STATUS_ERR;
        goto end;
    }

    int status_len = 0;
    memcpy(&status_len, package->_recv_buffer + 4, 4);

    ResponseStatus status;
    status.ParseFromArray(package->_recv_buffer + 8, status_len);
    if (status.status() != 0) {
        info("return error, status: %d, message: %s", status.status(), status.msg().c_str());
        goto end;
    }

    // if status is 0, then request content must be existed
    if (package_len - 8 - status_len <= 0) {
        info("request not occured, error package");
        goto end;
    }

    bool ret = package->_response->ParseFromArray(package->_recv_buffer + 8 + status_len,
            package_len - 8 - status_len);
    if (!ret) {
        info("unserialize faild, error package");
        goto end;
    }

    end:
    ev->faild_func = NULL;
    ev->send_param = NULL;
    ev->send_func = NULL;
    ev->recv_param = NULL;
    ev->recv_func = NULL;
    wakeup_invoke(provider_id, &RpcClient::_invoke_signals);
    return 0;
}

void RpcClient::init() {
    _netinf = event_create(EVENT_LIST_MAX);
}
}

#ifdef DEBUG_RPCCLIENT
using fankux::RpcClient;
using namespace fankux;

int i;

void* call_routine(void* arg) {
    ModelService_Stub* model_service = (ModelService_Stub*) arg;

    ModelResponse response;
    ModelRequest request;
    request.set_key("to_string");
    request.set_seq((google::protobuf::int32) pthread_self());

    if (i % 2) {
        model_service->to_string(NULL, &request, &response, NULL);
    } else {
        model_service->hello(NULL, &request, &response, NULL);
    }

    info("invoking finished, response, msg: %s", response.msg().c_str());
    request.Clear();
    response.Clear();

    return (void*) 0;
}

int main(int argc, char** argv) {
    google::protobuf::RpcChannel* channel = new RpcClient("127.0.0.1", 7234);
    ((RpcClient*) channel)->run(true);
    sleep(1);

    ModelService_Stub* model_service = new ModelService::Stub(channel);

    int num = 0;
    if (argc > 1) {
        num = atoi(argv[1]);
    }

    pthread_t pids[num];
    for (i = 1; i <= num; ++i) {
        pthread_create(&pids[i], NULL, call_routine, model_service);
    }

    for (i = 1; i <= num; ++i) {
        pthread_join(pids[i], NULL);
    }

    return 0;
}

#endif
