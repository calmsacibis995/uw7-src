#ident	"@(#)in.nbdgem.c	1.2 	9/29/97"

/*      Copyright (c) 1997 The Santa Cruz Operation, Inc.. All Rights
 *	Reserved.    
 */
	
/*      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF THE SANTA CRUZ
 *	OPERATION INC.  
 *      The copyright notice above does not evidence any        
 *      actual or intended publication of such source code.     
 */
/****************************************************************************
 *
 * NetBIOS Daemon - Gemini specific code
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
#include <net/if_dl.h>
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
extern struct nb_ifinfo *vp;	 	/* the write pointer */
extern	char		*name;

nl_catd catd;

#ifdef DEBUG
#include <assert.h>
#define ASSERT  assert
#else
#define ASSERT( x )
#endif

int naddr_conf 		= 0;		/* num addrs configured by user */

#define	DEV_ROUTE	"/dev/route"

/* flags for ifinfo_scan() */

#define SCAN_GETNUM 0 /* return the number of addresses on the system */
#define SCAN_FILL_ALL 1 /* fills nb_ifinfo array with all interface info */
#define SCAN_FILL_SEL 2 /* fills nb_ifinfo array according to supplied list */
#define USABLE_IF ((if_flags & IFF_UP) && (if_flags & (IFF_POINTTOPOINT | IFF_BROADCAST)))

/* IP device fd */
int ipfd;

/* NetBIOS device fd */
int nbfd;

unsigned long *aslist = NULL;	/* address search list */

/*
 * return the mtu for the interface "pname"
 */
short
if_getmtu(char *pname)
{
	struct ifreq ifr;
	struct strioctl ioc;
	int errn;

	strncpy(ifr.ifr_name, pname, IFNAMSIZ);
	ioc.ic_cmd = SIOCGIFMTU;
	ioc.ic_timout = 0;
	ioc.ic_len = sizeof(ifr);
	ioc.ic_dp = (caddr_t) &ifr;
	if (ioctl(ipfd, I_STR, (caddr_t) &ioc) < 0) {
		errn = errno;
		syslog(LOG_ERR, "%s:", pname);
		error(MSGSTR(MSG_NBD_IFMTU,"SIOCGIFMTU failed\n"), 1, errn);
	}
	return ifr.ifr_metric;
}

/*
 * Free off allocated memory
 */

void
ifinfo_free(struct ifa_msghdr *p)
{
	free(((char *)p) - sizeof(struct rt_giarg));
}

/*
 * Unpack the addresses from packed format (bitmask addrs
 * determines which addresses are present) into a managable 
 * rt_addrinfo structure.
 */
void
xaddrs(struct rt_addrinfo *info,
       struct sockaddr *sa,
       struct sockaddr *lim,
       int addrs)
{
	int i;
	static struct sockaddr sa_zero;
	struct sockaddr_in *sin;

#define RNDUP(a) ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) \
		    : sizeof(long))

	memset(info, 0, sizeof(*info));
	info->rti_addrs = addrs;
#ifdef DEBUG_NB
	printf( "xaddr: addrs 0x%x, sa 0x%x, lim 0x%x\n", addrs, sa, lim);
	printf( "size 0x%x\n", sizeof(struct sockaddr));
#endif
	for (i = 0; i < RTAX_MAX && sa < lim; i++) {
		if ((addrs & (1 << i)) == 0)
			continue;
		info->rti_info[i] = (sa->sa_len != 0) ? sa : &sa_zero;
#ifdef DEBUG_NB
		sin = (struct sockaddr_in *)info->rti_info[i];
		printf( "xaddr: index %d, len %d, ADDR %s\n", i, sa->sa_len,
			(char *)inet_ntoa(sin->sin_addr));
#endif
		sa = (struct sockaddr *)((char*)(sa)
					 + RNDUP(sa->sa_len));
	}
}

/* 
 * Add the addr/baddr paitr to the list (duplicate checking is performed
 * later).
 * If a user supplied list exists the pair is added only 
 * if it matches an element of the list.
 *
 * This routine modified vp - the write pointer.
 */

int 
add_list(struct sockaddr_in *sina, struct sockaddr_in *sinb, short mtu)
{
	int j;
	int add=0;
	struct nb_ifinfo *vpa; 
	unsigned long *plist;


	if(aslist) {
		plist = aslist;
		for(j=0; j<naddr_conf; j++, plist++) {
			if(SA(sina) != *plist){
#ifdef DEBUG_NB
				printf("ignoring addr:%s\n",
					inet_ntoa(SA(sina)));
#endif
				continue;
			}
			add++;
			break;
		}
	}
	else /* no user supplied list */
		add++;

	if(add) {
		vp->i_addr 	= SA(sina);
		vp->i_baddr 	= SA(sinb);
		vp->i_mtu 	= mtu;
#ifdef DEBUG_NB
		printf("found addr:%s ", inet_ntoa(vp->i_addr));
		printf("bc:%s\n", inet_ntoa(vp->i_baddr));
#endif
		vp++;
	}

	return add;

}

/* 
 * scan the information extracted using ifinfo_get
 * 
 * Performs tasks acording to flag
 * SCAN_GETNUM - return the number of addresses including loopback
 * SCAN_FILL_ALL - fills the nb_ifinfo array with all interface info
 *	           and returns the number found (ignoring loopback)
 * SCAN_FILL_SEL - fills the nb_ifinfo array according to the supplied list
 *                 of required addresses and returns number found
 */
int 
ifinfo_scan(struct ifa_msghdr *ifam_start, int iflen, int flag)
{
	struct in_addr addr;
	struct ifa_msghdr *ifam = ifam_start;
	struct if_msghdr *ifm;
	int if_flags;
	struct rt_addrinfo ainfo;
	struct sockaddr *sa, *sb;
	struct sockaddr_in *sina, *sinb;
	struct sockaddr_dl *sdl;
	int naddr=0;
	short mtu;

	if(flag == SCAN_FILL_ALL || flag == SCAN_FILL_SEL)
		vp = addrp;

	while ((char *)ifam < (char *)ifam_start + iflen) {

		ASSERT(ifam->ifam_type == RTM_IFINFO);

		ifm = (struct if_msghdr *)ifam;

		sdl = (struct sockaddr_dl *)(ifm + 1);
		sdl->sdl_data[sdl->sdl_nlen] = 0;

#ifdef DEBUG_NB
		printf("ifinfo_scan:  if_index = %d, Interface : %s\n",
			ifm->ifm_index, sdl->sdl_data);
#endif

		/* Save the flags */

		if_flags = ifm->ifm_flags;

		if(USABLE_IF)
			mtu = if_getmtu(sdl->sdl_data);

		/* Skip over this interface entry 
		 * and start scanning each address 
		 * for the interface
		 */ 

		ifam = (struct ifa_msghdr *)((char *)ifam + ifam->ifam_msglen);

		/* Check the list of addrs */

		while ((char *)ifam < (char *)ifam_start + iflen) {
			
			if (ifam->ifam_type != RTM_NEWADDR)
				break;

#ifdef DEBUG_NB
			printf("ifinfo_scan: RTM_NEWADDR, ifam_addrs 0x%x\n",
				ifam->ifam_addrs);
#endif

			/* Skip over this address entry */

			if (if_flags & IFF_LOOPBACK) {
				ifam = (struct ifa_msghdr *)
					((char *)ifam + ifam->ifam_msglen);
				continue;
			}

			if(!USABLE_IF) {
				/* Skip over this address entry */
				ifam = (struct ifa_msghdr *)
					((char *)ifam + ifam->ifam_msglen);
				continue;
			}


			if ( (ifam->ifam_addrs & RTA_IFA)  &&
			     (ifam->ifam_addrs & RTA_BRD) ) {

				xaddrs(&ainfo, (struct sockaddr *)(ifam+1),
				       (struct sockaddr *)
				       ((char *)ifam + ifam->ifam_msglen),
				       ifam->ifam_addrs);

				sa = ainfo.rti_info[RTAX_IFA];
				sb = ainfo.rti_info[RTAX_BRD];

			    	if (sa->sa_family == AF_INET) {

					sina = (struct sockaddr_in *)sa;
					sinb = (struct sockaddr_in *)sb;

					switch(flag) {
					case SCAN_GETNUM:
						naddr++;
						break;
					case SCAN_FILL_ALL:
					case SCAN_FILL_SEL:
						naddr+=add_list(sina,sinb,mtu);
						break;
					}

#ifdef DEBUG_NB
				/* don't concatenate these printf's or 
				 * the buffer used by inet_ntoa gets corrupted
				 */
				printf( "ifinfo_scan: RTM_NEWADDR, IFA %s",
					(char *)inet_ntoa(sina->sin_addr));
				printf( " BCAST %s\n",
					(char *)inet_ntoa(sinb->sin_addr));
#endif

				}

			}

			ifam = (struct ifa_msghdr *)
				((char *)ifam + ifam->ifam_msglen);
		}
	}	     
	return naddr;
}

/*
 * Get all the interface info
 */
struct ifa_msghdr *
ifinfo_get(int *len)
{
	struct rt_giarg *gp;
	int sysctl_buf_size = 0, needed;
	char *sysctl_buf;
	int rtfd;

	rtfd = open(DEV_ROUTE, O_RDWR);
	if (rtfd < 0) {
		error("IPCP - Failed to open routing driver", 0, errno);
		return NULL;
	}

	/* always need at least sizeof(rt_giarg) in
	 * the buffer
	 */
	sysctl_buf = (char *)malloc(sizeof(struct rt_giarg));
	sysctl_buf_size = sizeof(struct rt_giarg);

	for(;;) {
		gp = (struct rt_giarg *)sysctl_buf;
		gp->gi_op = KINFO_RT_DUMP;
		gp->gi_where = (caddr_t)sysctl_buf;
		gp->gi_size = sysctl_buf_size;
		gp->gi_arg = 0;

		if (ioctl(rtfd, RTSTR_GETIFINFO, sysctl_buf) < 0) {
			error("IPCP - RTSTR_GETIFINFO failed", 0, errno);
			free(sysctl_buf);
			close(rtfd);
			return NULL;
		}

		needed = gp->gi_size;
		if (sysctl_buf_size >= needed)
			break;
		
		free(sysctl_buf);
		sysctl_buf_size = needed;
		sysctl_buf = (char *)malloc(sysctl_buf_size);
	}

	close(rtfd);

	needed -= sizeof(struct rt_giarg);
	*len = needed;
	return (struct ifa_msghdr *)(sysctl_buf + sizeof(struct rt_giarg));
}
void
do_loopback() 
{
	vp = addrp;
	vp->i_addr  = htonl(INADDR_LOOPBACK); /* address */
	vp->i_baddr = htonl(0x7F0000FF); /* broadcast address */
	vp->i_mtu   = if_getmtu("lo0");
	vp++;
}

int 
do_addrs(char *str)
{
	char *p, *p1;
	int naddr=0;			/* number of search addresses */
	int i;
	unsigned long *plist;		/* pointer into address list */
	int iflen;
	struct ifa_msghdr *ifam_start, *ifam;

	cvtlow(str);

	if ((ipfd = open(_PATH_IP, O_RDONLY)) < 0) {
		perror(name);
		unlink(_PATH_PID);
		exit(1);
	}

	ifam = ifam_start = ifinfo_get(&iflen);
        if (!ifam_start){
		close(ipfd);
                return 0;
	}

	/* allocate an array large enough to take all the addresses
	 * on the system
	 */

	naddr = ifinfo_scan(ifam, iflen, SCAN_GETNUM);

#ifdef DEBUG_NB
	printf("number of addresses  %d\n", naddr);
#endif
	addrp=(struct nb_ifinfo *)malloc(sizeof(struct nb_ifinfo)*naddr);
	if(addrp == NULL) {
		perror(name);
		unlink(_PATH_PID);
		exit(1);
	}
	
	if (*str == NULL) {
		if ((naddr = ifinfo_scan(ifam, iflen, SCAN_FILL_ALL)) == 0){
			naddr = 1;
			do_loopback();
		}
        } 
	else if (strcmp(str,"localhost") == 0 || strcmp(str,"loopback") == 0){
		naddr = 1;
		do_loopback();
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
	
		naddr_conf = 0;

		while((p=strtok(p1, " 	")) != NULL){
			p1 = NULL;
			naddr_conf++;
		}
		
		if ((p1 = strdup(str)) == NULL)
			exit(1);

		aslist = (unsigned long *)malloc(naddr_conf*(sizeof addrp->i_addr));
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
		for (i=0; i<naddr_conf; i++){
			printf("search address:%s\n", inet_ntoa(*plist));
			plist++;
		}
#endif

		/* the address list (aslist) is filled with naddr_conf
		 * addesses - now go and find them.
		 */

		naddr = ifinfo_scan(ifam, iflen, SCAN_FILL_SEL);
	}

	ifinfo_free(ifam_start);

	close(ipfd);

	return naddr;
	
}
