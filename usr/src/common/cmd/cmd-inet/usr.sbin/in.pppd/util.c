#ident	"@(#)util.c	1.2"
#ident	"$Header$"
extern char *gettxt();
#ident "@(#) $Id: util.c,v 1.6 1995/01/17 23:42:13 neil Exp"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1995 Legent Corporation
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
/*      SCCS IDENTIFICATION        */

#include <sys/types.h>
#include <sys/termios.h>
#include <sys/termiox.h>
#include <stdio.h>
#include <sys/errno.h>
#include <string.h>
#include <fcntl.h>
#include <stropts.h>
#include <sys/stream.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <net/if.h>

#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>

#include <net/if_arp.h>
#include <netinet/if_ether.h>

#ifndef _KERNEL
#define _KERNEL
#include <sys/ksynch.h>
#undef _KERNEL
#else
#include <sys/ksynch.h>
#endif

#include <netinet/ppp_lock.h>
#include <netinet/ppp.h>

#include <sys/dlpi.h>

#include "paths.h"
#include <errno.h>

#include <pfmt.h>
#include <locale.h>
#include <unistd.h>

#include "pppd.h"

extern char	IFNAME_PREF[80];
extern char	ifname[IFNAMSIZ];

struct baudstr {
	char	*pstr;
	int	rate;
} baudrates[] = {
#if defined(B50)
	{ "50",  B50},
#endif
#if defined(B75)
	{ "75",  B75},
#endif
#if defined(B110)
	{ "110",  B110},
#endif
#if defined(B134)
	{ "134",  B134},
#endif
#if defined(B150)
	{ "150",  B150},
#endif
#if defined(B200)
	{ "200",  B200},
#endif
#if defined(B300)
	{ "300",  B300},
#endif
#if defined(B600)
	{ "600",  B600},
#endif
#if defined(B1200)
	{"1200",  B1200},
#endif
#if defined(B1800)
	{"1800",  B1800},
#endif
#if defined(B2400)
	{"2400",  B2400},
#endif
#if defined(B4800)
	{"4800",  B4800},
#endif
#if defined(B9600)
	{"9600",  B9600},
	{"9.6",   B9600},
#endif
#if defined(B19200) || defined(EXTA)
#if !defined(B19200)
#define B19200 EXTA
#endif
	{"19200",B19200},
	{"19.2", B19200},
#endif
#if defined(B38400) || defined(EXTB)
#if !defined(B38400)
#define B38400 EXTB
#endif
	{"38400",B38400},
	{"38.4", B38400},
#endif
	{(char *)NULL, 0}
};

#define bcmp(s1, s2, len)	memcmp(s1, s2, len)
#define bcopy(s1, s2, len)	memcpy(s2, s1, len)
#define bzero(str,n)            memset((str), (char) 0, (n)); 


/* rm_route: remove a route from the routing table
 *	return 
 *	0 : succeed
 *	-1 : fail
 */
int
rm_route(dst)
struct sockaddr *dst;
{
	int	s;
	struct	rtentry	route;
	s = socket(AF_INET, SOCK_RAW, 0);
	if (s < 0) {
		util_syslog(gettxt(":347", "socket fail"));
		return(-1);
	}
	route.rt_flags = RTF_HOST;
	route.rt_dst = *dst;

	if (sioctl(s, SIOCDELRT,(caddr_t)&route, sizeof(route)) < 0) {
		util_syslog(gettxt(":348", "SIOCDELRT fail"));
		close(s);
		return(-1);
	}

	close(s);
	return(0);
}

/*
 * return termios format baudrate given a string describing it
 */
int getbaud(s)
	char	*s;
{
	int i;
	struct baudstr *bsp;

	bsp = baudrates;

	while (bsp->pstr != (char *)NULL) {
		if (strcmp(s, bsp->pstr) == 0) {
			return bsp->rate;
		}
		bsp++;
	}
	return 0;
}

/*
 * turn termios type baudrate into an integer.
 */
getbaudrate(b)
	int b;
{
	switch (b) {
#if defined(B50)
	case B50:
		return 50;
#endif
#if defined(B75)
	case B75:
		return 75;
#endif
#if defined(B110)
	case B110:
		return 110;
#endif
#if defined(B134)
	case B134:
		return 134;
#endif
#if defined(B150)
	case B150:
		return B150;
#endif
#if defined(B200)
	case B200:
		return 200;
#endif
#if defined(B300)
	case B300:
		return 300;
#endif
#if defined(B600)
	case B600:
		return 600;
#endif
#if defined(B1200)
	case B1200:
		return 1200;
#endif
#if defined(B2400)
	case B2400:
		return 2400;
#endif
#if defined(B4800)
	case B4800:
		return 4800;
#endif
#if defined(B9600)
	case B9600:
		return 9600;
#endif
#if defined(EXTA)
	case EXTA:
		return 19200;
#else
#if defined(B19200)
	case B19200:
		return 19200;
#endif
#endif
#if defined(EXTB)
	case EXTB:
		return 38400;
#else
#if defined(B38400)
	case B38400:
		return 38400;
#endif
#endif
	case B0:
	default:
		return 0xffff;
	}
}

/*
 * return 
 *	-1: fail
 *	0: succeed
 */
int
dlbind(fd,sap)
int	fd;
int	sap;
{
	struct strbuf   ctlbuf;
	dl_bind_req_t bind_req;
	dl_error_ack_t *error_ack;
	union DL_primitives dl_prim;
	int             flags = 0;

	bind_req.dl_primitive = DL_BIND_REQ;
	bind_req.dl_sap = sap;
	ctlbuf.len = sizeof(dl_bind_req_t);
	ctlbuf.buf = (char *) &bind_req;
	if (putmsg(fd, &ctlbuf, NULL, 0) < 0){
		util_syslog(gettxt(":350", "dlbind: putmsg fail"));
		return(-1);
	}

	ctlbuf.maxlen = sizeof(dl_prim);
	ctlbuf.len = 0;
	ctlbuf.buf = (char *) &dl_prim;
	if (getmsg(fd, &ctlbuf, NULL, &flags) < 0){
		util_syslog(gettxt(":351", "dlbind: getmsg fail"));
		return(-1);
	}
	switch (dl_prim.dl_primitive) {
	case DL_BIND_ACK:{
		dl_bind_ack_t *b_ack;

		b_ack = (dl_bind_ack_t *) &dl_prim;
		if (ctlbuf.len < sizeof(dl_bind_ack_t) ||
		    b_ack->dl_sap != sap){
			util_syslog(gettxt(":352", "dlbind: protocol error"));
			return(-1);
		}
		else{
			return 0;
		}
	}

	case DL_ERROR_ACK:
		if (ctlbuf.len < sizeof(dl_error_ack_t)){
			perror(gettxt(":352", "dlbind: protocol error"));
			return(-1);
		}
		else {
			error_ack = (dl_error_ack_t *) & dl_prim;
			switch (error_ack->dl_errno) {
			case DL_BADSAP:
				util_syslog(gettxt(":353", "dlbind: bad SAP"));
				return(-1);

			case DL_ACCESS:
				util_syslog(gettxt(":354", "dlbind: access error"));
				return(-1);

			case DL_SYSERR:
				util_syslog(gettxt(":355", "dlbind: system error"));
				return(-1);

			default:
				util_syslog(gettxt(":352", "dlbind: protocol error"));
				return(-1);
			}
		}

	default:
		util_syslog(gettxt(":352", "dlbind: protocol error"));
		return(-1);
	}
	/* NOTREACHED */
}

/*
 * Do a STREAMS ioctl call
 */
int
sioctl(fd, cmd, dp, len)
	int	fd, cmd, len;
	caddr_t	dp;
{
	struct strioctl	iocb;

	iocb.ic_cmd = cmd;
	iocb.ic_timout = 15;
	iocb.ic_dp = dp;
	iocb.ic_len = len;
	return ioctl(fd, I_STR, &iocb);
}

/* 
 * get an entry from a PPP configureation file 
 * entries may continue onto multiple lines by giving a '\' as the last
 * character of a line.
 * Comments start with '#'
 * return NULL: end of file or buffer too small
 */   
char *
pppfgets(line, n, fd)
	char *line;
	int n;
	FILE *fd;
{
	char tmp[PPPBUFSIZ];
	int i, j, len = 0;

	if (fgets(tmp, PPPBUFSIZ, fd) == NULL)
		return(NULL);

	while (len < n) {
		/* skip space, '\t' and '\n' */
		j = 0;
		while (tmp[j] == ' ' || tmp[j] == '\t' || tmp[j] == '\n')
			j++;
		
		/* look for '\0', '\n' and POUND key */
		for (i = j; tmp[i] != '\n' && tmp[i] != '\0' && tmp[i] != POUND; i++);
		if (tmp[i] == POUND || tmp[i] == '\n')
			tmp[i] = '\0';

		/*  not exceed line buffer size */ 
		if (len + i - j + 1 < n)
			bcopy(&tmp[j], &line[len], i - j + 1);
		else {
			util_syslog(gettxt(":71", "Config file entry too large"));
			return(NULL);
		}

		len += i - j + 1;

		/* If continue on next line, read again */ 
		if (len < 2 || line[len-2] != '\\') {
			return(line);
		}
		else {
			line[len-2] = ' ';
			/* reset line array pointer -- delete '\0' */
			len --;
		}
		if (fgets(tmp, PPPBUFSIZ, fd) == NULL)
			return(line);
	}
	return(NULL);
}



#include <sys/resource.h>
#include "pppu_proto.h"

#define NOFILES 20      /* just in case */

int
getdtablesize()
{
        struct rlimit   rl;

        if ( getrlimit(RLIMIT_NOFILE, &rl) == 0 )
                return(rl.rlim_max);
        else
                return(NOFILES);
}

int
control_hwd_flow(fd, enable)
int fd;
int enable;
{
  struct termiox termx;
    
  if (sioctl(fd, TCGETX, (caddr_t) &termx, sizeof(termx)) == -1) 
    return(-1);

  if (enable)
    termx.x_hflag |= (RTSXOFF | CTSXON);
  else
    termx.x_hflag &= ~(RTSXOFF | CTSXON);

  if (sioctl(fd, TCSETX, (caddr_t) &termx, sizeof(termx)) == -1) 
  return(-1);

  return(0);

}




