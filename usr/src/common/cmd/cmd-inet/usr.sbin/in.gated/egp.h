#ident	"@(#)egp.h	1.4"
#ident	"$Header$"

/*
 * Public Release 3
 * 
 * $Id$
 */

/*
 * ------------------------------------------------------------------------
 * 
 * Copyright (c) 1996, 1997 The Regents of the University of Michigan
 * All Rights Reserved
 *  
 * Royalty-free licenses to redistribute GateD Release
 * 3 in whole or in part may be obtained by writing to:
 * 
 * 	Merit GateDaemon Project
 * 	4251 Plymouth Road, Suite C
 * 	Ann Arbor, MI 48105
 *  
 * THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE REGENTS OF THE
 * UNIVERSITY OF MICHIGAN AND MERIT DO NOT WARRANT THAT THE
 * FUNCTIONS CONTAINED IN THE SOFTWARE WILL MEET LICENSEE'S REQUIREMENTS OR
 * THAT OPERATION WILL BE UNINTERRUPTED OR ERROR FREE. The Regents of the
 * University of Michigan and Merit shall not be liable for
 * any special, indirect, incidental or consequential damages with respect
 * to any claim by Licensee or any third party arising from use of the
 * software. GateDaemon was originated and developed through release 3.0
 * by Cornell University and its collaborators.
 * 
 * Please forward bug fixes, enhancements and questions to the
 * gated mailing list: gated-people@gated.merit.edu.
 * 
 * ------------------------------------------------------------------------
 * 
 * Copyright (c) 1990,1991,1992,1993,1994,1995 by Cornell University.
 *     All rights reserved.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT
 * LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * GateD is based on Kirton's EGP, UC Berkeley's routing
 * daemon	 (routed), and DCN's HELLO routing Protocol.
 * Development of GateD has been supported in part by the
 * National Science Foundation.
 * 
 * ------------------------------------------------------------------------
 * 
 * Portions of this software may fall under the following
 * copyrights:
 * 
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms are
 * permitted provided that the above copyright notice and
 * this paragraph are duplicated in all such forms and that
 * any documentation, advertising materials, and other
 * materials related to such distribution and use
 * acknowledge that the software was developed by the
 * University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote
 * products derived from this software without specific
 * prior written permission.  THIS SOFTWARE IS PROVIDED
 * ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */


#ifdef	PROTO_EGP
#include "egp_param.h"

/* For parser */
#define	EGP_LIMIT_DISTANCE	EGP_DISTANCE_MIN, EGP_DISTANCE_MAX

#define	EGP_LIMIT_PKTSIZE	1024, (u_int) MIN(IP_MAXPACKET, task_maxpacket)

#define	EGP_LIMIT_P1		EGP_P1, MAXHELLOINT
#define	EGP_LIMIT_P2		EGP_P2, MAXPOLLINT

#define EGP_N_POLLAGE	3		/* minimum number of poll intervals before a route is deleted when not updated */

/* Tracing */
#ifdef	DEBUG
#define	TR_EGP_DEBUG		TR_USER_6
#endif	/* DEBUG */
#define	TR_EGP_DETAIL_HELLO	TR_DETAIL_1	/* HELLO/I-H-U */
#define	TR_EGP_DETAIL_ACQUIRE	TR_DETAIL_2	/* ACQUIRE/CEASE */
#define	TR_EGP_DETAIL_UPDATE	TR_DETAIL_3	/* NR/POLL */

#define	TR_EGP_INDEX_PACKETS	0	/* All packets */
#define	TR_EGP_INDEX_HELLO	1	/* HELLO/I-H-U */
#define	TR_EGP_INDEX_ACQUIRE	2	/* Acquire/Cease */
#define	TR_EGP_INDEX_UPDATE	3	/* NR/POLL */

/*
 *	EGP Polling rate structure.
 */
struct egp_rate {
    time_t rate_min;
    time_t rate_last;
    time_t rate_window[RATE_WINDOW];
};


/*
 *	EGP stats definition
 */
struct egpstats_t {
    u_int inmsgs;
    u_int inerrors;
    u_int outmsgs;
    u_int outerrors;
    u_int inerrmsgs;
    u_int outerrmsgs;
    u_int stateups;
    u_int statedowns;
#ifdef	PROTO_SNMP
    int trigger;
#define	EGP_TRIGGER_START	1
#define	EGP_TRIGGER_STOP	2
    void_t trigger_save;
#endif	/* PROTO_SNMP */
};


/* Structure used for maintaining a sorted list of networks to be announced to a peer */

typedef struct _egp_route {
    struct _egp_route *route_forw;
    struct _egp_route *route_back;
    struct _egp_dist *route_dist;
    rt_entry	*route_rt;		/* Array of routes */
} egp_route ;

#define	EGP_ROUTE_LIST(rp, list)	for (rp = (list)->route_forw; rp != list; rp = rp->route_forw)
#define	EGP_ROUTE_LIST_END(rp, list)	

typedef struct _egp_dist {
    struct _egp_dist *dist_forw;
    struct _egp_dist *dist_back;
    struct _egp_gwentry *dist_gwentry;
    metric_t	dist_distance;		/* Distance for this list */
    int		dist_n_routes;		/* Count of routes */
    egp_route	dist_routes;		/* List of routes */
} egp_dist ;

#define	EGP_DIST_LIST(dp, list)		for (dp = (list)->dist_forw; dp != list; dp = dp->dist_forw)
#define	EGP_DIST_LIST_END(dp, list)	

typedef struct _egp_gwentry {
    struct _egp_gwentry *gwe_forw;
    struct _egp_gwentry *gwe_back;
    struct in_addr gwe_addr;		/* Address of this gateway */
    int		gwe_n_distances;	/* Count of distances */
    egp_dist	gwe_distances;		/* List of distances */
} egp_gwentry ;

#define	EGP_GW_LIST(gp, list)		for (gp = (list)->gwe_forw; gp != list; gp = gp->gwe_forw)
#define	EGP_GW_LIST_END(gp, list)

/*
 * Structure egpngh stores state information for an EGP neighbor. There is
 * one such structure allocated at initialization for each of the trusted EGP
 * neighbors read from the initialization file. The egpngh structures are in a
 * singly linked list pointed to by external variable "egp_neighbor_head".
 * The major states of a neighbor are IDLE, ACQUIRED, DOWN, UP and CEASE.
 */


typedef struct _egp_neighbor {
    struct _egp_neighbor *ng_forw;
    struct _egp_neighbor *ng_back;
    char ng_name[16];			/* Printable address of this neighbor */

    /* Status */
    int ng_state;			/* Current state of protocol */
    flag_t ng_flags;
    trace *ng_trace_options;		/* Trace flags from parser */
    u_int ng_status;			/* Info saved for cease retransmission */

    /* Variables */
    u_int ng_R;				/* receive sequence number */
    u_int ng_S;				/* send sequence number */
    time_t ng_T1;			/* interval between Hello command retransmissions */
    time_t ng_T2;			/* interval between Poll command retransmissions */
    time_t ng_T3;			/* interval during which neighbor-reachibility indications are counted */
    time_t ng_P1;			/* Minimum interval acceptable between successive Hello commands received */
    time_t ng_P2;			/* Minimum interval acceptable between successive Poll commands received */
    int ng_M;				/* hello polling mode */
    int ng_j;				/* neighbor-up threshold */
    int ng_k;				/* neighbor-down threshold */
    int ng_V;				/* EGP version we speak */

    /* Polling info */
    struct egp_rate ng_poll_rate;	/* Polling rate */
    struct egp_rate ng_hello_rate;	/* Hello rate */
    u_int ng_R_lastpoll;		/* sequence number of last poll */
    u_int ng_S_lasthello;		/* sequence number of last hello sent */
    int ng_noupdate;			/* # successive polls (new id) which did not receive a valid update */

    /* Addresses */
    sockaddr_un *ng_gateway;		/* Address of local gateway */
    if_addr_entry *ng_lcladdr;		/* Configured local address */
    if_addr *ng_ifap;			/* Pointer to Interface for sending packets */
    gw_entry ng_gw;			/* gw_entry for this peer */
#define	ng_addr		ng_gw.gw_addr
#define	ng_import	ng_gw.gw_import
#define	ng_export	ng_gw.gw_export
#define	ng_time		ng_gw.gw_time
#define	ng_peer_as	ng_gw.gw_peer_as
#define	ng_local_as	ng_gw.gw_local_as
#define	ng_task		ng_gw.gw_task
    task_timer *ng_timer_t1;
    task_timer *ng_timer_t2;
    task_timer *ng_timer_t3;

    sockaddr_un *ng_paddr;		/* Last address he polled */
    sockaddr_un *ng_saddr;		/* Address I should poll */

    /* Neighbor Reachability Info */
    int ng_responses;			/* Shift register of responses for determining
				        reachability, each set bit corresponds to a
				        response, each zero to no response */

    /* Configured info */
    flag_t ng_options;			/* Option flags */
    u_int ng_version;			/* Configuration specified version */
    metric_t ng_metricout;		/* Metric to use for all outgoing nets */
    pref_t ng_preference;		/* Preference */
    pref_t ng_preference2;		/* Alternate preference */
    int ng_ttl;				/* TTL */

    /* Max acquire info */
    struct _egp_neighbor *ng_gr_head;	/* Pointer to head of group */
    u_int ng_gr_acquire;		/* Maximum neighbors to acquire in this group */
    u_int ng_gr_number;			/* Number of neighbors in this group */
    u_int ng_gr_index;			/* Group number */

    /* Statistics */
    struct egpstats_t ng_stats;		/* Statistic structre */

    /* Network list */
    size_t ng_length;			/* Length of update */
    size_t ng_send_size;		/* Size of kernel buffer */
    egp_gwentry *ng_net_gw[2];		/* Interior/exterior Gateways and their distance lists */
} egp_neighbor ;

/* States */
#define NGS_IDLE		0
#define NGS_ACQUISITION		1
#define NGS_DOWN		2
#define NGS_UP			3
#define NGS_CEASE		4

/* flags */
#define	NGF_SENT_UNSOL		0x01	/* An unsolicited update has been sent since we were polled */
#define	NGF_SENT_POLL		0x02	/* A Poll has been sent, an update is outstanding */
#define	NGF_SENT_REPOLL		0x04	/* A rePoll has been sent, an update is outstanding */
#define	NGF_RECV_REPOLL		0x08	/* An rePoll has been received on this id */
#define	NGF_RECV_UNSOL		0x10	/* An unsolicited update has been received since we sent a poll */
#define	NGF_PROC_POLL		0x20	/* A Poll is being processed, don't send an unsolicited if we transition to Up */
#define	NGF_DELETE		0x40	/* Delete after re-init should be cleared by parser */
#define	NGF_WAIT		0x80	/* Waiting for deleted neighbor to start us */
#define	NGF_GENDEFAULT		0x100	/* Default generation has been requested */
#define	NGF_POLICY		0x200	/* Have policy */

/* Options */
#define NGO_METRICOUT		0x01	/* Use and outbound metric */
#define NGO_PEERAS		0x02	/* Verify inbound AS number */
#define NGO_LOCALAS		0x04	/* Use this outbound AS number */
#define NGO_NOGENDEFAULT	0x08	/* Don't consider this neighbor for default generation */
#define	NGO_LCLADDR		0x10	/* Local address was specified */
#define	NGO_SADDR		0x20	/* IP Source Network was specified */
#define	NGO_GATEWAY		0x40	/* Address of local gateway to Source Network */
#define	NGO_MAXACQUIRE		0x80	/* Maxacquire for this group */
#define	NGO_VERSION		0x0100	/* EGP version number to use initially */
#define	NGO_PREFERENCE		0x0200	/* Preference for this AS */
#define	NGO_PREFERENCE2		0x0400	/* Alternate preference for this peer */
#define	NGO_P1			0x0800	/* P1 was specified */
#define	NGO_P2			0x1000	/* P2 was specified */
#define	NGO_TTL			0x2000	/* TTL explicitly specified */
#define	NGO_DEFAULTIN		0x4000	/* Allow default via EGP */
#define	NGO_DEFAULTOUT		0x8000	/* Allow default via EGP */

/* Basic EGP packet */

typedef struct _egp_packet_header {
    u_int8 egp_ver;			/* Version # */
    u_int8 egp_type;			/* Opcode */
    u_int8 egp_code;
    u_int8 egp_status;
    u_int16 egp_chksum;
    u_int16 egp_system;			/* Autonomous system */
    u_int16 egp_id;
} egp_packet_header;


/* EGP neighbor acquisition packet */
typedef struct _egp_packet_acquire {
    egp_packet_header ea_pkt;
    u_int16 ea_hint;			/* Hello interval in seconds */
    u_int16 ea_pint;			/* NR poll interval in seconds */
} egp_packet_acquire;

/* EGP NR poll packet */
typedef struct _egp_packet_poll {
    egp_packet_header ep_pkt;
    u_int16 ep_unused;
    u_int32 ep_net;			/* Source net */
} egp_packet_poll;

/* EGP NR Message packet */
typedef struct _egp_packet_nr {
    egp_packet_header en_pkt;
    u_int8 en_igw;			/* No. internal gateways */
    u_int8 en_egw;			/* No. external gateways */
    u_int32 en_net;			/* shared net */
} egp_packet_nr;

#define NRMAXNETUNIT 9			/* maximum size per net in octets of net part of NR message */
/* EGP Error packet */
typedef struct _egp_packet_error {
    egp_packet_header ee_pkt;
    u_int16 ee_rsn;
    u_int8 ee_egphd[12];		/* First 12 bytes of bad egp pkt */
} egp_packet_error;


typedef union _egp_packet {
    egp_packet_header	header;
    egp_packet_acquire	acquire;
    egp_packet_poll	poll;
    egp_packet_nr	nr;
    egp_packet_error	error;
} egp_packet;

#define EGPVER	2
#define	EGPVERDEFAULT	2
#define	EGPVMASK	0x03		/* We speak version 2 and sometime version 3 */

/* EGP Types */
#define EGP_PKT_NR		1
#define	EGP_PKT_POLL		2
#define EGP_PKT_ACQUIRE		3
#define EGP_PKT_HELLO		5
#define	EGP_PKT_ERROR		8
#define	EGP_PKT_MAX		9

/* Neighbor Acquisition Codes */
#define EGP_CODE_ACQ_REQUEST		0	/* Neighbor acq. request */
#define EGP_CODE_ACQ_CONFIRM		1	/* Neighbor acq. confirmation */
#define EGP_CODE_ACQ_REFUSE		2	/* Neighbor acq. refuse */
#define EGP_CODE_CEASE			3	/* Neighbor cease */
#define EGP_CODE_CEASE_ACK		4	/* Neighbor cease ack */

/* Neighbor Acquisition Message Status Info */
#define EGP_STATUS_UNSPEC	0
#define	EGP_STATUS_ACTIVE	1
#define	EGP_STATUS_PASSIVE	2
#define	EGP_STATUS_NORESOURCES	3
#define	EGP_STATUS_ADMINPROHIB	4
#define	EGP_STATUS_GOINGDOWN	5
#define	EGP_STATUS_PARAMPROB	6
#define	EGP_STATUS_PROTOVIOL	7

/* Neighbor Hello Codes */
#define EGP_CODE_HELLO	0
#define EGP_CODE_HEARDU	1

/* Reachability, poll and update status */
#define EGP_STATUS_INDETERMINATE	0
#define EGP_STATUS_UP			1
#define EGP_STATUS_DOWN			2
#define EGP_STATUS_UNSOLICITED		128

/* Error reason status */
#define	EGP_REASON_UNSPEC	0
#define EGP_REASON_BADHEAD	1
#define	EGP_REASON_BADDATA	2
#define EGP_REASON_NOREACH	3
#define	EGP_REASON_XSPOLL	4
#define EGP_REASON_NORESPONSE	5
#define	EGP_REASON_UVERSION	6
#define	EGP_REASON_MAX		EGP_REASON_UVERSION


#define EGP_ERROR	-1
#define EGP_NOERROR	-2


extern trace *egp_trace_options;			/* Trace flags from parser */
extern const bits egp_trace_types[];			/* EGP specific trace flags */
extern struct egpstats_t egp_stats;

PROTOTYPE(egp_ngp_alloc,
	  extern egp_neighbor *,
	  (egp_neighbor *));
PROTOTYPE(egp_ngp_free,
	  extern void,
	  (egp_neighbor *));
PROTOTYPE(egp_init,
	  extern void,
	  (void));
PROTOTYPE(egp_var_init,
	  extern void,
	  (void));
PROTOTYPE(egp_dump,
	  extern void,
	  (task *, FILE *));
PROTOTYPE(egp_ngp_dump,
	  extern void,
	  (task *, FILE *));
PROTOTYPE(egp_recv,
	  extern void,
	  (task *));
PROTOTYPE(egp_event_t1,
	  extern void,
	  (task_timer *,
	   time_t));
PROTOTYPE(egp_event_t2,
	  extern void,
	  (task_timer *,
	   time_t));
PROTOTYPE(egp_event_t3,
	  extern void,
	  (task_timer *,
	   time_t));
PROTOTYPE(egp_event_stop,
	  extern void,
	  (egp_neighbor * ngp,
	   int status));
PROTOTYPE(egp_event_start,
	  extern void,
	  (task * tp));
PROTOTYPE(egp_ngp_idlecheck,
	  extern void,
	  (egp_neighbor * ngp));
PROTOTYPE(egp_rt_recv,
	  extern int,
	  (egp_neighbor * ngp,
	   egp_packet * pkt,
	   size_t egplen));
PROTOTYPE(egp_rt_send,
	  extern int,
	  (egp_neighbor * ngp,
	   egp_packet_nr * nrpkt));
PROTOTYPE(egp_rt_newaddr,
	  extern int,
	  (egp_neighbor *,
	   sockaddr_un *));
PROTOTYPE(egp_rt_policy,
	  extern void,
	  (egp_neighbor *,
	   rt_list *));
PROTOTYPE(egp_rt_newpolicy,
	  extern void,
	  (egp_neighbor *));
PROTOTYPE(egp_rt_dump,
	  extern void,
	  (FILE *fd,
	   egp_neighbor *));
PROTOTYPE(egp_rt_terminate,
	  extern void,
	  (egp_neighbor *));
PROTOTYPE(egp_rt_reinit,
	  extern void,
	  (egp_neighbor *));
PROTOTYPE(egp_neighbor_changed,
	  extern int,
	  (egp_neighbor * ngpo,
	   egp_neighbor * ngpn));
PROTOTYPE(egp_trap_neighbor_loss,
	  extern void,
	  (egp_neighbor *));
PROTOTYPE(egp_ngp_ifa_select,
	  extern if_addr *,
	  (egp_neighbor *));
PROTOTYPE(egp_ngp_ifa_bind,
	  extern int,
	  (egp_neighbor *,
	   if_addr *));

extern const bits egp_states[];
extern const bits egp_flags[];
extern const bits egp_options[];

extern u_int egp_reachability[];	/* Number of bits in a given state of
				           the reachability register */

extern const char *egp_acq_codes[];	/* Acquisition packet types */
extern const char *egp_reach_codes[];	/* Reachability codes */
extern const char *egp_nr_status[];	/* Network reachability states */
extern const char *egp_acq_status[];	/* Acquisition packet codes */
extern const char *egp_reasons[];	/* Error code reasons */

extern int egp_neighbors;		/* number of egp neighbors */
extern egp_neighbor egp_neighbor_head;	/* start of linked list of egp neighbor state tables */

#if	defined(PROTO_SNMP)
extern void egp_sort_neighbors();
#endif				/* defined(PROTO_SNMP) */

extern u_int egprid_h;			/* sequence number of received egp packet */
extern int doing_egp;			/* Are we running EGP protocols? */
extern size_t egp_pktsize;		/* Maximum packet size */
extern size_t egp_maxpacket;		/* Maximum packet size the system supports */
extern pref_t egp_preference;		/* Preference for EGP routes */
extern pref_t egp_preference2;		/* Alternate preference for EGP */
extern metric_t egp_default_metric;	/* default EGP metric */
extern adv_entry *egp_import_list;	/* List of EGP advise entries */
extern adv_entry *egp_export_list;	/* List of EGP export entries */

#define	EGP_LIST(ngp)	for (ngp = egp_neighbor_head.ng_forw; ngp != &egp_neighbor_head; ngp = ngp->ng_forw)
#define	EGP_LIST_END(ngp)

/* SNMP support */
#ifdef	PROTO_SNMP
PROTOTYPE(egp_mib_init,
	  extern void,
	  (int));
PROTOTYPE(egp_sort_neighbors,
	  extern void,
	  (egp_neighbor *));
#endif	/* PROTO_SNMP */


#endif	/* PROTO_EGP */
