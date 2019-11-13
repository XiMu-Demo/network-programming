//
//  13_chapter.c
//  TCP&C-Demo
//
//  Created by sheng wang on 2019/11/13.
//  Copyright © 2019 feisu. All rights reserved.
//

//参考文章：https://blog.csdn.net/u010694337/article/details/75214218

#include "unp.c"

#define    MAXFD    64

extern int    daemon_proc;    /* defined in error.c */

int
daemon_init(const char *pname, int facility)
{
    int        i;
    pid_t    pid;

    if ( (pid = fork()) < 0)
        return (-1);
    else if (pid)
        _exit(0);            /* parent terminates */

    /* child 1 continues... */

//由于创建守护进程的第一步是调用fork()函数来创建子进程，再将父进程退出。由于在调用了fork()函数的时候，子进程拷贝了父进程的会话期、进程组、控制终端等资源、虽然父进程退出了，但是会话期、进程组、控制终端等并没有改变，因此，需要用setsid()函数来时该子进程完全独立出来，从而摆脱其他进程的控制
    if (setsid() < 0)            /* become session leader */
        return (-1);

    /* 屏蔽一些有关控制终端操作的信号 * 防止在守护进程没有正常运转起来时，因控制终端受到干扰退出或挂起 * */
    signal(SIGINT,  SIG_IGN);// 终端中断
    signal(SIGHUP,  SIG_IGN);// 连接挂断
    signal(SIGQUIT, SIG_IGN);// 终端退出
    signal(SIGPIPE, SIG_IGN);// 向无读进程的管道写数据
    signal(SIGTTOU, SIG_IGN);// 后台程序尝试写操作
    signal(SIGTTIN, SIG_IGN);// 后台程序尝试读操作
    signal(SIGTERM, SIG_IGN);// 终止
    
    //再次fork产生子进程作为守护进程，是为了确保本守护进程将来即使打开了一个终端设备，也不会自动获取终端的控制权
    if ( (pid = fork()) < 0)
        return (-1);
    else if (pid)
        _exit(0);            /* child 1 terminates */

    /* child 2 continues... */

    daemon_proc = 1;            /* for err_XXX() functions */

    /*
进程活动时，其工作目录所在的文件系统不能卸载。例如：我们是从/mnt/usb目录下启动该护进程的，那么如果守护进程的工作目录就是/mnt/usb，我们
     就无法在守护进程还在运行的情况下umount/mnt/usb。所以一般需要将守护的工作目录切换到根目录。护的工作目录切换到根目录。chdir("/");
     */
    chdir("/");                /* change working directory */

//文件权限掩码是指屏蔽掉文件权限中的对应位。比如掩码是500，它就屏蔽了文件创建者的可读与可执行权限。由于子进程要继承父进程的文件权限掩 码，这势必影响子进程中新创建的文件的访问权限，为避免该影响，就需要重新对子进程中的权限掩码清零。通常的使用方法为函数：程中的权限掩码清零。通常的 使用方法为函数：umask(0)
    umask(0);
    
//同文件权限码一样，子进程还会从父进程那里继承一些已经打开了的文件。这些被打开的文件可能永远不会被守护进程读写，但它们一样消耗系统资源，而且会导致文件所在的文件系统无法卸载。因此在子进程中需要将这些文件关闭。
    for (i = 0; i < MAXFD; i++)
        close(i);

    /* redirect stdin, stdout, and stderr to /dev/null ,从而防止服务器把错误输出到了客户端*/
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_RDWR);
    open("/dev/null", O_RDWR);

    openlog(pname, LOG_PID, facility);

    return (0);                /* success */
}


int
main(int argc, char **argv)
{
    int                     listenfd, connfd;
    struct sockaddr_in      servaddr, clientaddr;
    char                    buff[MAXLINE];
    time_t                  ticks;
    socklen_t               childlen;
    

    daemon_init(argv[0], 0);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(1313);    /* daytime server */

    bind(listenfd, (SA *) &servaddr, sizeof(servaddr));
    listen(listenfd, LISTENQ);

    for ( ; ; ) {
        childlen = sizeof(clientaddr);
        connfd = accept(listenfd, (SA *)&clientaddr, &childlen);
        err_msg("connect from %s", sock_ntop((SA *)&clientaddr, childlen));
        ticks = time(NULL);
        snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
        write(connfd, buff, strlen(buff));

        close(connfd);
    }
}
