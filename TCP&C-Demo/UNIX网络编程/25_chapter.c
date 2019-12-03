//
//  25_chapter.c
//  TCP&C-Demo
//
//  Created by 西木柚子 on 2019/12/2.
//  Copyright © 2019 feisu. All rights reserved.
//

//配合8_chapter_server 使用，本文件做为client端

#include    <setjmp.h>
#include "unp.c"


#define    RTT_DEBUG

struct rtt_info {
  float        rtt_rtt;    /* most recent measured RTT, seconds */
  float        rtt_srtt;    /* smoothed RTT estimator, seconds */
  float        rtt_rttvar;    /* smoothed mean deviation, seconds */
  float        rtt_rto;    /* current RTO to use, seconds */
  int        rtt_nrexmt;    /* #times retransmitted: 0, 1, 2, ... */
  uint32_t    rtt_base;    /* #sec since 1/1/1970 at start */
};

#define    RTT_RXTMIN      2    /* min retransmit timeout value, seconds */
#define    RTT_RXTMAX     60    /* max retransmit timeout value, seconds */
#define    RTT_MAXNREXMT     3    /* max #times to retransmit */

                /* function prototypes */
void     rtt_debug(struct rtt_info *);
void     rtt_init(struct rtt_info *);
void     rtt_newpack(struct rtt_info *);
int         rtt_start(struct rtt_info *);
void     rtt_stop(struct rtt_info *, uint32_t);
int         rtt_timeout(struct rtt_info *);
uint32_t rtt_ts(struct rtt_info *);
extern int    rtt_d_flag;    /* can be set nonzero for addl info */
static struct rtt_info   rttinfo;
static int    rttinit = 0;
static struct msghdr    msgsend, msgrecv;    /* assumed init to 0 */
static struct hdr {
  uint32_t    seq;    /* sequence # */
  uint32_t    ts;        /* timestamp when sent */
} sendhdr, recvhdr;


static void    sig_alrm(int signo);
static sigjmp_buf    jmpbuf;


//MARK:- UDP 可靠传输
ssize_t
dg_send_recv(int fd, const void *outbuff, size_t outbytes,
             void *inbuff, size_t inbytes,
             const SA *destaddr, socklen_t destlen)
{
    ssize_t            n;
    struct iovec    iovsend[2], iovrecv[2];

    if (rttinit == 0) {
        rtt_init(&rttinfo);        /* first time we're called */
        rttinit = 1;
        rtt_d_flag = 1;
    }

    sendhdr.seq++;
    msgsend.msg_name = (void *)destaddr;
    msgsend.msg_namelen = destlen;
    msgsend.msg_iov = iovsend;
    msgsend.msg_iovlen = 2;
    iovsend[0].iov_base = &sendhdr;
    iovsend[0].iov_len = sizeof(struct hdr);
    iovsend[1].iov_base = (void *)outbuff;
    iovsend[1].iov_len = outbytes;

    msgrecv.msg_name = NULL;
    msgrecv.msg_namelen = 0;
    msgrecv.msg_iov = iovrecv;
    msgrecv.msg_iovlen = 2;
    iovrecv[0].iov_base = &recvhdr;
    iovrecv[0].iov_len = sizeof(struct hdr);
    iovrecv[1].iov_base = (void *)inbuff;
    iovrecv[1].iov_len = inbytes;

    Signal(SIGALRM, sig_alrm);
    rtt_newpack(&rttinfo);        /* initialize for this packet */

sendagain:
    fprintf(stderr, "send %4d: ", sendhdr.seq);
    sendhdr.ts = rtt_ts(&rttinfo);
    sendmsg(fd, &msgsend, 0);

    alarm(rtt_start(&rttinfo));    /* calc timeout value & start timer */
    rtt_debug(&rttinfo);

    if (sigsetjmp(jmpbuf, 1) != 0) {
        if (rtt_timeout(&rttinfo) < 0) {
            err_msg("dg_send_recv: no response from server, giving up");
            rttinit = 0;    /* reinit in case we're called again */
            errno = ETIMEDOUT;
            return(-1);
        }
        err_msg("dg_send_recv: timeout, retransmitting");
        goto sendagain;
    }

    do {
        n = recvmsg(fd, &msgrecv, 0);
        fprintf(stderr, "recv %4d\n", recvhdr.seq);
    } while (n < sizeof(struct hdr) || recvhdr.seq != sendhdr.seq);

    alarm(0);            /* stop SIGALRM timer */
        /* 4calculate & store new RTT estimator values */
    rtt_stop(&rttinfo, rtt_ts(&rttinfo) - recvhdr.ts);
    printf("recvhdr.ts:%d \n", recvhdr.ts);
    return(n - sizeof(struct hdr));    /* return size of received datagram */
}

static void
sig_alrm(int signo)
{
    siglongjmp(jmpbuf, 1);
}
/* end dgsendrecv2 */

ssize_t
Dg_send_recv(int fd, const void *outbuff, size_t outbytes,
             void *inbuff, size_t inbytes,
             const SA *destaddr, socklen_t destlen)
{
    ssize_t    n;

    n = dg_send_recv(fd, outbuff, outbytes, inbuff, inbytes,
                     destaddr, destlen);
    if (n < 0)
        err_quit("dg_send_recv error");

    return(n);
}

void
dg_cli_1(FILE *fp, int sockfd, const SA *pservaddr, socklen_t servlen)
{
    ssize_t    n;
    char    sendline[MAXLINE], recvline[MAXLINE + 1];

    while (fgets(sendline, MAXLINE, fp) != NULL) {
        n = Dg_send_recv(sockfd, sendline, strlen(sendline),
                         recvline, MAXLINE, pservaddr, servlen);

        recvline[n] = 0;    /* null terminate */
        fputs(recvline, stdout);
    }
}

int main (int argc, char ** argv)
{
    int sockfd;
    struct sockaddr_in server_addr;

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERV_PORT);
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    dg_cli_1(stdin, sockfd, (SA *)&server_addr, sizeof(server_addr));

    exit(EXIT_SUCCESS);
}


//MARK:- RTT

int        rtt_d_flag = 0;        /* debug flag; can be set by caller */

/*
 * Calculate the RTO value based on current estimators:
 *        smoothed RTT plus four times the deviation
 */
#define    RTT_RTOCALC(ptr) ((ptr)->rtt_srtt + (4.0 * (ptr)->rtt_rttvar))

static float
rtt_minmax(float rto)
{
    if (rto < RTT_RXTMIN)
        rto = RTT_RXTMIN;
    else if (rto > RTT_RXTMAX)
        rto = RTT_RXTMAX;
    return(rto);
}

void
rtt_init(struct rtt_info *ptr)
{
    struct timeval    tv;

    gettimeofday(&tv, NULL);
    ptr->rtt_base = tv.tv_sec;        /* # sec since 1/1/1970 at start */

    ptr->rtt_rtt    = 0;
    ptr->rtt_srtt   = 0;
    ptr->rtt_rttvar = 0.75;
    ptr->rtt_rto = rtt_minmax(RTT_RTOCALC(ptr));
        /* first RTO at (srtt + (4 * rttvar)) = 3 seconds */
}
/* end rtt1 */

/*
 * Return the current timestamp.
 * Our timestamps are 32-bit integers that count milliseconds since
 * rtt_init() was called.
 */

/* include rtt_ts */
uint32_t
rtt_ts(struct rtt_info *ptr)
{
    uint32_t        ts;
    struct timeval    tv;

    gettimeofday(&tv, NULL);
    ts = ((tv.tv_sec - ptr->rtt_base) * 1000) + (tv.tv_usec / 1000);
    return(ts);
}

void
rtt_newpack(struct rtt_info *ptr)
{
    ptr->rtt_nrexmt = 0;
}

int
rtt_start(struct rtt_info *ptr)
{
    return((int) (ptr->rtt_rto + 0.5));        /* round float to int */
        /* 4return value can be used as: alarm(rtt_start(&foo)) */
}
/* end rtt_ts */

/*
 * A response was received.
 * Stop the timer and update the appropriate values in the structure
 * based on this packet's RTT.  We calculate the RTT, then update the
 * estimators of the RTT and its mean deviation.
 * This function should be called right after turning off the
 * timer with alarm(0), or right after a timeout occurs.
 */

/* include rtt_stop */
void
rtt_stop(struct rtt_info *ptr, uint32_t ms)
{
    double        delta;

    ptr->rtt_rtt = ms / 1000.0;        /* measured RTT in seconds */

    /*
     * Update our estimators of RTT and mean deviation of RTT.
     * See Jacobson's SIGCOMM '88 paper, Appendix A, for the details.
     * We use floating point here for simplicity.
     */

    delta = ptr->rtt_rtt - ptr->rtt_srtt;
    ptr->rtt_srtt += delta / 8;        /* g = 1/8 */

    if (delta < 0.0)
        delta = -delta;                /* |delta| */

    ptr->rtt_rttvar += (delta - ptr->rtt_rttvar) / 4;    /* h = 1/4 */

    ptr->rtt_rto = rtt_minmax(RTT_RTOCALC(ptr));
}
/* end rtt_stop */

/*
 * A timeout has occurred.
 * Return -1 if it's time to give up, else return 0.
 */

/* include rtt_timeout */
int
rtt_timeout(struct rtt_info *ptr)
{
    ptr->rtt_rto *= 2;        /* next RTO */

    if (++ptr->rtt_nrexmt > RTT_MAXNREXMT)
        return(-1);            /* time to give up for this packet */
    return(0);
}
/* end rtt_timeout */

/*
 * Print debugging information on stderr, if the "rtt_d_flag" is nonzero.
 */

void
rtt_debug(struct rtt_info *ptr)
{
    if (rtt_d_flag == 0)
        return;

    fprintf(stderr, "rtt = %.3f, srtt = %.3f, rttvar = %.3f, rto = %.3f\n",
            ptr->rtt_rtt, ptr->rtt_srtt, ptr->rtt_rttvar, ptr->rtt_rto);
    fflush(stderr);
}
