//
//  30_chapter_server_6.c
//  TCP&C-Demo
//
//  Created by 西木柚子 on 2019/12/3.
//  Copyright © 2019 feisu. All rights reserved.
//

#include "unp.c"
#define    MAXN    16384        /* max # bytes client can request */

typedef struct {
  pthread_t        thread_tid;        /* thread ID */
  long            thread_count;    /* # connections handled */
} Thread;
Thread    *tptr;        /* array of Thread structures; calloc'ed */

#define    MAXNCLI    32
int                    clifd[MAXNCLI], iget, iput;
pthread_mutex_t        clifd_mutex;
pthread_cond_t        clifd_cond;
static int            nthreads;
pthread_mutex_t        clifd_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t        clifd_cond = PTHREAD_COND_INITIALIZER;



int
main(int argc, char **argv)
{
    int            i, listenfd = 0, connfd;
    void        sig_int(int), thread_make(int);
    socklen_t    addrlen = 0, clilen;
    struct sockaddr    *cliaddr;

    if (argc == 3)
        listenfd = tcp_listen(NULL, argv[1], &addrlen);
    else if (argc == 4)
        listenfd = tcp_listen(argv[1], argv[2], &addrlen);
    else
        err_quit("usage: serv08 [ <host> ] <port#> <#threads>");
    cliaddr = malloc(addrlen);

    nthreads = atoi(argv[argc-1]);
    tptr = calloc(nthreads, sizeof(Thread));
    iget = iput = 0;

        /* 4create all the threads */
    for (i = 0; i < nthreads; i++)
        thread_make(i);        /* only main thread returns */

    Signal(SIGINT, sig_int);

    for ( ; ; ) {
        clilen = addrlen;
        connfd = accept(listenfd, cliaddr, &clilen);

        pthread_mutex_lock(&clifd_mutex);
        clifd[iput] = connfd;
        if (++iput == MAXNCLI)
            iput = 0;
        /*
         input等于连接进入的client数目，iget等于线程池里面的线程数目（nthreads大小），二者相等，
         说明client数目大于线程池里面的线程数目，这个会导致新进入的client没法链接，故报错退出
         */
        if (iput == iget)
            err_quit("iput = iget = %d", iput);
        pthread_cond_signal(&clifd_cond);
        pthread_mutex_unlock(&clifd_mutex);
    }
}
/* end serv08 */

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
thread_make(int i)
{
    void    *thread_main(void *);

    pthread_create(&tptr[i].thread_tid, NULL, &thread_main, (void *) i);
    return;        /* main thread returns */
}

void *
thread_main(void *arg)
{
    int        connfd;
    void    web_child(int);

    printf("thread %d starting\n", (int) arg);
    for ( ; ; ) {
        pthread_mutex_lock(&clifd_mutex);
        while (iget == iput)
            pthread_cond_wait(&clifd_cond, &clifd_mutex);
        connfd = clifd[iget];    /* connected socket to service */
        if (++iget == MAXNCLI)
            iget = 0;
        pthread_mutex_unlock(&clifd_mutex);
        tptr[(int) arg].thread_count++;

        web_child(connfd);        /* process request */
        close(connfd);
    }
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
