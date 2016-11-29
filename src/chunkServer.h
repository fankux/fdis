//
// Created by fankux on 16-7-30.
// using for file transfer which not through protobuf
//

#ifndef WEBC_TRUNKSERVER_H
#define WEBC_TRUNKSERVER_H

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "common/check.h"
#include "net.h"
#include "event.h"
#include "rpc/RpcServer.h"

namespace fdis {
class ChunkServer {

public:
    void run(bool background) {
        if (background) {
            pthread_create(&_routine_pid, NULL, poll_routine, _netinf);
        } else {
            poll_routine(_netinf);
        }
    }

    void init(const uint16_t port) {
        _port = port;

        _netinf = event_create(EVENT_LIST_MAX);
        check_null_oom(_netinf, exit(EXIT_FAILURE), "epoll api info init");
        _netinf->listenfd = create_tcpsocket_listen(_port);
        _ev = event_info_create(_netinf->listenfd);
        check_null_oom(_ev, exit(EXIT_FAILURE), "list event init");
        _ev->flags = EVENT_ACCEPT | EVENT_READ;
        _ev->recv_func = request_handler;
        event_add(_netinf, _ev);
    }

private:
    static int request_handler(struct event* ev) {
        debug("chunk request_handler fired");

        char chunk[8192];
        ssize_t readlen = read_tcp(ev->fd, chunk, sizeof(chunk));
        if (readlen == -1) {
            warn("read_tcp faild, fd: %d, errno:%d, error:%s", ev->fd, errno, strerror(errno));
            ev->status = EVENT_STATUS_ERR;
            return -1;
        }

        int package_len = 0;
        memcpy(&package_len, chunk, 4);
//        if (readlen <= 4 || package_len != readlen) {
//            warn("tcp package length incorrect, drop it, read len: %lu, package len: %d",
//                    readlen, package_len);
//            ev->status = EVENT_STATUS_ERR;
//            return -1;
//        }

        FILE* fp = NULL;
        fp = fopen("./chunk", "a+");
        check_null(fp, return -1, "open chunk file, errno : %d, error: %s", errno, strerror(errno));

        if (fwrite(chunk + 4, (size_t) readlen - 4, 1, fp) == -1) {
            fatal("write chunk file failed, errno : %d, error: %s", errno, strerror(errno));
            fclose(fp);
            ev->status = EVENT_STATUS_ERR;
            return -1;
        }

        ssize_t idx = 4;
        do {
            readlen = read_tcp(ev->fd, chunk, sizeof(chunk));
            if (readlen == -1) {
                warn("read_tcp faild, fd: %d, errno:%d, error:%s", ev->fd, errno, strerror(errno));
                fclose(fp);
                ev->status = EVENT_STATUS_ERR;
                return -1;
            }

            if (fwrite(chunk, (size_t) readlen, 1, fp) <= 0) {
                if (ferror(fp)) {
                    fatal("write chunk file failed, errno : %d, error: %s", errno, strerror(errno));
                    fclose(fp);
                    ev->status = EVENT_STATUS_ERR;
                    return -1;
                }
            }
            idx += readlen;
        } while (idx < package_len);
        fclose(fp);

        InvokeParam* param = &ChunkServer::_invoke_param;
        param->_status = 0;
        ev->flags = EVENT_WRITE;
        ev->send_func = response_handler;

        return 0;
    }

    static int response_handler(struct event* ev) {
        debug("chunk response_handler fired");

        char ssend[] = "OK";
        ssize_t write_len = write_tcp(ev->fd, ssend, sizeof(ssend));
        if (write_len == -1) {
            warn("write_tcp faild, fd: %d, errno:%d, error:%s", ev->fd, errno, strerror(errno));
            ev->status = EVENT_STATUS_ERR;
            return -1;
        }

        ev->flags = EVENT_READ;
        ev->send_func = NULL;
        return 0;
    }

    static void* poll_routine(void* arg);

private:
    uint16_t _port;

    struct event* _ev;
    struct netinf* _netinf;
    static InvokeParam _invoke_param;
    static pthread_t _routine_pid;
};

InvokeParam ChunkServer::_invoke_param;
pthread_t ChunkServer::_routine_pid;

void* ChunkServer::poll_routine(void* arg) {
    struct netinf* netinf = (struct netinf*) arg;
    event_loop(netinf);
    return (void*) 0;
}

}


#endif //WEBC_TRUNKSERVER_H
