#ident	"@(#)rtpm:inet.c	1.3.1.1"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stream.h>
#include <sys/protosw.h>

#include <netinet/in.h>
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

#include <netdb.h>

#include <sys/sockio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stropts.h>
#include <mas.h>
#include "rtpm.h"

struct tcpstat  tcpstat;
struct tcp_stuff	tcp_stuff;
struct udpstat  udpstat;
struct ipstat   ipstat;
struct ip_stuff   ip_stuff;
struct icmpstat icmpstat;



struct net_total net_total;

struct netstat {
	char *device;
	int fd;
	struct strioctl ioc;	/* cmd, timeout, len, buf */
} netstat[] = {
	"/dev/tcp", -2,
	{SIOCGTCPSTUFF, INFTIM,sizeof(struct  tcp_stuff),(caddr_t)&tcp_stuff},
	"/dev/udp", -2,
	{SIOCGUDPSTATS, INFTIM,sizeof(struct  udpstat),(caddr_t)&udpstat},
	"/dev/ip",  -2,
	{SIOCGIPSTUFF,  INFTIM,sizeof(struct   ip_stuff),(caddr_t)&ip_stuff},
	"/dev/icmp",-2,
	{SIOCGICMPSTATS,INFTIM,sizeof(struct icmpstat),(caddr_t)&icmpstat},
};

#define NNET (sizeof(netstat)/sizeof(struct netstat))

net_stat() {

	int i;

	for( i = 0 ; i < NNET; i++ ) {
		switch (netstat[i].fd) {
		case -1: continue;	/* open failed previously */
		case -2: netstat[i].fd = open(netstat[i].device, O_RDONLY);
		default:	/* FALLTHROUGH */
			(void)ioctl(netstat[i].fd,I_STR,
			  (caddr_t)&netstat[i].ioc);
			break;
		}
	}
	bcopy ((char *)&(tcp_stuff.tcp_stat), (char *)&tcpstat,
		sizeof (struct tcpstat));
	bcopy ((char *)&(ip_stuff.ip_stat), (char *)&ipstat,
		sizeof (struct ipstat));

	net_total.tcp = tcpstat.tcps_sndtotal + tcpstat.tcps_rcvtotal;
	net_total.icmp = icmpstat.icps_intotal + icmpstat.icps_outtotal;
	net_total.ip = 	ipstat.ips_total + ipstat.ips_outrequests;
	net_total.udp = udpstat.udps_outtotal + udpstat.udps_indelivers;
	net_total.errs = ipstat.ips_outerrors + ipstat.ips_inerrors +
	  icmpstat.icps_outerrors + icmpstat.icps_error +
	  tcpstat.tcps_inerrors + udpstat.udps_inerrors;
	  
	return(0);
}
