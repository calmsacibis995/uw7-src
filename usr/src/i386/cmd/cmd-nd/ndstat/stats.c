#ident "@(#)stats.c	25.2"
#ident "$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1995.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <stropts.h>
#include <nlist.h>

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/scodlpi.h>
#include <sys/mdi.h>
#include <sys/dlpimod.h>
#include <sys/lli31.h>

#include <sys/ddi.h>
#include <sys/dlpi_ether.h>
#include <sys/privilege.h>

#include <net/if_types.h>  /* IFFDDI, etc. */
#include "ndstat.h"

extern int debug_flag;
extern int mdionly;
extern void search_netdrivers(void);
extern void search_dlpimdi(void);

#define DLPIMDI "/etc/inst/nd/dlpimdi"
#define NETDRIVERS "/etc/confnet.d/netdrivers"
enum type type;

void
search_for_drivers(void)
{
/*	Search both files as dlpimdi is only one to pick up ISDN drivers. */
/*	Netdrivers is the only one to pick ODI or DLPI drivers            */
/*	dev_uniq() sorts out duplicate devices.                           */

	search_dlpimdi();
	search_netdrivers();
}

void
search_netdrivers(void)
{
	char buf[80];
	char fname[90];
	char *cp;
	FILE *fd;

	strcpy(fname, NETDRIVERS);
	fd = fopen(fname, "r");
	if (fd == (FILE *) 0) {
		catfprintf(stderr,32,"ndstat: Unable to open %s\n", fname);
		perror("ndstat");
		return;
	}
	while (fgets(buf, sizeof(buf), fd)) {
		cp=buf;
		while (cp != (buf + 80)) {
			/* see netdrivers(4): if we hit nl or whitespace done */
			if (isspace(*cp)) {
				*cp = 0;
				if (*buf == '/') {
					/* netdrivers(4) says it won't be
					 * absolute but if it is then use it
					 */
					snprintf(fname, 90, "%s", buf);
				} else {
					snprintf(fname, 90, "/dev/%s", buf);
				}
				break;
			}
			cp++;
		}
		if (cp != (buf + 80)) {
			display_statistics(fname);
		}
	}
	fclose(fd);
}

void
search_dlpimdi(void)
{
	char buf[80];
	char fname[90];
	char *cp;
	const char *ssop;
	FILE *fd;

	strcpy(fname, DLPIMDI);
	fd = fopen(fname, "r");
	if (fd == (FILE *)0) {
		catfprintf(stderr,32,"ndstat: Unable to open %s\n", fname);
		perror("ndstat");
		return;
	}
	while (fgets(buf, sizeof(buf), fd)) {
		cp=strchr(buf, ':');
		if (cp == NULL)
			continue;
		*cp=0;
		snprintf(fname, 90, "/dev/%s", buf);
		display_statistics(fname);
	}
	fclose(fd);
}

	/* H/W Independent statistics structure */
static int	stat_fd = -1;
static char	stat_buf[sizeof(dl_get_statistics_ack_t) +
			 sizeof(struct dlpi_stats)];
int	hwdep_type;
char	hwdep_buf[4096];
		

struct dlpi_stats *
get_hw_independent_stats(int fd)
{
	if (do_stats(fd) == -1)
		return( (struct dlpi_stats *)0 );
	return( (struct dlpi_stats *)stat_buf );
}

static int nsets=0;
static int nprvs=0;
static struct pm_setdef *sdefs=(struct pm_setdef *)0;
extern int privname();

/* called by havepriv() */
static  int
setnum(char *name)
{
	int     i;

	for(i = 0; i < nsets; ++i){
		if(!strcmp(name,sdefs[i].sd_name)){
			if(sdefs[i].sd_objtype == PS_PROC_OTYPE){
				return(i);
			}
			return(-1);     /*Not a process privilege set*/
		}
	}
	return(-1);
}

/* does this LWP have the priviledge specified in privneeded?
 * returns 1 if yes, 0 if no, -1 if error
 * borrowed routine from ndcfg in cmdops.c (what a pain to figure this out
 * originally)
 */
int 
havepriv(int privneeded)
{
	int count,i;
	priv_t priv,needed;
	priv_t  *buff=(priv_t *)0;

	nsets = secsys(ES_PRVSETCNT, 0);
	if(nsets < 0) {
		return(-1);
	}
	sdefs = (setdef_t *)malloc(nsets * sizeof(setdef_t));
	if(!sdefs){
		return(-1);
	}
	(void)secsys(ES_PRVSETS, (char *)sdefs);
	nprvs = 0;
	for (i = 0; i < nsets; ++i) {
		if(sdefs[i].sd_objtype == PS_PROC_OTYPE) {
			nprvs += sdefs[i].sd_setcnt;  /* NPRIVS(27) per set */
		}
	}

	buff=(priv_t *)malloc(nprvs * sizeof(priv_t));
	if (!buff) {
		free(sdefs);
		return(-1);
	}

	/* count has the total number of priviledges in all possible sets.  
	 * At a minimum this means all priviledges in work and max.
	 */
	count = procpriv(GETPRV,buff,nprvs);

	if (!count) {
		free(buff);
		free(sdefs);
		return(0);  /* no privileges, so return 0 */
	}
	if (nprvs < privneeded) {
		free(buff);
		free(sdefs);
		return(0);  /* we don't go up that high; return 0 */
	}

	/* needed represents the priviledge we need, shoved into a priv_t */
	needed=sdefs[setnum("work")].sd_mask | privneeded;

	/* for each possible priviledge we have in all of our sets, determine
	 * if it's the one we're looking for
	 * each priviledge in buff[] has the priviledge OR'ed in with the
	 * bitmask, so the above OR is necessary for a direct comparision.
	 */
	for(i = 0; i < count; ++i) {
		if(buff[i] == needed) {
			free(sdefs);
			free(buff);
			return(1);
		}
	}

	free(sdefs);
	free(buff);

	return(0);
}

int
do_stats(int fd)
{
	if (type == MDI) {
		if (do_get_stats_req(fd) == 0)
			goto ok;
	} else if (type == LLI) {
		if (do_lli31_stats(fd) == 0)
			goto ok;
	} else if (type == ODIDLPI) {
		if (do_odidlpi_stats(fd) == 0)
			goto ok;
	}

	return(-1);
ok:
	stat_fd = fd;
	return(0);
}

int
do_odidlpi_stats(int fd)
{
	struct dlpi_stats *s= (struct dlpi_stats *) stat_buf;
	extern DL_mib_t *mib;
	extern int list_info;
	char *id_string=(char *)(mib + 1);
	mac_stats_eth_t *eth=(mac_stats_eth_t *)hwdep_buf;
	mac_stats_tr_t  *tok=(mac_stats_tr_t *)hwdep_buf;
	mac_stats_fddi_t *fddi=(mac_stats_fddi_t *)hwdep_buf;
	int loop;
	/* fill in the dlpi_stats structure from information in the DL_mib_t
	 * also set the global variable "hwdep_type".
	 * also fill in hwdep_buf according to hwdep_type:
	 *    mac_stats_eth_t *mp = (mac_stats_eth_t *)hwdep_buf;
	 *    mac_stats_tr_t *mp = (mac_stats_tr_t *)hwdep_buf;
	 *    mac_stats_fddi_t *mp = (mac_stats_fddi_t *)hwdep_buf;
	 */

	/* no need to call catfprintf here as this is the vendor's string 
	 * straight from the transmogrifier and ends up in driver's Space.c.
	 */
	if (list_info == LIST_LONG && mib->ifDescrLen > 0) {
		/* id_string is always null terminated by kernel */
		printf("%s\n",id_string);  
	}

	s->dl_nsaps = 0; /* #SAPs currently bound to DLPI */
	s->dl_maxsaps = 0; /* Max. #SAPs usable */
	s->dl_rxunbound = mib->ifInDiscards + 
			  mib->ifInDiscards +
			  mib->ifOutDiscards; /*#frames received not delivered*/
	s->dl_nroutes = 0; /* #Source Routes currently in use */
	s->dl_maxroutes = 0; /* Max #Source Routes usable */
	/* we want this to model CONFORMANCE in the .bcfg file.
	 * for MDI this is mac_driver_version from MAC_INFO_ACK.  For
	 * ODI we would prefer 0x0110 (ODI 1.1) 0x0310 (ODI 3.1) or 
	 * 0x0320 (ODI 3.2).   The current transmogrifier sets 3.2 in 
	 * the Master file.
	 * for DLPI we want 0x02 (DL_CURRENT_VERSION)
	 * since we don't differenciate ODI and DLPI here we just use
	 * DL_CURRENT_VERSION
	 */
	s->mac_driver_version = DL_CURRENT_VERSION;
	switch (mib->ifType) {
		case IFETHERNET_CSMACD:
			/* set everything to 0 */
			memset(eth, sizeof(struct mac_stats_eth), 0x0);

			s->mac_media_type = MAC_CSMACD;
			/* s->mac_max_sdu = 1514; */
			s->mac_stats_len = sizeof(mac_stats_eth_t);
			eth->mac_align = mib->ifSpecific.etherAlignErrors;
			eth->mac_badsum = mib->ifSpecific.etherCRCerrors;
			eth->mac_sqetesterrors = 0;
			eth->mac_frame_def = 0;
			eth->mac_oframe_coll = 0;
			eth->mac_xs_coll = mib->ifSpecific.etherCollisions;
			eth->mac_tx_errors = mib->ifSpecific.etherAbortErrors;
			eth->mac_carrier = mib->ifSpecific.etherCarrierLost;
			eth->mac_badlen = 0;
			eth->mac_no_resource = mib->ifSpecific.etherMissedPkts +
					       mib->ifSpecific.etherReadqFull;
			for (loop=0; loop<15; loop++) {
				eth->mac_colltable[loop] = 0;
			}
			eth->mac_spur_intr = 0;
			eth->mac_frame_nosr = mib->ifSpecific.etherRcvResources;
			eth->mac_baddma = mib->ifSpecific.etherOverrunErrors + 
					  mib->ifSpecific.etherUnderrunErrors;
			eth->mac_timeouts = 0;

			break;

		case IFISO88025_TOKENRING:

#if 0
                Obligatory long rant^H^H^H^Hcomment follows:
due to header file problems we don't get token ring specific information
from dlpi_token.h DL_mib_t TKR_MIB structure because we can't include
both dlpi_ether.h and dlpi_token.h as they both define structures of
different sizes... the kernel DLIOCGMIB code for dlpi_ether.c and
dlpi_token.c ensures that the ioctl size that comes down is the proper
size before answering, so we would have to try GMIB twice with both
structures in order to get proper statistical information.  This also
means copying most of dlpi_token.h (at least the TKR_MIB structure)
into this source code and casting appropriately.  Too much hassle for
Broken-Ring(tm) so we skip it.  Good thing there's no dlpi_fddi.h too.

ODI (lslstr.c) internally takes the same approach, assuming that the DLIOCGMIB
will be passing in a ethernet-sized DL_mib_t structure.  It doesn't
fill in any of the media-specific fields for IFISO88025_TOKENRING and
IFFDDI, so statistics for these media types will never make it to user
space.  If you're truly desperate for them  

Just be aware that you _could_ obtain token ring specific statistics from
the DLIOCGMIB ioctl but only for DLPI (not ODI) token ring drivers!

If you _really_ want to get these statistics for ODI drivers, use kdb and call 
the numerous routines in usr/src/work/uts/io/odi/lsl/lslprint.c which 
will print all the internal fields to the console.

Gotta love it..

#endif

			s->mac_media_type = MAC_TPR;
			/* s->mac_max_sdu = 988, 1500, 2000  */
			s->mac_stats_len = sizeof(mac_stats_tr_t);

			/* set everything to 0 */
			memset(tok, sizeof(struct mac_stats_tr), 0x0);
			break;

		case IFFDDI:
			s->mac_media_type = MAC_FDDI;
			s->mac_stats_len = sizeof(mac_stats_fddi_t);

			/* set everything to 0 */
			memset(fddi, sizeof(struct mac_stats_fddi), 0x0);
			break;

		/* don't have to worry about MAC_ISDN_BRI/MAC_ISDN/PRI here */
	}
	s->mac_max_sdu = mib->ifMtu;  /* not technically correct */
	s->mac_min_sdu = 1;
	/* mac_addr_length should be MAC_ADDR_SZ+((sp->llcmode==LLC_1)?1:0); */
	s->mac_addr_length = 6;
	s->mac_ifspeed = mib->ifSpeed;

	/* from usr/src/work/uts/io/odi/lsl/lslstr.c:
	 * the HSM keeps double ulong_t counters for Tx and Rx bytes. the
	 * corresponding fields in the MIB are only one ulong_t wide. in the
	 * unlikely event (drop me a line if this ever happens) that we stay
	 * up long enough to wrap the HSM's low-order ulong_t for either
	 * counter, we'll end up with incorrect values reported.
	 */

	/* can't differenciate between multicast and broadcast :-( */
	s->mac_rx.mac_bcast = mib->ifInNUcastPkts;
	s->mac_rx.mac_mcast = mib->ifInNUcastPkts;
	s->mac_rx.mac_ucast = mib->ifInUcastPkts;
	s->mac_rx.mac_error = mib->ifInErrors;
	s->mac_rx.mac_octets = mib->ifInOctets;
	s->mac_rx.mac_queue_len = 0;

	/* can't differenciate between multicast and broadcast :-( */
	s->mac_tx.mac_bcast = mib->ifOutNUcastPkts;
	s->mac_tx.mac_mcast = mib->ifOutNUcastPkts;
	s->mac_tx.mac_ucast = mib->ifOutUcastPkts;
	s->mac_tx.mac_error = mib->ifOutErrors;
	s->mac_tx.mac_octets = mib->ifOutOctets;
	s->mac_tx.mac_queue_len = mib->ifOutQlen;

	/* since no mac_stats_eth_t statistic for suspended, and ODI can't
	 * be ddi8, and unlikely that any dlpi drivers will be ddi8, just
	 * force these to 0
	 */
	s->mac_suspended = 0;
	s->mac_suspenddrops = 0;

	for (loop=0; loop < 6; loop++) {
		s->mac_reserved[loop] = 0;
	}

	hwdep_type = s->mac_media_type;

	return(0);
}

int
do_get_stats_req(int fd)
{
	dl_get_statistics_req_t	*req;
	dl_get_statistics_ack_t	*ack;

	struct strbuf ctl, data;
	int flags;

	ctl.buf = stat_buf;
	ctl.len = sizeof(dl_get_statistics_req_t);

	req = (dl_get_statistics_req_t *)stat_buf;
	req->dl_primitive = DL_GET_STATISTICS_REQ;

	if (putmsg(fd, &ctl, NULL, RS_HIPRI) < 0) {
		fflush(stdout);
		if (debug_flag) {
			catfprintf(stderr, 34, "ndstat: putmsg(%s) failed\n", "DL_GET_STATISTICS_REQ");
			perror("ndstat");
		}
		return(-1);
	}

	ctl.maxlen = sizeof(stat_buf);
	ctl.len = 0;

	data.buf = hwdep_buf;
	data.maxlen = sizeof(hwdep_buf);
	data.len = 0;

	flags=RS_HIPRI;
	if (getmsg(fd, &ctl, &data, &flags) < 0) {
		fflush(stdout);
		if (debug_flag) {
			catfprintf(stderr, 35, "ndstat: getmsg(%s) failed\n", "DL_GET_STATISTICS_ACK");
			perror("ndstat");
		}
		return(-1);
	}
	if (flags != RS_HIPRI) {
		if (debug_flag)
			catfprintf(stderr,36,"ndstat: getmsg(%s) OK, but flags is wrong (=%x)\n", "DL_GET_STATS_ACK", flags);
		return(-1);
	}

	ack = (dl_get_statistics_ack_t *)stat_buf;
	if (ack->dl_primitive == DL_GET_STATISTICS_ACK) {
		if (ack->dl_stat_length != sizeof(struct dlpi_stats)) {
			if (debug_flag)
				catfprintf(stderr,37,"ndstat: Incorrect stats_len returned, was %d should be %d\n", ack->dl_stat_length, sizeof(struct dlpi_stats));
			return(-1);
		}
		memcpy(stat_buf, stat_buf+ack->dl_stat_offset, ack->dl_stat_length);
		hwdep_type = ((struct dlpi_stats *)stat_buf)->mac_media_type;

		return(0);
	} else if (ack->dl_primitive != DL_ERROR_ACK) {
		if (debug_flag)
			catfprintf(stderr,38,"ndstat: getstats(%s) - Incorrect primitive returned (0x%x)\n", "DL_GET_STATISTICS_ACK", ack->dl_primitive);
	}
	return(-1);
}

int
do_lli31_stats(int fd)
{
	extern lli31_mac_stats_t *do_macioc_getstat(int);

	lli31_mac_stats_t	*mp;
	struct dlpi_stats	*sp;

	dl_info_req_t	*req;
	dl_info_ack_t	ack;

	struct strbuf ctl;
	int flags;

	sp=(struct dlpi_stats *)stat_buf;
	mp = do_macioc_getstat(fd);
	if (!mp)
		return(-1);
	memset(stat_buf, 0, sizeof(struct dlpi_stats));

	sp->mac_tx.mac_ucast = mp->mac_frame_xmit;
	sp->mac_tx.mac_mcast = mp->mac_mcast_xmit;
	sp->mac_tx.mac_bcast = mp->mac_bcast_xmit;
	sp->mac_tx.mac_octets= mp->mac_ooctets;
	sp->mac_rx.mac_ucast = mp->mac_frame_recv;
	sp->mac_rx.mac_mcast = mp->mac_mcast_recv;
	sp->mac_rx.mac_bcast = mp->mac_bcast_recv;
	sp->mac_rx.mac_octets= mp->mac_ioctets;
	sp->mac_ifspeed = mp->mac_ifspeed;

	ctl.buf = (char *)&ack;
	ctl.len = sizeof(dl_info_req_t);

	req = (dl_info_req_t *)&ack;
	req->dl_primitive = DL_INFO_REQ;

	if (putmsg(fd, &ctl, NULL, RS_HIPRI) < 0) {
		fflush(stdout);
		if (debug_flag) {
			catfprintf(stderr, 34, "ndstat: putmsg(%s) failed\n", "DL_INFO_REQ");
			perror("ndstat");
		}
		return(-1);
	}

	ctl.maxlen = sizeof(ack);
	ctl.len = 0;

	flags=RS_HIPRI;
	if (getmsg(fd, &ctl, NULL, &flags) < 0) {
		fflush(stdout);
		if (debug_flag) {
			catfprintf(stderr, 35, "ndstat: getmsg(%s) failed\n", "DL_INFO_ACK");
			perror("ndstat");
		}
		return(-1);
	}
	if (flags != RS_HIPRI) {
		if (debug_flag)
			catfprintf(stderr,36,"ndstat: getmsg(%s) OK, but flags is wrong (=%x)\n", "DL_INFO_ACK", flags);
		return(-1);
	}
	if (ack.dl_primitive != DL_INFO_ACK) {
		if (debug_flag)
			catfprintf(stderr,38,"ndstat: getstats(%s) - Incorrect primitive returned (0x%x)\n", "DL_INFO_ACK", ack.dl_primitive);
		return(-1);
	}

	sp->mac_media_type = ack.dl_mac_type;
	if (sp->mac_media_type == DL_ETHER)
		sp->mac_media_type = DL_CSMACD;
	sp->mac_min_sdu = ack.dl_min_sdu;
	sp->mac_max_sdu = ack.dl_max_sdu;
	sp->mac_addr_length = ack.dl_addr_length;

	convert_to_hwdep(mp);
	return(0);
}

char *
hw_addr(int fd, int getraddr)
{
	extern char *do_macioc_getaddr(int, int);
	unchar buf[sizeof(dl_phys_addr_ack_t) + 6];

	dl_phys_addr_req_t	*req;
	dl_phys_addr_ack_t	*ack;
	static char b[2][32];
	uchar_t addr[6];  /* DL_MAC_ADDR_LEN */
	uchar_t *x;

	struct strbuf ctl;
	int flags;

	if (type == ODIDLPI) {
		/* DL_PHYS_ADDR_REQ will be NAK'ed, so don't bother trying */

		/* note ODIDLPI doesn't have factory vs in-use address with
		 * variable getraddr
		 */
		struct strioctl si;

		si.ic_cmd = DLIOCGENADDR;
		si.ic_timout = 0;
		si.ic_dp = (char *)addr;
		si.ic_len = sizeof(addr);

		if (ioctl(fd, I_STR, &si) < 0) {
			if (debug_flag) {
				catfprintf(stderr, 39, "ndstat: %s ioctl failed\n", "MACIOC_GETMCA");
				perror("ndstat");
			}
			return(NULL);
		}
		x = addr;
		sprintf(b[getraddr], "%02x:%02x:%02x:%02x:%02x:%02x",
					*(x+0),*(x+1),*(x+2),*(x+3),*(x+4),*(x+5)
			);
		return(b[getraddr]);
	}

	/* MDI/LLI gets here */
	ctl.buf = (char *)buf;
	ctl.len = sizeof(dl_phys_addr_req_t);

	req = (dl_phys_addr_req_t *)buf;
	req->dl_primitive = DL_PHYS_ADDR_REQ;
	req->dl_addr_type = getraddr ? DL_FACT_PHYS_ADDR : DL_CURR_PHYS_ADDR;

	if (putmsg(fd, &ctl, NULL, RS_HIPRI) < 0) {
		fflush(stdout);
		if (debug_flag) {
			fprintf(stderr, "\n");
			catfprintf(stderr, 34, "ndstat: putmsg(%s) failed\n", "DL_PHYS_ADDR_REQ");
			perror("ndstat");
		}
		return(NULL);
	}

	ctl.maxlen = sizeof(buf);
	ctl.len = 0;

	flags=RS_HIPRI;
	if (getmsg(fd, &ctl, NULL, &flags) < 0) {
		fflush(stdout);
		if (debug_flag) {
			fprintf(stderr, "\n");
			catfprintf(stderr, 35, "ndstat: getmsg(%s) failed\n", "DL_PHYS_ADDR_ACK");
			perror("ndstat");
		}
		return(NULL);
	}
	if (flags != RS_HIPRI) {
		if (debug_flag) {
			fprintf(stderr, "\n");
			catfprintf(stderr,36,"ndstat: getmsg(%s) OK, but flags is wrong (=%x)\n", "DL_PHYS_ADDR_ACK", flags);
		}
		return(NULL);
	}
	ack = (dl_phys_addr_ack_t *)buf;
	if (ack->dl_primitive == DL_PHYS_ADDR_ACK) {
		x = (uchar_t *)ctl.buf+ack->dl_addr_offset;
		goto print_out;
	} else if (ack->dl_primitive != DL_ERROR_ACK) {
		if (debug_flag) {
			fprintf(stderr, "\n");
			catfprintf(stderr,38,"ndstat: getstats(%s) - Incorrect primitive returned (0x%x)\n", "DL_PHYS_ADDR_ACK", ack->dl_primitive);
		}
	return(NULL);
	}
	x = (uchar_t *)do_macioc_getaddr(fd,getraddr);
	if (x)
		goto print_out;
	return(NULL);
print_out:
	sprintf(b[getraddr], "%02x:%02x:%02x:%02x:%02x:%02x",
				*(x+0),*(x+1),*(x+2),*(x+3),*(x+4),*(x+5)
		);
	return(b[getraddr]);
}

int
mca_table(int fd)
{
	unchar	buf[256], *x;
	dl_mctable_req_t	*req;
	dl_mctable_ack_t	*ack;
	struct strbuf ctl;
	int flags;
	int len;
	unchar buffer[6][133];  /* most ODI/DLPI can only handle 16 though */
	
	if (type == ODIDLPI) {
		/* DL_MCTABLE_REQ is a SCOism (scodlpi.h) and ODI/DLPI will
		 * NAK it, so why try?
		 */
		struct strioctl si;
		int multicount;

#if 0
		si.ic_cmd = DLIOCADDMULTI;
		si.ic_timout = 0;
		si.ic_dp = (char *)&buffer[0][0];
		si.ic_len = 6;
		buffer[0][0]='\01';
		buffer[0][1]='\02';
		buffer[0][2]='\03';
		buffer[0][3]='\04';
		buffer[0][4]='\05';
		buffer[0][5]='\06';
		multicount=ioctl(fd, I_STR, &si);
		if (multicount < 0) {
			printf("ADDMULTI failed");
			perror("");
			exit(1);
		}
#endif

		/* DLIOCGETMULTI will fail unless you have P_DRIVER 
		 * priviledge 
		 */
		if (havepriv(P_DRIVER) != 1) return(0);

		si.ic_cmd = DLIOCGETMULTI;
		si.ic_timout = 0;
		si.ic_dp = (char *)&buffer[0][0];
		si.ic_len = 132 * 6;

		if ((multicount=ioctl(fd, I_STR, &si)) < 0) {
			if (debug_flag) {
				catfprintf(stderr, 39, "ndstat: %s ioctl failed\n", "DLIOCGETMULTI1");
				perror("ndstat");
			}
			return(-1);
		}

		if (multicount == 0) return(0);

		if (multicount > 132) multicount = 132;

		len = multicount * 6;
		goto start_dlpiodi;
	}
	/* MDI/LLI to get here */
	ctl.buf = (char *)buf;
	ctl.len = sizeof(dl_mctable_req_t);

	req = (dl_mctable_req_t *)buf;
	req->dl_primitive = DL_MCTABLE_REQ;

	if (putmsg(fd, &ctl, NULL, RS_HIPRI) < 0) {
		fflush(stdout);
		if (debug_flag) {
			catfprintf(stderr, 34, "ndstat: putmsg(%s) failed\n", "DL_MCTABLE_REQ");
			perror("ndstat");
		}
		return(-1);
	}

	ctl.maxlen = sizeof(buf);
	ctl.len = 0;

	flags=RS_HIPRI;
	if (getmsg(fd, &ctl, NULL, &flags) < 0) {
		fflush(stdout);
		if (debug_flag) {
			catfprintf(stderr, 35, "ndstat: getmsg(%s) failed\n", "DL_MCTABLE_ACK");
			perror("ndstat");
		}
		return(-1);
	}
	if (flags != RS_HIPRI) {
		if (debug_flag)
			catfprintf(stderr,36,"ndstat: getmsg(%s) OK, but flags is wrong (=%x)\n", "DL_MCTABLE_ACK", flags);
		return(-1);
	}
	ack = (dl_mctable_ack_t *)buf;

	if (ack->dl_primitive == DL_MCTABLE_ACK) {
		len = ack->dl_mctable_len;
		memcpy(buf, buf+ack->dl_mctable_offset,len); 
		goto print_out;
	} else if (ack->dl_primitive != DL_ERROR_ACK) {
		if (debug_flag)
			catfprintf(stderr,38,"ndstat: getstats(%s) - Incorrect primitive returned (0x%x)\n", "DL_MCTABLE_ACK", ack->dl_primitive);
		return(-1);
	}
	len = do_macioc_getmca(fd,buf, sizeof(buf));
	if (len == -1)
		return(-1);
print_out:
	if (ack->dl_all_mca) {
		printf("\n");
		catfprintf(stdout, 349, "\t\tReceive all multicast addresses: Enabled\n");
	}
	if (!len)
		return(0);
start_dlpiodi:
	printf("\n");
	catfprintf(stdout, 180,   "\t\tMulticast address table");
	catfprintf(stdout, 314, "\n\t\t-----------------------");
	if (type == ODIDLPI) {
		x=&buffer[0][0];
	} else {
		/* MDI/LLI */
		x=buf;
	}
	while (len) {
		printf("\n\t\t%02x:%02x:%02x:%02x:%02x:%02x",
				*(x+0),*(x+1),*(x+2),*(x+3),*(x+4),*(x+5));
		len -= 6;
		x+= 6;
	}
	printf("\n");
	return(0);
}


int
get_per_card_info(char *dev, struct per_card_info *cp) 
{

	char	*netname;
	char	netxcardinfo[32];
	int	corefd;
	struct	nlist	nl[] =
	{
		{ "" },
		{ (char *)NULL }
	};

	if (type != MDI) {
		if (debug_flag)
			fprintf(stderr, "skipping per_card_info: not an MDI driver\n");
		return(-1);
	}

	if (strncmp("/dev/", dev, 5) != 0) {
		if (debug_flag)
			fprintf(stderr, "Device name %s does not begin with \"/dev/net\"\n", dev);
		return(-1);
	}
	netname = dev + 5;
	if (strncmp("net", netname, 3) != 0) {
		if (debug_flag)
			fprintf(stderr, "Device name %s does not begin with \"/dev/net\"\n", dev);
		return(-1);
	}

	if ((corefd = open("/dev/kmem", O_RDONLY)) < 0) {
		if (debug_flag) 
			fprintf(stderr, "Open of /dev/kmem failed\n");
		return(-1);
	}
	
	strcpy(netxcardinfo, netname);
	strcat(netxcardinfo, "cardinfo");
	nl[0].n_name = netxcardinfo;
	if (nlist("/unix", nl) < 0) {
		if (debug_flag)
			fprintf(stderr, "nlist of /unix failed\n");
		return(-1);
	}

	if (nl[0].n_value == 0) {
		if (debug_flag)
			fprintf(stderr, "n_value = 0\n");
		return(-1);
	}

	lseek(corefd, (long)nl[0].n_value, 0);
	read(corefd, cp, sizeof(struct per_card_info));
	return(0);
}
