#ifndef EVENT_H
#define EVENT_H

#include <sys/epoll.h>
#include <netinet/in.h>

#include "common/flist.h"

#ifdef __cplusplus
extern "C" {
namespace fankux{
#endif

#define EVENT_LIST_MAX  65535
#define EVENT_READ      1
#define EVENT_WRITE     2
#define EVENT_ACCEPT    4
#define EVENT_REUSE     8

#define EVENT_STATUS_ERR 0
#define EVENT_STATUS_OK 1

extern int event_active;

struct event {
    /* network */
    int fd;
    int status;
    int flags;
    struct epoll_event epev;

    /* job */
    int async;
    int keepalive;

    int (* recv_func)(struct event* ev);

    int (* send_func)(struct event* ev);

    int (* faild_func)(struct event* ev);

    void* recv_param;
    void* send_param;
    void* faild_param;
};

struct netinf {
    int listenfd;

    /* epoll field */
    int epfd;
    struct epoll_event* eplist;
    int list_num;
};

struct event* event_info_create(int fd);

void event_info_init(struct event* ev);

void event_info_free(struct event* event);

void netinfo_free(struct netinf* netinf);

struct netinf* event_create(size_t event_size);

int event_poll(struct netinf* netinfo);

void event_loop(struct netinf* netinf);

void event_stop(struct netinf* netinf);

int event_connect(struct netinf* netinf, struct event* ev, struct sockaddr_in* saddr);

void event_add(struct netinf* netinf, struct event* ev);

void event_mod(struct netinf* netinf, struct event* ev);

#ifdef __cplusplus
}
};
#endif

#endif //EVENT_H