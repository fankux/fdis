//
// Created by fankux on 16-7-30.
//

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "common/check.h"
#include "net.h"
#include "event.h"

#include "chunkServer.h"

namespace fdis {

size_t ChunkServer::_recv_buf_size;
char* ChunkServer::_recv_buf;
InvokeParam ChunkServer::_invoke_param;
pthread_t ChunkServer::_routine_pid;

ChunkServer::ChunkServer() {
    ChunkServer::_ev = NULL;
    ChunkServer::_netinf = NULL;
    ChunkServer::_recv_buf = NULL;
}

ChunkServer::~ChunkServer() {
    ffree(ChunkServer::_ev);
    ChunkServer::_ev = NULL;
    ffree(ChunkServer::_netinf);
    ChunkServer::_netinf = NULL;
    ffree(ChunkServer::_recv_buf);
    ChunkServer::_recv_buf = NULL;
}

void ChunkServer::init(const ServerConf& conf) {
    _port = conf.listen_port;
    _recv_buf_size = conf.recv_buf_size;
    _recv_buf = (char*) malloc(conf.recv_buf_size * sizeof(char));
    check_null_oom(_recv_buf, exit(EXIT_FAILURE), "chunk server recv buf init");
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

void ChunkServer::run(bool background) {
    if (background) {
        pthread_create(&_routine_pid, NULL, poll_routine, _netinf);
    } else {
        poll_routine(_netinf);
    }
}

int ChunkServer::stop(uint32_t mills) {
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

int ChunkServer::request_handler(struct event* ev) {
    debug("chunk request_handler fired");

    struct PackHead head;
    // 1. get head of file content
    ssize_t readlen = read_tcp(ev->fd, _recv_buf, sizeof(head.meta));
    if (readlen == -1) {
        warn("read_tcp faild, fd: %d, errno:%d, error:%s", ev->fd, errno, strerror(errno));
        ev->status = EVENT_STATUS_ERR;
        return -1;
    }
    if (readlen != sizeof(head.meta)) {
        warn("chunk meta length not match, drop it");
        ev->status = EVENT_STATUS_ERR;
        return -1;
    }
    memcpy(&head.meta, _recv_buf, sizeof(head.meta));
    if (head.meta.file_size == 0) {
        warn("chunk file size empty, skip it");
        return 0;
    }
    if (head.meta.name_size == 0) {
        warn("chunk meta name size empty, drop it");
        ev->status = EVENT_STATUS_ERR;
        return -1;
    }
    if (head.meta.name_size >= _recv_buf_size) {
        warn("chunk meta name size to large, drop it");
        ev->status = EVENT_STATUS_ERR;
        return -1;
    }

    readlen = read_tcp(ev->fd, _recv_buf, head.meta.name_size + 1);
    if (readlen == -1) {
        warn("read_tcp faild, fd: %d, errno:%d, error:%s", ev->fd, errno, strerror(errno));
        ev->status = EVENT_STATUS_ERR;
        return -1;
    }
    if (readlen != head.meta.name_size + 1) {
        warn("chunk name length not match, drop it");
        ev->status = EVENT_STATUS_ERR;
        return -1;
    }
    head.name_buf = _recv_buf; // just point to recv buffer
    head.name_buf[head.meta.name_size] = '\0';

    // TODOparse fs path, ensure file
    FILE* fp = NULL;
    fp = fopen(head.name_buf, "a+");
    if (fp == NULL) {
        fatal("open chunk file, errno : %d, error: %s", errno, strerror(errno));
        ev->status = EVENT_STATUS_ERR;
        return -1;
    }

    // start to loop read for data chunk, recv_buf now is free for use
    size_t buf_idx = 0;
    ChunkBody chunk;
    do {
        readlen = read_tcp_retry(ev->fd, _recv_buf, sizeof(chunk.meta), 3, 100000);
        if (readlen == NET_FAIL || readlen == NET_YET) {
            warn("read_tcp faild, fd: %d, errno:%d, error:%s", ev->fd, errno, strerror(errno));
            ev->status = EVENT_STATUS_ERR;
            return -1;
        }
        if (readlen != sizeof(chunk.meta)) {
            warn("chunk head length not match, drop it");
            ev->status = EVENT_STATUS_ERR;
            return -1;
        }
        memcpy(&chunk.meta, _recv_buf, sizeof(chunk.meta));
        if (chunk.meta.size >= _recv_buf_size) {
            warn("chunk body size to large, drop it");
            ev->status = EVENT_STATUS_ERR;
            return -1;
        }

        readlen = read_tcp_retry(ev->fd, _recv_buf, chunk.meta.size, 3, 100000);
        if (readlen == NET_FAIL || readlen == NET_YET) {
            warn("read_tcp faild, fd: %d, errno:%d, error:%s", ev->fd, errno, strerror(errno));
            ev->status = EVENT_STATUS_ERR;
            return -1;
        }
        if (readlen != chunk.meta.size) {
            warn("chunk length not match, drop it");
            ev->status = EVENT_STATUS_ERR;
            return -1;
        }

        chunk.content = _recv_buf;
        if (fwrite(chunk.content, chunk.meta.size, 1, fp) == -1) {
            fatal("write chunk file failed, errno : %d, error: %s", errno, strerror(errno));
            fclose(fp);
            ev->status = EVENT_STATUS_ERR;
            return -1;
        }
    } while (!chunk.meta.is_last);

    // TODO CRC

    InvokeParam* param = &ChunkServer::_invoke_param;
    param->_status = 0;
    ev->flags = EVENT_WRITE;
    ev->send_func = response_handler;

    return 0;
}

int ChunkServer::response_handler(struct event* ev) {
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

void* ChunkServer::poll_routine(void* arg) {
    struct netinf* netinf = (struct netinf*) arg;
    event_loop(netinf);
    return (void*) 0;
}

}
