//
//  24_chapter_server.c
//  TCP&C-Demo
//
//  Created by sheng wang on 2019/11/21.
//  Copyright © 2019 feisu. All rights reserved.
//

#include "unp.c"

int        listenfd, connfd;

void    sig_urg(int);

void select_oob_server(int argc, char **argv)
{
    int        listenfd = 0, connfd, justreadoob = 0;
    ssize_t     n;
    char    buff[100];
    fd_set    rset, xset;

    if (argc == 2)
        listenfd = tcp_listen(NULL, argv[1], NULL);
    else if (argc == 3)
        listenfd = tcp_listen(argv[1], argv[2], NULL);
    else
        err_quit("usage: tcprecv03 [ <host> ] <port#>");

    connfd = accept(listenfd, NULL, NULL);

    FD_ZERO(&rset);
    FD_ZERO(&xset);
    for ( ; ; ) {
        FD_SET(connfd, &rset);
        if (justreadoob == 0)
            FD_SET(connfd, &xset);

        select(connfd + 1, &rset, NULL, &xset, NULL);

        if (FD_ISSET(connfd, &xset)) {
            n = recv(connfd, buff, sizeof(buff)-1, MSG_OOB);
            buff[n] = 0;        /* null terminate */
            printf("read %zd OOB byte: %s\n", n, buff);
            justreadoob = 1;
            FD_CLR(connfd, &xset);
        }

        if (FD_ISSET(connfd, &rset)) {
            if ( (n = read(connfd, buff, sizeof(buff)-1)) == 0) {
                printf("received EOF\n");
                exit(0);
            }
            buff[n] = 0;    /* null terminate */
            printf("read %zd bytes: %s\n", n, buff);
            justreadoob = 0;
        }
    }
}


void tcp_oob_server(int argc, char **argv)
{
    ssize_t        n;
       char    buff[100];

       if (argc == 2)
           listenfd = tcp_listen(NULL, argv[1], NULL);
       else if (argc == 3)
           listenfd = tcp_listen(argv[1], argv[2], NULL);
       else
           err_quit("usage: tcprecv01 [ <host> ] <port#>");

       connfd = accept(listenfd, NULL, NULL);

       Signal(SIGURG, sig_urg);
       fcntl(connfd, F_SETOWN, getpid());

       for ( ; ; ) {
           if ( (n = read(connfd, buff, sizeof(buff)-1)) == 0) {
               printf("received EOF\n");
               exit(0);
           }
           buff[n] = 0;    /* null terminate */
           printf("read %zd bytes: %s\n", n, buff);
       }
}

void
sig_urg(int signo)
{
    ssize_t        n;
    char    buff[100];

    printf("SIGURG received\n");
    n = recv(connfd, buff, sizeof(buff)-1, MSG_OOB);
    buff[n] = 0;        /* null terminate */
    printf("read %zd OOB byte: %s\n", n, buff);
}


//MARK:- 服务端心跳
static int    servfd1;
static int    nsec;            /* #seconds between each alarm */
static int    maxnalarms;        /* #alarms w/no client probe before quit */
static int    nprobes;        /* #alarms since last client probe */
static void    server_sig_urg(int), server_sig_alrm(int);

void
heartbeat_serv(int servfd_arg, int nsec_arg, int maxnalarms_arg)
{
    servfd1 = servfd_arg;        /* set globals for signal handlers */
    if ( (nsec = nsec_arg) < 1)
        nsec = 1;
    if ( (maxnalarms = maxnalarms_arg) < nsec)
        maxnalarms = nsec;

    Signal(SIGURG, server_sig_urg);
    fcntl(servfd1, F_SETOWN, getpid());

    Signal(SIGALRM, server_sig_alrm);
    alarm(nsec);
}

static void
server_sig_urg(int signo)
{
    ssize_t        n;
    char    c;
    if ( (n = recv(servfd1, &c, 1, MSG_OOB)) < 0) {
        if (errno != EWOULDBLOCK)
            err_sys("recv error");
    }
    printf("RECEIVE client URG MESSAGE: %c \n", c);
    send(servfd1, "1", 1, MSG_OOB);    /* echo back out-of-band byte */

    nprobes = 0;            /* reset counter */
    return;                    /* may interrupt server code */
}

static void
server_sig_alrm(int signo)
{
    printf("server_sig_alrm \n");

    if (++nprobes > maxnalarms) {
        printf("no probes from client\n");
        exit(0);
    }
    alarm(nsec);
    return;                    /* may interrupt server code */
}

void server_str_echo(int sockfd)
{
    ssize_t n;
    char buf[MAXLINE];

again:
    heartbeat_serv(sockfd, 1, 5);
    while ((n = read(sockfd, buf, MAXLINE)) > 0) {
        writen(sockfd, buf, n);
        if (n < 0 && errno == EINTR) {
            goto again;
        }
        else if (n < 0 ){
            printf("str_echo: read error");
        }
    }
}


void test_server_heartbeat()
{
    int listenfd, connfd;
     pid_t childpid;
     socklen_t childlen;
     struct sockaddr_in childaddr, serveraddr;
     
     listenfd = socket(AF_INET, SOCK_STREAM, 0);
     bzero(&serveraddr, sizeof(serveraddr));
     serveraddr.sin_family = AF_INET;
     serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
     serveraddr.sin_port = htons(SERV_PORT);
    
     if (bind(listenfd, (SA *)&serveraddr, sizeof(serveraddr)) == -1 ){
         printf("bind error");
     }
     if (listen(listenfd, LISTENQ) == -1){
         printf("listen error");
     }
     
     while (1) {
         childlen = sizeof(childaddr);
         if ( (connfd = accept(listenfd, (SA *)&childaddr, &childlen)) < 0) {
             if (errno == EINTR) {
                 continue;
             }else{
                 printf("accpet error");
             }
         }
         printf("client connect \n");
         if ((childpid = fork()) == 0) {
             printf("child process forked \n");
             close(listenfd);
             server_str_echo(connfd);
//             close(connfd);
             exit(0);
         }
         close(connfd);
     }
}

int
main(int argc, char **argv)
{
    test_server_heartbeat();
}
