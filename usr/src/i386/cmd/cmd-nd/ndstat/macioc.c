#ident "@(#)macioc.c	11.2"
#ident "$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1996.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <stropts.h>

#include <sys/types.h>
#include <sys/stream.h>
#define LLI31_UNSUPPORTED
#include <sys/mdi.h>
#include <sys/lli31.h>
#include <sys/dlpi_ether.h>
#include <sys/privilege.h>
#include <net/if_types.h>  /* IFFDDI, etc. */

#include "ndstat.h"

extern int debug_flag;

int
do_macioc_getmca(int fd, char *addr, int len) 
{
	struct strioctl si;
	int i;

	si.ic_cmd = MACIOC_GETMCA;
	si.ic_timout = 0;
	si.ic_dp = addr;
	si.ic_len = len;

	if (ioctl(fd, I_STR, &si) < 0) {
		if (debug_flag) {
			catfprintf(stderr, 39, "ndstat: %s ioctl failed\n", "MACIOC_GETMCA");
			perror("ndstat");
		}
		return(-1);
	}

	return(si.ic_len);
}

int
clear_stats(fd)
{
	struct strioctl si;

	if (type == LLI || type == MDI) {
		/* both understand this ioctl */
		si.ic_cmd = MACIOC_CLRSTAT;
		si.ic_timout = 0;
		si.ic_dp = 0;
		si.ic_len = 0;

		if (ioctl(fd, I_STR, &si) < 0) {
			if (debug_flag) {
				catfprintf(stderr, 39, "ndstat: %s ioctl failed\n", "MACIOC_CLRSTAT");
				perror("ndstat");
			}
			return(-1);
		}
		return(0);
	} else 
	if (type == ODIDLPI) {
		extern int havepriv(int);
		extern DL_mib_t *mib;
		struct strioctl strioc;
		int ret;

		/* DLIOCSMIB will fail unless you have driver priviledge */
		if (havepriv(P_DRIVER) != 1) {
			catfprintf(stderr, 331, "ndstat: Must have driver privilege for -c option\n");
			return(-1);
		}
		/* zero out fields in exiting mib structure then send it
		 * back to the kernel.  can't do a bzero because if we
		 * change things like mib->ifOperStatus then we will
		 * bring the board down!  So only zero out things that would
		 * be considered "safe"
		 */
		mib->ifInOctets = 0;
		mib->ifInUcastPkts = 0;
		mib->ifInNUcastPkts = 0;
		mib->ifInDiscards = 0;
		mib->ifInErrors = 0;
		mib->ifInUnknownProtos = 0;
		mib->ifOutOctets = 0;
		mib->ifOutUcastPkts = 0;
		mib->ifOutNUcastPkts = 0;
		mib->ifOutDiscards = 0;
		mib->ifOutErrors = 0;
		mib->ifOutQlen = 0;   /* never used */
	        switch (mib->ifType) {
			case IFETHERNET_CSMACD:
				mib->ifSpecific.etherAlignErrors = 0;
				mib->ifSpecific.etherCRCerrors = 0;
				mib->ifSpecific.etherMissedPkts = 0;
				mib->ifSpecific.etherOverrunErrors = 0;
				mib->ifSpecific.etherUnderrunErrors = 0;
				mib->ifSpecific.etherCollisions = 0;
				mib->ifSpecific.etherAbortErrors = 0;
				mib->ifSpecific.etherCarrierLost = 0;
				mib->ifSpecific.etherReadqFull = 0;
				mib->ifSpecific.etherRcvResources = 0;
				/* don't touch etherDependent[12345] */
				break;

			case IFISO88025_TOKENRING:

#if 0
see comment in stats.c about why we can't do anything here...
#endif
				break;

			case IFFDDI:
#if 0
see comment in stats.c about why we can't do anything here...
#endif
				break;
		}

		strioc.ic_len = sizeof(DL_mib_t);
		strioc.ic_timout = 0;
		strioc.ic_dp = (char *)&mib;
		strioc.ic_cmd = DLIOCSMIB;

		ret=ioctl(fd, I_STR, &strioc);
		if (ret < 0) {
			fflush(stdout);
			if (debug_flag) {
				catfprintf(stderr, 39, "ndstat: %s ioctl failed\n", "DLIOCSMIB");
				perror("ndstat");
			}
			return(-1);
		}
		return(0);
	}
}

lli31_mac_stats_t *
do_macioc_getstat(int fd)
{
	struct strioctl si;
	static lli31_mac_stats_t ms;

	si.ic_cmd = MACIOC_GETSTAT;
	si.ic_timout = 0;
	si.ic_dp = (char *)&ms;
	si.ic_len = sizeof(lli31_mac_stats_t);

	if (ioctl(fd, I_STR, &si) < 0) {
		fflush(stdout);
		if (debug_flag) {
			catfprintf(stderr, 39, "ndstat: %s ioctl failed\n", "MACIOC_GETSTAT");
			perror("ndstat");
		}
		return (lli31_mac_stats_t *)0;
	}
	return (&ms);
}

char *
do_macioc_getaddr(int fd, int getraddr)
{
	struct strioctl si;
	static char hw[6];

	si.ic_cmd = getraddr ? MACIOC_GETRADDR : MACIOC_GETADDR;
	si.ic_timout = 0;
	si.ic_dp = hw;
	si.ic_len = sizeof(hw);

	if (ioctl(fd, I_STR, &si) < 0) {
		if (debug_flag) {
			fprintf(stderr, "\n");
			catfprintf(stderr, 39, "ndstat: %s ioctl failed\n", "MACIOC_GET(R)ADDR");
			perror("ndstat");
		}
		return((char *)0);
	}
	return(hw);
}
