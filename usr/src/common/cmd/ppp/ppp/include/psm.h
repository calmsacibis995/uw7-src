#ifndef _PSM_H
#define _PSM_H

#ident	"@(#)psm.h	1.10"

#include "fsm.h"
#include "act.h"
/*
 * Version number of the PSM API
 */
#define PSM_API_VERSION 1
#define PPP_VERSION_STRING "SCO PPP 7.0v1.0 for Unixware 7.0"

#undef ASSERT
#define ASSERT(x) ((x) || (psm_assert(#x, __FILE__, __LINE__), 0) || (assert(x),0))

/*
 * LCP Codes
 *
 * LCP Headers are used by most (if not all) other Contol Protocols
 * for encapsulation.
 *
 * These codes define the available LCP Packet types
 */
#define CFG_REQ	1
#define CFG_ACK 2
#define CFG_NAK 3
#define CFG_REJ 4
#define TRM_REQ 5
#define TRM_ACK 6
#define COD_REJ 7
#define PRT_REJ 8
#define ECO_REQ 9
#define ECO_RPL 10
#define DSC_REQ 11
#define IDENT	12
#define TIM_REM	13
#define LCP_LAST 13

/*
 * If a protocol declares itself as requiring the use of the LCP
 * FSM, then codes 1 through 7 will be handled by psm_rcv,
 * others will be passed to the protocol support modules
 * code receive function.
 */
#define LCP_LAST_BASIC 7

/*
 * When a configure option is passed to a PSM for processing
 * the CFG_MKNAK, CFG_CHECK,  CFG_DEFAULT
 * values are used to describe the action required.
 * On return, an event code is required, this can be
 * CFG_DISCARD, CFG_ACK or CFG_NAK
 */
#define CFG_DISCARD 14 /*Discard Packet */
#define CFG_MKNAK 15 /* Nak required */
#define CFG_CHECK 16 /* Check configuration */
#define CFG_DEFAULT 17 /* Set default value */
#define CFG_DISCARD_OK 18 /* Discard but don't count as an error */
/*
 * This structure is used to define a Configure Option for a protocol.
 */
struct psm_opt_tab_s {
	/* Function used to process the option */
	int 		(*op_rcv)();
	/* Function used to process the option */
	int 		(*op_snd)();
	ushort_t	op_flags;	
	ushort_t	op_len;		/* Length of the option */
	char 		*op_name;	/* Name of option (debug) */
};

/*
 * The minimum length for any configuration option
 */
#define MIN_OP_LEN 2

/*
 * Values for op_flags
 */
#define	OP_LEN_EXACT	0x0001	/* Require exact match on field length */
#define OP_LEN_MIN	0x0002	/* Must have minimim */
#define OP_NO_REJ 	0x0004	/* Don't call func if state is REJect */
#define OP_NO_NAK 	0x0008	/* Don't call func if state is NAK */
#define OP_MULTI 	0x0010	/* May be listed more than once in a request */
#define OP_REJISACK	0x0020	/* Treat reject of option as ack */

#define OP_NOSUPP(txt) \
	NULL, NULL, OP_LEN_MIN, MIN_OP_LEN, (txt)

/*
 * Protocol receive LCP packet code to handler table 
 */
struct psm_code_tab_s {
	ushort_t pc_code;
	ushort_t pc_flags;	
	int	(*pc_rcv)();
	char	*pc_name;
};

#define PC_CHKID      	0x0001	/* Check the lcp_id == the fsm->cp_lastid */
#define PC_ACCEPT	0x0002 	/* Code is acceptable */
#define PC_NOREJ	0x0004	/* Code cannot be rejected - Cannot operate */
				   

/*
 * Each PSM must supply the structure defined below, this specifies
 * the type of protocol and the functions provided for its support.
 */
typedef struct psm_tab_s {
	int		pt_version;
	/* General definitions */
	char 		*pt_desc;	/* Description of the protocol/alg */
	ushort_t	pt_proto;	/* CP Protocol value */
	uint_t		pt_flags;	/* Flags */
	ushort_t	pt_pri;		/* Message priority */
	
	ushort_t	*pt_netproto;	/* List of protocols wish to rcv */

	int		(*pt_load)();	/* Load routine */
	int		(*pt_unload)();	/* Unload routine */
	int		(*pt_alloc)();	/* Allocate private data */
	int		(*pt_free)();	/* Free private data */
	int		(*pt_init)();	/* Initialisation priv data func */

	int		(*pt_rcv)();	/* The protocol handler */
	int		(*pt_snd)();	/* Func to send a packet */

	int		(*pt_log)();	/* Protocol datagram log function */
	int		(*pt_status)();	/* Protocol specific status */

	union {
		/*
		 * The following fields must be defined if the PT_FSM bit in 
		 * the pt_flags is set.
		 */
		struct fsm_s {
			/* Control Protocol action table */
			void (*ft_act[6])();
			/* Config opts table */
			struct psm_opt_tab_s *ft_option;
			/* Number of configuration options */
			ushort_t ft_numopts;
			/*
			 * LCP style Code table, can be null if no other
			 * codes are supported
			 */
			struct psm_code_tab_s *ft_code;

			/* Func to configure an interface (only for NCP's) */
			int (*ft_k2d_msg)();

			/*
			 * Function called to receive messages
			 * from kernel, only if CCP or ECP
			 */

			int (*ft_cfg)();
			/* Func to deconfigure an interface (only for NCP's) */
			int (*ft_decfg)();
		} fsm;

		/*
		 * Fields for algorithms, PT_COMP ot PT_ENCR bit
		 * set in the flags
		 */
		struct {
			/* Fucntion to receive a Reset Request */
			int (*al_rcv_resetreq)();
			/* Fucntion to generate a Reset Ack */
			void (*al_rcv_resetack)();
			/* Function called when protocol is Up */
			void (*al_up)();
			/* Function called when protocol is going Down */
			void (*al_down)();
			/* Function called to handle kernel messages */
			void (*al_k2d_msg)();

			void (*al_pad[1])();

			struct psm_opt_tab_s *al_option; /* Option support */

			ushort_t al_num;	/* Algorithm number */


		} alg;

		/*
		 * An these may be defined when the PT_FSM bit is not set
		 */
		struct other_s {
			/* Start for non-FSM protocols */
			void (*ot_start)();
			/* Terminate for non-FSM protocols */
			void (*ot_term)();
		} other;

	} un;

} psm_tab_t;

#define pt_act un.fsm.ft_act
#define pt_option un.fsm.ft_option
#define pt_numopts un.fsm.ft_numopts
#define pt_code un.fsm.ft_code
#define pt_cfg un.fsm.ft_cfg
#define pt_k2d_msg un.fsm.ft_k2d_msg
#define pt_decfg un.fsm.ft_decfg

#define pt_algnum un.alg.al_num
#define pt_algoption un.alg.al_option
#define pt_rcv_resetreq un.alg.al_rcv_resetreq
#define pt_rcv_resetack un.alg.al_rcv_resetack
#define pt_alup un.alg.al_up
#define pt_aldown un.alg.al_down
#define pt_alk2d_msg un.alg.al_k2d_msg

#define pt_start un.other.ot_start
#define pt_term un.other.ot_term

/*
 * Values for pt_flags
 */
#define	PT_NCP		0x0001 	/* Is a Network Control Protocol */
#define PT_AUTH 	0x0002	/* Is an Authentication protocol */
#define PT_COMP		0x0004	/* Is a Compression algorithm */
#define PT_FSM		0x0008	/* Protocol uses FSM */
#define PT_LCP		0x0010	/* Is LCP */
#define PT_LINK		0x0020	/* Protocol may be used in a link definition */
#define PT_BUNDLE 	0x0040	/* Proto may be used in a bundle definition */
#define PT_ENCR		0x0080	/* Is an encryprtion algorithm */
#define PT_WANTD 	0x0100	/* User level psm wants datagrams */

/*
 * Values for Protocol Priorities
 *
 * PT_PRI_HI	- Used when the protocol messages need to get to their
 *		  destination ahead of other protocols. E.g. LCP
 * PT_PRI_MED	- Used when messages should not be delayed by other
 *		  network layer traffic
 * PT_PRI_LOW	- This is the same priority as interactive network layer
 *		  traffic.
 */
#define PT_PRI_HI	0x03
#define PT_PRI_MED	0x02
#define PT_PRI_LOW	0x01

/*
 * Message levels for protolog
 */
typedef enum {
	MSG_INFO = 0,	/* Information - low detail - Summary*/
	MSG_AUDIT,
	MSG_INFO_LOW,	/* Information - low detail - Detailed */
	MSG_INFO_MED,	/* Information - medium detail - Packet */
	MSG_INFO_HIGH,	/* Information - high detail */
	MSG_ULR,	/* Generated on receipt of User Level Requests */
	MSG_DEBUG,	/* Debug message */
	MSG_WARN,	/* Warning message */
	MSG_ERROR,	/* Error message */
	MSG_DEF_ERROR,	/* Definition error */
	MSG_FATAL	/* Fatal error occured - Daemon will exit */
} msg_level_t;


/*
 * Max block size to send to peer (Including protocol field .. hence +2)
 */
#define PSM_MTU(ah) \
	(((ah)->ah_type == DEF_LINK ? \
	 (ah)->ah_link.al_peer_mru : \
	 (ah)->ah_bundle.ab_mtu) + 2)


/*
 * Debug modes
 */
#define DEBUG_ANVL	0x01	/* Set if running in anvl test mode */
#define DEBUG_PPPD	0x02	/* Set if debuffing pppd */

/*
 * Function prototypes for PSM Interface
 */
void psm_log(msg_level_t level, proto_hdr_t *ph, char *fmt, ...);
struct psm_code_tab_s *psm_code(psm_tab_t *pt, ushort_t code);
struct psm_tab_s *psm_getpt(char *proto);
int psm_rcv(proto_hdr_t *ph, db_t *db);
int psm_snd(proto_hdr_t *ph, db_t *db);
void psm_add_pf(proto_hdr_t *ph, db_t *db);
void psm_init();
void psm_unload();
int psm_bind(proto_hdr_t *ph);
void psm_log(msg_level_t level, proto_hdr_t *ph, char *fmt, ...);
int psm_loghex(int level, proto_hdr_t *ph, char *p, int cnt);
int psm_logstr(int level, proto_hdr_t *ph, char *p, int cnt);
void psm_ncp_up(proto_hdr_t *ph);
void psm_ncp_down(proto_hdr_t *ph);
void psm_close_link(act_hdr_t *al, int reason);
void psm_reject(act_hdr_t *ah, db_t *db, ushort_t proto);
alg_hdr_t *psm_find_alg(proto_hdr_t *ph, ushort_t alg, int flags);
alg_hdr_t *psm_get_alg(proto_hdr_t *ph, int flags);
void psm_set_alg(proto_hdr_t *ph, int flags, alg_hdr_t *ag);

#endif /* _PSM_H */
