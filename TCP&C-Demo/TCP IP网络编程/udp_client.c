//
//  udp_client.c
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

int main(int argc, char* argv[])
{
    
    int client_sock = 0;
    char message[BUF_SIZE];
    char server_message[BUF_SIZE];
    long str_len;
//    socklen_t server_address_size;
    struct sockaddr_in server_address;
    
    if (argc!= 3) {
        printf("usage: %s <port> \n", argv[0]);
        exit(EXIT_SUCCESS);
    }
    
    client_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (client_sock == -1) {
        error_handling("UDP socket creation error");
    }
    
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(atoi(argv[2]));
    
    connect(client_sock, (struct sockaddr*)&server_address, sizeof(server_address));
    
    while (1) {
        fputs("insert message (q to quit)", stdout);
        fgets(message, sizeof(message), stdin);
        if (!strcmp(message, "q\n")) {
            break;
        }
//        sendto(client_sock, message, strlen(message), 0, (struct sockaddr*)&server_address, sizeof(server_address));
//        server_address_size = sizeof(server_address);
//        str_len = recvfrom(client_sock, server_message, BUF_SIZE, 0, (struct sockaddr*)&server_address, &server_address_size);
//        server_message[str_len] = 0;
        printf("message prepare to send:%s", message);
        write(client_sock, message, strlen(message));
        str_len = read(client_sock, server_message, sizeof(message)-1);
        server_message[str_len] = 0;
        printf("message from server:%s", server_message);
    }
    
    close(client_sock);
    return 0;
    
}


void error_handling (char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
