#ifndef FDIS_RPCSERVER_H
#define FDIS_RPCSERVER_H

#include "common/flist.hpp"
#include "rpc.h"

namespace fdis {

struct NetInfo {
    struct sockaddr_in sockaddr;
};

struct Provider {
public:
    Provider() : succ_dones(FMAP_T_INT32, FMAP_DUP_KEY),
                 fail_dones(FMAP_T_INT32, FMAP_DUP_NONE) {}

    NetInfo net_info;
    Service* service;
    Map<procedure_id_t, Closure> succ_dones;
    Map<procedure_id_t, Closure> fail_dones;
};

struct InternalData {
    Message* request;
    Message* response;
    Provider* provider;
};

class InvokeParam;

class RpcServer {
public:
    RpcServer() : _port(0), _ev(NULL), _netinf(NULL),
                  _providers(FMAP_T_INT32, FMAP_DUP_KEY) {}

    virtual ~RpcServer();

    void init(const uint16_t port);

    void run(bool background = false);

    int stop(uint32_t mills);

    void reg_provider(const int id, Provider& provider);

    Provider* get_provider(const int id);

private:
    static int request_handler(struct event* ev);

    static int response_handler(struct event* ev);

    static void* poll_routine(void* arg);

    static void default_succ_func() {}

    static void default_fail_func() {}

public:
    static Closure* default_succ;

    static Closure* default_fail;

private:
    uint16_t _port;
    struct event* _ev;
    struct netinf* _netinf;
    Map<int, Provider> _providers;

    static pthread_t _routine_pid;

    static InvokeParam _invoke_param;
};

class InvokeParam {
public:
    int _status;
    RpcServer* _server;
    std::string _msg;

    Map<procedure_id_t, InternalData> _internal_data;
    Method* _method;

    InvokeParam() : _server(NULL), _status(0), _msg(), _method(NULL),
                    _internal_data(FMAP_T_INT64, FMAP_DUP_KEY) {}
};

class RpcServerFactory {
public:
    static RpcServer& instance() {
        static RpcServer rpcServer;
        return rpcServer;
    }
};
}

#endif // FDIS_RPCSERVER_H
