#ident	"@(#)rt_table.h	1.4"
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
 * rt_table.h
 *
 * Routing table data and parameter definitions.
 *
 */

/* Target specific information */

PROTOTYPE(rttsi_get, extern void, (rt_head *, u_int, byte *));
PROTOTYPE(rttsi_set, extern void, (rt_head *, u_int, byte *));
PROTOTYPE(rttsi_reset, extern void, (rt_head *, u_int));

/* Macros to support a bit per protocol in the routing structure */

/* Don't change this value here, change it in the config file */
#ifndef	RTBIT_SIZE
#define	RTBIT_SIZE	1
#endif	/* RTBIT_SIZE */

#ifndef	NBBY
#define	RTBIT_NBBY	8
#else	/* NBBY */
#define	RTBIT_NBBY	NBBY
#endif	/* NBBY */

typedef u_int32 rtbit_mask;

#define	RTBIT_NB	(sizeof(rtbit_mask) * RTBIT_NBBY)	/* bits per mask */
#define	RTBIT_NBITS	(RTBIT_SIZE * RTBIT_NB)

#define	RTBIT_MASK(name)	rtbit_mask	name[RTBIT_SIZE]

#define	RTBIT_NSHIFT	5
#define	RTBIT_NBIT(x)	(0x01 << ((x) & (RTBIT_NB-1)))
#define	RTBIT_NBYTE(x)	((x) >> RTBIT_NSHIFT)

#define	RTBIT_SET(n, p)		BIT_SET((p)[RTBIT_NBYTE((n)-1)], RTBIT_NBIT((n)-1))
#define	RTBIT_CLR(n, p)		BIT_RESET((p)[RTBIT_NBYTE((n)-1)], RTBIT_NBIT((n)-1))
#define	RTBIT_ISSET(n, p)	BIT_TEST((p)[RTBIT_NBYTE((n)-1)], RTBIT_NBIT((n)-1))

#define	rtbit_isset(rt, bit)	RTBIT_ISSET(bit, (rt)->rt_bits)

/*
 * The number of multipath routes supported by the forwarding engine.
 */

#ifndef	RT_N_MULTIPATH
#define	RT_N_MULTIPATH	1
#endif	/* RT_N_MULTIPATH */

/* Structure used to indicate changes to a route */

typedef struct {
    flag_t	rtc_flags;
    short	rtc_n_gw;
    short	rtc_gw_sel;
    sockaddr_un	*rtc_routers[RT_N_MULTIPATH];
    struct _if_addr	*rtc_ifaps[RT_N_MULTIPATH];
    metric_t	rtc_metric;
    metric_t	rtc_metric2;
    metric_t	rtc_tag;
#ifdef	PROTO_ASPATHS
    as_path	*rtc_aspath;
#endif	/* PROTO_ASPATHS */
} rt_changes;

#define	RTCF_NEXTHOP	BIT(0x01)		/* Next hop change */
#define	RTCF_METRIC	BIT(0x02)		/* Metric change */
#define	RTCF_METRIC2	BIT(0x04)		/* Metric change */
#define	RTCF_ASPATH	BIT(0x08)		/* AS path change */
#define	RTCF_TAG	BIT(0x10)		/* Tag change */


/* Route aggregation structure */

typedef struct _rt_aggr_entry {
    struct _rt_aggr_entry *rta_forw;
    struct _rt_aggr_entry *rta_back;
    struct _rt_aggr_head *rta_head;		/* Head of the list */
    rt_entry *rta_rt;				/* Back pointer to this route */
    pref_t rta_preference;			/* Saved policy preference */
} rt_aggr_entry;

struct _rt_aggr_head {
    rt_aggr_entry rtah_rta;			/* Aggregate entry and head of list */
#define	rtah_rta_forw		rtah_rta.rta_forw
#define	rtah_rta_back		rtah_rta.rta_back
#define	rtah_rta_rt		rtah_rta.rta_rt
#define	rtah_rta_aggr_rt	rtah_rta.rta_aggr_rt
#define	rtah_rta_preference	rtah_rta.rta_preference
    flag_t	rtah_flags;
#define	RTAHF_BRIEF		BIT(0x01)	/* Generate atomic aggregate aspath */
#define	RTAHF_CHANGED		BIT(0x02)	/* The contributors changed */
#define	RTAHF_ASPCHANGED	BIT(0x04)	/* The AS path changed */
#define	RTAHF_ONLIST		BIT(0x08)	/* On our private list */
#define	RTAHF_GENERATE		BIT(0x10)	/* Generate a real route, not an aggregate */
#ifdef	PROTO_ASPATHS
    as_path_list *rtah_aplp;	/* AS Path list pointer */
#endif	/* PROTO_ASPATHS */
};

#define	AGGR_LIST(list, rta) \
	do { \
		register rt_aggr_entry *Xrta_next = ((rta) = (list))->rta_forw; \
		while (Xrta_next != (list)) { \
		    (rta) = Xrta_next; \
		    Xrta_next = Xrta_next->rta_forw;
#define	AGGR_LIST_END(rtq, rt)	} } while (0)

#define	ADVF_AGGR_BRIEF		ADVF_USER1
#define	ADVF_AGGR_GENERATE	ADVF_USER2	

/*
 *	Each rt_head entry contains a destination address and the root entry of
 *	a doubly linked lists of type rt_entry.  Each rt_entry contains
 *	information about how this occurance of a destination address was
 *	learned, next hop, ...
 *
 *	The rt_entry structure contains a pointer back to it's rt_head
 *	structure.
 */

/*
 *	Define link field as a macro.  These three fields must be in the same
 *	relative order in the rt_head and rt_entry structures.
 */
#define rt_link struct _rt_entry *rt_forw, *rt_back; struct _rt_head *rt_head

struct _rt_head {
    struct  _radix_node *rth_radix_node;	/* Tree glue and other values */
    sockaddr_un *rth_dest;			/* The destination */
    sockaddr_un *rth_dest_mask;			/* Subnet mask for this route */
    flag_t rth_state;				/* Global state bits */
    struct _rt_entry *rth_active;		/* Pointer to the active route */
    struct _rt_entry *rth_last_active;		/* Pointer to the previous	 active route */
    struct _rt_entry *rth_holddown;		/* Pointer to route in holddown */
    rt_aggr_entry *rth_aggregate;		/* Aggregate list entry */
    rt_changes *rth_changes;			/* Pointer to changes in active route */
    rt_link;					/* Routing table chain */
    struct _rt_tsi *rth_tsi;			/* Target specific information */
    byte rth_entries;				/* Number of routes for this destintation */
    byte rth_n_announce;			/* Number of routes with announce bits set */
    byte rth_aggregate_depth;			/* Depth of this destination as an aggregate */
};


/**/

struct _rt_entry {
    rt_link;					/* Chain and head pointers */
#define	rt_dest		rt_head->rth_dest	/* Route resides in rt_head */
#define	rt_dest_mask	rt_head->rth_dest_mask	/* Mask resides in rt_head */
#define	rt_active	rt_head->rth_active	/* Pointer to the active route */
#define	rt_holddown	rt_head->rth_holddown	/* Pointer to the route in holddown */
#define	rt_n_announce	rt_head->rth_n_announce
    short rt_n_gw;				/* Number of next hops */
    short rt_gw_sel;				/* Index to selected next hop */
    sockaddr_un *rt_routers[RT_N_MULTIPATH];	/* Next Hops */
    struct _if_addr *rt_ifaps[RT_N_MULTIPATH];	/* Interface to send said packets to */
    gw_entry *rt_gwp;				/* Gateway we learned this route from */
    metric_t rt_metric;				/* Interior metric of this route */
    metric_t rt_metric2;
    metric_t rt_tag;				/* Route tag */
    flag_t rt_state;				/* Gated flags for this route */
    rtq_entry rt_rtq;
#define	rt_time	rt_rtq.rtq_time		/* Time this route was last reset */
#define	rt_age(rt)	(time_sec - (rt)->rt_time)
    void_t rt_datas[2];				/* Protocol specific data */
#define	rt_data	rt_datas[0]
    RTBIT_MASK(rt_bits);			/* Announcement bits */
    u_char	rt_n_bitsset;			/* Count of bits set */
    pref_t rt_preference;			/* Preference for this route */
    pref_t rt_preference2;			/* 2nd preference for route */
#ifdef	PROTO_ASPATHS
    struct _as_path *rt_aspath;			/* AS path for this route */
#endif	/* PROTO_ASPATHS */
};

#if	RT_N_MULTIPATH > 1
#define	RT_ROUTER(rt)	((rt)->rt_routers[(rt)->rt_gw_sel])
#define	RT_IFAP(rt)	((rt)->rt_ifaps[(rt)->rt_gw_sel])
#define	RTC_ROUTER(rtc)	((rtc)->rtc_routers[(rtc)->rtc_gw_sel])
#define	RTC_IFAP(rtc)	((rtc)->rtc_ifaps[(rtc)->rtc_gw_sel])
#else	/* RT_N_MULTIPATH > 1 */
#define	RT_ROUTER(rt)	((rt)->rt_routers[0])
#define	RT_IFAP(rt)	((rt)->rt_ifaps[0])
#define	RTC_ROUTER(rtc)	((rtc)->rtc_routers[0])
#define	RTC_IFAP(rtc)	((rtc)->rtc_ifaps[0])
#endif	/* RT_N_MULTIPATH */

/*
 * "State" of routing table entry.
 */

#define	RTS_ELIGIBLE		BIT(0x0)		/* Route is eligible to become active */
#define	RTS_REMOTE		BIT(0x01)		/* route is for ``remote'' entity */
#define RTS_NOTINSTALL  	BIT(0x02)		/* don't install this route in kernel */
#define RTS_NOADVISE		BIT(0x04)		/* This route not to be advised */
#define RTS_INTERIOR    	BIT(0x08)		/* an interior route */
#define RTS_EXTERIOR    	BIT(0x10)		/* an exterior route */
#define	RTS_NETROUTE		(RTS_INTERIOR|RTS_EXTERIOR)
#define	RTS_PENDING		BIT(0x20)		/* Route is pending because of holddown on another route */
#define	RTS_DELETE		BIT(0x40)		/* Route is deleted */
#define	RTS_HIDDEN		BIT(0x80)		/* Route is present but not used because of policy */
#define	RTS_ACTIVE		BIT(0x0100)		/* Route is active */
#define	RTS_INITIAL		BIT(0x0200)		/* This route is being added */
#define	RTS_STATEMASK		(RTS_ACTIVE|RTS_DELETE|RTS_HIDDEN|RTS_INITIAL)
#define	RTS_RELEASE		BIT(0x0400)		/* This route is scheduled for release */
#define	RTS_FLASH		BIT(0x0800)		/* This route is scheduled for a flash */
#define	RTS_ONLIST		BIT(0x1000)		/* This route is on the flash list */
#define	RTS_RETAIN		BIT(0x2000)		/* This static route retained at shutdown */
#define	RTS_GROUP		BIT(0x4000)	/* This is a multicast group */
#define	RTS_GATEWAY		BIT(0x8000)	/* This is not an interface route */
#define	RTS_REJECT		BIT(0x010000)	/* Send unreachable when trying to use this route */
#define	RTS_STATIC		BIT(0x020000)	/* Added by route command */
#define	RTS_BLACKHOLE		BIT(0x040000)	/* Silently drop packets to this net */
#define	RTS_IFSUBNETMASK	BIT(0x080000)	/* Subnet mask derived from interface */


#define	RTPROTO_ANY		0	/* Matches any protocol */
#define RTPROTO_DIRECT		1	/* route is directly connected */
#define RTPROTO_KERNEL		2	/* route was installed in kernel when we started */
#define RTPROTO_REDIRECT	3	/* route was received via a redirect */
#define RTPROTO_DEFAULT		4	/* route is GATEWAY default */
#define	RTPROTO_OSPF		5	/* OSPF AS Internal routes */
#define	RTPROTO_OSPF_ASE	6	/* OSPF AS External routes */
#define	RTPROTO_IGRP		7	/* cisco IGRP */
#define RTPROTO_HELLO		8	/* DCN HELLO */
#define RTPROTO_RIP		9	/* Berkeley RIP */
#define	RTPROTO_BGP		10	/* Border gateway protocol */
#define RTPROTO_EGP		11	/* route was received via EGP */
#define	RTPROTO_STATIC		12	/* route is static */
#define	RTPROTO_SNMP		13	/* route was installed by SNMP */
#define	RTPROTO_IDPR		14	/* InterDomain Policy Routing */
#define	RTPROTO_ISIS		15	/* IS-IS */
#define	RTPROTO_SLSP		16	/* Simple Link State Protocol */
#define	RTPROTO_IDRP		17	/* InterDomain Routing Protocol */
#define	RTPROTO_INET		18	/* For INET specific stuff */
#define	RTPROTO_IGMP		19	/* For IGMP stuff */
#define	RTPROTO_AGGREGATE	20	/* Aggregate route */
#define	RTPROTO_DVMRP		21	/* Distance Vector Multicast Routing Protocol */
#define	RTPROTO_RDISC		22	/* Router Discovery */
#define	RTPROTO_MAX		23	/* The number of protocols allocated */

#define	RTPROTO_BIT(proto)	((flag_t) (1 << ((proto) - 1)))
#define	RTPROTO_BIT_ANY		((flag_t) -1)

/*
 *	Preferences of the various route types
 */
#define	RTPREF_KERNEL_TEMP	0	/* For managing the forwarding table */
#define	RTPREF_DIRECT		0	/* Routes to interfaces */
#define	RTPREF_OSPF		10	/* OSPF Internal route */
#define	RTPREF_ISIS_L1		15	/* IS-IS level 1 route */
#define	RTPREF_ISIS_L2		18	/* IS-IS level 2 route */
#define	RTPREF_SLSP		19	/* NSFnet backbone SPF */
#define	RTPREF_DEFAULT		20	/* defaultgateway and EGP default */
#define	RTPREF_REDIRECT		30	/* redirects */
#define	RTPREF_KERNEL		40	/* learned via route socket */
#define	RTPREF_SNMP		50	/* route installed by network management */
#define	RTPREF_RDISC		55	/* Router Discovery */
#define	RTPREF_STATIC		60	/* Static routes */
#define	RTPREF_IGRP		80	/* Cisco IGRP */
#define	RTPREF_HELLO		90	/* DCN Hello */
#define	RTPREF_RIP		100	/* Berkeley RIP */
#define	RTPREF_DIRECT_AGGREGATE	110	/* P2P interface aggregate routes */
#define	RTPREF_DIRECT_DOWN	120	/* Routes to interfaces that are down */
#define	RTPREF_AGGREGATE	130	/* Aggregate default preference */
#define	RTPREF_OSPF_ASE		150	/* OSPF External route */
#define	RTPREF_IDPR		160	/* InterDomain Policy Routing */
#define	RTPREF_BGP_EXT		170	/* Border Gateway Protocol - external peer */
#define	RTPREF_EGP		200	/* Exterior Gateway Protocol */
#define	RTPREF_KERNEL_REMNANT	254	/* Routes in kernel at startup */


/**/

/* Structure with route parameters passed to rt_add. */

typedef struct {
    sockaddr_un *rtp_dest;
    sockaddr_un *rtp_dest_mask;
    int		rtp_n_gw;
    sockaddr_un *rtp_routers[RT_N_MULTIPATH];
#define	rtp_router	rtp_routers[0]
    gw_entry	*rtp_gwp;
    metric_t	rtp_metric;
    metric_t	rtp_metric2;
    metric_t	rtp_tag;
    flag_t	rtp_state;
    pref_t	rtp_preference;
    pref_t	rtp_preference2;
    void_t	rtp_rtd;
#ifdef	PROTO_ASPATHS
    as_path	*rtp_asp;
#endif	/* PROTO_ASPATHS */
} rt_parms;

#define	RTPARMS_INIT(n_gw, metric, state, preference) \
    { \
	(sockaddr_un *) 0, \
	(sockaddr_un *) 0, \
	n_gw, \
	{ (sockaddr_un *) 0 }, \
	(gw_entry *) 0, \
	metric, \
	(metric_t) 0, \
	(metric_t) 0, \
	state, \
	preference \
    }
											     

/* Macros to access the routing table - when the table format changes I */
/* just change these and everything works, right?  */

/*
 *	Change lists
 */

#define	RTL_SIZE	task_pagesize	/* Size of change list (one page) */

struct _rt_list {
    struct _rt_list *rtl_next;			/* Pointer to next on chain */
    struct _rt_list *rtl_root;			/* Pointer to root of list */
    void_t *rtl_fillp;				/* Pointer to last filled location */
    u_int rtl_count;				/* Number of entries on this list */	
    void_t rtl_entries[1];			/* Pointers to routes */
};


extern block_t rtlist_block_index;

/* Reset a list */
#define	RTLIST_RESET(rtl) \
	if (rtl) (rtl) = (rtl)->rtl_root; \
	    while (rtl) { \
		register rt_list *Xrtln = (rtl)->rtl_next; \
		task_block_free(rtlist_block_index, (void_t) (rtl)); \
		(rtl) = Xrtln; \
	    }

/* Scan a change list */
#define	RT_LIST(rth, rtl, type) \
    if (rtl) { \
	rt_list *rtl_root = (rtl)->rtl_root; \
	do { \
	    register void_t *Xrthp; \
	    for (Xrthp = (void_t *) (rtl)->rtl_entries; Xrthp <= (rtl)->rtl_fillp; Xrthp++) \
		if ((rth = (type *) *Xrthp))

#define	RT_LIST_END(rth, rtl, type) \
	    rth = (type *) 0; \
	} while (((rtl) = (rtl)->rtl_next)) ; \
	(rtl) = rtl_root; \
    }

/* Add an entry to a change list */
#define RTLIST_ADD(rtl, data) \
	do { \
	    register void_t Xdata = (void_t) (data); \
	    if (!(rtl)) { \
		(rtl) = (rt_list *) task_block_alloc(rtlist_block_index); \
		(rtl)->rtl_root = (rtl); \
		(rtl)->rtl_fillp = (rtl)->rtl_entries; \
		*(rtl)->rtl_fillp = Xdata; \
		(rtl)->rtl_root->rtl_count++; \
	    } else if ((rtl)->rtl_fillp < (rtl)->rtl_entries || Xdata != *(rtl)->rtl_fillp) { \
	        if (!(rtl) || (caddr_t) ++(rtl)->rtl_fillp == (caddr_t) (rtl) + RTL_SIZE) { \
		    (rtl)->rtl_fillp--; \
		    (rtl)->rtl_next = (rt_list *) task_block_alloc(rtlist_block_index); \
		    (rtl)->rtl_next->rtl_root = (rtl)->rtl_root; \
		    (rtl) = (rtl)->rtl_next; \
		    (rtl)->rtl_fillp = (rtl)->rtl_entries; \
	        } \
	        *(rtl)->rtl_fillp = Xdata; \
	        (rtl)->rtl_root->rtl_count++; \
	    } \
	} while (0)

/* Scan all routes for this destination */
#define	RT_ALLRT(rt, rth)	{ for (rt = (rth)->rt_forw; rt != (rt_entry *) &(rth)->rt_forw; rt = rt->rt_forw)
#define	RT_ALLRT_END(rt, rth)	if (rt == (rt_entry *) &(rth)->rt_forw) rt = (rt_entry *) 0; }

/* Scan all routes for this destination, in reverse order */
#define	RT_ALLRT_REV(rt, rth)	{ for (rt = (rth)->rt_back; rt != (rt_entry *) &(rth)->rt_forw; rt = rt->rt_back)
#define	RT_ALLRT_REV_END(rt, rth)	if (rt == (rt_entry *) &(rth)->rt_forw) rt = (rt_entry *) 0; }

/* Only route in use for this destination */
#define	RT_IFRT(rt, rth)	if ((rt = rth->rth_holddown) || (rt = rth->rth_active))
#define	RT_IFRT_END

/*
 *	Aggregate lists
 */

#ifdef	PROTO_INET
extern adv_entry *aggregate_list_inet;	/* Aggregation policy */
#endif	/* PROTO_INET */
#ifdef	PROTO_ISO
extern adv_entry *aggregate_list_iso;	/* Aggregation policy */
#endif	/* PROTO_ISO */

/*  Macro implementation of rt_refresh.  If this is not defined, a	*/
/*  function will be used.						*/
#define	rt_refresh(rt) \
	do { \
	    register rt_entry *Xrt = (rt); \
	     if (Xrt->rt_rtq.rtq_forw) { \
		REMQUE(&Xrt->rt_rtq); \
		INSQUE(&Xrt->rt_rtq, Xrt->rt_gwp->gw_rtq.rtq_back); \
	     } \
	     Xrt->rt_time = time_sec; \
	} while (0)

extern const bits rt_state_bits[];	/* Route state bits */
extern const bits rt_proto_bits[];	/* Protocol types */
extern struct _task *rt_task;
extern gw_entry *rt_gw_list;		/* List of gateways for static routes */
extern gw_entry *rt_gwp;		/* Gateway structure for static routes */


PROTOTYPE(rt_family_init,
	  extern void,
	  (void));		/* Initialize routing table )*/
PROTOTYPE(rt_init_mib,
	  extern void,
	  (int));
PROTOTYPE(rt_open,
	  extern void,
	  (task * tp));		/* Open routing table for updating )*/
PROTOTYPE(rt_close,
	  extern void,
	  (task * tp,
	   gw_entry * gwp,
	   int changes,
	   char *message));	/* Signal completion of updates */
PROTOTYPE(rt_add,
	  extern rt_entry *,
	  (rt_parms *));
#ifdef	PROTO_ASPATHS
PROTOTYPE(rt_change_aspath,
	   extern rt_entry *,
	   (rt_entry *,
	    metric_t,
	    metric_t,
	    metric_t,
	    pref_t,
	    pref_t,
	    int,
	    sockaddr_un **,
	    as_path *));
#define	rt_change(rt, m, m2, t, p, p2, n_gw, r)	rt_change_aspath(rt, m, m2, t, p, p2, n_gw, r, rt->rt_aspath)
#else	/* PROTO_ASPATHS */
PROTOTYPE(rt_change,
	   extern rt_entry *,
	   (rt_entry *,
	    metric_t,
	    metric_t,
	    metric_t,
	    pref_t,
	    pref_t,
	    int,
	    sockaddr_un **));
#define	rt_change_aspath(rt, m, m2, t, p, p2, n_gw, r, a)	rt_change(rt, m, m2, t, p, p2, n_gw, r)
#endif	/* PROTO_ASPATHS */
/* Delete a route from the routing table */
PROTOTYPE(rt_delete,
	  extern void,
	  (rt_entry *));
PROTOTYPE(rt_flash_update,
	  extern void,
	  (task_job *));
PROTOTYPE(rt_new_policy,
	  extern void,
	  (void));
/* Lookup a route the way the kernel would */
PROTOTYPE(rt_lookup,
	  extern rt_entry *,
	  (flag_t,
	   flag_t,
	   sockaddr_un *dst,
	   flag_t));
/* Locate a route given dst, table and proto */
PROTOTYPE(rt_locate,
	  extern rt_entry *,
	  (flag_t,
	   sockaddr_un *,
	   sockaddr_un *,
	   flag_t));
/* Locate a route given dst, table, proto and gwp */
PROTOTYPE(rt_locate_gw,
	  extern rt_entry *,
	  (flag_t,
	   sockaddr_un *,
	   sockaddr_un *,
	   gw_entry *));
/* Allocate a bit */
PROTOTYPE(rtbit_alloc,
	  extern u_int,
	  (task *tp,
	   int,
	   u_int,
	   void_t,
	   _PROTOTYPE(dump,
		      void,
		      (FILE *,
		       rt_head *,
		       void_t,
		       const char *))));
/* Release any routes and free the bit */
PROTOTYPE(rtbit_reset_all,
	  extern void,
	  (task *,
	   u_int,
	   gw_entry *));
PROTOTYPE(rtbit_free,
	  extern void,
	  (task *,
	   u_int));
/* Get a list of active routes */
PROTOTYPE(rthlist_active,
	  extern rt_list *,
	  (int));
/* Get a list of all routes */
PROTOTYPE(rtlist_all,
	  extern rt_list *,
	  (int));
/* Get a list of all routes */
PROTOTYPE(rthlist_all,
	  extern rt_list *,
	  (int));
PROTOTYPE(rthlist_match,
	  extern rt_list *,
	  (sockaddr_un *));
/* Set an announcement bit */
PROTOTYPE(rtbit_set,
	  extern void,
	  (rt_entry *,
	   u_int));
/* Reset and announcement bit */
PROTOTYPE(rtbit_reset,
	  extern rt_entry *,
	  (rt_entry *,
	   u_int));
/* Reset and announcement bit */
PROTOTYPE(rtbit_reset_pending,
	  extern rt_entry *,
	  (rt_entry *,
	   u_int));
PROTOTYPE(rt_static_init_family,
	  extern void,
	  (int));
/* Create a table for family */
PROTOTYPE(rt_table_init_family,
	  extern void,
	  (int af));
PROTOTYPE(rt_table_locate,
	  extern rt_head *,
	  (sockaddr_un *dst,
	   sockaddr_un *mask));
PROTOTYPE(aggregate_dump,
	  extern void,
	  (FILE *));
#ifdef	PROTO_ASPATHS
PROTOTYPE(rt_parse_route_aspath,
	  extern int,
	  (sockaddr_un *,
	   sockaddr_un *,
	   adv_entry *,
	   adv_entry *,
	   pref_t,
	   flag_t,
	   as_path *,
	   char *));
#define	rt_parse_route(d, m, g, i, p, s, e)	rt_parse_route_aspath(d, m, g, i, p, s, (as_path *) 0, e)
#else	/* PROTO_ASPATHS */
PROTOTYPE(rt_parse_route,
	  extern int,
	  (sockaddr_un *,
	   sockaddr_un *,
	   adv_entry *,
	   adv_entry *,
	   pref_t,
	   flag_t,
	   char *));
#endif	/* PROTO_ASPATHS */
#ifdef	PROTO_SNMP
PROTOTYPE(rt_table_getnext,
	  extern rt_entry *,
	  (sockaddr_un *,
	   sockaddr_un *,
	   int,
	   _PROTOTYPE(job,
		      rt_entry *,
		      (rt_head *,
		       void_t)),
	   void_t));
PROTOTYPE(rt_table_get,
	  extern rt_entry *,
	  (sockaddr_un *,
	   sockaddr_un *,
	   _PROTOTYPE(job,
		      rt_entry *,
		      (rt_head *,
		       void_t)),
	   void_t));
#endif	/* PROTO_SNMP */


extern int rt_default_active;		/* TRUE if gateway default is active */
extern int rt_default_needed;		/* TRUE if gateway default is needed */
extern rt_parms rt_default_rtparms;

/* Delete pseudo default at termination time */
#define	rt_default_reset()	rt_default_active = 1; (void) rt_default_delete()

PROTOTYPE(rt_default_add,		/* Request installation of gateway default */
	  extern void,
	  (void));
PROTOTYPE(rt_default_delete,		/* Request deletion of gateway default */
	  extern void,
	  (void));



/* Redirects */
#define	REDIRECT_CONFIG_IN	1
#define	REDIRECT_CONFIG_MAX	2

extern trace *redirect_trace_options;		/* Trace flags from parser */
extern int redirect_n_trusted;			/* Number of trusted ICMP gateways */
extern pref_t redirect_preference;		/* Preference for ICMP redirects */
extern gw_entry *redirect_gw_list;		/* List of learned and defined ICMP gateways */
extern adv_entry *redirect_import_list;		/* List of routes that we can import */
extern adv_entry *redirect_int_policy;		/* List of interface policy */
extern gw_entry *redirect_gwp;			/* Gateway pointer for redirect routes */
extern const bits redirect_trace_types[];	/* Redirect specific trace types */
PROTOTYPE(redirect,
	  extern void,
	  (sockaddr_un *,
	   sockaddr_un *,
	   sockaddr_un *,
	   sockaddr_un *));	       	/* Process a redirect */
PROTOTYPE(redirect_init,
	  extern void,
	  (void));
PROTOTYPE(redirect_var_init,
	  extern void,
	  (void));
PROTOTYPE(redirect_disable,
	  extern void,
	  (proto_t));
PROTOTYPE(redirect_enable,
	  extern void,
	  (proto_t));
PROTOTYPE(redirect_delete_router,
	  extern void,
	  (rt_list *));
