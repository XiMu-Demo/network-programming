//
//  udp_server.c
//  test-c
//
//  Created by sheng wang on 2019/9/26.
//  Copyright © 2019 feisu. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1000
static void error_handling (char *message);


int main (int argc, char* argv[])
{
    int server_sock = 0;
    char message[BUF_SIZE];
    char receive_message[BUF_SIZE];

    long str_len;
    socklen_t client_address_size;
    struct sockaddr_in server_address, client_address;
    
    if (argc!= 2) {
        printf("usage: %s <port> \n", argv[0]);
        exit(EXIT_SUCCESS);
    }
    
    server_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (server_sock == -1) {
        error_handling("UDP socket creation error");
    }
    
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(atoi(argv[1]));
    
    if (bind(server_sock, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        error_handling("bind () error");
    }
    
    while (1) {
        client_address_size = sizeof(client_address);
        str_len = recvfrom(server_sock, receive_message, BUF_SIZE, 0, (struct sockaddr*)&client_address, &client_address_size);
        printf("message from client: %s \n", receive_message );
        sendto(server_sock, message, str_len, 0, (struct sockaddr*)&client_address, client_address_size);
    }
    
    close(server_sock);
    return 0;
    
}


void error_handling (char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
