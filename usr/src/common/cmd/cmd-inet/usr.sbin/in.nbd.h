#ident "@(#)in.nbd.h	1.2 9/29/97"
/*      Copyright (c) 1997 The Santa Cruz Operation, Inc.. All Rights
 *	Reserved.    
 */
	
	
/*      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF THE SANTA CRUZ
 *	OPERATION INC.  
 *      The copyright notice above does not evidence any        
 *      actual or intended publication of such source code.     
 */
#ifndef MSGSTR
#define MSGSTR(num,str) catgets(catd, MS_NB, (num), (str))
#endif
#ifndef MC_FLAGS
#define MC_FLAGS NL_CAT_LOCALE
#endif

/* TCP_KEEPIDLE and TCPTV_MIN_KEEPIDLE are defined for the new gemini
 * tcp stack, if on the old stack the ioctl will fail but the daemon will 
 * continue so you just can't set a keepidle time on the old stack
 */

#ifndef TCP_KEEPIDLE
#define TCP_KEEPIDLE		0x04	/* keepalive idle value */
#define	TCPTV_MIN_KEEPIDLE      (10)	/* minimum time before probing */
#endif



#define SINA(ifra, intno) ((struct sockaddr_in *)&((ifra)->addrs[intno].addr))
#define SA(sin) ((sin)->sin_addr.s_addr)
#define SINB(ifra, intno) ((struct sockaddr_in *)&((ifra)->addrs[intno].dstaddr))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define _PATH_TCP       "/dev/inet/tcp"
#define _PATH_UDP       "/dev/udp"
#define _PATH_NBD       "/dev/netbios0"
#define _PATH_NB        "/dev/netbios"
#define _PATH_IP        "/dev/ip"
#define _PATH_PID       "/etc/inet/nbd.pid"

/* number of endpoints to give driver at one time */
#define NENDPOINTS	8
/* minimum buffer size for driver ioctls */
#define MINBUF		512 

#ifdef	UNIXWARE2_1		/* S001 vvvvv */
#define	bzero(s, len)		memset(s, 0, len)
#define	bcopy(b1, b2, len)	memcpy(b2, b1, len)
#endif				/* S001 ^^^^^ */
