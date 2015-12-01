#ifndef NET_H
#define NET_H

#define TCP_PORT 7234

#ifdef __cplusplus
extern "C" {
namespace fankux {
#endif

struct sock_info;

int create_tcpsocket();

int create_tcpsocket_listen(int port);

int set_nonblocking(int fd);

int accept_tcp(int fd, struct sock_info* info);

ssize_t read_tcp(int fd, char* buf, size_t buflen);

ssize_t write_tcp(int fd, const char* buf, size_t buflen);

#endif /* NET_H */

#ifdef __cplusplus
}
};
#endif
