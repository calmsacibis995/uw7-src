#ifndef	_NET_SOCKIO_H	/* wrapper symbol for kernel use */
#define	_NET_SOCKIO_H	/* subject to change without notice */

#ident	"@(#)sockio.vh	1.4"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *  	          All rights reserved.
 */

#ifdef _KERNEL_HEADERS

#include <fs/ioccom.h> /* SVR4.0COMPAT */
#include <net/convsa.h>

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/ioccom.h> /* SVR4.0COMPAT */
#include <sys/convsa.h>

#else

/* General socket ioctl definitions. */
#include <sys/ioccom.h> /* SVR4.0COMPAT */
#include <sys/convsa.h>

#endif /* _KERNEL_HEADERS */

#if !(defined(_KERNEL) || defined(INKERNEL) || defined(_INKERNEL))
/*
 * This macro is private to the implmentation of libsocket.
 */
#ifdef _SOCK_ABI
VERSION_DECLARE(__NETLIB_VERSION__) {
	VERSION_OPTIONS("-I -i macro -p");
	extern int _s_ioctl(int, int, ... /* void * */);
}
#endif /* _SOCK_ABI */

/*
 * The socket library needs to intercept the socket ioctls to do various
 * processing and sockaddr conversion, depending on the interface version.
 */
#ifndef __IOCTL_VERSIONED__
#define __IOCTL_VERSIONED__
VERSION_DECLARE(__NETLIB_VERSION__) {
	VERSION_OPTIONS("-I -i func -p");
	extern int ioctl(int, int, ... /* void * */);
}
#endif /* !__IOCTL_VERSIONED__ */

#endif /* !(defined(_KERNEL) || defined(INKERNEL) || defined(_INKERNEL)) */

#if 1 /* XXX support for new networking code */
#define SIOCGPCBSIZ     _IO('p', 2)                     /* get # of pcbs */
#define SIOCGPCB        _IOR('p', 3, struct pcbrecord)  /* get all pcbs */
#define SIOCGIPSTATS    _IOR('p', 4, struct ipstat)     /* get ipstat */
#define SIOCGICMPSTATS  _IOR('p', 5, struct icmpstat)   /* get icmpstat */
#define SIOCGUDPSTATS   _IOR('p', 6, struct udpstat)    /* get udpstat */
#define SIOCGTCPSTATS   _IOR('p', 7, struct tcpstat)    /* get tcpstat */

#define	SIOCFLUSHRT	_IOW('r', 12, struct ortentry)	/* flush routes */
#define	SIOCGRTSIZ	_IO('r', 13)			/* get # of ortentry */
#define	SIOCGRTTAB	_IOR('r', 14, struct ortentry)	/* get route tables */
#define	SIOCGRTSTATS	_IOR('r', 15, struct rtstat)	/* get route statistic*/

#define SIOCGIFSTATS	_IOR('i', 74, struct ifstats)	/* get one ifstats */
#define SIOCGIFSTATS_ALL _IOR('i', 75, struct ifstats)	/* get all ifstats */

#define SIOCGIFONEP     _IOW('i', 42, struct ifreq)     /* get one-packet */
#define SIOCSIFONEP     _IOW('i', 43, struct ifreq)     /* set one-packet */

#define	SIOCGARPSIZ	_IO('i', 33)                   	/* get arptab size */
#define	SIOCGARPTAB	_IOR('i', 34, struct arptab)	/* get arptab */

#define SIOTCPGDEBUG	_IO('i', 35)	/* get tcp_ndebug */
#define SIOTCPGDEBX	_IO('i', 36)	/* get tcp_debx addr */
#define SIOTCPGDATA	_IO('i', 37)	/* get tcp_debx values */

#define	SIOCSHRDTYPE	_IOW('i', 38, int)		/* set hardware type */
#endif

/* socket i/o controls */
#define	SIOCSHIWAT	_IOW('s',  0, int)		/* set high watermark */
#define	SIOCGHIWAT	_IOR('s',  1, int)		/* get high watermark */
#define	SIOCSLOWAT	_IOW('s',  2, int)		/* set low watermark */
#define	SIOCGLOWAT	_IOR('s',  3, int)		/* get low watermark */
#define	SIOCATMARK	_IOR('s',  7, int)		/* at oob mark? */
#define	SIOCSPGRP	_IOW('s',  8, int)		/* set process group */
#define	SIOCGPGRP	_IOR('s',  9, int)		/* get process group */

#define	SIOCADDRT	_IOW('r', 10, struct ortentry)	/* add route */
#define	SIOCDELRT	_IOW('r', 11, struct ortentry)	/* delete route */
#define SIOCGETVIFCNT	_IOWR('r',12, struct sioc_vif_req)/*get vif count info*/
#define SIOCGETSGCNT	_IOWR('r', 13, struct sioc_sg_req) /*get mroute stats*/

#define	SIOCSIFADDR	_IOW('i', 12, struct ifreq)	/* set ifnet address */
#define	SIOCGIFADDR	_IOWR('i',13, struct ifreq)	/* get ifnet address */
#define	SIOCSIFDSTADDR	_IOW('i', 14, struct ifreq)	/* set p-p address */
#define	SIOCGIFDSTADDR	_IOWR('i',15, struct ifreq)	/* get p-p address */
#define	SIOCSIFFLAGS	_IOW('i', 16, struct ifreq)	/* set ifnet flags */
#define	SIOCGIFFLAGS	_IOWR('i',17, struct ifreq)	/* get ifnet flags */
#define	SIOCSIFMEM	_IOW('i', 18, struct ifreq)	/* set interface mem */
#define	SIOCGIFMEM	_IOWR('i',19, struct ifreq)	/* get interface mem */
#define	SIOCGIFCONF	_IOWR('i',20, struct ifconf)	/* get ifnet list */
#define	SIOCSIFMTU	_IOW('i', 21, struct ifreq)	/* set if_mtu */
#define	SIOCGIFMTU	_IOWR('i',22, struct ifreq)	/* get if_mtu */

	/* from 4.3BSD */
#define	SIOCGIFBRDADDR	_IOWR('i',23, struct ifreq)	/* get broadcast addr */
#define	SIOCSIFBRDADDR	_IOW('i',24, struct ifreq)	/* set broadcast addr */
#define	SIOCGIFNETMASK	_IOWR('i',25, struct ifreq)	/* get net addr mask */
#define	SIOCSIFNETMASK	_IOW('i',26, struct ifreq)	/* set net addr mask */
#define	SIOCGIFMETRIC	_IOWR('i',27, struct ifreq)	/* get IF metric */
#define	SIOCSIFMETRIC	_IOW('i',28, struct ifreq)	/* set IF metric */

#define	SIOCSARP	_IOW('i', 30, struct arpreq)	/* set arp entry */
#define	SIOCGARP	_IOWR('i',31, struct arpreq)	/* get arp entry */
#define	SIOCDARP	_IOW('i', 32, struct arpreq)	/* delete arp entry */
#define	SIOCUPPER       _IOW('i', 40, struct ifreq)       /* attach upper layer */
#define	SIOCLOWER       _IOW('i', 41, struct ifreq)       /* attach lower layer */
#define	SIOCSETSYNC	_IOW('i',  44, struct ifreq)	/* set syncmode */
#define	SIOCGETSYNC	_IOWR('i', 45, struct ifreq)	/* get syncmode */
#define	SIOCSSDSTATS	_IOWR('i', 46, struct ifreq)	/* sync data stats */
#define	SIOCSSESTATS	_IOWR('i', 47, struct ifreq)	/* sync error stats */

#define	SIOCSPROMISC	_IOW('i', 48, int)		/* request promisc mode
							   on/off */
#define	SIOCADDMULTI	_IOW('i', 49, struct ifreq)	/* set m/c address */
#define	SIOCDELMULTI	_IOW('i', 50, struct ifreq)	/* clr m/c address */

#define	SIOCSIFTYPE	_IOW('i', 51, struct ifreq)	/* set if MIB type */
#define	SIOCGIFTYPE	_IOR('i', 52, struct ifreq)	/* get if MIB type */

#define	SIOCSIFPERF	_IOW('i', 53, struct ifreq)	/* set if perf info */
#define	SIOCGIFPERF	_IOR('i', 54, struct ifreq)	/* get if perf info */

/* protocol i/o controls */
#define	SIOCSNIT	_IOW('p',  0, struct nit_ioc)	/* set nit modes */
#define	SIOCGNIT	_IOWR('p', 1, struct nit_ioc)	/* get nit modes */

/* STREAMS based socket emulation */

#define SIOCPROTO	_IOW('s', 51, struct socknewproto)	/* link proto */
#define SIOCGETNAME	_IOR('s', 52, struct sockaddr)	/* getsockname */
#define SIOCGETPEER	_IOR('s', 53, struct sockaddr)	/* getpeername */
#define IF_UNITSEL	_IOW('s', 54, int)	/* set unit number */
#define SIOCXPROTO	_IO('s', 55)	/* empty proto table */

#define SIOCIFDETACH	_IOW('i', 56, struct ifreq)	/* detach interface */
#define SIOCGENPSTATS	_IOWR('i', 57, struct ifreq)	/* get ENP stats */
#define SIOCX25XMT	_IOWR('i', 59, struct ifreq)	/* start a slp proc in
							 * x25if */
#define SIOCX25RCV	_IOWR('i', 60, struct ifreq)	/* start a slp proc in
							 * x25if */
#define SIOCX25TBL	_IOWR('i', 61, struct ifreq)	/* xfer lun table to
							 * kernel */
#define SIOCSLGETREQ	_IOWR('i', 71, struct ifreq)	/* wait for switched
							 * SLIP request */
#define SIOCSLSTAT	_IOW('i', 72, struct ifreq)	/* pass SLIP info to
							 * kernel */
#define SIOCSIFNAME	_IOW('i', 73, struct ifreq)	/* set interface name */
#define SIOCGENADDR	_IOWR('i', 85, struct ifreq)	/* Get ethernet addr */
#define SIOCSOCKSYS	_IOW('i', 86, struct socksysreq)/* Pseudo socket syscall */
#define SIOCSIFDEBUG	_IOW('i', 87, struct ifreq)	/* set debug level */
#define SIOCGIFDEBUG	_IOWR('i', 88, struct ifreq)	/* get debug level */

#define	SIOCGIFALL	_IOWR('i',89, struct ifreq_all)	/* get interface info */
#define	SIOCGIFNUM	_IOWR('i', 90, struct ifreq)	/* get # of interface */
#define	SIOCGIFANUM	_IOWR('i', 91, struct ifreq)	/* get # of addresses */
#define	SIOCGIFINDEX	_IOWR('i',92, struct ifreq)	/* get if_index */

#define	SIOCAIFADDR	_IOW('i', 93, struct ifaliasreq)/* add alias address */
#define	SIOCDIFADDR	_IOW('i', 94, struct ifreq)	/* del alias address */
#define	SIOCGIFALIAS	_IOWR('i', 95, struct ifaliasreq)/* get alias address */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_SOCKIO_H */
