//
//  fork.c
//  test-c
//
//  Created by sheng wang on 2019/9/29.
//  Copyright © 2019 feisu. All rights reserved.
//

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int gval = 10;

void fork_process(void);
void zombie_process(void);
int no_zombie_process_wait(void);
int no_zombie_process_waitpid(void);


int main11 (int argc, char *argv[])
{

    int status;
    pid_t pid = fork();
    
    if (pid == 0) {
        sleep(15);
        return 24;
    }
    else{
        while (!waitpid(-1, &status, WNOHANG)) {
            sleep(3);
            puts("sleep 3sec \n");
        }
        if (WIFEXITED(status)) {
            printf("child send %d \n", WEXITSTATUS(status));
        }
    }
    return 0;
}

void fork_process(void)
{
    pid_t pid;
    int lval = 20;
    gval++;
    lval += 5;
    
    pid = fork();
    if (pid == 0) {
        gval += 2;
        lval+= 2;
    }else{
        gval -= 2;
        lval -= 2;
    }
    
    if (pid == 0) {
        printf("child process: [%d, %d]\n", gval, lval);
    }
    else{
        printf("parent process: [%d, %d] \n", gval, lval);
    }
}

void zombie_process(void)
{
    pid_t pid = fork();
    
    if (pid == 0) {
        puts("I'm a chile process");
    }else{
        printf("child process ID: %d \n", pid);
        sleep(30);
    }
    
    if (pid == 0) {
        puts("end child process");
    }
    else{
        puts("end parent process");
    }
}


int no_zombie_process_wait(void)
{
    int status;
    pid_t pid = fork();
    
    if (pid == 0) {
        puts("child exit 3");
        return 3;
    }
    else{
        printf("child PID : %d \n", pid);
        pid = fork();
        if (pid == 0) {
            puts("child exit 7");
            exit(7);
        }
        else{
            printf("child pid: %d \n", pid);
            wait(&status);
            if (WIFEXITED(status)) {
                printf("child send one :%d \n", WEXITSTATUS(status));
            }
            wait(&status);
            if (WIFEXITED(status)) {
                printf("child send two :%d \n", WEXITSTATUS(status));
                sleep(30);
            }
        }
        return 0;
    }
}

int no_zombie_process_waitpid(void)
{
    int status;
    pid_t pid = fork();
    
    if (pid == 0) {
        sleep(15);
        return 24;
    }
    else{
        while (!waitpid(-1, &status, WNOHANG)) {
            sleep(3);
            puts("sleep 3sec \n");
        }
        if (WIFEXITED(status)) {
            printf("child send %d \n", WEXITSTATUS(status));
        }
        return 0;
    }
}
