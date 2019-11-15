//
//  1_chapter_server.c
//  TCP&C-Demo
//
//  Created by sheng wang on 2019/11/13.
//  Copyright © 2019 feisu. All rights reserved.
//

#include "unp.c"

int
main(int argc, char **argv)
{
    int                    listenfd, connfd;
    struct sockaddr_in    servaddr;
    char                buff[MAXLINE];
    time_t                ticks;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(SERV_PORT);    /* daytime server */

    bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

    listen(listenfd, LISTENQ);

    for ( ; ; ) {
        connfd = accept(listenfd, (SA *) NULL, NULL);

        ticks = time(NULL);
        snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
        write(connfd, buff, strlen(buff));

        close(connfd);
    }
}

