/*	copyright	"%c%"	*/
#ident	"@(#)ping.c	1.3"
#ident	"$Header$"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/file.h>
#include <sys/signal.h>

#include <net/if.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <ctype.h>
#include <locale.h>
#include <pfmt.h>
#include <errno.h>

#define	MAXWAIT		10	/* max time to wait for response, sec. */
#define	MAXPACKET	4096	/* max packet size */
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN	64
#endif

#ifdef SYSV
#define	bzero(s,n)	memset((s), 0, (n))
#define	bcopy(f,t,l)	memcpy((t),(f),(l))
#endif /* SYSV */

int	verbose;
int	stats;
int	flood = 0;
int	interval = 0;
int	filled = 0;
int	quiet = 0;
int	numeric = 0;
u_char	packet[MAXPACKET];
int	options;
extern	int errno;

int s;			/* Socket file descriptor */
struct hostent *hp;	/* Pointer to host info */

struct sockaddr whereto;/* Who to ping */
int datalen;		/* How much data */

u_char outpack[MAXPACKET];

struct NoMsgs{
	char	*msgno;
	char	*msgs;
};

char usage[] = ":62:Usage:\tping host [timeout]\n\tping -s[drvRlqnf] [-i intrvl] [-p patrn] host [data size] [npackets]\n";

char *hostname;
char hnamebuf[MAXHOSTNAMELEN];

int npackets;
int ntransmitted = 0;		/* sequence # for outbound packets = #sent */
int ident;

int nreceived = 0;		/* # of packets we got back */
int timing = 0;
int tmin = 999999999;
int tmax = 0;
int tsum = 0;			/* sum of all times, for doing average */
int record = 0;			/* true if using record route */
int loose = 0;			/* true if using loose source route */

# define MAX_ROUTES 9		/* maximum number of source route space */
# define ROUTE_SIZE (IPOPT_OLEN + IPOPT_OFFSET + \
	   		MAX_ROUTES * sizeof(struct in_addr))
void finish(), catcher();


#ifdef BSD
#define	setbuf(s, b)	setlinebuf((s))
#endif /* BSD */

/*
 * 			M A I N
 */
main(argc, argv)
char *argv[];
{
	struct sockaddr_in from;
	struct sockaddr_in *to = (struct sockaddr_in *) &whereto;
	int on = 1;
	int timeout = 20;
	struct protoent *proto;
	int c, n;
	u_char *datap = &outpack[8+sizeof(struct timeval)];
	struct timeval tv;
	fd_set fdmask;
	extern int optind;
	extern char *optarg;

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxping");
	(void)setlabel("UX:ping");

	while ( (c = getopt(argc, argv, "drRlvsfi:p:qn")) != -1 ) {
		switch (c) {
		case 'd':
			options |= SO_DEBUG;
			break;
		case 'r':
			options |= SO_DONTROUTE;
			break;
		case 'R':
			record = 1;
			break;
		case 'l':
			loose = 1;
			break;
		case 'v':
			verbose++;
			break;
		case 's':
			stats++;
			break;
		case 'f':
			if (getuid() != 0) {
				pfmt(stderr, MM_ERROR,
					":63:you must be root to use the -f option.\n");
				exit(1);
			}
			flood = 1;
			stats = 1;
			break;
		case 'i':
			interval = atoi(optarg);
			if (interval <= 0) {
				pfmt(stderr, MM_ERROR,
					":64:bad timing interval.\n");
				exit(1);
			}
			stats = 1;
			break;
		case 'p':
			fill(datap, optarg);
			filled = 1;
			break;
		case 'q':
			quiet = 1;
			break;
		case 'n':
			numeric = 1;
			break;
		}
	}

	if (flood && interval) {
		(void)pfmt(stderr, MM_ERROR,
		    ":65:-f and -i are incompatible options.\n");
		exit(1);
	}

	n = argc - optind;
	if ( n <= 0 ) {
		pfmt(stderr,MM_ACTION,usage);
		exit(1);
	}

	bzero( (char *)&whereto, sizeof(struct sockaddr) );
	to->sin_family = AF_INET;
	to->sin_addr.s_addr = inet_addr(argv[optind]);
	if (to->sin_addr.s_addr != -1) {
		strcpy(hnamebuf, argv[optind]);
	} else {
		hp = gethostbyname(argv[optind]);
		if (hp) {
			to->sin_family = hp->h_addrtype;
			bcopy(hp->h_addr, (caddr_t)&to->sin_addr, hp->h_length);
			strcpy(hnamebuf, hp->h_name);
		} else {
			pfmt(stderr,MM_ERROR,
				":2:unknown host %s\n", argv[optind]);
			exit(1);
		}
	}
	hostname = hnamebuf;
	if ( n >= 2 )
		datalen = atoi( argv[optind+1] );
	else
		datalen = 64-8;
	if ( n > 2 )
		npackets = atoi(argv[optind+2]);
	if (!stats && n >= 2 ) {
		npackets = 0;
		timeout = atoi(argv[optind+1]);
		datalen = 64-8;
	}
	if (datalen > MAXPACKET) {
		pfmt(stderr, MM_ERROR, ":3:packet size too large\n");
		exit(1);
	}
	if (datalen >= sizeof(struct timeval))
		timing = 1;

	if (!filled) {
		int i;

		for (i = 8; i < datalen; ++i)
			*datap++ = i;
	}

	ident = (int)getpid() & 0xFFFF;

	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
		pfmt(stderr,MM_NOGET|MM_ERROR,
			"socket: %s\n",strerror(errno));
		exit(5);
	}
	if (options & SO_DEBUG)
		setsockopt(s, SOL_SOCKET, SO_DEBUG, &on, sizeof(on));
	if (options & SO_DONTROUTE)
		setsockopt(s, SOL_SOCKET, SO_DONTROUTE, &on, sizeof(on));
	if (record || loose) {
	  /*
	   * Set the record route option
	   */
	    char srr[ROUTE_SIZE + 1];
	    int optsize = sizeof(srr);
	    
	    bzero(srr, sizeof(srr));
	    srr[0] = IPOPT_RR;
	    srr[1] = ROUTE_SIZE;
	    srr[2] = IPOPT_MINOFF;
	    if (loose) {
	        struct sockaddr_in mine; 

		srr[0] = IPOPT_LSRR;
		srr[1] = 11;
		get_myaddress( &mine );
	    	bcopy( (char *)&to->sin_addr, srr+3, sizeof(struct in_addr));
	    	bcopy( (char *)&mine.sin_addr, srr+7, sizeof(struct in_addr));
		if (record) {
		    srr[11] = IPOPT_RR;
		    srr[12] = ROUTE_SIZE - 11;
		    srr[13] = IPOPT_MINOFF;
		}
		else {
		    optsize = 12;
		}
	    }
	    if (setsockopt(s, IPPROTO_IP, IP_OPTIONS, srr, optsize) < 0)
		pfmt(stderr,MM_NOGET|MM_ERROR,
			"IP Options: %s\n",strerror(errno));
	}

	if (stats)
		pfmt(stdout, MM_NOSTD,
			":4:PING %s: %d data bytes\n", hostname, datalen );

	setbuf(stdout, (char *)0);

	if (stats)
		signal( SIGINT, finish );
	signal(SIGALRM, catcher);

	if (!flood)
		catcher();	/* start things going */

	FD_ZERO(&fdmask);

	for (;;) {
		int len = sizeof (packet);
		size_t fromlen = sizeof (from);
		int cc;

		if (flood) {
			pinger();
			tv.tv_sec = 0;
			tv.tv_usec = 10000;
			FD_SET(s, &fdmask);
			if (select(s + 1, &fdmask, (fd_set *)NULL,
			    (fd_set *)NULL, &tv) < 1)
				continue;
		}
		if (!stats && ntransmitted > timeout) {
			pfmt(stderr,MM_INFO,
				":5:no answer from %s\n", hostname);
			exit(1);
		}
		if ( (cc=recvfrom(s, packet, len, 0, (struct sockaddr *)&from, &fromlen)) < 0) {
			if( errno == EINTR )
				continue;
			pfmt(stderr,MM_NOGET|MM_ERROR,
				"recvfrom: %s\n",strerror(errno));
			continue;
		}
		pr_pack( packet, cc, &from );
		if (npackets && nreceived >= npackets)
			finish();
	}
	/*NOTREACHED*/
}

/*
 * 			C A T C H E R
 * 
 * This routine causes another PING to be transmitted, and then
 * schedules another SIGALRM for 1 second from now.
 * 
 * Bug -
 * 	Our sense of time will slowly skew (ie, packets will not be launched
 * 	exactly at 1-second intervals).  This does not affect the quality
 *	of the delay and loss statistics.
 */
void
catcher()
{
	int waittime;

	pinger();
	if (npackets == 0 || ntransmitted < npackets) {
#ifdef SYSV
		signal(SIGALRM, catcher);
#endif SYSV
		alarm(interval ? interval : 1);
	} else {
		if (nreceived) {
			waittime = 2 * tmax / 1000;
			if (waittime == 0)
				waittime = 1;
		} else
			waittime = MAXWAIT;
		signal(SIGALRM, finish);
		alarm(waittime);
	}
}

/*
 * 			P I N G E R
 * 
 * Compose and transmit an ICMP ECHO REQUEST packet.  The IP packet
 * will be added on by the kernel.  The ID field is our UNIX process ID,
 * and the sequence number is an ascending integer.  The first 8 bytes
 * of the data portion are used to hold a UNIX "timeval" struct in VAX
 * byte-order, to compute the round-trip time.
 */
pinger()
{
	register struct icmp *icp = (struct icmp *) outpack;
	int i, cc;
	register struct timeval *tp = (struct timeval *) &outpack[8];

	icp->icmp_type = loose ? ICMP_ECHOREPLY : ICMP_ECHO;
	icp->icmp_code = 0;
	icp->icmp_cksum = 0;
	icp->icmp_seq = ntransmitted++;
	icp->icmp_id = ident;		/* ID */

	cc = datalen+8;			/* skips ICMP portion */

	if (timing)
		gettimeofday( tp, (struct timezone *) NULL );

	/* Compute ICMP checksum here */
	icp->icmp_cksum = in_cksum( icp, cc );

	/* cc = sendto(s, msg, len, flags, to, tolen) */
	i = sendto( s, outpack, cc, 0, &whereto, sizeof(struct sockaddr) );

	if( i < 0 || i != cc )  {
		if( i<0 ) {
		    pfmt(stderr,MM_NOGET|MM_ERROR,
			"sendto: %s\n",strerror(errno));
		    if (!stats)
			exit(1);
		}
		pfmt(stdout,MM_NOSTD,
			":6:wrote %s %d chars, ret=%d\n",
				hostname, cc, i );
		fflush(stdout);
	}
	if (flood & !quiet)
		putchar('.');
}

/*
 * 			P R _ T Y P E
 *
 * Convert an ICMP "type" field to a printable string.
 */
char *
pr_type( t )
register int t;
{
	int	no;
	static struct NoMsgs ttab[] = {
		{ ":7", "Echo Reply" },
		{ ":8", "ICMP 1" },
		{ ":9", "ICMP 2" },
		{ ":10", "Dest Unreachable" },
		{ ":11", "Source Quence" },
		{ ":12", "Redirect" },
		{ ":13", "ICMP 6" },
		{ ":14", "ICMP 7" },
		{ ":15", "Echo" },
		{ ":16", "ICMP 9" },
		{ ":17", "ICMP 10" },
		{ ":18", "Time Exceeded" },
		{ ":19", "Parameter Problem" },
		{ ":20", "Timestamp" },
		{ ":21", "Timestamp Reply" },
		{ ":22", "Info Request" },
		{ ":23", "Info Reply" },
		{ ":24", "Netmask Request" },
		{ ":25", "Netmask Reply" }
	};

	if( t < 0 || t > 16 )
		return(gettxt(":26","OUT-OF-RANGE"));

	return(gettxt(ttab[t].msgno,ttab[t].msgs));
}

/*
 *			P R _ N A M E
 *
 * Return a string name for the given IP address.
 */
char *pr_name(addr)
    struct in_addr addr;
{
	char *inet_ntoa();
	struct hostent *phe;
	static char buf[256];

	if (numeric || !(phe = gethostbyaddr((char *)&addr.s_addr, 4, AF_INET))) {
		return( inet_ntoa(addr));
	}
	(void) sprintf(buf, "%s (%s)", phe->h_name, inet_ntoa(addr));
	return(buf);
}


/*
 *			P R _ P A C K
 *
 * Print out the packet, if it came from us.  This logic is necessary
 * because ALL readers of the ICMP socket get a copy of ALL ICMP packets
 * which arrive ('tis only fair).  This permits multiple copies of this
 * program to be run without having intermingled output (or statistics!).
 */
pr_pack( buf, cc, from )
char *buf;
int cc;
struct sockaddr_in *from;
{
	struct ip *ip;
	register struct icmp *icp;
	register long *lp = (long *) packet;
	register int i;
	struct timeval tv;
	struct timeval *tp;
	int hlen, triptime;
	struct sockaddr_in *to = (struct sockaddr_in *) &whereto;
	long w;
	char *name;
	static struct NoMsgs unreach[] = {
	    ":27", "Net Unreachable",
	    ":28", "Host Unreachable",
	    ":29", "Protocol Unreachable",
	    ":68", "Port Unreachable",
	    ":30", "Fragmentation needed and DF set",
	    ":31", "Source Route Failed"
	};
	static struct NoMsgs redirect[] = {
	    ":32", "Net",
	    ":33", "Host",
	    ":34", "TOS Net",
	    ":35", "TOS Host"
	};

	gettimeofday( &tv, (struct timezone *) NULL );
	name = pr_name(from->sin_addr);

	ip = (struct ip *) buf;
	hlen = ip->ip_hl << 2;
	if (cc < hlen + ICMP_MINLEN) {
		if (verbose)
			pfmt(stdout,MM_NOSTD,
				":36:packet too short (%d bytes) from %s\n", cc,
					name);
		return;
	}
	cc -= hlen;
	icp = (struct icmp *)(buf + hlen);
	if (ip->ip_p == 0) {
		/*
		 * Assume that we are running on a pre-4.3BSD system
		 * such as SunOS before 4.0
		 */
		 icp = (struct icmp *)buf;
	}
	switch (icp->icmp_type) {
	  case ICMP_UNREACH:
	    ip = &icp->icmp_ip;
	    if (ip->ip_dst.s_addr == to->sin_addr.s_addr || verbose) {
	      if ((icp->icmp_code >= 0) &&
		  (icp->icmp_code < (sizeof(unreach) / sizeof(struct NoMsgs)))) {
		pfmt(stdout,MM_NOSTD,":37:ICMP %s from gateway %s\n",
		     gettxt(unreach[icp->icmp_code].msgno,
			    unreach[icp->icmp_code].msgs),
		     name);
	      } else {
		pfmt(stdout,MM_NOSTD,
		     ":69:ICMP Destination Unreachable, Bad Code: %d from gateway %s\n",
		     icp->icmp_code,name);
	      }
	    }
	    break;

	  case ICMP_REDIRECT:
	    ip = &icp->icmp_ip;
	    if (ip->ip_dst.s_addr == to->sin_addr.s_addr || verbose) {
		pfmt(stdout,MM_NOSTD,
			":38:ICMP %s redirect from gateway %s\n",
				gettxt(redirect[icp->icmp_code].msgno,
					redirect[icp->icmp_code].msgs),
				name);
		pfmt(stdout,MM_NOSTD,
			":39: to %s", pr_name(icp->icmp_gwaddr) );
		pfmt(stdout,MM_NOSTD,
			":40: for %s\n", pr_name(ip->ip_dst) );
	    }
	    break;

	  case ICMP_ECHOREPLY:
	    if ( icp->icmp_id != ident )
		return;			/* 'Twas not our ECHO */

	    if (!stats) {
		pfmt(stdout,MM_NOSTD,":41:%s is alive\n", hostname);
		exit(0);
	    }

	    tp = (struct timeval *)&icp->icmp_data[0];
	    if (flood)
		putchar('\b');
	    else if (!quiet) {
	    	pfmt(stdout,MM_NOSTD,":42:%d bytes from %s: ", cc, name);
	    	pfmt(stdout,MM_NOSTD,":43:icmp_seq=%d. ", icp->icmp_seq );
	    }
	    if (timing) {
		tvsub( &tv, tp );
		triptime = tv.tv_sec*1000+(tv.tv_usec/1000);
		if (!(flood || quiet))
			pfmt(stdout,MM_NOSTD,":44:time=%d. ms\n", triptime );
		tsum += triptime;
		if ( triptime < tmin )
			tmin = triptime;
		if ( triptime > tmax )
			tmax = triptime;
	    } else if (!(flood || quiet))
		putchar('\n');
	    nreceived++;
	    break;

	  default:
	    if (verbose) {
		pfmt(stdout,MM_NOSTD,
			":45:%d bytes from %s:\n", cc, name);
		pfmt(stdout, MM_NOSTD,
			":46:icmp_type=%d (%s) ",
				icp->icmp_type, pr_type(icp->icmp_type) );
		pfmt(stdout,MM_NOSTD,
			":47:icmp_code=%d\n", icp->icmp_code );
		for( i=0; i<12; i++)
		    printf("x%2.2x: x%8.8x\n", i*sizeof(long), *lp++ );
	    }
	    break;
	}
	buf += sizeof(struct ip);
	hlen -= sizeof(struct ip);
	if (verbose && hlen > 0)
		pr_options(buf, hlen);
}

/*
 *		P R _ O P T I O N S
 * Print out the ip options.
 */
pr_options(opt, optlength)
	unsigned char *opt;
	int optlength;
{
    int curlength;

    pfmt(stdout,MM_NOSTD,":48:  IP options: ");
    while (optlength > 0) {
       curlength = opt[1];
       switch (*opt) {
         case IPOPT_EOL:
	    optlength = 0;
	    break;
	 
	 case IPOPT_NOP:
	    opt++;
	    optlength--;
	    continue;
	 
	 case IPOPT_RR:
	    pfmt(stdout,MM_NOSTD,":49: <record route> ");
	    ip_rrprint(opt, curlength);
	    break;
	 
	 case IPOPT_TS:
	    pfmt(stdout,MM_NOSTD,":50: <time stamp>");
	    break;
	 
	 case IPOPT_SECURITY:
	    pfmt(stdout,MM_NOSTD,":51: <security>");
	    break;
	 
	 case IPOPT_LSRR:
	    pfmt(stdout,MM_NOSTD,":52: <loose source route> ");
	    ip_rrprint(opt, curlength);
	    break;
	 
	 case IPOPT_SATID:
	    pfmt(stdout,MM_NOSTD,":53: <stream id>");
	    break;
	 
	 case IPOPT_SSRR:
	    pfmt(stdout,MM_NOSTD,":54: <strict source route> ");
	    ip_rrprint(opt, curlength);
	    break;
	 
	 default:
	    pfmt(stdout,MM_NOSTD,":55: <option %d, len %d>",
		*opt, curlength);
	    break;
       }
       /*
        * Following most options comes a length field
	*/
	opt += curlength;
	optlength -= curlength;
    }
    printf("\n");
  }


/*
 * Print out a recorded route option.
 */
ip_rrprint(opt, length)
    unsigned char *opt;
    int length;
{
    int pointer;
    struct in_addr addr;

    opt += IPOPT_OFFSET;
    length -= IPOPT_OFFSET;

    pointer = *opt++;
    pointer -= IPOPT_MINOFF;
    length--;
    while (length > 0) {
	bcopy((char *)opt, (char *)&addr, sizeof(addr));
	printf( "%s", pr_name(addr) );
	if (pointer == 0)
	    pfmt(stdout,MM_NOSTD,":56:(Current)");
	opt += sizeof(addr);
	length -= sizeof(addr);
	pointer -= sizeof(addr);
	if (length >0)
	    printf( ", ");
    }
}


/*
 *			I N _ C K S U M
 *
 * Checksum routine for Internet Protocol family headers (C Version)
 *
 */
in_cksum(addr, len)
u_short *addr;
int len;
{
	register int nleft = len;
	register u_short *w = addr;
	register u_short answer;
	u_short odd_byte = 0;
	register int sum = 0;

	/*
	 *  Our algorithm is simple, using a 32 bit accumulator (sum),
	 *  we add sequential 16 bit words to it, and at the end, fold
	 *  back all the carry bits from the top 16 bits into the lower
	 *  16 bits.
	 */
	while( nleft > 1 )  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if( nleft == 1 ) {
		*(u_char *)(&odd_byte) = *(u_char *)w;
		sum += odd_byte;
	}

	/*
	 * add back carry outs from top 16 bits to low 16 bits
	 */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return (answer);
}

/*
 * 			T V S U B
 * 
 * Subtract 2 timeval structs:  out = out - in.
 * 
 * Out is assumed to be >= in.
 */
tvsub( out, in )
register struct timeval *out, *in;
{
	if( (out->tv_usec -= in->tv_usec) < 0 )   {
		out->tv_sec--;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}

/*
 *			F I N I S H
 *
 * Print out statistics, and give up.
 * Heavily buffered STDIO is used here, so that all the statistics
 * will be written with 1 sys-write call.  This is nice when more
 * than one copy of the program is running on a terminal;  it prevents
 * the statistics output from becomming intermingled.
 */
void
finish()
{
	pfmt(stdout,MM_NOSTD,
		":57:\n----%s PING Statistics----\n", hostname );
	pfmt(stdout,MM_NOSTD,
		":58:%d packets transmitted, ", ntransmitted );
	pfmt(stdout,MM_NOSTD,
		":59:%d packets received, ", nreceived );
	if (ntransmitted)
	    pfmt(stdout,MM_NOSTD,":60:%d%% packet loss",
		(int) (((ntransmitted-nreceived)*100) / ntransmitted ) );
	printf("\n");
	if (nreceived && timing)
	    pfmt(stdout,MM_NOSTD,
		":61:round-trip (ms)  min/avg/max = %d/%d/%d\n",
			tmin,
			tsum / nreceived,
			tmax );
	fflush(stdout);
	exit(0);
}



/*
 * Get host's IP address via ioctl.
 */

static
get_myaddress(addr)
	struct sockaddr_in *addr;
{
	int soc;
	char buf[1024];
	struct ifconf ifc;
	struct ifreq ifreq, *ifr;
	int len;

#ifdef SYSV
	if ((soc = open("/dev/ip", O_RDWR)) < 0) {
		pfmt(stderr,MM_NOGET|MM_ERROR,
			"open /dev/ip: %s\n",strerror(errno));
		exit(1);
	}
#else
	if ((soc = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		pfmt(stderr,MM_NOGET|MM_ERROR,
		    "socket(AF_INET, SOCK_DGRAM, 0): %s\n",strerror(errno));
		exit(1);
	}
#endif /* SYSV */
	bzero((char *) &ifc, sizeof(ifc));
	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = &buf[0];
	if (ifioctl(soc, SIOCGIFCONF, (char *)&ifc) < 0) {
		pfmt(stderr,MM_NOGET|MM_ERROR,
			"ioctl (SIOCGIFCONF): %s\n",strerror(errno));
		exit(1);
	}
	ifr = ifc.ifc_req;
	for (len = ifc.ifc_len; len; len -= sizeof ifreq) {
		ifreq = *ifr;
		if (ifioctl(soc, SIOCGIFFLAGS, (char *)&ifreq) < 0) {
			pfmt(stderr,MM_NOGET|MM_ERROR,
				"ioctl (SIOCGIFFLAGS): %s\n",strerror(errno));
			exit(1);
		}
		if ((ifreq.ifr_flags & IFF_UP) &&
		    ifr->ifr_addr.sa_family == AF_INET) {
			bzero ((char *) addr, sizeof (struct sockaddr_in));
			*addr = *((struct sockaddr_in *)&ifr->ifr_addr);
			break;
		}
		ifr++;
	}
	(void) close(soc);
}


#ifdef SYSV
#include <sys/stropts.h>
#endif SYSV

ifioctl(s, cmd, arg)
        int s;
        int cmd;
        char *arg;
{
#ifdef SYSV
        struct strioctl ioc;
        int ret;

	bzero((char *) &ioc, sizeof(ioc));
        ioc.ic_cmd = cmd;
        ioc.ic_timout = 0;
        if (cmd == SIOCGIFCONF) {
                ioc.ic_len = ((struct ifconf *) arg)->ifc_len;
                ioc.ic_dp = ((struct ifconf *) arg)->ifc_buf;
        } else {
                ioc.ic_len = sizeof(struct ifreq);
                ioc.ic_dp = arg;
        }
        ret = ioctl(s, I_STR, (char *) &ioc);
        if (ret != -1 && cmd == SIOCGIFCONF)
                ((struct ifconf *) arg)->ifc_len = ioc.ic_len;
        return(ret);
#else
        return (ioctl(s, cmd, arg);
#endif SYSV
}

fill(bp, patp)
	char *bp, *patp;
{
	register int ii, jj, kk;
	int pat[16];
	char *cp;

	for (cp = patp; *cp; cp++)
		if (!isxdigit(*cp)) {
			(void)pfmt(stderr, MM_ERROR,
			    ":66:patterns must be specified as hex digits.\n");
			exit(1);
		}
	ii = sscanf(patp,
	    "%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x",
	    &pat[0], &pat[1], &pat[2], &pat[3], &pat[4], &pat[5], &pat[6],
	    &pat[7], &pat[8], &pat[9], &pat[10], &pat[11], &pat[12],
	    &pat[13], &pat[14], &pat[15]);

	if (ii > 0)
		for (kk = 0; kk <= MAXPACKET - (8 + ii); kk += ii)
			for (jj = 0; jj < ii; ++jj)
				bp[jj + kk] = pat[jj];
	if (!quiet) {
		(void)pfmt(stdout, MM_NOSTD, ":67:PATTERN: 0x");
		for (jj = 0; jj < ii; ++jj)
			(void)printf("%02x", bp[jj] & 0xFF);
		(void)printf("\n");
	}
}

