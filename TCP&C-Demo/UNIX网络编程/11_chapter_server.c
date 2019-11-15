//
//  11_chapter_server.c
//  TCP&C-Demo
//
//  Created by sheng wang on 2019/11/15.
//  Copyright © 2019 feisu. All rights reserved.
//

#include "unp.c"


void udp_time_server1()
{
    int sockfd;
     struct sockaddr_in server_addr, client_addr;
     ssize_t     n;
     socklen_t clientlen;
     time_t ticks;
     char msg[MAXLINE];

     sockfd = socket(AF_INET, SOCK_DGRAM, 0);
     bzero(&server_addr, sizeof(server_addr));
     server_addr.sin_family = AF_INET;
     server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
     server_addr.sin_port = htons(SERV_PORT);
     
     bind(sockfd, (SA *)&server_addr, sizeof(server_addr));
    
     while (1) {
         n = recvfrom(sockfd, msg, MAXLINE, 0, (SA*)&client_addr, &clientlen);
         ticks = time(NULL);
         snprintf(msg, MAXLINE, "%.24s\r\n", ctime(&ticks));
         printf("send message to client:%s \n",msg);
         sendto(sockfd, msg, MAXLINE, 0, (SA*)&client_addr, clientlen);
     }
}


void
udp_time_serve2(int argc, char **argv)
{
    int                sockfd = 0;
    ssize_t            n;
    char               buff[MAXLINE];
    time_t             ticks;
    socklen_t          len;
    struct sockaddr_storage    cliaddr;

    if (argc == 2)
        sockfd = udp_server(NULL, argv[1], NULL);
    else if (argc == 3)
        sockfd = udp_server(argv[1], argv[2], NULL);
    else
        err_quit("usage: daytimeudpsrv [ <host> ] <service or port>");

    for ( ; ; ) {
        len = sizeof(cliaddr);
        n = recvfrom(sockfd, buff, MAXLINE, 0, (SA *)&cliaddr, &len);
        printf("datagram from %s\n", sock_ntop((SA *)&cliaddr, len));

        ticks = time(NULL);
        snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
        sendto(sockfd, buff, strlen(buff), 0, (SA *)&cliaddr, len);
    }
}


int main(int argc, char **argv)
{
    udp_time_serve2(argc, argv);
    exit(EXIT_SUCCESS);
}
