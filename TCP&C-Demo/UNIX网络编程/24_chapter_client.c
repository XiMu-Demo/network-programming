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
static int        clientfd;
static int        nsec;            /* #seconds betweeen each alarm */
static int        maxnprobes;        /* #probes w/no response before quit */
static int        nprobes;        /* 从收到服务器应答之前进行了多少次alarm处理 */
static void sig_urg(int), sig_alrm(int);


static void
sig_urg(int signo)
{
    ssize_t        n;
    char    c;
 /*
SO_OOBINLINE套接字选项默认情况下是禁止的，对于这样的接收端套接字，该数据字节并不放入套接字接收缓冲区，而是被放入该连接的一个独立的单字节带外缓冲区。
  接收进程从这个单字节缓冲区读入数据的唯一办法就是指定MSG_OOB标志调用recv、recvfrom或recvmsg
  */
    if ( (n = recv(clientfd, &c, 1, MSG_OOB)) < 0) {
        if (errno != EWOULDBLOCK)
            err_sys("recv error");
    }
    printf("receive server URG MESSAGE: %c \n", c);
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
    send(clientfd, "3", 1, MSG_OOB);
    alarm(nsec);
    return;                    /* may interrupt client code */
}


static void tcp_connect_2(const char *addr) {
    struct sockaddr_in servaddr;

    clientfd = socket (AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    inet_pton(AF_INET, addr, &servaddr.sin_addr);

    connect(clientfd, (SA *) &servaddr, sizeof(servaddr));

}

//nsec_arg：多久（秒）探测一次，maxnprobes_arg：一共进行多少次探测
void
heartbeat_cli(int clientfd_arg, int nsec_arg, int maxnprobes_arg)
{
    clientfd = clientfd_arg;        /* set globals for signal handlers */
    if ( (nsec = nsec_arg) < 1)
        nsec = 1;
    if ( (maxnprobes = maxnprobes_arg) < nsec)
        maxnprobes = nsec;
    nprobes = 0;

    signal(SIGURG, sig_urg);
    fcntl(clientfd, F_SETOWN, getpid());

    signal(SIGALRM, sig_alrm);
    alarm(nsec);
}


void test_client_heartbeat(int argc, char **argv)
{
   if(argc != 2) {
       err_quit("%s <IPAddress>", argv[0]);
   }

   tcp_connect_2(argv[1]);

   heartbeat_cli(clientfd, 1, 5);

   for(;;) { }
}


//MARK:- 带外标记

void out_of_band_client(int argc, char **argv)
{
   int        sockfd;

    if (argc != 3)
        err_quit("usage: tcpsend04 <host> <port#>");

    sockfd = tcp_connect(argv[1], argv[2]);

    write(sockfd, "123", 3);
    printf("wrote 3 bytes of normal data\n");
    
    send(sockfd, "4", 1, MSG_OOB);
    printf("wrote 1 byte of OOB data\n");

    write(sockfd, "5", 1);
    printf("wrote 1 byte of normal data\n");


}

int
main(int argc, char **argv)
{
    out_of_band_client(argc, argv);
    exit(0);
}
