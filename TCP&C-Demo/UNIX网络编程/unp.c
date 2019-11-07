//
//  unp.c
//  TCP&C-Demo
//
//  Created by sheng wang on 2019/11/5.
//  Copyright © 2019 feisu. All rights reserved.
//

#include "unp.h"
#include <stdarg.h>


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


char *sock_ntop(const struct sockaddr *sa, socklen_t salen)
{
    char portstr[8];
    static char ipstr[128];
    
    switch (sa->sa_family) {
        case AF_INET:{
            struct  sockaddr_in *sin = (struct sockaddr_in*)sa;
            if (inet_ntop(AF_INET, &sin->sin_addr, ipstr, INET_ADDRSTRLEN) == NULL) {
                return NULL;
            }
            if (ntohs(sin->sin_port) != 0) {
                snprintf(portstr, sizeof(portstr), ":%d", ntohs(sin->sin_port));
                strcat(ipstr, portstr);
            }
            return ipstr;
        }
    }
    return NULL;
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
         EINTR中断了的系统调用是没有完成的调用, 它的失败是临时性的, 如果再次调用则可能成功, 这并不是真正的失败, 所以要对这种情况进行处理:
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
