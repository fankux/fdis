#ifndef CHUNK_RPCPROVIDER_H
#define CHUNK_RPCPROVIDER_H

#include <stdint.h>
#include "google/protobuf/service.h"

#include "common/fmap.hpp"

namespace fankux {

class RpcServer {
public:
    RpcServer();

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

    struct Map<int, google::protobuf::Service> _services;

    static class InvokeParam {
    public:
        RpcServer* _server;
        int _status;
        std::string _msg;
        struct google::protobuf::Message* _request;
        struct google::protobuf::Message* _response;

        google::protobuf::MethodDescriptor* _method;

        InvokeParam() : _server(NULL), _status(0), _msg(), _request(NULL), _response(NULL),
                _method(NULL) {}
    } _invoke_param;
};


class RpcServerFactory {
public:
    static RpcServer& instance() {
        static RpcServer rpcServer;
        return rpcServer;
    }
};

}

#endif // CHUNK_RPCPROVIDER_H
