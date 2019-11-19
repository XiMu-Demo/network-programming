//
//  15_chapter_client.c
//  TCP&C-Demo
//
//  Created by sheng wang on 2019/11/18.
//  Copyright © 2019 feisu. All rights reserved.
//

#include "unp.c"

//使用socket的读写流，以及标准输入、输出，一共四条流来完成非阻塞I/O
void
non_blocking_str_cli(FILE *fp, int sockfd)
{
    int            maxfdp1, val, stdineof;
    ssize_t        n, nwritten;
    fd_set        rset, wset;
    char        to[MAXLINE], fr[MAXLINE];
    char        *toiptr, *tooptr, *friptr, *froptr;

    val = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, val | O_NONBLOCK);

    val = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, val | O_NONBLOCK);

    val = fcntl(STDOUT_FILENO, F_GETFL, 0);
    fcntl(STDOUT_FILENO, F_SETFL, val | O_NONBLOCK);

    toiptr = tooptr = to;    /* initialize buffer pointers */
    friptr = froptr = fr;
    stdineof = 0;

    maxfdp1 = max(max(STDIN_FILENO, STDOUT_FILENO), sockfd) + 1;
    for ( ; ; ) {
        FD_ZERO(&rset);
        FD_ZERO(&wset);
        if (stdineof == 0 && toiptr < &to[MAXLINE])
            FD_SET(STDIN_FILENO, &rset);    /* read from stdin */
        if (friptr < &fr[MAXLINE])
            FD_SET(sockfd, &rset);            /* read from socket */
        if (tooptr != toiptr)
            FD_SET(sockfd, &wset);            /* data to write to socket */
        if (froptr != friptr)
            FD_SET(STDOUT_FILENO, &wset);    /* data to write to stdout */

        select(maxfdp1, &rset, &wset, NULL, NULL);

        //标准输入可读
        if (FD_ISSET(STDIN_FILENO, &rset)) {
            if ( (n = read(STDIN_FILENO, toiptr, &to[MAXLINE] - toiptr)) < 0) {
                if (errno != EWOULDBLOCK)
                    err_sys("read error on stdin");

            } else if (n == 0) {
                fprintf(stderr, "%s: EOF on stdin\n", gf_time());
                stdineof = 1;            /* all done with stdin */
                if (tooptr == toiptr)
                    shutdown(sockfd, SHUT_WR);/* send FIN */

            } else {//读到的字节数大于0，下一步就是把这些读到的字节写入socket fd，所以要代开sockfd
                fprintf(stderr, "%s: read %zd bytes from stdin\n", gf_time(), n);
                toiptr += n;            /* # just read */
                FD_SET(sockfd, &wset);    /* try and write to socket below */
            }
        }

        //如果sockfd可读
        if (FD_ISSET(sockfd, &rset)) {
            if ( (n = read(sockfd, friptr, &fr[MAXLINE] - friptr)) < 0) {
                if (errno != EWOULDBLOCK)
                    err_sys("read error on socket");

            } else if (n == 0) {
                fprintf(stderr, "%s: EOF on socket\n", gf_time());
                if (stdineof)
                    return;        /* normal termination */
                else
                    err_quit("str_cli: server terminated prematurely");

            } else {//从sockfd中读入的字节数大于0，下一步把这些内容写入到到标准输出流，所以需要打开STDOUT_FILENO标志位
                fprintf(stderr, "%s: read %zd bytes from socket\n",
                                gf_time(), n);
                friptr += n;        /* # just read */
                FD_SET(STDOUT_FILENO, &wset);    /* try and write below */
            }
        }

        //如果标准输出可写并且可写入的字节数大于0，那么久调用write写入到STDOUT_FILENO
        if (FD_ISSET(STDOUT_FILENO, &wset) && ( (n = friptr - froptr) > 0)) {
            if ( (nwritten = write(STDOUT_FILENO, froptr, n)) < 0) {
                if (errno != EWOULDBLOCK)
                    err_sys("write error to stdout");

            } else {
                fprintf(stderr, "%s: wrote %zd bytes to stdout\n",
                                gf_time(), nwritten);
                froptr += nwritten;        /* # just written */
                if (froptr == friptr)//需要写入的内容都写完了，指针都回复到缓冲区开始的位置
                    froptr = friptr = fr;    /* back to beginning of buffer */
            }
        }

        //如果socket输出可写并且可写入的字节数大于0，那么久调用write把内容写入到sockfd
        if (FD_ISSET(sockfd, &wset) && ( (n = toiptr - tooptr) > 0)) {
            if ( (nwritten = write(sockfd, tooptr, n)) < 0) {
                if (errno != EWOULDBLOCK)
                    err_sys("write error to socket");

            } else {
                fprintf(stderr, "%s: wrote %zd bytes to socket\n",
                                gf_time(), nwritten);
                tooptr += nwritten;    /* # just written */
                if (tooptr == toiptr) {
                    toiptr = tooptr = to;    /* back to beginning of buffer */
                    if (stdineof)//如果标准输入遇到了EOF，那么久发送shutdown关闭写入流
                        shutdown(sockfd, SHUT_WR);    /* send FIN */
                }
            }
        }
    }
}


//使用多进程fork来完成读写流分离，实现非阻塞I/O，父进程写入socket，子进程从socket读
void
str_cli_fork(FILE *fp, int sockfd)
{
    pid_t    pid;
    char    sendline[MAXLINE], recvline[MAXLINE];

    if ( (pid = fork()) == 0) {        /* child: server -> stdout */
        while (readline(sockfd, recvline, MAXLINE) > 0)
            fputs(recvline, stdout);
 /* 当服务器崩溃，子进程读到了EOF，那么子进程应该通知父进程不要在往socket中写入内容了，此处是直接杀死父进程，当然也可以不这么做，而是默默退出，然后在父进程中处理SIGCHILD信号
  */
        kill(getppid(), SIGTERM);
        exit(0);
    }

        /* parent: stdin -> server */
    while (fgets(sendline, MAXLINE, fp) != NULL)
        writen(sockfd, sendline, strlen(sendline));

    shutdown(sockfd, SHUT_WR);    /* EOF on stdin, send FIN */
//父进程写入完毕之后就暂停进程，直到被上面的子进程杀死，这么做是为了精准测量从父进程读入数据==>写入socket==>子进程从socket中读取完毕数据或者收到EOF==>子进程杀死父进程，这一整套完整的流程所需要的时间
    pause();
    return;
}

//当在一个非阻塞的tcpsocket上面调用connect，会立即返回一个EINPROGRESS错误，
//但是三次握手已经发起了，然后我们可以用select来检测该connect在指定时间内是否建立成功，再做相应的处理
int
non_blocking_connect(int sockfd, const SA *saptr, socklen_t salen, int nsec)
{
    int                flags, n, error;
    socklen_t        len;
    fd_set            rset, wset;
    struct timeval    tval;

    flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);//设置socket非阻塞

    error = 0;
    if ( (n = connect(sockfd, saptr, salen)) < 0)
        if (errno != EINPROGRESS)//非阻塞的connect操作期待返回EINPROGRESS错误，如果不是该错误，那么就有问题
            return(-1);

    /* Do whatever we want while the connect is taking place. */

    if (n == 0)//client和sever都是一台主机上，那么connect会立马连接上
        goto done;    /* connect completed immediately */

    FD_ZERO(&rset);
    FD_SET(sockfd, &rset);
    wset = rset;
    tval.tv_sec = nsec;
    tval.tv_usec = 0;

    if ( (n = select(sockfd+1, &rset, &wset, NULL,
                     nsec ? &tval : NULL)) == 0) {//n=0，说明sockfd没有发生变化就超时了，返回错误
        close(sockfd);        /* timeout */
        errno = ETIMEDOUT;
        return(-1);
    }

    if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset)) {
        len = sizeof(error);
        //如果sockfd可读或者可写，再检测sockfd上是否有错误发生，没有错误发生说明connect连接成功，否则失败
        if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
            return(-1);            /* Solaris pending error */
    } else
        err_quit("select error: sockfd not set");

done:
    fcntl(sockfd, F_SETFL, flags);    /* restore file status flags */

    if (error) {
        close(sockfd);        /* just in case */
        errno = error;
        return(-1);
    }
    return(0);
}


//MARK:- 非阻塞connect web客户端
#define    MAXFILES    20
#define    SERV        "80"    /* port number or service name */

struct file {
  char    *f_name;            /* filename */
  char    *f_host;            /* hostname or IPv4/IPv6 address */
  int    f_fd;                /* descriptor */
  int     f_flags;            /* F_xxx below */
} file[MAXFILES];

#define    F_CONNECTING    1    /* connect() in progress */
#define    F_READING        2    /* connect() complete; now reading */
#define    F_DONE            4    /* all done */

#define    GET_CMD        "GET %s HTTP/1.0\r\n\r\n"

//nlefttoread：等待读取的文件数目，nlefttoconn：尚无tcp连接的文件数，nconn：当前打开的连接数
int        nconn, nfiles, nlefttoconn, nlefttoread, maxfd;
fd_set    rset, wset;

            /* function prototypes */
void    home_page(const char *, const char *);
void    start_connect(struct file *);
void    write_get_cmd(struct file *);


int
main(int argc, char **argv)
{
    int        i, fd, n, maxnconn, flags, error;
    char    buf[MAXLINE];
    fd_set    rs, ws;

    if (argc < 5)
        err_quit("usage: web <#conns> <hostname> <homepage> <file1> ...");
    maxnconn = atoi(argv[1]);

    nfiles = min(argc - 4, MAXFILES);
    for (i = 0; i < nfiles; i++) {
        file[i].f_name = argv[i + 4];
        file[i].f_host = argv[2];
        file[i].f_flags = 0;
    }
    printf("nfiles = %d\n", nfiles);

    home_page(argv[2], argv[3]);

    FD_ZERO(&rset);
    FD_ZERO(&wset);
    maxfd = -1;
    nlefttoread = nlefttoconn = nfiles;
    nconn = 0;
/* end web1 */
/* include web2 */
    while (nlefttoread > 0) {
        while (nconn < maxnconn && nlefttoconn > 0) {
                /* 4find a file to read */
            for (i = 0 ; i < nfiles; i++)
                if (file[i].f_flags == 0)
                    break;
            if (i == nfiles)
                err_quit("nlefttoconn = %d but nothing found", nlefttoconn);
            start_connect(&file[i]);
            nconn++;
            nlefttoconn--;
        }

        rs = rset;
        ws = wset;
        n = select(maxfd+1, &rs, &ws, NULL, NULL);

        for (i = 0; i < nfiles; i++) {
            flags = file[i].f_flags;
            if (flags == 0 || flags & F_DONE)
                continue;
            fd = file[i].f_fd;
            if (flags & F_CONNECTING &&
                (FD_ISSET(fd, &rs) || FD_ISSET(fd, &ws))) {
                n = sizeof(error);
                if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &n) < 0 ||
                    error != 0) {
                    err_ret("nonblocking connect failed for %s",
                            file[i].f_name);
                }
                    /* 4connection established */
                printf("connection established for %s\n", file[i].f_name);
                FD_CLR(fd, &wset);        /* no more writeability test */
                write_get_cmd(&file[i]);/* write() the GET command */

            } else if (flags & F_READING && FD_ISSET(fd, &rs)) {
                if ( (n = read(fd, buf, sizeof(buf))) == 0) {
                    printf("end-of-file on %s\n", file[i].f_name);
                    close(fd);
                    file[i].f_flags = F_DONE;    /* clears F_READING */
                    FD_CLR(fd, &rset);
                    nconn--;
                    nlefttoread--;
                } else {
                    printf("read %d bytes from %s\n", n, file[i].f_name);
                }
            }
        }
    }
    exit(0);
}
/* end web2 */


void
home_page(const char *host, const char *fname)
{
    int        fd;
    ssize_t n;
    char    line[MAXLINE];

    fd = tcp_connect(host, SERV);    /* blocking connect() */

    n = snprintf(line, sizeof(line), GET_CMD, fname);
    writen(fd, line, n);

    for ( ; ; ) {
        if ( (n = read(fd, line, MAXLINE)) == 0)
            break;        /* server closed connection */

        printf("read %zd bytes of home page\n", n);
        /* do whatever with data */
    }
    printf("end-of-file on home page\n");
    close(fd);
}


void
start_connect(struct file *fptr)
{
    int                fd, flags, n;
    struct addrinfo    *ai;

    ai = Host_serv(fptr->f_host, SERV, 0, SOCK_STREAM);

    fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    fptr->f_fd = fd;
    printf("start_connect for %s, fd %d\n", fptr->f_name, fd);

        /* 4Set socket nonblocking */
    flags = Fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

        /* 4Initiate nonblocking connect to the server. */
    if ( (n = connect(fd, ai->ai_addr, ai->ai_addrlen)) < 0) {
        if (errno != EINPROGRESS)
            err_sys("nonblocking connect error");
        fptr->f_flags = F_CONNECTING;
        FD_SET(fd, &rset);            /* select for reading and writing */
        FD_SET(fd, &wset);
        if (fd > maxfd)
            maxfd = fd;

    } else if (n >= 0)                /* connect is already done */
        write_get_cmd(fptr);    /* write() the GET command */
}


void
write_get_cmd(struct file *fptr)
{
    int        n;
    char    line[MAXLINE];

    n = snprintf(line, sizeof(line), GET_CMD, fptr->f_name);
    writen(fptr->f_fd, line, n);
    printf("wrote %d bytes for %s\n", n, fptr->f_name);

    fptr->f_flags = F_READING;            /* clears F_CONNECTING */

    FD_SET(fptr->f_fd, &rset);            /* will read server's reply */
    if (fptr->f_fd > maxfd)
        maxfd = fptr->f_fd;
}

