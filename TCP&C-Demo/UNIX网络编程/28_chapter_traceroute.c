//
//  28_chapter_traceroute.c
//  TCP&C-Demo
//
//  Created by 西木柚子 on 2019/11/27.
//  Copyright © 2019 feisu. All rights reserved.
//
#include "unp.c"
#include    <netinet/in_systm.h>
#include    <netinet/ip.h>
#include    <netinet/ip_icmp.h>
#include    <netinet/ip6.h>
#include    <netinet/icmp6.h>
#include    <netinet/udp.h>


//MARK:- 申明变量和函数

#define    BUFSIZE        1500

struct rec {                    /* format of outgoing UDP data */
  u_short    rec_seq;            /* sequence number */
  u_short    rec_ttl;            /* TTL packet left with */
  struct timeval    rec_tv;        /* time packet left */
};

            /* globals */
char     recvbuf[BUFSIZE];
char     sendbuf[BUFSIZE];

int         datalen;            /* # bytes of data following ICMP header */
char    *host;
u_short     sport, dport;
int         nsent;                /* add 1 for each sendto() */
pid_t     pid;                /* our PID */
int         probe, nprobes;
int         sendfd, recvfd;    /* send on UDP sock, read on raw ICMP sock */
int         ttl, max_ttl;
int         verbose;

            /* function prototypes */
const char    *icmpcode_v4(int);
const char    *icmpcode_v6(int);
int         recv_v4(int, struct timeval *);
int         recv_v6(int, struct timeval *);
void     sig_alrm(int);
void     traceloop(void);
void     tv_sub(struct timeval *, struct timeval *);

struct proto {
  const char    *(*icmpcode)(int);
  int     (*recv)(int, struct timeval *);
  struct sockaddr  *sasend;    /* sockaddr{} for send, from getaddrinfo */
  struct sockaddr  *sarecv;    /* sockaddr{} for receiving */
  struct sockaddr  *salast;    /* last sockaddr{} for receiving */
  struct sockaddr  *sabind;    /* sockaddr{} for binding source port */
  socklen_t           salen;    /* length of sockaddr{}s */
  int            icmpproto;    /* IPPROTO_xxx value for ICMP */
  int       ttllevel;        /* setsockopt() level to set TTL */
  int       ttloptname;        /* setsockopt() name to set TTL */
} *pr;


struct proto    proto_v4 = { icmpcode_v4, recv_v4, NULL, NULL, NULL, NULL, 0,
                             IPPROTO_ICMP, IPPROTO_IP, IP_TTL };
#ifdef    IPV6
struct proto    proto_v6 = { icmpcode_v6, recv_v6, NULL, NULL, NULL, NULL, 0,
                             IPPROTO_ICMPV6, IPPROTO_IPV6, IPV6_UNICAST_HOPS };
#endif

int        datalen = sizeof(struct rec);    /* defaults */
int        max_ttl = 30;
int        nprobes = 3;
u_short    dport = 32768 + 666;
static int gotalarm;


//MARK:- 实现
const char *
icmpcode_v6(int code)
{
#ifdef    IPV6
    static char errbuf[100];
    switch (code) {
    case  ICMP6_DST_UNREACH_NOROUTE:
        return("no route to host");
    case  ICMP6_DST_UNREACH_ADMIN:
        return("administratively prohibited");
    case  ICMP6_DST_UNREACH_NOTNEIGHBOR:
        return("not a neighbor");
    case  ICMP6_DST_UNREACH_ADDR:
        return("address unreachable");
    case  ICMP6_DST_UNREACH_NOPORT:
        return("port unreachable");
    default:
        sprintf(errbuf, "[unknown code %d]", code);
        return errbuf;
    }
#endif
}


const char *
icmpcode_v4(int code)
{
    static char errbuf[100];
    switch (code) {
    case  0:    return("network unreachable");
    case  1:    return("host unreachable");
    case  2:    return("protocol unreachable");
    case  3:    return("port unreachable");
    case  4:    return("fragmentation required but DF bit set");
    case  5:    return("source route failed");
    case  6:    return("destination network unknown");
    case  7:    return("destination host unknown");
    case  8:    return("source host isolated (obsolete)");
    case  9:    return("destination network administratively prohibited");
    case 10:    return("destination host administratively prohibited");
    case 11:    return("network unreachable for TOS");
    case 12:    return("host unreachable for TOS");
    case 13:    return("communication administratively prohibited by filtering");
    case 14:    return("host recedence violation");
    case 15:    return("precedence cutoff in effect");
    default:    sprintf(errbuf, "[unknown code %d]", code);
                return errbuf;
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
sig_alrm(int signo)
{
    gotalarm = 1;    /* set flag to note that alarm occurred */
    return;            /* and interrupt the recvfrom() */
}


/*
 * Return: -3 on timeout
 *           -2 on ICMP time exceeded in transit (caller keeps going)
 *           -1 on ICMP port unreachable (caller is done)
 *         >= 0 return value is some other ICMP unreachable code
 */

int
recv_v6(int seq, struct timeval *tv)
{
#ifdef    IPV6
    int                    hlen2, icmp6len, ret;
    ssize_t                n;
    socklen_t            len;
    struct ip6_hdr        *hip6;
    struct icmp6_hdr    *icmp6;
    struct udphdr        *udp;

    gotalarm = 0;
    alarm(3);
    for ( ; ; ) {
        if (gotalarm)
            return(-3);        /* alarm expired */
        len = pr->salen;
        n = recvfrom(recvfd, recvbuf, sizeof(recvbuf), 0, pr->sarecv, &len);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            else
                err_sys("recvfrom error");
        }

        icmp6 = (struct icmp6_hdr *) recvbuf; /* ICMP header */
        if ( ( icmp6len = n ) < 8)
            continue;                /* not enough to look at ICMP header */
    
        if (icmp6->icmp6_type == ICMP6_TIME_EXCEEDED &&
            icmp6->icmp6_code == ICMP6_TIME_EXCEED_TRANSIT) {
            if (icmp6len < 8 + sizeof(struct ip6_hdr) + 4)
                continue;            /* not enough data to look at inner header */

            hip6 = (struct ip6_hdr *) (recvbuf + 8);
            hlen2 = sizeof(struct ip6_hdr);
            udp = (struct udphdr *) (recvbuf + 8 + hlen2);
            if (hip6->ip6_nxt == IPPROTO_UDP &&
                udp->uh_sport == htons(sport) &&
                udp->uh_dport == htons(dport + seq))
                ret = -2;        /* we hit an intermediate router */
                break;

        } else if (icmp6->icmp6_type == ICMP6_DST_UNREACH) {
            if (icmp6len < 8 + sizeof(struct ip6_hdr) + 4)
                continue;            /* not enough data to look at inner header */

            hip6 = (struct ip6_hdr *) (recvbuf + 8);
            hlen2 = sizeof(struct ip6_hdr);
            udp = (struct udphdr *) (recvbuf + 8 + hlen2);
            if (hip6->ip6_nxt == IPPROTO_UDP &&
                udp->uh_sport == htons(sport) &&
                udp->uh_dport == htons(dport + seq)) {
                if (icmp6->icmp6_code == ICMP6_DST_UNREACH_NOPORT)
                    ret = -1;    /* have reached destination */
                else
                    ret = icmp6->icmp6_code;    /* 0, 1, 2, ... */
                break;
            }
        } else if (verbose) {
            printf(" (from %s: type = %d, code = %d)\n",
                    sock_ntop(pr->sarecv, pr->salen),
                    icmp6->icmp6_type, icmp6->icmp6_code);
        }
        /* Some other ICMP error, recvfrom() again */
    }
    alarm(0);                    /* don't leave alarm running */
    gettimeofday(tv, NULL);        /* get time of packet arrival */
    return(ret);
#endif
}



int
recv_v4(int seq, struct timeval *tv)
{
    int                hlen1, hlen2, icmplen, ret;
    socklen_t        len;
    ssize_t            n;
    struct ip        *ip, *hip;
    struct icmp        *icmp;
    struct udphdr    *udp;

    gotalarm = 0;
    alarm(3);
    for ( ; ; ) {
        if (gotalarm)
            return(-3);        /* alarm expired */
        len = pr->salen;
        n = recvfrom(recvfd, recvbuf, sizeof(recvbuf), 0, pr->sarecv, &len);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            else
                err_sys("recvfrom error");
        }

        ip = (struct ip *) recvbuf;    /* start of IP header */
        hlen1 = ip->ip_hl << 2;        /* length of IP header */
    
        icmp = (struct icmp *) (recvbuf + hlen1); /* start of ICMP header */
        if ( (icmplen = n - hlen1) < 8)
            continue;                /* not enough to look at ICMP header */
    
        if (icmp->icmp_type == ICMP_TIMXCEED &&
            icmp->icmp_code == ICMP_TIMXCEED_INTRANS) {
            if (icmplen < 8 + sizeof(struct ip))
                continue;            /* not enough data to look at inner IP */

            hip = (struct ip *) (recvbuf + hlen1 + 8);
            hlen2 = hip->ip_hl << 2;
            if (icmplen < 8 + hlen2 + 4)
                continue;            /* not enough data to look at UDP ports */

            udp = (struct udphdr *) (recvbuf + hlen1 + 8 + hlen2);
             if (hip->ip_p == IPPROTO_UDP &&
                udp->uh_sport == htons(sport) &&
                udp->uh_dport == htons(dport + seq)) {
                ret = -2;        /* we hit an intermediate router */
                break;
            }

        } else if (icmp->icmp_type == ICMP_UNREACH) {
            if (icmplen < 8 + sizeof(struct ip))
                continue;            /* not enough data to look at inner IP */

            hip = (struct ip *) (recvbuf + hlen1 + 8);
            hlen2 = hip->ip_hl << 2;
            if (icmplen < 8 + hlen2 + 4)
                continue;            /* not enough data to look at UDP ports */

            udp = (struct udphdr *) (recvbuf + hlen1 + 8 + hlen2);
             if (hip->ip_p == IPPROTO_UDP &&
                udp->uh_sport == htons(sport) &&
                udp->uh_dport == htons(dport + seq)) {
                if (icmp->icmp_code == ICMP_UNREACH_PORT)
                    ret = -1;    /* have reached destination */
                else
                    ret = icmp->icmp_code;    /* 0, 1, 2, ... */
                break;
            }
        }
        if (verbose) {
            printf(" (from %s: type = %d, code = %d)\n",
                    sock_ntop(pr->sarecv, pr->salen),
                    icmp->icmp_type, icmp->icmp_code);
        }
        /* Some other ICMP error, recvfrom() again */
    }
    alarm(0);                    /* don't leave alarm running */
    gettimeofday(tv, NULL);        /* get time of packet arrival */
    return(ret);
}


void
traceloop(void)
{
    int                    seq, code, done;
    double                rtt;
    struct rec            *rec;
    struct timeval        tvrecv;

    recvfd = socket(pr->sasend->sa_family, SOCK_RAW, pr->icmpproto);
    setuid(getuid());        /* don't need special permissions anymore */

#ifdef    IPV6
    if (pr->sasend->sa_family == AF_INET6 && verbose == 0) {
        struct icmp6_filter myfilt;
        ICMP6_FILTER_SETBLOCKALL(&myfilt);
        ICMP6_FILTER_SETPASS(ICMP6_TIME_EXCEEDED, &myfilt);
        ICMP6_FILTER_SETPASS(ICMP6_DST_UNREACH, &myfilt);
        setsockopt(recvfd, IPPROTO_IPV6, ICMP6_FILTER,
                    &myfilt, sizeof(myfilt));
    }
#endif

    sendfd = socket(pr->sasend->sa_family, SOCK_DGRAM, 0);

    pr->sabind->sa_family = pr->sasend->sa_family;
    sport = (getpid() & 0xffff) | 0x8000;    /* our source UDP port # */
    sock_set_port(pr->sabind, pr->salen, htons(sport));
    bind(sendfd, pr->sabind, pr->salen);

    sig_alrm(SIGALRM);

    seq = 0;
    done = 0;
    for (ttl = 1; ttl <= max_ttl && done == 0; ttl++) {
        setsockopt(sendfd, pr->ttllevel, pr->ttloptname, &ttl, sizeof(int));
        bzero(pr->salast, pr->salen);

        printf("%2d--> ", ttl);
        fflush(stdout);

        for (probe = 0; probe < nprobes; probe++) {
            rec = (struct rec *) sendbuf;
            rec->rec_seq = ++seq;
            rec->rec_ttl = ttl;
            gettimeofday(&rec->rec_tv, NULL);

            sock_set_port(pr->sasend, pr->salen, htons(dport + seq));
            sendto(sendfd, sendbuf, datalen, 0, pr->sasend, pr->salen);

            if ( (code = (*pr->recv)(seq, &tvrecv)) == -3)
                printf(" *");        /* timeout, no reply */
            else {
                char    str[NI_MAXHOST];

                if (sock_cmp_addr(pr->sarecv, pr->salast, pr->salen) != 0) {
                    if (getnameinfo(pr->sarecv, pr->salen, str, sizeof(str),
                                    NULL, 0, 0) == 0)
                        printf(" %s (%s)", str,
                                sock_ntop(pr->sarecv, pr->salen));
                    else
                        printf(" %s",
                                sock_ntop(pr->sarecv, pr->salen));
                    memcpy(pr->salast, pr->sarecv, pr->salen);
                }
                tv_sub(&tvrecv, &rec->rec_tv);
                rtt = tvrecv.tv_sec * 1000.0 + tvrecv.tv_usec / 1000.0;
                printf("  %.3f ms", rtt);

                if (code == -1)        /* port unreachable; at destination */
                    done++;
                else if (code >= 0)
                    printf(" (ICMP %s)", (*pr->icmpcode)(code));
            }
            fflush(stdout);
        }
        printf("\n");
    }
}



void
test_traceroute(int argc, char **argv)
{
    int                c;
    struct addrinfo    *ai;
    char *h;

    opterr = 0;        /* don't want getopt() writing to stderr */
    while ( (c = getopt(argc, argv, "m:v")) != -1) {
        switch (c) {
        case 'm':
            if ( (max_ttl = atoi(optarg)) <= 1)
                err_quit("invalid -m value");
            break;

        case 'v':
            verbose++;
            break;

        case '?':
            err_quit("unrecognized option: %c", c);
        }
    }

    if (optind != argc-1)
        err_quit("usage: traceroute [ -m <maxttl> -v ] <hostname>");
    host = argv[optind];

    pid = getpid();
    Signal(SIGALRM, sig_alrm);

    ai = Host_serv(host, NULL, 0, 0);

    h = sock_ntop(ai->ai_addr, ai->ai_addrlen);
    printf("traceroute to %s (%s): %d hops max, %d data bytes\n",
           ai->ai_canonname ? ai->ai_canonname : h,
           h, max_ttl, datalen);

        /* initialize according to protocol */
    if (ai->ai_family == AF_INET) {
        pr = &proto_v4;
#ifdef    IPV6
    } else if (ai->ai_family == AF_INET6) {
        pr = &proto_v6;
        if (IN6_IS_ADDR_V4MAPPED(&(((struct sockaddr_in6 *)ai->ai_addr)->sin6_addr)))
            err_quit("cannot traceroute IPv4-mapped IPv6 address");
#endif
    } else
        err_quit("unknown address family %d", ai->ai_family);

    pr->sasend = ai->ai_addr;        /* contains destination address */
    pr->sarecv = calloc(1, ai->ai_addrlen);
    pr->salast = calloc(1, ai->ai_addrlen);
    pr->sabind = calloc(1, ai->ai_addrlen);
    pr->salen = ai->ai_addrlen;

    traceloop();

    exit(0);
}


int
main(int argc, char **argv)
{
    test_traceroute(argc, argv);
    exit(0);
}


