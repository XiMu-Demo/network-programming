//
//  5-chapter.c
//  TCP&C-Demo
//
//  Created by sheng wang on 2019/11/6.
//  Copyright © 2019 feisu. All rights reserved.
//

#include "unp.h"
#include "unp.c"

int main()
{
    int listenfd, connfd;
    pid_t childpid;
    socklen_t childlen;
    struct sockaddr_in childaddr, serveraddr;
    
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(SERV_PORT);
   
    if (bind(listenfd, (SA *)&serveraddr, sizeof(serveraddr)) == -1 ){
        printf("bind error");
    }
    if (listen(listenfd, LISTENQ) == -1){
        printf("listen error");
    }
    //防止子进程僵死
    Signal(SIGCHLD, sig_chld);
    
    while (1) {
        childlen = sizeof(childaddr);
        if ( (connfd = accept(listenfd, (SA *)&childaddr, &childlen)) < 0) {
            if (errno == EINTR) {
                continue;
            }else{
                printf("accpet error");
            }
        }
        printf("client connect \n");
        if ((childpid = fork()) == 0) {
            printf("child process forked \n");
            close(listenfd);
            str_echo(connfd);
            close(connfd);
            exit(0);
        }
        close(connfd);
    }
    
}
