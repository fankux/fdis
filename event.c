#include <stdio.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "net.h"

#define LOG_CLI 1

#include "flog.h"

char buffer[4096];

inline void do_request(int fd)
{
    memset(buffer, 0, sizeof(buffer));
    read(fd, buffer, sizeof(buffer));
    log("request came, sock:%d, content:%s", fd, buffer);
}

inline void do_response(int fd)
{
    sprintf(buffer, "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Content-Length: 12\r\n\r\n"
            "Hello World!");
    write(fd, buffer, strlen(buffer));
}

int listen_sock = 0;

void release(int signal_num)
{
    if (listen_sock != -1 && listen_sock != 0) {
        close(listen_sock);
    }
}

#define DEBUG_EVENT
#ifdef DEBUG_EVENT

int main(void)
{
    int listen_sock = create_tcpsocket(80);
    log("%d", listen_sock);

    signal(SIGINT, release);
    signal(SIGTERM, release);

    struct epoll_event ev, events[10];
    int epfd = epoll_create(10);
    if (epfd == -1) {
        perror("epoll faild");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = listen_sock;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1) {
        perror("epoll add listen faild");
        exit(EXIT_FAILURE);
    }


    int conn_sock;
    for (; ;) {
        int nfds = epoll_wait(epfd, events, 10, 1000);
        if (nfds == -1) {
            perror("epoll_pwait");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < nfds; ++i) {
            int fire_sock = events[i].data.fd;
            if (fire_sock == listen_sock) {
                conn_sock = accept_tcp(listen_sock, NULL);
                if (conn_sock == -1) {
                    perror("accept");
                    exit(EXIT_FAILURE);
                }
                set_nonblocking(conn_sock);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = conn_sock;
                if (epoll_ctl(epfd, EPOLL_CTL_ADD, conn_sock, &ev) == -1) {
                    perror("epoll_ctl: conn_sock");
                    exit(EXIT_FAILURE);
                }
            } else if (events->events & EPOLLIN) {
                log("EPOLLIN fired, fd: %d", fire_sock);

                do_request(fire_sock);

                ev.events = EPOLLET | EPOLLOUT;
                ev.data.fd = fire_sock;
                epoll_ctl(epfd, EPOLL_CTL_MOD, events[i].data.fd, &ev);
            }

            if (events->events & EPOLLOUT) {
                log("EPOLLOUT fired, fd: %d", fire_sock);

                do_response(fire_sock);

                ev.events = EPOLLET | EPOLLIN;
                ev.data.fd = fire_sock;
                epoll_ctl(epfd, EPOLL_CTL_DEL, fire_sock, &ev);
            }
        }
    }
}

#endif /* DEBUG_EVENT */