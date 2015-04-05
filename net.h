#ifndef NET_H
#define NET_H

#define TCP_PORT 7234

struct sock_info;

int create_tcpsocket(int port);
int set_nonblocking(int fd);
int accept_tcp(int fd, struct sock_info * info);

#endif /* NET_H */