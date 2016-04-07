#ifndef FANKUX_WOKER_H
#define FANKUX_WOKER_H

#include <stdint.h>
#include "rpc/RpcServer.h"
#include "rpc/RpcClient.h"

namespace fankux {

class Worker {
public:
    Worker(const std::string& addr, uint16_t port) : _id(0), //_server(RpcServerFactory::instance()),
                                                     _client(addr, port) {}

    void init();

    void run(bool background);

private:
    static pthread_t _heartbeat_thread;

    static void* hreatbeat_rountine(void* arg);

private:
    uint32_t _id;
    RpcServer _server;
    RpcClient _client;
};
}

#endif // FANKUX_WOKER_H
