/* include serv06 */
#include    "unp.c"
#define    MAXN    16384        /* max # bytes client can request */

//MARK:- TCP并发服务器，每个客户端一个线程
int
main(int argc, char **argv)
{
    int                listenfd = 0, connfd;
    void            sig_int(int);
    void            *doit(void *);
    pthread_t        tid;
    socklen_t        clilen, addrlen = 0;
    struct sockaddr    *cliaddr;

    if (argc == 2)
        listenfd = tcp_listen(NULL, argv[1], &addrlen);
    else if (argc == 3)
        listenfd = tcp_listen(argv[1], argv[2], &addrlen);
    else
        err_quit("usage: serv06 [ <host> ] <port#>");
    cliaddr = malloc(addrlen);

    Signal(SIGINT, sig_int);

    for ( ; ; ) {
        clilen = addrlen;
        connfd = accept(listenfd, cliaddr, &clilen);

        pthread_create(&tid, NULL, &doit, (void *) connfd);
    }
}

void *
doit(void *arg)
{
    void    web_child(int);

    pthread_detach(pthread_self());
    web_child((int) arg);
    close((int) arg);
    return(NULL);
}
/* end serv06 */

void
sig_int(int signo)
{
    void    pr_cpu_time(void);

    pr_cpu_time();
    exit(0);
}

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
