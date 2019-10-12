//
//  client.c
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


void half_close_client(int argc, char* argv[])
{
    int sock;
    struct sockaddr_in server_address;
    char message[BUF_SIZE];
    FILE *readfp, *writefp;
    long str_len, recv_len, recv_cnt;
    
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
    
    
    //方法二：标准I/O函数
    readfp = fdopen(sock, "r");
    writefp = fdopen(sock, "w");
    while (1) {
        if (fgets(message, sizeof(message), readfp) == NULL) {
            break;
        }
        fputs(message, stdout);
        fflush(stdout);
    }
    fputs("from client: thank you \n", writefp);
    fflush(writefp);
    fclose(writefp);
    fclose(readfp);
}

void client (int argc, char* argv[])
{
    int sock;
    struct sockaddr_in server_address;
    char message[BUF_SIZE];
    FILE *readfp, *writefp;
    long str_len, recv_len, recv_cnt;
    
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
    
    //方法1：标准read、write的socket函数
    //    while (1) {
    //        fputs("input message (q to quit)", stdout);
    //        fgets(message, BUF_SIZE, stdin);
    //        if (!strcmp(message, "q\n")) {
    //            break;
    //        }
    //        recv_len = 0;
    //        str_len = write(sock, message, strlen(message));
    //
    //        while (recv_len < str_len) {
    //            recv_cnt = read(sock, &message[recv_len], BUF_SIZE-1);
    //            if (recv_cnt == -1) {
    //                error_handling("read() error");
    //            }
    //            recv_len += recv_cnt;
    //        }
    //        message[str_len] = 0;
    //        printf("message from server: %s \n", message );
    //    }
    //
    //    close(sock);
    
    //方法二：标准I/O函数
    readfp = fdopen(sock, "r");
    writefp = fdopen(sock, "w");
    while (1) {
        fputs("input message (q to quit)", stdout);
        fgets(message, BUF_SIZE, stdin);
        if (!strcmp(message, "q\n")) {
            break;
        }
        fputs(message, writefp);
        fflush(writefp);
        fgets(message, BUF_SIZE, readfp);
        printf("message from server: %s \n", message );
    }
    
    fclose(writefp);
    fclose(readfp);
}


int main (int argc, char* argv[])
{
    half_close_client(argc,argv);
    return 0;
}




void error_handling (char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
