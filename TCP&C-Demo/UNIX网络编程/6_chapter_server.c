//
//  6_chapter.c
//  TCP&C-Demo
//
//  Created by sheng wang on 2019/11/11.
//  Copyright © 2019 feisu. All rights reserved.
//

#include "unp.h"

int main(int argc, char **argv)
{
    int i, maxi, maxfd, listenfd, connfd, sockfd;
    int nready, client[FD_SETSIZE];
    ssize_t n;
    fd_set rset, allset;
    char buf[MAXLINE];
    socklen_t client_len;
    struct sockaddr_in client_addr, server_addr;
    
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERV_PORT);
    
    if (bind(listenfd, (SA *)&server_addr, sizeof(server_addr)) < 0){
        printf("bind() error");
    }
    if (listen(listenfd, LISTENQ) < 0){
        printf("listen() error");
    }
    
    maxfd = listenfd;
    maxi = -1;
    for (i = 0; i < FD_SETSIZE; i++) {
        client [i] = -1;
    }
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);
    
    while (1) {
        rset = allset;
        //负值：select错误； 正值：某些文件可读写或出错, 也就是状态发生变化的描述符总数； 0：等待超时，没有可读写或错误的文件
        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        
        if (FD_ISSET(listenfd, &rset)) {//listenfd发生了变化，说明新客户端连接进来了
            printf("new client connect \n");
            client_len = sizeof(client_addr);
            connfd = accept(listenfd, (SA* )&client_addr, &client_len);
            
            for (i = 0; i < FD_SETSIZE; i++) {
                if (client[i] < 0) {
                    printf("new client : %d fd: %d \n", i, connfd);
                    client[i] = connfd;//把变化的client fd存起来
                    break;
                }
            }
            
            if (i == FD_SETSIZE) {
                printf("too many clients \n");
            }
            
            FD_SET(connfd, &allset);
            if (connfd > maxfd) {
                maxfd = connfd;
            }
            if (i > maxi) {
                maxi = i;
            }
            if (--nready <= 0) {//负值表示selcted错误
                continue;
            }
        }
        

        for (i = 0; i <= maxi; i++) {
            if ( (sockfd = client[i]) < 0 ) {//找出发生变化的client fd
                printf("client fd: %d", sockfd);
                continue;
            }
            
            if (FD_ISSET(sockfd, &rset)) {
                if ((n = read(sockfd, buf, MAXLINE)) == 0) {//读到EOF就关闭对应的client fd
                    printf("client close");
                    close(sockfd);
                    FD_CLR(sockfd, &allset);
                    client[i] = -1;
                }else{//否则就把从client fd中读到的内容发送给client
                    printf("client message: %s \n", buf);
                    write(sockfd, buf, n);
                    memset(&buf, 0, sizeof(buf));
                }
                
                if (--nready < 0) {
                    break;
                }
            }
        }
        
    }
}


void poll_server(int argc, char ** argv)
{
        int                    i, maxi, listenfd, connfd, sockfd;
        int                    nready;
        ssize_t                n;
        char                buf[MAXLINE];
        socklen_t            clilen;
        struct pollfd        client[OPEN_MAX];
        struct sockaddr_in    cliaddr, servaddr;

        listenfd = Socket(AF_INET, SOCK_STREAM, 0);

        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family      = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port        = htons(SERV_PORT);

        bind(listenfd, (SA *) &servaddr, sizeof(servaddr));
        listen(listenfd, LISTENQ);

        client[0].fd = listenfd;
        client[0].events = POLLRDNORM;
        for (i = 1; i < OPEN_MAX; i++)
            client[i].fd = -1;        /* -1 indicates available entry */
        maxi = 0;                    /* max index into client[] array */
    /* end fig01 */

    /* include fig02 */
        for ( ; ; ) {
            nready = poll(client, maxi+1, INFTIM);

            if (client[0].revents & POLLRDNORM) {    /* new client connection */
                clilen = sizeof(cliaddr);
                connfd = accept(listenfd, (SA *) &cliaddr, &clilen);
                printf("new client: %s\n", Sock_ntop((SA *) &cliaddr, clilen));

                for (i = 1; i < OPEN_MAX; i++)
                    if (client[i].fd < 0) {
                        client[i].fd = connfd;    /* save descriptor */
                        break;
                    }
                if (i == OPEN_MAX)
                    err_quit("too many clients");

                client[i].events = POLLRDNORM;
                if (i > maxi)
                    maxi = i;                /* max index in client[] array */

                if (--nready <= 0)
                    continue;                /* no more readable descriptors */
            }

            for (i = 1; i <= maxi; i++) {    /* check all clients for data */
                if ( (sockfd = client[i].fd) < 0)
                    continue;
                if (client[i].revents & (POLLRDNORM | POLLERR)) {//数据可读，或者返回了错误
                    if ( (n = read(sockfd, buf, MAXLINE)) < 0) {
                        if (errno == ECONNRESET) {
                                /*4connection reset by client */
                            printf("client[%d] aborted connection\n", i);
                            close(sockfd);
                            client[i].fd = -1;
                        } else
                            err_sys("read error");
                    } else if (n == 0) {
                            /*4connection closed by client */
                        printf("client[%d] closed connection\n", i);
                        close(sockfd);
                        client[i].fd = -1;
                    } else
                        writen(sockfd, buf, n);

                    if (--nready <= 0)
                        break;                /* no more readable descriptors */
                }
            }
        }
}
