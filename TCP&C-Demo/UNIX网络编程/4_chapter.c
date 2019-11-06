//
//  4_chapter.c
//  TCP&C-Demo
//
//  Created by sheng wang on 2019/11/5.
//  Copyright © 2019 feisu. All rights reserved.
//

#include "unp.h"
#include "unp.c"


int main(int argc , char **argv)
{
    int listenfd, connfd;
    socklen_t len;
    struct sockaddr_in serveraddr, clientaddr;
    char buff[MAXLINE];
    time_t ticks;
    
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(13);
    
    bind(listenfd, (SA *)&serveraddr, sizeof(serveraddr));
    listen(listenfd, LISTENQ);
    

    while (1) {
        len = sizeof(clientaddr);
        connfd = accept(listenfd, (SA *)&clientaddr, &len);
        const char *ip = inet_ntop(AF_INET, &clientaddr.sin_addr, buff, sizeof(buff));
        int port = ntohs(clientaddr.sin_port);
        printf("connect from %s, port %d \n", ip, port);
        
        ticks = time(NULL);
        snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
        write(connfd, buff, strlen(buff));
        
        close(connfd);
    }
    
}
