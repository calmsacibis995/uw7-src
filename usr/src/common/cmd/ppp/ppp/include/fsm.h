#ifndef _FSM_H
#define _FSM_H

#ident	"@(#)fsm.h	1.6"

/*
 * FSM state structure.  Each control protocol keeps
 * its own FSM state structure for each connection.  This structure
 * contains the working state and other required information for a
 * connections instance of a particular control protocol.
 */

typedef struct proto_state_s {

	uchar_t	ps_state;	/* cp fsm state for this protocol */
	uchar_t	ps_lastid;	/* last id field value */
	void 	*ps_timid;	/* Timeout ID */
	ushort_t ps_timeout;	/* configured restart timeout value */
	uchar_t	ps_max_cfg;	/* Max-configure retry configured */
	uchar_t	ps_max_trm;	/* Max-Termination retry configured */
	uchar_t	ps_max_fail;	/* Max-cnf Nak w/o Ack */
	uchar_t	ps_retransmits;	/* cnf/trm requests counter */
	uint_t ps_badstate;	/* number of illegal states */
	ushort_t ps_reason;	/* failure flags */
	ushort_t ps_txcnt;		/* Number of packets sent */
	ushort_t ps_rxcnt;		/* Number of good packets received */
	ushort_t ps_rxbad;		/* # of pkts rx that were discarded */

} proto_state_t;

#define PR_MAXFAIL	0x0001	/* Max failures reached */
#define PR_MAXCFG	0x0002	/* Max configure reached */
#define PR_LTERM	0x0004	/* Local end terminated */
#define PR_PTERM	0x0008	/* Peer end terminated */
#define PR_REJ		0x0010	/* Protocol was rejected */

/*
 *	FSM states.
 */
#define INITIAL		0	/* Lower layer is unavailable */
#define STARTING	1	/* Open counterpart of INITIAL */
#define CLOSED		2	/* No connection, lower layer is available */
#define STOPPED		3	/* Open counterpart of CLOSED */
#define CLOSING		4	/* We’ve sent a Terminate Request */
#define STOPPING	5	/* Open counterpart to CLOSING */
#define REQSENT		6	/* We’ve sent a Configure Request */
#define ACKRCVD		7	/* We’ve received a Configure Ack */
#define ACKSENT		8	/* We’ve sent a Configure Ack */
#define OPENED		9	/* Connection open */
#define PPP_STATES	10	/* Total number of states defined */

/*
 * FSM events.
 */
#define	UP	0	/* lower layer is ready */
#define	DOWN	1	/* lower layer is down */
#define	OPEN	2	/* administrative link open indication */
#define	CLOSE	3	/* administrative link close indication */
#define	TO_P	4	/* timeout with retransmits counter > 0 */
#define	TO_M	5	/* timeout with retransmits counter == 0 */
#define	RCR_P	6	/* got good configure request */
#define	RCR_M	7	/* got bad configure request */
#define	RCA		8	/* got configure Ack */
#define	RCN		9	/* got configure Nak/Rej */
#define	RTR		10	/* got terminate request */
#define	RTA		11	/* got terminate Ack */
#define	RUC		12	/* got unknown code */
#define	RXJ_P	13	/* got acceptable reject */
#define	RXJ_M	14	/* got catastrophic reject */
#define	RXR		15	/* got echo/discard */
#define PPP_EVENTS 16	/* Total number of events defined */
#define DSCRD		17 /* Discard packet */

/*
 * This structure defines the actions that will be taken when an event
 * occurs.  Note that ctrl_act is no longer a bit map.
 */

struct	state_tbl_ent {
	void		(*snd_act)();	/* send out packets */
	void		(*gen_act)();	/* initialize counter, timer */
	uchar_t	ctrl_act;	/* index (+1) to routines provided by CP */
#	  define tlu	1	/* Call cp layer up routine *action[0]() */
#	  define tld	2	/* Call cp layer down routine *action[1]() */
#	  define tls	3	/* Call cp layer start routine *action[2]() */
#	  define tlf	4	/* Call cp layer finish routine *action[3]() */
#	  define restrt 5	/* Call cps restart routine *action[4]() */
#	  define cross  6	/* Call cps cross conn routine *action[5]() */
#	  define c_tld  7	/* Call cross and tld routines *action[6]() */
	ushort_t	next_state;	/* next state */
};

#endif /* _FSM_H */
