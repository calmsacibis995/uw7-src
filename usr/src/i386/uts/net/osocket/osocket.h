#ifndef _NET_OSOCKET_OSOCKET_H	/* wrapper symbol for kernel use */
#define _NET_OSOCKET_OSOCKET_H	/* subject to change without notice */

#ident	"@(#)osocket.h	1.2"
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

#ifdef _KERNEL_HEADERS

#ifndef _NET_SOCKMOD_H
#include <net/sockmod.h>	/* REQUIRED */
#endif

#ifndef _UTIL_TYPES_H
#include <util/types.h>			/* REQUIRED */
#endif

#elif defined(_KERNEL) || defined(_KMEMUSER)


#include <sys/sockmod.h>	/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/* Enhanced Application Compatibility Support */

/* These definations are here because some of them differ 
 * from SVR4.0 socket.h 
 */

/*
 * Definitions related to sockets: types, address families, options.
 */

/*
 * Types
 */
#define	OSOCK_STREAM	1	/* stream socket */
#define	OSOCK_DGRAM	2	/* datagram socket */
#define	OSOCK_RAW	3	/* raw-protocol interface */
#define	OSOCK_RDM	4	/* reliably-delivered message */
#define	OSOCK_SEQPACKET	5	/* sequenced packet stream */

/*
 * Option flags per-socket.
 */
#define	OSO_DEBUG	0x0001	/* turn on debugging info recording */
#define	OSO_ACCEPTCONN	0x0002	/* socket has had listen() */
#define	OSO_REUSEADDR	0x0004	/* allow local address reuse */
#define	OSO_KEEPALIVE	0x0008	/* keep connections alive */
#define	OSO_DONTROUTE	0x0010	/* just use interface addresses */
#define	OSO_BROADCAST	0x0020	/* permit sending of broadcast msgs */
#define	OSO_USELOOPBACK	0x0040	/* bypass hardware when possible */
#define	OSO_LINGER	0x0080	/* linger on close if data present */
#define	OSO_OOBINLINE	0x0100	/* leave received OOB data in line */
#define OSO_ORDREL	0x0200	/* give use orderly release */
#define OSO_IMASOCKET	0x0400	/* use socket semantics (affects bind) */

/*
 * Additional options, not kept in so_options.
 */
#define OSO_SNDBUF	0x1001	/* send buffer size */
#define OSO_RCVBUF	0x1002	/* receive buffer size */
#define OSO_SNDLOWAT	0x1003	/* send low-water mark */
#define OSO_RCVLOWAT	0x1004	/* receive low-water mark */
#define OSO_SNDTIMEO	0x1005	/* send timeout */
#define OSO_RCVTIMEO	0x1006	/* receive timeout */
#define	OSO_ERROR	0x1007	/* get error status and clear */
#define	OSO_TYPE		0x1008	/* get socket type */
#define OSO_PROTOTYPE	0x1009	/* get/set protocol type */

/*
 * Structure used for manipulating linger option.
 */
struct olinger {
	int             l_onoff;/* option on/off */
	int             l_linger;	/* linger time */
};

/*
 * Level number for (get/set)sockopt() to apply to socket itself.
 */
#define	OSOL_SOCKET	0xffff	/* options for socket level */

/*
 * An option specification consists of an opthdr, followed by the value of
 * the option.  An options buffer contains one or more options.  The len
 * field of opthdr specifies the length of the option value in bytes.  This
 * length must be a multiple of sizeof(long) (use OPTLEN macro).
 */

struct oopthdr {
	long            level;	/* protocol level affected */
	long            name;	/* option to modify */
	long            len;	/* length of option value */
};

#define OOPTLEN(x) ((((x) + sizeof(long) - 1) / sizeof(long)) * sizeof(long))
#define OOPTVAL(opt) ((char *)(opt + 1))

/*
 * the ooptdefault structure is used for internal tables of option default
 * values.
 */
struct ooptdefault {
	int             optname;/* the option */
	char           *val;	/* ptr to default value */
	int             len;	/* length of value */
};

/*
 * the oopproc structure is used to build tables of options processing
 * functions for dooptions().
 */
struct oopproc {
	int             level;	/* options level this function handles */
	int             (*func) ();	/* the function */
};

/*
 * Address families.
 */
#define	OAF_UNSPEC	0	/* unspecified */
#define	OAF_UNIX		1	/* local to host (pipes, portals) */
#define	OAF_INET		2	/* internetwork: UDP, TCP, etc. */
#define	OAF_IMPLINK	3	/* arpanet imp addresses */
#define	OAF_PUP		4	/* pup protocols: e.g. BSP */
#define	OAF_CHAOS	5	/* mit CHAOS protocols */
#define	OAF_NS		6	/* XEROX NS protocols */
#define	OAF_NBS		7	/* nbs protocols */
#define	OAF_ECMA		8	/* european computer manufacturers */
#define	OAF_DATAKIT	9	/* datakit protocols */
#define	OAF_CCITT	10	/* CCITT protocols, X.25 etc */
#define	OAF_SNA		11	/* IBM SNA */
#define OAF_DECnet	12	/* DECnet */
#define OAF_DLI		13	/* Direct data link interface */
#define OAF_LAT		14	/* LAT */
#define	OAF_HYLINK	15	/* NSC Hyperchannel */
#define	OAF_APPLETALK	16	/* Apple Talk */

#define	OAF_MAX		17

/*
 * Structure used by kernel to store most addresses.
 */
struct osockaddr {
        union {
            struct {
                u_short sa_saus_family;   /* address family */
                char    sa_saus_data[14]; /* up to 14 bytes data */
            } sa_s;
            unsigned long _align;       /* to restrict alignment */
        } sa_un;
};

/*
 * Structure used by kernel to pass protocol information in raw sockets.
 */
struct osockproto {
	unsigned short  sp_family;	/* address family */
	unsigned short  sp_protocol;	/* protocol */
};

/*
 * Protocol families, same as address families for now.
 */
#define	OPF_UNSPEC	OAF_UNSPEC
#define	OPF_UNIX	OAF_UNIX
#define	OPF_INET	OAF_INET
#define	OPF_IMPLINK	OAF_IMPLINK
#define	OPF_PUP		OAF_PUP
#define	OPF_CHAOS	OAF_CHAOS
#define	OPF_NS		OAF_NS
#define	OPF_NBS		OAF_NBS
#define	OPF_ECMA	OAF_ECMA
#define	OPF_DATAKIT	OAF_DATAKIT
#define	OPF_CCITT	OAF_CCITT
#define	OPF_SNA		OAF_SNA

#define	OPF_MAX		12


#define	OMSG_OOB	0x1	/* process out-of-band data */
#define	OMSG_PEEK	0x2	/* peek at incoming message */
#define	OMSG_DONTROUTE	0x4	/* send without using routing tables */

#define	OMSG_MAXIOVLEN	16

/*
 * This ioctl code uses BSD style ioctl's to avoid copyin/out problems.
 * Ioctl's have the command encoded in the lower word, and the size of any in
 * or out parameters in the upper word.  The high 2 bits of the upper word
 * are used to encode the in/out status of the parameter; for now we restrict
 * parameters to at most 128 bytes.
 */
#define	OIOCPARM_MASK	0x7f	/* parameters must be < 128 bytes */
#define	OIOC_VOID	0x20000000	/* no parameters */
#define	OIOC_OUT		0x40000000	/* copy out parameters */
#define	OIOC_IN		0x80000000	/* copy in parameters */
#define	OIOC_INOUT	(OIOC_IN|OIOC_OUT)
/* the 0x20000000 is so we can distinguish new ioctl's from old */
#define	_OIOS(x,y)	(OIOC_VOID|(x<<8)|y)
#define	_OIOSR(x,y,t)	(OIOC_OUT|((sizeof(t)&OIOCPARM_MASK)<<16)|(x<<8)|y)
#define	_OIOSW(x,y,t)	(OIOC_IN|((sizeof(t)&OIOCPARM_MASK)<<16)|(x<<8)|y)
/* this should be _OIOSRW, but stdio got there first */
#define	_OIOSWR(x,y,t)	(OIOC_INOUT|((sizeof(t)&OIOCPARM_MASK)<<16)|(x<<8)|y)

/*
 * Socket ioctl commands
 * 
 */

#define OSIOCATMARK	_OIOSR('S', 5, int)	/* at oob mark? */
#define OSIOCSPGRP	_OIOSW('S', 6, int)	/* set process group */
#define OSIOCGPGRP	_OIOSR('S', 7, int)	/* get process group */
#define OFIONREAD	_OIOSR('S', 8, int)	/* BSD compatibilty */
#define OFIONBIO	_OIOSW('S', 9, int)	/* BSD compatibilty */
#define OFIOASYNC	_OIOSW('S', 10, int)	/* BSD compatibilty */
#define OSIOCPROTO	_OIOSW('S', 11, struct osocknewproto)	/* link proto */
#define OSIOCXPROTO	_OIOS('S', 15)	/* empty proto table */
#define OSIOCGIFFLAGS	_OIOSWR('I', 16, struct oifreq )/* get ifnet flags */
#define OSIOCGIFCONF	_OIOSWR('I', 17, struct oifconf )/* get ifnet list */
#define OSIOCSOCKSYS	_OIOSW('I', 66, struct osocksysreq) /* socket calls */

/*
 * Struct used for one-packet mode params in if ioctls
 */
struct oonepacket {
	int	spsize;		/* short packet size */
	int	spthresh;	/* short packet threshold */
};

/*
 * Interface request structure used for socket ioctl's.  All interface
 * ioctl's must have parameter definitions which begin with ifr_name.  The
 * remainder may be interface specific. 
 */
struct oifreq {
#define	IFNAMSIZ	16
	char            ifr_name[IFNAMSIZ];	/* if name, e.g. "en0" */
	union {
		struct osockaddr ifru_addr;
		struct osockaddr ifru_dstaddr;
		struct osockaddr ifru_broadaddr;
		short           ifru_flags;
		int             ifru_metric;
		caddr_t         ifru_data;
		char            ifru_enaddr[6];
		struct oonepacket ifru_onepacket;
	}               ifr_ifru;
#define	oifr_addr	ifr_ifru.ifru_addr	/* address */
#define	oifr_dstaddr	ifr_ifru.ifru_dstaddr	/* other end of p-to-p link */
#define	oifr_broadaddr	ifr_ifru.ifru_broadaddr	/* broadcast address */
#define	oifr_flags	ifr_ifru.ifru_flags	/* flags */
#define	oifr_metric	ifr_ifru.ifru_metric	/* metric */
#define	oifr_data	ifr_ifru.ifru_data	/* for use by interface */
#define oifr_enaddr	ifr_ifru.ifru_enaddr	/* ethernet address */
#define oifr_onepacket	ifr_ifru.ifru_onepacket	/* one-packet mode params */
};

/*
 * Structure used in SIOCGIFCONF request. Used to retrieve interface
 * configuration for machine (useful for programs which must know all
 * networks accessible). 
 */
struct oifconf {
	int             ifc_len;/* size of associated buffer */
	union {
		caddr_t         ifcu_buf;
		struct oifreq   *ifcu_req;
	}               ifc_ifcu;
#define	oifc_buf	ifc_ifcu.ifcu_buf	/* buffer address */
#define	oifc_req	ifc_ifcu.ifcu_req	/* array of structures returned */
};

/*
 * This structure is used to encode pseudo system calls
 */
struct osocksysreq {
	int             args[7];
};

/*
 * This structure is used for adding new protocols to the list supported by
 * sockets.
 */

struct osocknewproto {
	int             family;	/* address family (AF_INET, etc.) */
	int             type;	/* protocol type (SOCK_STREAM, etc.) */
	int             proto;	/* per family proto number */
	dev_t           dev;	/* major/minor to use (must be a clone) */
	int             flags;	/* protosw flags */
};



#define	 OMAXHOSTNAMELEN	64

#define  OSO_ACCEPT	1
#define  OSO_BIND	2
#define  OSO_CONNECT	3
#define  OSO_GETPEERNAME	4
#define  OSO_GETSOCKNAME	5
#define  OSO_GETSOCKOPT	6
#define  OSO_LISTEN	7
#define  OSO_RECV	8
#define  OSO_RECVFROM	9
#define  OSO_SEND	10
#define  OSO_SENDTO	11
#define  OSO_SETSOCKOPT	12
#define  OSO_SHUTDOWN	13
#define  OSO_SOCKET	14
#define  OSO_SELECT	15
#define  OSO_GETIPDOMAIN	16
#define  OSO_SETIPDOMAIN	17
#define  OSO_ADJTIME	18
#define  OSO_SETREUID	19
#define  OSO_SETREGID	20
#define  OSO_GETTIME	21


/* Domain defines */
/*
 * Structure per communications domain. 
 */
struct odomain {
	int             dom_family;	/* AF_xxx */
	struct oprotosw *dom_protosw;
	struct odomain  *dom_next;
};


/*
 * Protocol switch table. 
 */
struct oprotosw {
	short           pr_type;/* socket type used for */
	struct odomain  *pr_domain;	/* domain protocol a member of */
	short           pr_protocol;	/* protocol number */
	short           pr_flags;	/* see below */
	dev_t           pr_device;	/* device to use for this proto */
	struct oprotosw *pr_next;/* next link in chain for this family */
};

/*
 * Note that in BSD these values are enforced via timeouts called through
 * protosw, in the Convergent implementation they are more on the order of
 * suggestions that drivers can use. 
 */

#define	OPR_SLOWHZ	2	/* 2 slow timeouts per second */
#define	OPR_FASTHZ	5	/* 5 fast timeouts per second */

/*
 * Values for pr_flags 
 */
#define	OPR_ATOMIC	0x01	/* exchange atomic messages only */
#define	OPR_ADDR		0x02	/* addresses given with messages */
/* in the current implementation, OPR_ADDR needs PR_ATOMIC to work */
#define	OPR_CONNREQUIRED	0x04	/* connection required by protocol */
/* note that OPR_WANTRCVD is ignored in the streams implementation */
#define	OPR_WANTRCVD	0x08	/* want PRU_RCVD calls */
#define	OPR_RIGHTS	0x10	/* passes capabilities */
#define OPR_BINDPROTO	0x20	/* pass protocol */

/*
 * These values are currently used by the network and transport layers of the
 * IP streams implementation, but since they were already here in the BSD
 * implementation, why break them? 
 *
 * N.B. The IMP code, in particular, pressumes the values of some of the
 * commands; change with extreme care. TODO (from BSD): spread out codes so
 * new ICMP codes can be accomodated more easily 
 */
#define	OPRC_IFDOWN		0	/* interface transition */
#define	OPRC_ROUTEDEAD		1	/* select new route if possible */
#define	OPRC_QUENCH		4	/* some said to slow down */
#define	OPRC_MSGSIZE		5	/* message size forced drop */
#define	OPRC_HOSTDEAD		6	/* normally from IMP */
#define	OPRC_HOSTUNREACH		7	/* ditto */
#define	OPRC_UNREACH_NET		8	/* no route to network */
#define	OPRC_UNREACH_HOST	9	/* no route to host */
#define	OPRC_UNREACH_PROTOCOL	10	/* dst says bad protocol */
#define	OPRC_UNREACH_PORT	11	/* bad port # */
#define	OPRC_UNREACH_NEEDFRAG	12	/* IP_DF caused drop */
#define	OPRC_UNREACH_SRCFAIL	13	/* source route failed */
#define	OPRC_REDIRECT_NET	14	/* net routing redirect */
#define	OPRC_REDIRECT_HOST	15	/* host routing redirect */
#define	OPRC_REDIRECT_TOSNET	16	/* redirect for type of service & net */
#define	OPRC_REDIRECT_TOSHOST	17	/* redirect for tos & host */
#define	OPRC_TIMXCEED_INTRANS	18	/* packet lifetime expired in transit */
#define	OPRC_TIMXCEED_REASS	19	/* lifetime expired on reass q */
#define	OPRC_PARAMPROB		20	/* header incorrect */

#define	OPRC_NCMDS		21


/* SOCKET VARIBLES */
/*
 * Kernel structure per socket. 
 */
struct osocket {
	short           so_type;	/* generic type, see osocket.h */
	short           so_options;	/* from osocket call, see osocket.h */
	short           so_linger;	/* time to linger while closing */
	short           so_state;	/* internal state flags SS_*, below */
	struct oprotosw so_proto;	/* protocol handle */

	int		so_addrlen;	/* address length of tp */
	long		so_flags;	/* flags */

	int		so_fd;		/* file descriptor */
	struct file    *so_fp;		/* file pointer for NDELAY */
	struct vnode   *so_uvp;		/* user's vnode pointer */
	int		so_sfd;		/* Sockmod file descriptor */
	struct file    *so_sfp;		/* file ptr for SVR4sock*/
	struct vnode   *so_svp;		/* system's vnode ptr for SVR4sock*/
	struct _si_user  so_user;	/* Sockmod udata */
	struct osockaddr so_addr;	/* Space to copy in user's address */
};

/*
 * Socket state bits. 
 */
#define	OSS_NOFDREF		0x001	/* no file table ref any more */
#define	OSS_ISCONNECTED		0x002	/* osocket connected to a peer */
#define	OSS_ISCONNECTING		0x004	/* in process of connecting to peer */
#define	OSS_ISDISCONNECTING	0x008	/* in process of disconnecting */
#define	OSS_CANTSENDMORE		0x010	/* can't send more data to peer */
#define	OSS_CANTRCVMORE		0x020	/* can't receive more data from peer */
#define	OSS_RCVATMARK		0x040	/* at mark on input */

#define	OSS_PRIV			0x080	/* privileged for broadcast, raw... */
#define	OSS_NBIO			0x100	/* non-blocking ops */
#define	OSS_ASYNC		0x200	/* async i/o notify */
#define OSS_IOCWAIT		0x400	/* ioctl already downstream */
#define OSS_READWAIT		0x800	/* send a wakeup on read */
#define OSS_WRITEWAIT		0x1000	/* send a wakeup on write enab */
#define OSS_BOUND		0x2000	/* osocket has been bound */
#define OSS_HIPRI		0x4000	/* M_PCPROTO on osock stream head */

#define OSF_RLOCK		0x0001	/* receiver is locked */
#define OSF_WLOCK                0x0002  /* sender is locked */
#define OSF_LOCKWAIT             0x0004  /* someone is waiting to lock */
#define OSF_RCOLL                0x0008  /* collision selecting (read) */
#define OSF_WCOLL                0x0010  /* collision selecting (write) */

#define OSOCK_AVAIL	((struct osocket *)0)
#define OSOCK_RESERVE	((struct osocket *)-1)
#define OSOCK_INPROGRESS ((struct osocket *)-2)

/* Enhanced Application Compatibility Support */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_OSOCKET_OSOCKET_H */
