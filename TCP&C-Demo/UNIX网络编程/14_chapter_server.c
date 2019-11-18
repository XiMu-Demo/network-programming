//
//  14_chapter_server.c
//  TCP&C-Demo
//
//  Created by sheng wang on 2019/11/15.
//  Copyright © 2019 feisu. All rights reserved.
//

#include "unp.c"
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/event.h>
#include <netdb.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>


#define IPADDRESS   "127.0.0.1"

//函数声明
//创建套接字并进行绑定
static int socket_bind(const char* ip,int port);
//IO多路复用poll
static void do_poll(int listenfd);
//处理多个连接
static void handle_connection(struct pollfd *connfds,int num);


//MARK:- KQUEUE

void kqueue_server()
{
     // Macos automatically binds both ipv4 and 6 when you do this.
    struct sockaddr_in6 addr = {};
    addr.sin6_len = sizeof(addr);
    addr.sin6_family = AF_INET6;
    addr.sin6_addr = in6addr_any; //(struct in6_addr){}; // 0.0.0.0 / ::
    addr.sin6_port = htons(SERV_PORT);
    
    int localFd = socket(addr.sin6_family, SOCK_STREAM, 0);
    assert(localFd != -1);
    
    int on = 1;
    setsockopt(localFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (bind(localFd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
    }
    assert(listen(localFd, 5) != -1);

    int kq = kqueue();
    
    struct kevent evSet;
    EV_SET(&evSet, localFd, EVFILT_READ, EV_ADD, 0, 0, NULL);
    assert(-1 != kevent(kq, &evSet, 1, NULL, 0, NULL));
    
    int junk = open("some.big.file", O_RDONLY);
    
    uint64_t bytes_written = 0;

    struct kevent evList[32];
    while (1) {
        // returns number of events
        int nev = kevent(kq, NULL, 0, evList, 32, NULL);
//        printf("kqueue got %d events\n", nev);
        
        for (int i = 0; i < nev; i++) {
            int fd = (int)evList[i].ident;
            
            if (evList[i].flags & EV_EOF) {
                printf("Disconnect\n");
                close(fd);
                // Socket is automatically removed from the kq by the kernel.
            } else if (fd == localFd) {
                struct sockaddr_storage addr;
                socklen_t socklen = sizeof(addr);
                int connfd = accept(fd, (struct sockaddr *)&addr, &socklen);
                assert(connfd != -1);
                
                // Listen on the new socket
                EV_SET(&evSet, connfd, EVFILT_READ, EV_ADD, 0, 0, NULL);
                kevent(kq, &evSet, 1, NULL, 0, NULL);
                printf("Got connection!\n");
                
                int flags = fcntl(connfd, F_GETFL, 0);
                assert(flags >= 0);
                fcntl(connfd, F_SETFL, flags | O_NONBLOCK);

                // schedule to send the file when we can write (first chunk should happen immediately)
                EV_SET(&evSet, connfd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, NULL);
                kevent(kq, &evSet, 1, NULL, 0, NULL);

            } else if (evList[i].filter == EVFILT_READ) {
                // Read from socket.
                char buf[1024];
                recv(fd, buf, sizeof(buf), 0);
                printf("message from client:%s", buf);
                
                
            } else if (evList[i].filter == EVFILT_WRITE) {
//                printf("Ok to write more!\n");
                
                off_t offset = (off_t)evList[i].udata;
                off_t len = 0;//evList[i].data;
                if (sendfile(junk, fd, offset, &len, NULL, 0) != 0) {
//                    perror("sendfile");
//                    printf("err %d\n", errno);
                    
                    if (errno == EAGAIN) {
                        // schedule to send the rest of the file
                        EV_SET(&evSet, fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, (void *)(offset + len));
                        kevent(kq, &evSet, 1, NULL, 0, NULL);
                    }
                }
                bytes_written += len;
                printf("wrote %lld bytes, %lld total\n", len, bytes_written);

            }
        }
    }
        
}


//MARK:- POLL
void poll_server()
{
    int  listenfd;
    listenfd = socket_bind(IPADDRESS,SERV_PORT);
    listen(listenfd,LISTENQ);
    do_poll(listenfd);
}


static int socket_bind(const char* ip,int port)
{
    int  listenfd;
    struct sockaddr_in servaddr;
    listenfd = socket(AF_INET,SOCK_STREAM,0);
    if (listenfd == -1)
    {
        perror("socket error:");
        exit(1);
    }
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&servaddr.sin_addr);
    servaddr.sin_port = htons(port);
    if (bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1)
    {
        perror("bind error: ");
        exit(1);
    }
    return listenfd;
}

static void do_poll(int listenfd)
{
    int  connfd;
    struct sockaddr_in cliaddr;
    socklen_t cliaddrlen;
    struct pollfd clientfds[OPEN_MAX];
    int maxi;
    int i;
    int nready;
    //添加监听描述符
    clientfds[0].fd = listenfd;
    clientfds[0].events = POLLIN;
    //初始化客户连接描述符
    for (i = 1;i < OPEN_MAX;i++)
        clientfds[i].fd = -1;
    maxi = 0;
    //循环处理
    for ( ; ; )
    {
        //获取可用描述符的个数
        nready = poll(clientfds,maxi+1,INFTIM);
        if (nready == -1)
        {
            perror("poll error:");
            exit(1);
        }
        //测试监听描述符是否准备好，revents表示fd实际发生的事件（POLLIN, POLLOUT之类的）,和POLLIN进行与操作，如果为1，就证明发生了POLLIN事件
        if (clientfds[0].revents & POLLIN)
        {
            cliaddrlen = sizeof(cliaddr);
            //接受新的连接
            if ((connfd = accept(listenfd,(struct sockaddr*)&cliaddr,&cliaddrlen)) == -1)
            {
                if (errno == EINTR)
                    continue;
                else
                {
                   perror("accept error:");
                   exit(1);
                }
            }
            fprintf(stdout,"accept a new client: %s:%d\n", inet_ntoa(cliaddr.sin_addr),cliaddr.sin_port);
            //将新的连接描述符添加到数组中
            for (i = 1;i < OPEN_MAX;i++)
            {
                if (clientfds[i].fd < 0)
                {
                    clientfds[i].fd = connfd;
                    break;
                }
            }
            if (i == OPEN_MAX)
            {
                fprintf(stderr,"too many clients.\n");
                exit(1);
            }
            //将新的描述符添加到读描述符集合中
            clientfds[i].events = POLLIN;
            //记录客户连接套接字的个数
            maxi = (i > maxi ? i : maxi);
            if (--nready <= 0)
                continue;
        }
        //处理客户连接
        handle_connection(clientfds,maxi);
    }
}

static void handle_connection(struct pollfd *connfds,int num)
{
    int i;
    ssize_t n;
    char buf[MAXLINE];
    memset(buf,0,MAXLINE);
    for (i = 1;i <= num;i++)
    {
        if (connfds[i].fd < 0)
            continue;
        //测试客户描述符是否准备好
        if (connfds[i].revents & POLLIN)
        {
            //接收客户端发送的信息
            n = read(connfds[i].fd,buf,MAXLINE);
            if (n == 0)
            {
                close(connfds[i].fd);
                connfds[i].fd = -1;
                continue;
            }
           // printf("read msg is: ");
            write(STDOUT_FILENO,buf,n);
            //向客户端发送buf
            write(connfds[i].fd,buf,n);
        }
    }
}

//MARK:- MAIN

int main(int argc,char *argv[])
{
    kqueue_server();
    return 0;
}
