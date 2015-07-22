#ifndef EVENT_H
#define EVENT_H

#include <sys/epoll.h>

#define EVENT_LIST_MAX  65535
#define EVENT_READ      1
#define EVENT_WRITE     2

#define EVENT_STATUS_ERR 0
#define EVENT_STATUS_OK 1

struct webc_event;

struct webc_event_send_param {
    struct webc_event *event;
    void *args;
};

struct webc_event {
    /* network */
    int fd;
    int status;
    int flags;
    struct epoll_event epev;

    /* job */
    int async;
    int keepalive;

    /* if call async, buf is fstr, must be freed at inner routine */
    void *(*recv_func)(char *buf, struct webc_event *event);

    void *(*send_func)(struct webc_event_send_param *param);

    char *retbuf; //fstr
    struct webc_event_send_param send_param;
};

struct webc_netinf {
    int listenfd;

    /* epoll field */
    int epfd;
    struct epoll_event *eplist;
    int list_num;
};

struct webc_event *event_create(int fd);

void event_free(struct webc_event *event);

int event_poll(struct webc_netinf *netinfo);

void event_loop(struct webc_netinf *netinf);

int event_connect(struct webc_netinf *netinf, struct webc_event *ev,
                  struct sockaddr_in *saddr);

#endif //EVENT_H
