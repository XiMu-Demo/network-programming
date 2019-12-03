//
//  30_chapter_server_5.c
//  TCP&C-Demo
//
//  Created by 西木柚子 on 2019/12/3.
//  Copyright © 2019 feisu. All rights reserved.
//

#include    "unp.c"
#define    MAXN    16384        /* max # bytes client can request */

//MARK:- TCP并发服务器，预先分配线程池

typedef struct {
  pthread_t        thread_tid;        /* thread ID */
  long            thread_count;    /* # connections handled */
} Thread;
Thread    *tptr;        /* array of Thread structures; calloc'ed */

int                listenfd, nthreads;
socklen_t        addrlen;
pthread_mutex_t    mlock1;

void
thread_make(int i)
{
    void    *thread_main(void *);

    pthread_create(&tptr[i].thread_tid, NULL, &thread_main, (void *) i);
    return;        /* main thread returns */
}

void *
thread_main(void *arg)
{
    int                connfd;
    void            web_child(int);
    socklen_t        clilen;
    struct sockaddr    *cliaddr;

    cliaddr = malloc(addrlen);

    printf("thread %d starting\n", (int) arg);
    for ( ; ; ) {
        clilen = addrlen;
        pthread_mutex_lock(&mlock1);
        connfd = accept(listenfd, cliaddr, &clilen);
        pthread_mutex_unlock(&mlock1);
        tptr[(int) arg].thread_count++;

        web_child(connfd);        /* process request */
        close(connfd);
    }
}


int
main(int argc, char **argv)
{
    int        i;
    void    sig_int(int), thread_make(int);

    if (argc == 3)
        listenfd = tcp_listen(NULL, argv[1], &addrlen);
    else if (argc == 4)
        listenfd = tcp_listen(argv[1], argv[2], &addrlen);
    else
        err_quit("usage: serv07 [ <host> ] <port#> <#threads>");
    nthreads = atoi(argv[argc-1]);
    tptr = calloc(nthreads, sizeof(Thread));

    for (i = 0; i < nthreads; i++)
        thread_make(i);            /* only main thread returns */

    Signal(SIGINT, sig_int);

    for ( ; ; )
        pause();    /* everything done by threads */
}
/* end serv07 */

void
sig_int(int signo)
{
    int        i;
    void    pr_cpu_time(void);

    pr_cpu_time();

    for (i = 0; i < nthreads; i++)
        printf("thread %d, %ld connections\n", i, tptr[i].thread_count);

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
