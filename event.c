#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>

#include "common/fmem.h"
#include "common/flog.h"
#include "common/check.h"
#include "net.h"
#include "event.h"

#ifdef __cplusplus
namespace fankux{
#endif

//void release(int signal_num) {
//    if (listen_sock != -1 && listen_sock != 0) {
//        close(listen_sock);
//    }
//}

int event_active;

inline struct event* event_info_create(int fd) {
    struct event* ev = fmalloc(sizeof(struct event));
    if (ev == NULL) return NULL;

    ev->fd = fd;
    ev->status = EVENT_STATUS_OK;
    ev->flags = EVENT_READ;
    ev->async = 0;
    ev->keepalive = 1;
    ev->recv_func = NULL;
    ev->send_func = NULL;
    ev->recv_param = NULL;
    ev->send_param = NULL;
    ev->faild_func = NULL;
    return ev;
}

inline void event_info_init(struct event* ev) {
    ev->fd = 0;
    ev->status = EVENT_STATUS_OK;
    ev->flags = EVENT_READ;
    ev->async = 0;
    ev->keepalive = 1;
    ev->recv_func = NULL;
    ev->send_func = NULL;
    ev->recv_param = NULL;
    ev->send_param = NULL;
    ev->faild_func = NULL;
}

inline void event_info_free(struct event* event) {
    if (event == NULL) return;
    ffree(event);
}

#define epoll_del(epfd, ev) do {                            \
    ev->status = EVENT_STATUS_ERR;                          \
    if (epoll_ctl((epfd), EPOLL_CTL_DEL, (ev)->fd, &(ev)->epev) == -1) {      \
        warn("epoll del faild, errno:%d, error:%s", errno, strerror(errno));  \
    }                                                       \
    close((ev)->fd);                                        \
} while(0)

#define epoll_add(epfd, ev) do {                            \
    (ev)->epev.events = EPOLLET |                           \
            ((ev)->flags & EVENT_READ ? EPOLLIN : 0) |      \
            ((ev)->flags & EVENT_WRITE ? EPOLLOUT : 0);     \
    (ev)->epev.data.ptr = (ev);                             \
    if (epoll_ctl((epfd), EPOLL_CTL_ADD, (ev)->fd, &(ev)->epev) == -1) {      \
        warn("epoll add faild, errno:%d, error:%s", errno, strerror(errno));  \
        (ev)->status = EVENT_STATUS_ERR;                    \
    }                                                       \
} while(0)

#define epoll_mod(epfd, ev) do {                            \
    (ev)->epev.events = EPOLLET |                           \
            ((ev)->flags & EVENT_READ ? EPOLLIN : 0) |      \
            ((ev)->flags & EVENT_WRITE ? EPOLLOUT : 0);     \
    (ev)->epev.data.ptr = (ev);                             \
    if (epoll_ctl((epfd), EPOLL_CTL_MOD, (ev)->fd, &(ev)->epev) == -1) {      \
        warn("epoll mod faild, errno:%d, error:%s", errno, strerror(errno));  \
        (ev)->status = EVENT_STATUS_ERR;                    \
    }                                                       \
} while(0)

static int event_err_handle(struct netinf* netinf) {
    struct epoll_event* eplist = netinf->eplist;
    struct event* ev;
    int epfd = netinf->epfd;
    int n = 0;

    for (int i = 0; i < netinf->list_num; ++i) {
        uint32_t fire_event = eplist[i].events;
        ev = eplist[i].data.ptr;
        if (ev == NULL) {
            debug("epoll error occured, but webc_event not create");
            continue;
        }
        if (ev->status & EVENT_STATUS_ERR) {
            debug("invalid sock, fd: %d", ev->fd);
            continue;
        }

        if (fire_event & EPOLLERR || fire_event & EPOLLHUP) {
            if (errno == 0) continue;

            warn("epoll error occured, fire_event:%d, errno:%d, error:%s",
                    fire_event, errno, strerror(errno));

            //TODO...error handling
            epoll_del(epfd, ev);
            ++n;
        }
    }

    return n;
}

static int event_accept_handle(struct netinf* netinf) {
    struct epoll_event* eplist = netinf->eplist;
    struct event* ev;
    struct event* conn_ev;
    int fire_sock;
    int conn_sock;
    int epfd = netinf->epfd;
    int n = 0;

    for (int i = 0; i < netinf->list_num; ++i) {
        ev = eplist[i].data.ptr;
        fire_sock = ev->fd;
        if (fire_sock != netinf->listenfd) continue;

        while ((conn_sock = accept_tcp(netinf->listenfd, NULL)) != -1) {
            debug("accept fired, listen fd: %d, conn fd: %d", fire_sock, conn_sock);

            if (conn_sock == -1) {
                warn("accept, fd:%d", fire_sock);
            }

            set_nonblocking(conn_sock);
            conn_ev = event_info_create(conn_sock);
            conn_ev->flags = (ev->flags & EVENT_READ ? EVENT_READ : 0) |
                             (ev->flags & EVENT_WRITE ? EVENT_WRITE : 0);
            conn_ev->async = ev->async;
            conn_ev->recv_func = ev->recv_func;
            conn_ev->send_func = ev->send_func;
            conn_ev->keepalive = ev->keepalive;
            conn_ev->recv_param = ev->recv_param;
            conn_ev->send_param = ev->send_param;
            conn_ev->faild_func = ev->faild_func;

            epoll_add(epfd, conn_ev);
            ++n;
        }
    }

    return n;
}

static int event_read_handle(struct netinf* netinf) {
    struct epoll_event* eplist = netinf->eplist;
    struct event* ev;
    int fire_sock;
    int sockerr;
    int n = 0;
    int epfd = netinf->epfd;
    socklen_t socklen = sizeof(int);

    for (int i = 0; i < netinf->list_num; ++i) {
        uint32_t fire_event = eplist[i].events;
        if (fire_event & EPOLLIN == 0) continue;

        ev = eplist[i].data.ptr;
        fire_sock = ev->fd;
        if (ev->flags & EVENT_ACCEPT) continue;
        if (ev->status == EVENT_STATUS_ERR) {
            debug("invalid sock, fd: %d", fire_sock);
            continue;
        }

        debug("EPOLLIN fired, fd: %d", fire_sock);
        getsockopt(fire_sock, SOL_SOCKET, SO_ERROR, &sockerr, &socklen);
        if (sockerr != 0) {
            debug("socket error, fd: %d, sockerror: %d, error: %s",
                    fire_sock, sockerr, strerror(sockerr));
            epoll_del(epfd, ev);
            continue;
        }

        if (ev->async) {
            //TODO...call function async, call by threadpoll
            ++n;
            continue;
        }

        if (ev->recv_func) {
            if (ev->recv_func(ev) != 0) {
                warn("EPOLLIN handler invoke error, fd: %d", fire_sock);
                epoll_del(epfd, ev);
            }
            epoll_mod(epfd, ev);
        }

        ++n;
    }

    return n;
}

static int event_write_handle(struct netinf* netinf) {
    struct epoll_event* eplist = netinf->eplist;
    struct event* ev;
    int fire_sock;
    int sockerr;
    int n = 0;
    int epfd = netinf->epfd;
    socklen_t socklen = sizeof(int);

    for (int i = 0; i < netinf->list_num; ++i) {
        uint32_t fire_event = eplist[i].events;
        if (fire_event & EPOLLOUT == 0) continue;

        ev = eplist[i].data.ptr;
        fire_sock = ev->fd;
        if (ev->flags & EVENT_ACCEPT) continue;
        if (ev->status == EVENT_STATUS_ERR) {
            debug("invalid sock, fd: %d", fire_sock);
            continue;
        }

        debug("EPOLLOUT fired, fd: %d", fire_sock);
        getsockopt(fire_sock, SOL_SOCKET, SO_ERROR, &sockerr, &socklen);
        if (sockerr != 0) {
            debug("socket error, fd: %d, sockerror: %d, error: %s", fire_sock,
                    sockerr, strerror(sockerr));
            epoll_del(epfd, ev);
            continue;
        }

        if (ev->async) {
            //TODO... call function async, call by threadpoll
            ++n;
            continue;
        }

        if (ev->send_func) {
            if (ev->send_func(ev) != 0) {
                warn("EPOLLOUT handler invok error, fd: %d", fire_sock);
                epoll_del(epfd, ev);
            }
        }

        if (ev->keepalive) {
            epoll_mod(epfd, ev);
        } else {
            epoll_del(epfd, ev);
        }
        ++n;
    }
    return n;
}

static int event_clear_handle(struct netinf* netinf) {
    struct epoll_event* eplist = netinf->eplist;
    struct event* ev;
    int n = 0;
    for (int i = 0; i < netinf->list_num; ++i) {
        ev = eplist[i].data.ptr;
        debug("clear handler...");
        if (ev->faild_func) {
            debug("call faild func");
            ev->faild_func(ev);
        }
        if (ev->status == EVENT_STATUS_ERR) {
            eplist[i].data.ptr = NULL;
            close(ev->fd);
            if (ev->flags & EVENT_REUSE == 0) {
                event_info_free(ev);
            }
            ++n;
        }
    }
    return n;
}

inline int event_poll(struct netinf* netinfo) {
    int n = epoll_wait(netinfo->epfd, netinfo->eplist, EVENT_LIST_MAX, 500);
    if (n == -1) {
        fatal("epoll_wait error, errno:%d, error:%s", errno, strerror(errno));
        return -1;
    }
    netinfo->list_num = n;
    return n;
}

struct netinf* event_create(size_t event_size) {
    struct netinf* netinf = fmalloc(sizeof(struct netinf));
    if (netinf == NULL) return NULL;

    int epfd = epoll_create(10);
    if (epfd == -1) {
        fatal("epoll faild, errno:%d, error:%s", errno, strerror(errno));
        return NULL;
    }

    netinf->epfd = epfd;
    netinf->list_num = 0;
    netinf->eplist = fcalloc(event_size, sizeof(struct epoll_event));
    check_null_oom(netinf->eplist, goto faild, "netinf eplist");

    return netinf;

    faild:
    netinfo_free(netinf);
    return NULL;
}

void netinfo_free(struct netinf* netinf) {
    ffree(netinf->eplist);
    ffree(netinf);
}

void event_loop(struct netinf* netinf) {
    signal(SIGPIPE, SIG_IGN);

    event_active = 1;
    while (event_active) {
        if (event_poll(netinf) == -1) {
            event_active = 0;
            continue;
        }
        event_err_handle(netinf);
        event_accept_handle(netinf);
        event_read_handle(netinf);
        event_write_handle(netinf);
        event_clear_handle(netinf);
    }
    close(netinf->epfd);
    warn("event loop exit");
}

// TODO..resource clean
inline void event_stop(struct netinf* netinf) {
    event_active = 0;
}

int event_connect(struct netinf* netinf, struct event* ev, struct sockaddr_in* saddr) {
    int ret = connect(ev->fd, (const struct sockaddr*) saddr, sizeof(struct sockaddr));
    if (ret == -1 && errno != EINPROGRESS) {
        warn("connect remote server fail, error:%s", strerror(errno));
        return -1;
    }
    if (errno == EINPROGRESS)
            debug("connect in progress");

    epoll_add(netinf->epfd, ev);

    return 0;
}

inline void event_add(struct netinf* netinf, struct event* ev) {
    epoll_add(netinf->epfd, ev);
}

inline void event_mod(struct netinf* netinf, struct event* ev) {
    epoll_mod(netinf->epfd, ev);
}

#ifdef DEBUG_EVENT

static inline void* test_recv(char* buf, struct event* event) {
    log("request come, sock:%d, content:%s", event->fd, buf);

    fstr_free(fstr_getpt(buf));
    return (void*) 0;
}

static inline void* test_send(struct event_send_param* param) {
    if (param == NULL) {
        //TODO...global error;
        return (void*) -1;
    }

    struct fstr* sbuf = fstr_createlen(NULL, 200);
    struct fstr* sargs = fstr_getpt(param->arg);
    /* TODO..fstr api */
    sbuf->len += sprintf(sbuf->buf, "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Content-Length: 12\r\n\r\n"
            "Hello World!%s", sargs->buf);
    sbuf->free -= sbuf->len;
    flist_add_tail(&param->event->retbuf, sbuf->buf);

    if (param->arg_free_func)
        param->arg_free_func(param->arg);
    param->arg = NULL;

    return (void*) 0;
}

static void* poll_routine(void* arg) {
    struct netinf* netinf = arg;
    int listenfd = create_tcpsocket_listen(TCP_PORT);
    log("listen fd: %d", listenfd);

    netinf->listenfd = listenfd;

    struct event* event = event_info_create(listenfd);
    event->flags = EVENT_ACCEPT | EVENT_READ | EVENT_WRITE;
    event->recv_func = test_recv;
    event->send_func = test_send;
    event->send_param.arg = fstr_create("heheda")->buf;
    event_add(netinf, event);

    event_loop(netinf);

    return (void*) 0;
}

int main(int argc, char** argv) {
    struct netinf* netinf = event_create(EVENT_LIST_MAX);

    pthread_t pid;
    pthread_create(&pid, NULL, poll_routine, (void*) netinf);
    sleep(1);

//    int sockfd = create_tcpsocket();
//    struct event *event = event_info_create(sockfd);
//    event->flags = EVENT_READ | EVENT_WRITE;
//    event->recv_func = test_recv;
//    event->send_func = test_send;
//    event->keepalive = 0;
//    event->send_param.arg = fstr_create("heheda")->buf;
//
//    struct sockaddr_in addr_in;
//    addr_in.sin_port = htons(TCP_PORT);
//    if (argc > 1) {
//        addr_in.sin_port = htons((uint16_t) atoi(argv[1]));
//    }
//    addr_in.sin_addr.s_addr = inet_addr("127.0.0.1");
//    addr_in.sin_family = AF_INET;
//    event_connect(netinf, event, &addr_in);

//    log("peer info : addr:%s, port:%d",
//        inet_ntoa(addr_in.sin_addr),
//        ntohs(addr_in.sin_port));

    while (1);
    return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* DEBUG_EVENT */
