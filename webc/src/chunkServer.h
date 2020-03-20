//
// Created by fankux on 16-7-30.
// using for file transfer which not through protobuf
//

#ifndef FIDS_CHUNKSERVER_H
#define FIDS_CHUNKSERVER_H


#include "common/common.h"
#include "rpc/RpcServer.h"

namespace fdis {

typedef enum {
    meta = 0,
    body,
};

struct PackMeta {
    uint32_t file_size;
    uint16_t name_size;
};

struct PackHead {
    struct PackMeta meta;
    char* name_buf;
};

struct ChunkMeta {
    uint8_t is_last:1;
    uint32_t size;
};

struct ChunkBody {
    struct ChunkMeta meta;
    char* content;
};

/**
 file pack structure
 ----------------
 32bit  file_size
 ----------------
 16bit  name_size
 ----------------
 N bit  name_buf
 ...
 ----------------
 chunk1
 ----------------
 chunk2
 ----------------
 chunk...
 ----------------
 chunkN last one
 ----------------
 crc
 ================

 chunk structure
 ----------------
 1bit   is_last
 ----------------
 32bit  body_size
 ----------------
 N bit  chunk_buf
 ...
 ...
 ================
*/

class ChunkServer {
AVOID_COPY(ChunkServer)

public:
    ChunkServer();

    virtual ~ChunkServer();

    void init(const ServerConf& conf);

    void run(bool background);

    int stop(uint32_t mills);

private:
    static int request_handler(struct event* ev);

    static int response_handler(struct event* ev);

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

}


#endif // FIDS_CHUNKSERVER_H
