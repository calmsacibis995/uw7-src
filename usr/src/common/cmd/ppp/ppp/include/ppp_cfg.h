#ifndef _PPP_CFG_H
#define _PPP_CFG_H

#ident	"@(#)ppp_cfg.h	1.6"

#include "ppp_type.h"

typedef struct cfg_hdr {
	struct	cfg_hdr *ch_next;	/* Used to create lists of configs */
	char	ch_id[MAXID + 1];	/* ID of configuration */
	uint_t	ch_stroff;		/* Offset to start of strings */
	uint_t	ch_refcnt;		/* References to this configuration */
	uint_t	ch_flags;		/* Config flags */
	int	ch_len;			/* Total length of config structure */
} cfg_hdr_t;

#define CHF_RONLY	0x01	/* Definition is readonly */
#define CHF_HIDE	0x02	/* Don't list/status/stats this definition */

struct cfg_link {
	struct cfg_hdr	ln_ch;
	uint_t	ln_type;
	uint_t	ln_dev;		/* Offset to device string */
	uint_t	ln_push;	/* Offset to push string */
	uint_t	ln_pop;		/* Offset to pop string */
	uint_t	ln_phone;	/* Offset to phone string */
	uint_t	ln_flow;	/* Flow control for the link */
	uint_t	ln_protocols;	/* List of protocols for this link lcpA, ccpA etc */
	uint_t	ln_bandwidth;	/* Approximate bandwidth of the link (bps) */
	uint_t	ln_debug;	/* Debug level */
	char 	ln_var[1];	/* Start of strings */
};

#define ln_id	ln_ch.ch_id
#define ln_len	ln_ch.ch_len

/*
 * FSM config parameters
 */
struct cfg_fsm {
	uint_t		fsm_req_tmout;	/* Max time for request */
	uint_t		fsm_max_trm;	/* Num of term req before peer dead */
	uint_t		fsm_max_cfg;	/* Max num configure attempts */
	uint_t		fsm_max_fail;	/* Max num naks */
};

/*
 * Generic protocol config message
 */
struct cfg_proto {
	struct cfg_hdr	pr_ch;
	uint_t		pr_name;	/* Text identifying protocol */
	struct cfg_fsm	pr_fsm;
	char 		pad[1024];	/* padding for protocol options */
};
#define pr_id	pr_ch.ch_id
#define pr_len	pr_ch.ch_len

/*
 * Generic algorithm config message
 */
struct cfg_alg {
	struct cfg_hdr	al_ch;
	uint_t		al_name;	/* Text identifying protocol */
	char 		pad[1024];	/* padding for protocol options */
};
#define al_id	al_ch.ch_id
#define al_len	al_ch.ch_len

/*
 * Bundle configuration
 */
struct cfg_bundle {
	struct cfg_hdr	bn_ch;
	uint_t		bn_type;	/* In/Out/Both */
	uint_t		bn_protos;	/* list of protos */
	uint_t		bn_mrru;	/* Max receive reconstructed unit */
	uint_t		bn_ssn;		/* Short Sequence numbers ? */
	uint_t		bn_maxlinks;	/* Max links in bundle */
	uint_t		bn_minlinks;	/* Min links in bundle */
	uint_t		bn_links;	/* Links avialable - string */
	uint_t		bn_ed;		/* Endpoint discriminator option */
	uint_t		bn_minfrag;	/* Minimum fragment size */
	uint_t		bn_maxfrags;	/* Max num frags for reass */
	uint_t		bn_mlidle;	/* Time after which links are idle */
	uint_t		bn_nulls;	/* Tx NULL packets on idle links */
	uint_t		bn_addload;	/* Load at which to add links */
	uint_t		bn_dropload;	/* Load at which to drop links */
	uint_t		bn_addsample;	/* Sample size for add load calcs */
	uint_t		bn_dropsample;	/* Sample size for drop load calcs */
	uint_t		bn_thrashtime;	/* Min time btwn add/drop */
	uint_t		bn_maxidle;	/* Time ncps idle before
					 * we drop transports */ 
	uint_t		bn_bod;		/* Bod type */
	uint_t		bn_debug;	/* Debug level */

	/* Incoming params */
	uint_t		bn_uid;		/* User ID string */
	uint_t		bn_cid;		/* Caller ID */
	uint_t		bn_authid;	/* Name of authenticated peer */

	/* Outgoing params */
	uint_t		bn_remote;	/* Name of remote system */
	uint_t		bn_phone;	/* Phone number to dial */
	uint_t		bn_bringup;	/* Type of bringup */

	/* Auth params */
	uint_t		bn_pap;		/* We require PAP */
	uint_t		bn_chap;	/* We require CHAP */
	uint_t		bn_authname;	/* Our auth name */
	uint_t		bn_peerauthname;/* Peers auth name */
	uint_t		bn_chapalg;	/* Authentication algorithm */
	uint_t		bn_authtmout;	/* Max time for authentication */

	char		bn_var[1];	/* Strings*/
};
#define bn_id	bn_ch.ch_id
#define bn_len	bn_ch.ch_len

#define OUT_AUTOMATIC	0x01	/* Link comes up when traffic is detected */
#define OUT_MANUAL	0x02	/* Link is brought up administratively */

struct cfg_auth {
	struct cfg_hdr	au_ch;
	uint_t		au_protocol;	/* Protocol use for */
 	uint_t		au_name;	/* Auth name */
	uint_t		au_peersecret;	/* Peer Secret for the auth def */
	uint_t		au_localsecret;	/* Peer Secret for the auth def */
	char		au_var[1];	/* Strings*/
};
#define au_id	au_ch.ch_id
#define au_len	au_ch.ch_len


struct cfg_global {
	struct cfg_hdr	gi_ch;
	uint_t		gi_type;	/* Type of global defs */
	uint_t		gi_pap;		/* We require PAP */
	uint_t		gi_chap;	/* We require CHAP */
	uint_t		gi_authname;	/* Our auth name */
	uint_t		gi_peerauthname;/* Peers auth name */
	uint_t		gi_chapalg;	/* Authentication algorithm */
	uint_t		gi_authtmout;	/* Max time for authentication */
	uint_t		gi_mrru;	/* MRRU */
	uint_t		gi_ssn;		/* Short sequence numbers */
	uint_t		gi_ed;		/* Endpoint disc */
	char		gi_var[1];	/* Strings*/
};
#define gi_id	gi_ch.ch_id
#define gi_len	gi_ch.ch_len

/*
 * Is this the best place for list enty definitions ?
 */
struct list_ent {
	void		*le_cookie;
	uint_t		le_refs;		/* Number of refs to entry */
	char		le_id[MAXID + 1];
};

/*
 * Flags for global ucfg_state
 */
#define UCFG_MODIFIED	0x01


#endif /* _PPP_CFG_H */
