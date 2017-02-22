#include <unistd.h>
#include "common/fmem.h"
#include "common/check.h"
#include "arranger.h"

namespace fdis {

Arranger g_ager;

/**
 * TODO..nodes info persist
 */
Arranger::Arranger() {}

void Arranger::init(ArrangerConf& conf) {
    pthread_mutex_init(&_lock, NULL);
    _conf = conf;
    // TODO... null check,
    _token_num = _conf._default_token_num;
    _node_num = _conf._default_worker_num;
    _node_space = min2level(_node_num);
    _node_start_id = 1; // TODO..get from persist file

    _nodes = (Node*) fcalloc(_node_space, sizeof(struct Node));
    for (size_t i = 0; i < _node_num; ++i) {
        fmemset(_nodes + i, 0, sizeof(struct Node));
        // TODO.. init persist node
    }

    struct Token* token = (Token*) fcalloc(_token_num, sizeof(struct Token));
    // TODO.. field

    _server.init(conf._server_port);

    // TODO.. check service init status

    _run_status = true;
}

void Arranger::run() {
    while (_run_status) {
        // holding
    }
}

void Arranger::stop() {
    _run_status = false;
}

struct Node* Arranger::add_node(struct Node* inode) {
    pthread_mutex_lock(&_lock);

    if (inode->id >= _node_space) {
        info("add new node, id : %d, expand _node space", inode->id);
        _node_space = _node_space << 1;
        struct Node* tmp_addr = (Node*) realloc(_nodes, _node_space * sizeof(struct Node));
        check_null_oom(tmp_addr, return NULL, "realloc _nodes faild, new size:%z, ", _node_space);
        _nodes = tmp_addr;
    }

    struct Node* node = &_nodes[_node_num];
    node->id = _node_start_id++;
    node->sockaddr = inode->sockaddr;
    node->last_report = inode->last_report;
    node->delay = inode->delay;
    node->status = NODE_OK;
    int ret = node->taskpool->init(_conf._threadpool_min, _conf._threadpool_max);
    if (ret == -1) {
        error("add node faild, taskpool init faild");
        return NULL;
    }

    ++_node_num;
    pthread_mutex_unlock(&_lock);

    debug("add node success, id : %d, current node num : %z", node->id, _node_num);

    return node;
}

int Arranger::handle_node(struct Node* innode) {
    struct Node* node;

    if (innode->id <= 0 && innode->id >= _node_num) { // add new node
        node = add_node(innode);
        check_null_oom(node, return -1, "handle node faild to add, ");
        return 0;
    }

    timeval now;
    timeval delta;
    gettimeofday(&now, NULL);

    node = &_nodes[innode->id];
    timeval_subtract(&delta, &node->last_report, &now);
    timeval_subtract(&delta, &node->last_report, &delta);

    if (delta.tv_sec > 0 || delta.tv_usec > _conf._lease_time * 1000) {
        warn("node lease timeout, id", node->id);
        node->status = NODE_EXPIRE;
    } else {
        node->status = NODE_OK;
    }

    return 0;
}

/**
 * sort worker by numbers of token in ascending order.
 */
void Arranger::sort_node() {
    Token tmp;
    bool swap;
    for (size_t i = 0; i < _node_num - 1; ++i) {
        swap = false;
        for (size_t j = i + 1; j < _node_num - 1; ++j) {
            if (_nodes[j].tokens.size() > _nodes[j + i].tokens.size()) {
                fmemcpy(&tmp, &_nodes[j], sizeof(struct Node));
                fmemcpy(&_nodes[j], &_nodes[j + 1], sizeof(struct Node));
                fmemcpy(&_nodes[j + 1], &tmp, sizeof(struct Node));
                swap = true;
            }
        }
        if (!swap) {
            break;
        }
    }
}

void Arranger::load_balance() {
    sort_node();

    int i = 0;
    struct Token* token = NULL;
    while ((token = _tokens.pop())) {
        int delta = (int) (_nodes[_node_num - 1].tokens.size() -
                           _nodes[i % _node_num].tokens.size());
        if (delta == 0) {
            delta = 1;
        }

        for (int k = 0; k < delta; ++k) {
            struct Node* node = &_nodes[i % _node_num];
            if (node->status != NODE_OK) {
                //TODO..while there're no workers in good status....
                ++i;
                continue;
            }

            token->node = node;
            node->tokens.add(*token);
            token = _tokens.pop();
            if (token == NULL) {
                break;
            }
        }
        if (token == NULL) {
            break;
        }
        ++i;
    }
}

}

#ifdef DEBUG_ARRANGER
using namespace fdis;

int main(void) {
    ArrangerConf* ager_conf = ArrangerConf::load("/home/fankux/app/fankux/webc/conf/arranger.conf");
    g_ager.init(*ager_conf);
    g_ager.run();

    ArrangerConf::release(ager_conf);
    return 0;
}

#endif /* DEBUG_ARRANGER */
