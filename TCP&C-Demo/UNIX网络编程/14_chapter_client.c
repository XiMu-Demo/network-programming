//
//  14_chapter_client.c
//  TCP&C-Demo
//
//  Created by sheng wang on 2019/11/15.
//  Copyright © 2019 feisu. All rights reserved.
//

#include "unp.c"


#define IPADDRESS   "127.0.0.1"


//信号处理只是简单的返回，是因为我们的Signal函数设置了SA_RESTART标志（可以继续执行之前被信号中断的逻辑），所以这里
//直接return，防止又回去接着执行connect逻辑
static void sig_alarm(int signo)
{
    return;
}


int connect_timeout(int sockfd, const SA* saptr, socklen_t salen, int nsec)
{
    Sigfunc *sigfunc;
    int     n;
    
    sigfunc = Signal(SIGALRM, sig_alarm);//Signal函数设置了SA_RESTART标志
    if (alarm(nsec) != 0) {
        err_msg("connect_timeout: alarm was already set");
    }
    
    //调用connect，如果返回负数，说明connect被SIGALRM系统调用中断，此时会返回EINTR，就把errno重置为ETIMEOUT
    if ((n = connect(sockfd, saptr, salen)) < 0) {
        close(sockfd);
        if (errno == EINTR) {
            errno = ETIMEDOUT;
        }
    }
    
    //关闭alarm
    alarm(0);
    //恢复原来被中断的信号处理函数（如果存在的话）
    Signal(SIGALRM, sigfunc);
    return n;
}

//如果recvfrom函数5s还没有收到对方的应答，就被SIGALRM系统信号中断，并输出“time out”
void udp_client_timeout(FILE *fp, int sockfd, const SA *pservaddr, socklen_t servlen)
{
    ssize_t    n;
    char    sendline[MAXLINE], recvline[MAXLINE + 1];

    Signal(SIGALRM, sig_alarm);

    while (fgets(sendline, MAXLINE, fp) != NULL) {

        sendto(sockfd, sendline, strlen(sendline), 0, pservaddr, servlen);

        alarm(5);
        if ( (n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL)) < 0) {
            if (errno == EINTR)
                fprintf(stderr, "socket timeout\n");
            else
                err_sys("recvfrom error");
        } else {
            alarm(0);
            recvline[n] = 0;    /* null terminate */
            fputs(recvline, stdout);
        }
    }
}

//select函数只会阻塞sec秒，一旦超过这个时间描述符还没有发生变化，该函数就会返回
int readable_timeo(int fd, int sec)
{
    fd_set            rset;
    struct timeval    tv;

    FD_ZERO(&rset);
    FD_SET(fd, &rset);

    tv.tv_sec = sec;
    tv.tv_usec = 0;

    return(select(fd+1, &rset, NULL, NULL, &tv));
        /* 4> 0 if descriptor is readable */
}

//使用select的timeout实现udp的recvfrom函数超时功能，只有在5s内socket发生了变化，才会调用recvfrom函数，否则就输出“timeout”
void udp_client_select_timeout(FILE *fp, int sockfd, const SA *pservaddr, socklen_t servlen)
{
    ssize_t    n;
    char    sendline[MAXLINE], recvline[MAXLINE + 1];

    while (fgets(sendline, MAXLINE, fp) != NULL) {

        sendto(sockfd, sendline, strlen(sendline), 0, pservaddr, servlen);

        if (readable_timeo(sockfd, 5) == 0) {//变化的fd数目为0，说明没有fd变化
            fprintf(stderr, "socket timeout\n");
        } else {
            n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL);
            recvline[n] = 0;    /* null terminate */
            fputs(recvline, stdout);
        }
    }
}

//使用socket option设置读写超时：SO_RCVTIMEO, SO_SNDTIMEO
void udp_client_socket_option_timeout(FILE *fp, int sockfd, const SA *pservaddr, socklen_t servlen)
{
    ssize_t                n;
    char            sendline[MAXLINE], recvline[MAXLINE + 1];
    struct timeval    tv;

    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while (fgets(sendline, MAXLINE, fp) != NULL) {
        sendto(sockfd, sendline, strlen(sendline), 0, pservaddr, servlen);
        n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL);
        if (n < 0) {
            if (errno == EWOULDBLOCK) {
                fprintf(stderr, "socket timeout\n");
                continue;
            } else
                err_sys("recvfrom error");
        }

        recvline[n] = 0;    /* null terminate */
        fputs(recvline, stdout);
    }
}



//测试poll client
static void handle_connection(int sockfd)
{
    char    sendline[MAXLINE],recvline[MAXLINE];
    struct pollfd pfds[2];
    ssize_t n;
    //添加连接描述符
    pfds[0].fd = sockfd;
    pfds[0].events = POLLIN;
    //添加标准输入描述符
    pfds[1].fd = STDIN_FILENO;
    pfds[1].events = POLLIN;
    for (; ;)
    {
        poll(pfds,2,-1);
        if (pfds[0].revents & POLLIN)
        {
            n = read(sockfd,recvline,MAXLINE);
            if (n == 0)
            {
                    fprintf(stderr,"client: server is closed.\n");
                    close(sockfd);
            }
            write(STDOUT_FILENO,recvline,n);
        }
        //测试标准输入是否准备好
        if (pfds[1].revents & POLLIN)
        {
            n = read(STDIN_FILENO,sendline,MAXLINE);
            if (n  == 0)
            {
                shutdown(sockfd,SHUT_WR);
        continue;
            }
            write(sockfd,sendline,n);
        }
    }
}

void poll_client()
{
    int                 sockfd;
    struct sockaddr_in  servaddr;
    sockfd = socket(AF_INET,SOCK_STREAM,0);
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    inet_pton(AF_INET,IPADDRESS,&servaddr.sin_addr);
    connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr));
    //处理连接描述符
    handle_connection(sockfd);
}

int main(int argc,char *argv[])
{
    poll_client();
    return 0;
}
