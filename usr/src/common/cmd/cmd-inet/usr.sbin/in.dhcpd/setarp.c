#ident "@(#)setarp.c	1.3"
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

/*
 * This file contains the setarp() function.  This was taken from bootpd.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stream.h>
#include <sys/time.h>
#include <sys/stropts.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/sockio.h>
#include <net/if_arp.h>
#include <netinet/if_ether.h>
#include <errno.h>
#include <syslog.h>
#include <fcntl.h>
#include "pathnames.h"
#include "dhcpd.h"
#include "proto.h"

/*
 * SVR4 uses streams ioctl for arp.
 */
int
arpioctl (int name, caddr_t arg)
{
	int d;
	struct strioctl sti;
	int ret;
	
	d = open (_PATH_ARP, O_RDONLY);
	if (d < 0) {
		perror("arp: open");
		exit(1);
	}
	sti.ic_cmd = name;
	sti.ic_timout = 0;
	sti.ic_len = sizeof (struct arpreq);
	sti.ic_dp = arg;
	ret = ioctl(d, I_STR, (caddr_t)&sti);
	close(d);
	return (ret);
}

/*
 * Setup the arp cache so that IP address 'ia' will be temporarily
 * bound to hardware address 'ha' of length 'len'.
 */
void
setarp(struct in_addr *ia, u_char *ha, int len)
{
	extern int errno;
	struct arpreq arpreq;		/* Arp request ioctl block */
	struct sockaddr_in *si;
	
	bzero((caddr_t)&arpreq, sizeof(arpreq));
	arpreq.arp_flags = ATF_INUSE | ATF_COM;
	
	/* Set up the protocol address. */
	arpreq.arp_pa.sa_family = AF_INET;
	si = (struct sockaddr_in *) &arpreq.arp_pa;
	si->sin_addr = *ia;
	
	/* Set up the hardware address. */
	bcopy(ha, arpreq.arp_ha.sa_data, len);
	
	if (arpioctl (SIOCSARP, (caddr_t) &arpreq) < 0) {
	    report(LOG_ERR, "ioctl(SIOCSARP): %s", strerror(errno));
	}
}
