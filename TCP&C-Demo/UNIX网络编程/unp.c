//
//  unp.c
//  TCP&C-Demo
//
//  Created by sheng wang on 2019/11/5.
//  Copyright © 2019 feisu. All rights reserved.
//

#include    "unp.h"
#include    <stdarg.h>
#include    <errno.h>        /* for definition of errno */
#include    <stdarg.h>        /* ANSI C header file */
//#include    "ourhdr.h"



//MARK: - read/write

ssize_t                        /* Read "n" bytes from a descriptor. */
readn(int fd, void *vptr, size_t n)
{
    size_t    nleft;
    ssize_t    nread;
    char    *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ( (nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR)
                nread = 0;        /* and call read() again */
            else
                return(-1);
        } else if (nread == 0)
            break;                /* EOF */

        nleft -= nread;
        ptr   += nread;
    }
    return(n - nleft);        /* return >= 0 */
}


ssize_t                        /* Write "n" bytes to a descriptor. */
writen(int fd, const void *vptr, size_t n)
{
    size_t        nleft;
    ssize_t        nwritten;
    const char    *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0;        /* and call write() again */
            else
                return(-1);            /* error */
        }

        nleft -= nwritten;
        ptr   += nwritten;
    }
    return(n);
}


//MARK: read 版本1

static ssize_t    read_cnt = 0;
static char    *read_ptr;
static char    read_buf[MAXLINE];

static ssize_t _my_read(int fd, char *ptr)
{

    if (read_cnt <= 0) {
    again:
        if ( (read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0){
            if (errno == EINTR) {
                goto  again;
            }
            return (-1);
        }
        else if (read_cnt == 0){
            return 0;
        }
        read_ptr = read_buf;//把read_ptr赋值到read_buf的第一个char字符所在的指针
    }
    read_cnt--;
    /*ptr内容指向read_buf的第一个字符，虽然每次都读取了MAXLINE个字符，但是实际上只把第一个字符传出去。
    这里就是该函数的意图所在，以为如果直接用read函数，那么read函数的缓冲区对其他函数来说就是个黑匣子，此处通过静态变量暴露出去的read_buf
     让其他函数可以知道此事缓冲区内部的内容
     */
    return(1);
    *ptr = *read_ptr++ ;
}


ssize_t _readline(int fd, void *vptr, size_t maxlen)
{
    int        n;
    ssize_t rc;
    char    c, *ptr;

    ptr = vptr;
    for (n = 1; n < maxlen; n++) {
//        if ( (rc = _my_read(fd, &c)) == 1) {
        if ( (rc = read(fd, &c, 1)) == 1) {
            *ptr++ = c;//每次用_my_read函数读取一个char c，然后存入ptr，也就是vptr里面，然后指针右移，以便存入下一个char c
            if (c == '\n')
                break;
        } else if (rc == 0) {
            *ptr = 0;
            printf("读取到了文件末尾EOF \n");
            return(n - 1);    /* EOF, n - 1 bytes were read */
        } else{
            printf("readline error \n");
            return(-1);    /* error */
        }
    }

    *ptr = 0;
    return(n);
}


ssize_t Readline(int fd, void *ptr, size_t maxlen)
{
    ssize_t        n;
    n = _readline(fd, ptr, maxlen);
    return(n);
}


//MARK: - TCP FUN
void ip_address_convert(void)
{
    char IPdotdec[20]; //存放点分十进制IP地址
    struct in_addr s; // IPv4地址结构体
    // 输入IP地址
    printf("Please input IP address: ");
    scanf("%s", IPdotdec);
    // 转换
    inet_pton(AF_INET, IPdotdec, (void *)&s);
    printf("inet_pton: 0x%x\n", s.s_addr); // 注意得到的字节序
    // 反转换
    inet_ntop(AF_INET, (void *)&s, IPdotdec, INET_ADDRSTRLEN);
    printf("inet_ntop: %s\n", IPdotdec);
}


char *
sock_ntop(const struct sockaddr *sa, socklen_t salen)
{
    char        portstr[8];
    static char str[128];        /* Unix domain is largest */

    switch (sa->sa_family) {
    case AF_INET: {
        struct sockaddr_in    *sin = (struct sockaddr_in *) sa;

        if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL)
            return(NULL);
        if (ntohs(sin->sin_port) != 0) {
            snprintf(portstr, sizeof(portstr), ":%d", ntohs(sin->sin_port));
            strcat(str, portstr);
        }
        return(str);
    }
/* end sock_ntop */

#ifdef    IPV6
    case AF_INET6: {
        struct sockaddr_in6    *sin6 = (struct sockaddr_in6 *) sa;

        str[0] = '[';
        if (inet_ntop(AF_INET6, &sin6->sin6_addr, str + 1, sizeof(str) - 1) == NULL)
            return(NULL);
        if (ntohs(sin6->sin6_port) != 0) {
            snprintf(portstr, sizeof(portstr), "]:%d", ntohs(sin6->sin6_port));
            strcat(str, portstr);
            return(str);
        }
        return (str + 1);
    }
#endif

#ifdef    AF_UNIX
    case AF_UNIX: {
        struct sockaddr_un    *unp = (struct sockaddr_un *) sa;

            /* OK to have no pathname bound to the socket: happens on
               every connect() unless client calls bind() first. */
        if (unp->sun_path[0] == 0)
            strcpy(str, "(no pathname bound)");
        else
            snprintf(str, sizeof(str), "%s", unp->sun_path);
        return(str);
    }
#endif

#ifdef    HAVE_SOCKADDR_DL_STRUCT
    case AF_LINK: {
        struct sockaddr_dl    *sdl = (struct sockaddr_dl *) sa;

        if (sdl->sdl_nlen > 0)
            snprintf(str, sizeof(str), "%*s (index %d)",
                     sdl->sdl_nlen, &sdl->sdl_data[0], sdl->sdl_index);
        else
            snprintf(str, sizeof(str), "AF_LINK, index=%d", sdl->sdl_index);
        return(str);
    }
#endif
    default:
        snprintf(str, sizeof(str), "sock_ntop: unknown AF_xxx: %d, len %d",
                 sa->sa_family, salen);
        return(str);
    }
    return (NULL);
}



void sock_set_wild(struct sockaddr *sa, socklen_t salen)
{
    const void    *wildptr;
    switch (sa->sa_family) {
    case AF_INET: {
        static struct in_addr    in4addr_any;
        in4addr_any.s_addr = htonl(INADDR_ANY);
        wildptr = &in4addr_any;
        break;
    }

#ifdef    IPV6
    case AF_INET6: {
        wildptr = &in6addr_any;
        break;
    }
#endif

    default:
        return;
    }
    sock_set_addr(sa, salen, wildptr);
    return;
}


void sock_set_addr(struct sockaddr *sa, socklen_t salen, const void *addr)
{
    switch (sa->sa_family) {
    case AF_INET: {
        struct sockaddr_in    *sin = (struct sockaddr_in *) sa;
        memcpy(&sin->sin_addr, addr, sizeof(struct in_addr));
        return;
    }

#ifdef    IPV6
    case AF_INET6: {
        struct sockaddr_in6    *sin6 = (struct sockaddr_in6 *) sa;
        memcpy(&sin6->sin6_addr, addr, sizeof(struct in6_addr));
        return;
    }
#endif
    }
    return;
}


void str_echo(int sockfd)
{
    ssize_t n;
    char buf[MAXLINE];
    
again:
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


void str_cli(FILE *fp, int sockfd)
{
    char sendline[MAXLINE], recvline[MAXLINE];
    
    while (fgets(sendline, MAXLINE, fp) != NULL) {
        printf("client input：%s", sendline);
        writen(sockfd, sendline, strlen(sendline));
        if (Readline(sockfd, recvline, MAXLINE) == 0) {
            printf("str_cli: server terminated prematurely \n");
        }
        printf("message from server: %s \n", recvline);
    }
}

void str_cli_select(FILE* fp, int sockfd)
{
    int maxfd, stdineof = 0;
    fd_set rset;
    char buf[MAXLINE];
    size_t n;
    
    
    FD_ZERO(&rset);
    while (1) {
        if (stdineof == 0) {
            FD_SET(fileno(fp), &rset);
        }
        FD_SET(sockfd, &rset);
        maxfd = max(fileno(fp), sockfd) + 1;
        select(maxfd, &rset, NULL, NULL, NULL);
        
        if (FD_ISSET(sockfd, &rset)) {
            if ((n = read(sockfd, buf, MAXLINE)) == 0) {
                //标准输入已经读取完毕，然后发送了shutdown，设置标志位stdineof
                //如果此时在socket上面也读到了EOF，并且此时stdineof标志位为1，那么说明从服务器接收到了全部的自己发送出去的数据
                //如果socket读到了EOF，但是stdineof不为1，那说明本地文件还没读取完，远程服务器就过早关闭了
                if (stdineof == 1 ){
                    return;
                }
                else {
                    printf("str_cli: server terminated prematurely \n");
                    exit(EXIT_SUCCESS);
                }
            }
                write(fileno(fp), buf, n);
        }
        
        if (FD_ISSET(fileno(fp), &rset)) {
            if ((n = read(fileno(fp), buf, MAXLINE)) == 0) {
                stdineof = 1;
                shutdown(sockfd, SHUT_WR);
                FD_CLR(fileno(fp), &rset);
                continue;
            }
            write(sockfd, buf, n);
        }
    }
}

//MARK: 基于域名和服务的tcp
int
tcp_listen(const char *host, const char *serv, socklen_t *addrlenp)
{
    int                listenfd, n;
    const int        on = 1;
    struct addrinfo    hints, *res, *ressave;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
        err_quit("tcp_listen error for %s, %s: %s",
                 host, serv, gai_strerror(n));
    ressave = res;

    do {
        listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (listenfd < 0)
            continue;        /* error, try next one */

        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
        if (bind(listenfd, res->ai_addr, res->ai_addrlen) == 0)
            break;            /* success */

        close(listenfd);    /* bind error, close and try next one */
    } while ( (res = res->ai_next) != NULL);

    if (res == NULL)    /* errno from final socket() or bind() */
        err_sys("tcp_listen error for %s, %s", host, serv);

    listen(listenfd, LISTENQ);

    if (addrlenp)
        *addrlenp = res->ai_addrlen;    /* return size of protocol address */

    freeaddrinfo(ressave);

    return(listenfd);
}



int
tcp_connect(const char *host, const char *serv)
{
    int                sockfd, n;
    struct addrinfo    hints, *res, *ressave;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
        err_quit("tcp_connect error for %s, %s: %s",
                 host, serv, gai_strerror(n));
    ressave = res;

    do {
        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd < 0)
            continue;    /* ignore this one */

        if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0)
            break;        /* success */

        close(sockfd);    /* ignore this one */
    } while ( (res = res->ai_next) != NULL);

    if (res == NULL)    /* errno set from final connect() */
        err_sys("tcp_connect error for %s, %s", host, serv);

    freeaddrinfo(ressave);

    return(sockfd);
}




//MARK: - UDP FUN

//服务端
void dg_echo(int sockfd, SA* pcliaddr, socklen_t clientlen)
{
    ssize_t     n;
    socklen_t len;
    char msg[MAXLINE];
    
    while (1) {
        len = clientlen;
        n = recvfrom(sockfd, msg, MAXLINE, 0, pcliaddr, &len);
        printf("message frome client: %s", msg);
        sendto(sockfd, msg, n, 0, pcliaddr, len);
    }
}

//客户端
void
dg_cli(FILE *fp, int sockfd, const SA *pservaddr, socklen_t servlen)
{
    ssize_t                n;
    char            sendline[MAXLINE], recvline[MAXLINE + 1];
    socklen_t        len;
    struct sockaddr    *preply_addr;

    preply_addr = malloc(servlen);

    while (fgets(sendline, MAXLINE, fp) != NULL) {

        sendto(sockfd, sendline, strlen(sendline), 0, pservaddr, servlen);

        len = servlen;
        n = recvfrom(sockfd, recvline, MAXLINE, 0, preply_addr, &len);
        if (len != servlen || memcmp(sock_ntop(pservaddr, len), sock_ntop(preply_addr, len), len) != 0) {
            printf("reply from %s (ignored)\n",
                    sock_ntop(preply_addr, len));
            continue;
        }

        recvline[n] = 0;    /* null terminate */
        printf("message frome server: %s", recvline);
    }
}

//udp客户端使用connect连接，这样该连接的错误就可以被返回
void
dg_cli_connect(FILE *fp, int sockfd, const SA *pservaddr, socklen_t servlen)
{
    ssize_t        n;
    char    sendline[MAXLINE], recvline[MAXLINE + 1];

    connect(sockfd, (SA *) pservaddr, servlen);

    while (fgets(sendline, MAXLINE, fp) != NULL) {
        write(sockfd, sendline, strlen(sendline));
        n = read(sockfd, recvline, MAXLINE);
        recvline[n] = 0;    /* null terminate */
        fputs(recvline, stdout);
    }
}

//使用getaddrinfo来使用域名和服务进行udp连接
int
udp_client(const char *host, const char *serv, SA **saptr, socklen_t *lenp)
{
    int                sockfd, n;
    struct addrinfo    hints, *res, *ressave;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
        err_quit("udp_client error for %s, %s: %s",
                 host, serv, gai_strerror(n));
    ressave = res;

    do {
        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd >= 0)
            break;        /* success */
    } while ( (res = res->ai_next) != NULL);

    if (res == NULL)    /* errno set from final socket() */
        err_sys("udp_client error for %s, %s", host, serv);

    *saptr = malloc(res->ai_addrlen);
    memcpy(*saptr, res->ai_addr, res->ai_addrlen);
    *lenp = res->ai_addrlen;

    freeaddrinfo(ressave);

    return(sockfd);
}

int
udp_connect_client(const char *host, const char *serv)
{
    int                sockfd, n;
    struct addrinfo    hints, *res, *ressave;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
        err_quit("udp_connect error for %s, %s: %s",
                 host, serv, gai_strerror(n));
    ressave = res;

    do {
        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd < 0)
            continue;    /* ignore this one */

        if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0)
            break;        /* success */

        close(sockfd);    /* ignore this one */
    } while ( (res = res->ai_next) != NULL);

    if (res == NULL)    /* errno set from final connect() */
        err_sys("udp_connect error for %s, %s", host, serv);

    freeaddrinfo(ressave);

    return(sockfd);
}

int
udp_server(const char *host, const char *serv, socklen_t *addrlenp)
{
    int                sockfd, n;
    struct addrinfo    hints, *res, *ressave;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
        err_quit("udp_server error for %s, %s: %s",
                 host, serv, gai_strerror(n));
    ressave = res;

    do {
        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd < 0)
            continue;        /* error - try next one */

        if (bind(sockfd, res->ai_addr, res->ai_addrlen) == 0)
            break;            /* success */

        close(sockfd);        /* bind error - close and try next one */
    } while ( (res = res->ai_next) != NULL);

    if (res == NULL)    /* errno from final socket() or bind() */
        err_sys("udp_server error for %s, %s", host, serv);

    if (addrlenp)
        *addrlenp = res->ai_addrlen;    /* return size of protocol address */

    freeaddrinfo(ressave);

    return(sockfd);
}




//MARK: - 信号处理

Sigfunc* Signal(int signo, Sigfunc* func)
{
    struct sigaction act, oldact;
    act.sa_handler = func;
    //sa_mask置空，意味着除了被捕获的信号之外的信号都不会被阻断
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    
    if (signo == SIGALRM) {
#ifdef SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;
#endif
    }
    else{
        /*
         一些IO系统调用执行时, 如 read 等待输入期间, 如果收到一个信号,系统将中断read, 转而执行信号处理函数. 当信号处理返回后, 系统遇到了一个问题:
         是重新开始这个系统调用, 还是让系统调用失败?早期UNIX系统的做法是, 中断系统调用, 并让系统调用失败, 比如read返回 -1, 同时设置 errno 为
         EINTR, 但是中断了的系统调用是没有完成的调用, 它的失败是临时性的, 如果再次调用则可能成功, 这并不是真正的失败, 所以要对这种情况进行处理:
         我们可以从信号的角度来解决这个问题,  安装信号的时候, 设置 SA_RESTART属性, 那么当信号处理函数返回后, 被该信号中断的系统调用将自动恢复.
         */
#ifdef SA_RESTART
        act.sa_flags |= SA_RESTART;
#endif
    }
    //oldact保存信号中断之前的设置的信号处理函数的指针，也就是act的信息
    if (sigaction(signo, &act, &oldact) < 0) {
        return SIG_ERR;
    }
    return oldact.sa_handler;
    
}


void sig_chld(int signo)
{
    pid_t    pid;
    int        stat;

    while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0) {
        printf("child %d terminated\n", pid);
    }
    return;
}


//MARK:- ERROR
int        daemon_proc;        /* set nonzero by daemon_init() */

static void    err_doit(int, int, const char *, va_list);

/* Nonfatal error related to system call
 * Print message and return */

void
err_ret(const char *fmt, ...)
{
    va_list        ap;

    va_start(ap, fmt);
    err_doit(1, LOG_INFO, fmt, ap);
    va_end(ap);
    return;
}

/* Fatal error related to system call
 * Print message and terminate */

void
err_sys(const char *fmt, ...)
{
    va_list        ap;

    va_start(ap, fmt);
    err_doit(1, LOG_ERR, fmt, ap);
    va_end(ap);
    exit(1);
}

/* Fatal error related to system call
 * Print message, dump core, and terminate */

void
err_dump(const char *fmt, ...)
{
    va_list        ap;

    va_start(ap, fmt);
    err_doit(1, LOG_ERR, fmt, ap);
    va_end(ap);
    abort();        /* dump core and terminate */
    exit(1);        /* shouldn't get here */
}

/* Nonfatal error unrelated to system call
 * Print message and return */

void
err_msg(const char *fmt, ...)
{
    va_list        ap;

    va_start(ap, fmt);
    err_doit(0, LOG_INFO, fmt, ap);
    va_end(ap);
    return;
}

/* Fatal error unrelated to system call
 * Print message and terminate */

void
err_quit(const char *fmt, ...)
{
    va_list        ap;

    va_start(ap, fmt);
    err_doit(0, LOG_ERR, fmt, ap);
    va_end(ap);
    exit(1);
}

/* Print message and return to caller
 * Caller specifies "errnoflag" and "level" */

static void
err_doit(int errnoflag, int level, const char *fmt, va_list ap)
{
    int        errno_save;
    long        n;
    char    buf[MAXLINE + 1];

    errno_save = errno;        /* value caller might want printed */
#ifdef    HAVE_VSNPRINTF
    vsnprintf(buf, MAXLINE, fmt, ap);    /* safe */
#else
    vsprintf(buf, fmt, ap);                    /* not safe */
#endif
    n = strlen(buf);
    if (errnoflag)
        snprintf(buf + n, MAXLINE - n, ": %s", strerror(errno_save));
    strcat(buf, "\n");

    if (daemon_proc) {
        syslog(level, "%s", buf);
    } else {
        fflush(stdout);        /* in case stdout and stderr are the same */
        fputs(buf, stderr);
        fflush(stderr);
    }
    return;
}

//MARK:- host name 转换
struct addrinfo *
Host_serv(const char *host, const char *serv, int family, int socktype)
{
    int                n;
    struct addrinfo    hints, *res;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_flags = AI_CANONNAME;    /* always return canonical name */
    hints.ai_family = family;        /* 0, AF_INET, AF_INET6, etc. */
    hints.ai_socktype = socktype;    /* 0, SOCK_STREAM, SOCK_DGRAM, etc. */

    if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
        err_quit("host_serv error for %s, %s: %s",
                 (host == NULL) ? "(no hostname)" : host,
                 (serv == NULL) ? "(no service name)" : serv,
                 gai_strerror(n));

    return(res);    /* return pointer to first on linked list */
}
