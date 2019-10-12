//
//  thread_client.c
//  test-c
//
//  Created by sheng wang on 2019/10/10.
//  Copyright © 2019 feisu. All rights reserved.
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 1024

static void error_handling (char *message);

void* send_msg(void* arg)
{
    int sock = *((int*)arg);
    char message[BUF_SIZE];
    while (1) {
        fgets(message, BUF_SIZE, stdin);
        if (!strcmp(message, "q\n")) {
            close(sock);
            exit(EXIT_SUCCESS);
        }
        write(sock, message, strlen(message));
    }
    return NULL;
}

void* receive_msg(void* arg)
{
    int sock = *((int *)arg);
    char name_msg[BUF_SIZE];
    long str_len;
    while (1) {
        str_len = read(sock, name_msg, BUF_SIZE - 1);
        if (str_len == -1) {
            return (void*)-1;
        }
        name_msg[str_len] = 0;
        printf("message from server: %s", name_msg);
    }
    return NULL;
}

void client (int argc, char* argv[])
{
    int sock;
    struct sockaddr_in server_address;
    pthread_t send_thread, receive_thread;
    void* thread_return;
    
    if (argc != 3) {
        printf("Usage: %s <ip> <port> \n", argv[0]);
        exit(1);
    }
    
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
    
    pthread_create(&send_thread, NULL, send_msg, (void*)&sock);
    pthread_create(&receive_thread, NULL, receive_msg, (void*)&sock);
    pthread_join(send_thread, &thread_return);
    pthread_join(receive_thread, &thread_return);
    close(sock);
    
}


int main (int argc, char* argv[])
{
    client(argc,argv);
    return 0;
}




void error_handling (char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
