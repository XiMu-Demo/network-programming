//
//  8_chapter_server.c
//  TCP&C-Demo
//
//  Created by sheng wang on 2019/11/12.
//  Copyright © 2019 feisu. All rights reserved.
//

#include "unp.h"
#include "unp.c"

int main (int argc, char ** argv)
{
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERV_PORT);
    
    bind(sockfd, (SA *)&server_addr, sizeof(server_addr));
    dg_echo(sockfd, (SA *)&client_addr, sizeof(client_addr));
}


//使用select同时处理tcp和udp连接
int
select_udp_tcp(int argc, char **argv)
{
    int                    listenfd, connfd, udpfd, nready, maxfdp1;
    char                mesg[MAXLINE];
    pid_t                childpid;
    fd_set                rset;
    ssize_t                n;
    socklen_t            len;
    const int            on = 1;
    struct sockaddr_in    cliaddr, servaddr;
    void                sig_chld(int);

        /* 4create listening TCP socket */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(SERV_PORT);

    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

    listen(listenfd, LISTENQ);

        /* 4create UDP socket */
    udpfd = socket(AF_INET, SOCK_DGRAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(SERV_PORT);

    bind(udpfd, (SA *) &servaddr, sizeof(servaddr));
/* end udpservselect01 */

/* include udpservselect02 */
    Signal(SIGCHLD, sig_chld);    /* must call waitpid() */

    FD_ZERO(&rset);
    maxfdp1 = max(listenfd, udpfd) + 1;
    for ( ; ; ) {
        FD_SET(listenfd, &rset);
        FD_SET(udpfd, &rset);
        if ( (nready = select(maxfdp1, &rset, NULL, NULL, NULL)) < 0) {
            if (errno == EINTR)
                continue;        /* back to for() */
            else
                err_sys("select error");
        }

        if (FD_ISSET(listenfd, &rset)) {
            len = sizeof(cliaddr);
            connfd = accept(listenfd, (SA *) &cliaddr, &len);
    
            if ( (childpid = fork()) == 0) {    /* child process */
                close(listenfd);    /* close listening socket */
                str_echo(connfd);    /* process the request */
                exit(0);
            }
            close(connfd);            /* parent closes connected socket */
        }

        if (FD_ISSET(udpfd, &rset)) {
            len = sizeof(cliaddr);
            n = recvfrom(udpfd, mesg, MAXLINE, 0, (SA *) &cliaddr, &len);

            sendto(udpfd, mesg, n, 0, (SA *) &cliaddr, len);
        }
    }
}
/* end udpservselect02 */
