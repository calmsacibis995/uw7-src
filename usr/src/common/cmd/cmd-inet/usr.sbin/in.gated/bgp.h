#ident	"@(#)bgp.h	1.4"
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


/*
 *	Gated implementation related BGP definitions
 */

/*
 * Send and receive limits
 */
#ifdef	BGPMAXPACKETSIZE
#define	BGPHMAXPACKETSIZE	BGPMAXPACKETSIZE
#else	/* BGPMAXPACKETSIZE */
#define	BGPHMAXPACKETSIZE	4096		/* Just compile it in */
#endif	/* BGPMAXPACKETSIZE */

#define	BGPMAXSENDPKTSIZE	1024	/* Largest we try to send (<= 4096) */
#define	BGPMAXV4SENDPKTSIZE	(BGPHMAXPACKETSIZE - sizeof(bgpOutBuffer))
#define	BGPRECVBUFSIZE		BGPHMAXPACKETSIZE	/* >= 4096 */

/*
 * Grot for parser
 */
#ifndef	BGP_KNOWN_VERSION
#define	BGP_KNOWN_VERSION(version)	((version) >= 2 && (version) <= 4)
#endif	/* BGP_KNOWN_VERSION */

/*
 * Hold times.  We advertise a hold time of 180 seconds and decline to
 * accept a hold time of less than 20 seconds (as this would have us
 * sending a keepalive every 6 seconds) unless we've been configured
 * for this.
 */
#define	BGP_HOLDTIME		180	/* What we advertise */
#define	BGP_MIN_HOLDTIME	20	/* What we'll accept */
#define	BGP_REAL_MIN_HOLDTIME	6	/* What we'll allow configured */
#define	BGP_TIME_ERROR		5	/* We are about this accurate in time */

/* Tracing */

#define	TR_BGP_DETAIL_OPEN	TR_DETAIL_1
#define	TR_BGP_DETAIL_UPDATE	TR_DETAIL_2
#define	TR_BGP_DETAIL_KEEPALIVE	TR_DETAIL_3

#define	TR_BGP_INDEX_PACKETS	0
#define	TR_BGP_INDEX_OPEN	1
#define	TR_BGP_INDEX_UPDATE	2
#define	TR_BGP_INDEX_KEEPALIVE	3

#define	TR_BGP_ASPATH		TR_USER_1

extern const flag_t bgp_trace_masks[];
extern const bits bgp_trace_types[];	/* BGP specific trace flags */


/* For parser */
#define	BGP_METRIC_SIZE		2
#define	BGP_METRIC_MAX		65535
#define	BGP_METRIC_MIN		0
#define	BGP_METRIC_NONE		((metric_t)(-1))
#define	BGP_LIMIT_METRIC	BGP_METRIC_MIN,BGP_METRIC_MAX

#define	BGP_HOLDTIME_MIN	0
#define	BGP_HOLDTIME_MAX	65535

#define	BGP_LIMIT_HOLDTIME	BGP_HOLDTIME_MIN, BGP_HOLDTIME_MAX

#define	BGP_LIMIT_KBUF		BGPHMAXPACKETSIZE, (metric_t) -1
#define	BGP_LIMIT_SBUF		0, (metric_t) -1

/*
 * The length of a BGP peer name
 */
#define	BGPPEERNAMELENGTH	40
#define	BGPPEERTASKNAMELENGTH	24
#define	BGPGROUPNAMELENGTH	48
#define	BGPGROUPTASKNAMELENGTH	28

#define	BGPSYNCTASKNAMELENGTH	20

/*
 * This shitty macro determines if a peer sorts less than another
 * peer for SNMP purposes.  Sort by remote address, then remote AS,
 * then local AS.
 */
#define	BGPSORT_ADDR(addr)	ntohl(sock2ip(addr))
#define	BGPSORT_LT(p1, addr, p2) \
    (((addr) < BGPSORT_ADDR((p2)->bgp_addr)) ? TRUE : \
      (((addr) > BGPSORT_ADDR((p2)->bgp_addr)) ? FALSE : \
	(((p1)->bgp_peer_as < (p2)->bgp_peer_as) ? TRUE : \
	  (((p1)->bgp_peer_as > (p2)->bgp_peer_as) ? FALSE : \
	    (((p1)->bgp_local_as < (p2)->bgp_local_as) ? TRUE : FALSE)))))


/*
 * BGP buffer management structure
 */
typedef struct _bgpBuffer {
    byte *bgpb_buffer;		/* Receive buffer for incoming pkts */
    byte *bgpb_bufpos;		/* Start of unconsumed chars in buf */
    byte *bgpb_readptr;		/* Pointer past end of data in buffer */
    byte *bgpb_endbuf;		/* Pointer to end of buffer */
} bgpBuffer;

/*
 * BGP outgoing buffer management structure.
 */
typedef struct _bgpOutBuffer {
    byte *bgpob_start;		/* Start of write data */
    byte *bgpob_end;		/* End of write data */
    byte *bgpob_clearto;	/* Clear buffer to here */
    flag_t bgpob_flags;		/* Indicates whether full message spooled */
} bgpOutBuffer;

#define	BGPOBF_FULL_MESSAGE	BIT(0x1)

/*
 * BGP authentication structure (nothing for now)
 */
typedef struct _bgpAuthinfo {
    int bgpa_type;			/* type of authentication */
} bgpAuthinfo;

/*
 *	BGP configuration structure
 *
 * This structure is imbedded in the peer and group structures and
 * contains information learned from parsing the config file.
 */
struct bgp_conf {
    flag_t	bgpc_options;
    trace	*bgpc_trace_options;
    sockaddr_un	*bgpc_gateway;
    if_addr_entry	*bgpc_lcladdr;
    time_t	bgpc_holdtime_out;
    metric_t	bgpc_metric_out;
    pref_t	bgpc_preference;
    pref_t	bgpc_preference2;
    as_t	bgpc_local_as;
    bgpAuthinfo	bgpc_authinfo;
#define	bgpc_authtype	bgpc_authinfo.bgpa_type
    u_int	bgpc_conf_version;
    size_t	bgpc_recv_bufsize;
    size_t	bgpc_send_bufsize;
    time_t	bgpc_rti_time;
    time_t	bgpc_rto_time;
    u_int	bgpc_setpref;
    int		bgpc_ttl;
} ;


/*
 *  BGP statistics structure
 *
 * This contains message and octet counters used to keep track of traffic
 */
struct bgp_stats {
    u_long	bgps_in_updates;	/* incoming update messages */
    u_long	bgps_out_updates;	/* outgoing update messages */
    u_long	bgps_in_notupdates;	/* incoming other than updates */
    u_long	bgps_out_notupdates;	/* outgoing other than updates */
    u_long	bgps_in_octets;		/* incoming total octets */
    u_long	bgps_out_octets;	/* outgoing total octets */
};

/*
 * Some types needed for bit manipulation
 */
typedef u_long bgp_bits;
typedef u_int32 bgp_nexthop;


/*
 * Metrics for BGP.  We store (to be) advertised metrics and nexthops in
 * a structure like this, and maintain a single copy of each distinct set
 * of metrics/nexthop, to make comparisons easy.
 */
typedef u_int32 bvalue_t;	/* should be pvalue_t */
typedef byte btype_t;		/* should be ptype_t */

typedef struct _bgp_metrics {
    bgp_nexthop bgpm_nexthop;
    bvalue_t bgpm_metric;
    bvalue_t bgpm_localpref;
    bvalue_t bgpm_tag;
    union {
	bvalue_t bgpm_MA_alltypes;
	struct {
	    btype_t bgpm_MT_metric;
	    btype_t bgpm_MT_localpref;
	    btype_t bgpm_MT_tag;
	    btype_t bgpm_MT_unused;
	} bgpm_MA_types;
    } bgpm_M_types;
#define	bgpm_tmetric	bgpm_M_types.bgpm_MA_types.bgpm_MT_metric
#define	bgpm_tlocalpref	bgpm_M_types.bgpm_MA_types.bgpm_MT_localpref
#define	bgpm_ttag	bgpm_M_types.bgpm_MA_types.bgpm_MT_tag
#define	bgpm_types	bgpm_M_types.bgpm_MA_alltypes
} bgp_metrics;

#define	BGPM_HAS_VALUE	(0x80)


/*
 * BGP metrics node structure.  BGP metrics are sorted into a patricia
 * tree, the node structure contains both the metrics and the associated
 * bookkeeping data.  This data type is actually private to bgp_rt.c,
 * but is defined here to keep it in front of its use.
 */
typedef struct _bgp_metrics_node {
    bgp_metrics bgpmn_metrics;
    u_int32 bgpmn_refcount;		/* references to node */
    struct _bgp_metrics_node *bgpmn_left;
    struct _bgp_metrics_node *bgpmn_right;
    u_int bgpmn_bit;
} bgp_metrics_node;


/*
 * BGP advertised route list.  This contains a pointer to a
 * route we have our bit set on, and the metrics the route
 * was advertised with (if any).
 */
typedef struct _bgp_adv_queue {
    struct _bgp_adv_queue *bgpv_next;
    struct _bgp_adv_queue *bgpv_prev;
} bgp_adv_queue;

typedef struct _bgp_adv_entry {
    bgp_adv_queue bgpe_q_entry;
#define	bgpe_next	bgpe_q_entry.bgpv_next
#define	bgpe_prev	bgpe_q_entry.bgpv_prev
    rt_entry *bgpe_rt;
    bgp_metrics *bgpe_metrics;
    flag_t bgpe_flags;
} bgp_adv_entry;

/*
 * Flags we know about.  Only 2 currently.
 */
#define	BGPEF_V4_ONLY	BIT(0x1)
#define	BGPEF_QUEUED	BIT(0x2)


/*
 * BGP incoming route queue list.
 *
 * When configured to do so we only believe changes to routes if they
 * have been stable for some number of seconds, though we believe
 * deletes immediately when they occur.  This provides some defense
 * against neighbours gone crazy, since we'll tend not to use their
 * routes.
 */
typedef struct _bgp_rti_entry {
    struct _bgp_rti_entry *bgpi_next;		/* next entry in chain */
    struct _bgp_rti_entry *bgpi_prev;		/* previous entry in chain */
    struct _rt_entry *bgpi_old_rt;		/* old route he told us */
    struct _rt_entry *bgpi_new_rt;		/* current route he told us */
    time_t bgpi_time;				/* time to install new route */
    pref_t bgpi_preference;			/* preference for new route */
} bgp_rti_entry;


/*
 * BGP outgoing route queue list for external peers
 */
typedef struct _bgp_rto_entry {
    struct _bgp_rto_entry *bgpo_next;		/* next entry in chain */
    struct _bgp_rto_entry *bgpo_prev;		/* previous entry in chain */
    time_t bgpo_time;				/* time to send new route */
    bgp_adv_entry *bgpo_advrt;
#define	bgpo_rt			bgpo_advrt->bgpe_rt
#define	bgpo_new_metrics	bgpo_advrt->bgpe_metrics
    bgp_metrics *bgpo_metrics;			/* metric/nh sent previously */
    as_path *bgpo_asp;				/* AS path sent previously */
} bgp_rto_entry;


/*
 * BGP old route info, used for internal/igp/routing/test peer group
 * outgoing lists
 */
typedef struct _bgpg_rtinfo_entry {
    struct _bgpg_rtinfo_entry *bgp_info_next;	/* next info struct in chain */
    bgp_metrics *bgp_info_metrics;		/* metrics last sent */
    as_path *bgp_info_asp;			/* AS path last sent */
    bgp_bits bgp_info_bits[1];			/* peer bits, expandable */
} bgpg_rtinfo_entry;


/*
 * BGP outgoing route queue list for internal/igp/test peer groups
 */
typedef struct _bgpg_rto_entry {
    struct _bgpg_rto_entry *bgpgo_next;		/* next entry in chain */
    struct _bgpg_rto_entry *bgpgo_prev;		/* previous entry in chain */
    time_t bgpgo_time;				/* time to send new route */
    bgp_adv_entry *bgpgo_advrt;			/* pointer to adv route entry */
#define	bgpgo_rt	bgpgo_advrt->bgpe_rt
#define	bgpgo_metrics	bgpgo_advrt->bgpe_metrics
    bgpg_rtinfo_entry *bgpgo_info;		/* old route info */
} bgpg_rto_entry;


/*
 * Queue structure.  This heads the list of to-be-sent routes
 * in bgp peer and group structures.
 */
typedef struct _bgp_rt_queue {
    struct _bgp_rt_queue *bgpq_next;	/* start of list */
    struct _bgp_rt_queue *bgpq_prev;	/* end of list */
    union {
	struct _bgp_asp_list **bgpq_Q_asp_hash;	/* hash list for asp sort */
	as_path *bgpq_Q_asp;		/* AS path pointer */
    } bgpq_Q_union;
#define	bgpq_asp_hash	bgpq_Q_union.bgpq_Q_asp_hash
#define	bgpq_asp	bgpq_Q_union.bgpq_Q_asp
    int bgpq_asp_hash_check;
} bgp_rt_queue;
    
/*
 * BGP AS path list pointer.  Routes in the list are sorted by outgoing
 * AS path to make collection of common path attributes simpler when
 * actually announcing the routes.  This structure heads each list.
 */
typedef struct _bgp_asp_list {
    union {
        bgp_rto_entry *bgpl_X2_rto_next;
	bgpg_rto_entry *bgpl_X2_grto_next;
    } bgpl_X2;
#define	bgpl_rto_next	bgpl_X2.bgpl_X2_rto_next
#define	bgpl_grto_next	bgpl_X2.bgpl_X2_grto_next
    union {
        bgp_rto_entry *bgpl_X1_rto_prev;
	bgpg_rto_entry *bgpl_X1_grto_prev;
    } bgpl_X1;
#define	bgpl_rto_prev	bgpl_X1.bgpl_X1_rto_prev
#define	bgpl_grto_prev	bgpl_X1.bgpl_X1_grto_prev
    bgp_rt_queue bgpl_asp_queue;
#define	bgpl_q_next	bgpl_asp_queue.bgpq_next
#define	bgpl_q_prev	bgpl_asp_queue.bgpq_prev
#define	bgpl_asp	bgpl_asp_queue.bgpq_asp
#define	bgpl_asp_hash_check	bgpl_asp_queue.bgpq_asp_hash_check
} bgp_asp_list;



/*
 * BGP group ifap list.  BGP groups with peers which care about
 * interfaces maintain a list of the interfaces which are interesting,
 * for which we maintain sensible next hops.  These are added to and
 * deleted from as the peers come and go.  They are used to decide how
 * to group announcements with a common next hop.
 */
typedef struct _bgp_ifap_list {
    struct _bgp_ifap_list *bgp_if_next;
    if_addr *bgp_if_ifap;
    union {
	u_int32 bgp_if_X_refcount;
	metric_t bgp_if_X_metric;
    } bgp_if_X;
#define	bgp_if_refcount	bgp_if_X.bgp_if_X_refcount
#define	bgp_if_metric	bgp_if_X.bgp_if_X_metric
} bgp_ifap_list;


/*
 * Support for BGP internal routing synchronization with an IGP.  The
 * scheme here is track both IGP routes and IBGP routes, building a
 * radix trie which contains the IGP routes and the IBGP next hops.
 * This way a next hop for an IBGP route may be found by finding
 * the IBGP next hop in the routing table, then walking back up
 * the trie to the first node with an IGP route attached.  The
 * latter contributes the next hops.
 */

/*
 * BGP synchronization next hops.  We keep a private next hop
 * structure so we don't lose changes, and to make installation
 * convenient.
 */
typedef struct _bsy_nexthop {
    struct _bsy_nexthop *bsynh_next;	/* next in hash chain */
    u_long bsynh_refcount;		/* reference count for this nh set */
    u_short bsynh_hash;			/* hash value for next hop */
    u_short bsynh_n_gw;			/* number of next hops */
    bgp_nexthop bsynh_nexthop[RT_N_MULTIPATH];
} bsy_nexthop;

/*
 * We hash the next hops we're using to speed lookups.  The hashing
 * is actually done on the next hops in the rt_entry, so we assume
 * this is what we're given to compute the hash.
 */
#define	BSYNH_HASH_SIZE		57

#if	RT_N_MULTIPATH == 1
#define	BSYNH_HASH(n_gw, gws, hash) \
    ((hash) = sock2ip(*(gws)) % BSYNH_HASH_SIZE)
#else	/* RT_N_MULTIPATH == 1 */
#define	BSYNH_HASH(n_gw, gws, hash) \
    do { \
	register int Xn_gw = (n_gw); \
	register sockaddr_un **Xgws = (gws); \
	register u_int32 Xtmp = 0; \
	do { \
	    Xtmp ^= sock2ip(*Xgws); \
	    Xgws++; \
	} while ((--Xn_gw) > 0); \
	(hash) = Xtmp % BSYNH_HASH_SIZE; \
    } while (0)
#endif	/* RT_N_MULTIPATH == 1 */

/*
 * This is an internal node in a BGP synchronization radix trie.  There
 * are pointers to both an IGP route and an IBGP next hop.
 */
typedef struct _bsy_rt_internal {
    struct _bsy_rt_internal *bsyi_left;	/* child when bit is zero */
    struct _bsy_rt_internal *bsyi_right;	/* child when bit is one */
    struct _bsy_rt_internal *bsyi_parent;	/* parent of this node */
    u_int32 bsyi_bit;			/* bit for this node */
    u_int32 bsyi_dest;			/* next hop/dest for this node */
    struct _bsy_nh_entry *bsyi_nexthop;	/* next hop node, if any */
    struct _bsy_igp_rt *bsyi_igp_rt;	/* IGP route, if any */
} bsy_rt_internal;

/*
 * This structure is used to keep track of IGP routes we're seen.
 * It has a pointer to the current next hop structure for this
 * route, and a private copy of the route's metric.
 */
typedef struct _bsy_igp_rt {
    bsy_rt_internal *bsy_igp_rti;	/* This entry's internal node */
    rt_entry *bsy_igp_rt;		/* The IGP route in question */
    u_int32 bsy_igp_metric;		/* IGP metric for this route */
    bsy_nexthop *bsy_igp_nexthop;	/* Current IGP next hops */
} bsy_igp_rt;

/*
 * This is used to keep track of IBGP routes whose next hop/preference
 * we are managing.
 */
typedef struct _bsy_ibgp_rt {
    struct _bsy_ibgp_rt *bsyb_next;	/* Circular list chain pointers */
    struct _bsy_ibgp_rt *bsyb_prev;
    struct _bsy_nh_entry *bsyb_nh;	/* Point to nh entry we're with */
    rt_entry *bsyb_rt;			/* Route this refers to */
    pref_t bsyb_pref;			/* Route's (natural) preference */
    flag_t bsyb_flags;			/* Flags for route */
} bsy_ibgp_rt;

#define	BSY_NEXTHOP(brt) \
    (htonl((brt)->bsyb_nh->bsyn_ibgp_rti->bsyi_dest))

/*
 * Flags for optimizing certain operations.  We keep track of routes
 * for which there are known to be alternates, and mark the current
 * active route.
 */
#define	BSYBF_ACTIVE	0x1		/* This route in use */
#define	BSYBF_ALT	0x2		/* Route has one or more alternates */

/*
 * This structure keeps track of next hops which arrived in IBGP routes,
 * and the IBGP routes which possessed these next hops.  It also keeps
 * track of IGP metrics, since these are used for equal preference
 * tie breaking.
 */
typedef struct _bsy_nh_entry {
    bsy_ibgp_rt *bsyn_next;		/* Circular list pointer for routes */
    bsy_ibgp_rt *bsyn_prev;
    bsy_rt_internal *bsyn_ibgp_rti;	/* Internal node we're attached to */
    bsy_nexthop *bsyn_igp_nexthop;	/* Current next hop(s) for route */
    u_int32 bsyn_igp_metric;		/* Current IGP metric for route */
    struct _bsy_nh_entry *bsyn_change_next;	/* Changed list pointer */
} bsy_nh_entry;

/*
 * The BGP synchronization structure.  This is the structure by which
 * the synchronization instance is referred to external to the module.
 * It holds the local task pointer, a pointer to the root of our radix
 * trie, the group that we are running with and an indicator of the IGP
 * we are running with.
 */
typedef struct _bgp_sync {
    task *bgp_sync_task;		/* Our task (holds our rtbit) */
    bsy_rt_internal *bgp_sync_trie;	/* Root of our radix trie */
    bsy_nh_entry *bgp_sync_nh_changes;	/* List of IBGP routes with changes */
    proto_t bgp_sync_proto;		/* The IGP we are sync'ing to */
    struct _bgpPeerGroup *bgp_sync_group;	/* Our group */
    u_int bgp_sync_n_hashed;		/* Number of hashed next hops */
    bsy_nexthop *bgp_sync_nh_hash[BSYNH_HASH_SIZE];	/* Next hop hash list */
    char bgp_sync_name[BGPSYNCTASKNAMELENGTH];	/* Name for task */
} bgp_sync;


/*
 *  BGP peer structure
 *
 * A note on input.  The maximum BGP packet size is 4096.  To receive a
 * packet this large each peer must have an input buffer of that size
 * allocated to it (sigh).  We try to keep system call overhead down
 * by always trying to fill the 4096 byte buffer on each read.  We then
 * process each complete message in the buffer in batch fashion.  Any
 * incomplete message fragment is then copied to the start of the
 * buffer (message fragments should occur infrequently) and attempt
 * to fill the remainder of the buffer again.  We entirely empty the
 * socket before closing the routing table and returning, as this
 * provides good processing efficiency when routing traffic is heavy.
 */
typedef struct _bgpPeer {
    struct _bgpPeer *bgp_next;		/* Pointer to next bgpPeer struct */
    struct _bgpPeer *bgp_sort_next;	/* Pointer to next sorted bgpPeer */
    struct _bgpPeerGroup *bgp_group;	/* Back pointer to group header */

    flag_t bgp_flags;			/* Protocol Flags */

    struct bgp_conf	bgp_conf;	/* Configuration information */
#define	bgp_options		bgp_conf.bgpc_options
#define	bgp_conf_version	bgp_conf.bgpc_conf_version
#define	bgp_authtype		bgp_conf.bgpc_authtype
#define	bgp_authinfo		bgp_conf.bgpc_authinfo
#define	bgp_trace_options      	bgp_conf.bgpc_trace_options
#define	bgp_gateway		bgp_conf.bgpc_gateway
#define	bgp_lcladdr		bgp_conf.bgpc_lcladdr
#define	bgp_holdtime_out	bgp_conf.bgpc_holdtime_out
#define	bgp_metric_out		bgp_conf.bgpc_metric_out
#define	bgp_conf_local_as	bgp_conf.bgpc_local_as
#define	bgp_preference		bgp_conf.bgpc_preference
#define	bgp_preference2		bgp_conf.bgpc_preference2
#define	bgp_recv_bufsize	bgp_conf.bgpc_recv_bufsize
#define	bgp_send_bufsize	bgp_conf.bgpc_send_bufsize
#define	bgp_rti_time		bgp_conf.bgpc_rti_time
#define	bgp_rto_time		bgp_conf.bgpc_rto_time
#define	bgp_setpref		bgp_conf.bgpc_setpref
#define	bgp_ttl			bgp_conf.bgpc_ttl

#define	bgp_type		bgp_group->bgpg_type

    u_int bgp_version;			/* Actual version in use */
    u_int bgp_hisversion;		/* Version he likes, if not ours */

    char bgp_name[BGPPEERNAMELENGTH];	/* Name of this peer */
    char bgp_task_name[BGPPEERTASKNAMELENGTH];	/* Name for peer's task */

    gw_entry bgp_gw;			/* GW block for this peer */
#define	bgp_proto	bgp_gw.gw_proto
#define	bgp_import	bgp_gw.gw_import
#define	bgp_export	bgp_gw.gw_export
#define	bgp_local_as	bgp_gw.gw_local_as
#define	bgp_peer_as	bgp_gw.gw_peer_as
#define	bgp_last_rcvd	bgp_gw.gw_time
#define	bgp_task	bgp_gw.gw_task
#define	bgp_addr	bgp_gw.gw_addr

#define	bgp_rtbit	bgp_gw.gw_task->task_rtbit

    u_int32 bgp_id;			/* BGP ID this peer told us */
    u_int32 bgp_out_id;			/* BGP ID we sent to peer */

    sockaddr_un *bgp_myaddr;		/* local address we talk to him via */
    if_addr *bgp_ifap;			/* the local interface for the peer */
    int bgp_ttl_current;		/* the ttl being used */

    /* Peer timer pointers */
    task_timer *bgp_connect_timer;	/* time to reattempt connection */
    task_timer *bgp_traffic_timer;	/* holdtime/keepalive timer */
    task_timer *bgp_route_timer;	/* incoming/outgoing route timer */

    /* Information related to holdtime/keepalive timing */
    time_t bgp_holdtime;		/* hold time we settled on */
    time_t bgp_last_sent;		/* last time we sent anything */
    time_t bgp_last_checked;		/* last time we checked status */
    time_t bgp_traffic_interval;	/* current interval we're using */
    time_t bgp_last_keepalive;		/* last time we sent a keepalive */

    /* Information about current and previous states */
    byte bgp_state;			/* Protocol State */
    byte bgp_laststate;			/* previous protocol state */
    byte bgp_lastevent;			/* last event */

    byte bgp_lasterror[2];		/* last error with this peer */
#define	bgp_last_code		bgp_lasterror[0]
#define	bgp_last_subcode	bgp_lasterror[1]

    /* Event counters and times */
    u_int bgp_connect_failed;		/* number of times connect has failed */
    u_short bgp_connect_slot;		/* slot number for connection */
    u_short bgp_index;			/* peer index in group */

    /* Traffic counters */
    struct bgp_stats		bgp_stats;
#define	bgp_in_updates		bgp_stats.bgps_in_updates
#define	bgp_out_updates		bgp_stats.bgps_out_updates
#define	bgp_in_notupdates	bgp_stats.bgps_in_notupdates
#define	bgp_out_notupdates	bgp_stats.bgps_out_notupdates
#define	bgp_in_octets		bgp_stats.bgps_in_octets
#define	bgp_out_octets		bgp_stats.bgps_out_octets

    /* Received packet buffer */
    bgpBuffer bgp_inbuf;
#define	bgp_buffer	bgp_inbuf.bgpb_buffer
#define	bgp_bufpos	bgp_inbuf.bgpb_bufpos
#define	bgp_readptr	bgp_inbuf.bgpb_readptr
#define	bgp_endbuf	bgp_inbuf.bgpb_endbuf

    /* Group bit */
    u_int bgp_group_bit;	/* Set in group route bit masks */

    /* Spooled write data */
    bgpOutBuffer *bgp_outbuf;	/* only non-NULL when data spooled */

    /* Peer incoming route queue */
    bgp_rti_entry *bgp_rti_next;
    bgp_rti_entry *bgp_rti_prev;

    /* Peer advertised route queue (external peers only) */
    bgp_adv_queue bgp_queue;

    /* Peer outgoing route queue (external peers only) */
    bgp_rt_queue bgp_asp_queue;
#define	bgp_asp_first		bgp_asp_queue.bgpq_next
#define	bgp_asp_last		bgp_asp_queue.bgpq_prev
#define	bgp_asp_hash		bgp_asp_queue.bgpq_asp_hash

    /* Route timers */
    time_t bgp_rto_next_time;		/* external peers only */
} bgpPeer;



/*
 * Group bits are kept in a structure designed to minimize memory
 * if we have fewer than 32 group peers.
 */
typedef union _bgpg_bitset {
	bgp_bits	bgp_gr_bits;		/* for bgpg_idx_size == 1 */
	bgp_bits	*bgp_gr_bitptr;		/* for bgpg_idx_size > 1 */
} bgpg_bitset;


/*
 * BGP peer group structure.  BGP peers are sorted into groups, where
 * the peers in a single group generally receive a single set of
 * advertisements (i.e. implement identical policy).  The group structures
 * are chained together, with each heading a list of peers.
 */
typedef struct _bgpPeerGroup {
    struct _bgpPeerGroup *bgpg_next;	/* chain pointer for groups */
    struct _bgpPeer *bgpg_peers;	/* pointer to head of list of peers */
    char	bgpg_name[BGPGROUPNAMELENGTH];	/* group name */
    char	bgpg_task_name[BGPGROUPTASKNAMELENGTH];
    u_int	bgpg_type;		/* internal|external|test */
    as_t	bgpg_peer_as;		/* AS this group operates in */
    u_short	bgpg_asbit;		/* AS bit for the local AS */
    flag_t	bgpg_flags;		/* Group peers */
    adv_entry	*bgpg_allow;		/* Popup - peers list */
    task	*bgpg_task;		/* Task for rtbit, internal only */
    u_int32	bgpg_out_id;		/* Outgoing ID for this group */
    u_int	bgpg_igp_rtbit;		/* IGP's rtbit */
    proto_t	bgpg_igp_proto;		/* IGP's protocol */
    u_int	bgpg_n_peers;		/* number of peers in group */
    u_short	bgpg_n_established;	/* number of established peers */
    u_short	bgpg_n_v4_established;	/* number of established v4 peers */
    proto_t	bgpg_proto;		/* protocol for internal routing */
    struct bgp_conf bgpg_conf;		/* Configured information */
#define	bgpg_options		bgpg_conf.bgpc_options
#define	bgpg_conf_version	bgpg_conf.bgpc_conf_version
#define	bgpg_authtype		bgpg_conf.bgpc_authtype
#define	bgpg_authinfo		bgpg_conf.bgpc_authinfo
#define	bgpg_trace_options	bgpg_conf.bgpc_trace_options
#define	bgpg_gateway		bgpg_conf.bgpc_gateway
#define	bgpg_lcladdr		bgpg_conf.bgpc_lcladdr
#define	bgpg_holdtime_out	bgpg_conf.bgpc_holdtime_out
#define	bgpg_metric_out		bgpg_conf.bgpc_metric_out
#define	bgpg_local_as		bgpg_conf.bgpc_local_as
#define	bgpg_preference		bgpg_conf.bgpc_preference
#define	bgpg_preference2	bgpg_conf.bgpc_preference2
#define	bgpg_recv_bufsize	bgpg_conf.bgpc_recv_bufsize
#define	bgpg_send_bufsize	bgpg_conf.bgpc_send_bufsize
#define	bgpg_rti_time		bgpg_conf.bgpc_rti_time
#define	bgpg_rto_time		bgpg_conf.bgpc_rto_time
#define	bgpg_setpref		bgpg_conf.bgpc_setpref
#define	bgpg_ttl		bgpg_conf.bgpc_ttl

    /* Group index/bitmask allocation data */
    u_short	bgpg_idx_maxalloc;	/* highest bit number allocated */
    u_short	bgpg_idx_size;		/* size of bit array (in u_long's) */
    block_t	bgpg_oinfo_blk;		/* for allocating oinfo structures */
    u_short	bgpg_n_v3_bits;		/* number of v3 bits set */
    u_short	bgpg_n_v4_bits;		/* number of v4 bits set */
    u_short	bgpg_n_v3_sync;		/* number of v3 sync bits set */
    u_short	bgpg_n_v4_sync;		/* number of v4 sync bits set */
    bgpg_bitset	bgpg_v3_bits;		/* version 3 peer bits */
    bgpg_bitset	bgpg_v4_bits;		/* version 4 peer bits */
    bgpg_bitset	bgpg_v3_sync;		/* synchronized version 3 peers */
    bgpg_bitset	bgpg_v4_sync;		/* synchronized version 4 peers */

    /* Interface list.  List of those interfaces we care about.  */
    bgp_ifap_list *bgpg_ifap_list;
    adv_entry *bgpg_ifap_policy;

    /* BGP synchronization structure (internal routing peers only) */
    bgp_sync *bgpg_sync;

    /* Group advertised route queue (non-external peers only) */
    bgp_adv_queue bgpg_queue;

    /* Group outgoing route queue (non-external groups only) */
    bgp_rt_queue bgpg_asp_queue;
#define	bgpg_asp_first		bgpg_asp_queue.bgpq_next
#define	bgpg_asp_last		bgpg_asp_queue.bgpq_prev
#define	bgpg_asp_hash		bgpg_asp_queue.bgpq_asp_hash

    /* Group outgoing route timer (non-external groups only) */
    task_timer *bgpg_route_timer;

    /* Route timer (non-external groups only) */
    time_t bgpg_rto_next_time;
} bgpPeerGroup;

/*
 * Types of peer groups
 */
#define	BGPG_EXTERNAL		0	/* external peer group */
#define	BGPG_INTERNAL		1	/* internal peer group */
#define	BGPG_INTERNAL_IGP	2	/* internal peer group running IGP */
#define	BGPG_INTERNAL_RT	3	/* internal routing peer */
#define	BGPG_TEST		4	/* anonymous watcher */

/*
 *	Group flags
 */
#define	BGPGF_DELETE		0x01	/* Group is being deleted */
#define	BGPGF_IDLED		0x02	/* Group is temporarily idled */
#define	BGPGF_RT_TIMER		0x04	/* Route timer running */

/*
 * When we are connected to by a remote server we create a proto-peer
 * structure and new task.  This bit of goo holds us until we receive
 * an open packet and can figure out what to do.
 */
typedef struct _bgpProtoPeer {
    struct _bgpProtoPeer *bgpp_next;	/* pointer to next in chain */
    task *bgpp_task;			/* task we're using for this */
#define	bgpp_addr	bgpp_task->task_addr
    char bgpp_name[BGPPEERNAMELENGTH];	/* name for this peer */
    sockaddr_un *bgpp_myaddr;		/* local address peer connected to */
    time_t bgpp_connect_time;		/* time connect was done */
    task_timer *bgpp_timeout_timer;	/* timer for connection timeout */
    int bgpp_ttl_current;		/* the current ttl */
    bgpBuffer bgpp_inbuf;		/* received packet buffer */
#define	bgpp_buffer	bgpp_inbuf.bgpb_buffer
#define	bgpp_bufpos	bgpp_inbuf.bgpb_bufpos
#define	bgpp_readptr	bgpp_inbuf.bgpb_readptr
#define	bgpp_endbuf	bgpp_inbuf.bgpb_endbuf
} bgpProtoPeer;


/* Flags */
#define	BGPF_DELETE		0x01	/* Delete this peer */
#define	BGPF_UNCONFIGURED	0x02	/* This is an unconfigured peer */
#define	BGPF_TRY_CONNECT	0x04	/* Time to try another connect */
#define	BGPF_WRITEFAILED	0x08	/* Attempt to write this peer failed */
#define	BGPF_IDLED		0x10	/* This peer is permanently idled */
#define	BGPF_GENDEFAULT		0x20	/* Requested default generation */
#define	BGPF_RT_TIMER		0x40	/* Route timer running */
#define	BGPF_SEND_RTN		0x80	/* task_send() routine running */
#define	BGPF_CLEANUP		0x0100	/* Cleanup scheduled for peer */
#define	BGPF_INITIALIZING	0x0200	/* This peer is initializing */

/* Options */
#define BGPO_METRIC_OUT		0x01	/* Use an outbound metric */
#define BGPO_LOCAL_AS		0x02	/* Use this outbound AS number */
#define BGPO_NOGENDEFAULT	0x04	/* Don't consider this peer for default generation */
#define	BGPO_GATEWAY		0x08	/* Address of local gateway to Source Network */
#define	BGPO_PREFERENCE		0x10	/* Preference for this AS */
#define	BGPO_LCLADDR		0x20	/* Our local address was specified */
#define	BGPO_HOLDTIME		0x40	/* Holdtime was specified */
#define	BGPO_PASSIVE		0x80	/* Don't actively try to connect */
#define	BGPO_VERSION		0x0100	/* Version number specified */
#define	BGPO_SETPREF		0x0200	/* Set preference from/to localpref */
#define	BGPO_KEEPALL		0x0400	/* Keep all routes from peer */
#define	BGPO_KEEPNONE		0x0800	/* Keep no routes from peer */
#define	BGPO_ANALRETENTIVE	0x1000	/* Log if we get something stupid */
#define	BGPO_NOAGGRID		0x2000	/* Don't include BGP ID in aggregator */
#define	BGPO_NOAUTHCHECK	0x4000	/* Don't check authentication */
#define	BGPO_KEEPALIVESALWAYS	0x8000	/* Always send keepalives */
#define	BGPO_PREFERENCE2	0x010000	/* Alternate preference set */
#define	BGPO_V3ASLOOPOKAY	0x020000	/* Okay to send AS loop to v3 */
#define	BGPO_NOV4ASLOOP		0x040000	/* Don't send AS loop to v4 */
#define	BGPO_TTL		0x100000	/* Explicit TTL specified */
#define	BGPO_LOGUPDOWN		0x200000	/* Syslog up/down transitions */
#define BGPO_MED                0x400000        /* Propagate MEDs for this AS */
#define BGPO_IGNOREFIRSTASHOP   0x400000 /* allow routes from route servers that don't prepend
 own ashop */

/* Group options */
#define BGPGO_METRIC_OUT	0x01	/* Use an outbound metric */
#define BGPGO_ASOUT		0x02	/* Use this outbound AS number */
#define BGPGO_NOGENDEFAULT	0x04	/* don't use group peers for default generation */
#define	BGPGO_GATEWAY		0x08	/* Address of local gateway to Source Network */
#define	BGPGO_PREFERENCE	0x10	/* Preference for this AS */
#define	BGPGO_LCLADDR		0x20	/* Our local address was specified */
#define	BGPGO_HOLDTIME		0x40	/* Holdtime was specified */
#define	BGPGO_PASSIVE		0x80	/* Don't actively try to connect */
#define	BGPGO_VERSION		0x0100	/* Version number specified */
#define	BGPGO_SETPREF		0x0200	/* Set preference from/to localpref */
#define	BGPGO_KEEPALL		0x0400	/* Keep all routes from peer */
#define	BGPGO_KEEPNONE		0x0800	/* Keep no routes from peer */
#define	BGPGO_ANALRETENTIVE	0x1000	/* Log if we get something stupid */
#define	BGPGO_NOAGGRID		0x2000	/* Don't include BGP ID in aggregator */
#define	BGPGO_NOAUTHCHECK	0x4000	/* Don't check authentication */
#define	BGPGO_KEEPALIVESALWAYS	0x8000	/* Always send keepalives */
#define	BGPGO_PREFERENCE2	0x010000	/* Alternate preference set */
#define	BGPGO_V3ASLOOPOKAY	0x020000	/* Okay to send AS loop to v3 */
#define	BGPGO_NOV4ASLOOP	0x040000	/* Don't send AS loop to v4 */
#define	BGPGO_TTL		0x100000	/* Explicit TTL specified */
#define	BGPGO_LOGUPDOWN		0x200000	/* Syslog up/down transitions */
#define BGPO_IGNOREFIRSTASHOP   0x400000 /* allow routes from route servers that don't prepend
                                            their own ashop */


/*
 * Variables in bgp_init.c
 */
extern int doing_bgp;			/* Are we running BGP protocols? */
extern trace *bgp_default_trace_options; /* Trace options from parser */
extern pref_t bgp_default_preference;	/* Preference for BGP routes */
extern pref_t bgp_default_preference2;	/* Alternate preference for BGP */
extern metric_t bgp_default_metric;	/* Default BGP metric to use */

extern adv_entry *bgp_import_list;	/* List of BGP advise entries */
extern adv_entry *bgp_import_aspath_list;	/* List of BGP import aspath policy */
extern adv_entry *bgp_export_list;	/* List of BGP export entries */


#define	BGP_PEER_LIST(bgp, bnp) \
	do { \
	    register bgpPeer *Xbnp = (bgp)->bgpg_peers; \
	    while (((bnp) = Xbnp)) { \
	        Xbnp = (bnp)->bgp_next;
#define	BGP_PEER_LIST_END(bgp, bnp) \
	    } \
	} while (0)


#define	BGP_GROUP_LIST(bgp)	for (bgp = bgp_groups; bgp; bgp = bgp->bgpg_next)
#define	BGP_GROUP_LIST_END(bgp)

/*
 * Routines in bgp_init.c
 */
PROTOTYPE(bgp_conf_check,
	  extern int,
	  (char *));
PROTOTYPE(bgp_conf_group_alloc, extern bgpPeerGroup *, (void));
PROTOTYPE(bgp_conf_group_add,
	  extern bgpPeerGroup *,
	  (bgpPeerGroup *,
	   char *));
PROTOTYPE(bgp_conf_group_check,
	  extern int,
	  (bgpPeerGroup *,
	   char *));
PROTOTYPE(bgp_conf_peer_alloc,
	  extern bgpPeer *,
	  (bgpPeerGroup *));
PROTOTYPE(bgp_conf_peer_add,
	  extern bgpPeer *,
	 (bgpPeerGroup *,
	  bgpPeer *,
	  char *));
PROTOTYPE(bgp_var_init,
	  extern void,
	  (void));
PROTOTYPE(bgp_init,
	  extern void,
	  (void));
