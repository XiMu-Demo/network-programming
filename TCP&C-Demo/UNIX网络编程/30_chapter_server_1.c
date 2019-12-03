//
//  30_chapter_server_1.c
//  TCP&C-Demo
//
//  Created by 西木柚子 on 2019/12/3.
//  Copyright © 2019 feisu. All rights reserved.
//

//MARK:- tcp并发服务器，为每个客户生成一个子进程
#include    <sys/resource.h>
#include "unp.c"


#ifndef    HAVE_GETRUSAGE_PROTO
int        getrusage(int, struct rusage *);
#endif

void
pr_cpu_time(void)
{
    double            user, sys;
    struct rusage    myusage, childusage;

    if (getrusage(RUSAGE_SELF, &myusage) < 0)
        err_sys("getrusage error");
    if (getrusage(RUSAGE_CHILDREN, &childusage) < 0)
        err_sys("getrusage error");

    user = (double) myusage.ru_utime.tv_sec +
                    myusage.ru_utime.tv_usec/1000000.0;
    user += (double) childusage.ru_utime.tv_sec +
                     childusage.ru_utime.tv_usec/1000000.0;
    sys = (double) myusage.ru_stime.tv_sec +
                   myusage.ru_stime.tv_usec/1000000.0;
    sys += (double) childusage.ru_stime.tv_sec +
                    childusage.ru_stime.tv_usec/1000000.0;

    printf("\nuser time = %g, sys time = %g\n", user, sys);
}


int
main(int argc, char **argv)
{
    int                    listenfd = 0, connfd;
    pid_t                childpid;
    void                sig_chld(int), sig_int(int), web_child(int);
    socklen_t            clilen, addrlen = 0;
    struct sockaddr        *cliaddr;

    if (argc == 2)
        listenfd = tcp_listen(NULL, argv[1], &addrlen);
    else if (argc == 3)
        listenfd = tcp_listen(argv[1], argv[2], &addrlen);
    else
        err_quit("usage: serv01 [ <host> ] <port#>");
    cliaddr = malloc(addrlen);

    Signal(SIGCHLD, sig_chld);
    Signal(SIGINT, sig_int);

    for ( ; ; ) {
        clilen = addrlen;
        if ( (connfd = accept(listenfd, cliaddr, &clilen)) < 0) {
            if (errno == EINTR)
                continue;        /* back to for() */
            else
                err_sys("accept error");
        }

        if ( (childpid = fork()) == 0) {    /* child process */
            close(listenfd);    /* close listening socket */
            web_child(connfd);    /* process request */
            exit(0);
        }
        close(connfd);            /* parent closes connected socket */
    }
}

void
sig_int(int signo)
{
    void    pr_cpu_time(void);

    pr_cpu_time();
    exit(0);
}


#define    MAXN    16384        /* max # bytes client can request */
void
web_child(int sockfd)
{
    long            ntowrite;
    ssize_t        nread;
    char        line[MAXLINE], result[MAXN];

    for ( ; ; ) {
        if ( (nread = Readline(sockfd, line, MAXLINE)) == 0)
            return;        /* connection closed by other end */

            /* 4line from client specifies #bytes to write back */
        ntowrite = atol(line);
        if ((ntowrite <= 0) || (ntowrite > MAXN))
            err_quit("client request for %d bytes", ntowrite);

        writen(sockfd, result, ntowrite);
    }
}
