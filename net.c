#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>

#include "net.h"

int set_nonblocking(int fd)
{
    int sock_opts;
    if (-1 == (sock_opts = fcntl(fd, F_GETFL))) {
        return -1;
    }
    if (-1 == fcntl(fd, F_SETFL, sock_opts | O_NONBLOCK)) {
        return -1;
    }

    return 0;
}

int create_tcpsocket(int port)
{
    struct sockaddr_in addr_in;
    memset(&addr_in, 0, sizeof(addr_in));
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons((uint16_t) port);
    addr_in.sin_addr.s_addr = htonl(INADDR_ANY);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (-1 == bind(sockfd, (struct sockaddr *) &addr_in, sizeof(addr_in)))
        return -1;

    if (-1 == listen(sockfd, 10))
        return -1;


    int reuse = 1;
    setsockopt(sockfd, SOL_SOCKET,SO_REUSEADDR, &reuse, sizeof(&reuse));

    return sockfd;
}

struct sock_info {
    struct sockaddr_in * addr;
    void * data;
};

int accept_tcp(int fd, struct sock_info * info)
{
    int conn_sock;
    if (info == NULL)
        conn_sock = accept(fd, NULL, 0);
    else
        conn_sock = accept(fd, (struct sockaddr *) info->addr, (socklen_t *) sizeof(info->addr));

    if (conn_sock == -1) {
        if (errno != EAGAIN && errno != ECONNABORTED && errno != EPROTO && errno != EINTR)
            return -1;
    }
    return conn_sock;
}