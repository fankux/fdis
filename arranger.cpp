#include <unistd.h>
#include "common/fmem.h"
#include "common/check.h"
#include "arranger.h"

namespace fankux {

Arranger* g_ager = new Arranger();

Arranger::Arranger() {
    init();
}

void Arranger::init() {
    pthread_mutex_init(&_lock, NULL);
    // TODO... null check,
    // TODO... worker should be created when heartbeat registering event dynamicly
    _token_num = DEFAULT_TOKEN_NUM;
    _node_num = DEFAULT_WORKER_NUM;
    _node_space = min2level(_node_num);

    _nodes = (Node*) fcalloc(_node_space, sizeof(struct Node));
    for (size_t i = 0; i < _node_num; ++i) {
        fmemset(_nodes + i, 0, sizeof(struct Node));
        // TODO.. field
    }

    struct Token* token = (Token*) fcalloc(_token_num, sizeof(struct Token));
    // TODO.. field

    _server.init();

    // TODO.. check service init status

    _run_status = true;
}

void Arranger::run() {
    while (_run_status) {
    }
}

void Arranger::stop() {
    _run_status = false;
}

struct Node* Arranger::add_node(struct Node* inode) {
    if (inode->id >= _node_space) {
        info("add new node, id : %d, expand _node space", inode->id);
        pthread_mutex_lock(&_lock);
        _node_space = _node_space << 1;
        struct Node* tmp_addr = (Node*) realloc(_nodes, _node_space * sizeof(struct Node));
        check_null_oom(tmp_addr, return NULL, "realloc _nodes faild, new size:%z, ", _node_space);
        _nodes = tmp_addr;
        pthread_mutex_unlock(&_lock);
    }
    struct Node* node = &_nodes[inode->id];
    node->id = inode->id;
    node->sockaddr = inode->sockaddr;
    node->last_report = inode->last_report;
    node->delay = inode->delay;
    node->status = NODE_OK;

    pthread_mutex_lock(&_lock);
    ++_node_num;
    pthread_mutex_unlock(&_lock);

    return node;
}

int Arranger::handle_node(struct Node* node) {
    struct Node* tmp_node;
    if (node->id >= _node_space) {
        tmp_node = add_node(node);
        check_null_oom(tmp_node, return -1, "handle node faild to add, ");
    }
    tmp_node = &_nodes[node->id];

    timeval now;
    timeval delta;
    gettimeofday(&now, NULL);

    timeval_subtract(&delta, &tmp_node->last_report, &now);
    timeval_subtract(&delta, &tmp_node->last_report, &delta);

    if (delta.tv_sec > 0 || delta.tv_usec > LEASE_TIME * 1000) {
        warn("node lease timeout, id", tmp_node->id);
        tmp_node->status = NODE_OFFLINE;
    } else {
        tmp_node->last_report = now;
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
using namespace fankux;
int main(void) {
    g_ager->run();

    return 0;
}

#endif /* DEBUG_ARRANGER */
