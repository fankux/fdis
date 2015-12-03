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

#ifdef DEBUG_WORKER
using fankux::Worker;

int main(void) {
    Worker woker;
    woker.init();
    woker.run();

    return 0;
}

#endif
