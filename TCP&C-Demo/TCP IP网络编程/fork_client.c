//
//  client.c
//  C_DEMO
//
//  Created by 西木柚子 on 2019/10/3.
//  Copyright © 2019 西木柚子. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>


#define BUF_SIZE 1000
static void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void read_routine(int sock ,char *buf)
{
    while (1) {
        long str_len = read(sock, buf, BUF_SIZE);
        if (str_len == 0) {
            return;
        }
        buf[str_len] = 0;
        printf("message from server:%s", buf);
    }
}

void write_routine(int sock, char *buf)
{
    while (1) {
        fgets(buf, BUF_SIZE, stdin);
        if (!strcmp(buf, "q\n")) {
            shutdown(sock, SHUT_WR);
        }
        write(sock, buf, strlen(buf));
    }
}

void txt()
{
   
}

void split_read_wirte_client (int argc, char *argv[])
{
    
    int sock;
    struct sockaddr_in server_address;
    char message[30];
    long str_len ,revc_len, revc_cnt;
    
    if (argc != 3) {
        printf("usage: %s <IP> <port> \n", argv[0]);
        exit(EXIT_SUCCESS);
    }
    
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        error_handling("socket() error");
    }
    
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(argv[1]);
    server_address.sin_port = htons(atoi(argv[2]));
    
    if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        error_handling("connect() error");
    }
    
    pid_t pid = fork();
    if (pid == 0) {
        write_routine(sock, message);
    }
    else{
        read_routine(sock, message);
    }
    
    close(sock);
}





static void client(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in server_address;
    char message[30];
    long str_len ,revc_len, revc_cnt;
    
    if (argc != 3) {
        printf("usage: %s <IP> <port> \n", argv[0]);
        exit(EXIT_SUCCESS);
    }
    
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        error_handling("socket() error");
    }
    
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(argv[1]);
    server_address.sin_port = htons(atoi(argv[2]));
    
    if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        error_handling("connect() error");
    }
    
    
    
    while (1) {
        fputs("input message (q to quit): ", stdout);
        fgets(message,BUF_SIZE , stdin);
        
        if (!strcmp(message, "q\n")) {
            break;
        }
        
        str_len = write(sock, message, strlen(message));
        revc_len = 0;
        
        while (revc_len < str_len) {
            revc_cnt = read(sock, &message[revc_len], BUF_SIZE-1);
            if (revc_cnt == -1) {
                error_handling("read error");
            }
            revc_len += revc_cnt;
        }
        message[revc_len] = 0;
        printf("message from server:%s", message);
        
    }
    close(sock);
}

int main(int argc, char *argv[])
{
    split_read_wirte_client(argc, argv);
    return 0;
}
