#ifndef _NET_ISOCKET_ISOCKET_H	/* wrapper symbol for kernel use */
#define _NET_ISOCKET_ISOCKET_H	/* subject to change without notice */

#ident	"@(#)isocket.h	1.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/stream.h>		/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/stream.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Copyright (c) 1982,1985, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1988 INTERACTIVE Systems Corporation
 *	All rights reserved.
 *
 *	RESTRICTED RIGHTS:
 *
 *	These programs are supplied under a license.  They may be used,
 *	disclosed, and/or copied only as permitted under such license
 *	agreement.  Any copy must contain the above copyright notice and
 *	this restricted rights notice.  Use, copying, and/or disclosure
 *	of the programs is strictly prohibited unless otherwise provided
 *	in the license agreement.
 *
 */


/*
 * Definitions related to isockets: types, address families, options.
 */

/*
 * Types
 */
#define	ISC_SOCK_STREAM	1		/* stream isocket */
#define	ISC_SOCK_DGRAM	2		/* datagram isocket */
#define	ISC_SOCK_RAW	3		/* raw-protocol interface */
#define	ISC_SOCK_RDM	4		/* reliably-delivered message */
#define	ISC_SOCK_SEQPACKET	5	/* sequenced packet stream */

/*
 * Option flags per-isocket.
 */
#define	ISC_SO_DEBUG	0x0001		/* turn on debugging info recording */
#define	ISC_SO_ACCEPTCONN	0x0002		/* isocket has had listen() */
#define	ISC_SO_REUSEADDR	0x0004		/* allow local address reuse */
#define	ISC_SO_KEEPALIVE	0x0008		/* keep connections alive */
#define	ISC_SO_DONTROUTE	0x0010		/* just use interface addresses */
#define	ISC_SO_BROADCAST	0x0020		/* permit sending of broadcast msgs */
#define	ISC_SO_USELOOPBACK	0x0040		/* bypass hardware when possible */
#define	ISC_SO_LINGER	0x0080		/* linger on close if data present */
#define	ISC_SO_OOBINLINE	0x0100		/* leave received OOB data in line */

/* options which will affect lower layers */
#define ISC_SO_IPOPTS	(ISC_DONTROUTE|ISC_BROADCAST|ISC_USELOOPBACK)
/*
 * Additional options, not kept in so_options.
 */
#define ISC_SO_SNDBUF	0x1001		/* send buffer size */
#define ISC_SO_RCVBUF	0x1002		/* receive buffer size */
#define ISC_SO_SNDLOWAT	0x1003		/* send low-water mark */
#define ISC_SO_RCVLOWAT	0x1004		/* receive low-water mark */
#define ISC_SO_SNDTIMEO	0x1005		/* send timeout */
#define ISC_SO_RCVTIMEO	0x1006		/* receive timeout */
#define	ISC_SO_ERROR	0x1007		/* get error status and clear */
#define	ISC_SO_TYPE		0x1008		/* get isocket type */

/*
 * Structure used for manipulating linger option.
 */
struct isc_linger {
	int	l_onoff;		/* option on/off */
	int	l_linger;		/* linger time */
};

/*
 * Level number for (get/set)isockopt() to apply to isocket itself.
 */
#define	ISC_SOL_SOCKET	0xffff		/* options for isocket level */

/*
 * Address families.
 */
#define	ISC_AF_UNSPEC	0		/* unspecified */
#define	ISC_AF_UNIX		1		/* local to host (pipes, portals) */
#define	ISC_AF_INET		2		/* internetwork: UDP, TCP, etc. */
#define	ISC_AF_IMPLINK	3		/* arpanet imp addresses */
#define	ISC_AF_PUP		4		/* pup protocols: e.g. BSP */
#define	ISC_AF_CHAOS	5		/* mit CHAOS protocols */
#define	ISC_AF_NS		6		/* XEROX NS protocols */
#define	ISC_AF_NBS		7		/* nbs protocols */
#define	ISC_AF_ECMA		8		/* european computer manufacturers */
#define	ISC_AF_DATAKIT	9		/* datakit protocols */
#define	ISC_AF_CCITT	10		/* CCITT protocols, X.25 etc */
#define	ISC_AF_SNA		11		/* IBM SNA */
#define ISC_AF_DECnet	12		/* DECnet */
#define ISC_AF_DLI		13		/* Direct data link interface */
#define ISC_AF_LAT		14		/* LAT */
#define	ISC_AF_HYLINK	15		/* NSC Hyperchannel */
#define	ISC_AF_APPLETALK	16		/* Apple Talk */

#define	ISC_AF_MAX		17


/*
 * Structure used by kernel to store most
 * addresses.
 */
struct isockaddr {
        union {
            struct {
                ushort_t	sa_saus_family;   /* address family */
                char     	sa_saus_data[14]; /* up to 14 bytes data */
            } sa_s;
            ulong_t _align;       /* to restrict alignment */
        } sa_un;
};

/*
 * Structure used by kernel to pass protocol
 * information in raw isockets.
 */
struct isockproto {
	ushort_t	sp_family;		/* address family */
	ushort_t	sp_protocol;		/* protocol */
};

/*
 * Protocol families, same as address families for now.
 */
#define	ISC_PF_UNSPEC	ISC_AF_UNSPEC
#define	ISC_PF_UNIX		ISC_AF_UNIX
#define	ISC_PF_INET		ISC_AF_INET
#define	ISC_PF_IMPLINK	ISC_AF_IMPLINK
#define	ISC_PF_PUP		ISC_AF_PUP
#define	ISC_PF_CHAOS	ISC_AF_CHAOS
#define	ISC_PF_NS		ISC_AF_NS
#define	ISC_PF_NBS		ISC_AF_NBS
#define	ISC_PF_ECMA		ISC_AF_ECMA
#define	ISC_PF_DATAKIT	ISC_AF_DATAKIT
#define	ISC_PF_CCITT	ISC_AF_CCITT
#define	ISC_PF_SNA		ISC_AF_SNA
#define ISC_PF_DECnet	ISC_AF_DECnet
#define ISC_PF_DLI		ISC_AF_DLI
#define ISC_PF_LAT		ISC_AF_LAT
#define	ISC_PF_HYLINK	ISC_AF_HYLINK
#define	ISC_PF_APPLETALK	ISC_AF_APPLETALK

#define	ISC_PF_MAX		ISC_AF_MAX

/*
 * Maximum queue length specifiable by listen.
 */
#define	ISC_SOMAXCONN	5

/*
 * Message header for recvmsg and sendmsg calls.
 */
struct isc_msghdr {
	caddr_t	msg_name;		/* optional address */
	int	msg_namelen;		/* size of address */
	struct iovec *msg_iov;		/* scatter/gather array */
	int	msg_iovlen;		/* # elements in msg_iov */
	caddr_t	msg_accrights;		/* access rights sent/received */
	int	msg_accrightslen;
};

#define	ISC_MSG_OOB		0x1	/* process out-of-band data */
#define	ISC_MSG_PEEK		0x2	/* peek at incoming message */
#define	ISC_MSG_DONTROUTE	0x4	/* send without using routing tables */

#define	ISC_MSG_MAXIOVLEN	16

/* 
 * The following is taken from SVR4's sys/isocket.h
 */

/*
 * An option specification consists of an opthdr, followed by the value of
 * the option.  An options buffer contains one or more options.  The len
 * field of opthdr specifies the length of the option value in bytes.  This
 * length must be a multiple of sizeof(long) (use OPTLEN macro).
 */

struct isc_opthdr {
	long	level;	/* protocol level affected */
	long	name;	/* option to modify */
	long	len;	/* length of option value */
};

#define ISC_OPTLEN(x) ((((x) + sizeof(long) - 1) / sizeof(long)) * sizeof(long))
#define ISC_OPTVAL(opt) ((char *)(opt + 1))


/*
 *	definitions for isocket to TLI conversion streams module
 *
 */


#define MAXSOCSZ INFPSZ
#define SOCHIWAT 4096
#define SOCLOWAT 2048


struct isocdev {
	queue_t *isoc_qptr;		/* ptr to write queue */
	ushort_t isoc_state;		/* state of isocket module */
	ushort_t isoc_domain;		/* network family (domain) name */
	ushort_t isoc_type;		/* network type */
	ushort_t isoc_proto;		/* network protocol */
	struct isockaddr isoc_saddr;	/* local network address */
	struct isockaddr isoc_daddr;	/* remote network address */
	ushort_t isoc_slen;		/* local network length */
	ushort_t isoc_dlen;		/* remote network length */
	ushort_t isoc_pend;	/* number of pending connections allowed */
	ushort_t isoc_backlog;	/* number of pending connections requested */
	ushort_t isoc_npend;		/* length of isoc_pendlist queue */
	mblk_t * isoc_pendlist;		/* queue of pending connections */
	struct isocdev *isoc_spend;	/* ptr to queue pending accept */
	mblk_t * isoc_template;	/* ptr to template of tli data message */
	mblk_t * isoc_opt;	/* ptr to queue of isocket options set */
	long   isoc_options;		/* isocket option bits */
	long   isoc_sequence;	/* sequence number of current connection */
	uint_t   isoc_ioctl;		/* id of ioctl pending */
	int    isoc_ioctlcmd;		/* cmd of ioctl pending */
	ushort_t isoc_prisig;		/* priority data signal to send */
	ushort_t isoc_flags;		/* isocket flags */
	struct isocdev *isoc_head;/* pointer back to the listening isocdev */

	/* SVR4 data-structures */

	struct si_udata *isoc_udata;	/* user data from sockmod */
	int    isoc_roundup;		/* # of bytes for alignment copy */
	int    isoc_smodcmd;		/* Pending Sockmod Cmd */
	int    isoc_cmd;		/* Pending socket cmd */
};

/* flags */
#define SOCF_IOCTLWAT	1	/* waiting for ioctl response */
#define SOCF_BINDWAT	2	/* waiting for bind response */
#define SOCF_DRAIN	4	/* wait for data to drain */
#define SOCF_DONTROUTE	8	
/* wait for locally generated command to clear the SO_DONTROUTE option */ 


/*
 *  ISC socket ioctl definitions
 */

/* socket i/o controls */

#define sIOC	('s'<<8)
#define IOCTL_TYPE(x)	((x>>8)&0xFF)
#define _IOCTL(x,n)	((x<<8)|n)
#define	ISC_SIOCSHIWAT	_IOCTL('s',  0) /* set high watermark */
#define	ISC_SIOCGHIWAT	_IOCTL('s',  1) /* get high watermark */
#define	ISC_SIOCSLOWAT	_IOCTL('s',  2) /* set low watermark */
#define	ISC_SIOCGLOWAT	_IOCTL('s',  3) /* get low watermark */
#define	ISC_SIOCATMARK	_IOCTL('s',  7) /* at oob mark? */
#define	ISC_SIOCSPGRP	_IOCTL('s',  8) /* set process group */
#define	ISC_SIOCGPGRP	_IOCTL('s',  9) /* get process group */
#define	ISC_SIOCSHUTDOWN	_IOCTL('s', 10) /* perform a shutdown */
#define	ISC_SIOCOOBSIG	_IOCTL('s', 40) /* set signal to send on oob */

#define rIOC	('r'<<8)
#define	ISC_SIOCADDRT	_IOCTL('r', 10) /* add route */
#define	ISC_SIOCDELRT	_IOCTL('r', 11) /* delete route */

#define iIOC	('i'<<8)
#define	ISC_SIOCSIFADDR	_IOCTL('i', 12) /* set ifnet address */
#define	ISC_SIOCGIFADDR	_IOCTL('i', 13) /* get ifnet address */
#define	ISC_SIOCSIFDSTADDR	_IOCTL('i', 14) /* set p-p address */
#define	ISC_SIOCGIFDSTADDR	_IOCTL('i', 15) /* get p-p address */
#define	ISC_SIOCSIFFLAGS	_IOCTL('i', 16) /* set ifnet flags */
#define	ISC_SIOCGIFFLAGS	_IOCTL('i', 17) /* get ifnet flags */
#define	ISC_SIOCGIFBRDADDR	_IOCTL('i', 18) /* get broadcast addr */
#define	ISC_SIOCSIFBRDADDR	_IOCTL('i', 19) /* set broadcast addr */
#define	ISC_SIOCGIFCONF	_IOCTL('i', 20) /* get ifnet list */
#define	ISC_SIOCGIFNETMASK	_IOCTL('i', 21) /* get net addr mask */
#define	ISC_SIOCSIFNETMASK	_IOCTL('i', 22) /* set net addr mask */
#define	ISC_SIOCGIFMETRIC	_IOCTL('i', 23) /* get IF metric */
#define	ISC_SIOCSIFMETRIC	_IOCTL('i', 24) /* set IF metric */

#define ISC_SIOCSIFNAME	_IOCTL('i', 25) /* set IF name - find with lindex */
#define ISC_SIOCGHOSTID	_IOCTL('i', 26) /* get host id */
#define ISC_SIOCSHOSTID	_IOCTL('i', 27) /* set host id */
#define ISC_SIOCGSTAT	_IOCTL('i', 28) /* get IF statistics */

#define	ISC_SIOCSARP	_IOCTL('i', 30) /* set arp entry */
#define	ISC_SIOCGARP	_IOCTL('i', 31) /* get arp entry */
#define	ISC_SIOCDARP	_IOCTL('i', 32) /* delete arp entry */
#define ISC_SIOCGTIM        _IOCTL('i', 33) /* get arp timeout parameters */
#define ISC_SIOCSTIM        _IOCTL('i', 34) /* set arp timeout parameters */
#define ISC_SIOCGENT        _IOCTL('i', 35) /* get an arp indexed table entry */

#define ISC_SIOCSIPCONF	_IOCTL('i', 36) /* set IP configuration options */
#define ISC_SIOCGIPCONF	_IOCTL('i', 37) /* get IP configuration options */
#define		IPCONF_FORWARDING	1 /* IP forwarding option */
#define 	IPCONF_SENDREDIRECTS	2 /* send redirects on misroute */
#define		IPCONF_GATEWAY		3 /* gateway action */

#if defined(SNMP)
#define ISC_SIOCGPHYSADDR	_IOCTL('i', 38) /* get IF physical address */
#endif	/* SNMP */


#define mIOC	('m'<<8)
#define	ISC_SIOCSCKSUM	_IOCTL('m', 38) /* set check sum option */
#define ISC_SIOCSETOPT	_IOCTL('m', 39) /* set options - bypasses TLI restriction */
#define ISC_SIOCGETOPT	_IOCTL('m', 40) /* get current options */
#define ISC_SIOCGETNAME	_IOCTL('m', 41) /* get socket name */


/*
 * Definitions for the Socket interface protocol.
 *
 */

/*
 * The following are the definitions of the Socket-TLI Interface
 * primitives.
 */

/*
 * Primitives that are initiated by the socket library.
 */

#define ISC_SO_SOCKET	0	/* socket request */
#define ISC_SO_BIND	1	/* bind request */
#define ISC_SO_CONNECT	2	/* connect request */
#define ISC_SO_LISTEN	3	/* listen request */
#define ISC_SO_ACCEPT	4	/* accept request */
#define ISC_SO_FDINSERT	5	/* fdinsert request */
#define ISC_SO_SEND	6	/* send request */
#define ISC_SO_SETOPT	7	/* setopt request */
#define ISC_SO_GETOPT	8	/* getopt request */
#define ISC_SO_GETNAME	9	/* getname request */
#define ISC_SO_GETPEER	10	/* getpeer request */
#define ISC_SO_SHUTDOWN	11	/* shutdown request */
#define ISC_SO_DISC_REQ	19	/* disconnect request */

/*
 * Primitives that are initiated by the socket module.
 */

#define ISC_SO_ACCEPT_ACK	12	/* accept acknowledgement */
#define ISC_SO_UNITDATA_IND	13	/* unitdata indication */
#define ISC_SO_GETOPT_ACK	14	/* getopt acknowledgement */
#define ISC_SO_GETNAME_ACK	15	/* getname acknowledgement */
#define ISC_SO_GETPEER_ACK	16	/* getpeer acknowledgement */
#define ISC_SO_ERROR_ACK	17	/* error acknowledgement */
#define ISC_SO_OK_ACK		18	/* ok acknowledgement */

#define ISC_SO_GETUDATA 	20	/* ok acknowledgement */


/*
 * Primitive non-fatal error return codes
 */
#define ISC_SO_OUTSTATE	1
#define ISC_SO_SYSERR	2


/*
 * The following are the events that drive the state machine.
 *
 * Initialization events */
#define ISC_SE_SOCKET_REQ	0	/* socket request */
#define ISC_SE_BIND_REQ		1	/* bind request */
#define ISC_SE_LISTEN_REQ	2	/* listen request */
#define ISC_SE_SETOPT_REQ	3	/* setopt request */
#define ISC_SE_GETOPT_REQ	4	/* getopt request */
#define ISC_SE_GETNAME_REQ	5	/* getname request */
#define ISC_SE_BIND_ACK		6	/* bind ack */
#define ISC_SE_GETOPT_ACK	7	/* getopt ack */
#define ISC_SE_GETNAME_ACK	8	/* getname ack */
#define ISC_SE_ERROR_ACK	9	/* error acknowledgement */
#define ISC_SE_OK_ACK		10	/* ok acknowledgement */

/* Connection oriented events */
#define ISC_SE_ACCEPT_REQ	11	/* accept request */
#define ISC_SE_ACCEPT_ACK	12	/* accept ack */
#define ISC_SE_CONN_REQ		13	/* connect request */
#define ISC_SE_GETPEER_REQ	14	/* getpeer request */
#define ISC_SE_GETPEER_ACK	15	/* getpeer ack */
#define ISC_SE_DATA_REQ		16	/* write request */
#define ISC_SE_DATA_IND		17	/* data indication */
#define ISC_SE_SHUTDOWN_REQ	18	/* shutdown request */

/* Unit data events */
#define ISC_SE_SEND_REQ		19	/* send request */
#define ISC_SE_UNITDATA_IND	20	/* unitdata indication */
#define ISC_SE_UDERROR_IND	21	/* unitdata error indication */

#define ISC_SE_NOEVENTS		22


/*
 * The following are the possible states of the Socket Interface.
 */

#define ISC_SS_OPENED		0	/* just opened; no info */
#define ISC_SS_UNBND		1	/* unbound */
#define ISC_SS_WACK_BREQ	2	/* waiting ack of T_BIND_REQ */
#define ISC_SS_BOUND		3	/* bound */
#define ISC_SS_PASSIVE		4	/* listening but not accepting */
#define ISC_SS_ACCEPTING	5	/* accepting */
#define ISC_SS_WAIT_FD		6	/* waiting for stream pointer */
#define ISC_SS_WACK_CREQ	7	/* waiting ack of T_CONN_REQ */
#define ISC_SS_WCON_CREQ	8	/* waiting confirmation of T_CONN_REQ */
#define ISC_SS_WACK_CRES	9	/* waiting ack of T_CONN_RES */
#define ISC_SS_CONNECTED	10	/* connected */
#define ISC_SS_CONN_RONLY	11	/* connected with write shutdown */
#define ISC_SS_CONN_WONLY	12	/* connected with read shutdown */
#define ISC_SS_SHUTDOWN		13	/* connected but shutdown */
#define ISC_SS_LINGER		14	/* closing with outstanding data */

#define ISC_SS_NOSTATES		15


/* 
 * The following structure definitions define the format of the
 * stream message block of the above primitives.
 * (everything is declared long to ensure proper alignment
 *  across different machines)
 */

/* socket request */

struct SO_socket {
	long	PRIM_type;	/* always SO_SOCKET */
	long	SOCK_domain;	/* socket domain */
	long	SOCK_type;	/* socket type */
	long	SOCK_prot;	/* socket protocol */
};
#define SO_SOCKET_SIZE	sizeof (struct SO_socket)

/* bind request */

struct SO_bind {
	long	PRIM_type;	/* always SO_BIND */
	long	ADDR_length;	/* addr length */
	long	ADDR_offset;	/* addr offset */
	long	CONN_queue;	/* connect indications requested */
};
#define SO_BIND_SIZE	sizeof (struct SO_bind)

/* listen request */

struct SO_listen {
	long	PRIM_type;	/* always SO_LISTEN */
	long	CONN_queue;	/* connect indications requested */
};
#define SO_LISTEN_SIZE	sizeof (struct SO_listen)

/* accept request */

struct SO_accept {
	long	PRIM_type;	/* always SO_ACCEPT */
};
#define SO_ACCEPT_SIZE	sizeof (struct SO_accept)

/* fdinsert request */

struct SO_fdinsert {
	long	PRIM_type;	/* always SO_FDINSERT */
	long	SEQ_number;	/* sequence number from T_conn_ind */
	struct queue *STR_ptr;	/* pointer to read queue of driver */
};
#define SO_FDINSERT_SIZE	sizeof (struct SO_fdinsert)

/* connection request */

struct SO_connect {
	long	PRIM_type;	/* always SO_CONNECT */
	long	DEST_length;	/* dest addr length */
	long	DEST_offset;	/* dest addr offset */
};
#define SO_CONNECT_SIZE	sizeof (struct SO_connect)

/* send request */

struct SO_send {
	long	PRIM_type;	/* always SO_SEND */
	long	DEST_length;	/* dest addr length */
	long	DEST_offset;	/* dest addr offset */
	long	DATA_flags;	/* data flags */
};
#define SO_SEND_SIZE	sizeof (struct SO_send)

/* setopt request */

struct SO_setopt {
	long	PRIM_type;	/* always SO_SETOPT */
	long	PROT_level;	/* protocol level */
	long	OPT_name;	/* option name */
	long	OPT_length;	/* options length */
	long	OPT_offset;	/* options offset */
};
#define SO_SETOPT_SIZE	sizeof (struct SO_setopt)

/* getopt request */

struct SO_getopt {
	long	PRIM_type;	/* always SO_GETOPT */
	long	PROT_level;	/* protocol level */
	long	OPT_name;	/* option name */
};
#define SO_GETOPT_SIZE	sizeof (struct SO_getopt)

/* getname request */

struct SO_getname {
	long	PRIM_type;	/* always SO_GETNAME */
};
#define SO_GETNAME_SIZE	sizeof (struct SO_getname)

/* getpeer request */

struct SO_getpeer {
	long	PRIM_type;	/* always SO_GETPEER */
};
#define SO_GETPEER_SIZE	sizeof (struct SO_getpeer)

/* shutdown request */

struct SO_shutdown {
	long	PRIM_type;	/* always SO_SHUTDOWN */
	long	SHUT_down;	/* shutdown flags */
};
#define SO_SHUTDOWN_SIZE	sizeof (struct SO_shutdown)

/* accept acknowledgment */

struct SO_accept_ack {
	long	PRIM_type;	/* always SO_ACCEPT_ACK */
	long	ADDR_length;	/* addr length */
	long	ADDR_offset;	/* addr offset */
	long	SOCK_domain;	/* socket domain */
	long	SOCK_type;	/* socket type */
	long	SOCK_prot;	/* socket protocol */
	long	SEQ_number;	/* sequence number from T_conn_ind */
};
#define SO_ACCEPT_ACK_SIZE	sizeof (struct SO_accept_ack)

/* unitdata indication */

struct SO_unitdata_ind {
	long	PRIM_type;	/* always SO_UNITDATA_IND */
	long	SRC_length;	/* source addr length */
	long	SRC_offset;	/* source addr offset */
	long	DATA_flags;	/* data flags */
};
#define SO_UNITDATA_IND_SIZE	sizeof (struct SO_unitdata_ind)

/* getopt acknowledgment */

struct SO_getopt_ack {
	long	PRIM_type;	/* always SO_GETOPT_ACK */
	long	PROT_level;	/* protocol level */
	long	OPT_name;	/* options name */
	long	OPT_length;	/* options length */
	long	OPT_offset;	/* options offset */
};
#define SO_GETOPT_ACK_SIZE	sizeof (struct SO_getopt_ack)

/* getname acknowledgment */

struct SO_getname_ack {
	long	PRIM_type;	/* always SO_GETNAME_ACK */
	long	NAME_length;	/* name length */
	long	NAME_offset;	/* name offset */
};
#define SO_GETNAME_ACK_SIZE	sizeof (struct SO_getname_ack)

/* getpeer acknowledgment */

struct SO_getpeer_ack {
	long	PRIM_type;	/* always SO_GETPEER_ACK */
	long	PEER_length;	/* peer name length */
	long	PEER_offset;	/* peer name offset */
};
#define SO_GETPEER_ACK_SIZE	sizeof (struct SO_getpeer_ack)

/* error acknowledgment */

struct SO_error_ack {
	long	PRIM_type;	/* always SO_ERROR_ACK */
	long	ERROR_prim;	/* primitive in error */
	long	SOCK_error;	/* SOCKET error code */
	long	UNIX_error;	/* UNIX error code */
};
#define SO_ERROR_ACK_SIZE	sizeof (struct SO_error_ack)

/* ok acknowledgment */

struct SO_ok_ack {
	long	PRIM_type;	/* always SO_OK_ACK */
	long	CORRECT_prim;	/* correct primitive */
};
#define SO_OK_ACK_SIZE	sizeof (struct SO_ok_ack)

struct SO_disc_req {
	long	PRIM_type;	/* always SO_FDINSERT */
	long	SEQ_number;	/* sequence number from T_conn_ind */
};
#define SO_DISC_REQ_SIZE	sizeof (struct SO_disc_req)

/*
 * The following is a union of the primitives
 */
union SO_primitives {
	long	SP_type;	/* primitive type */
	struct SO_socket SP_socket;	/* socket request */
	struct SO_bind SP_bind;	/* bind request */
	struct SO_listen SP_listen;	/* listen request */
	struct SO_accept SP_accept;	/* accept response */
	struct SO_fdinsert SP_fdinsert;	/* fdinsert request */
	struct SO_connect SP_connect;	/* connect request */
	struct SO_send SP_send;	/* send data request */
	struct SO_setopt SP_setopt;	/* setopt request */
	struct SO_getopt SP_getopt;	/* getopt request */
	struct SO_getname SP_getname;	/* getname request */
	struct SO_getpeer SP_getpeer;	/* getpeer request */
	struct SO_shutdown SP_shutdown;	/* shutdown request */
	struct SO_accept_ack SP_accept_ack;	/* accept ack */
	struct SO_unitdata_ind SP_unitdata_ind;/* unitdata ind */
	struct SO_getopt_ack SP_getopt_ack;	/* getopt ack */
	struct SO_getname_ack SP_getname_ack;	/* getname ack */
	struct SO_getpeer_ack SP_getpeer_ack;	/* getpeer ack */
	struct SO_error_ack SP_error_ack;	/* error ack */
	struct SO_ok_ack SP_ok_ack;	/* ok ack */
	struct SO_disc_req SP_disc_req;	/* disconnect request */
};
#define SO_PRIMITIVES_SIZE	sizeof (union SO_primitives)




/*
 *	Renamed ISC extended error codes to support the socket module for SVR4.
 */
/*
 * Selected error codes
 * only network extensions
 */

#define EXTERBASE	65	/* where extended errno's start */

#define	ISC_EWOULDBLOCK	(35+EXTERBASE)	/* Operation would block */
#define	ISC_EINPROGRESS	(36+EXTERBASE)	/* Operation now in progress */
#define	ISC_EALREADY	(37+EXTERBASE)	/* Operation already in progress */

/* ipc/network software */

/* argument errors */
#define	ISC_ENOTSOCK	(38+EXTERBASE)	/* Socket operation on non-socket */
#define	ISC_EDESTADDRREQ	(39+EXTERBASE)	/* Destination address required */
#define	ISC_EMSGSIZE	(40+EXTERBASE)	/* Message too long */
#define	ISC_EPROTOTYPE	(41+EXTERBASE)	/* Protocol wrong type for socket */
#define	ISC_ENOPROTOOPT	(42+EXTERBASE)	/* Protocol not available */
#define	ISC_EPROTONOSUPPORT	(43+EXTERBASE)	/* Protocol not supported */
#define	ISC_ESOCKTNOSUPPORT	(44+EXTERBASE)	/* Socket type not supported */
#define	ISC_EOPNOTSUPP	(45+EXTERBASE)	/* Operation not supported on socket */
#define	ISC_EPFNOSUPPORT	(46+EXTERBASE)	/* Protocol family not supported */
#define	ISC_EAFNOSUPPORT	(47+EXTERBASE)	/* Address family not supported by protocol family */
#define	ISC_EADDRINUSE	(48+EXTERBASE)	/* Address already in use */
#define	ISC_EADDRNOTAVAIL	(49+EXTERBASE)	/* Can't assign requested address */

/* operational errors */
#define	ISC_ENETDOWN	(50+EXTERBASE)	/* Network is down */
#define	ISC_ENETUNREACH	(51+EXTERBASE)	/* Network is unreachable */
#define	ISC_ENETRESET	(52+EXTERBASE)	/* Network dropped connection on reset */
#define	ISC_ECONNABORTED	(53+EXTERBASE)	/* Software caused connection abort */
#define	ISC_ECONNRESET	(54+EXTERBASE)	/* Connection reset by peer */
#define	ISC_ENOBUFS		(55+EXTERBASE)	/* No buffer space available */
#define	ISC_EISCONN		(56+EXTERBASE)	/* Socket is already connected */
#define	ISC_ENOTCONN	(57+EXTERBASE)	/* Socket is not connected */
#define	ISC_ESHUTDOWN	(58+EXTERBASE)	/* Can't send after socket shutdown */
#define	ISC_ETOOMANYREFS	(59+EXTERBASE)	/* Too many references: can't splice */
#define	ISC_ETIMEDOUT	(60+EXTERBASE)	/* Connection timed out */
#define	ISC_ECONNREFUSED	(61+EXTERBASE)	/* Connection refused */

#define	ISC_ELOOP		(62+EXTERBASE)	/* Too many levels of symbolic links */

/* should be rearranged */
#define	ISC_EHOSTDOWN	(64+EXTERBASE)	/* Host is down */
#define	ISC_EHOSTUNREACH	(65+EXTERBASE)	/* No route to host */

/* quotas & mush */
#define	ISC_EPROCLIM	(67+EXTERBASE)	/* Too many processes */
#define	ISC_EUSERS		(68+EXTERBASE)	/* Too many users */
#define	ISC_EDQUOT		(69+EXTERBASE)	/* Disc quota exceeded */

#define ISC_EXT_ERRNO_MAX	ISC_EDQUOT


#if defined(__cplusplus)
	}
#endif

#endif /* _NET_ISOCKET_ISOCKET_H */
