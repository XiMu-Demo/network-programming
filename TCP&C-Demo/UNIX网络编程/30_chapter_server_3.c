//
//  30_chapter.c
//  TCP&C-Demo
//
//  Created by 西木柚子 on 2019/12/3.
//  Copyright © 2019 feisu. All rights reserved.
//
#include "unp.c"
#include    <sys/resource.h>
#include    <sys/mman.h>

#define    MAXN    16384        /* max # bytes client can request */

//MARK:- 预先派生子进程池，accpet使用线程上锁
static int        nchildren;
static pid_t    *pids;


static pthread_mutex_t    *mptr;    /* actual mutex will be in shared memory */

void
my_lock_init()
{
    int        fd;
    pthread_mutexattr_t    mattr;

    fd = Open("/dev/zero", O_RDWR, 0);

    mptr = Mmap(0, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE,
                MAP_SHARED, fd, 0);
    close(fd);

    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(mptr, &mattr);
}


void
my_lock_wait()
{
    pthread_mutex_lock(mptr);
}

void
my_lock_release()
{
    pthread_mutex_unlock(mptr);
}



void
sig_int(int signo)
{
    int        i;
    void    pr_cpu_time(void);

        /* 4terminate all children */
    for (i = 0; i < nchildren; i++)
        kill(pids[i], SIGTERM);
    while (wait(NULL) > 0)        /* wait for all children */
        ;
    if (errno != ECHILD)
        err_sys("wait error");

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


pid_t
child_make(int i, int listenfd, int addrlen)
{
    pid_t    pid;
    void    child_main(int, int, int);

    if ( (pid = fork()) > 0){
        printf("pid:%d", pid);
        return(pid);        /*只返回父进程 */
    }else{
        child_main(i, listenfd, addrlen);    /* never returns */

    }
    return pid;
}


void
child_main(int i, int listenfd, int addrlen)
{
    int                connfd;
    void            web_child(int);
    socklen_t        clilen;
    struct sockaddr    *cliaddr;

    cliaddr = malloc(addrlen);

    printf("child %ld starting\n", (long) getpid());
    for ( ; ; ) {
        clilen = addrlen;
        my_lock_wait();
        connfd = accept(listenfd, cliaddr, &clilen);
        my_lock_release();
        web_child(connfd);        /* process the request */
        close(connfd);
    }
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


int
main(int argc, char **argv)
{

    int            listenfd = 0, i;
    socklen_t    addrlen = 0;
    void        sig_int(int);
    pid_t        child_make(int, int, int);

    if (argc == 3)
        listenfd = tcp_listen(NULL, argv[1], &addrlen);
    else if (argc == 4)
        listenfd = tcp_listen(argv[1], argv[2], &addrlen);
    else
        err_quit("usage: serv02 [ <host> ] <port#> <#children>");
    nchildren = atoi(argv[argc-1]);
    pids = calloc(nchildren, sizeof(pid_t));
    
    my_lock_init();
    for (i = 0; i < nchildren; i++)
        pids[i] = child_make(i, listenfd, addrlen);    /* parent returns */

    Signal(SIGINT, sig_int);

    for ( ; ; )
        pause();    /* everything done by children */
}
