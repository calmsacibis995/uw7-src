#ident "@(#)ping.c	1.2"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1996 Computer Associates International, Inc.
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#include <unistd.h>
#include <syslog.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include "dhcpd.h"
#include "proto.h"

extern int debug;

static u_char pktbuf[1024];
static u_short seq = 0;

/*
 * Wait for .5 sec for a response.
 */

static struct timeval wait_time = { 0, 500000 };

#define ICMP_PKT_SIZE	64		/* size of packet we send */

/*
 * checksum -- compute checksum of buf for len bytes
 */

static u_short
checksum(u_char *buf, int len)
{
	u_long sum;
	u_char *p;

	sum = 0;
	p = buf;
	for (; len > 1; len -= 2, p += 2) {
		sum += (p[0] << 8) | p[1];
		if (sum > 0xffff) {
			sum = (sum & 0xffff) + 1;
		}
	}
	if (len == 1) {
		sum += *p << 8;
		if (sum > 0xffff) {
			sum = (sum & 0xffff) + 1;
		}
	}

	return ~sum & 0xffff;
}

/*
 * time_add -- return t1 + t2 in res.
 */

static void
time_add(struct timeval *t1, struct timeval *t2, struct timeval *res)
{
	res->tv_sec = t1->tv_sec + t2->tv_sec;
	res->tv_usec = t1->tv_usec + t2->tv_usec;
	if (res->tv_usec >= 1000000) {
		res->tv_usec -= 1000000;
		res->tv_sec++;
	}
}

/*
 * time_sub -- return t1 - t2 in res.
 */

static void
time_sub(struct timeval *t1, struct timeval *t2, struct timeval *res)
{
	res->tv_sec = t1->tv_sec - t2->tv_sec;
	res->tv_usec = t1->tv_usec - t2->tv_usec;
	if (res->tv_usec < 0) {
		res->tv_usec += 1000000;
		res->tv_sec--;
	}
}

/*
 * time_ge -- true if t1 >= t2.
 */

static int
time_ge(struct timeval *t1, struct timeval *t2)
{
	if (t1->tv_sec > t2->tv_sec) {
		return 1;
	}
	else if (t1->tv_sec == t2->tv_sec) {
		return t1->tv_usec >= t2->tv_usec;
	}
	else {
		return 0;
	}
}

/*
 * ping -- send an ICMP echo request to the given address.
 * Returns 1 if a response is received, or 0 if not.
 */

int
ping(struct in_addr *addr)
{
	int s, i, len, iplen, ret;
	size_t fromlen;
	u_short ident, seq_sent;
	struct sockaddr_in sin, from;
	struct icmp *icmp;
	struct ip *ip;
	fd_set fds;
	struct timeval start_time, end_time, select_time, now;

	if (debug > 1) {
		report(LOG_INFO, "Pinging %s", inet_ntoa(*addr));
	}

	ret = 0;

	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) {
		report(LOG_ERR, "ping: Unable to create ICMP socket.");
		return 0;
	}

	FD_ZERO(&fds);

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr = *addr;

	icmp = (struct icmp *) pktbuf;
	icmp->icmp_type = ICMP_ECHO;
	icmp->icmp_code = 0;
	icmp->icmp_cksum = 0;
	ident = getpid() & 0xffff;
	icmp->icmp_id = htons((u_short) ident);
	icmp->icmp_seq = htons((u_short) seq);
	seq_sent = seq;
	seq++;

	for (i = 0; i < ICMP_PKT_SIZE - ICMP_MINLEN; i++) {
		icmp->icmp_data[i] = i;
	}

	icmp->icmp_cksum = checksum(pktbuf, ICMP_PKT_SIZE);
	icmp->icmp_cksum = htons(icmp->icmp_cksum);

	if (sendto(s, pktbuf, ICMP_PKT_SIZE, 0, (struct sockaddr *) &sin,
	    sizeof(sin)) == -1) {
		report(LOG_ERR, "ping: sendto failed");
		goto done;
	}

	gettimeofday(&start_time, NULL);
	time_add(&start_time, &wait_time, &end_time);

	for (;;) {
		gettimeofday(&now, NULL);
		if (time_ge(&now, &end_time)) {
			if (debug > 1) {
				report(LOG_INFO, "Ping timed out.");
			}
			goto done;
		}
		time_sub(&end_time, &now, &select_time);
		FD_SET(s, &fds);
		if ((ret = select(s + 1, &fds, NULL, NULL, &select_time))
		    == -1) {
			report(LOG_ERR, "ping: select failed");
			goto done;
		}
		if (ret == 0) {
			if (debug > 1) {
				report(LOG_INFO, "Ping timed out.");
			}
			goto done;
		}
		fromlen = sizeof(from);
		if ((ret = recvfrom(s, pktbuf, sizeof(pktbuf), 0,
		    (struct sockaddr *) &from, &fromlen)) == -1) {
			report(LOG_ERR, "ping: recvfrom failed");
			goto done;
		}
		if (from.sin_addr.s_addr != addr->s_addr) {
			if (debug > 1) {
				report(LOG_INFO, "ping: received ICMP message from %s (ignoring)",
					inet_ntoa(from.sin_addr));
			}
			continue;
		}
		if (ret < sizeof(struct ip)) {
			report(LOG_ERR, "ping: Packet smaller than IP header (%d)",
				ret);
			goto done;
		}
		ip = (struct ip *) pktbuf;
		iplen = ip->ip_hl * 4;
		if (ret < iplen + ICMP_MINLEN) {
			report(LOG_ERR, "Packet smaller than IP header + ICMP header (%d)",
				ret);
			goto done;
		}
		icmp = (struct icmp *) &pktbuf[iplen];
		if (debug > 1) {
			report(LOG_INFO, "ping: received type %d code %d id 0x%x seq %d",
				icmp->icmp_type, icmp->icmp_code,
				ntohs(icmp->icmp_id), ntohs(icmp->icmp_seq));
		}
		if (icmp->icmp_type == ICMP_ECHOREPLY
		    && icmp->icmp_code == 0
		    && ntohs(icmp->icmp_id) == ident
		    && ntohs(icmp->icmp_seq) == seq_sent) {
			ret = 1;
			goto done;
		}
	}

done:
	close(s);
	return ret;
}
