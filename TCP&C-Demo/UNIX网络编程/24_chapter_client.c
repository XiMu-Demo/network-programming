//
//  25_chapter_client.c
//  TCP&C-Demo
//
//  Created by sheng wang on 2019/11/21.
//  Copyright © 2019 feisu. All rights reserved.
//

#include "unp.c"


/*
 服务端输出如下：
 read 3 bytes: 123
 SIGURG received
 read 1 OOB byte: 5 （客户端发送的45的带外数据，但是只有一位被oob读取出来，而且可以看到5比4优先读取)
 read 1 bytes: 4
 read 2 bytes: 67
 SIGURG received
 read 1 OOB byte: 8
 read 2 bytes: 90
 received EOF
 
 */

void tcp_oob_client(int argc, char **argv)
{
    int        sockfd;

      if (argc != 3)
          err_quit("usage: tcpsend01 <host> <port#>");

      sockfd = tcp_connect(argv[1], argv[2]);

      write(sockfd, "123", 3);
      printf("wrote 3 bytes of normal data\n");
      sleep(1);

      send(sockfd, "45", 2, MSG_OOB);
      printf("wrote 1 byte of OOB data\n");
      sleep(1);

      write(sockfd, "67", 2);
      printf("wrote 2 bytes of normal data\n");
      sleep(1);

      send(sockfd, "8", 1, MSG_OOB);
      printf("wrote 1 byte of OOB data\n");
      sleep(1);

      write(sockfd, "90", 2);
      printf("wrote 2 bytes of normal data\n");
      sleep(1);
}


//MARK:- 客户端心跳
static int        servfd;
static int        nsec;            /* #seconds betweeen each alarm */
static int        maxnprobes;        /* #probes w/no response before quit */
static int        nprobes;        /* 从收到服务器应答之前进行了多少次alarm处理 */
static void    sig_urg(int), sig_alrm(int);

//nsec_arg：多久（秒）探测一次，maxnprobes_arg：一共进行多少次探测
void
heartbeat_cli(int servfd_arg, int nsec_arg, int maxnprobes_arg)
{
    servfd = servfd_arg;        /* set globals for signal handlers */
    if ( (nsec = nsec_arg) < 1)
        nsec = 1;
    if ( (maxnprobes = maxnprobes_arg) < nsec)
        maxnprobes = nsec;
    nprobes = 0;

    Signal(SIGURG, sig_urg);
    fcntl(servfd, F_SETOWN, getpid());

    Signal(SIGALRM, sig_alrm);
    alarm(nsec);
}

static void
sig_urg(int signo)
{
    ssize_t        n;
    char    c;
 
    if ( (n = recv(servfd, &c, 1, MSG_OOB)) < 0) {
        if (errno != EWOULDBLOCK)
            err_sys("recv error");
    }
    printf("RECEIVE server URG MESSAGE: %c \n", c);
    nprobes = 0;            /* reset counter */
    return;                    /* may interrupt client code */
}

static void
sig_alrm(int signo)
{
    if (++nprobes > maxnprobes) {
        fprintf(stderr, "server is unreachable\n");
        exit(0);
    }
    printf("client sig_alrm \n");
    send(servfd, "1", 1, MSG_OOB);
    alarm(nsec);
    return;                    /* may interrupt client code */
}


void client_str_cli(FILE *fp, int sockfd)
{
    int maxfd, nready, stdineof = 0;
    fd_set rset;
    char buf[MAXLINE];
    size_t n;
    
    
    FD_ZERO(&rset);
    heartbeat_cli(sockfd, 1, 5);
    while (1) {
        if (stdineof == 0) {
            FD_SET(fileno(fp), &rset);
        }
        FD_SET(sockfd, &rset);
        maxfd = max(fileno(fp), sockfd) + 1;
        if ( (nready = select(maxfd, &rset, NULL, NULL, NULL)) < 0) {
            if (errno == EINTR)
                continue;        /* back to for() */
            else
                err_sys("select error");
        }
        
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

void test_client_heartbeat(int argc, char **argv)
{
    int sockfd;
       struct sockaddr_in serveraddr;
       
       if (argc != 2) {
           printf("usage: tcpcli <ipaddress>");
       }
       
       sockfd = socket(AF_INET, SOCK_STREAM, 0);
       bzero(&serveraddr, sizeof(serveraddr));
       serveraddr.sin_family = AF_INET;
       serveraddr.sin_port = htons(SERV_PORT);
       inet_pton(AF_INET, argv[1], &serveraddr.sin_addr);

       connect(sockfd, (SA *)&serveraddr, sizeof(serveraddr));
       client_str_cli(stdin, sockfd);
       exit(0);
}



int
main(int argc, char **argv)
{
    test_client_heartbeat(argc, argv);
    exit(0);
}
