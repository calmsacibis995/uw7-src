#ident	"@(#)pppd.h	1.2"
#ident	"$Header$"
#ident "@(#) pppd.h,v 1.8 1995/08/28 14:24:11 neil Exp"
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
 *
 */
/*      SCCS IDENTIFICATION        */

#define IFNAME_SIZE	63	/* maximum length for interface name */
#define	PPPBUFSIZ	8192	/* maximum characters for a conf file entry */

/* ppp host information */
struct ppphostent {
        char    loginname[NAME_SIZE + 1];       /* ppp login name */
        char    uucp_system[NAME_SIZE + 1];     /* uucp name */
        ushort  uucpretry;                      /* uucp retry time */
        char    device[TTY_SIZE + 1];           /* static link tty device */
        char    speed[NAME_SIZE + 1];           /* static link speed */
        char    attach[NAME_SIZE + 1];          /* attach name */
        short   flow;                           /* flow control fot the link */
        short   clocal;                         /* Do not wait for carrier on tty */
        char    tag[NAME_SIZE + 1];             /* filter tag */
        short   proxy;                          /* proxy arp */
        struct  ppp_configure ppp_cnf;          /* configuration info.*/
        char    pool_tag_local[NAME_SIZE+1];    /* Address pool name -- local addr  */
        char    pool_tag_remote[NAME_SIZE+1];   /* Address pool name -- remote addr */
};

typedef struct pppd_mod_list pppd_mod_list_t;

/* Assumed Max length of module name */

#define PPP_MOD_NM_LN  256
 
struct pppd_mod_list {
  char mod_name[PPP_MOD_NM_LN];
  pppd_mod_list_t *next;
};


/* structure holds ppp/ip descriptor */ 
struct	ip_made_s {
	int ipfd;		/* ip file descriptor */
	int pppfd;		/* ppp file descriptor */
	int ipmuxid;		/* ip muxid */
};

/* structure holds ppp link information */
struct conn_made_s {
        struct  conn_made_s      *next;  /* Next Element in list (DON'T MOVE)*/
        struct  conn_made_s      *prev;  /* Next Element in list (DON'T MOVE)*/
	struct	ip_made_s	ip;
	char	ifname[IFNAMSIZ];	/* interface name */
	char    tty_name[256];          /* Name of tty */
	int     speed;                  /* Speed */
	int     flow;                   /* Flow Control */
	short	type;			/* link type */	
	int fd;				/* mbcl file descriptor */
	int cfd;			/* the actual tty file descriptor */
	int muxid;			/* mux id */
	short pgrp;			/* ppp shell/dial process id */
	struct	sockaddr_in	 dst;	/* remoet address */
	struct	sockaddr_in	 src;	/* local address */
	char	tag[NAME_SIZE+1];	/* filter tag */
	short	proxy;			/* add proxy arp entry */
	unchar	debug;			/* debug level */
	struct	conn_conf_s	*conf;	/* poniter to configure structure */
	pppd_mod_list_t *mod_list;      /* List of modules popped from fd */
	char pool_tag_local[NAME_SIZE+1];     /* Address pooling tag */
	char pool_tag_remote[NAME_SIZE+1];     /* Address pooling tag */
        int attach_fd;                  /* fd open to pppattach */
        char attach_name[NAME_SIZE+1];  /* Attachname for link (if applicable) */
};

/* number of ppp connections the pppd supports */ 
#if defined(N_CONN)
#undef N_CONN
#endif
#define N_CONN 64


/* link type */
#define	INCOMING	1			
#define	OUTGOING	2		
#define	STATIC_LINK		3	
#define	OUTATTACH	4	
#define MAX_LINK_TYPE   4

/* structure holds outgoing or static ppp interface configuration information */
struct conn_conf_s {
        struct  conn_conf_s     *next;  /* Next element in list (DON'T MOVE)*/
        struct  conn_conf_s     *prev;  /* Next element in list (DON'T MOVE)*/
	struct	ip_made_s	ip;
	char	ifname[IFNAMSIZ];	/* interface name */
	char	device[TTY_SIZE+1];	/* device name */
	char	speed[NAME_SIZE+1];	/* line speed */
	short	flow;			/* line flow control */
	char	tag[NAME_SIZE+1];	/* filter tag */
	struct	sockaddr_in	remote;	/* remoet address */
	struct	sockaddr_in	local;	/* local address */
	struct	sockaddr_in	mask;	/* mask */
	struct	conn_made_s	*made;	/* pointer to link structure */
};


/* message between ppp shell, pppstat, ppp daemon child and PPP daemon */
typedef struct msg {
	char	m_type;			/* message type */
	short	m_pid;			/* process id */
	char	m_tty[TTY_SIZE+1];	/* tty name */
        int     m_fd;                   /* Used by pppattach */
	union {
		char	name[NAME_SIZE+1];	/* PPP login name */
		struct	sockaddr_in remoteaddr;	/* remote host address */
		char	attach[NAME_SIZE+1];	/* PPP attach name */
	} m_addr;
#define	m_name		m_addr.name
#define	m_remote	m_addr.remoteaddr
#define	m_attach	m_addr.attach
} msg_t;

/* m_type's */
#define	MTTY		16
#define	MPID		15
#define	MSTAT		17
#define	MDIAL		18
#define	MDATTACH	19
#define	MATTACH		20	
#define CSTAT           21
#define MONITOR         22
#define MONITOR_CLOSE   23
#define POOLSTATS       24
#define MDATTACH_FAIL   25
#define MDATTACH_BUSY   26

/* notion used in PPP configuration files */ 
#define	ASTERISK	'*'
#define POUND		'#' 
#define	COLON		':'
#define	PLUS		'+'

#define	RADDR		"remote="
#define	LADDR		"local="
#define	MASK		"mask="
#define	UUCPNAME	"uucp="
#define	STATICDEV	"staticdev="
#define	SPEED		"speed="
#define	FILTER		"filter="
#define	IDLE		"idle="
#define	REQTMOUT	"reqtmout="
#define	CONF		"conf="
#define	TERM		"term="
#define	NAK		"nak="
#define	MRU		"mru="
#define	ACCM		"accm="
#define	AUTH		"auth="
#define	NAME		"name="
#define	AUTHPAP		"pap"
#define	AUTHCHAP	"chap"
#define	NOMGC		"nomgc"
#define	NOPROTCOMP	"noprotcomp"
#define	NOACCOMP	"noaccomp"
#define	NOIPADDR	"noipaddr"
#define	RFC1172ADDR	"rfc1172addr"
#define	NOVJ		"novj"
#define	OLD		"old"
#define	AUTHTMOUT	"authtmout="
#define	FLOW		"flow="
#define	CLOCL		"clocal"
#define	HFLOW		"rtscts"
#define	SFLOW		"xonxoff"
#define	RETRY		"retry="
#define	DEBUGGING	"debug="
#define	PROXY		"proxy"
#define	ATTACH		"attach="
#define	MAXSLOT		"maxslot="
#define	NOSLOTCOMP	"noslotcomp"

/* 
 * conn_made and conn_conf list allocation/Free macros
 */

#define CONN_MADE_ADD()   (struct conn_made_s *) element_add((list_header_t **) &conn_made, \
						      sizeof(struct conn_made_s))

#define CONN_MADE_REMOVE(cm) element_free((list_header_t **) &conn_made, \
					  (list_header_t *) (cm))

#define CONN_CONF_ADD()   (struct conn_conf_s *) element_add((list_header_t **) &conn_conf, \
						      sizeof(struct conn_conf_s))

#define CONN_CONF_REMOVE(cf) element_free((list_header_t **) &conn_conf, \
					  (list_header_t *) (cf))

