//
//  28_chapter.c
//  TCP&C-Demo
//
//  Created by 西木柚子 on 2019/11/26.
//  Copyright © 2019 feisu. All rights reserved.
//
#include "unp.c"
#include    <netinet/in_systm.h>
#include    <netinet/ip.h>
#include    <netinet/ip_icmp.h>
#include    <netinet/ip6.h>
#include    <netinet/icmp6.h>




//MARK:- ping
#define    BUFSIZE        1500
            /* globals */
char     sendbuf[BUFSIZE];

int         datalen;            /* # bytes of data following ICMP header */
char    *host;
int         nsent;                /* add 1 for each sendto() */
pid_t     pid;                /* our PID */
int         sockfd;
int         verbose;
int nsend=0,nreceived=0;

            /* function prototypes */
void     init_v6(void);
void     proc_v4(char *, ssize_t, struct msghdr *, struct timeval *);
void     proc_v6(char *, ssize_t, struct msghdr *, struct timeval *);
void     send_v4(void);
void     send_v6(void);
void     readloop(void);
void     sig_alrm(int);
void     tv_sub(struct timeval *, struct timeval *);
void statistics(void );

struct proto {
  void     (*fproc)(char *, ssize_t, struct msghdr *, struct timeval *);
  void     (*fsend)(void);
  void     (*finit)(void);
  struct sockaddr  *sasend;    /* sockaddr{} for send, from getaddrinfo */
  struct sockaddr  *sarecv;    /* sockaddr{} for receiving */
  socklen_t        salen;        /* length of sockaddr{}s */
  int               icmpproto;    /* IPPROTO_xxx value for ICMP */
} *pr;

     /* data that goes with ICMP echo request */

struct proto    proto_v4 = { proc_v4, send_v4, NULL, NULL, NULL, 0, IPPROTO_ICMP };
#ifdef    IPV6
struct proto    proto_v6 = { proc_v6, send_v6, init_v6, NULL, NULL, 0, IPPROTO_ICMPV6 };
#endif
int    datalen = 56;


struct xx {
    char *aa;
    int  bb;
}x = {"sdsdd", 199};

void key_control(int sig)
{
    if (sig == SIGINT) {
       statistics();
    }
}

void statistics()
{       printf("\n--------------------PING statistics-------------------\n");
        printf("%d packets transmitted, %d received , %%%d lost\n",nsend,nreceived,
                        (nsend-nreceived)/nsend*100);
        close(sockfd);
        exit(1);
}

void
readloop(void)
{
    int                size;
    char            recvbuf[BUFSIZE];
    char            controlbuf[BUFSIZE];
    struct msghdr    msg;
    struct iovec    iov;
    ssize_t            n;
    struct timeval    tval;

    if ((sockfd = socket(pr->sasend->sa_family, SOCK_RAW, pr->icmpproto)) < 0) {
          err_sys("socket() error");
    }
    setuid(getuid());        /* don't need special permissions any more，回收该程序的root权限 */
    if (pr->finit)
        (*pr->finit)();

    size = 60 * 1024;        /* OK if setsockopt fails */
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));

    sig_alrm(SIGALRM);        /* send first packet */

    iov.iov_base = recvbuf;
    iov.iov_len = sizeof(recvbuf);
    msg.msg_name = pr->sarecv;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = controlbuf;
    for ( ; ; ) {
        msg.msg_namelen = pr->salen;
        msg.msg_controllen = sizeof(controlbuf);

        n = recvmsg(sockfd, &msg, 0);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            else
                err_sys("recvmsg error");
        }
        if (n == 0) {
            statistics();
        }

        gettimeofday(&tval, NULL);
        (*pr->fproc)(recvbuf, n, &msg, &tval);
    }
}


void
tv_sub(struct timeval *out, struct timeval *in)
{
    if ( (out->tv_usec -= in->tv_usec) < 0) {    /* out -= in */
        --out->tv_sec;
        out->tv_usec += 1000000;
    }
    out->tv_sec -= in->tv_sec;
}

void
proc_v4(char *ptr, ssize_t len, struct msghdr *msg, struct timeval *tvrecv)
{
    int                hlen1, icmplen;
    double            rtt;
    struct ip        *ip;
    struct icmp        *icmp;
    struct timeval    *tvsend;

    ip = (struct ip *) ptr;        /* start of IP header */
    hlen1 = ip->ip_hl << 2;        /* length of IP header, 左移两位也就是乘以4，IP首部长度表示多少个四字节长度，比如该字段为5，表示ip首部长度为5*4=20，包括ip头部+可选项长度 */
    if (ip->ip_p != IPPROTO_ICMP)
        return;                /* not ICMP */

    icmp = (struct icmp *) (ptr + hlen1);    /* start of ICMP header */
    if ( (icmplen = (int)len - hlen1) < 8)//ICMP头部长8字节，最小长度也不能小于8字节
        return;                /* malformed packet */

    if (icmp->icmp_type == ICMP_ECHOREPLY) {
        if (icmp->icmp_id != pid)
            return;            /* not a response to our ECHO_REQUEST */
        if (icmplen < 16) //icmp首部8Byte + icmp携带的可选数据长度56Byte = 64
            return;            /* not enough data to use */
        
        nreceived++;
        tvsend = (struct timeval *) icmp->icmp_data;
//        printf("receive:%s--%d\n",((struct xx *) icmp->icmp_data)->aa,((struct xx *) icmp->icmp_data)->bb);
        tv_sub(tvrecv, tvsend);
        rtt = tvrecv->tv_sec * 1000.0 + tvrecv->tv_usec / 1000.0;

        if (verbose) {
            printf("verbose-mode:  %d bytes from %s: type = %d, code = %d\n",
                    icmplen, sock_ntop(pr->sarecv, pr->salen),
                    icmp->icmp_type, icmp->icmp_code);
        }else{
            printf("%d bytes from %s: seq=%u, ttl=%d, rtt=%.3f ms\n",
            icmplen, sock_ntop(pr->sarecv, pr->salen),
            icmp->icmp_seq, ip->ip_ttl, rtt);
        }
        
    }
}


void
proc_v6(char *ptr, ssize_t len, struct msghdr *msg, struct timeval* tvrecv)
{
#ifdef    IPV6
    double                rtt;
    struct icmp6_hdr    *icmp6;
    struct timeval        *tvsend;
    struct cmsghdr        *cmsg;
    int                    hlim;

    icmp6 = (struct icmp6_hdr *) ptr;
    if (len < 8)
        return;                /* malformed packet */

    if (icmp6->icmp6_type == ICMP6_ECHO_REPLY) {
        if (icmp6->icmp6_id != pid)
            return;            /* not a response to our ECHO_REQUEST */
        if (len < 16)
            return;            /* not enough data to use */

        tvsend = (struct timeval *) (icmp6 + 1);
        tv_sub(tvrecv, tvsend);
        rtt = tvrecv->tv_sec * 1000.0 + tvrecv->tv_usec / 1000.0;

        hlim = -1;
        for (cmsg = CMSG_FIRSTHDR(msg); cmsg != NULL; cmsg = CMSG_NXTHDR(msg, cmsg)) {
            if (cmsg->cmsg_level == IPPROTO_IPV6 &&
                cmsg->cmsg_type == IPV6_MAXHLIM) {
                hlim = *(u_int32_t *)CMSG_DATA(cmsg);
                break;
            }
        }
        printf("%zd bytes from %s: seq=%u, hlim=",
                len, sock_ntop(pr->sarecv, pr->salen),
                icmp6->icmp6_seq);
        if (hlim == -1)
            printf("???");    /* ancillary data missing */
        else
            printf("%d", hlim);
        printf(", rtt=%.3f ms\n", rtt);
    } else if (verbose) {
        printf("  %zd bytes from %s: type = %d, code = %d\n",
                len, sock_ntop(pr->sarecv, pr->salen),
                icmp6->icmp6_type, icmp6->icmp6_code);
    }
#endif    /* IPV6 */
}


void
init_v6()
{
#ifdef IPV6
    int on = 1;

    if (verbose == 0) {
        /* install a filter that only passes ICMP6_ECHO_REPLY unless verbose */
        struct icmp6_filter myfilt;
        ICMP6_FILTER_SETBLOCKALL(&myfilt);
        ICMP6_FILTER_SETPASS(ICMP6_ECHO_REPLY, &myfilt);
        setsockopt(sockfd, IPPROTO_IPV6, ICMP6_FILTER, &myfilt, sizeof(myfilt));
        /* ignore error return; the filter is an optimization */
    }

    /* ignore error returned below; we just won't receive the hop limit */
#ifdef IPV6_RECVHOPLIMIT
    /* RFC 3542 */
    setsockopt(sockfd, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &on, sizeof(on));
#else
    /* RFC 2292 */
    setsockopt(sockfd, IPPROTO_IPV6, IPV6_MAXHLIM, &on, sizeof(on));
#endif
#endif
}




void
sig_alrm(int signo)
{
    (*pr->fsend)();

    alarm(1);
    return;
}

void
send_v4(void)
{
    int            len;
    struct icmp    *icmp;

    
    
    icmp = (struct icmp *) sendbuf;
    icmp->icmp_type = ICMP_ECHO;
    icmp->icmp_code = 0;
    icmp->icmp_id = pid;
    icmp->icmp_seq = nsent++;
    memset(icmp->icmp_data, 0xa5, datalen);    /* fill with pattern */
    
//    memcpy((struct xx*)icmp->icmp_data, &x, sizeof(x));
    
    gettimeofday((struct timeval *) icmp->icmp_data, NULL);

    len = 8 + datalen;        /* checksum ICMP header and data */
    icmp->icmp_cksum = 0;
    icmp->icmp_cksum = in_cksum((u_short *) icmp, len);
    sendto(sockfd, sendbuf, len, 0, pr->sasend, pr->salen);
    nsend++;
}

void
send_v6()
{
#ifdef    IPV6
    int                    len;
    struct icmp6_hdr    *icmp6;

    icmp6 = (struct icmp6_hdr *) sendbuf;
    icmp6->icmp6_type = ICMP6_ECHO_REQUEST;
    icmp6->icmp6_code = 0;
    icmp6->icmp6_id = pid;
    icmp6->icmp6_seq = nsent++;
    memset((icmp6 + 1), 0xa5, datalen);    /* fill with pattern */
    gettimeofday((struct timeval *) (icmp6 + 1), NULL);

    len = 8 + datalen;        /* 8-byte ICMPv6 header */

    sendto(sockfd, sendbuf, len, 0, pr->sasend, pr->salen);
        /* 4kernel calculates and stores checksum for us */
#endif    /* IPV6 */
}


uint16_t
in_cksum(uint16_t *addr, int len)
{
    int                nleft = len;
    uint32_t        sum = 0;
    uint16_t        *w = addr;
    uint16_t        answer = 0;

    /*
     * Our algorithm is simple, using a 32 bit accumulator (sum), we add
     * sequential 16 bit words to it, and at the end, fold back all the
     * carry bits from the top 16 bits into the lower 16 bits.
     */
    while (nleft > 1)  {
        sum += *w++;
        nleft -= 2;
    }

        /* 4mop up an odd byte, if necessary */
    if (nleft == 1) {
        *(unsigned char *)(&answer) = *(unsigned char *)w ;
        sum += answer;
    }

        /* 4add back carry outs from top 16 bits to low 16 bits */
    sum = (sum >> 16) + (sum & 0xffff);    /* add hi 16 to low 16 */
    sum += (sum >> 16);            /* add carry */
    answer = ~sum;                /* truncate to 16 bits */
    return(answer);
}

void test_icmp(int argc , char ** argv)
{
    int                c;
        struct addrinfo    *ai;
        char *h;

        opterr = 0;        /* don't want getopt() writing to stderr */
        while ( (c = getopt(argc, argv, "v")) != -1) {
            switch (c) {
            case 'v':
                verbose++;
                break;

            case '?':
                err_quit("unrecognized option: %c", c);
            }
        }

        if (optind != argc-1)
            err_quit("usage: ping [ -v ] <hostname>");
        host = argv[optind];

        pid = getpid() & 0xffff;    /* ICMP ID field is 16 bits */
    printf("****%d--%d \n", pid | 0x8000, getpid());
        Signal(SIGALRM, sig_alrm);

        ai = Host_serv(host, NULL, 0, 0);

        h = sock_ntop(ai->ai_addr, ai->ai_addrlen);
        printf("PING %s (%s): %d data bytes\n",
                ai->ai_canonname ? ai->ai_canonname : h,
                h, datalen);

            /* 4initialize according to protocol */
        if (ai->ai_family == AF_INET) {
            pr = &proto_v4;
    #ifdef    IPV6
        } else if (ai->ai_family == AF_INET6) {
            pr = &proto_v6;
            if (IN6_IS_ADDR_V4MAPPED(&(((struct sockaddr_in6 *)
                                     ai->ai_addr)->sin6_addr)))
                err_quit("cannot ping IPv4-mapped IPv6 address");
    #endif
        } else
            err_quit("unknown address family %d", ai->ai_family);

        pr->sasend = ai->ai_addr;
        pr->sarecv = calloc(1, ai->ai_addrlen);
        pr->salen = ai->ai_addrlen;

        Signal(SIGINT, key_control);
        readloop();
}




//MARK:- main函数
int
main(int argc, char **argv)
{
    test_icmp(argc, argv);
    exit(0);
}
