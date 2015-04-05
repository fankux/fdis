#include "events.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <unistd.h>
#include <sys/epoll.h>
#include <netinet/in.h>


int event_init(int ev_arr_size){
    struct epoll_event ev;

    /* the arg not need since kernel 2.6.8, but must positive */
    return epoll_create(ev_arr_size);
}

int event_poll(int epfd, void * ev_arr, int ev_arr_size){

    epoll_wait(epfd, ev_arr, ev_arr_size, EVENT_POLL_TIME);

    return 0;
}


/* epoll event loop, the buffer recevied while send to "CommandDo",
** it would prase data package, add command to a command queue;
** if get a connect event, it will create a client, if a client has
** 'keep alive' flag, the sock wouldn't close, but any socket which
** recevied event occured error would kicked out from epoll list  */
int EventLoopStart(){
    struct epoll_event ev;
    struct sockaddr_in addr_client;
    socklen_t addr_client_len = sizeof(addr_client);
    int sockfd, connfd;
    int nfds; /* number of events */
    int i, flag, buf_len, ip_exist, send_buf_len, buf_index = 0;
    char buf[SABER_RECVBUF_MAX + 1] = "\0";
    fdList * client_list;
    fdListNode * client_list_node;

    printf("Eventing loop good!\nSaberEngine initialized success!\n");

    server.event_flag = 1;
    while(server.event_flag){
        nfds = epoll_wait(server.epoll_fd, server.event_list,
                SABER_EVENT_SIZE, 50);
        if(nfds > 0) {
        }else if(nfds == -1){
            exit(EXIT_FAILURE);
        }

        for(i = 0; i < nfds; ++i){
            sockfd = server.event_list[i].data.fd;
            client_list = server.client_list;
            if(sockfd == server.listen_fd){/* connect event */
                /* read all request may be come out at same time */
#ifdef DEBUG_INFO
				printf("connect event fired!\n");
#endif
                while((connfd = accept(server.listen_fd,
                        (struct sockaddr *)&addr_client,
                        &addr_client_len)) > 0){
#ifdef DEBUG_INFO
					printf("accepted, connfd is:%d\n", connfd);
#endif
                    if(client_list->len >= SABER_SERVER_CLIENT_MAX)
                        break;

                    set_noblocking(connfd);
                    ev.data.fd = connfd;
                    ev.events = EPOLLIN | EPOLLET;
                    epoll_ctl(server.epoll_fd, EPOLL_CTL_ADD, connfd, &ev);

                    client_list->CmpValFunc = listCmpFuncIp;
                    /* if this ip exist */
                    ip_exist = fdListGet(client_list,
                            &addr_client.sin_addr.s_addr,
                            &client_list_node);
#ifdef DEBUG_INFO
					printf("list get result:%d\n", ip_exist);
#endif
                    /* add new one to list */
                    if(ip_exist == FDLIST_NONE){
#ifdef DEBUG_INFO
						printf("new client added\n");
#endif
                        fdListAddHead(server.client_list,
                                (void *)sclntCreate(
                                        connfd,
                                        addr_client.sin_addr.s_addr));
                    }else{ /* change the ip with new connfd */
#ifdef DEBUG_INFO
						printf("got the client\n");
#endif
                        ((sclnt *)client_list_node->data)->fd = connfd;
                    }
                }
            }else if(server.event_list[i].events & EPOLLIN ){
                /* read event, a connect fd will be deleted from
                ** epoll_list when reading complete */
#ifdef DEBUG_INFO
				printf("read event fired,buf_index:%d\n", buf_index);
#endif
                flag = 1;
                while(flag){
                    buf_len = read(sockfd, buf + buf_index, SABER_RECVBUF_BLOCK);
                    if(-1 == buf_len && errno != EAGAIN){ /* error */
#ifdef DEBUG_INFO
						printf("read error occur\n");
#endif
                        epoll_delete(server.epoll_fd, sockfd, ev);
                        clientlist_forcedelete(client_list, &sockfd);

                        break;
                    }else if(-1 == buf_len && errno == EAGAIN){/* completed */
#ifdef DEBUG_INFO
						printf("reading complete, buf_len:%d\n", buf_index);
#endif
                        buf[buf_index] = '\0';
                        CommandDo(addr_client.sin_addr.s_addr, buf);

                        epoll_setwrite(server.epoll_fd, sockfd, ev);

                        break;
                    }else if(0 == buf_len){ /* client shutdown */
#ifdef DEBUG_INFO
							printf("client shutdown\n");
#endif
                        epoll_delete(server.epoll_fd, sockfd, ev);
                        clientlist_forcedelete(client_list, &sockfd);

                        break;
                    }/* else{} recevied a part of buf */
                    /* max buf size achieved */
                    if(SABER_RECVBUF_MAX - buf_index <= SABER_RECVBUF_BLOCK){
#ifdef DEBUG_INFO
						printf("achieved buf max, buf_index:%d\n", buf_index);
#endif
                        flag = 0;
                        buf[SABER_RECVBUF_MAX] = '\0';
                        CommandDo(addr_client.sin_addr.s_addr, buf);

                        epoll_setwrite(server.epoll_fd, sockfd, ev);
                    }else if(SABER_RECVBUF_BLOCK == buf_len){
                        /* still reading */
#ifdef DEBUG_INFO
						printf("still reading...buf_index: %d\n", buf_index);
#endif
                        flag = 1;
                        buf_index += buf_len;
                    }else{/* complete */
#ifdef DEBUG_INFO
							printf("read buf:%s\n", buf);
#endif
                        flag = 0;
                        buf[buf_len] = '\0';
                        CommandDo(addr_client.sin_addr.s_addr, buf);

                        epoll_setwrite(server.epoll_fd, sockfd, ev);
                    }
                }
                buf_index = 0;
            }
            if(server.event_list[i].events & EPOLLOUT){/* wirte event */
#ifdef DEBUG_INFO
							printf("write event fired\n");
#endif
                client_list->CmpValFunc = listCmpFuncIp;
                ip_exist = fdListGet(client_list,
                        &addr_client.sin_addr.s_addr,
                        &client_list_node);
                if(ip_exist == FDLIST_NONE){
#ifdef DEBUG_INFO
					printf("client info destoryed\n");
#endif
                    epoll_delete(server.epoll_fd, sockfd, ev);
                }else{/* got the client info */
                    send_buf_len =
                            ((sclnt *)client_list_node->data)->result->len;
                    buf_index = 0;

                    while(send_buf_len != buf_index){
                        buf_len =
                                write(sockfd,
                                        ((sclnt *)client_list_node->data)->result->buf+
                                                buf_index, send_buf_len);
                        if(-1 == buf_len && errno == EAGAIN){
                            /* writing complete */
#ifdef DEBUG_INFO
							printf("writing complete!\n");
#endif
                            break;
                        }else if(-1 == buf_len && errno != EAGAIN){/* error */
#ifdef DEBUG_INFO
							printf("write error occur:%s \n", strerror(errno));
#endif
                            break;
                        }else if(buf_len >= 0){/* writing, maybe complete */
#ifdef DEBUG_INFO
							printf("write content:%s, buf_len:%d, buf_index:%d\n",
								   ((sclnt *)client_list_node->data)->result->buf,
								   buf_len, buf_index);
#endif
                            buf_index += buf_len;
                        }
                    }
                    buf_index = 0;

                    epoll_delete(server.epoll_fd, sockfd, ev);
                    clientlist_trydelete(client_list, &sockfd);
                }
            }
        }
    }
    kill(server.persist_pid, SIGINT);
}