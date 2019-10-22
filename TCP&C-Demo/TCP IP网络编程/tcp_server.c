//
//  server.c
//  test-c
//
//  Created by sheng wang on 2019/9/24.
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

//标准I/O的流分离下实现半关闭，P263
void dup_split_server(int argc, char *argv[])
{
    int server_socket;
    int client_socket;
    FILE *readfp, *writefp;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    socklen_t client_address_size;
    int str_len, i, client_str_len;
    char message[BUF_SIZE];
    char client_message[BUF_SIZE];
    
    if (argc != 2) {
        printf("Usage: %s <port> \n", argv[0]);
        exit(1);
    }
    
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
        printf("connected client \n");
    }
    
    //方式2
    readfp = fdopen(client_socket, "r");
    //dup复制文件描述符，因为readfp和writefp公用一个fd，关闭其中一个都会导致另外一个一起关闭
    writefp = fdopen(dup(client_socket) , "w");
    fputs("frome server: hi~ client \n", writefp);
    fputs("i love all of the world \n", writefp);
    fflush(writefp);
    
    shutdown(fileno(writefp), SHUT_WR);
    fclose(writefp);

    fgets(message, sizeof(message), readfp);
    fputs(message, stdout);
    fclose(readfp);
}


void server(int argc, char *argv[])
{
    int server_socket;
    int client_socket;
    FILE *readfp, *writefp;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    socklen_t client_address_size;
    int str_len, i, client_str_len;
    char message[BUF_SIZE];
    char client_message[BUF_SIZE];

    if (argc != 2) {
        printf("Usage: %s <port> \n", argv[0]);
        exit(1);
    }

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

    //方式1：合并I/O
//    while ((str_len = read(client_socket, message, BUF_SIZE) != 0)) {
//        printf("message from client: %s \n", message );
//        write(client_socket, message, BUF_SIZE);
//    }
//    close(client_socket);
//    close(server_socket);


    //方式2, 标准I/O分离流
    readfp = fdopen(client_socket, "r");
    writefp = fdopen(client_socket , "w");
    while (!feof(readfp)) {
        fgets(message, BUF_SIZE, readfp);
        fputs(message, writefp);
        fflush(writefp);
    }
    fclose(readfp);
    fclose(writefp);
}

int  main12211(int argc, char *argv[])
{
    dup_split_server(argc, argv);

    return 0;
}


void error_handling (char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
