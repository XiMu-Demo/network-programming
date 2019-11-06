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


ssize_t
readline(int fd, void *vptr, size_t maxlen)
{
    ssize_t    n, rc;
    char    c, *ptr;

    ptr = vptr;
    for (n = 1; n < maxlen; n++) {
again:
        if ( (rc = read(fd, &c, 1)) == 1) {//每个字节都调用read，会导致该函数非常缓慢
            *ptr++ = c;
            if (c == '\n')
                break;    /* newline is stored, like fgets() */
        } else if (rc == 0) {//读到文件末尾了
            *ptr = 0;//末尾置零
            return(n - 1);    /* EOF, n - 1 bytes were read */
        } else {
            if (errno == EINTR)
                goto again;
            return(-1);        /* error, errno set by read() */
        }
    }

    *ptr = 0;    /* null terminate like fgets() */
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
