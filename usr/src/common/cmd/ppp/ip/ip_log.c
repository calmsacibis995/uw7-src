#ident	"@(#)ip_log.c	1.6"

#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <synch.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <net/if.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <aas/aas.h>
#include <sys/ppp_ip.h>
#include <sys/ppp.h>
#include <sys/times.h>

#include "ppp_type.h"
#include "ppp_cfg.h"
#include "psm.h"
#include "act.h"
#include "lcp_hdr.h"
#include "ip_cfg.h"
#include "ppp_proto.h"

STATIC int
tcp_log(proto_hdr_t *pr, char *dp, int len)
{
	struct tcphdr *tcp = (struct tcphdr *)dp;
	struct servent *se;

	psm_log(MSG_INFO, pr, "TCP header (len now %d)\n", len);
	psm_log(MSG_INFO, pr, "src port = %d dst port = %d\n",
		 ntohs(tcp->th_sport), ntohs(tcp->th_dport));

	se = getservbyport(ntohs(tcp->th_sport), "tcp");
	if (se && se->s_name) {
		psm_log(MSG_INFO, pr, "src service = %s\n", se->s_name);
	}
	se = getservbyport(ntohs(tcp->th_dport), "tcp");
	if (se && se->s_name) {
		psm_log(MSG_INFO, pr, "dst service = %s\n", se->s_name);
	}

	psm_log(MSG_INFO, pr, "seq = %lu ack = %lu\n",
		 ntohl(tcp->th_seq), ntohl(tcp->th_ack));
	psm_log(MSG_INFO, pr, "off = %d flags = %x window = %u\n",
		 tcp->th_x2_off, tcp->th_flags, ntohs(tcp->th_win) & 0xffff);
	psm_log(MSG_INFO, pr, "chk sum = 0x%x urgp = %x\n",
		 ntohs(tcp->th_sum) & 0xffff,
		 ntohs(tcp->th_urp) & 0xffff);
}

STATIC int
udp_log(proto_hdr_t *pr, char *dp, int len)
{
	struct udphdr *udp = (struct udphdr *)dp;
	struct servent *se;

	psm_log(MSG_INFO, pr, "UDP header (len now %d)\n", len);
	psm_log(MSG_INFO, pr, "src port = %d dst port = %d\n",
		 ntohs(udp->uh_sport), ntohs(udp->uh_dport));

	se = getservbyport(ntohs(udp->uh_sport), "udp");
	if (se) {
		psm_log(MSG_INFO, pr, "got se\n");
	}
	if (se && se->s_name) {
		psm_log(MSG_INFO, pr, "src service = %s\n", se->s_name);
	}
	se = getservbyport(ntohs(udp->uh_dport), "udp");
	if (se && se->s_name) {
		psm_log(MSG_INFO, pr, "dst service = %s\n", se->s_name);
	}

	psm_log(MSG_INFO, pr, "udp len = 0x%4.4x sum = 0x%4.4x\n",
		 ntohs(udp->uh_ulen) & 0xffff,
		 ntohs(udp->uh_sum) & 0xffff);
}

STATIC char *icmp_msgtype[] = {
	"ECHO Reply",
	"1",
	"2",
	"Unreachable",
	"Source Quench",
	"Redirect",
	"6",
	"7",
	"ECHO Request",
	"Router Advertisment",
	"Router Solicitation",
	"Time Exceeded",
	"IP Header Bad",
	"Timestamp Request",
	"Timestamp Reply",
	"Address mask Request",
	"Address mask Reply",
};

STATIC int
icmp_log(proto_hdr_t *pr, char *dp, int len)
{
	struct icmp *icmp = (struct icmp *)dp;
	struct timeval *tp;
	struct tm local_time;
	char tbuff[20];

	psm_log(MSG_INFO, pr, "ICMP header (len now %d)\n", len);

	if (icmp->icmp_type <= ICMP_MAXTYPE) {
		psm_log(MSG_INFO, pr,
			"Type = %s (%d)\n",
			icmp_msgtype[icmp->icmp_type], icmp->icmp_type);
	} else {
		psm_log(MSG_INFO, pr,
			"Type = Invalid (%d)\n", icmp->icmp_type);
	}

	psm_log(MSG_INFO, pr, "Code = %d Checksum = 0x%4.4x\n",
		icmp->icmp_code, ntohs(icmp->icmp_cksum));


	/* Just decode echo request/reply for now */
	switch (icmp->icmp_type) {
	case ICMP_ECHO:
	case ICMP_ECHOREPLY:
		psm_log(MSG_INFO, pr, "Identifier = %d\n",
			/*ntohs*/(icmp->icmp_id));
		psm_log(MSG_INFO, pr, "Sequence Number = %d\n",
			/*ntohs*/(icmp->icmp_seq));

		/* The icmp_data contains a timeval ... decode it */
		if (len - (&icmp->icmp_data[0] - dp) >= 
		    sizeof(struct timeval)) {

			tp = (struct timeval *)&icmp->icmp_data[0];
			localtime_r(&(tp->tv_sec), &local_time);
			strftime(tbuff, 20, "%T", &local_time);
			psm_log(MSG_INFO, pr,
				"TimeStamp %s.%2.2d\n", tbuff,
				tp->tv_usec/10000);
		}
		break;
	default:
		psm_log(MSG_INFO, pr, "No further decode\n");
	}
}

int
ip_log(proto_hdr_t *pr, char *dp, int len)
{
	struct ip *ip = (struct ip *)dp;
	char buf[80];
	char *proto = NULL;
	int frag = 0;

	frag = ntohs(ip->ip_off) & 0x1fff;

	switch (ip->ip_p) {
	case IPPROTO_ICMP:
		proto= "ICMP";
		break;
	case IPPROTO_TCP:
		proto = "TCP";
		break;
	case IPPROTO_UDP:
		proto = "UDP";
		break;
	default:
		break;
	}		

	psm_log(MSG_INFO, pr, "\n");
	psm_log(MSG_INFO, pr, "%s %s (total %d bytes received)\n",
		 proto ? proto : "IP",
		 ntohs(ip->ip_off) & 0x3fff ? 
		 (ntohs(ip->ip_off) & 0x1fff ?
		 "subsequent fragment"  : "first fragment")
		 : "packet",
		 len);

	psm_log(MSG_INFO, pr, "---------\n");
	psm_log(MSG_INFO, pr, "ver = %d head len = %d tos = %d\n",
		 ip->ip_v, ip->ip_hl << 2, ip->ip_tos);
	psm_log(MSG_INFO, pr,
		 "tot len = 0x%4.4x id = %u  ttl = 0x%2.2x\n",
		 ntohs(ip->ip_len) & 0xffff,
		 ntohs(ip->ip_id) & 0xffff,
		 ip->ip_ttl);

	psm_log(MSG_INFO, pr, "frag offset 0x%4.4x MF %s DF %s\n",
		 ntohs(ip->ip_off) & 0x1fff,
		 ntohs(ip->ip_off) & IP_MF ? "true" : "false",
		       ntohs(ip->ip_off) & IP_DF ? "true" : "false");

	psm_log(MSG_INFO, pr, "sum = 0x%4.4x\n",
		 ntohs(ip->ip_sum)& 0xffff);
	sprintf(buf, "src = %s dst = ", (char *)inet_ntoa(ip->ip_src));
	strcat(buf, (char *)inet_ntoa(ip->ip_dst));
	psm_log(MSG_INFO, pr, "%s\n", buf);

	len -= ip->ip_hl << 2;

	switch (ip->ip_p) {
	case IPPROTO_IP:
		psm_log(MSG_INFO, pr, "protocol = DUMMY for IP\n");
		break;
	case IPPROTO_ICMP:
		/*psm_log(MSG_INFO, pr, "protocol = ICMP\n");*/
		if (!(ntohs(ip->ip_off) & 0x1fff))
			icmp_log(pr, dp + (ip->ip_hl << 2), len);
		break;
	case IPPROTO_TCP:
		if (!(ntohs(ip->ip_off) & 0x1fff))
			tcp_log(pr, dp + (ip->ip_hl << 2), len);
		break;
	case IPPROTO_UDP:
		if (!(ntohs(ip->ip_off) & 0x1fff))
			udp_log(pr, dp + (ip->ip_hl << 2), len);
		break;
	default:
		psm_log(MSG_INFO, pr, "protocol = %d\n", ip->ip_p);
		break;
	}		

}
