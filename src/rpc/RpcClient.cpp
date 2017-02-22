#define LOG_LEVEL 10

#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include "proto/meta.pb.h"

#include "common/flog.h"
#include "common/check.h"
#include "net.h"
#include "rpc/RpcClient.h"
#include "rpc/InvokeController.h"

namespace fdis {

Map<procedure_id_t, pthread_mutex_t> RpcClient::_provider_locks(FMAP_T_INT64, FMAP_DUP_KEY);
Map<procedure_id_t, InvokePackage> RpcClient::_invoke_packages(FMAP_T_INT64, FMAP_DUP_KEY);
Map<procedure_id_t, MutexCond> RpcClient::_invoke_signals(FMAP_T_INT64, FMAP_DUP_KEY);

pthread_mutex_t RpcClient::_lock = PTHREAD_MUTEX_INITIALIZER;

RpcClient::~RpcClient() {
    event_stop(_netinf);
    pthread_join(_routine_pid, NULL);
}

void RpcClient::init(const std::string& addr, uint16_t port) {
    _addr = addr;
    _port = port;
    _netinf = event_create(EVENT_LIST_MAX);
}

void RpcClient::run(bool background) {
    if (background) {
        pthread_create(&_routine_pid, NULL, poll_routine, _netinf);
    } else {
        poll_routine(_netinf);
    }
}

/**
 * done is not null, invoke async
 * else invoke sync
 */
void RpcClient::CallMethod(const google::protobuf::MethodDescriptor* method,
        google::protobuf::RpcController* controller,
        const google::protobuf::Message* request,
        google::protobuf::Message* response, google::protobuf::Closure* done) {
    std::string errmsg;
    int service_id = method->service()->index();
    int method_id = method->index();
    debug("service: %s(%d)==>method: %s(%d)", method->service()->full_name().c_str(),
            service_id, method->full_name().c_str(), method_id);

    procedure_id_t procedure_id = build_procedure_id(service_id, method_id);
    pthread_mutex_t* provider_lock = get_provider_lock(service_id, method_id, procedure_id);
    if (provider_lock == NULL) {
        errmsg = "faild to get provider lock";
        goto faild;
    }
//    debug("lock accquiring, pid: %ld", procedure_id);
    pthread_mutex_lock(provider_lock);
//    debug("lock accquired!!!, pid: %ld", procedure_id);

    InvokePackage* package = get_invoke_package(service_id, method_id, procedure_id);
    if (package == NULL) {
        pthread_mutex_unlock(provider_lock);
        errmsg = "faild to get invoke package";
        goto faild;
    }
    package->response = response;
    if (!populate_invoke_package(package, service_id, method_id, request, done == NULL)) {
        pthread_mutex_unlock(provider_lock);
        errmsg = "faild to populate invoke package";
        goto faild;
    }

    MutexCond* invoke_signal = NULL;
    if (done == NULL) {
        invoke_signal = get_invoke_signal(service_id, method_id, procedure_id);
        if (invoke_signal == NULL) {
            pthread_mutex_unlock(provider_lock);
            errmsg = "faild to get invoke signal";
            goto faild;
        }
//        debug("signal lock accquiring");
        pthread_mutex_lock(&invoke_signal->first);

        if (!invoke(method, errmsg)) {
            pthread_mutex_unlock(provider_lock);
            pthread_mutex_unlock(&invoke_signal->first);
            goto faild;
        }
    } else {
        if (!invoke(method, errmsg)) {
            pthread_mutex_unlock(provider_lock);
            goto faild;
        }
    }

    if (done == NULL) {
        struct timespec timeout;
        timeout.tv_sec = time(0) + RpcClient::INVOKE_TIMEOUT / 1000;
        timeout.tv_nsec = RpcClient::INVOKE_TIMEOUT * 1000000;
        // blocking untill invoking finish
//        debug("signal lock accquired, waiting!!!");
        int sig_ret = pthread_cond_timedwait(&invoke_signal->second, &invoke_signal->first,
                &timeout);
        if (sig_ret == ETIMEDOUT) {
            errmsg = "faild to invoke, timeout: " + RpcClient::INVOKE_TIMEOUT;
            error("faild to invoke, timeout: %d", RpcClient::INVOKE_TIMEOUT);
            pthread_mutex_unlock(provider_lock);
            pthread_mutex_unlock(&invoke_signal->first);
            goto faild;
        }
//        debug("signal waked!!!");
        pthread_mutex_unlock(&invoke_signal->first);
//        debug("signal lock unlocked!!!");
    }

    pthread_mutex_unlock(provider_lock);
//    debug("lock unlocked!!!, pid: %ld", procedure_id);
    if (controller != NULL) {
        if (package->status == FAILD) {
            controller->SetFailed(package->errmsg);
        } else {
            controller->Reset();
        }
    }

    if ((done && controller && !controller->Failed()) || (done && controller == NULL)) {
        done->Run();
    }
    return;

    faild:
    if (controller != NULL) {
        controller->SetFailed(errmsg);
    }
    return;
}

bool RpcClient::invoke(const google::protobuf::MethodDescriptor* method, std::string& errmsg) {
    int service_id = method->service()->index();
    int method_id = method->index();
    procedure_id_t procedure_id = build_procedure_id(service_id, method_id);
    struct event* ev = RpcClient::_evs.get(procedure_id);
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = RpcClient::CONNECT_TIMEOUT * 1000;

    if (ev == NULL) {
        ev = event_info_create(create_tcpsocket());
        if (ev == NULL) {
            errmsg = "faild to alloc event info";
            error("faild to alloc event info, service id:%d, method id:%d", service_id, method_id);
            return false;
        }

        RpcClient::_evs.add(procedure_id, *ev);
        ev->flags = EVENT_REUSE | EVENT_WRITE;
        ev->fail_func = failed_callback;
        ev->fail_param = (void*) method;
        ev->send_func = invoke_callback;
        ev->send_param = (void*) method;

        if (!connect(ev, &timeout)) {
            errmsg = "invoke connect error";
            error("invoke connect error, service id:%d, method id:%d", service_id, method_id);
            return false;
        }
    } else {
        ev->flags = EVENT_REUSE | EVENT_WRITE;
        ev->send_func = invoke_callback;
        ev->send_param = (void*) method;
        if (ev->status == EVENT_STATUS_ERR) {
            warn("arragner lose connect, try to reconnect");
            ev->fd = create_tcpsocket();
            ev->status = EVENT_STATUS_OK;
            if (!connect(ev, &timeout)) {
                errmsg = "invoke reconnect error";
                return false;
            }
        } else {
            ev->status = EVENT_STATUS_OK;
            event_mod(_netinf, ev);
        }
    }
    return true;
}

InvokePackage* RpcClient::get_invoke_package(const int sid, const int mid,
        const procedure_id_t pid) {
    InvokePackage* invoke_queue = RpcClient::_invoke_packages.get(pid);
    if (invoke_queue == NULL) {
        invoke_queue = new(std::nothrow) InvokePackage;
        check_null_oom(invoke_queue, return NULL,
                "invoke queue, service id:%d, method id:%d", sid, mid);
        RpcClient::_invoke_packages.add(pid, *invoke_queue);
    }
    return invoke_queue;
}

pthread_mutex_t* RpcClient::get_provider_lock(const int sid, const int mid,
        const procedure_id_t pid) {

    pthread_mutex_t* lock = RpcClient::_provider_locks.get(pid);
    if (lock == NULL) {
        // double check, lock if provider lock not available yet in cause of double new
        pthread_mutex_lock(&RpcClient::_lock);
        if ((lock = RpcClient::_provider_locks.get(pid)) == NULL) {
            lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
            check_null_oom(lock, pthread_mutex_unlock(&RpcClient::_lock), return NULL,
                    "provider lock, service id : %d, method id : %d", sid, mid);
            pthread_mutex_init(lock, NULL);
            RpcClient::_provider_locks.add(pid, *lock);
        }
        pthread_mutex_unlock(&RpcClient::_lock);
    }
    return lock;
}

MutexCond* RpcClient::get_invoke_signal(const int sid, const int mid,
        const procedure_id_t pid) {
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
        const google::protobuf::Message* request, const bool async) {
    RequestMeta meta;
    meta.set_service_id(sid);
    meta.set_method_id(mid);

    int meta_len = meta.ByteSize();
    int request_len = request->ByteSize();
    package->package_len = 8 + meta_len + request_len;
    if (package->package_len >= sizeof(package->recv_buffer)) {
        // TODO send for some batch
        error("too large package, len: %d, max len: %zu", request_len,
                sizeof(package->recv_buffer));
        return false;
    }

//    debug("meta len: %d, request_len: %d, ModelRequest len: %d", meta_len, request_len,
//            ((ModelRequest*) request)->ByteSize());
    memcpy(package->recv_buffer, &package->package_len, 4);
    memcpy(package->recv_buffer + 4, &meta_len, 4);
    bool ret = meta.SerializeToArray(package->recv_buffer + 8, meta_len);
    if (!ret) {
        error("serialize meta faild");
        return false;
    }
    ret = request->SerializeToArray(package->recv_buffer + 8 + meta_len, request_len);
    if (!ret) {
        error("serialize request faild");
        return false;
    }
    package->errmsg.clear();
    package->status = DOING;
    package->async = async;

    return true;
}

bool RpcClient::connect(struct event* ev, struct timeval* timeout) {
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(_port);
    addr.sin_addr.s_addr = inet_addr(_addr.c_str());
    return event_connect(_netinf, ev, &addr, timeout) == 0;
}

void* RpcClient::poll_routine(void* arg) {
    struct fdis::netinf* netinf = (struct fdis::netinf*) arg;
    event_loop(netinf);
    return (void*) 0;
}

static inline int wakeup_invoke(procedure_id_t provider_id,
        Map<procedure_id_t, MutexCond>* signals) {
    MutexCond* pair = signals->get(provider_id);
    if (pair == NULL) {
        return -1;
    }
//    debug("signal lock accquiring");
    pthread_mutex_lock(&pair->first);
//    debug("signal lock accquired, signal!!!");
    pthread_cond_signal(&pair->second);
//    debug("signal signaled!!!");
    pthread_mutex_unlock(&pair->first);
//    debug("signal lock unlocked!!!");
    return 0;
}

int RpcClient::failed_callback(struct event* ev) {
    debug("faild fired");

    google::protobuf::MethodDescriptor* method =
            (google::protobuf::MethodDescriptor*) ev->fail_param;

    int service_id = method->service()->index();
    int method_id = method->index();
    procedure_id_t procedure_id = build_procedure_id(service_id, method_id);

    InvokePackage* package = RpcClient::_invoke_packages.get(procedure_id);
    if (package == NULL) {
        warn("empty package, maybe error occured already, service id:%d, method id:%d",
                service_id, method_id);
        return 0;
    }
    package->errmsg = "network error";
    package->status = FAILD;
    if (package->async) {
        int ret = wakeup_invoke(procedure_id, &RpcClient::_invoke_signals);
        check_cond(ret == 0, return 0,
                "provider signal invalid, service id:%d, method id:%d, provider id : %d",
                service_id, method_id, procedure_id);
    }
    return 0;
}

int RpcClient::invoke_callback(struct event* ev) {
    debug("invoke fired");

    ev->flags = EVENT_REUSE;
    google::protobuf::MethodDescriptor* method =
            (google::protobuf::MethodDescriptor*) ev->send_param;
    int service_id = method->service()->index();
    int method_id = method->index();
    procedure_id_t procedure_id = build_procedure_id(service_id, method_id);
    InvokePackage* package = RpcClient::_invoke_packages.get(procedure_id);
    if (package == NULL) {
        warn("empty package, maybe error occured already, service id:%d, method id:%d",
                service_id, method_id);
        return 0;
    }

    ssize_t write_len = write_tcp(ev->fd, package->recv_buffer, (size_t) package->package_len);
    if (write_len == -1) {
        error("write_tcp faild, fd: %d", ev->fd);
        ev->status = EVENT_STATUS_ERR;
        package->errmsg = "write_tcp faild";
        package->status = FAILD;
        if (package->async) {
            wakeup_invoke(procedure_id, &RpcClient::_invoke_signals);
        }
        return -1;
    }

    ev->flags = EVENT_REUSE | EVENT_READ;
    ev->send_param = NULL;
    ev->send_func = NULL;
    ev->recv_func = return_callback;
    ev->recv_param = method;
    return 0;
}

int RpcClient::return_callback(struct event* ev) {
    debug("return fired");

    ev->flags = EVENT_REUSE;

    google::protobuf::MethodDescriptor* method =
            (google::protobuf::MethodDescriptor*) ev->recv_param;
    int service_id = method->service()->index();
    int method_id = method->index();
    procedure_id_t procedure_id = build_procedure_id(service_id, method_id);
    InvokePackage* package = RpcClient::_invoke_packages.get(procedure_id);
    if (package == NULL) {
        warn("empty package, maybe error occured already, service id:%d, method id:%d",
                service_id, method_id);
        goto end;
    }

    ssize_t readlen = read_tcp(ev->fd, package->recv_buffer, sizeof(package->recv_buffer));
    if (readlen == -1) {
        error("read tcp faild, fd: %d, errno:%d, error:%s", ev->fd, errno, strerror(errno));
        ev->status = EVENT_STATUS_ERR;
        package->errmsg = "read tcp error";
        package->status = FAILD;
        if (package->async) {
            wakeup_invoke(procedure_id, &RpcClient::_invoke_signals);
        }
        return -1;
    }
    if (readlen < 8) {
        error("return package too low, len:%z", readlen);
        ev->status = EVENT_STATUS_ERR;
        package->errmsg = "return package too low";
        package->status = FAILD;
        if (package->async) {
            wakeup_invoke(procedure_id, &RpcClient::_invoke_signals);
        }
        return -1;
    }

    int package_len = 0;
    memcpy(&package_len, package->recv_buffer, 4);
    if (package_len != readlen) {
        error("package len incorrect, package len:%d, read len: %z", package_len, readlen);
        ev->status = EVENT_STATUS_ERR;
        package->errmsg = "package len incorrect";
        package->status = FAILD;
        goto end;
    }

    int status_len = 0;
    memcpy(&status_len, package->recv_buffer + 4, 4);

    ResponseStatus status;
    status.ParseFromArray(package->recv_buffer + 8, status_len);
    if (status.status() != 0) {
        error("return error, status: %d, message: %s", status.status(), status.message().c_str());
        package->errmsg = "return error";
        package->status = FAILD;
        goto end;
    }

    // if status is 0, then request content must be existed
    if (package_len - 8 - status_len <= 0) {
        error("request not occured, error package");
        package->errmsg = "request not occured";
        package->status = FAILD;
        goto end;
    }

    bool ret = package->response->ParseFromArray(package->recv_buffer + 8 + status_len,
            package_len - 8 - status_len);
    if (!ret) {
        error("unserialize faild, error package");
        package->errmsg = "unserialized faild";
        package->status = FAILD;
        goto end;
    }
    package->status = SUCCESS;

    end:
    ev->recv_param = NULL;
    ev->recv_func = NULL;
    if (package != NULL && package->async) {
        wakeup_invoke(procedure_id, &RpcClient::_invoke_signals);
    }
    return 0;
}

}

#ifdef DEBUG_RPCCLIENT
using fdis::RpcClient;
using namespace fdis;

int i;

void* call_routine(void* arg) {
    ModelService_Stub* model_service = (ModelService_Stub*) arg;

    InvokeController controller;
    ModelResponse response;
    ModelRequest request;
    request.set_key("to_string");
    request.set_seq((google::protobuf::int32) pthread_self());

    if (i % 2) {
        model_service->to_string(&controller, &request, &response, NULL);
    } else {
        model_service->hello(&controller, &request, &response, NULL);
    }

    if (controller.Failed()) {
        error("invoke faild, err msg: %s", controller.ErrorText().c_str());
    } else {
        info("invoking finished, response, msg: %s", response.msg().c_str());
    }
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
