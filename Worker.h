#ifndef FANKUX_WOKER_H
#define FANKUX_WOKER_H

#include <stdint.h>

#include "rpc.h"

namespace fankux {

class Worker {
public:
    Worker() : _id(0), _server(RpcServerFactory::instance()) {}

    void init();

    void run();

private:
    uint32_t _id;
    RpcServer& _server;
};
}

#endif // FANKUX_WOKER_H
