//
//  11_chapter_client.c
//  TCP&C-Demo
//
//  Created by sheng wang on 2019/11/15.
//  Copyright © 2019 feisu. All rights reserved.
//

#include "unp.c"




/*
 使用域名和服务名来进行tcp连接到server
1、在/etc/hosts加入：127.0.0.1 server
2、在/etc/services加入：daytime  9877/udp
3、在终端输入./client server daytime，即可得到正确输入，server端编译1_chapter_server
*/
void tcp_connect_time_client(int argc, char ** argv)
{
    int                sockfd;
    ssize_t n;
    char            recvline[MAXLINE + 1];
    socklen_t        len;
    struct sockaddr_storage    ss;

    if (argc != 3)
        err_quit("usage: daytimetcpcli <hostname/IPaddress> <service/port#>");

    sockfd = tcp_connect(argv[1], argv[2]);

    len = sizeof(ss);
    getpeername(sockfd, (SA *)&ss, &len);
    printf("connected to %s\n", sock_ntop((SA *)&ss, len));

    while ( (n = read(sockfd, recvline, MAXLINE)) > 0) {
        recvline[n] = 0;    /* null terminate */
        fputs(recvline, stdout);
    }
}


//使用域名和服务名通过udp连接到server，server端编译1_chapter_server得到
void udp_time_client(int argc, char ** argv)
{
    int                sockfd;
    ssize_t n;
    char            recvline[MAXLINE + 1];
    socklen_t        salen;
    struct sockaddr    *sa;

    if (argc != 3)
        err_quit("usage: daytimeudpcli1 <hostname/IPaddress> <service/port#>");

    sockfd = udp_client(argv[1], argv[2], (SA **) &sa, &salen);

    printf("sending to %s\n",sock_ntop(sa, salen));

    sendto(sockfd, "", 1, 0, sa, salen);    /* send 1-byte datagram */
    n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL);
    recvline[n] = '\0';    /* null terminate */
    printf("message frome server: %s", recvline);

}




int
main(int argc, char **argv)
{
    udp_time_client(argc, argv);
    exit(0);
}
