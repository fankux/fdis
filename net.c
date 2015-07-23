#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "net.h"

int set_nonblocking(int fd) {
    int sock_opts;
    if (-1 == (sock_opts = fcntl(fd, F_GETFL))) {
        return -1;
    }
    if (-1 == fcntl(fd, F_SETFL, sock_opts | O_NONBLOCK)) {
        return -1;
    }

    return 0;
}

inline int create_tcpsocket() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) return -1;

    int reuse = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(&reuse));
    set_nonblocking(sockfd);

    return sockfd;
}

int create_tcpsocket_listen(int port) {
    struct sockaddr_in addr_in;
    memset(&addr_in, 0, sizeof(addr_in));
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons((uint16_t) port);
    addr_in.sin_addr.s_addr = htonl(INADDR_ANY);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (bind(sockfd, (struct sockaddr *) &addr_in, sizeof(addr_in)) == -1)
        return -1;

    if (listen(sockfd, 10) == -1)
        return -1;

    int reuse = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(&reuse));
    set_nonblocking(sockfd);

    return sockfd;
}

struct sock_info {
    struct sockaddr_in *addr;
    void *data;
};

int accept_tcp(int fd, struct sock_info *info) {
    int conn_sock;
    if (info == NULL)
        conn_sock = accept(fd, NULL, 0);
    else
        conn_sock = accept(fd, (struct sockaddr *) info->addr,
                           (socklen_t *) sizeof(info->addr));

    if (conn_sock == -1) {
        if (errno != EAGAIN && errno != ECONNABORTED && errno != EPROTO &&
            errno != EINTR)
            return -1;
    }
    return conn_sock;
}

int read_tcp(int fd, char *buf, size_t buflen, size_t *readlen) {
    size_t n = 0;
    ssize_t nread;
    while ((nread = read(fd, buf + n, buflen - 1)) > 0) {
        n += nread;
    }
    if (nread == -1 && errno != EAGAIN) {
        return -1;
    }
    if (readlen) *readlen = (size_t) nread;
    return 0;
}

int write_tcp(int fd, char *buf, size_t buflen, size_t *writelen) {
    int nwrite;
    size_t n = buflen;
    while (n > 0) {
        nwrite = write(fd, buf + buflen - n, n);
        if (nwrite < n) {
            if (nwrite == -1 && errno != EAGAIN) {
                return -1;
            }
            break;
        }
        n -= nwrite;
    }
    if (writelen) *writelen = buflen - n;
    return 0;
}