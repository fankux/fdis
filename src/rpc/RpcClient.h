#ifndef FDIS_RPCCLIENT_H
#define FDIS_RPCCLIENT_H

#include "google/protobuf/service.h"
#include "rpc.h"

namespace fdis {

typedef std::pair<pthread_mutex_t, pthread_cond_t> MutexCond;

static const size_t INVOKE_PACKAGE_LEN = 1024;

enum InvokeStatus {
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
    google::protobuf::Message* response;
};

class RpcClient : public google::protobuf::RpcChannel {
public:
    RpcClient() : _evs(FMAP_T_INT64, FMAP_DUP_KEY) {}

    virtual ~RpcClient();

    void init(const std::string& addr, uint16_t port);

    void run(bool background = false);

    void CallMethod(const google::protobuf::MethodDescriptor* method,
            google::protobuf::RpcController* controller,
            const google::protobuf::Message* request,
            google::protobuf::Message* response, google::protobuf::Closure* done);

private:
    pthread_mutex_t* get_provider_lock(const int sid, const int mid, const procedure_id_t pid);

    MutexCond* get_invoke_signal(const int sid, const int mid, const procedure_id_t pid);

    InvokePackage* get_invoke_package(const int sid, const int mid, const procedure_id_t pid);

    bool populate_invoke_package(InvokePackage* package, const int sid, const int mid,
            const google::protobuf::Message* request, const bool async = false);

    bool invoke(const google::protobuf::MethodDescriptor* method, std::string& errmsg);

    bool connect(struct event* _ev, struct timeval* timeout);

    static void* poll_routine(void* arg);

    static int invoke_callback(struct event* ev);

    static int return_callback(struct event* ev);

    static int failed_callback(struct event* ev);

private:
    uint16_t _port;
    std::string _addr;
    pthread_t _routine_pid;
    struct netinf* _netinf;
    struct Map<procedure_id_t, event> _evs;

    static const int CONNECT_TIMEOUT = 3000; // connect timeout in mills
    static const int INVOKE_TIMEOUT = 5000; // timeout in mills
    static Map<procedure_id_t, pthread_mutex_t> _provider_locks;
    static Map<procedure_id_t, InvokePackage> _invoke_packages;
    static Map<procedure_id_t, MutexCond> _invoke_signals;
    static pthread_mutex_t _lock;
};

}

#endif // FDIS_RPCCLIENT_H
