#ifndef _ACT_H
#define _ACT_H

#ident	"@(#)act.h	1.9"

#include <synch.h>
#include <sys/ppp_pcid.h>
#include "ppp_type.h"

#define INUSE 0xabcd1234 /* Magic number showing in use */

/*
 * Structure for chaining algorithms
 */
typedef struct alg_hdr_s {
	uint_t 			ag_inuse;	/* Debug */
	struct cfg_alg		*ag_cfg;	/* Proto configuration */
	struct alg_hdr_s	*ag_next;	/* Next in chain */
	struct act_hdr_s	*ag_parent;	/* Parent link/bundle ??? */
	ushort_t 		ag_id;		/* Option number for alg */
	ushort_t		ag_flags;	/* Flags */
	struct psm_tab_s	*ag_psmtab;	/* PSM table */
	void 			*ag_priv;	/* Alg private data */	
} alg_hdr_t;

#define AGF_NEG	0x0001	/* Configured for Rx */

/*
 * Atructure to hold heads of algorithm lists
 */
struct alg_s {
	alg_hdr_t *as_tx_algs; 	/* List of configured algorithms tx */
	alg_hdr_t *as_rx_algs; 	/* List of configured algorithms rx */
	alg_hdr_t *as_tx;	/* The chosen tx alg */
	alg_hdr_t *as_rx;	/* The chosen rx alg */
};

/*
 * Structure containing info about protocols
 */
typedef struct proto_hdr_s {
	uint_t ph_inuse; /*DEBUG*/
	struct proto_hdr_s *ph_next;	/* Next protocol in list */
	struct act_hdr_s *ph_parent;	/* Parent of this proto (link/bundle)*/
	struct cfg_hdr *ph_cfg;		/* Proto configuration */
	ushort_t ph_flags;		/* Flags */
	proto_state_t ph_state;		/* FSM for this protocol */
	struct psm_tab_s *ph_psmtab;	/* Protocol Support Module table */

	ushort_t ph_peer_opts[16];	/* Options selected by peer */
	ushort_t ph_rej_opts[16];	/* Options rejected by peer */
	ushort_t ph_cod_rej[16]; 	/* LCP Codes rejected by peer */
	db_t *ph_rtbuf;			/* Retransmit buffer, contains
					 * last configure/terminate 
					 * request sent */

	void *ph_priv;			/* Protocol private data */
	void *ph_rx_stats;		/* Statistics structure */
	void *ph_tx_stats;		/* Statistics structure */
} proto_hdr_t;

#define PHF_AUTO	0x0001 /* This protocol is on an auto bringup bundle */
#define PHF_IDLE	0x0002 /* The protocol is idle (NCP's) */
#define PHF_NCP_UP	0x0004 /* The protocol (NCP) is up/opened */
/*
 * Macros to access the opts fields in proto_hdr_s
 */
#define PH_TSTBIT(bit, ary) \
	(ary[(bit) / 16] & (1 <<(bit) % 16))

#define PH_SETBIT(bit, ary) \
	ary[(bit) / 16] |= 1 << ((bit) % 16)

#define PH_RESETBITS(ary) \
	ary[0] = 0; ary[1] = 0; ary[2] = 0; ary[3] = 0; \
	ary[4] = 0; ary[5] = 0; ary[6] = 0; ary[7] = 0; \
	ary[8] = 0; ary[9] = 0; ary[10] = 0; ary[11] = 0;  \
	ary[12] = 0; ary[13] = 0; ary[14] = 0; ary[15] = 0; 

/*
 * Per physical link definitions
 */
typedef struct act_link_s {
	uint_t		al_flags;	/* Link status flags */
	int		al_fd;		/* File descriptor for this device */
	char		*al_uid;	/* User name of incoming link */
	char 		*al_cid;	/* Caller ID of incoming link */
	int		al_type;	/* Link Type */
	ushort_t	al_auth_tmout;	/* Timeout for authentication phase */
	uint_t		al_bandwidth; /* Estimate bw of link */

	/*
	 * These options are stored here and on in an LCP private
	 * data structure because they have cross protocol impact,
	 * mru for e.g. is important when sending all datagrams.
	 * .. and so LCP options need to be  accessibile from a large
	 * number of modules
	 */
	uint_t		al_local_opts;
	ushort_t	al_local_mru;
	ulong_t		al_local_accm;
	ulong_t		al_local_magic;
	ushort_t	al_local_mrru;
	uchar_t		al_local_chap_alg;	/* Chap algorithm */
	ushort_t	al_local_qual_proto;
	ushort_t	al_local_ld;

	uint_t		al_peer_opts;
	ushort_t	al_peer_mru;
	ulong_t		al_peer_accm;
	ulong_t	 	al_peer_magic;
	ushort_t	al_peer_mrru;
	uchar_t		al_peer_chap_alg;	/* Chap algorithm */
	ushort_t	al_peer_qual_proto;
	ushort_t	al_peer_ld;

	uchar_t		*al_peer_auth_name;
	uchar_t		al_peer_auth_namelen;

	uchar_t		al_peer_ed_class; /* Class of peers ed */
	uchar_t		al_peer_ed_len;
	uchar_t		al_peer_ed[MAX_ED_SIZE]; /* Peers ED */

	struct calls_s	*al_connect;	/* Connection details */

	struct act_hdr_s *al_next;	/* Next in bundle chain of links */
	struct act_hdr_s *al_bundle;	/* Bundle that link is active in */
	struct proto_hdr_s *al_lcp;	/* A cheat to get to lcp fast */

	struct pc_stats_s al_stats;	/* The links stats (from kernel) */

	ushort_t	al_reason;	/* Failure reasons */
	ushort_t	al_cindex;	/* Index in bundles config */
} act_link_t;

/*
 * Active Link Flags (al_flags)
 */
#define ALF_INUSE	0x0001
#define ALF_PHYS_UP 	0x0002	/* Physical layer is UP */
#define ALF_COND	0x0004	/* Link is conditioned */
#define ALF_LOCAL_AUTH	0x0008	/* Local Auth Succeeded */
#define ALF_PEER_AUTH	0x0010	/* Peers Auth Succeeded */
#define ALF_INCOMING	0x0020	/* Incoming caused the link to join bundle */
#define ALF_OUTGOING	0x0040	/* Outgoing call caused link to join bundle */
#define ALF_RENEG	0x0080	/* Have performed a re-negotiation on the link */
#define ALF_ML_NEG	0x0100	/* ML Options are negotiable */
#define ALF_PRIMARY	0x0200	/* First link in a bundle */
#define ALF_DIAL	0x0400	/* Link is dialing */
#define ALF_HANGUP	0x0800	/* Link is hanging up */
#define ALF_OPEN	0x1000	/* Link is counted in ab_open_links */
#define ALF_AUTO	0x2000	/* Incoming link ... auto detected */

/*
 * Bit fields for al_reason
 */
#define ALR_AUTHFAIL	0x0001	/* Auth failed */
#define ALR_LOOPBACK	0x0002	/* Possible loopback */
#define ALR_LOWBW	0x0004	/* Link droped due to low utilization */
#define ALR_HANGUP	0x0008	/* Detected h/w hangup */
#define ALR_RENEG	0x0010	/* Linked dropped for renegotiation */
#define ALR_DIAL	0x0020	/* Dial failed */
#define ALR_LOWQUAL	0x0040	/* Closed due to low quality */
#define ALR_BADIN	0x0080	/* Rejected incoming call */
#define ALR_MISMATCH	0x0100	/* Links id didn't match bundle */
#define ALR_USERCLOSE	0x0200	/* User requested link closed */
#define ALR_AUTHNEG	0x0400	/* Auth not negotiated correctly */
#define ALR_AUTHTMOUT	0x0800	/* Authentication timed-out */
/*
 * The Bundle definition.
 */
typedef struct act_bundle_s {

	uint_t		ab_flags;	/* Bundle flags */

	/*
	 * When this bundle has active links, they will be
	 * pointed to on the following list
	 */
	struct act_hdr_s *ab_links;	
	ushort_t	ab_addindex;	/* The link index last tried when
					 * making an outgoing call.. index into
					 * our list of configured links */
	ushort_t	ab_dropindex;	/* The link index last dropped */
	
	char 		*ab_lastphonenum; /* Last phone number dialed */

	ushort_t	ab_local_mrru;	/* Max Rx Reconstructed Unit */
	ushort_t	ab_local_ssn;	/* Sort Sequence Numbers */
	ushort_t	ab_local_ed;	/* Endpoint discriminator ? */
	ushort_t	ab_peer_mrru;	/* Max Rx Reconstructed Unit */
	ushort_t	ab_peer_ssn;	/* Sort Sequence Numbers */
	ushort_t	ab_peer_ed;	/* Endpoint discriminator ? */
	uchar_t		ab_local_ed_class; /* Class of peers ed */
	uchar_t		ab_local_ed_len;
	uchar_t		ab_local_ed_addr[MAX_ED_SIZE]; /* Peers ED */
	ushort_t	ab_numlinks;	/* Number of links attached */
	ushort_t	ab_open_links;	/* Number of links OPENED */
	ushort_t	ab_open_ncps;	/* Number of ncp's open */
	ushort_t	ab_idle_ncps;	/* Number of idle ncp's */
	ushort_t	ab_mtu;		/* Current max transmission unit */

	struct bl_stats_s ab_stats;

	ushort_t	ab_reason;	/* Failure reasons */
} act_bundle_t;		  

/*
 * Values for ab_flags
 */
#define ABF_CALLED_OUT	0x0001	/* Bundle initiated connection */
#define ABF_CALLED_IN	0x0002	/* Peer Called us to initiate connection */
#define ABF_AUDIT	0x0004	/* Bundle requires an audit entry */
#define ABF_LDS		0x0008	/* Links require dirscriminators */

/*
 * Bits for ab_reason
 */
#define ABR_IDLE	0x0001	/* Bundle was idle */
#define ABR_USERCLOSE	0x0002	/* User requested bundle closed */

typedef struct act_hdr_s act_hdr_t;

/*
 * If the ah_refcnt is > 0 then the structure may not be freed
 */
struct act_hdr_s {
	uint_t		ah_inuse;/*DEBUG*/
	act_hdr_t	*ah_next;	/* Next definition */
	mutex_t		ah_mutex;	/* Definition lock */
	atomic_int_t	*ah_refcnt;	/* Refcount to this structure */
	int		ah_type;	/* Definition type DEF_LINK 
					   or DEF_BUNDLE*/
	int		ah_index;	/* Identifier for bundle/link
					 * (e.g. mux id) */
	short		ah_phase;	/* Bundle/Link Phase */
	short		ah_debug;	/* Debug level */

	struct proto_hdr_s *ah_protos; 	/*  List of protcols for this element */
	struct cfg_hdr	*ah_cfg;	/* Definition configuration */
	union {
		struct act_link_s link;
		struct act_bundle_s bundle;
	} un;
};
#define ah_link un.link
#define ah_bundle un.bundle




#endif /* _ACT_H */
