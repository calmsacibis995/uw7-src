#ident	"@(#)hwaddr.c	1.2"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

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

/*
 * hwaddr.c - routines that deal with hardware addresses.
 * (i.e. Ethernet)
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#ifdef  SVR4
#include <sys/stropts.h>
#endif
#if defined(SUNOS) || defined(SVR4)
#include <fcntl.h>
#include <sys/sockio.h>
#endif
#include <net/if_arp.h>
#include <netinet/in.h>
#include <stdio.h>
#ifndef	NO_UNISTD
#include <unistd.h>
#endif
#include <syslog.h>

#ifdef SVR4
/* Yes, memcpy is OK here (no overlapped copies). */
#include <memory.h>
#define bcopy(a,b,c)    memcpy(b,a,c)
#define bzero(p,l)      memset(p,0,l)
#define bcmp(a,b,c)     memcmp(a,b,c)
#endif

#include "bootpd.h"	/* for MAXHADDRLEN */
#include "hwaddr.h"
#include "report.h"

extern int debug;

/*
 * Hardware address lengths (in bytes) and network name based on hardware
 * type code.  List in order specified by Assigned Numbers RFC; Array index
 * is hardware type code.  Entries marked as zero are unknown to the author
 * at this time.  .  .  .
 */

struct hwinfo hwinfolist[] = {
	{ 0, "Reserved"	 },	/* Type 0:  Reserved (don't use this)   */
	{ 6, "Ethernet"	 },	/* Type 1:  10Mb Ethernet (48 bits)	*/
	{ 1, "3Mb Ethernet" },	/* Type 2:   3Mb Ethernet (8 bits)	*/
	{ 0, "AX.25"	 },	/* Type 3:  Amateur Radio AX.25		*/
	{ 1, "ProNET"	 },	/* Type 4:  Proteon ProNET Token Ring   */
	{ 0, "Chaos"	 },	/* Type 5:  Chaos			*/
	{ 6, "IEEE 802"	 },	/* Type 6:  IEEE 802 Networks		*/
	{ 0, "ARCNET"	 }	/* Type 7:  ARCNET			*/
};
int hwinfocnt = sizeof(hwinfolist) / sizeof(hwinfolist[0]);

#ifdef SVR4
/*
 * SVR4 uses streams ioctl for arp.
 */
int
arpioctl (name, arg)
     int name;
     caddr_t arg;
{
	int d;
	struct strioctl sti;
	int ret;
	
	d = open ("/dev/arp", O_RDONLY);
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

#endif


/*
 * Setup the arp cache so that IP address 'ia' will be temporarily
 * bound to hardware address 'ha' of length 'len'.
 */
void
setarp(s, ia, ha, len)
	int s;					/* socket fd */
	struct in_addr *ia;
	u_char *ha;
	int len;
{
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
	
#ifdef SVR4
	if (arpioctl (SIOCSARP, (caddr_t) &arpreq) < 0) {
#else
	if (ioctl(s, SIOCSARP, (caddr_t)&arpreq) < 0) {
#endif
	    report(LOG_ERR, "ioctl(SIOCSARP): %s", get_errmsg());
	}
}


/*
 * Convert a hardware address to an ASCII string.
 */
char *haddrtoa(haddr, hlen)
    u_char *haddr;
    int hlen;
{
	static char haddrbuf[3 * MAXHADDRLEN + 1];
	char *bufptr;

	if (hlen > MAXHADDRLEN)
	    hlen = MAXHADDRLEN;

	bufptr = haddrbuf;
	while (hlen > 0) {
		sprintf(bufptr, "%02X.", (unsigned) (*haddr++ & 0xFF));
		bufptr += 3;
		hlen--;
	}
	bufptr[-1] = 0;
	return (haddrbuf);
}
