#ifndef _ULR_H
#define _ULR_H
#ident	"@(#)ulr.h	1.5"

#include "ppp_type.h"
#include "ppp_cfg.h"
#include "fsm.h"
#include "act.h"
#include "hist.h"

#include "cs.h" /* Should be <cs.h> */
#define SYSNAME_SIZE 16

#define ULR_PSM_MAX 256

/*
 * Definition used for user-level requests
 */

/*
 * Call history
 */
struct ulr_callhistory {
	int	prim;				/* ULR_CALLHIST */
	int	error;				/* Any associated error */
	int	cookie;				/* The entry number */
	int	more;				/* More entries available ? */
	int	num;				/* Number of ents in here */
	
	struct histent_s he;
};	

/*
 * Incoming call notification
 */
struct ulr_incoming {
	int	prim;				/* Primitive, ULR_INCOMING */
	int	error;				/* Any associated error */
	char	uid[MAXUIDLEN + 1];		/* Users login if any */
	char	ics[ICS_MIN_IDENT];		/* ics stuff */
	int	fd;				/* File descriptor of dev */
};

/*
 * Attach/Detach bundle
 */
struct ulr_attach {
	int	prim;				/* Primitive, ULR_ATTACH */
	int	error;				/* Any associated error */
	char 	bundle[MAXID + 1];		/* PPP bundle name */
	int 	derror;				/* Any dial specific error */
	char 	link[MAXID + 1];		/* PPP link name */
};

/*
 * Status notification
 */
struct ulr_status {
	int	prim;				/* Primitive, ULR_STATUS */
	int	error;				/* Any associated error */
	int	type;
	char 	id[MAXID + 1];			/* link/bundle name */
	int	index;				/* link/bundle index */
	act_hdr_t ah;
};

/*
 * Statistic notification
 */
struct ulr_stats {
	int	prim;				/* Primitive, ULR_STATS */
	int	error;				/* Any associated error */
	int	type;
	char 	id[MAXID + 1];			/* link/bundle name */
	int	index;				/* link/bundle index */
	union {
		int a;
		struct pc_stats_s pc;		/* Link stats */
		struct bl_stats_s bl;		/* Bundle stats */
	} un_1;
};
#define spc un_1.pc
#define sbl un_1.bl

struct ulr_psm_stat {
	int	prim;			/* Primitive, ULR_PSM_STATS */
	int	error;			/* Any associated error */
	int	type;			/* Link or bundle ? */
	char	id[MAXID + 1];		/* Link/bundle id */
	int	cookie;			/* The entry number */
	int	more;			/* More entries are available ? */
	char 	psmid[MAXID + 1];	/* PSM name */
	char 	cfgid[MAXID + 1];	/* Name of config definition */
	int	flags;			/* Any flags */
	char	data[ULR_PSM_MAX];	/* PSM Specific data */
	char	data2[ULR_PSM_MAX];	/* PSM Specific data */
};
#define rxdata data
#define txdata data2

struct ulr_debug {
	int	prim;			/* Primitive, ULR_DEBUG */
	int	error;			/* Any associated error */
	int	type;
	int	level;
	char	id[MAXID + 1];		/* link/bundle name */
};

struct ulr_cfg {
	int	prim;		/* Primitive, ULR_SET_CFG */
	int	error;		/* Any associated error */
	int 	type;		/* Link, Group, Proto .. See DEF_ in def.h */
	union {
		struct cfg_hdr 		hdr;
		struct cfg_link 	link;
		struct cfg_proto	proto;
		struct cfg_bundle	bundle;
		struct cfg_auth		auth;
		struct cfg_global	global;
		struct cfg_alg		alg;
	} un;
};
#define ucch		un.hdr
#define uclink		un.link
#define ucproto	un.proto
#define ucbundle	un.bundle
#define ucauth		un.auth
#define ucglobal	un.global
#define ucalg		un.alg

#define ucid	ucch.ch_id
#define uclen	ucch.ch_len

struct ulr_list_cfg {
	int		prim;		/* Primitive, ULR_LIST_CFG */
	int		error;		/* Any associated error */
	int		type;		/* DEF_LINK, DEF_GR ... */
	int		num;		/* Number of entries contained */
	struct list_ent	entry;		/* First entry */
};

/*
 * Used to return the links associated with a bundle
 */
struct ulr_list_links {
	int	prim;			/* Primitive, ULR_LIST_LINKS */
	int	error;			/* Any associated error */
	char 	bundle[MAXID + 1];	/* PPP bundle name */
	int	numlinks;		/* Number of links found */
	/* Followed by link index's */
	int	index[1];
};

/*
 * Get/Set status of config. Modified/Clean
 */
struct ulr_state {
	int	prim;			/* Primitive, ULR_CFGSTATE */
	int	error;			/* Any associated error */
	int	cmd;
	int	cfgstate;		/* Config state */
	int	debug;			/* Debug mode */
};
#define UDC_GET		0x01
#define UDC_SET		0x02

struct ulr_log {
	int prim;		/* ULR_LOG */
	int error;
	char msg[1];		/* Start of message to log */
};

struct ulr_prim {
	int prim;
	int error;
};

/*
 * Union of message types
 */
typedef union {
	struct ulr_callhistory callhist;
	struct ulr_incoming incoming;
	struct ulr_attach attach;
	struct ulr_status status_act;
	struct ulr_cfg ucfg;
	struct ulr_list_cfg lcfg;
	struct ulr_stats stats;
	struct ulr_debug debug;
	struct ulr_list_links links;
	struct ulr_psm_stat psmstat;
	struct ulr_state state;
	struct ulr_prim prim;
} ulr_msg_t;

/*
 * Max message size supported for IPC between ppptalk and pppd
 */
#define ULR_MAX_MSG 4096

/*
 * Message types
 */
#define ULR_INCOMING	0
#define ULR_ATTACH	1
#define ULR_DETACH	2
#define ULR_KILL	3
#define ULR_STATUS	4
#define ULR_SET_CFG	5
#define ULR_GET_CFG	6
#define ULR_LIST_CFG	7
#define ULR_DEL_CFG	8
#define ULR_RESET_CFG	9
#define ULR_STOP	10
#define ULR_LOG		11
#define ULR_LINKADD	12
#define ULR_LINKDROP	13
#define ULR_LINKKILL	14
#define ULR_STATS	15
#define ULR_DEBUG	16
#define ULR_LIST_LINKS	17
#define ULR_PSM_STATS	18
#define ULR_PSM_STATUS	19
#define ULR_STATE	20
#define ULR_CALLHIST	21
#define ULR_HISTCLEAR	22

#define ULR_END		23	/* This must be the last in the list */

#endif _ULR_H
