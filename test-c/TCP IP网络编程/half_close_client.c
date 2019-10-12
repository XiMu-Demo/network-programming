//
//  half_close_client.c
//  test-c
//
//  Created by sheng wang on 2019/9/27.
//  Copyright © 2019 feisu. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>


#define BUF_SIZE 1024
static void error_handling (char *message);


int main6 (int argc, char* argv[])
{
    int sock;
    FILE *fp;
    struct sockaddr_in server_address;
    char buf[BUF_SIZE];
    long read_count;
    
    if (argc != 3) {
        printf("Usage: %s <ip> <port> \n", argv[0]);
        exit(1);
    }
    
    fp = fopen("/Users/shengwang/学习/test-c/test-c/File1", "a+");
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        error_handling("socket() error");
    }
    
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(argv[1]);
    server_address.sin_port = htons(atoi(argv[2]));
    
    if (connect(sock, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        error_handling("connect() error");
    }else{
        puts("connected......");
    }
    
    while((read_count = read(sock, buf, BUF_SIZE)) != 0) {
        fwrite((void *)buf, 1, read_count, fp);
    }
    
    puts("received file data");
    write(sock, "thank you", 10);
    fclose(fp);
    close(sock);
    return 0;
}




void error_handling (char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

