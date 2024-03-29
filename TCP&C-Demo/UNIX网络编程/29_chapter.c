//
//  28_chapter_dns.c
//  TCP&C-Demo
//
//  Created by 西木柚子 on 2019/12/2.
//  Copyright © 2019 feisu. All rights reserved.
//

#include    "unp.c"
#include    <pcap.h>
#include    <netinet/in_systm.h>    /* required for ip.h */
#include    <netinet/in.h>
#include    <netinet/ip.h>
#include    <netinet/ip_var.h>
#include    <netinet/udp.h>
#include    <netinet/udp_var.h>
#include    <net/if.h>
#include    <netinet/if_ether.h>
#include    <setjmp.h>


#define    TTL_OUT        64                /* outgoing TTL */

                    /* declare global variables */
struct sockaddr    *dest, *local;
socklen_t        destlen, locallen;
int         datalink;
char        *device;
pcap_t      *pd;
int        rawfd;
int        snaplen;
int        verbose;
int        zerosum;
struct udpiphdr *udp_read(void);
struct sockaddr    *dest, *local;
struct sockaddr_in locallookup;
socklen_t        destlen, locallen;
int        datalink;        /* from pcap_datalink(), in <net/bpf.h> */
char       *device;            /* pcap device */
pcap_t     *pd;                /* packet capture struct pointer */
int        rawfd;            /* raw socket to write on */
int        snaplen = 200;    /* amount of data to capture */
int        verbose;
int        zerosum;
int        rawfd;            /* raw socket to write on */


void             cleanup(int);
char            *next_pcap(int *);
void             open_output(void);
void             open_pcap(void);
void             send_dns_query(void);
void             test_udp(void);
void             udp_write(char *, int);
static void    usage(const char *);
static sigjmp_buf    jmpbuf;
static int            canjump;
struct udpiphdr    *udp_check(char *, int);



//MARK:- UDP read

/*
 * Read from the network until a UDP datagram is read that matches
 * the arguments.
 */

/* include udp_read */
struct udpiphdr *
udp_read(void)
{
    int                    len;
    char                *ptr;
    struct ether_header    *eptr;

    for ( ; ; ) {
        ptr = next_pcap(&len);

        switch (datalink) {
        case DLT_NULL:    /* loopback header = 4 bytes */
            return(udp_check(ptr+4, len-4));

        case DLT_EN10MB:
            eptr = (struct ether_header *) ptr;
            if (ntohs(eptr->ether_type) != ETHERTYPE_IP)
                err_quit("Ethernet type %x not IP", ntohs(eptr->ether_type));
            return(udp_check(ptr+14, len-14));

        case DLT_SLIP:    /* SLIP header = 24 bytes */
            return(udp_check(ptr+24, len-24));

        case DLT_PPP:    /* PPP header = 24 bytes */
            return(udp_check(ptr+24, len-24));

        default:
            err_quit("unsupported datalink (%d)", datalink);
        }
    }
}
/* end udp_read */

/*
 * Check the received packet.
 * If UDP and OK, return pointer to packet.
 * If ICMP error, return NULL.
 * We assume the filter picks out desired UDP datagrams.
 */

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


/* include udp_check */
struct udpiphdr *
udp_check(char *ptr, int len)
{
    int                    hlen;
    struct ip            *ip;
    struct udpiphdr        *ui;
/* *INDENT-OFF* */

    if (len < sizeof(struct ip) + sizeof(struct udphdr))
        err_quit("len = %d", len);
/* *INDENT-ON* */

        /* 4minimal verification of IP header */
    ip = (struct ip *) ptr;
    if (ip->ip_v != IPVERSION)
        err_quit("ip_v = %d", ip->ip_v);
    hlen = ip->ip_hl << 2;
/* *INDENT-OFF* */
    if (hlen < sizeof(struct ip))
        err_quit("ip_hl = %d", ip->ip_hl);
    if (len < hlen + sizeof(struct udphdr))
        err_quit("len = %d, hlen = %d", len, hlen);
/* *INDENT-ON* */

    if ( (ip->ip_sum = in_cksum((uint16_t *) ip, hlen)) != 0)
        err_quit("ip checksum error");

    if (ip->ip_p == IPPROTO_UDP) {
        ui = (struct udpiphdr *) ip;
        return(ui);
    } else
        err_quit("not a UDP packet");
    return NULL;
}
/* end udp_check */



//MARK:- UDP Write

void
open_output(void)
{
    int    on=1;
    /*
     * Need a raw socket to write our own IP datagrams to.
     * Process must have superuser privileges to create this socket.
     * Also must set IP_HDRINCL so we can write our own IP headers.
     */

    rawfd = socket(dest->sa_family, SOCK_RAW, 0);

    setsockopt(rawfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on));
}
/* end open_output_raw */

/*
 * "buf" points to an empty IP/UDP header,
 * followed by "ulen" bytes of user data.
 */

/* include udp_write */
void
udp_write(char *buf, int userlen)
{
    struct udpiphdr        *ui;
    struct ip            *ip;

        /* 4fill in and checksum UDP header */
    ip = (struct ip *) buf;
    ui = (struct udpiphdr *) buf;
    bzero(ui, sizeof(*ui));
            /* 8add 8 to userlen for pseudoheader length */
    ui->ui_len = htons((uint16_t) (sizeof(struct udphdr) + userlen));
            /* 8then add 28 for IP datagram length */
    userlen += sizeof(struct udpiphdr);

    ui->ui_pr = IPPROTO_UDP;
    ui->ui_src.s_addr = ((struct sockaddr_in *) local)->sin_addr.s_addr;
    ui->ui_dst.s_addr = ((struct sockaddr_in *) dest)->sin_addr.s_addr;
    ui->ui_sport = ((struct sockaddr_in *) local)->sin_port;
    ui->ui_dport = ((struct sockaddr_in *) dest)->sin_port;
    ui->ui_ulen = ui->ui_len;
    if (zerosum == 0) {
#if 1    /* change to if 0 for Solaris 2.x, x < 6 */
        if ( (ui->ui_sum = in_cksum((u_int16_t *) ui, userlen)) == 0)
            ui->ui_sum = 0xffff;
#else
        ui->ui_sum = ui->ui_len;
#endif
    }

        /* 4fill in rest of IP header; */
        /* 4ip_output() calcuates & stores IP header checksum */
    ip->ip_v = IPVERSION;
    ip->ip_hl = sizeof(struct ip) >> 2;
    ip->ip_tos = 0;
#if defined(linux) || defined(__OpenBSD__)
    ip->ip_len = htons(userlen);    /* network byte order */
#else
    ip->ip_len = userlen;            /* host byte order */
#endif
    ip->ip_id = 0;            /* let IP set this */
    ip->ip_off = 0;            /* frag offset, MF and DF flags */
    ip->ip_ttl = TTL_OUT;

    sendto(rawfd, buf, userlen, 0, dest, destlen);
}
/* end udp_write */


//MARK:- UDP DNS TEST
void
sig_alrm(int signo)
{
    if (canjump == 0)
        return;
    siglongjmp(jmpbuf, 1);
}
/* end sig_alrm */

/* include test_udp */
void
test_udp(void)
{
    volatile int    nsent = 0, timeout = 3;
    struct udpiphdr    *ui;

    Signal(SIGALRM, sig_alrm);

    if (sigsetjmp(jmpbuf, 1)) {
        if (nsent >= 3)
            err_quit("no response");
        printf("timeout\n");
        timeout *= 2;        /* exponential backoff: 3, 6, 12 */
    }
    canjump = 1;    /* siglongjmp is now OK */

    send_dns_query();
    nsent++;

    alarm(timeout);
    ui = udp_read();
    canjump = 0;
    alarm(0);

    if (ui->ui_sum == 0)
        printf("UDP checksums off\n");
    else
        printf("UDP checksums on\n");
    if (verbose)
        printf("received UDP checksum = %x\n", ntohs(ui->ui_sum));
}
/* end test_udp */


//MARK:- send dns query

/*
 * Build a DNS A query for "a.root-servers.net" and write it to
 * the raw socket.
 */

/* include send_dns_query */
void
send_dns_query(void)
{
    size_t        nbytes;
    char        *buf, *ptr;

    buf = malloc(sizeof(struct udpiphdr) + 100);
    ptr = buf + sizeof(struct udpiphdr);/* leave room for IP/UDP headers */

    *((uint16_t *) ptr) = htons(1234);    /* identification */
    ptr += 2;
    *((uint16_t *) ptr) = htons(0x0100);    /* flags: recursion desired */
    ptr += 2;
    *((uint16_t *) ptr) = htons(1);        /* # questions */
    ptr += 2;
    *((uint16_t *) ptr) = 0;            /* # answer RRs */
    ptr += 2;
    *((uint16_t *) ptr) = 0;            /* # authority RRs */
    ptr += 2;
    *((uint16_t *) ptr) = 0;            /* # additional RRs */
    ptr += 2;

    memcpy(ptr, "\001a\014root-servers\003net\000", 20);
    ptr += 20;
    *((uint16_t *) ptr) = htons(1);        /* query type = A */
    ptr += 2;
    *((uint16_t *) ptr) = htons(1);        /* query class = 1 (IP addr) */
    ptr += 2;

    nbytes = (ptr - buf) - sizeof(struct udpiphdr);
    udp_write(buf, nbytes);
    if (verbose)
        printf("sent: %zu bytes of data\n", nbytes);
}
/* end send_dns_query */


//MARK:- capture packet
#define    CMD        "udp and src host %s and src port %d"

void
open_pcap(void)
{
    uint32_t            localnet, netmask;
    char                cmd[MAXLINE], errbuf[PCAP_ERRBUF_SIZE],
                        str1[INET_ADDRSTRLEN], str2[INET_ADDRSTRLEN];
    struct bpf_program    fcode;

    if (device == NULL) {
        if ( (device = pcap_lookupdev(errbuf)) == NULL)
            err_quit("pcap_lookup: %s", errbuf);
    }
    printf("device = %s\n", device);

        /* 4hardcode: promisc=0, to_ms=500 */
    if ( (pd = pcap_open_live(device, snaplen, 0, 500, errbuf)) == NULL)
        err_quit("pcap_open_live: %s", errbuf);

    if (pcap_lookupnet(device, &localnet, &netmask, errbuf) < 0)
        err_quit("pcap_lookupnet: %s", errbuf);
    if (verbose)
        printf("localnet = %s, netmask = %s\n",
               inet_ntop(AF_INET, &localnet, str1, sizeof(str1)),
               inet_ntop(AF_INET, &netmask, str2, sizeof(str2)));

    snprintf(cmd, sizeof(cmd), CMD,
             sock_ntop(dest, destlen),
             ntohs(sock_get_port(dest, destlen)));
    if (verbose)
        printf("cmd = %s\n", cmd);
    if (pcap_compile(pd, &fcode, cmd, 0, netmask) < 0)
        err_quit("pcap_compile: %s", pcap_geterr(pd));

    if (pcap_setfilter(pd, &fcode) < 0)
        err_quit("pcap_setfilter: %s", pcap_geterr(pd));

    if ( (datalink = pcap_datalink(pd)) < 0)
        err_quit("pcap_datalink: %s", pcap_geterr(pd));
    if (verbose)
        printf("datalink = %d\n", datalink);
}
/* end open_pcap */

/* include next_pcap */
char *
next_pcap(int *len)
{
    char                *ptr;
    struct pcap_pkthdr    hdr;

        /* 4keep looping until packet ready */
    while ( (ptr = (char *) pcap_next(pd, &hdr)) == NULL)
        ;

    *len = hdr.caplen;    /* captured length */
    return(ptr);
}
/* end next_pcap */





//MARK:- main 函数

int
main(int argc, char *argv[])
{
    int                c, lopt=0;
    char            *ptr, localname[1024], *localport;
    struct addrinfo    *aip;
/* end main1 */

/* include main2 */
    opterr = 0;        /* don't want getopt() writing to stderr */
    while ( (c = getopt(argc, argv, "0i:l:v")) != -1) {
        switch (c) {

        case '0':
            zerosum = 1;
            break;

        case 'i':
            device = optarg;            /* pcap device */
            break;

        case 'l':            /* local IP address and port #: a.b.c.d.p */
            if ( (ptr = strrchr(optarg, '.')) == NULL)
                usage("invalid -l option");

            *ptr++ = 0;                    /* null replaces final period */
            localport = ptr;            /* service name or port number */
            strncpy(localname, optarg, sizeof(localname));
            lopt = 1;
            break;

        case 'v':
            verbose = 1;
            break;

        case '?':
            usage("unrecognized option");
        }
    }
/* end main2 */
/* include main3 */
    if (optind != argc-2)
        usage("missing <host> and/or <serv>");

        /* 4convert destination name and service */
    aip = Host_serv(argv[optind], argv[optind+1], AF_INET, SOCK_DGRAM);
    dest = aip->ai_addr;        /* don't freeaddrinfo() */
    destlen = aip->ai_addrlen;

    /*
     * Need local IP address for source IP address for UDP datagrams.
     * Can't specify 0 and let IP choose, as we need to know it for
     * the pseudoheader to calculate the UDP checksum.
     * If -l option supplied, then use those values; otherwise,
     * connect a UDP socket to the destination to determine the right
     * source address.
     */
    if (lopt) {
            /* 4convert local name and service */
        aip = Host_serv(localname, localport, AF_INET, SOCK_DGRAM);
        local = aip->ai_addr;        /* don't freeaddrinfo() */
        locallen = aip->ai_addrlen;
    } else {
        int s;
        s = socket(AF_INET, SOCK_DGRAM, 0);
        connect(s, dest, destlen);
        /* kernel chooses correct local address for dest */
        locallen = sizeof(locallookup);
        local = (struct sockaddr *)&locallookup;
        getsockname(s, local, &locallen);
        if (locallookup.sin_addr.s_addr == htonl(INADDR_ANY))
            err_quit("Can't determine local address - use -l\n");
        close(s);
    }

    open_output();        /* open output, either raw socket or libnet */

    open_pcap();        /* open packet capture device */

    setuid(getuid());    /* don't need superuser privileges anymore */

    Signal(SIGTERM, cleanup);
    Signal(SIGINT, cleanup);
    Signal(SIGHUP, cleanup);

    test_udp();

    cleanup(0);
}
/* end main3 */

static void
usage(const char *msg)
{
    err_msg(
"usage: udpcksum [ options ] <host> <serv>\n"
"options: -0    send UDP datagram with checksum set to 0\n"
"         -i s  packet capture device\n"
"         -l a.b.c.d.p  local IP=a.b.c.d, local port=p\n"
"         -v    verbose output"
);

    if (msg[0] != 0)
        err_quit("%s", msg);
    exit(1);
}


void
cleanup(int signo)
{
    struct pcap_stat    stat;

    putc('\n', stdout);

    if (verbose) {
        if (pcap_stats(pd, &stat) < 0)
            err_quit("pcap_stats: %s\n", pcap_geterr(pd));
        printf("%d packets received by filter\n", stat.ps_recv);
        printf("%d packets dropped by kernel\n", stat.ps_drop);
    }

    exit(0);
}
