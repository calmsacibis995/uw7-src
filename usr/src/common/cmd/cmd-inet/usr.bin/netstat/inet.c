#ident "@(#)inet.c	1.3"
#ident "$Header$"

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <paths.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/strstat.h>

#include <netinet/in.h>
#include <netinet/in_mp.h>
#include <net/route.h>
#include <netinet/in_systm.h>
#include <netinet/in_pcb.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcpip.h>
#include <netinet/tcp_seq.h>
#define TCPSTATES
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcp_debug.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netinet/igmp.h>
#include <netinet/igmp_var.h>

#include <netdb.h>
#include "proto.h"

struct inpcb    *inp;
struct tcpcb    *tcp_ptr;
extern int      kmem;
extern int      Aflag;
extern int      aflag;
extern int      nflag;


/*
 * Print a summary of connections related to an Internet protocol.  For TCP,
 * also give state of connection. Listening processes (aflag) are suppressed
 * unless the -a (all) flag is specified. 
 */
protopr(name)
	char           *name;
{
	struct strioctl strioc;
	struct gi_arg *gp, gi_arg;
	int    istcp, isudp, isip;
	int    fd, block_size, ioctl_cmd, size;
	static int    first = 1;
	char *buf, *next, *lim,  DEVICE[32], *prev;

	istcp = strcmp(name, "tcp") == 0;
	isudp = strcmp(name, "udp") == 0;
	isip = strcmp(name, "ip") == 0;

	if (istcp) {
		strcpy(DEVICE, _PATH_TCP);
		ioctl_cmd = STIOCGTCB;
		block_size = sizeof(struct inpcb) + sizeof(struct tcpcb);
	} else {
		block_size = sizeof(struct inpcb);
		if (isudp) {
			strcpy(DEVICE, _PATH_UDP);
			ioctl_cmd = STIOCGUDB;
		}
		else {
			strcpy(DEVICE, _PATH_RIP);
			ioctl_cmd = STIOCGRCB;
		}
	}

	if ((fd = open(DEVICE, O_RDONLY)) < 0) {
		fprintf(stderr,"netstat: ");
		perror(DEVICE);
		exit(1);
	}

	strioc.ic_cmd = SIOCSMGMT;
	strioc.ic_dp = (char *)0;
	strioc.ic_len = 0;
	strioc.ic_timout = -1;

	if (ioctl(fd, I_STR, &strioc) < 0) {
		fprintf(stderr,"netstat: ");
		perror("protopr: ioctl: SIOCSMGMT");
		exit(1);
	}

	gi_arg.gi_size = 0;
	gi_arg.gi_where = (caddr_t)&gi_arg;

	if ((size = ioctl(fd, ioctl_cmd, &gi_arg)) < 0) {
		fprintf(stderr,"netstat: ");
		perror("protopr: ioctl");
		exit(1);
	}

	if (size == 0) {
		/* persistent links can cause this to be true for IP */
		return;
	}

	size = (11 * size) / 10;
	if ((buf = (char *)malloc(size * block_size)) == NULL) {
		fprintf(stderr,"netstat: ");
		perror("protopr: malloc");
		exit(1);
	}

	gp = (struct gi_arg *)buf;
	gp->gi_size = size;
	gp->gi_where = (caddr_t)buf;

	if ((size = ioctl(fd, ioctl_cmd, buf)) < 0) {
		fprintf(stderr,"netstat: ");
		perror("protopr: ioctl");
		(void) free(buf);
		exit(1);
	}

	lim = buf + size * block_size; 
	prev = buf;
	for (next = buf; next < lim; next += block_size) {

		inp = (struct inpcb *)next;

		if (!aflag && inet_lnaof(inp->inp_laddr) == INADDR_ANY)
			continue;

		if (istcp)
			tcp_ptr = (struct tcpcb *) (next + sizeof(*inp));

		if (first) {
			printf("Active Internet connections");
			if (aflag)
				printf(" (including servers)");
			putchar('\n');
			if (Aflag)
				printf("%-8.8s ", "PCB");
			printf(Aflag ?
				"%-5.5s %-6.6s %-6.6s  %-18.18s %-18.18s %s\n" :
				"%-5.5s %-6.6s %-6.6s  %-22.22s %-22.22s %s\n",
			       "Proto", "Recv-Q", "Send-Q",
			       "Local Address", "Foreign Address", "(state)");
			first = 0;
		}

		if (Aflag)
			if (istcp)
				printf("%8x ", inp->inp_ppcb);
			else
				printf("%8x ", ((inpcb_t *)prev)->inp_next);
		printf("%-5.5s %6d %6d ", name,
			istcp ? tcp_ptr->t_iqsize : 0,
			istcp ? tcp_ptr->t_qsize : 0);
		inetprint(&(inp->inp_laddr), inp->inp_lport, name);
		inetprint(&(inp->inp_faddr), inp->inp_fport, name);
		if (istcp) {
			if (tcp_ptr->t_state < 0 || tcp_ptr->t_state >= TCP_NSTATES)
				printf(" %d", tcp_ptr->t_state);
			else
				printf(" %s", tcpstates[tcp_ptr->t_state]);
		}
		putchar('\n');
		prev = next;
	}

	(void) close(fd);
	(void) free(buf);

}

/*
 * Dump TCP statistics structure. 
 */
tcp_stats(name)
	char           *name;
{
	int  fd;
	struct tcpstat  tcpstat;
	struct tcp_stuff tcp_stuff;
	struct strioctl strioc;

	if ((fd = open(_PATH_TCP, O_RDONLY)) < 0) {
		fprintf(stderr,"netstat: ");
		perror(_PATH_TCP);
		exit(1);
	}

	strioc.ic_cmd = SIOCGTCPSTUFF;
	strioc.ic_dp = (char *)&tcp_stuff;
	strioc.ic_len = sizeof(struct tcp_stuff);
	strioc.ic_timout = -1;

	if (ioctl(fd, I_STR, &strioc) < 0) {
		fprintf(stderr,"netstat: ");
		perror("tcp_stats: ioctl: SIOCGTCPSTUFF");
		exit(1);
	}

	bcopy ((char *)&(tcp_stuff.tcp_stat), (char *)&tcpstat,
		sizeof (struct tcpstat));

	printf("%s:\n", name);

#define	p(f, m)		printf(m, tcpstat.f, plural(tcpstat.f))
#define	p2(f1, f2, m)	printf(m, tcpstat.f1, plural(tcpstat.f1), tcpstat.f2, plural(tcpstat.f2))
  
	p(tcps_sndtotal, "\t%lu packet%s sent\n");
	p(tcps_tookfast, "\t\t%lu packet%s used fast path\n");
	p2(tcps_sndpack,tcps_sndbyte,
		"\t\t%lu data packet%s (%lu byte%s)\n");
	p2(tcps_sndrexmitpack, tcps_sndrexmitbyte,
		"\t\t%lu data packet%s (%lu byte%s) retransmitted\n");
	p2(tcps_sndacks, tcps_delack,
		"\t\t%lu ack-only packet%s (%lu delayed)\n");
	p(tcps_sndurg, "\t\t%lu URG only packet%s\n");
	p(tcps_sndprobe, "\t\t%lu window probe packet%s\n");
	p(tcps_sndwinup, "\t\t%lu window update packet%s\n");
	p(tcps_sndctrl, "\t\t%lu control packet%s\n");
	p(tcps_sndrsts, "\t\t\t%lu reset%s\n");

	p(tcps_rcvtotal, "\t%lu packet%s received\n");
	p2(tcps_rcvackpack, tcps_rcvackbyte, "\t\t%lu ack%s (for %lu byte%s)\n");
	p(tcps_rcvdupack, "\t\t%lu duplicate ack%s\n");
	p(tcps_rcvacktoomuch, "\t\t%lu ack%s for unsent data\n");
	p2(tcps_rcvpack, tcps_rcvbyte,
		"\t\t%lu packet%s (%lu byte%s) received in-sequence\n");
	p2(tcps_rcvduppack, tcps_rcvdupbyte,
		"\t\t%lu completely duplicate packet%s (%lu byte%s)\n");
	p2(tcps_rcvpartduppack, tcps_rcvpartdupbyte,
		"\t\t%lu packet%s with some dup. data (%lu byte%s duped)\n");
	p2(tcps_rcvoopack, tcps_rcvoobyte,
		"\t\t%lu out-of-order packet%s (%lu byte%s)\n");
	p2(tcps_rcvpackafterwin, tcps_rcvbyteafterwin,
		"\t\t%lu packet%s (%lu byte%s) of data after window\n");
	p(tcps_rcvwinprobe, "\t\t%lu window probe%s\n");
	p(tcps_rcvwinupd, "\t\t%lu window update packet%s\n");
	p(tcps_rcvafterclose, "\t\t%lu packet%s received after close\n");
	p(tcps_rcvbadsum, "\t\t%lu discarded for bad checksum%s\n");
	p(tcps_rcvbadoff, "\t\t%lu discarded for bad header offset field%s\n");
	p(tcps_rcvshort, "\t\t%lu discarded because packet too short\n");
	p(tcps_inerrors,
		"\t\t%lu system error%s encountered during processing\n");

	p(tcps_connattempt, "\t%lu connection request%s\n");
	p(tcps_accepts, "\t%lu connection accept%s\n");
	p(tcps_connects, "\t%lu connection%s established (including accepts)\n");
	p2(tcps_closed, tcps_drops,
		"\t%lu connection%s closed (including %lu drop%s)\n");
	p(tcps_conndrops, "\t%lu embryonic connection%s dropped\n");
	p(tcps_attemptfails, "\t%lu failed connect and accept request%s\n");
	p(tcps_estabresets, "\t%lu reset%s received while established\n");
	p2(tcps_rttupdated, tcps_segstimed,
		"\t%lu segment%s updated rtt (of %lu attempt%s)\n");

	p(tcps_rexmttimeo, "\t%lu retransmit timeout%s\n");
	p(tcps_timeoutdrop, "\t\t%lu connection%s dropped by rexmit timeout\n");

	p(tcps_persisttimeo, "\t%lu persist timeout%s\n");
	p(tcps_resched, "\t\t%lu alloc failure%s caused reschedule\n");

	p(tcps_keeptimeo, "\t%lu keepalive timeout%s\n");
	p(tcps_keepprobe, "\t\t%lu keepalive probe%s sent\n");
	p(tcps_keepdrops, "\t\t%lu connection%s dropped by keepalive\n");
	p(tcps_preddat, "\t%lu segment%s predicted\n");
	p(tcps_predack, "\t%lu ack%s predicted\n");

	(void) close(fd);
#undef p
#undef p2
}

/*
 * Dump UDP statistics structure. 
 */
udp_stats(name)
	char           *name;
{
	int  fd;
	struct udpstat  udpstat;
	struct strioctl strioc;

	if ((fd = open(_PATH_UDP, O_RDONLY)) < 0) {
		fprintf(stderr,"netstat: ");
		perror(_PATH_UDP);
		exit(1);
	}

	strioc.ic_cmd = SIOCGUDPSTATS;
	strioc.ic_dp = (char *)&udpstat;
	strioc.ic_len = sizeof(struct udpstat);
	strioc.ic_timout = -1;

	if (ioctl(fd, I_STR, &strioc) < 0) {
		fprintf(stderr,"netstat: ");
		perror("udp_stats: ioctl: SIOCGUDPSTATS");
		exit(1);
	}

	printf("%s:\n\t%lu incomplete header%s\n", name,
	       udpstat.udps_hdrops, plural(udpstat.udps_hdrops));
	printf("\t%lu bad data length field%s\n",
	       udpstat.udps_badlen, plural(udpstat.udps_badlen));
	printf("\t%lu bad checksum%s\n",
	       udpstat.udps_badsum, plural(udpstat.udps_badsum));
/*
	printf("\t%lu full socket%s\n",
	       udpstat.udps_fullsock, plural(udpstat.udps_fullsock));
 */
	printf("\t%lu bad port%s (%lu %s broadcast/multicast)\n",
	       udpstat.udps_noports, plural(udpstat.udps_noports),
	       udpstat.udps_noportbcast, waswere(udpstat.udps_noportbcast));
	printf("\t%lu input packet%s delivered\n",
	       udpstat.udps_indelivers, plural(udpstat.udps_indelivers));
	printf("\t%lu system error%s during input\n",
	       udpstat.udps_inerrors, plural(udpstat.udps_inerrors));
	printf("\t%lu packet%s sent\n",
	       udpstat.udps_outtotal, plural(udpstat.udps_outtotal));
	printf("\t%lu streams allocation failure%s\n",
	       udpstat.udps_nosr, plural(udpstat.udps_nosr));

	(void) close(fd);
}

/*
 * Dump IP statistics structure. 
 */
ip_stats(name)
	char           *name;
{
	int  fd;
	struct ipstat   ipstat;
	struct ip_stuff ip_stuff;
	struct strioctl strioc;

	if ((fd = open(_PATH_IP, O_RDONLY)) < 0) {
		fprintf(stderr,"netstat: ");
		perror(_PATH_IP);
		exit(1);
	}

	strioc.ic_cmd = SIOCGIPSTUFF;
	strioc.ic_dp = (char *)&ip_stuff;
	strioc.ic_len = sizeof(struct ip_stuff);
	strioc.ic_timout = -1;

	if (ioctl(fd, I_STR, &strioc) < 0) {
		fprintf(stderr,"netstat: ");
		perror("ip_stats: ioctl: SIOCGIPSTUFF");
		exit(1);
	}

	bcopy ((char *)&(ip_stuff.ip_stat), (char *)&ipstat,
		sizeof (struct ipstat));

	printf("%s:\n\t%lu total packets received\n", name,
	       ipstat.ips_total);
	printf("\t%lu bad header checksum%s\n",
	       ipstat.ips_badsum, plural(ipstat.ips_badsum));
	printf("\t%lu with bad IP version number\n", ipstat.ips_badver);
	printf("\t%lu with size smaller than minimum\n", ipstat.ips_tooshort);
	printf("\t%lu with data size < data length\n", ipstat.ips_toosmall);
	printf("\t%lu with header length < data size\n", ipstat.ips_badhlen);
	printf("\t%lu with data length < header length\n", ipstat.ips_badlen);
	printf("\t%lu with unknown protocol\n", ipstat.ips_unknownproto);
	printf("\t%lu with link layer broadcast addr but unicast IP addr\n",
			ipstat.ips_badbcast);
	printf("\t%lu fragment%s received\n",
	       ipstat.ips_fragments, plural(ipstat.ips_fragments));
	printf("\t%lu fragment%s dropped (dup or out of space)\n",
	       ipstat.ips_fragdropped, plural(ipstat.ips_fragdropped));
	printf("\t%lu fragment%s dropped after timeout\n",
	       ipstat.ips_fragtimeout, plural(ipstat.ips_fragtimeout));
	printf("\t%lu packet%s reassembled\n",
	       ipstat.ips_reasms, plural(ipstat.ips_reasms));
	printf("\t%lu packet%s forwarded\n",
	       ipstat.ips_forward, plural(ipstat.ips_forward));
	printf("\t%lu packet%s not forwardable\n",
	       ipstat.ips_cantforward, plural(ipstat.ips_cantforward));
	printf("\t%lu no routes\n", ipstat.ips_noroutes);
	printf("\t%lu redirect%s sent\n",
	       ipstat.ips_redirectsent, plural(ipstat.ips_redirectsent));
	printf("\t%lu system error%s during input\n",
	       ipstat.ips_inerrors, plural(ipstat.ips_inerrors));
	printf("\t%lu packet%s delivered\n",
	       ipstat.ips_indelivers, plural(ipstat.ips_indelivers));
	printf("\t%lu total packet%s sent\n",
	       ipstat.ips_outrequests, plural(ipstat.ips_outrequests));
	printf("\t%lu system error%s during output\n",
	       ipstat.ips_outerrors, plural(ipstat.ips_outerrors));
	printf("\t%lu packet%s fragmented\n",
	       ipstat.ips_pfrags, plural(ipstat.ips_pfrags));
	printf("\t%lu packet%s not fragmentable\n",
	       ipstat.ips_fragfails, plural(ipstat.ips_fragfails));
	printf("\t%lu fragment%s created\n",
	       ipstat.ips_frags, plural(ipstat.ips_frags));
	printf("\t%lu PCB%s failed connect due to bad source address\n",
		ipstat.ips_badsrcaddr, plural(ipstat.ips_badsrcaddr));

	(void) close(fd);
}

static char    *icmpnames[] = {
			       "echo reply",
			       "#1",
			       "#2",
			       "destination unreachable",
			       "source quench",
			       "routing redirect",
			       "#6",
			       "#7",
			       "echo",
			       "router advertisement",
			       "router solicitation",
			       "time exceeded",
			       "parameter problem",
			       "time stamp",
			       "time stamp reply",
			       "information request",
			       "information request reply",
			       "address mask request",
			       "address mask reply",
};

/*
 * Dump ICMP statistics. 
 */
icmp_stats(name)
	char           *name;
{
	struct icmpstat icmpstat;
	register int    i, fd, first;
	struct strioctl strioc;

	if ((fd = open(_PATH_ICMP, O_RDONLY)) < 0) {
		fprintf(stderr,"netstat: ");
		perror(_PATH_ICMP);
		exit(1);
	}

	strioc.ic_cmd = SIOCSMGMT;
	strioc.ic_dp = (char *)0;
	strioc.ic_len = 0;
	strioc.ic_timout = -1;

	if (ioctl(fd, I_STR, &strioc) < 0) {
		fprintf(stderr,"netstat: ");
		perror("protopr: ioctl: SIOCSMGMT");
		exit(1);
	}

	strioc.ic_cmd = SIOCGICMPSTATS;
	strioc.ic_dp = (char *)&icmpstat;
	strioc.ic_len = sizeof(struct icmpstat);
	strioc.ic_timout = -1;

	if (ioctl(fd, I_STR, &strioc) < 0) {
		fprintf(stderr,"netstat: ");
		perror("icmp_stats: ioctl: SIOCGICMPSTATS");
		exit(1);
	}

	printf("%s:\n\t%lu call%s to icmp_error\n", name,
	       icmpstat.icps_error, plural(icmpstat.icps_error));
	printf("\t%lu error%s not generated because old message was icmp\n",
	       icmpstat.icps_oldicmp, plural(icmpstat.icps_oldicmp));
	for (first = 1, i = 0; i < ICMP_MAXTYPE + 1; i++)
		if (icmpstat.icps_outhist[i] != 0) {
			if (first) {
				printf("\tOutput histogram:\n");
				first = 0;
			}
			printf("\t\t%s: %lu\n", icmpnames[i],
			       icmpstat.icps_outhist[i]);
		}
	printf("\t%lu message%s with bad code fields\n",
	       icmpstat.icps_badcode, plural(icmpstat.icps_badcode));
	printf("\t%lu message%s < minimum length\n",
	       icmpstat.icps_tooshort, plural(icmpstat.icps_tooshort));
	printf("\t%lu bad checksum%s\n",
	       icmpstat.icps_checksum, plural(icmpstat.icps_checksum));
	printf("\t%lu message%s with bad length\n",
	       icmpstat.icps_badlen, plural(icmpstat.icps_badlen));
	printf("\t%lu bad netmask%s received\n",
	       icmpstat.icps_badmask, plural(icmpstat.icps_badmask));
	for (first = 1, i = 0; i < ICMP_MAXTYPE + 1; i++)
		if (icmpstat.icps_inhist[i] != 0) {
			if (first) {
				printf("\tInput histogram:\n");
				first = 0;
			}
			printf("\t\t%s: %lu\n", icmpnames[i],
			       icmpstat.icps_inhist[i]);
		}
	printf("\t%lu message response%s generated\n",
	       icmpstat.icps_reflect, plural(icmpstat.icps_reflect));
	printf("\t%lu message%s received\n",
	       icmpstat.icps_intotal, plural(icmpstat.icps_intotal));
	printf("\t%lu message%s sent\n",
	       icmpstat.icps_outtotal, plural(icmpstat.icps_outtotal));
	printf("\t%lu message%s not sent due to bad source address\n",
	       icmpstat.icps_badaddr, plural(icmpstat.icps_badaddr));
	printf("\t%lu system error%s during output\n",
	       icmpstat.icps_outerrors, plural(icmpstat.icps_outerrors));

	(void) close(fd);
}

/*
 * Pretty print an Internet address (net address + port). If the nflag was
 * specified, use numbers instead of names. 
 */
int
inetprint(
	register struct in_addr *in,
	u_short         port,
	char           *proto
	)
{
	struct servent *sp = 0;
	char            line[1024], *cp, *strchr();
	int             width;

	sprintf(line, "%.*s.", (Aflag && !nflag) ? 12 : 16, inetname(*in));
	cp = strchr(line, '\0');
	if (!nflag && port)
		sp = getservbyport((int)port, proto);
	if (sp || port == 0)
		sprintf(cp, "%.8s", sp ? sp->s_name : "*");
	else
		sprintf(cp, "%u", ntohs((u_short) port) & 0xffff);
	width = Aflag ? 18 : 22;
	printf(" %-*.*s", width, width, line);
}

/*
 * Construct an Internet address representation. If the nflag has been
 * supplied, give numeric value, otherwise try for symbolic name. 
 */
char           *
inetname(in)
	struct in_addr  in;
{
	register char  *cp;
	struct hostent *hp;
	static char     line[MAXHOSTNAMELEN + 1];
	struct netent  *np;
	static char     domain[MAXHOSTNAMELEN + 1];
	static int      first = 1;
	extern char    *strchr();

	if (first && !nflag) {
		first = 0;
		if (gethostname(domain, MAXHOSTNAMELEN) == 0 &&
		    (cp = strchr(domain, '.')))
			(void) strcpy(domain, cp + 1);
		else
			domain[0] = 0;
	}
	cp = 0;
	if (!nflag && in.s_addr != INADDR_ANY) {
		u_long             net = inet_netof(in);
		u_long             lna = inet_lnaof(in);

		if (lna == INADDR_ANY) {
			np = getnetbyaddr(net, AF_INET);
			if (np)
				cp = np->n_name;
		}
		if (cp == 0) {
			hp = gethostbyaddr((char *)&in, sizeof(in), AF_INET);
			if (hp) {
				if ((cp = strchr(hp->h_name, '.')) &&
				    !strcasecmp(cp + 1, domain))
					*cp = 0;
				cp = hp->h_name;
			}
		}
	}
	if (in.s_addr == INADDR_ANY)
		strcpy(line, "*");
	else if (cp)
		strcpy(line, cp);
	else {
		in.s_addr = ntohl(in.s_addr);
#define C(x)	((x) & 0xff)
		sprintf(line, "%u.%u.%u.%u", C(in.s_addr >> 24),
			C(in.s_addr >> 16), C(in.s_addr >> 8), C(in.s_addr));
	}
	return (line);
}

/*
 * Dump IGMP statistics.
 */
igmp_stats(name)
	char *name;
{
	struct igmpstat igmpstat;
	register int    fd;
	struct strioctl strioc;

	if ((fd = open(_PATH_IGMP, O_RDONLY)) < 0) {
		fprintf(stderr,"netstat: ");
		perror(_PATH_IGMP);
		exit(1);
	}

	strioc.ic_cmd = SIOCSMGMT;
	strioc.ic_dp = (char *)0;
	strioc.ic_len = 0;
	strioc.ic_timout = -1;

	if (ioctl(fd, I_STR, &strioc) < 0) {
		fprintf(stderr,"netstat: ");
		perror("protopr: ioctl: SIOCSMGMT");
		exit(1);
	}

	strioc.ic_cmd = SIOCGIGMPSTATS;
	strioc.ic_dp = (char *)&igmpstat;
	strioc.ic_len = sizeof(struct igmpstat);
	strioc.ic_timout = -1;

	if (ioctl(fd, I_STR, &strioc) < 0) {
		fprintf(stderr,"netstat: ");
		perror("igmp_stats: ioctl: SIOCGIGMPSTATS");
		exit(1);
	}

	printf("%s:\n", name );
	printf("\t%lu message%s received\n",
	  igmpstat.igps_rcv_total, plural(igmpstat.igps_rcv_total));
	printf("\t%lu message%s received with too few bytes\n",
	  igmpstat.igps_rcv_tooshort, plural(igmpstat.igps_rcv_tooshort));
	printf("\t%lu message%s received with bad checksum\n",
	  igmpstat.igps_rcv_badsum, plural(igmpstat.igps_rcv_badsum));
	printf("\t%lu membership quer%s received\n",
	  igmpstat.igps_rcv_queries, pluraly(igmpstat.igps_rcv_queries));
	printf("\t%lu membership quer%s received with invalid field(s)\n",
	  igmpstat.igps_rcv_badqueries, pluraly(igmpstat.igps_rcv_badqueries));
	printf("\t%lu membership report%s received\n",
	  igmpstat.igps_rcv_reports, plural(igmpstat.igps_rcv_reports));
	printf("\t%lu membership report%s received with invalid field(s)\n",
	  igmpstat.igps_rcv_badreports, plural(igmpstat.igps_rcv_badreports));
	printf("\t%lu membership report%s received for groups to which we belong\n",
	  igmpstat.igps_rcv_ourreports, plural(igmpstat.igps_rcv_ourreports));
	printf("\t%lu membership report%s sent\n",
	  igmpstat.igps_snd_reports, plural(igmpstat.igps_snd_reports));
	printf("\t%lu total packet%s sent\n",
	  igmpstat.igps_outtotal, plural(igmpstat.igps_outtotal));
	printf("\t%lu output error%s\n",
	  igmpstat.igps_outerrors, plural(igmpstat.igps_outerrors));
}

void
lockstats()
{
	struct in_lockstats il;
	int f, r;
	struct strioctl si;

	f = open(_PATH_IP, O_RDONLY);
	if (f < 0) {
		fprintf(stderr,"netstat: ");
		perror(_PATH_IP);
		exit(1);
	}

	si.ic_cmd = SIOCGINLOCKSTATS;
	si.ic_dp = (char *)&il;
	si.ic_timout = -1;
	si.ic_len = sizeof(il);

	r = ioctl(f, I_STR, (char *)&si);
	if (r < 0) {
		fprintf(stderr,"netstat: ");
		perror("SIOCGINLOCKSTATS");
		exit(1);
	}
	close(f);

	printf("Global Multiprocessor Locking Statistics:\n");
	printf("%-10.10s%12.12s%12.12s\n", "Lock", "Read", "Write");

	printf("%-10.10s%12lu%12lu\n", "arp", il.ils_arp_rd, il.ils_arp_wr);
	printf("%-10.10s%12lu%12lu\n", "cache", il.ils_proto_rd, 
		il.ils_proto_wr);
	printf("%-10.10s%12lu%12lu\n", "icmp", il.ils_icmp_rd, il.ils_icmp_wr);
	printf("%-10.10s%12lu%12lu\n", "icmpqbot", il.ils_icmpq_rd, il.ils_icmpq_wr);
	printf("%-10.10s%12lu%12lu\n", "ifnet", il.ils_ifnet_rd, 
		il.ils_ifnet_wr);
	printf("%-10.10s%12lu%12lu\n", "ifstats", il.ils_ifstats_rd, 
		il.ils_ifstats_wr);
	printf("%-10.10s%12lu%12lu\n", "igmp", il.ils_igmp_rd, il.ils_igmp_wr);
	printf("%-10.10s%12lu%12lu\n", "igmpqbot", il.ils_igmpq_rd, il.ils_igmpq_wr);
	printf("%-10.10s%12lu%12lu\n", "in_rnhead", il.ils_rnhead_rd, 
		il.ils_rnhead_wr);
	printf("%-10.10s%12lu%12lu\n", "incf", il.ils_incf_rd, il.ils_incf_wr);
	printf("%-10.10s%12lu%12lu\n", "ip_pcb", il.ils_ip_pcb_rd, 
		il.ils_ip_pcb_wr);
	printf("%-10.10s%12lu%12lu\n", "ipq", il.ils_ipq_rd, 
		il.ils_ipq_wr);
	printf("%-10.10s%12lu%12lu\n", "rip", il.ils_rip_rd, il.ils_rip_wr);
	printf("%-10.10s%12lu%12lu\n", "ripqbot", il.ils_ripq_rd, il.ils_ripq_wr);
	printf("%-10.10s%12lu%12lu\n", "rte", il.ils_rte_rd, il.ils_rte_wr);
	printf("%-10.10s%12lu%12lu\n", "tcp", il.ils_tcp_rd, il.ils_tcp_wr);
	printf("%-10.10s%12lu%12lu\n", "tcpqbot", il.ils_tcpq_rd, il.ils_tcpq_wr);
	printf("%-10.10s%12lu%12lu\n", "tcptr", il.ils_tcptr_rd, 
		il.ils_tcptr_wr);
	printf("%-10.10s%12lu%12lu\n", "tcpconn", il.ils_tcpconn_rd, 
		il.ils_tcpconn_wr);
	printf("%-10.10s%12lu%12lu\n", "timer", il.ils_timer_rd, il.ils_timer_wr);
	printf("%-10.10s%12lu%12lu\n", "udp", il.ils_udp_rd, il.ils_udp_wr);
	printf("%-10.10s%12lu%12lu\n", "udpqbot", il.ils_udpq_rd, il.ils_udpq_wr);
}
