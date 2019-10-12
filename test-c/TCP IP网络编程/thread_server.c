//
//  thread.c
//  test-c
//
//  Created by sheng wang on 2019/10/10.
//  Copyright © 2019 feisu. All rights reserved.
//

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>

long long num = 0;
pthread_mutex_t mutex;
#define BUF_SIZE 1000


void *thread_main(void *arg)
{
    int i;
    int cnt = *((int *)arg);
    char *msg = (char *)malloc(sizeof(char) * 50);
    strcpy(msg, "hello, i am thread \n");
    
    for (i = 0; i < cnt; i++) {
        sleep(1);
        puts("running thread");
    }
    return (void *)msg;
}

int creat_thread(void)
{
    pthread_t t_id;
    int thread_param = 3;
    void *thr_ret;
    
    if (pthread_create(&t_id,  NULL, thread_main, (void*)&thread_param) !=0) {
        puts("error");
        return  -1;
    }
    
    if (pthread_join(t_id, &thr_ret) != 0) {
        puts("pthread_join error");
        return  -1;
    }
    printf("thread return message :%s \n", (char *)thr_ret);
    sleep(10);
    puts("end of main");
    return 0;
}


///////////////////////////////////////////////////////////

void * thread_Inc(void *arg)
{
    int i;
    pthread_mutex_lock(&mutex);
    for (i = 0; i < 5000000; i++) {
        num += 1;
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}


void * thread_des(void *arg)
{
    int i;
    pthread_mutex_lock(&mutex);
    for (i = 0; i < 5000000; i++) {
        num -= 1;
    }
    pthread_mutex_unlock(&mutex);

    return NULL;
}

void error_handling (char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}


void thread_mutex()
{
    pthread_t thread_id[100];
    int i;
    
    pthread_mutex_init(&mutex, NULL);
    
    for (i = 0; i < 100; i ++) {
        if (i%2) {
            pthread_create(&(thread_id[i]), NULL, thread_Inc, NULL);
        }
        else{
            pthread_create(&(thread_id[i]), NULL, thread_des, NULL);
        }
    }
    
    for (i = 0; i < 100; i ++) {
        pthread_join(thread_id[i], NULL);
    }
    printf("result: %lld \n", num);
    pthread_mutex_destroy(&mutex);
}

///////////////////////////////////////////////////////////
int client_cnt = 0;
int client_socks[256];
pthread_mutex_t mutx;

void send_msg(char *msg, long length)
{
    int i;
    pthread_mutex_lock(&mutx);
    for (i = 0; i < client_cnt; i++) {
        write(client_socks[i], msg, length);
    }
    pthread_mutex_unlock(&mutx);
}

void* handle_clinet_message(void *arg)
{
    int sock = *((int *)arg);
    long str_len = 0;
    int i;
    char msg[BUF_SIZE];
    char str[str_len];

    while ((str_len = read(sock, msg, sizeof(msg))) != 0) {
        strncpy(str, msg, str_len);
        printf("message from client: %s", str);
        send_msg(msg, str_len);
    }
    
    //跳出了上面的循环，说明读到了EOF，表明客户端断开了连接，此时需要移除对应的client的socket
    pthread_mutex_lock(&mutx);
    for (i = 0; i < client_cnt; i ++) {
        if (sock == client_socks[i]) {
            while (i++ < client_cnt - 1) {
                client_socks[i] = client_socks[i+1];
            }
            break;
        }
    }
    client_cnt--;
    pthread_mutex_unlock(&mutx);
    close(sock);
    return NULL;
}

void multi_thread_server(int argc, char *argv[])
{
    int server_socket;
    int client_socket;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    socklen_t client_address_size;
    pthread_t t_id;
    
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
    
    while (1) {
        client_address_size = sizeof(client_address);
        client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_size);
        if (client_socket == -1) {
            error_handling("accpet() error");
        }else{
            printf("connected client");
        }
        pthread_mutex_lock(&mutx);
        client_socks[client_cnt++] = client_socket;
        pthread_mutex_unlock(&mutx);
        
        pthread_create(&t_id, NULL, handle_clinet_message, (void*)&client_socket);
        pthread_detach(t_id);//销毁线程，并且不阻塞线程
        printf("connect client ip: %s \n", inet_ntoa(client_address.sin_addr));
    }
    close(server_socket);
    

}

int main(int argc, char *argv[])
{
    multi_thread_server(argc,argv);
    return 0;
}
