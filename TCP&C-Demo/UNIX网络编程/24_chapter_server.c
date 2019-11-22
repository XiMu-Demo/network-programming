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

        /* 若产生异常事件，则读取带外数据 */
        if (FD_ISSET(connfd, &xset)) {
            n = recv(connfd, buff, sizeof(buff)-1, MSG_OOB);
            buff[n] = 0;        /* null terminate */
            printf("read %zd OOB byte: %s\n", n, buff);
            justreadoob = 1; /* 防止多次读取带外数据 */
            FD_CLR(connfd, &xset);
        }

        /* 从套接字读取普通数据 */
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
static int    servfd;
static int    nsec;            /* #seconds between each alarm */
static int    maxnalarms;        /* #alarms w/no client probe before quit */
static int    nprobes;        /* #alarms since last client probe */
static void server_sig_urg(int),  server_sig_alrm(int);

static void
 server_sig_urg(int signo)
{
    char c;
    if(recv(connfd, &c, 1, MSG_OOB) < 0) {
       if(errno != EWOULDBLOCK) {
           err_sys("recv error");
       }
    }
    printf("receive client URG MESSAGE: %c \n", c);
    send(connfd, "2", 1, MSG_OOB);
    nprobes = 0;
    return ;          /* may interrupt server code */
}

static void
 server_sig_alrm(int signo)
{
    if (++nprobes > maxnalarms) {
        printf("no probes from client\n");
        exit(0);
    }
    alarm(nsec);
    return;                    /* may interrupt server code */
}


static int Accept_1(int fd, struct sockaddr * sa, socklen_t * salenptr) {
    int n;

    while((n = accept(fd, sa, salenptr)) < 0) {
        if(errno == EINTR || errno == ECONNABORTED) {
            continue;
        } else {
            err_sys("accept error");
        }
    }
    return n;
}



static int tcp_listen_1(void) {
    int listenfd;
    struct sockaddr_in servaddr;

    listenfd = socket (AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

    listen(listenfd, LISTENQ);

    return listenfd;
}

void
heartbeat_serv(int servfd_arg, int nsec_arg, int maxnalarms_arg)
{
    servfd = servfd_arg;        /* set globals for signal handlers */
    if ( (nsec = nsec_arg) < 1)
        nsec = 1;
    if ( (maxnalarms = maxnalarms_arg) < nsec)
        maxnalarms = nsec;

    signal(SIGURG,  server_sig_urg);
    fcntl(connfd, F_SETOWN, getpid());

    signal(SIGALRM,  server_sig_alrm);
    alarm(nsec);
    
}


void test_server_heartbeat()
{
    int listenfd;
    ssize_t n;
    char buff[100];
    
    listenfd = tcp_listen_1();
    connfd = Accept_1(listenfd, NULL, NULL);
    heartbeat_serv(servfd, 1, 5);

    for ( ; ; ) {
       if ( (n = read(connfd, buff, sizeof(buff)-1)) == 0) {
           printf("received EOF\n");
           exit(0);
       }
       buff[n] = 0;    /* null terminate */
       printf("read %zd bytes: %s\n", n, buff);
   }
}

//MARK:- 带外标记

/*
 服务端输出如下：
 read 3 bytes: 123
 at OOB mark
 read 2 bytes: 45
 received EOF
 
 首先server sleep(5)保证了接下来的read操作之前数据都已经接受完毕了，虽然要求读取buf[100]的字节，但是因为client分三次发送数据，中间的4是带外数据，所以先读取了123，接着再一次读取剩下的45。
 也就是说读操作会在遇到带外标记的时候停止
 */
void out_of_band_server(int argc, char **argv)
{
    int        listenfd = 0, connfd, on=1;
    ssize_t     n;
    char    buff[100];

    if (argc == 2)
        listenfd = tcp_listen(NULL, argv[1], NULL);
    else if (argc == 3)
        listenfd = tcp_listen(argv[1], argv[2], NULL);
    else
        err_quit("usage: tcprecv04 [ <host> ] <port#>");

    setsockopt(listenfd, SOL_SOCKET, SO_OOBINLINE, &on, sizeof(on));//因为我们的客户端在普通数据流中发送的带外数据，所以server需要开启SO_OOBINLINE选项，才可以在普通数据流中读取带外数据

    connfd = accept(listenfd, NULL, NULL);
    sleep(5);//保证接下来的读取操作之前client发送过来的数据都全部接受完毕了

    for ( ; ; ) {
        if (sockatmark(connfd))
            printf("at OOB mark\n");

        if ( (n = read(connfd, buff, sizeof(buff)-1)) == 0) {
            printf("received EOF\n");
            exit(0);
        }
        buff[n] = 0;    /* null terminate */
        printf("read %zd bytes: %s\n", n, buff);
    }
}


int
main(int argc, char **argv)
{
    out_of_band_server(argc, argv);
}
