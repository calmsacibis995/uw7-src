#ident	"@(#)in.nbdUW21.c	1.2"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1993 Lachman Technology, Inc.
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
/****************************************************************************
 *
 * NetBIOS Daemon - UNIXWARE 2.1 specific code
 *
 ****************************************************************************/

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/tiuser.h>
#include <sys/tihdr.h>
#include <sys/nb/nbuser.h>
#include <sys/nb/nb_const.h>
#include <sys/nb/nb_ioctl.h>
#include <sys/nb/nbtpi.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/sockio.h>
#include <string.h>
#include <ctype.h>
#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#include <net/if.h>
#include <netinet/in_f.h>
#include <net/route.h>
#include <netinet/in_var.h>
#include <netinet/ip_str.h>
#include <thread.h>
#include <netdb.h>
#include <syslog.h>
#include <locale.h>
#include "nb_msg.h"
#include "in.nbd.h"

extern struct nb_ifinfo *addrp;		/* where it starts */
extern struct nb_ifinfo *vp; 		/* the write pointer */
extern char	*name;

nl_catd catd;

/* void *tserver(struct nb_dns *nb_dns); */
void *tserver(void *arg);
void *sigserver(void *arg);
void namethread();
void sigthread();

/* TCP_KEEPIDLE and TCPTV_MIN_KEEPIDLE are defined for the new gemini
 * tcp stack, if on the old stack the ioctl will fail but the daemon will 
 * continue so you just can't set a keepidle time on the old stack
 */


/* NetBIOS device fd */
int nbfd;

int num_a;			/* number of addresses on system */
unsigned long *aslist;		/* address search list */


/*
 * return the destination address for a ppp link 
 */

unsigned long
get_pppdst(int fd, struct ifreq_all *ifra)
{
	struct ifreq ifr;
	struct strioctl ioc;
	int errn;

	strncpy(ifr.ifr_name,  ifra->if_entry.if_name, IFNAMSIZ);
	ioc.ic_cmd = SIOCGIFDSTADDR;
	ioc.ic_timout = 0;
	ioc.ic_len = sizeof(ifr);
	ioc.ic_dp = (caddr_t) &ifr;
	if (ioctl(fd, I_STR, (caddr_t) &ioc) < 0) {
		errn = errno;
		syslog(LOG_ERR, "%s:", ifra->if_entry.if_name);
		error(MSGSTR(MSG_NBD_IFDST,"SIOCGIFDSTADDR failed\n"), 1, errn);
	}

	return SA((struct sockaddr_in*)&ifr.ifr_dstaddr);
}

/* 
 * scans the ifreq_all structure and fills in the address array. 
 * Ignores loopback driver and those interfaces which aren't up
 * 
 * If n_found then the routine fills in relevant broadcast 
 * addresses, otherwise it gets both address and broadcast addresses
 */

int 
scan_ifa(int fd, struct ifreq_all *ifra, short mtu, int n_found)
{
	int i, j;
	int naddr=0;
	struct nb_ifinfo *vpa; 
	struct sockaddr_in *sin;
	u_long flags = ifra->if_entry.if_flags;
	unsigned long *plist;

	if (!((flags & IFF_UP)&&(flags & (IFF_POINTTOPOINT | IFF_BROADCAST))))
		return 0;

	for (i=0; i<ifra->if_naddr; i++) {
		sin = SINA(ifra,i);
		if (!(sin->sin_family & AF_INET))
			continue; 
		if (!n_found) {		/* configured to find all interfaces */
			vp->i_addr 	= SA(sin);
			vp->i_mtu 	= mtu;
			if(flags & IFF_BROADCAST) {
				sin   		= SINB(ifra, i);
				vp->i_baddr 	= SA(sin);
			}
			else			/* its point to point */
				vp->i_baddr = get_pppdst(fd, ifra);
#ifdef DEBUG_NB
			fprintf(stderr, "found addr:%s ", inet_ntoa(vp->i_addr));
			fprintf(stderr, "bc:%s\n", inet_ntoa(vp->i_baddr));
#endif
			vp++;
			naddr++;

		}
		else{		/* configured to use specified addresses */
			plist =aslist;
			for(j=0; j<n_found; j++, plist++) {
				if(SA(sin) != *plist){
#ifdef DEBUG_NB
					fprintf(stderr, "ignoring addr:%s\n",
						inet_ntoa(SA(sin)));
#endif
					continue;
				}
				vp->i_addr = *plist;
				vp->i_mtu = mtu;
				if(flags & IFF_BROADCAST) {
					sin 		= SINB(ifra, i);
					vp->i_baddr 	= SA(sin);
				}
				else 		/* its point to point */
					vp->i_baddr = get_pppdst(fd, ifra);
#ifdef DEBUG_NB
			fprintf(stderr,"found addr:%s ", inet_ntoa(vp->i_addr));
			fprintf(stderr,"bc:%s\n", inet_ntoa(vp->i_baddr));

#endif
				vp++;
				naddr++;
				break;
			}
		}
	}
	return naddr;
}

/*
 * return the mtu for the interface "pname"
 */

short
get_ifmtu(int fd, char *pname)
{
	struct ifreq ifr;
	struct strioctl ioc;
	int errn;

	strncpy(ifr.ifr_name, pname, IFNAMSIZ);
	ioc.ic_cmd = SIOCGIFMTU;
	ioc.ic_timout = 0;
	ioc.ic_len = sizeof(ifr);
	ioc.ic_dp = (caddr_t) &ifr;
	if (ioctl(fd, I_STR, (caddr_t) &ioc) < 0) {
		errn = errno;
		syslog(LOG_ERR, "%s:", pname);
		error(MSGSTR(MSG_NBD_IFMTU,"SIOCGIFMTU failed\n"), 1, errn);
	}
	return ifr.ifr_metric;
}

/* fill in interface information structure  */

int
get_ifall(int fd, int ifn, struct ifreq_all *ifra, short *mtu) 
{
	struct strioctl ioc;
	int errn;

	/*
	 * get all entries
	 */
	ioc.ic_cmd = SIOCGIFALL;
	ioc.ic_timout = 0;
	ioc.ic_len = sizeof(struct ifreq_all);
	ioc.ic_dp = (caddr_t) ifra;

	ifra->if_number = ifn;

	if (ioctl(fd, I_STR, (caddr_t) &ioc) < 0) {
		errn = errno;
		close(fd);
		error(MSGSTR(MSG_NBD_IFALL,"SIOCGIFALL failed\n"), 0, errn);
		return(-1);
	}

	/* 
	 * now get mtu 
 	 */

	*mtu = get_ifmtu(fd, ifra->if_entry.if_name);
}



/* return the number of addresses on the system */
int
get_numa(int fd) 
{
	struct ifreq ifr;
	struct strioctl ioc;

	ioc.ic_cmd = SIOCGIFANUM;
	ioc.ic_timout = 0;
	ioc.ic_len = sizeof(struct ifreq);
	ioc.ic_dp = (caddr_t) &ifr;
	
	if (ioctl(fd, I_STR, (caddr_t) &ioc) < 0) {
		perror(name);
		close(fd);
		return(-1);
	}

	return ifr.ifr_metric;
 
}

/* return the number of interfaces on the system */

int
get_numif(int fd)
{
	struct ifreq ifr;
	struct strioctl ioc;

	ioc.ic_cmd = SIOCGIFNUM;
	ioc.ic_timout = 0;
	ioc.ic_len = sizeof(struct ifreq);
	ioc.ic_dp = (caddr_t) &ifr;
	
	if (ioctl(fd, I_STR, (caddr_t) &ioc) < 0) {
		perror(name);
		close(fd);
		return(-1);
	}

	return ifr.ifr_metric;
 
}


/*
 * get the information on all the interfaces and fill 
 * in the address array
 * returns total number of entries obtained ;
 *
 * if addrp == null 
 *	all information is extracted.
 * else
 *	gets bc addrs matching existing addresses
 *
 */
int
if_all(int fd, int n_found)
{
	int i, ifn;
	int num_if;			/* number of interfaces */
	int nusable=0;			/* number of usable addrs for netbios */
	struct ifreq_all ifra;
	short mtu;

	num_if = get_numif(fd);

#ifdef DEBUG_NB
	fprintf(stderr,"number of interfaces %d\n", num_if);
#endif

	if (!num_a)
		return 0;

	vp = addrp;

	for (ifn=1; ifn<=num_if; ifn++) {
		get_ifall(fd, ifn, &ifra, &mtu);
		nusable += scan_ifa(fd, &ifra, mtu, n_found);
	}
	/* if (n_found && (nusable != n_found)){
	 *	syslog(LOG_ERR, MSGSTR(MSG_NBD_NOBCADDR,
	 *	   "%s: cannot find broadcast address\n"), name); 
	 *	exit(1);
	 * }
	 */

	return nusable;
}
void
do_loopback(int fd) 
{
	vp = addrp;
	vp->i_addr  = htonl(INADDR_LOOPBACK); /* address */
	vp->i_baddr = htonl(0x7F0000FF); /* broadcast address */
	vp->i_mtu   = get_ifmtu(fd, "lo0");
	vp++;
}

int 
do_addrs(char *str)
{
	char *p, *p1;
	int naddr=0;			/* number of search addresses */
	int i;
	int fd;
	unsigned long *plist;		/* pointer into address list */

	cvtlow(str);

	if ((fd = open(_PATH_IP, O_RDONLY)) < 0) {
		perror(name);
		unlink(_PATH_PID);
		exit(1);
	}

	/* allocate an array large enough to take all the addresses
	 * on the system
	 */

	num_a  = get_numa(fd);

#ifdef DEBUG_NB
	fprintf(stderr,"number of addresses  %d\n", num_a);
#endif
	addrp=(struct nb_ifinfo *)malloc(sizeof(struct nb_ifinfo)*num_a);
	if(addrp == NULL) {
		perror(name);
		unlink(_PATH_PID);
		exit(1);
	}
	
	if (*str == NULL) {
		if ((naddr = if_all(fd, 0)) == 0){
			naddr = 1;
			do_loopback(fd);
		}
        } 
	else if (strcmp(str,"localhost") == 0 || strcmp(str,"loopback") == 0){
		naddr = 1;
		do_loopback(fd);
        }
	else {
		/* we've been passed a list.
		 * Parse it to figure out how many we have
		 * Allocate memory for them
		 * and read them in
		 */

		if ((p1 = strdup(str)) == NULL)
			exit(1);

		/* figure out how many addresses we've been passed  (naddr)
		 * Allocate an array to store the search list in  (aslist)
		 * Fill in the search list 
		 */

		while((p=strtok(p1, " 	")) != NULL){
			p1 = NULL;
			naddr++;
		}

		if ((p1 = strdup(str)) == NULL)
			exit(1);

		aslist = (unsigned long *)malloc(naddr *(sizeof addrp->i_addr));
		if(aslist == NULL) {
			perror(name);
			exit(1);
		}

		plist = aslist;
		while((p=strtok(p1, " 	")) != NULL){
			if ((*plist++ = inet_addr(p)) == -1) {
				syslog(LOG_ERR, MSGSTR(MSG_NBD_INVADDR,
				   "%s:Invalid address: %s\n") ,name ,p);
				exit(1);
			}
			p1 = NULL;
		}

#ifdef DEBUG_NB
	plist=aslist;
	for (i=0; i< naddr; i++){
		fprintf(stderr,"search address:%s\n", inet_ntoa(*plist));
		plist++;
	}
#endif

		/* the address list (aslist) is filled with naddr addesses - 
		 * now go and find them
 		 * naddr gets reset to the number of useable interfaces
 		 * since there may be more than one interface with a 
		 * given address
		 */

		naddr = if_all(fd, naddr);
			
	}

	(void) close(fd);

	return naddr;
	
}
