#ifndef	_NET_SOCKMOD_H	/* wrapper symbol for kernel use */
#define	_NET_SOCKMOD_H	/* subject to change without notice */

#ident	"@(#)sockmod.h	1.6"
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
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */
#include <io/stream.h>	/* REQUIRED */
#include <net/tiuser.h>	/* REQUIRED */
#include <net/un.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */
#include <sys/stream.h>	/* REQUIRED */
#include <sys/tiuser.h>	/* REQUIRED */
#include <sys/un.h>	/* REQUIRED */

#else

/* for sockaddr_un */
#include <sys/un.h> /* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

/* internal flags - in addition to the ones in timod.h */
#define	S_WINFO		0x01000		/* waiting for T_info to complete */
#define	S_WRDISABLE	0x02000		/* write service queue disabled */
#define	S_WUNBIND	0x04000		/* waiting on T_OK_ACK for T_UNBIND_REQ */
#define	S_RBLOCKED	0x08000		/* read side is/was blocked */
#define	S_WBLOCKED	0x10000		/* write side is/was blocked */
#define	S_WCLOSE	0x20000		/* Waiting to free the so_so, but
					   have pending esballoc'ed msgs.  */
#define S_CLOSING	0x40000		/* closing down socket */
#define S_XPG4		0x80000		/* provider supports T_ADDR_REQ */	
#define S_AFUNIXL	0x100000	/* local address family is AF_UNIX */
#define S_AFUNIXR	0x200000	/* remote address family is AF_UNIX */
#define S_BUSY		0x400000	/* do_ERROR in progress */
#define S_DCE		0x800000	/* special error handling for DCE */
#define S_IPV6MAPPED	0x1000000	/* IPv4 mapped IPv6 addresses */
#define S_ACCEPTING	0x2000000	/* performing accept() in library */

#endif /* _KERNEL || _KMEMUSER */

/* socket module ioctls */
#define	SIMOD 		('I'<<8)

/*
 * The following are ioctl handled specially by the socket
 * module which were not handled by timod.
 */
#define	SI_GETUDATA		(SIMOD|101)
#define	SI_SHUTDOWN		(SIMOD|102)
#define	SI_LISTEN		(SIMOD|103)
#define	SI_SETMYNAME		(SIMOD|104)
#define	SI_SETPEERNAME		(SIMOD|105)
#define	SI_GETINTRANSIT		(SIMOD|106)
#define	SI_TCL_LINK		(SIMOD|107)
#define	SI_TCL_UNLINK		(SIMOD|108)
/*
 *	added support for netstat to walk down the so_ux_list
 */
#define	SI_UX_COUNT		(SIMOD|109)
#define	SI_UX_LIST		(SIMOD|110)
/*
 *	special for DCE
 */
#define SI_ETOG			(SIMOD|111)
/*
 * 	add support for accept interleaving
 */
#define SI_SET_SEM		(SIMOD|112) 

/*
 * UnixWare 2.1 version of si_udata structure. Maintained
 * for UDK compatibility within the socket library.
 */ 
struct si_udata_u21 {
	int	tidusize;	/* TIDU size          */
	int	addrsize;	/* address size	      */
	int	optsize;	/* options size	      */
	int	etsdusize;	/* expedited size     */
	int	servtype;	/* service type       */
	int	so_state;	/* socket states      */
	int	so_options;	/* socket options     */
};

struct si_udata {
	int	tidusize;	/* TIDU size          */
	int	addrsize;	/* address size	      */
	int	optsize;	/* options size	      */
	int	etsdusize;	/* expedited size     */
	int	servtype;	/* service type       */
	int	so_state;	/* socket states      */
	int	so_options;	/* socket options     */
	key_t	sem_key;	/* semaphore key */
	int	sem_id;		/* semaphore identifier */
	short	sem_num;	/* semaphore number   */
};

struct _si_user {
	struct	_si_user 	*next;		/* next one 	      */
	struct	_si_user 	*prev;		/* previous one	      */
	int		  	fd;		/* file descripter    */
	int		  	ctlsize;	/* ctl buffer size    */
	char   		 	*ctlbuf;	/* ctl buffer         */
	int			family;		/* protocol family    */
	struct	si_udata	udata;		/* socket info	      */
	int			flags;
	ssize_t		  	ownsize;	/* local name size    */
	struct sockaddr	 	*ownname;	/* local name         */
	ssize_t		  	peersize;	/* remote name size   */
	struct sockaddr	 	*peername;	/* remote name        */
};

/*
 * Flag bits.
 */
#define		S_SIGIO		0x1	/* If set, user has SIGIO enabled */
#define		S_SIGURG	0x2	/* If set, user has SIGURG enabled */
/* For XPG4.2 conformance (used in libsocket/socket/s_ioctl.c) */
#define		S_PGRP		0x4	/* If set, user has SETOWN pgrp id */
#define		S_NOSEM		0x8	/* If set, sockmod does not support 
					   semaphores */
/*
 * Used for the tortuous UNIX domain
 * naming.
 */
struct ux_dev {
	dev_t	dev;
	ino_t	ino;
};

struct ux_extaddr {
	size_t	size;				/* Size of following address */
	union	{
		struct ux_dev	tu_addr;	/* User selected address */
		int		tp_addr;	/* TP selected address */
	} addr;
};
#define	extdev		ux_extaddr.addr.tu_addr.dev
#define	extino		ux_extaddr.addr.tu_addr.ino
#define	extsize		ux_extaddr.size
#define	extaddr		ux_extaddr.addr

struct bind_ux {
	struct	sockaddr_un	name;
	struct	ux_extaddr	ux_extaddr;
};

/*
 * 	support structure for netstat AF_UNIX to support SI_UX_LIST ioctl
 */
struct	soreq {
	void			*so_addr;	/* address of this so */
	int			servtype;	/* service type       */
	void			*so_conn;
	struct	ux_extaddr	lux_dev;
	struct	netbuf		laddr;
	struct	sockaddr_un	sockaddr;
};

/*
 * Semaphore structure for accept interleaving.
 */
struct si_sem {
	int	sem_id;
	short	sem_num;
};

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Doubly linked list of so_so (used to track UNIX domain sockets as well
 * as all existing sockmod instantiations)
 */
struct so_list {
	struct so_so *next;
	struct so_so *prev;
};

struct queue;		/* This is for user level compilations to succeed */
struct msgb;		/* This is for user level compilations to succeed */


struct so_so {
	lock_t			*so_lock;
	event_t			*so_event;
	long 			flags;
	struct queue		*rdq;
	struct msgb  		*iocsave;
	struct t_info		tp_info;
	struct netbuf		raddr;
	struct netbuf		laddr;
	struct ux_extaddr	lux_dev;
	struct ux_extaddr	rux_dev;
	int			so_error;
	struct msgb  		*oob;
	struct so_so		*so_conn;
	struct msgb		*consave;
	struct si_udata		udata;
	int			so_option;
	struct msgb  		*bigmsg;
	struct so_list		so_ux;
	struct so_list		so_list;
	int			hasoutofband;
	struct msgb		*urg_msg;
	struct msgb		*so_lowmem;
	int			sndbuf;
	int			rcvbuf;
	int			sndlowat;
	int			rcvlowat;
	int			linger;
	int			sndtimeo;
	int			rcvtimeo;
	int			prototype;
	int			esbcnt;
	toid_t			so_bid;
	struct msgb		*conninds;
};

#endif /* _KERNEL || _KMEMUSER */

#if !defined(_KERNEL)

extern struct _si_user 	*_s_checkfd();
extern struct _si_user 	*_s_open();
extern void 		 _s_aligned_copy();
extern struct netconfig	*_s_match();
extern int 	 	 _s_sosend();
extern int		 _s_soreceive();
extern int 		 _s_getudata();
extern int 		 _s_is_ok();
extern int 		 _s_do_ioctl();
extern int 		 _s_min();
extern int		 _s_max();
extern void		 _s_close();
extern int		 _s_getfamily();
extern int		 _s_uxpathlen();
extern void		 (*sigset())();
extern struct _si_user	*_s_fdmap();
extern struct _si_user	*_s_fdlookup();
extern void		_s_fdunmap();
extern int		_s_init_sem();
extern int 		_s_P();
extern int		_s_V();

/*
 * Socket library debugging
 */
extern int		_s_sockdebug;
#define	SOCKDEBUG(S, A, B)	\
			if ((((S) && (S)->udata.so_options & SO_DEBUG)) || \
						_s_sockdebug) { \
				(void)syslog(LOG_ERR, (A), (B)); \
			}

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_SOCKMOD_H */
