//
//  8_chapter_client.c
//  TCP&C-Demo
//
//  Created by sheng wang on 2019/11/12.
//  Copyright © 2019 feisu. All rights reserved.
//

#include "unp.h"
#include "unp.c"
#include <sys/poll.h>

int main (int argc, char ** argv)
{
    int sockfd;
    struct sockaddr_in server_addr;
    
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERV_PORT);
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    dg_cli_connect(stdin, sockfd, (SA *)&server_addr, sizeof(server_addr));

    exit(EXIT_SUCCESS);
}
