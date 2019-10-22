//
//  day_time.c
//  test-c
//
//  Created by sheng wang on 2019/10/12.
//  Copyright © 2019 feisu. All rights reserved.
//

#include <stdio.h>
#include    "unp.h"

int
main112111(int argc, char *argv[])
{
    int                    sockfd;
    long                    n;
    char                recvline[MAXLINE + 1];
    struct sockaddr_in    servaddr;
    
    printf("xxxxxxx");

    if (argc != 2){
        puts("usage: a.out <IPaddress>");
        return 0;
    }

    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        puts("socket error");
        return 0;
    }


    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(13);    /* daytime server */
    printf("inet_pton  for %s--%d", argv[1], servaddr.sin_port);

    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)//把argv[1]的字符串ip地址转换为二进制地址赋值给servaddr.sin_addr
    {
        printf("inet_pton error for %s--%d", argv[1], servaddr.sin_addr.s_addr);
        return 0;
    }

    if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0){
        puts("connect error");
        return 0;
    }


    while ( (n = read(sockfd, recvline, MAXLINE)) > 0) {
        recvline[n] = 0;    /* null terminate */
        if (fputs(recvline, stdout) == EOF){
            puts("fputs error");
            return 0;
        }
    }
    if (n < 0){
        puts("read error");
        return 0;
    }

    
    return 0;
}
