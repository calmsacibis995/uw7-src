#ifndef	_NET_SOCKET_H	/* wrapper symbol for kernel use */
#define	_NET_SOCKET_H	/* subject to change without notice */

#ident	"@(#)socket.vh	1.3"
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
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ifdef _KERNEL_HEADERS

#include <net/inet/byteorder.h>		/* REQUIRED */
#include <util/types.h>			/* REQUIRED */
#include <net/bitypes.h>		/* REQUIRED */
#include <net/convsa.h>			/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/byteorder.h>		/* REQUIRED */
#include <sys/types.h>			/* REQUIRED */
#include <sys/bitypes.h>		/* REQUIRED */
#include <sys/convsa.h>			/* REQUIRED */

#else

#if (defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1)
#include <sys/types.h>
#include <sys/uio.h>

#define SHUT_RD  0
#define SHUT_WR  1
#define SHUT_RDWR 2

#else
#include <sys/byteorder.h>	/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */
#include <sys/bitypes.h>	/* REQUIRED */
#include <sys/netconfig.h> 	/* SVR4.0COMPAT */
#endif /* (defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1)*/

#include <sys/convsa.h>			/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if !(defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1)
/*
 * Definitions related to sockets: types, address families, options.
 */

#ifndef NC_TPI_CLTS
#define NC_TPI_CLTS	1		/* must agree with netconfig.h */
#define NC_TPI_COTS	2		/* must agree with netconfig.h */
#define NC_TPI_COTS_ORD	3		/* must agree with netconfig.h */
#define	NC_TPI_RAW	4		/* must agree with netconfig.h */
#endif /* !NC_TPI_CLTS */
#endif /* !(defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1)*/

/*
 * Types
 */

#define	SOCK_STREAM	2	/* stream socket - must be the same as NC_TPI_COTS */
#define	SOCK_DGRAM	1	/* datagram socket  - must be the same as NC_TPI_CLTS*/
#define	SOCK_RAW	4	/* raw-protocol interface  - must be the same as NC_TPI_RAW*/
#define	SOCK_RDM	5		/* reliably-delivered message */
#define	SOCK_SEQPACKET	6		/* sequenced packet stream */
/*
 * Option flags per-socket.
 */
#define	SO_DEBUG	0x0001		/* turn on debugging info recording */
#define	SO_ACCEPTCONN	0x0002		/* socket has had listen() */
#define	SO_REUSEADDR	0x0004		/* allow local address reuse */
#define	SO_KEEPALIVE	0x0008		/* keep connections alive */
#define	SO_DONTROUTE	0x0010		/* just use interface addresses */
#define	SO_BROADCAST	0x0020		/* permit sending of broadcast msgs */
#define	SO_USELOOPBACK	0x0040		/* bypass hardware when possible */
#define	SO_LINGER	0x0080		/* linger on close if data present */
#define	SO_OOBINLINE	0x0100		/* leave received OOB data in line */
#define SO_ORDREL	0x0200		/* give use orderly release */
#define SO_IMASOCKET	0x0400		/* use socket semantics */
#define SO_MGMT		0x0800		/* is used for mgmt. purposes */
#define SO_REUSEPORT	0x1000		/* allow local port reuse */
#define SO_LISTENING	0x2000		/* in the process of listen()ing */

/*
 * N.B.: The following definition is present only for compatibility
 * with release 3.0.  It will disappear in later releases.
 */
#define	SO_DONTLINGER	(~SO_LINGER)	/* ~SO_LINGER */

/*
 * Additional options, not kept in so_options.
 */
#define	SO_SNDBUF	0x1001		/* send buffer size */
#define	SO_RCVBUF	0x1002		/* receive buffer size */
#define	SO_SNDLOWAT	0x1003		/* send low-water mark */
#define	SO_RCVLOWAT	0x1004		/* receive low-water mark */
#define	SO_SNDTIMEO	0x1005		/* send timeout */
#define	SO_RCVTIMEO	0x1006		/* receive timeout */
#define	SO_ERROR	0x1007		/* get error status and clear */
#define	SO_TYPE		0x1008		/* get socket type */
#define SO_PROTOTYPE	0x1009		/* get/set protocol type */

/*
 * Structure used for manipulating linger option.
 */
struct	linger {
	int	l_onoff;		/* option on/off */
	int	l_linger;		/* linger time */
};

/*
 * Level number for (get/set)sockopt() to apply to socket itself.
 */
#define	SOL_SOCKET	0xffff		/* options for socket level */

#if defined(_KERNEL) || defined(_KMEMUSER)
/*
 * For kernel internal use only.
 * Current maximum size of all SOL_SOCKET level options.
 * This value includes the "struct opthdr" overhead.
 */

#define SOL_SOCKET_MAXSZ	276
#endif

/*
 * Address families.
 */
#define AF_UNSPEC       0		/* unspecified (must use sockaddr) */
#define AF_UNIX         1		/* local to host (pipes, portals) */
#define AF_LOCAL	AF_UNIX		/* synonym for above */
#define AF_INET         2		/* internetwork: UDP, TCP, etc. */
#define AF_IMPLINK      3		/* arpanet imp addresses */
#define AF_PUP          4		/* pup protocols: e.g. BSP */
#define AF_CHAOS        5		/* mit CHAOS protocols */
#define AF_NS           6		/* XEROX NS protocols */
#define AF_NBS          7		/* nbs protocols */
#define AF_ECMA         8		/* european computer manufacturers */
#define AF_DATAKIT      9		/* datakit protocols */
#define AF_CCITT        10		/* CCITT protocols, X.25 etc */
#define AF_SNA          11		/* IBM SNA */
#define AF_DECnet       12		/* DECnet */
#define AF_DLI          13		/* Direct data link interface */
#define AF_LAT          14		/* LAT */
#define AF_HYLINK       15		/* NSC Hyperchannel */
#define AF_APPLETALK    16		/* Apple Talk */
#define AF_NIT          17		/* Network Interface Tap */
#define AF_802          18		/* IEEE 802.2, also ISO 8802 */
#define AF_OSI          19		/* umbrella for all families used */
#define AF_ISO          AF_OSI		/* umbrella for all families used */
#define AF_X25          20		/* CCITT X.25 in particular */
#define AF_OSINET       21		/* AFI = 47, IDI = 4 */
#define AF_GOSIP        22		/* U.S. Government OSI */
#define AF_YNET         23		/* */
#define AF_ROUTE        24		/* */
#define AF_LINK         25		/* */
#if !(defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1)
#define pseudo_AF_XTP   26		/* */
#endif
#define AF_INET6	27		/* Internet Protocol Version 6 */
#define	AF_MAX		27

#define AF_INET_BSWAP	0x0200		/* ushort_t byte-swapped AF_INET */

/*
 * Structure used by kernel to store most
 * addresses.
 */
/* saus_ and s. need to be changed to sa_saus and sa_s  for namespace */
struct sockaddr {
	union {
	    struct {
#ifdef __NEW_SOCKADDR__
		sa_len_t	sa_saus_len;	/* length of struct */
#endif
		sa_family_t	sa_saus_family;	/* address family */
		char		sa_saus_data[14];/* 14 bytes of data */
	    } sa_s;
	    unsigned long _align;		/* to restrict alignment */
	} sa_un;
#ifdef __NEW_SOCKADDR__
#define sa_len		sa_un.sa_s.sa_saus_len
#endif
#define sa_family 	sa_un.sa_s.sa_saus_family
#define sa_data 	sa_un.sa_s.sa_saus_data
};

#if !( defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1)
/*
 * Structure used by kernel to pass protocol
 * information in raw sockets.
 */
struct sockproto {
	ushort_t	sp_family;		/* address family */
	ushort_t	sp_protocol;		/* protocol */
};
#endif /* !( defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1)*/

/*
 * Protocol families, same as address families for now.
 */
#define	PF_UNSPEC	AF_UNSPEC
#define	PF_UNIX		AF_UNIX
#define PF_LOCAL	AF_LOCAL
#define	PF_INET		AF_INET
#define	PF_IMPLINK	AF_IMPLINK
#define	PF_PUP		AF_PUP
#define	PF_CHAOS	AF_CHAOS
#define	PF_NS		AF_NS
#define	PF_NBS		AF_NBS
#define	PF_ECMA		AF_ECMA
#define	PF_DATAKIT	AF_DATAKIT
#define	PF_CCITT	AF_CCITT
#define	PF_SNA		AF_SNA
#define	PF_DECnet	AF_DECnet
#define	PF_DLI		AF_DLI
#define	PF_LAT		AF_LAT
#define	PF_HYLINK	AF_HYLINK
#define	PF_APPLETALK	AF_APPLETALK
#define	PF_NIT		AF_NIT
#define	PF_802		AF_802
#define	PF_OSI		AF_OSI
#define	PF_ISO		PF_OSI
#define	PF_X25		AF_X25
#define	PF_OSINET	AF_OSINET
#define	PF_GOSIP	AF_GOSIP
#define PF_YNET         AF_YNET
#define PF_ROUTE        AF_ROUTE
#define PF_LINK         AF_LINK
#if !(defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1)
#define pseudo_PF_XTP   pseudo_AF_XTP
#endif
#define PF_INET6	AF_INET6
#define	PF_MAX		AF_MAX

/*
 * Maximum queue length specifiable by listen.
 */
#define	SOMAXCONN	5

/*
 * Message header for recvmsg and sendmsg calls.
 *
 * The X/Open Version 4.0 Specification includes different members in 
 * the msghdr structure than those defined in the original SVR4 structure. 
 * The new members are msg_control, msg_controllen and msg_flags.
 *
 * The msg_control field points to ancillary data. It consists of a 
 * sequence of pairs, each consisting of a cmsghdr structure followed by 
 * a data array. The data array may point to access rights. It is a 
 * replacement for the SVR4 msg_accrights member.
 *
 * The recvmsg and sendmsg calls have been versioned to use the 
 * correct members of the msghdr structure.
 *
 * By default, a program compiled with "sys/socket.h" will expect any 
 * ancillary data pointed to by msg_control to be in the X/Open format.
 * 
 * A program compiled with the SVR4 flag will expect any access rights
 * pointed to by msg_accrights to be in the SVR4 format.  
 *
 * Existing binaries, will use functions from the library which will be 
 * compatible with the SVR4 version of the msghdr structure.
 */ 

struct msghdr {
	void 	*msg_name;		/* optional address */
	size_t	msg_namelen;		/* size of address */
	struct	iovec *msg_iov;		/* scatter/gather array */
	int	msg_iovlen;		/* # elements in msg_iov */
	void 	*msg_control;		/* X/Open */
	size_t	msg_controllen;		/* X/Open */
	int	msg_flags;		/* X/Open */
};
#define msg_accrights		msg_control	/* map to X/Open member */
#define msg_accrightslen	msg_controllen	/* map to X/Open member */

/*
 * msg_control header
 */
struct cmsghdr {
	size_t	cmsg_len; 	/* data byte count, including hdr */
	int	cmsg_level;	/* originating protocol */
	int	cmsg_type;	/* protocol-specific type */
};

/* given pointer to struct cmsghdr, return pointer to data */
#define CMSG_DATA(cmsg)         ((unsigned char *)((cmsg) + 1))
                                                             

/* given pointer to struct cmsghdr, return pointer to next cmsghdr */
#define CMSG_NXTHDR(mhdr, cmsg)  \
	( ((unsigned char *)(cmsg) + (cmsg)->cmsg_len >= \
	   (unsigned char *)(mhdr)->msg_control + (mhdr)->msg_controllen) ? \
	  (struct cmsghdr *) NULL : \
	  (struct cmsghdr *) ((unsigned char *)(cmsg) + (cmsg)->cmsg_len) )

                                                                          
#define CMSG_FIRSTHDR(mhdr)     ((struct cmsghdr *)(mhdr)->msg_control)

#define SCM_RIGHTS	1
                                                                        
#define	MSG_OOB		0x1		/* process out-of-band data */
#define	MSG_PEEK	0x2		/* peek at incoming message */
#define	MSG_DONTROUTE	0x4		/* send without using routing tables */

#define MSG_CTRUNC	0x8
#define MSG_TRUNC	0x10
#define MSG_EOR		0x30
#define MSG_WAITALL	0x20
#define	MSG_MAXIOVLEN	16

#if !(defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1 )
/*
 * An option specification consists of an opthdr, followed by the value of
 * the option.  An options buffer contains one or more options.  The len
 * field of opthdr specifies the length of the option value in bytes.  This
 * length must be a multiple of sizeof(long) (use OPTLEN macro).
 */

struct opthdr {
	long            level;	/* protocol level affected */
	long            name;	/* option to modify */
	long            len;	/* length of option value */
};

#define OPTLEN(x) ((((x) + sizeof(long) - 1) / sizeof(long)) * sizeof(long))
#define OPTVAL(opt) ((void *)(opt + 1))

/*
 * the optdefault structure is used for internal tables of option default
 * values.
 */
struct optdefault {
	int             optname;/* the option */
	void           *val;	/* ptr to default value */
	int             len;	/* length of value */
};

/*
 * the opproc structure is used to build tables of options processing
 * functions for dooptions().
 */
struct opproc {
	int             level;	/* options level this function handles */
	int             (*func) ();	/* the function */
};

/*
 * This structure is used to encode pseudo system calls
 */
struct socksysreq {
	int             args[7];
};

/*
 * This structure is used for adding new protocols to the list supported by
 * sockets.
 */

struct socknewproto {
	int             family;	/* address family (AF_INET, etc.) */
	int             type;	/* protocol type (SOCK_STREAM, etc.) */
	int             proto;	/* per family proto number */
	dev_t           dev;	/* major/minor to use (must be a clone) */
	int             flags;	/* protosw flags */
};

/*
 * Definitions and structures for accessing the size and contents of
 * kernel tables (ie. pcb lists).
 */

#define GIARG	0x1
#define CONTI	0x2
#define GITAB	0x4

struct gi_arg {
	caddr_t		gi_where;
	unsigned int	gi_size;
};

#endif /* !(defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1 )*/


/* defines for user/kernel interface */

#if (INTEL == 31) || (ATT == 31)
#define SOCKETSYS	88	/* MUST BE CHANGED DEPENDING ON OS/SYSENT.C!! */
#else
#define SOCKETSYS	83	/* MUST BE CHANGED DEPENDING ON OS/SYSENT.C!! */
#endif

#define  SO_ACCEPT	1
#define  SO_BIND	2
#define  SO_CONNECT	3
#define  SO_GETPEERNAME	4
#define  SO_GETSOCKNAME	5
#define  SO_GETSOCKOPT	6
#define  SO_LISTEN	7
#define  SO_RECV	8
#define  SO_RECVFROM	9
#define  SO_SEND	10
#define  SO_SENDTO	11
#define  SO_SETSOCKOPT	12
#define  SO_SHUTDOWN	13
#define  SO_SOCKET	14
#define  SO_SOCKPOLL	15
#define  SO_GETIPDOMAIN	16
#define  SO_SETIPDOMAIN	17
#define  SO_ADJTIME	18

#if !(defined(_KERNEL) || defined(INKERNEL) || defined(_INKERNEL))

/*
 * The following interfaces are versioned due to incompatible changes
 * in struct sockaddr and struct msghdr.
 */
VERSION_DECLARE(__NETLIB_VERSION__) {
	extern int bindresvport(int, struct sockaddr *);
	extern int connect(int, const struct sockaddr *, size_t);
	extern int accept(int, struct sockaddr *, size_t *);
	extern int bind(int, const struct sockaddr *, size_t);
	extern int getpeername(int, struct sockaddr *, size_t *);
	extern int getsockname(int, struct sockaddr *, size_t *);
	extern ssize_t recvfrom(int, void *, size_t, int, struct sockaddr *,
				size_t *);
	extern ssize_t recvmsg(int, struct msghdr *, int);
	extern ssize_t sendto(int, const void *, size_t, int,
			      const struct sockaddr *, size_t);
	extern ssize_t sendmsg(int, const struct msghdr *, int);
	extern int setsockname(int, struct sockaddr *, size_t);
	extern int setpeername(int, struct sockaddr *, size_t);
}

#ifdef __STDC__

extern int getsockopt(int, int, int, void *, size_t *);
extern int listen(int, int);
extern ssize_t recv(int, void *, size_t, int);
extern ssize_t send(int, const void *, size_t, int);
extern int setsockopt(int, int, int, const void *, size_t);
extern int shutdown(int, int);
extern int socket(int, int, int);
extern int socketpair(int, int, int, int[2]);

#else /* !__STDC__ */

extern int getsockopt();
extern int listen();
extern int recv();
extern int send();
extern int setsockopt();
extern int shutdown();
extern int socket();
extern int socketpair();

#endif /* __STDC__ */

#endif /* !(defined(_KERNEL) || defined(INKERNEL) || defined(_INKERNEL)) */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_SOCKET_H */
