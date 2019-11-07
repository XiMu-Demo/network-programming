//
//  5-chapter_client.c
//  TCP&C-Demo
//
//  Created by sheng wang on 2019/11/6.
//  Copyright © 2019 feisu. All rights reserved.
//

#include "unp.h"
#include "unp.c"

int main(int argc, char ** argv)
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
    
    Signal(SIGPIPE, sig_chld);
    Signal(EPIPE, sig_chld);

    connect(sockfd, (SA *)&serveraddr, sizeof(serveraddr));
    str_cli(stdin, sockfd);
    exit(0);
}
