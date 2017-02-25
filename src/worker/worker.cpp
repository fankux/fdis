#include <unistd.h>
#include <sstream>
#include "proto/ArrangerService.pb.h"

#include "common/check.h"
#include "worker/worker.h"
#include "rpc/InvokeController.h"

namespace fdis {

pthread_t Worker::_heartbeat_thread;

void* Worker::hreatbeat_rountine(void* arg) {
    Worker* worker = (Worker*) arg;

    ArrangerService_Stub* ager_service = new ArrangerService::Stub(
            (google::protobuf::RpcChannel*) &worker->_client);

    InvokeController controller;
    HeartbeatRequest request;
    HeartbeatResponse response;

    int i = 0;
    while (i++ < 100) {
        struct timeval now;
        gettimeofday(&now, NULL);
        request.set_timestamp(now.tv_sec);
        request.set_timemills(now.tv_usec / 1000);
        request.set_id(1);
        request.set_port(7240);

        ager_service->echo(&controller, &request, &response, NULL);

        if (controller.Failed()) {
            error("invoke faild, err msg: %s", controller.ErrorText().c_str());
        } else {
            info("invoking finished, response, msg: %ld.%ld",
                    response.timestamp(), response.timemills());
        }
        request.Clear();
        response.Clear();

        sleep(3);
    }

    return (void*) 0;
}

void Worker::init(uint16_t server_port, const std::string& addr, uint16_t client_port) {
    info("worker init, id: %d", _id);

    _client.init(addr, client_port);
//    _server.init(server_port);

    struct ServerConf conf;
    conf.listen_port = 7380;
    conf.recv_buf_size = 8192;
    _chunk_server.init(conf);
    sleep(3);
}

void Worker::run(bool background) {
    _client.run(true);

//    int ret = pthread_create(&Worker::_heartbeat_thread, NULL, Worker::hreatbeat_rountine, this);
//    if (ret != 0) {
//        error("hreatbreat rountine create error");
//        return;
//    }
//    _server.run(background);
    _chunk_server.run(background);
}
}

#ifdef DEBUG_WORKER

using namespace fdis;

int main(void) {
    Worker worker;
    worker.init(7240, "127.0.0.1", 7234);
    worker.run(false);

    return 0;
}

#endif
