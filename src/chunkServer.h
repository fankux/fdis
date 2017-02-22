//
// Created by fankux on 16-7-30.
// using for file transfer which not through protobuf
//

#ifndef FIDS_CHUNKSERVER_H
#define FIDS_CHUNKSERVER_H

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "common/check.h"
#include "common/fmem.h"
#include "net.h"
#include "event.h"
#include "rpc/RpcServer.h"

namespace fdis {

typedef enum {
    meta = 0,
    body,
};

struct ChunkMeta {
    uint16_t name_size;
    char* name;
    char zero;
};

struct ChunkPack {
    uint16_t is_last:1;
    uint16_t seq:15;
    uint16_t reserved;
    uint32_t body_size;
    char* body;
    char zero
};


class ChunkServer {
AVOID_COPY(ChunkServer)

public:
    ChunkServer() {
        ChunkServer::_ev = NULL;
        ChunkServer::_netinf = NULL;
        ChunkServer::_recv_buf = NULL;
    }

    virtual ~ChunkServer() {
        ffree(ChunkServer::_ev);
        ChunkServer::_ev = NULL;
        ffree(ChunkServer::_netinf);
        ChunkServer::_netinf = NULL;
        ffree(ChunkServer::_recv_buf);
        ChunkServer::_recv_buf = NULL;
    }

    void init(const ServerConf& conf) {
        _port = conf.listen_port;
        ChunkServer::_recv_buf_size = conf.recv_buf_size;
        ChunkServer::_recv_buf = (char*) malloc(conf.recv_buf_size * sizeof(char));
        check_null_oom(_netinf, exit(EXIT_FAILURE), "chunk server recv buf init");
        _recv_buf[0] = '\0';

        _netinf = event_create(EVENT_LIST_MAX);
        check_null_oom(_netinf, exit(EXIT_FAILURE), "epoll api info init");
        _netinf->listenfd = create_tcpsocket_listen(_port);
        _ev = event_info_create(_netinf->listenfd);
        check_null_oom(_ev, exit(EXIT_FAILURE), "list event init");
        _ev->flags = EVENT_ACCEPT | EVENT_READ;
        _ev->recv_func = request_handler;
        event_add(_netinf, _ev);
    }

    void run(bool background) {
        if (background) {
            pthread_create(&_routine_pid, NULL, poll_routine, _netinf);
        } else {
            poll_routine(_netinf);
        }
    }

    int stop(uint32_t mills) {
        if (ChunkServer::_routine_pid != 0) {
            if (mills == 0) {
                pthread_join(ChunkServer::_routine_pid, NULL);
                return 0;
            } else {
                timespec tm_spec;
                tm_spec.tv_sec = mills / 1000;
                tm_spec.tv_nsec = (mills % 1000) * 1000;
                return pthread_timedjoin_np(ChunkServer::_routine_pid, NULL, &tm_spec);
            }
        }
        return 0;
    }

private:
    static int request_handler(struct event* ev) {
        debug("chunk request_handler fired");

        // 1. get head of file content
        ssize_t readlen = read_tcp(ev->fd, _recv_buf, _recv_buf_size);
        if (readlen == -1) {
            warn("read_tcp faild, fd: %d, errno:%d, error:%s", ev->fd, errno, strerror(errno));
            ev->status = EVENT_STATUS_ERR;
            return -1;
        }

        ChunkMeta meta;
        if (readlen <= sizeof(meta.name_size)) {
            warn("chunk meta too short, drop it");
            ev->status = EVENT_STATUS_ERR;
            return -1;
        }

        size_t buf_idx = 0;
        memcpy(&meta.name_size, _recv_buf, sizeof(meta.name_size));
        if (meta.name_size == 0) {
            warn("chunk meta name empty, drop it");
            ev->status = EVENT_STATUS_ERR;
            return -1;
        }
        buf_idx += sizeof(meta.name_size);
        meta.name = _recv_buf + buf_idx; // just point to recv buffer
        meta.zero = '\0';
        buf_idx += meta.name_size + 1;

        // parse fs path, ensure file
        FILE* fp = NULL;
        fp = fopen(meta.name, "a+");
        if (fp == NULL) {
            fatal("open chunk file, errno : %d, error: %s", errno, strerror(errno));
            ev->status = EVENT_STATUS_ERR;
            return -1;
        }

        // start to loop read for data chunk
        ChunkPack pack;
        do {
            if (buf_idx < readlen) {
                memcpy(&pack.is_last, _recv_buf + buf_idx, sizeof(pack.is_last));

                if (fwrite(chunk + 4, (size_t) readlen - 4, 1, fp) == -1) {
                    fatal("write chunk file failed, errno : %d, error: %s", errno, strerror(errno));
                    fclose(fp);
                    ev->status = EVENT_STATUS_ERR;
                    return -1;
                }
            }

            buf_idx = 0;
            readlen = read_tcp(ev->fd, _recv_buf, _recv_buf_size);
            if (readlen == NET_YET) {
                usleep(100000);
                continue;
            }
        } while (readlen != -1);

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

    static size_t _recv_buf_size;
    static char* _recv_buf;
};

size_t ChunkServer::_recv_buf_size;
char* ChunkServer::_recv_buf;
InvokeParam ChunkServer::_invoke_param;
pthread_t ChunkServer::_routine_pid;

void* ChunkServer::poll_routine(void* arg) {
    struct netinf* netinf = (struct netinf*) arg;
    event_loop(netinf);
    return (void*) 0;
}

}


#endif // FIDS_CHUNKSERVER_H
