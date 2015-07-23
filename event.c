#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "common/common.h"
#include "common/fstr.h"
#include "common/fmem.h"
#include "common/flog.h"
#include "net.h"
#include "webc.h"
#include "event.h"

//void release(int signal_num) {
//    if (listen_sock != -1 && listen_sock != 0) {
//        close(listen_sock);
//    }
//}

inline struct webc_event *event_info_create(int fd) {
    struct webc_event *ev = malloc(sizeof(struct webc_event));
    if (ev == NULL) return NULL;

    ev->fd = fd;
    ev->status = EVENT_STATUS_OK;
    ev->flags = EVENT_READ;
    ev->async = 0;
    ev->keepalive = 1;
    ev->recv_func = NULL;
    ev->send_func = NULL;

    return ev;
}

inline void event_info_free(struct webc_event *event) {
    if (event == NULL) return;

    if (event->send_param.arg_free_func)
        event->send_param.arg_free_func(event->send_param.arg);
    event->send_param.arg = NULL;

    ffree(event);
}

#define epoll_add(epfd, flags, ev) do {                     \
    (ev)->epev.events = (flags);                            \
    (ev)->epev.data.ptr = (ev);                             \
    if (epoll_ctl((epfd), EPOLL_CTL_ADD, (ev)->fd, &(ev)->epev) == -1) {   \
        error("epoll add faild, errno:%d, error:%s", errno, strerror(errno));  \
    }                                                       \
} while(0)

#define epoll_mod(epfd, flags, ev) do {                     \
    (ev)->epev.events = (flags);                            \
    (ev)->epev.data.ptr = (ev);                             \
    if (epoll_ctl((epfd), EPOLL_CTL_MOD, (ev)->fd, &(ev)->epev) == -1) {       \
        error("epoll mod faild, errno:%d, error:%s", errno, strerror(errno));  \
    }                                                       \
} while(0)

/* In  kernel  versions before 2.6.9, the EPOLL_CTL_DEL operation required a non-null pointer
in event, even though this argument is ignored.  Since Linux 2.6.9, event can be specified
as NULL when using EPOLL_CTL_DEL.  Applications that need to be portable to kernels before
2.6.9 should specify a non-null pointer in event. */
#define epoll_del(epfd, ev) do {                            \
    ev->status = EVENT_STATUS_ERR;                          \
    if (epoll_ctl((epfd), EPOLL_CTL_DEL, (ev)->fd, &(ev)->epev) == -1) {       \
        error("epoll del faild, errno:%d, error:%s", errno, strerror(errno));  \
    }                                                       \
} while(0)

static int event_err_handle(struct webc_netinf *netinf) {
    struct epoll_event *eplist = netinf->eplist;
    struct webc_event *ev;
    int n = 0, epfd = netinf->epfd;

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

            error("epoll error occured, errno:%d, error:%s",
                  errno, strerror(errno));

            //TODO...error handling
            epoll_del(epfd, ev);
            ++n;
        }
    }

    return n;
}

static int event_accept_handle(struct webc_netinf *netinf) {
    struct epoll_event *eplist = netinf->eplist;
    struct webc_event *ev, *conn_ev;
    int fire_sock, conn_sock, n = 0, epfd = netinf->epfd;
    int flags;

    for (int i = 0; i < netinf->list_num; ++i) {
        ev = eplist[i].data.ptr;
        fire_sock = ev->fd;
        if (fire_sock != netinf->listenfd) continue;

        while ((conn_sock = accept_tcp(netinf->listenfd, NULL)) != -1) {
            debug("accept fired, fd: %d", conn_sock);

            if (conn_sock == -1) {
                error("accept, fd:%d", fire_sock);
            }

            set_nonblocking(conn_sock);
            conn_ev = event_info_create(conn_sock);
            conn_ev->flags = (ev->flags & EVENT_READ ? EVENT_READ : 0) |
                             (ev->flags & EVENT_WRITE ? EVENT_WRITE : 0);
            conn_ev->async = ev->async;
            conn_ev->recv_func = ev->recv_func;
            conn_ev->send_func = ev->send_func;
            conn_ev->send_param = ev->send_param;

//            epoll_del(epfd, ev);
            flags = EPOLLET | (conn_ev->flags & EVENT_READ ? EPOLLIN : 0) |
                    (conn_ev->flags & EVENT_WRITE ? EPOLLOUT : 0);
            epoll_add(epfd, flags, conn_ev);

            //TODO...job after connection
            ++n;
        }
    }

    return n;
}

static int event_read_handle(struct webc_netinf *netinf) {
    struct epoll_event *eplist = netinf->eplist;
    struct webc_event *ev;
    int fire_sock, status, flags, sockerr, n = 0, epfd = netinf->epfd;
    char recvbuf[BUFSIZ] = "\0";
    socklen_t socklen = sizeof(int);
    size_t recvlen = 0;

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
            debug("socket error, fd: %d, sockerror: %d, error: %s", fire_sock,
                  sockerr, strerror(sockerr));
            epoll_del(epfd, ev);
            continue;
        }

        status = read_tcp(fire_sock, recvbuf, BUFSIZ, &recvlen);
        if (status == -1) {
            epoll_del(epfd, ev);
            continue;
        }
        if (ev->async) {
            //TODO...call function async, call by threadpoll
        } else {
            if (ev->recv_func) {
                struct fstr *frecv = fstr_createlen(recvbuf, recvlen);
                ev->recv_func(frecv->buf, ev);
            }
        }

        /*TODO...job after reading,
         * some worker join to thread pool,
         * it would be register a callback in webc_event,
         * then threadpoll's callback would call it and change the event mask of epoll
         */

        flags = EPOLLET | (ev->flags & EVENT_READ ? EPOLLIN : 0) |
                (ev->flags & EVENT_WRITE ? EPOLLOUT : 0);
        epoll_mod(epfd, flags, ev);
        ++n;
    }

    return n;
}

static int event_write_handle(struct webc_netinf *netinf) {
    struct epoll_event *eplist = netinf->eplist;
    struct webc_event *ev;
    int fire_sock, status, sockerr, n = 0, epfd = netinf->epfd;
    socklen_t socklen = sizeof(int);
    size_t writelen = 0;

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
        } else {
            if (ev->send_func) {
                ev->send_param.event = ev;
                ev->send_func(&ev->send_param);
            }
        }

        //send_func could produce retbuf which is fstr
        if (ev->retbuf) {
            struct fstr *ssend = fstr_getpt(ev->retbuf);
            status = write_tcp(ev->fd, ssend->buf, (size_t) ssend->len,
                               &writelen);
            ffree(ssend);
            ev->retbuf = NULL;
            if (status == -1) {
                error("write_tcp faild, fd:%d, errno: %d, error:%s",
                      ev->fd, errno, strerror(errno));
                epoll_del(epfd, ev);
                continue;
            }
        }

        if (ev->keepalive) {
            epoll_mod(epfd, EPOLLET | EPOLLIN, ev);
        } else {
            epoll_del(epfd, ev);
        }
        ++n;
    }

    return n;
}

static int event_clear_handle(struct webc_netinf *netinf) {
    struct epoll_event *eplist = netinf->eplist;
    struct webc_event *ev;
    int n = 0;
    for (int i = 0; i < netinf->list_num; ++i) {
        ev = eplist[i].data.ptr;
        if (ev->status == EVENT_STATUS_ERR) {
            eplist[i].data.ptr = NULL;
            close(ev->fd);
            event_info_free(ev);
            ++n;
        }
    }
    return n;
}

inline int event_poll(struct webc_netinf *netinfo) {
    int n = epoll_wait(netinfo->epfd, netinfo->eplist, EVENT_LIST_MAX, 500);
    if (n == -1) {
        fatal("epoll_wait error, errno:%d, error:%s", errno, strerror(errno));
        return -1;
    }
    netinfo->list_num = n;
    return n;
}

struct webc_netinf *event_create(size_t event_size) {
    struct webc_netinf *netinf = fmalloc(sizeof(struct webc_netinf));
    if (netinf == NULL) return NULL;

    int epfd = epoll_create(10);
    if (epfd == -1) {
        fatal("epoll faild, errno:%d, error:%s", errno, strerror(errno));
        return NULL;
    }

    netinf->epfd = epfd;
    netinf->list_num = 0;
    netinf->eplist = fcalloc(event_size, sizeof(struct epoll_event));
    check_null_goto_oom(netinf->eplist, faild, "netinf eplist");

    return netinf;

    faild:
    netinfo_free(netinf);
    return NULL;
}

void netinfo_free(struct webc_netinf *netinf) {
    ffree(netinf->eplist);
    ffree(netinf);
}

void event_loop(struct webc_netinf *netinf) {
//    signal(SIGINT, release);
//    signal(SIGTERM, release);
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

int event_connect(struct webc_netinf *netinf, struct webc_event *ev,
                  struct sockaddr_in *saddr) {
    int ret = connect(ev->fd, (const struct sockaddr *) saddr,
                      sizeof(struct sockaddr));
    if (ret == -1 && errno != EINPROGRESS) {
        error("connect remote server fail, error:%s", strerror(errno));
        return -1;
    }
    if (errno == EINPROGRESS)
        log("connect in progress");

    // add to poll
    int flags = EPOLLET;
    flags |= ev->flags & EVENT_READ ? EPOLLIN : 0;
    flags |= ev->flags & EVENT_WRITE ? EPOLLOUT : 0;
    epoll_add(netinf->epfd, flags, ev);

    return 0;
}

#ifdef DEBUG_EVENT

static inline void *test_recv(char *buf, struct webc_event *event) {
    log("request come, sock:%d, content:%s", event->fd, buf);

    fstr_free(fstr_getpt(buf));
    return (void *) 0;
}

static inline void *test_send(struct webc_event_send_param *param) {
    if (param == NULL) {
        //TODO...global error;
        return (void *) -1;
    }

    struct fstr *sbuf = fstr_createlen(NULL, 200);
    struct fstr *sargs = fstr_getpt(param->arg);
    /* TODO..fstr api */
    sbuf->len += sprintf(sbuf->buf, "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Content-Length: 12\r\n\r\n"
            "Hello World!%s", sargs->buf);
    sbuf->free -= sbuf->len;
    param->event->retbuf = sbuf->buf;

    if (param->arg_free_func)
        param->arg_free_func(param->arg);
    param->arg = NULL;

    return (void *) 0;
}

static void *poll_routine(void *arg) {
    struct webc_netinf *netinf = arg;
    int listenfd = create_tcpsocket_listen(TCP_PORT);
    log("listen fd: %d", listenfd);

    netinf->listenfd = listenfd;

    struct webc_event *event = event_info_create(listenfd);
    event->flags = EVENT_ACCEPT | EVENT_READ | EVENT_WRITE;
    event->recv_func = test_recv;
    event->send_func = test_send;
    event->send_param.arg = fstr_create("heheda")->buf;
    epoll_add(netinf->epfd, EPOLLET | EPOLLIN, event);

    event_loop(netinf);

    fstr_free(event->send_param.arg);
    event_info_free(event);
    return (void *) 0;
}

int main(int argc, char **argv) {
    struct webc_netinf *netinf = event_create(EVENT_LIST_MAX);

    pthread_t pid;
    pthread_create(&pid, NULL, poll_routine, (void *) netinf);
    sleep(1);


    int sockfd = create_tcpsocket();
    struct webc_event *event = event_info_create(sockfd);
    event->flags = EVENT_READ | EVENT_WRITE;
    event->recv_func = test_recv;
    event->send_func = test_send;
    event->send_param.arg = fstr_create("heheda")->buf;

    struct sockaddr_in addr_in;
    addr_in.sin_port = htons(TCP_PORT);
    if (argc > 1) {
        addr_in.sin_port = htons((uint16_t) atoi(argv[1]));
    }
    addr_in.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr_in.sin_family = AF_INET;
//    event_connect(&netinf, event, &addr_in);


    log("peer info : addr:%s, port:%d",
        inet_ntoa(addr_in.sin_addr),
        ntohs(addr_in.sin_port));

    while (1);
    return 0;
}

#endif /* DEBUG_EVENT */
