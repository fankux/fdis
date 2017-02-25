//
// Created by fankux on 16-10-8.
//

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include "net.h"

int main() {
    int fd = fdis::create_tcpsocket_block();

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(7380);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (connect(fd, (const sockaddr*) &addr, sizeof(sockaddr_in)) == -1) {
        printf("connect socket error\n");
        return -1;
    }

    FILE* fp = fopen("./testconf.conf", "r");
    if (fp == NULL) {
        printf("open failed");
        return -1;
    }

    struct stat fs;
    stat("./content.txt", &fs);

    ssize_t ret;
    ssize_t len = 0;
    char buf[4];
    memcpy(buf, &fs.st_size, 4);
    fdis::write_tcp(fd, buf, 4);
    printf("package len : %li\n", fs.st_size);

    while(1) {
        if ((len = fread(buf, 1, 1, fp)) != 1){
            break;
        }

        printf("buf : %s\n", buf);

        printf("buf len : %zd\n", len);
        ret = fdis::write_tcp(fd, buf, (size_t) len);
        printf("write len : %zd\n", ret);
    }

    sleep(3);
    close(fd);
    fclose(fp);

    return 0;
}
