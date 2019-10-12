//
//  select.c
//  test-c
//
//  Created by sheng wang on 2019/10/8.
//  Copyright © 2019 feisu. All rights reserved.
//

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>


#define BUF_SIZE 30

void select_server(int argc, char *argv[])
{
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    struct timeval timeout;
    fd_set reads, copy_reads;
    socklen_t address_length;
    int fd_max, fd_num ,i;
    long str_length;
    char buf[BUF_SIZE];
    
    server_socket = socket(PF_INET, SOCK_STREAM, 0);
    printf("server socket: %d \n", server_socket);
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(atoi(argv[1]));
    
    if (bind((server_socket), (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        puts("bind() error \n");
    }
    if (listen(server_socket, 5) == -1) {
        puts("listen() error \n");
    }
    
    FD_ZERO(&reads);
    FD_SET(server_socket, &reads);
    fd_max = server_socket;
    
    while (1) {
        copy_reads = reads;
        timeout.tv_sec = 5;
        timeout.tv_usec = 5000;
        
        if ((fd_num = select(fd_max+1, &copy_reads, 0, 0, &timeout)) == -1) {
            break;
        }
        
        if (fd_num == 0) {
            continue;
        }
        //开启三个客户端连接，然后观察输出
        for (i = 0; i < fd_max + 1; i++) {
            if (FD_ISSET(i, &copy_reads)) {//监听的的fd发生变化
                printf("fd 是 : %d 的socket发生了变化 \n", i);
                if (i == server_socket) {//服务端的socket发生了变化，说明是新client连接进来
                    printf("server_socket is : %d \n", i);
                    address_length = sizeof(client_address);
                    client_socket = accept(server_socket, (struct sockaddr*)&client_address, &address_length);
                    //每次有新client连接进来，就把该client的socket的fd设置一下进行监听
                    FD_SET(client_socket, &reads);
                    if (fd_max < client_socket) {//y因为增加了新的client监听，所以监听的fd数目需要更新到最新的socket值，系统分配socket值是递增的，默认0：标准输入，1：标准输出，2：错误输出，所以分配的socket从3开始
                        fd_max = client_socket;
                    }
                    printf("connect client :%d \n", client_socket);
                }
                else{//非服务端socket的fd发生变化，而是client的socket的fd发生变化，说明有数据输入
                    memset(&buf, 0, sizeof(buf));
                    str_length = read(i, buf, BUF_SIZE);
                    if (str_length == 0) {//收到EOF关闭socket
                        FD_CLR(i, &reads);
                        close(i);
                        printf("close client %d \n", i);
                    }
                    else{//非EOF，就写入数据回传给客户端
                        printf("message from clinet: %s",buf);
                        write(i, buf, str_length);
                        
                    }
                }
            }
        }
        
    }
    
    close(server_socket);
}

void select_console(void)
{
    fd_set reads, temps;
    int result;
    long str_length;
    char buf[BUF_SIZE];
    struct timeval timeout;
    
    FD_ZERO(&reads);
    FD_SET(1, &reads);
    
    while (1) {

        temps = reads;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        result = select(1, &temps, 0, 0,  &timeout);
        
        if (result == -1) {
            puts("select() error");
            break;
        }
        else if (result == 0){
            puts("timeout");
        }
        else{
            if (FD_ISSET(1, &temps)) {
                str_length = read(0, buf, BUF_SIZE);
                buf[str_length] = 0;
                printf("message from console:%s", buf);
            }
        }
        
    }
}




int main(int argc, char *argv[])
{
    select_server(argc,argv) ;
    return 0;
}
