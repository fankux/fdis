#ifndef FANKUX_RPC_H
#define FANKUX_RPC_H

#include <stdint.h>
#include "google/protobuf/service.h"

#include "common/fmap.hpp"
#include "event.h"

namespace fankux {

typedef int64_t provider_id_t;
typedef std::pair<google::protobuf::Message*, google::protobuf::Message*> InternalData;
typedef std::pair<pthread_mutex_t, pthread_cond_t> MutexCond;

#define build_provider_id(__service_id__, __method_id__) \
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

enum InvokeStatus{
    DOING = 1,
    SUCCESS,
    FAILD
};

struct InvokePackage {
    bool async;
    std::string errmsg;
    int status;
    int package_len;
    char recv_buffer[INVOKE_PACKAGE_LEN];
    char read_buffer[INVOKE_PACKAGE_LEN];
    google::protobuf::Message* response;
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
    pthread_mutex_t* get_provider_lock(const int sid, const int mid, const provider_id_t pid);

    MutexCond* get_invoke_signal(const int sid, const int mid, const provider_id_t pid);

    InvokePackage* get_invoke_package(const int sid, const int mid, const provider_id_t pid);

    bool populate_invoke_package(InvokePackage* package, const int sid, const int mid,
            const google::protobuf::Message* request, const bool async = false);

    bool invoke(const google::protobuf::MethodDescriptor* method, std::string& errmsg);

    bool connect(struct event* _ev);

    static void* poll_routine(void* arg);

    static int invoke_callback(struct event* ev);

    static int return_callback(struct event* ev);

    static int faild_callback(struct event* ev);

    void init();

private:
    uint16_t _port;
    std::string _addr;
    pthread_t _routine_pid;
    struct netinf* _netinf;
    struct Map<provider_id_t, event> _evs;

    static const int INVOKE_TIMEOUT = 5000; // timeout in mills
    static Map<provider_id_t, pthread_mutex_t> _provider_locks;
    static Map<provider_id_t, InvokePackage> _invoke_packages;
    static Map<provider_id_t, MutexCond> _invoke_signals;
    static pthread_mutex_t _lock;
};

class InvokeController : public google::protobuf::RpcController {

public:
    inline InvokeController() : _success(true), _errmsg("") {}

    void Reset();

    bool Failed() const;

    std::string ErrorText() const;

    void StartCancel();

    void SetFailed(const std::string& reason);

    bool IsCanceled() const;

    void NotifyOnCancel(google::protobuf::Closure* callback);

private:
    bool _success;
    std::string _errmsg;
};

}

#endif // FANKUX_RPC_H
