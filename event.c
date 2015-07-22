#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

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

inline struct webc_event *event_create(int fd) {
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

inline void event_free(struct webc_event *event) {
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
    if (epoll_ctl((epfd), EPOLL_CTL_DEL, (ev)->fd, &(ev)->epev) == -1) {       \
        error("epoll del faild, errno:%d, error:%s", errno, strerror(errno));  \
    }                                                       \
    ev->status = EVENT_STATUS_ERR;                          \
} while(0)

static int event_err_handle(struct webc_netinf *netinf) {
    struct epoll_event *eplist = netinf->eplist;
    struct webc_event *ev;
    int n = 0, epfd = netinf->epfd;

    for (int i = 0; i < netinf->list_num; ++i) {
        uint32_t fire_event = eplist[i].events;
        ev = eplist[i].data.ptr;
        if (ev->status == EVENT_STATUS_ERR) continue;

        if (fire_event & EPOLLERR || fire_event & EPOLLHUP) {
            if (errno == 0) continue;

            error("epoll error occured, errno:%d, error:%s",
                  errno, strerror(errno));
            if (ev == NULL) {
                debug("epoll error occured, but webc_event not create");
                continue;
            }
            //TODO...error handling
            epoll_del(epfd, ev);
        }
        ++n;
    }

    return n;
}

static int event_accept_handle(struct webc_netinf *netinf) {
    struct epoll_event *eplist = netinf->eplist;
    struct webc_event *ev;
    int fire_sock, conn_sock, n = 0, epfd = netinf->epfd;
    int flags;

    for (int i = 0; i < netinf->list_num; ++i) {
        fire_sock = eplist[i].data.fd;
        if (fire_sock != netinf->listenfd) continue;

        conn_sock = accept_tcp(netinf->listenfd, NULL);
        if (conn_sock == -1) {
            error("accept, fd:%d", fire_sock);
        }

        set_nonblocking(conn_sock);
        ev = event_create(conn_sock);
        flags = EPOLLET | (ev->flags & EVENT_READ ? EPOLLIN : 0) |
                (ev->flags & EVENT_WRITE ? EPOLLOUT : 0);

        epoll_add(epfd, flags, ev);

        //TODO...job after connection
        ++n;
    }

    return n;
}

static int event_read_handle(struct webc_netinf *netinf) {
    struct epoll_event *eplist = netinf->eplist;
    struct webc_event *ev;
    int fire_sock, status, n = 0, epfd = netinf->epfd;
    char recvbuf[BUFSIZ] = "\0";
    size_t recvlen = 0;
    int flags;

    for (int i = 0; i < netinf->list_num; ++i) {
        uint32_t fire_event = eplist[i].events;
        if (fire_event & EPOLLIN == 0) continue;

        fire_sock = eplist[i].data.fd;
        debug("EPOLLIN fired, fd: %d", fire_sock);
        ev = eplist[i].data.ptr;
        ev->epev.data.fd = fire_sock;

        status = read_tcp(ev->fd, recvbuf, BUFSIZ, &recvlen);
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
    int fire_sock, status, n = 0, epfd = netinf->epfd;
    size_t writelen = 0;

    for (int i = 0; i < netinf->list_num; ++i) {
        uint32_t fire_event = eplist[i].events;
        if (fire_event & EPOLLOUT == 0) continue;

        fire_sock = eplist[i].data.fd;
        debug("EPOLLOUT fired, fd: %d", fire_sock);

        ev = eplist[i].data.ptr;
        ev->epev.data.fd = fire_sock;

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

inline int event_poll(struct webc_netinf *netinfo) {
    int n = epoll_wait(netinfo->epfd, netinfo->eplist, EVENT_LIST_MAX, 500);
    if (n == -1) {
        fatal("epoll_wait error, errno:%d, error:%s", errno, strerror(errno));
        return -1;
    }
    netinfo->list_num = n;
    return n;
}

void event_loop(struct webc_netinf *netinf) {
//    signal(SIGINT, release);
//    signal(SIGTERM, release);

    struct epoll_event events[10];
    int epfd = epoll_create(10);
    if (epfd == -1) {
        fatal("epoll faild, errno:%d, error:%s", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    netinf->epfd = epfd;
    netinf->eplist = events;
    netinf->list_num = 0;

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
    }
    close(epfd);
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
    log("request came, sock:%d, content:%s", event->fd, buf);

    fstr_free(fstr_getpt(buf));
    return (void *) 0;
}

static inline void *test_send(struct webc_event_send_param *param) {
    if (param == NULL) {
        //TODO...global error;
        return (void *) -1;
    }

    struct fstr *sbuf = fstr_createlen(NULL, 200);
    struct fstr *sargs = fstr_getpt(param->args);
    sprintf(sbuf->buf, "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Content-Length: 12\r\n\r\n"
            "Hello World!%s", sargs->buf);
    param->event->retbuf = sbuf->buf;

    fstr_free(sargs);

    return (void *) 0;
}

static void *poll_routine(void *arg) {
    int listenfd = create_tcpsocket_listen(TCP_PORT);
    log("%d", listenfd);

    struct webc_netinf *netinf = arg;
    netinf->listenfd;

    event_loop(netinf);
    return (void *) 0;
}

int main(int argc, char **argv) {
    struct webc_netinf netinf;

    pthread_t pid;
    pthread_create(&pid, NULL, poll_routine, (void *) &netinf);
    sleep(1);


    int sockfd = create_tcpsocket();
    struct webc_event *event = event_create(sockfd);
    event->flags = EVENT_READ | EVENT_WRITE;
    event->recv_func = test_recv;
    event->send_func = test_send;
    event->send_param.args = fstr_create("heheda");

    struct sockaddr_in addr_in;
    addr_in.sin_port = htons(TCP_PORT);
    if (argc > 1) {
        addr_in.sin_port = htons((uint16_t) atoi(argv[1]));
    }
    addr_in.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr_in.sin_family = AF_INET;
    event_connect(&netinf, event, &addr_in);


    log("peer info : addr:%s, port:%d",
        inet_ntoa(addr_in.sin_addr),
        ntohs(addr_in.sin_port));

    while (1);
    return 0;
}

#endif /* DEBUG_EVENT */
