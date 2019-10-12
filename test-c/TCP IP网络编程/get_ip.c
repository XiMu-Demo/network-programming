//
//  get_ip.c
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
#include <netdb.h>



#define BUF_SIZE 1024
static void error_handling (char *message);
void get_ip_by_name(int argc, char *argv[]);
void get_name_by_ip(int argc, char *argv[]);



int  main(int argc, char *argv[])
{
    get_name_by_ip(argc, argv);
    return 0;
}

void get_ip_by_name(int argc, char *argv[])
{
    int i;
    struct hostent *host;
    
    if (argc != 2) {
        printf("Usage: %s <addr>\n", argv[0]);
        exit(EXIT_SUCCESS);
    }
    
    host = gethostbyname(argv[1]);
    if (!host) {
        error_handling("gethost error");
    }
    
    printf("official name: %s\n", host->h_name);
    
    for (i = 0; host->h_aliases[i]; i++) {
        printf("aliases %d :%s \n", i+1, host->h_aliases[i]);
    }
    for (i = 0; host->h_addr_list[i]; i++) {
        printf("ip address %d :%s \n", i+1, inet_ntoa(*(struct in_addr*)host -> h_addr_list[i]));
    }
}

void get_name_by_ip(int argc, char *argv[])
{
    int i;
    struct hostent *host;
    struct sockaddr_in address;
    if (argc != 2) {
        printf("Usage: %s <addr>\n", argv[0]);
        exit(EXIT_SUCCESS);
    }
    
    memset(&address, 0, sizeof(address));
    address.sin_addr.s_addr = inet_addr(argv[1]);
    host = gethostbyaddr((char *)&address.sin_addr, 4, AF_INET);
    if (!host) {
        error_handling("gethost error");
    }
    
    printf("official name: %s\n", host->h_name);
    
    for (i = 0; host->h_aliases[i]; i++) {
        printf("aliases %d :%s \n", i+1, host->h_aliases[i]);
    }
    for (i = 0; host->h_addr_list[i]; i++) {
        printf("ip address %d :%s \n", i+1, inet_ntoa(*(struct in_addr*)host -> h_addr_list[i]));
    }
    
}



void error_handling (char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
