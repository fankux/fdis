#ifndef FANKUX_ARRANGER_H
#define FANKUX_ARRANGER_H

#include <stddef.h>
#include "common/fqueue.hpp"
#include "rpc/RpcClient.h"
#include "ArrangerServer.h"

namespace fankux {

struct Token;

enum node_status {
    NODE_OFFLINE = 0,
    NODE_LOADING,
    NODE_OK
};

struct Node {
    int32_t id;
    Queue<struct Token> tokens;

    // server related
    int status;

    struct timeval last_report;
    struct timeval delay;

    RpcClient* rpcClient;
    unsigned int ip;
    unsigned short port;
};

struct Token {
    int id;
    void* key;
    struct Node* node;
};

class Arranger {
public:
    Arranger();

    void init();

    void run();

    void stop();

    int handle_node(struct Node* node);

    struct Node* add_node(struct Node* inode);

private:
    void sort_node();

    void load_balance();

private:
    ArrangerServer _server;

    pthread_mutex_t _lock;
    bool _run_status;

    // all token list
    Queue<struct Token> _tokens;
    size_t _token_num; // not changed at runtime

    // nodes array
    struct Node* _nodes;
    size_t _node_num;
    size_t _node_space;

    static const int DEFAULT_TOKEN_NUM = 32;
    static const int DEFAULT_WORKER_NUM = 3;
    static const int HANDLE_RETRY = 3;
    static const uint64_t LEASE_TIME = 4; /* mills */
};

extern Arranger* g_ager;

}

#endif /* FANKUX_ARRANGER_H */
