#ident "@(#)tcpcfg.c	1.2"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1994 Lachman Technology, Inc.
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
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include "proto.h"

#define INBUFSIZE MAXHOSTNAMELEN

char *cmdname;
struct in_addr *myaddr, *broadcast, *netmask;
unsigned long dflt_netmask, legal_netmask;


conv_addr(str, addr)
char *str;
struct in_addr *addr;
{
	unsigned long val;

	if ((val = inet_addr(str)) == 0xffffffff)
		return 0;
	else {
		addr->s_addr = val;
		return 1;
	}
}

conv_inetaddr(str, addr)
char *str;
struct in_addr *addr;
{
	if (!(conv_addr(str, addr))) {
		printf("Invalid internet address.\n");
		exit(1);
		/* NOTREACHED */
	} else
		return 1;
}

conv_netmask(str, mask)
char *str;
struct in_addr *mask;
{
	struct in_addr mask0;
	unsigned long val;

	mask0.s_addr = mask->s_addr;
	if (!(conv_addr(str, &mask0))) {
		printf("Invalid subnet mask.\n");
		exit(1);
		/* NOTREACHED */
	} else {
		val = ntohl(mask0.s_addr);
		if ((val & legal_netmask) != legal_netmask) {
			printf("Subnet mask must contain network mask.\n");
			exit(1);
			/* NOTREACHED */
		} else {
			mask->s_addr = htonl(val);
			return 1;
		}
	}
}

void
usage()
{
	printf("usage: %s -i <ip addr> [-n <netmask>] [-1]\n", cmdname);
	exit(1);
	/* NOTREACHED */
}

main(argc, argv)
int argc;
char *argv[];
{
	int c, broadcast_ones = 0;
	extern char *optarg;
	unsigned long haddr;

	cmdname = argv[0];
	if (argc < 3) {
		usage();
		/* NOTREACHED */
	}
	broadcast = (struct in_addr *) malloc(sizeof(struct in_addr));
	while ((c = getopt(argc, argv, "i:n:1")) != -1) {
		switch (c) {
		case '1':
			broadcast_ones = 1;
			break;
		case 'i':
			myaddr = (struct in_addr *)malloc(sizeof(struct in_addr));
			conv_inetaddr(optarg, myaddr);
#ifdef DEBUG
			printf("myaddr=%s\n", myaddr);
#endif
			break;
		case 'n':
			netmask = (struct in_addr *)malloc(sizeof(struct in_addr));
			if (myaddr == (struct in_addr *) 0) {
				usage();
				/* NOTREACHED */
			}
			conv_netmask(optarg, netmask);
#ifdef DEBUG
			printf("netmask=%s\n", netmask);
#endif
			dflt_netmask=ntohl(netmask->s_addr);
			break;
		case '?':
			usage();
			/* NOTREACHED */
		}
	}

	if (myaddr != (struct in_addr *) 0) {
		haddr = htonl(myaddr->s_addr);
		if (IN_CLASSA(haddr))
			dflt_netmask = IN_CLASSA_NET;
		else if (IN_CLASSB(haddr))
			dflt_netmask = IN_CLASSB_NET;
		else if (IN_CLASSC(haddr))
			dflt_netmask = IN_CLASSC_NET;
		legal_netmask = dflt_netmask;
	} else {
		usage();
		/* NOTREACHED */
	}

	if (netmask == (struct in_addr *) 0) {
		netmask = (struct in_addr *)malloc(sizeof(struct in_addr));
		netmask->s_addr = htonl(dflt_netmask);
	}

	broadcast->s_addr = (myaddr->s_addr & netmask->s_addr) |
		((broadcast_ones ? INADDR_BROADCAST : INADDR_ANY) &
		~netmask->s_addr);

	printf("IP=%s\n", inet_ntoa(*myaddr));
	printf("NM=%s\n", inet_ntoa(*netmask));
	printf("BD=%s\n", inet_ntoa(*broadcast));
}
