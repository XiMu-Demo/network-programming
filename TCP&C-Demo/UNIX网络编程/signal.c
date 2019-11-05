//
//  signal.c
//  TCP&C-Demo
//
//  Created by sheng wang on 2019/10/22.
//  Copyright © 2019 feisu. All rights reserved.
//

#include "unp.h"



void read_childpro(int sig)
{
    int status;
    pid_t id = waitpid(-1, &status, WNOHANG);
    if (WIFEXITED(status)) {
        printf("remove proc id:%d \n", id);
        printf("child send: %d \n", WEXITSTATUS(status));
    }
}

int main()
{
    int pid = fork();
    if(pid > 0)
    {
        struct sigaction s;
        memset(&s, 0, sizeof(s));
        s.sa_handler = read_childpro;
        sigfillset(&s.sa_mask);
        /*
         下面这句话去掉，输出如下：
         remove proc id:69304
         child send: 22
         End
         然后程序结束
         
         下面这句话不注释，输出如下：
         remove proc id:69297
         child send: 22
         
         然后程序会回到之前的被中断的系统调用scanf函数处，等待用户输入，一旦用户输入完毕后，程序才会终止
         http://49d2d554.wiz03.com/wapp/pages/view/share/s/19QJlk1ln4BA22s3BC3c9d430od0332DXk8m2UifrV3KVUu0
         */
        s.sa_flags |= SA_RESTART;
        assert(sigaction(SIGCHLD, &s, NULL) != 1);
    }
    else if(pid == 0)
    {
        sleep(3);
        exit(22);
    }
    
    char str[10];
    scanf("%s", str);
    printf("%s\n", str);
    
    puts("End");
    
    return 0;
}

