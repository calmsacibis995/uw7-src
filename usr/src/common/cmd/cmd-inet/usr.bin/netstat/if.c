#ident "@(#)if.c	1.7"
#ident "$Header$"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/strstat.h>
#include <fcntl.h>
#include <paths.h>

#include <net/if.h>
#ifdef notyet
#include <net/if_dl.h>
#endif
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include "proto.h"

#include <sys/dlpi.h>
#include <sys/scodlpi.h>
#include <sys/mdi.h>

#define	YES	1
#define	NO	0

extern	int tflag;
extern	int dflag;
extern	int nflag;
extern	char *interface;
extern	int unit;

extern int aflag;

int
get_if_all(if_all)
	struct ifreq_all *if_all;
{
	int  fd;
	struct strioctl strioc;

	if ((fd = open(_PATH_IP, O_RDONLY)) < 0) {
		fprintf(stderr,"netstat: ");
		perror(_PATH_IP);
		exit(1);
	}

	strioc.ic_cmd = SIOCGIFALL;
	strioc.ic_dp = (char *)if_all;
	strioc.ic_len = sizeof(struct ifreq_all);
	strioc.ic_timout = -1;

	if (ioctl(fd, I_STR, &strioc) < 0) {
		perror("netstat: get_if_all: SIOCGIFALL");
		(void) close(fd);
		exit(1);
	}

	(void) close(fd);
	return(0);
}

int
get_ifcnt()
{
	int  fd;
	struct strioctl strioc;
	struct ifreq ifr;

	if ((fd = open(_PATH_IP, O_RDONLY)) < 0) {
		fprintf(stderr,"netstat: ");
		perror(_PATH_IP);
		exit(1);
	}

	strioc.ic_cmd = SIOCGIFNUM;
	strioc.ic_dp = (char *)&ifr;
	strioc.ic_len = sizeof(struct ifreq);
	strioc.ic_timout = -1;

	if (ioctl(fd, I_STR, &strioc) < 0) {
		perror("netstat: get_ifcnt: SIOCGIFNUM");
		(void) close(fd);
		exit(1);
	}

	(void) close(fd);
	return ifr.ifr_nif;
}

/*
 * Print a description of the network interfaces.
 */
intpr(interval)
	int interval;
{
	struct in_ifaddr ifaddr;
	struct sockaddr *sa;
	char name[IFNAMSIZ];
	char origname[IFNAMSIZ];
	struct ifstats ifstat;
	struct ifreq_all if_all;
	int prev_idx, curr_if, r, max_if;
	int ifn;

	if (interval) {
		sidewaysintpr((unsigned)interval);
		return;
	}

	printf("%-5.5s %-5.5s %-11.11s %-15.15s %-8.8s %-5.5s %-8.8s %-6.6s",
		"Name", "Mtu", "Network", "Address", "Ipkts", "Ierrs",
		"Opkts", "Oerrs ");
	printf(" %-5s", "Coll");
#ifdef notyet
	if (tflag)
		printf(" %s", "Time");
	if (dflag)
		printf(" %s", "Drop");
#endif
	putchar('\n');

	max_if = get_ifcnt();
	prev_idx = 0;
	curr_if = 1;
	while (curr_if <= max_if) {
		struct sockaddr_in *sin;
		register char *cp;
		int n, m;
		int idx;

		if_all.if_entry.if_index = ++prev_idx;
		get_if_all (&if_all);
		ifn = if_all.if_naddr;
		prev_idx = if_all.if_entry.if_index;
again:
		idx = if_all.if_naddr - ifn;
		bcopy ((char *)(if_all.if_entry.if_name), (char *)name, IFNAMSIZ);
		name[IFNAMSIZ - 1] = '\0';
		/*
		 * save name so that appending '*' if iface is
		 * down will not screw up stats
		 */
		strcpy(origname, name);
		if (interface != 0 && strcmp(name, interface) != 0) {
				curr_if++;
				continue;
		}
		cp = strchr(name, '\0');
		if ((if_all.if_entry.if_flags & IFF_UP) == 0)
			*cp++ = '*';
		*cp = '\0';

		printf("%-5.5s %-5u ", name, if_all.if_entry.if_maxtu);
		if (if_all.if_naddr == 0) {
			printf("%-11.11s ", "none");
			printf("%-15.15s ", "none");
		} else {
			bcopy ((char *)&(if_all.addrs[idx].addr),
				(char *) &(ifaddr.ia_addr),
				sizeof(struct sockaddr_in));
			bcopy ((char *)&(if_all.addrs[idx].netmask),
				(char *) &(ifaddr.ia_sockmask),
				sizeof(struct sockaddr_in));

			ifaddr.ia_subnetmask = ntohl(ifaddr.ia_sockmask.sin_addr.s_addr); 
			ifaddr.ia_subnet = (ifaddr.ia_subnetmask
				 & ifaddr.ia_addr.sin_addr.s_addr);
			sa = (struct sockaddr *)&(ifaddr.ia_addr);

			switch (sa->sa_family) {
			case AF_UNSPEC:
				printf("%-11.11s ", "none");
				printf("%-15.15s ", "none");
				break;
			case AF_INET:
				sin = (struct sockaddr_in *)sa;
				{	struct in_addr z;
					z.s_addr = ifaddr.ia_subnet;
				printf("%-11.15s ",
					netname(z,
						htonl(ifaddr.ia_subnetmask),-1));
				}
				printf("%-15.15s ", routename(sin->sin_addr));
				break;
#ifdef notyet
			case AF_LINK:
				{
				    struct sockaddr_dl *sdl =
						(struct sockaddr_dl *)sa;
			    	    cp = (char *)LLADDR(sdl);
			    	    n = sdl->sdl_alen;
				}
				m = printf("<Link>");
				goto hexprint;
#endif
			default:
				m = printf("(%d)", sa->sa_family);
				for (cp = sizeof(struct sockaddr) + (char *)sa;
					--cp > sa->sa_data && (*cp == 0);) {}
				n = cp - sa->sa_data + 1;
				cp = sa->sa_data;
			hexprint:
				while (--n >= 0)
					m += printf("%x%c", *cp++ & 0xff,
						    n > 0 ? '.' : ' ');
				m = 28 - m;
				while (m-- > 0)
					putchar(' ');
				break;
			}
		}
		if (if_all.if_statsavail == 1) {
			r = if_all.if_statsavail;
			bcopy ((char *)&(if_all.if_stats),
				(char *)&ifstat, sizeof(struct ifstats));
		}
		else
			r = getifstats(origname, &ifstat);

		if (r) {
			printf("%-8u %-5u %-8u %-6u %-u",
			    ifstat.ifs_ipackets, ifstat.ifs_ierrors,
			    ifstat.ifs_opackets, ifstat.ifs_oerrors,
			    ifstat.ifs_collisions);
		} else {
			printf("No Statistics Available");
		}
#ifdef notyet
		if (tflag)
			printf(" %3u", if_all.if_entry.if_timer);
		if (dflag)
			printf(" %3u", if_all.if_entry.if_snd.ifq_drops);
#endif
		putchar('\n');

		if (aflag) {
			int i = 0;
			struct in_addr z;
			while (if_all.addrs[idx].multi[i]) {
				z.s_addr = if_all.addrs[idx].multi[i];
				printf("%23s %-19.19s\n", "", routename(z));
				i++;
			}
		}
	
		/* If we're already done with the desired interface */
		if (interface != 0 && strcmp(name, interface) == 0)
			break;
		ifn--;
		if (ifn > 0)
			goto again;
		curr_if++;
	}
}

struct	iftot {
	char	ift_name[IFNAMSIZ];	/* interface name */
	int	ift_ip;			/* input packets */
	int	ift_ie;			/* input errors */
	int	ift_op;			/* output packets */
	int	ift_oe;			/* output errors */
	int	ift_co;			/* collisions */
	int	ift_dr;			/* drops */
};

struct iftot *iftot;

u_char	signalled;			/* set if alarm goes off "early" */

/*
 * Print a running summary of interface statistics.
 * Repeat display every interval seconds, showing statistics
 * collected over that interval.  Assumes that interval is non-zero.
 * First line printed at top of screen is always cumulative.
 */
sidewaysintpr(interval)
	unsigned interval;
{
	struct ifstats ifstat;
	struct ifreq_all if_all;
	register struct iftot *ip, *total;
	register int line;
	struct iftot *lastif, *sum, *interesting = 0;
	int oldmask;
	int prev_idx, curr_if;
	void catchalarm();
	char name[IFNAMSIZ];
	int s, r;
	int ifnum = 0;
	struct ifreq ifr;
	struct strioctl si;

	s = open(_PATH_IP, O_RDWR);
	if (s < 0) {
		fprintf(stderr, "netstat: ");
		perror(_PATH_IP);
		exit(1);
	}

	si.ic_len = sizeof(struct ifreq);
	si.ic_dp = (char *)&ifr;
	si.ic_timout = -1;
	si.ic_cmd = SIOCGIFANUM;
	r = ioctl(s, I_STR, (char *)&si);
	if (r < 0) {
		perror("netstat: SIOCGIFANUM");
		exit(1);
	}

	ifnum = ifr.ifr_nif;
	iftot = (struct iftot *)malloc(sizeof(*iftot) * (ifnum + 3));
	if (iftot == 0) {
		fprintf(stderr,"netstat: iftot malloc failed\n");
		exit(1);
	}

	bzero((char *)iftot, sizeof(*iftot) * (ifnum + 3));
	lastif = iftot;
	sum = iftot + ifnum + 1;
	total = sum - 1;
	interesting = iftot;

	for (curr_if = 1, prev_idx = 0, ip = iftot; curr_if <= ifnum;
		curr_if++) {
		char *cp;

		if_all.if_entry.if_index = ++prev_idx;
		get_if_all (&if_all);
		prev_idx = if_all.if_entry.if_index;

		bzero(ip->ift_name, sizeof(ip->ift_name));
		ip->ift_name[0] = '(';
		bcopy ((char *)(if_all.if_entry.if_name),
			(char *)(ip->ift_name + 1), (IFNAMSIZ-1));
		if (interface && (strcmp(ip->ift_name + 1, interface) == 0))
			interesting = ip;
		ip->ift_name[IFNAMSIZ - 1] = '\0';
		cp = strchr(ip->ift_name, '\0');
		*cp++ = ')';
		*cp++ = '\0';
		ip++;
	}
	lastif = ip;
	(void)sigset(SIGALRM, (void(*)())catchalarm);
	signalled = NO;
	(void)alarm(interval);
banner:
	printf("   input    %-6.6s    output       ", interesting->ift_name);
	if (lastif - iftot > 0) {
#ifdef notyet
		if (dflag)
			printf("      ");
#endif
		printf("     input   (Total)    output");
	}
	for (ip = iftot; ip < iftot + ifnum; ip++) {
		ip->ift_ip = 0;
		ip->ift_ie = 0;
		ip->ift_op = 0;
		ip->ift_oe = 0;
		ip->ift_co = 0;
		ip->ift_dr = 0;
	}
	putchar('\n');
	printf("%8.8s %6.6s %8.8s %6.6s %6.6s ",
		"packets", "errs", "packets", "errs", "colls");
#ifdef notyet
	if (dflag)
		printf("%5.5s ", "drops");
#endif
	if (lastif - iftot > 0)
		printf(" %8.8s %6.6s %8.8s %6.5s %6.6s",
			"packets", "errs", "packets", "errs", "colls");
#ifdef notyet
	if (dflag)
		printf(" %5.5s", "drops");
#endif
	putchar('\n');
	fflush(stdout);
	line = 0;
loop:
	sum->ift_ip = 0;
	sum->ift_ie = 0;
	sum->ift_op = 0;
	sum->ift_oe = 0;
	sum->ift_co = 0;
	sum->ift_dr = 0;

	for (curr_if = 1, prev_idx = 0, ip = iftot;
	    (curr_if <= ifnum) && (ip < lastif); ip++, curr_if++) {

		if_all.if_entry.if_index = ++prev_idx;
		get_if_all (&if_all);
		prev_idx = if_all.if_entry.if_index;

		bcopy ((char *)(if_all.if_entry.if_name),
			(char *)name, IFNAMSIZ);
		name[IFNAMSIZ - 1] = '\0';
		if (if_all.if_statsavail == 1)
			bcopy ((char *)&(if_all.if_stats),
				(char *)&ifstat, sizeof(struct ifstats));
		else
			(void)getifstats(name, &ifstat);

		if (ip == interesting) {
			printf("%8u %6u %8u %6u %6u",
				ifstat.ifs_ipackets - ip->ift_ip,
				ifstat.ifs_ierrors - ip->ift_ie,
				ifstat.ifs_opackets - ip->ift_op,
				ifstat.ifs_oerrors - ip->ift_oe,
				ifstat.ifs_collisions - ip->ift_co);
#ifdef notyet
			if (dflag)
				printf(" %6u",
				    if_all.if_entry.if_snd.ifq_drops - ip->ift_dr);
#endif
		}
		ip->ift_ip = ifstat.ifs_ipackets;
		ip->ift_ie = ifstat.ifs_ierrors;
		ip->ift_op = ifstat.ifs_opackets;
		ip->ift_oe = ifstat.ifs_oerrors;
		ip->ift_co = ifstat.ifs_collisions;
#ifdef notyet
		ip->ift_dr = if_all.if_entry.if_snd.ifq_drops;
#endif
		sum->ift_ip += ip->ift_ip;
		sum->ift_ie += ip->ift_ie;
		sum->ift_op += ip->ift_op;
		sum->ift_oe += ip->ift_oe;
		sum->ift_co += ip->ift_co;
		sum->ift_dr += ip->ift_dr;
	}
	if (lastif - iftot > 0) {
		printf("  %8d %6d %8d %6d %6d",
			sum->ift_ip - total->ift_ip,
			sum->ift_ie - total->ift_ie,
			sum->ift_op - total->ift_op,
			sum->ift_oe - total->ift_oe,
			sum->ift_co - total->ift_co);
#ifdef notyet
		if (dflag)
			printf(" %5d", sum->ift_dr - total->ift_dr);
#endif
	}
	*total = *sum;
	putchar('\n');
	fflush(stdout);
	line++;
	if (! signalled) {
		sigpause(SIGALRM);
	}
	signalled = NO;
	(void)alarm(interval);
	if (line == 21)
		goto banner;
	goto loop;
	/*NOTREACHED*/
}

/*
 * Called if an interval expires before sidewaysintpr has completed a loop.
 * Sets a flag to not wait for the alarm.
 */
void
catchalarm()
{
	signalled = YES;
}

int	
dlpi_get_stats(ifname, ifstatp)
	char *ifname;
	struct ifstats *ifstatp;
{
	dl_get_statistics_req_t	*req;
	dl_get_statistics_ack_t	*ack;
	struct dlpi_stats res_stats;
	struct strbuf ctl, data;
	int flags;
	int fd;
	char	hwdep_buf[1024];
	char stat_buf[DL_GET_STATISTICS_ACK_SIZE  + sizeof(struct dlpi_stats)];
	char devname[32];

	strcpy(devname, _PATH_DEV);
	strcat(devname, ifname);
	if ((fd = open(devname, O_RDWR)) < 0 ) {
		return -1;
	}

	bzero(stat_buf, sizeof(stat_buf));

	ctl.buf = stat_buf;
	ctl.len = sizeof(dl_get_statistics_req_t);

	req = (dl_get_statistics_req_t *) stat_buf;
	req->dl_primitive = DL_GET_STATISTICS_REQ;

	if (putmsg(fd, &ctl, NULL, RS_HIPRI) < 0) {
		close(fd);
                return -1;
        }

	bzero(stat_buf, sizeof(stat_buf));
	bzero(hwdep_buf, sizeof(hwdep_buf));

	ctl.buf = stat_buf;
        ctl.maxlen = sizeof(stat_buf);
        ctl.len = 0;
 
        data.buf = hwdep_buf;
        data.maxlen = sizeof(hwdep_buf);
        data.len = 0;
 
        flags=RS_HIPRI;
        if (getmsg(fd, &ctl, &data, &flags) < 0) {
		close(fd);
                return -1;
	}
	close(fd);

	ack = (dl_get_statistics_ack_t *) stat_buf;
	
	if (ack->dl_primitive == DL_GET_STATISTICS_ACK) {
		if (ack->dl_stat_length != sizeof(struct dlpi_stats)) {
			close(fd);
			return -1;
		}
		memcpy((caddr_t) &res_stats, stat_buf+ack->dl_stat_offset,
						ack->dl_stat_length);
	}
	else {
		close(fd);
		return -1 ;
	}

	/*
	 * Copy stuff we need into the ifstats structure that we got.
	 */

	ifstatp->ifs_ipackets =	res_stats.mac_rx.mac_bcast +
			 	res_stats.mac_rx.mac_mcast +
				res_stats.mac_rx.mac_ucast;

	ifstatp->ifs_ierrors =  res_stats.mac_rx.mac_error;

	ifstatp->ifs_opackets =	 res_stats.mac_tx.mac_bcast +
				 res_stats.mac_tx.mac_mcast +
				 res_stats.mac_tx.mac_ucast;
			
	ifstatp->ifs_oerrors =  res_stats.mac_tx.mac_error;


	ifstatp->ifs_collisions = 0;

	if (res_stats.mac_media_type == DL_CSMACD) {
		mac_stats_eth_t *eth_stats = (mac_stats_eth_t *) hwdep_buf;
		int i;

		for (i=0; i< 16; i++) {
			ifstatp->ifs_collisions += (i+1) * 
						eth_stats->mac_colltable[i];
		}
	}

	return 0;
}

int
getifstats(ifname, ifstatp)
	char  *ifname;
	struct ifstats *ifstatp;
{
	if (!ifstatp)
		return 0;

	if (!dlpi_get_stats(ifname, ifstatp)) {
		return 1;
	}	 

	return 0;
}
