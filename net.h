#ifndef NET_H
#define NET_H

#define TCP_PORT 7234

struct sock_info;

int create_tcpsocket();

int create_tcpsocket_listen(int port);

int set_nonblocking(int fd);

int accept_tcp(int fd, struct sock_info *info);

int read_tcp(int fd, char *buf, size_t buflen, size_t *readlen);

int write_tcp(int fd, char *buf, size_t buflen, size_t *writelen);

#endif /* NET_H */