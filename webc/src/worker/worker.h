#ifndef FDIS_WOKER_H
#define FDIS_WOKER_H

#include <stdint.h>
#include <proto/WorkerService.pb.h>
#include <chunkServer.h>
#include "rpc/RpcServer.h"
#include "rpc/RpcClient.h"

namespace fdis {

class Worker : public WorkerService {
public:
    Worker() : _id(0), _client() {}

    void init(uint16_t server_port, const std::string& addr, uint16_t client_port);

    void run(bool background);

private:
    static pthread_t _heartbeat_thread;

    static void* hreatbeat_rountine(void* arg);

private:
    uint32_t _id;
    RpcServer _server;
    RpcClient _client;
    ChunkServer _chunk_server;
};
}

#endif // FDIS_WOKER_H
