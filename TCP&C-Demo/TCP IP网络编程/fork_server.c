//
//  fork.c
//  C_DEMO
//
//  Created by 西木柚子 on 2019/10/2.
//  Copyright © 2019 西木柚子. All rights reserved.
//

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>

#define BUF_SIZE 1000

void timeout(int sig)
{
    if (sig == SIGALRM){
        puts("time out!");
    }
    alarm(2);
}

void key_control(int sig)
{
    if (sig == SIGINT) {
        puts("CRTL+C pressed");
    }
}

void signal_method(void)
{
    int i;
       signal(SIGALRM, timeout);
       signal(SIGINT, key_control);
       alarm(2);
       
       for (i = 0; i < 3; i++) {
           puts("wait.....");
           sleep(100);
       }
}

void sigaction_method(void)
{
    int i;
    struct sigaction act;
    act.sa_handler = timeout;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGALRM, &act, 0);
    
    alarm(2);
    
    for (i = 0; i<3; i++) {
        puts("wait...");
        sleep(100);
    }
}

void read_childpro(int sig)
{
    int status;
    pid_t id = waitpid(-1, &status, WNOHANG);
    if (WIFEXITED(status)) {
        printf("remove proc id:%d \n", id);
        printf("child send: %d \n", WEXITSTATUS(status));
    }
}


int remove_zombine(void)
{
    pid_t pid;
    struct sigaction act;
    act.sa_handler = read_childpro;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGCHLD, &act, 0);
    
    pid = fork();
    if (pid == 0) {
        puts("Hi! i am child process");
        sleep(10);
        return 12;
    }
    else{
        printf("parent：child pro id:%d \n", pid);
        pid = fork();
        if (pid == 0) {
            puts("i am child process");
            sleep(10);
            exit(24);
        }
        else{
            int i;
            printf("child pro id:%d \n", pid);
            for (i = 0; i<5; i++) {
                puts("wait....");
                sleep(5);
            }
        }
    }
    return 0;
}

static void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

int multi_process_server(int argc, char *argv[])
{
     int server_sock, client_sock, state;
     struct sockaddr_in server_address, clinet_address;
     pid_t pid;
     struct sigaction act;
     socklen_t address_size;
     char buf[BUF_SIZE];
     long str_len;
     if (argc != 2) {
         printf("usage: %s <port> \n",argv[0]);
         exit(EXIT_SUCCESS);
     }
     
     act.sa_handler = read_childpro;
     sigemptyset(&act.sa_mask);
     act.sa_flags = 0;
     state = sigaction(SIGCHLD, &act, 0);
     
     server_sock = socket(PF_INET, SOCK_STREAM, 0);
     memset(&server_address, 0, sizeof(server_address));
     server_address.sin_family = AF_INET;
     server_address.sin_addr.s_addr = htonl(INADDR_ANY);
     server_address.sin_port = htons(atoi(argv[1]));
     
     if (bind(server_sock, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
         error_handling("bind error");
     }
     if (listen(server_sock, 5) == -1) {
         error_handling("listen error");
     }

     while (1) {
         address_size = sizeof(clinet_address);
         client_sock = accept(server_sock, (struct sockaddr*)&clinet_address, &address_size);
         if (client_sock == -1) {
             continue;
         }
         else{
             puts("new client connect...");
         }
         pid = fork();

         if (pid == 0) {
             close(server_sock);
             while ((str_len = read(client_sock, buf, BUF_SIZE)) !=0) {
                 write(client_sock, buf, str_len);
                 printf("message from client: %s", buf);
             }
             close(client_sock);
             return 0;
         }
         else{
             close(client_sock);
         }
     }
     return 0;
}

void single_pipe(void)
{
    int fds[2];
    char str1[] = "who are you?";
    char str2[] = "I'm Lucy";
    char buf[BUF_SIZE];
    pid_t pid;
    
    pipe(fds);
    pid = fork();
    if (pid == 0) {
        write(fds[1], str1, sizeof(str1));
        sleep(2);
        read(fds[0], buf, BUF_SIZE);
        printf("child proc: %s \n", buf);
    }
    else{
        read(fds[0], buf, BUF_SIZE);
        printf("parent proc :%s \n", buf);
        write(fds[1], str2, sizeof(str2));
        sleep(3);
    }
}

int muliti_pipe_process_server(int argc, char *argv[])
{
    int serv_sock, clnt_sock, state;
    struct sockaddr_in serv_adr, clnt_adr;
    int fds[2];
 
    pid_t pid;
    struct sigaction act;
    socklen_t adr_sz;
    long str_len;
    char buf[BUF_SIZE];
    if (argc != 2) {
        printf("Usage: %s <port> \n",argv[0]);
        exit(1);
    }
     
    act.sa_handler = read_childpro;            //设置信号处理函数
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    state = sigaction(SIGCHLD,&act,0);            //子进程终止时调用Handler
    
    serv_sock = socket(PF_INET,SOCK_STREAM,0);
    memset(&serv_adr,0,sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));
 
    if (bind(serv_sock,(struct sockaddr*)&serv_adr,sizeof(serv_adr)) == -1)
        error_handling("bind() error");
    if (listen(serv_sock,5) == -1)
        error_handling("listen() error");
 
    pipe(fds);
    pid = fork();
    if (pid == 0)
    {
        FILE* fp = fopen("/Users/shengwang/学习/network-programming/test-c/TCP IP网络编程/123.txt","wt");
        char msgbuf[BUF_SIZE] = "";
        int i;
        long len;
        
        for (i = 0; i < 100; i++ )
        {
            len = read(fds[0],msgbuf,BUF_SIZE)+1;    //从管道出口fds[0]读取数据并保存到文件中
            printf("wirte to file :%s", msgbuf);
            size_t t =  fwrite((void*)msgbuf,1,BUF_SIZE,fp);
            printf("write byte: %zu \n",t);
        }
        fclose(fp);
        return 0;
    }
 
    while (1)
    {
        adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock,(struct sockaddr*)&clnt_adr,&adr_sz);
        if (clnt_sock == -1)
            continue;
        else
            puts("new client connected...");
        //这里创建一个子进程来将数据写入管道
        pid = fork();
        if (pid == 0)        //子进程运行区域
        {
            close(serv_sock);
            while((str_len = read(clnt_sock,buf,BUF_SIZE)) != 0)
            {
                write(clnt_sock,buf,str_len);
                write(fds[1],buf,str_len);        //将从客户端接收到的数据写入到管道入口fds[1]中
                printf("message from client: %s", buf);
            }
 
            close(clnt_sock);
            puts("client disconnected...");
            return 0;        //调用Handler
        }
        else                //父进程运行区域
            close(clnt_sock);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    remove_zombine();
  
    return 0;

}
