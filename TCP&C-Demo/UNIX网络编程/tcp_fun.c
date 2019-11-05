//
//  tcp_fun.c
//  TCP&C-Demo
//
//  Created by sheng wang on 2019/11/4.
//  Copyright © 2019 feisu. All rights reserved.
//

#include "unp.h"

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



int main(int argc, const char * argv[] )
{
    char buf[10];
    ssize_t n;
    while((n = read(0,buf,10)) > 0){//  海燕高尔基在苍茫的大海上狂风卷积
        write(1,buf,n);//从buf中输出n个字节的信息到标准输出中return 0;
    }
}
