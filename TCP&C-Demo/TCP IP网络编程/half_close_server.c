//
//  half_close_server.c
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


int  main5(int argc, char *argv[])
{
    int server_socket, client_socket;
    FILE *fp;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_size;
    char buffer[BUF_SIZE];
    char client_message[BUF_SIZE];
    long read_count;
    
    if (argc != 2) {
        printf("Usage: %s <port> \n", argv[0]);
        exit(1);
    }
    
    fp = fopen("/Users/shengwang/学习/test-c/test-c/File", "r");

    server_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        error_handling("socket() error");
    }
    
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(atoi(argv[1]));
    
    if (bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address)) == -1) {
        error_handling("bind() error");
    }
    
    if (listen(server_socket, 5) == -1) {
        error_handling("listen() error");
    }
    
    client_address_size = sizeof(client_address);
    client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_size);
    if (client_socket == -1) {
        error_handling("accpet() error");
    }else{
        printf("connected client");
    }
    
    while (1) {
        //从fp读取BUF_SIZE个元素容到buffer，每个元素大小为1字节
        read_count = fread((void*)buffer, 1, BUF_SIZE, fp);
        if (read_count < BUF_SIZE) {
            write(client_socket, buffer, read_count);
            break;
        }
        write(client_socket, buffer, BUF_SIZE);
    }
    
    //半关闭服务端连接，客户端连接此时并没有关闭
    shutdown(client_socket, SHUT_WR);
    //读取客户端发送的数据
    read(client_socket, client_message, BUF_SIZE);
    printf("message from client :%s \n", client_message);
    
    fclose(fp);
    close(client_socket);
    close(server_socket);
    return 0;
}


void error_handling (char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}


