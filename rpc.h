#ifndef FANKUX_RPC_H
#define FANKUX_RPC_H

#include <stdint.h>
#include "google/protobuf/service.h"

#include "common/fmap.hpp"
#include "common/fqueue.hpp"
#include "event.h"

namespace fankux {

typedef int64_t provider_id_t;
typedef std::pair<google::protobuf::Message*, google::protobuf::Message*> InternalData;
typedef std::pair<pthread_mutex_t, pthread_cond_t> MutexCond;

#define build_service_method_id(__service_id__, __method_id__) \
    (((__service_id__) << (sizeof(__service_id__) * 8)) | (__method_id__))

class RpcServer {
public:
    RpcServer() : _port(0), _ev(NULL), _netinf(NULL) {}

    virtual ~RpcServer();

    void init(const uint16_t port);

    void run();

    void reg_provider(const int id, google::protobuf::Service& service);

private:
    static int request_handler(struct event* ev);

    static int response_handler(struct event* ev);

private:
    uint16_t _port;
    struct event* _ev;
    struct netinf* _netinf;

    Map<int, google::protobuf::Service> _services;

    static class InvokeParam {
    public:
        int _status;
        RpcServer* _server;
        std::string _msg;

        Map<provider_id_t, InternalData> _internal_data;
        google::protobuf::MethodDescriptor* _method;

        InvokeParam() : _server(NULL), _status(0), _msg(), _method(NULL) {}
    } _invoke_param;
};

class RpcServerFactory {
public:
    static RpcServer& instance() {
        static RpcServer rpcServer;
        return rpcServer;
    }
};

static const size_t INVOKE_PACKAGE_LEN = 1024;

struct InvokePackage {
    int _package_len;
    char _recv_buffer[INVOKE_PACKAGE_LEN];
    char _read_buffer[INVOKE_PACKAGE_LEN];
    google::protobuf::Message* _response;
};

class RpcClient : public google::protobuf::RpcChannel {
public:
    RpcClient(const std::string& addr, uint16_t port);

    virtual ~RpcClient();

    void run(bool background = false);

    void CallMethod(const google::protobuf::MethodDescriptor* method,
            google::protobuf::RpcController* controller,
            const google::protobuf::Message* request,
            google::protobuf::Message* response, google::protobuf::Closure* done);

private:
    MutexCond* get_provider_lock(const int sid, const int mid, const provider_id_t pid);

    Queue<InvokePackage>* get_invoke_queue(const int sid, const int mid, const provider_id_t pid);

    bool populate_invoke_package(InvokePackage* package, const int sid, const int mid,
            const google::protobuf::Message* request);

    bool connect(struct event* _ev);

    static void* poll_routine(void* arg);

    static int invoke_callback(struct event* ev);

    static int return_callback(struct event* ev);

    static int faild_callback(struct event* ev);

    void init();

private:
    uint16_t _port;
    std::string _addr;
    struct netinf* _netinf;
    pthread_t _routine_pid;

    struct Map<provider_id_t, event> _evs;
    static Map<provider_id_t, Queue<InvokePackage> > _invoke_packages;
    static Map<provider_id_t, MutexCond> _provider_locks;
    static pthread_mutex_t _lock;
};

}

#endif // FANKUX_RPC_H
