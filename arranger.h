#ifndef FANKUX_ARRANGER_H
#define FANKUX_ARRANGER_H

#include <stddef.h>
#include "common/fqueue.hpp"
#include "rpc/RpcClient.h"
#include "ArrangerServer.h"
#include "config.h"
#include "threadpool.hpp"

namespace fankux {

struct Token;

enum node_status {
    NODE_OK = 0,
    NODE_LOADING,
    NODE_OFFLINE,
    NODE_EXPIRE
};

struct Node {
    int32_t id;
    Queue<struct Token> tokens;
    Threadpool* taskpool;

    // server related
    int status;

    struct timeval last_report;
    struct timeval delay;

    RpcClient* rpcClient;
    struct sockaddr_in sockaddr;
};

struct Token {
    int id;
    void* key;
    struct Node* node;
};

class Arranger;

class ArrangerConf {
    friend class Arranger;

public:
    static ArrangerConf* load(const std::string& path);

    static void release(ArrangerConf* conf);

private:
    ArrangerConf() {};

private:
    fconf_t* _conf;

    uint32_t _default_token_num;
    uint32_t _default_worker_num;
    uint32_t _handle_retry;
    uint32_t _lease_time;
    uint32_t _threadpool_min;
    uint32_t _threadpool_max;
};

class Arranger {
public:
    Arranger();

    void init(ArrangerConf& conf);

    void run();

    void stop();

    int handle_node(struct Node* node);

    struct Node* add_node(struct Node* inode);

private:
    void sort_node();

    void load_balance();

private:
    ArrangerConf _conf;
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
};

extern Arranger g_ager;

}

#endif /* FANKUX_ARRANGER_H */
