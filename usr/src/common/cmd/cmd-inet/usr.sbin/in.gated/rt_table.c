#ident	"@(#)rt_table.c	1.4"
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


#define	INCLUDE_RT_VAR

#include "include.h"
#ifdef	PROTO_INET
#include "inet.h"
#endif	/* PROTO_INET */
#ifdef	PROTO_ISO
#include "iso.h"
#endif	/* PROTO_ISO */
#include "krt.h"

static const bits rt_change_bits[] =
{
    { RTCF_NEXTHOP,	"NextHop" },
    { RTCF_METRIC,	"Metric" },
    { RTCF_TAG,		"Tag" },
    { RTCF_ASPATH,	"ASPath" },
    {0}
};

const bits rt_state_bits[] =
{
    {RTS_REMOTE,		"Remote"},
    {RTS_NOTINSTALL,		"NotInstall"},
    {RTS_NOADVISE,		"NoAdvise"},
    {RTS_INTERIOR,		"Int"},
    {RTS_EXTERIOR,		"Ext"},
    {RTS_PENDING,		"Pending"},
    {RTS_DELETE,		"Delete"},
    {RTS_HIDDEN,		"Hidden"},
    {RTS_ACTIVE,		"Active"},
    {RTS_INITIAL,		"Initial"},
    {RTS_RELEASE,		"Release"},
    {RTS_FLASH,			"Flash"},
    {RTS_ONLIST,		"OnList"},
    {RTS_RETAIN,		"Retain"},
    {RTS_GROUP,			"Group"},
    {RTS_GATEWAY,		"Gateway"},
    {RTS_REJECT,		"Reject"},
    {RTS_STATIC,		"Static"},
    {RTS_BLACKHOLE,		"Blackhole"},
    {RTS_IFSUBNETMASK,		"IfSubnetMask"},
    {0}
};

const bits rt_proto_bits[] = {
    {RTPROTO_ANY,	"Any" },
    {RTPROTO_DIRECT,	"Direct"},
    {RTPROTO_KERNEL,	"Kernel"},
    {RTPROTO_REDIRECT,	"Redirect"},
    {RTPROTO_DEFAULT,	"Default"},
    {RTPROTO_OSPF,	"OSPF"},
    {RTPROTO_OSPF_ASE,	"OSPF_ASE"},
    {RTPROTO_IGRP,	"IGRP"},
    {RTPROTO_HELLO,	"HELLO"},
    {RTPROTO_RIP,	"RIP"},
    {RTPROTO_BGP,	"BGP"},
    {RTPROTO_EGP,	"EGP"},
    {RTPROTO_STATIC,	"Static"},
    {RTPROTO_SNMP,	"MIB"},
    {RTPROTO_IDPR,	"IDPR"},
    {RTPROTO_ISIS,	"IS-IS"},
    {RTPROTO_SLSP,	"SLSP"},
    {RTPROTO_IDRP,	"IDRP"},
    {RTPROTO_INET,	"INET"},
    {RTPROTO_IGMP,	"IGMP"},
    {RTPROTO_AGGREGATE,	"Aggregate"},
    {RTPROTO_DVMRP,	"DVMRP"},
    {RTPROTO_RDISC,	"RDISC"},
    {0}
};

task *rt_task = (task *) 0;

struct rtaf_info rtaf_info[AF_MAX] = { { 0 } };

static task *rt_opentask = (task *) 0;	/* Protocol that has table open */
static task_job *rt_flash_job = (task_job *) 0;

static block_t rt_block_index = (block_t) 0;		/* Block allocation index for an rt_entry */
static block_t rth_block_index = (block_t) 0;
static block_t rtchange_block_index = (block_t) 0;

#define	rt_check_open(p, name)	assert(rt_opentask)


/**/
/* Byte allocation table */
static rtbit_info rtbit_map[RTBIT_NBITS];
typedef u_short	rtbit_type;
/* Bit allocation map */
static rtbit_type rttsi_map[RTBIT_NBITS * MAX(sizeof(u_long), sizeof(u_long *))/RTTSI_SIZE];

static block_t rttsi_block_index;

/* Allocate a tsi field */
static void
rttsi_alloc  __PF1(ip, rtbit_info *)
{
    u_int i;
    rtbit_type mask0 = 0;

    /* Verify that the map size is the same as tsi blocks */
    assert(sizeof (rtbit_type) * NBBY == RTTSI_SIZE);

    /* Generate the mask we are looking for */
    for (i = 1; i <= ip->rtb_length; i++) {
	mask0 |= 1 << (RTTSI_SIZE - i);
    }

    /* Find a place where the mask fits */
    /* This will not find a mask that crosses an 8 byte boundry */
    for (i = 0; i < sizeof (rttsi_map) / sizeof (rtbit_type); i++) {
	rtbit_type mask = mask0;

	ip->rtb_index = i * RTTSI_SIZE;

	do {
	    if (!(rttsi_map[i] & mask)) {
		rttsi_map[i] |= mask;
		return;
	    }
	    ip->rtb_index++;
	} while (!(mask & 1) && (mask >>= 1));
    }

    assert(FALSE);	/* No bits available */
}

/* Set the tsi for a route */
void
rttsi_get __PF3(rth, rt_head *,
		bit, u_int,
		value, byte *)
{
    register rtbit_info *ip = &rtbit_map[bit-1];
    register u_int block = ip->rtb_index / RTTSI_SIZE;
    register rt_tsi *tsi = rth->rth_tsi;
    register int i = ip->rtb_length;

    while (tsi && block--) {
	tsi = tsi->tsi_next;
    }
    if (tsi) {
	byte *cp = &tsi->tsi_tsi[ip->rtb_index % RTTSI_SIZE];

	while (i--) {
	    *value++ = *cp++;
	}
    } else {
	while (i--) {
	    *value++ = (char) 0;
	}
    }
}


/* Get the tsi for a route */
void
rttsi_set __PF3(rth, rt_head *,
		bit, u_int,
		value, byte *)
{
    register rtbit_info *ip = &rtbit_map[bit-1];
    register u_int block = ip->rtb_index / RTTSI_SIZE;
    register rt_tsi *tsi = rth->rth_tsi;
    register int i = ip->rtb_length;
    register byte *cp;

    if (!tsi) {
	rth->rth_tsi = (rt_tsi *) task_block_alloc(rttsi_block_index);
	tsi = rth->rth_tsi;
    }
    while (block--) {
	if (!tsi->tsi_next) {
	    tsi->tsi_next = (rt_tsi *) task_block_alloc(rttsi_block_index);
	}
	tsi = tsi->tsi_next;
    }
    cp = &tsi->tsi_tsi[ip->rtb_index % RTTSI_SIZE];
    while (i--) {
	*cp++ = *value++;
    }
}


/* Reset the tsi for a route */
void
rttsi_reset __PF2(rth, rt_head *,
		  bit, u_int)
{
    register rtbit_info *ip = &rtbit_map[bit-1];
    register u_int block = ip->rtb_index / RTTSI_SIZE;
    register rt_tsi *tsi = rth->rth_tsi;
    register int i = ip->rtb_length;
    register byte *cp;

    if (!tsi) {
	return;
    }
    while (block--) {
	if (!tsi->tsi_next) {
	    return;
	}
	tsi = tsi->tsi_next;
    }
    cp = &tsi->tsi_tsi[ip->rtb_index % RTTSI_SIZE];
    while (i--) {
	*cp++ = (byte) 0;
    }
}


/* Free the TSI field */
static inline void
rttsi_release __PF1(release_rth, rt_head *)
{
    register rt_tsi *tsi = release_rth->rth_tsi;

    while (tsi) {
	register rt_tsi *otsi = tsi;

	tsi = tsi->tsi_next;

	task_block_free(rttsi_block_index, (void_t) otsi);
    }
}


static void
rttsi_free __PF1(ip, rtbit_info *)
{
    u_int i;
    rtbit_type mask = 0;

    for (i = 1; i <= ip->rtb_length; i++) {
	mask |= 1 << (RTTSI_SIZE - i);
    }

    rttsi_map[ip->rtb_index / RTTSI_SIZE] &= ~(mask >> (ip->rtb_index % RTTSI_SIZE));

    ip->rtb_index = ip->rtb_length = 0;
}


static void
rttsi_dump __PF2(fp, FILE *,
		 rth, rt_head *)
{
    u_int bit;

    (void) fprintf(fp,
		   "\t\t\tTSI:\n");

    for (bit = 1; bit <= RTBIT_NBITS; bit++) {
	if (rtbit_map[bit-1].rtb_dump) {
	    rtbit_map[bit-1].rtb_dump(fp, rth, rtbit_map[bit-1].rtb_data, "\t\t\t\t");
	}
    }
}



/**/
/*
 *	Remove an rt_head pointer.
 */
static void
rth_remove __PF1(remove_rth, rt_head *)
{
    rt_table_delete(remove_rth);

    /* Count this rt_head */
    rtaf_info[socktype(remove_rth->rth_dest)].rtaf_dests--;

    sockfree(remove_rth->rth_dest);

    rttsi_release(remove_rth);

    task_block_free(rth_block_index, (void_t) remove_rth);
}


/*
 *	Locate the rt_head pointer for this destination.  Create one if it does not exist.
 */
static inline rt_head *
rth_locate __PF4(locate_dst, sockaddr_un *,
		 locate_mask, sockaddr_un *,
		 locate_state, flag_t *,
		 locate_errmsg, const char **)
{
    rt_head *locate_rth = (rt_head *) 0;

    *locate_errmsg = (char *) 0;

    if (BIT_TEST(*locate_state, RTS_GROUP)) {
	assert(!locate_mask);
    } else {
	if (!locate_mask) {
	    *locate_errmsg = "mask not specified";
	    return (rt_head *) 0;
	}
	/* Locate proper mask */
	locate_mask = mask_locate(locate_mask);
    }

    /* Locate this entry in the table */
    locate_rth = rt_table_locate(locate_dst, locate_mask);
    if (locate_rth) {
	/* Existing route */

	if (locate_rth->rth_dest_mask != locate_mask) {
	    *locate_errmsg = "mask conflict";
	    return (rt_head *) 0;
	}
    } else {
	/* New route */
	
	locate_rth = (rt_head *) task_block_alloc(rth_block_index);

	/* Copy destination */
	locate_rth->rth_dest = sockdup(locate_dst);

	/* Clean up the address */
	sockclean(locate_rth->rth_dest);
	
	/* Set the mask */
	if (locate_mask) {
	    locate_rth->rth_dest_mask = locate_mask;
	}

	/* Count this rt_head */
	rtaf_info[socktype(locate_rth->rth_dest)].rtaf_dests++;

	if (BIT_TEST(*locate_state, RTS_GROUP)) {
	    BIT_SET(locate_rth->rth_state, RTS_GROUP);
	} else {
	    switch (socktype(locate_dst)) {
#ifdef	PROTO_INET
	    case AF_INET:
		if (sock2host(locate_rth->rth_dest, locate_rth->rth_dest_mask)) {
		    *locate_errmsg = "host bits not zero";
		    goto Return;
		}
		break;
#endif	/* PROTO_INET */
	    }
	}

	locate_rth->rt_forw = locate_rth->rt_back = (rt_entry *) &locate_rth->rt_forw;
	locate_rth->rt_head = locate_rth;

	/* Add this entry to the table */
	rt_table_add(locate_rth);

    Return:
	if (*locate_errmsg) {
	    if (locate_rth->rth_dest) {
		sockfree(locate_rth->rth_dest);
	    }
	    task_block_free(rth_block_index, (void_t) locate_rth);
	    locate_rth = (rt_head *) 0;
	}
    }

    return locate_rth;
}


/**/
static inline void
rtchanges_free  __PF1(rtc_rth, rt_head *)
{
    rt_changes *rtc_rtcp = rtc_rth->rth_changes;

    if (!rtc_rtcp) {
	return;
    }
    
#ifdef	PROTO_ASPATHS
    if (BIT_TEST(rtc_rtcp->rtc_flags, RTCF_ASPATH) &&
	rtc_rtcp->rtc_aspath) {
	aspath_unlink(rtc_rtcp->rtc_aspath);
    }
#endif	/* PROTO_ASPATHS */

    if (BIT_TEST(rtc_rtcp->rtc_flags, RTCF_NEXTHOP)) {
	int i = rtc_rtcp->rtc_n_gw;

	/* Free the routers */
	while (i--) {
	    if (rtc_rtcp->rtc_routers[i]) {
		sockfree(rtc_rtcp->rtc_routers[i]);
		IFA_FREE(rtc_rtcp->rtc_ifaps[i]);
	    }
	}
    }

    task_block_free(rtchange_block_index, (void_t) rtc_rtcp);

    rtc_rth->rth_changes = (rt_changes *) 0;
}


/**/


/* Free a route */
static inline rt_entry *
rt_free  __PF1(free_rt, rt_entry *)
{
    if (free_rt) {
	register int free_i;
	register rt_head *free_rth = free_rt->rt_head;
	rt_entry *prev_rt = free_rt->rt_back;

#ifdef	PROTO_ASPATHS
	/* Free the AS path.  Do it before freeing anything else */
	if (free_rt->rt_aspath) {
	    aspath_rt_free(free_rt);
	}
#endif	/* PROTO_ASPATHS */

	if (free_rth) {
	    if (free_rt == free_rth->rth_last_active) {
		/* This is the last active route reset it */
		free_rth->rth_last_active = (rt_entry *) 0;

		if (free_rth->rth_changes) {
		    /* Clean up rt_change block */
		    rtchanges_free(free_rth);
		}
	    }
	    if (!free_rth->rth_entries) {
		if (!BIT_TEST(free_rth->rth_state, RTS_ONLIST)) {
		    /* This is not on a list OK to remove it now */

		    rth_remove(free_rth);
		    prev_rt = (rt_entry *) 0;
		}
	    }
	}

#ifdef	PROTO_SNMP
	/* Make sure the SNMP code does not have a cached pointer to this route */
	rt_mib_free_rt(free_rt);
#endif	/* PROTO_SNMP */

	/* Release any route specific data, remove the route from the change */
	/* and free this route */
	if (free_rt->rt_data && free_rt->rt_gwp->gw_rtd_free) {
	    free_rt->rt_gwp->gw_rtd_free(free_rt, free_rt->rt_data);
	}

	for (free_i = 0; free_i < free_rt->rt_n_gw; free_i++) {
	    sockfree(free_rt->rt_routers[free_i]);
	    IFA_FREE(free_rt->rt_ifaps[free_i]);
	}

	/* And finally free the block */
	task_block_free(rt_block_index, (void_t) free_rt);

	free_rt = prev_rt;
    }

    return free_rt;
}

/**/
/* Routing table state machine support routines */

/*
 * rt_trace() traces changes to the routing tables
 */
static inline void
rt_trace __PF3(tp, task *,
	       t_rt, rt_entry *,
	       action, const char *)
{
    /* XXX - Need indication of active and holddown */
    
    tracef("%-8s %-15A ",
	   action,
	   t_rt->rt_dest);
    if (t_rt->rt_dest_mask) {
	tracef(" %-15A ",
	       t_rt->rt_dest_mask);
    }

    switch (t_rt->rt_n_gw) {
	register int i;

    case 0:
	break;

    case 1:
	tracef("gw %-15A",
	       RT_ROUTER(t_rt));
	break;

    default:
	tracef("gw");
	for (i 	= 0; i < t_rt->rt_n_gw; i++) {
	    tracef("%c%A",
		   i ? ',' : ' ',
		   t_rt->rt_routers[i]);
	}

	break;
    }

    tracef(" %-8s pref %d/%d metric %d/%d",
	   trace_state(rt_proto_bits, t_rt->rt_gwp->gw_proto),
	   t_rt->rt_preference,
	   t_rt->rt_preference2,
	   t_rt->rt_metric,
	   t_rt->rt_metric2);

    switch (t_rt->rt_n_gw) {
	register int i;

    case 0:
	break;

    default:
	for (i = 0; i < t_rt->rt_n_gw; i++) {
	    if (t_rt->rt_ifaps[i]) {
		tracef("%c%s",
		       i ? ',' : ' ',
		       t_rt->rt_ifaps[i]->ifa_link->ifl_name);
	    }
	}
	break;
    }

    tracef(" <%s>",
	   trace_bits(rt_state_bits, t_rt->rt_state));
    if (t_rt->rt_gwp->gw_peer_as) {
	tracef("  as %d",
	       t_rt->rt_gwp->gw_peer_as);
    }	

    /* XXX - Format protocol specific information? */

    trace_only_tp(tp,
		  TRC_NOSTAMP,
		  (NULL));
}

static RTBIT_MASK(rt_holddown_bits);	/* Bits that belong to holddown protocols */

static const char *log_change = "CHANGE";
static const char *log_release = "RELEASE";

#define	rt_set_delete(rt)  \
    do { \
	BIT_SET((rt)->rt_state, RTS_DELETE); \
	rtaf_info[socktype((rt)->rt_dest)].rtaf_deletes++; \
    } while (0)
#define	rt_reset_delete(rt) \
    do { \
	BIT_RESET((rt)->rt_state, RTS_DELETE); \
	rtaf_info[socktype((rt)->rt_dest)].rtaf_deletes--; \
    } while (0)

#define	rt_set_release(rt)	BIT_SET((rt)->rt_state, RTS_RELEASE)

#define	rt_set_holddown(rt) \
    do { \
	if (!(rt)->rt_holddown) { \
	    (rt)->rt_holddown = (rt); \
	    rtaf_info[socktype((rt)->rt_dest)].rtaf_holddowns++; \
	} \
    } while (0)
#define	rt_reset_holddown(rt) \
    do { \
	if ((rt)->rt_holddown == (rt)) { \
	    (rt)->rt_holddown = (rt_entry *) 0; \
	    rtaf_info[socktype((rt)->rt_dest)].rtaf_holddowns--; \
	} \
    } while (0)

#define	rt_set_pending(rt) \
    do { \
	BIT_SET((rt)->rt_state, RTS_PENDING); \
    } while (0)
#define	rt_reset_pending(rt) \
    do { \
	BIT_RESET((rt)->rt_state, RTS_PENDING); \
    } while (0)

#define	rt_set_active(rt) \
    do { \
	(rt)->rt_active = rt; \
	BIT_SET((rt)->rt_state, RTS_ACTIVE); \
	rtaf_info[socktype((rt)->rt_dest)].rtaf_actives++; \
    } while (0)
#define	rt_reset_active(rt) \
    do { \
	(rt)->rt_active = (rt_entry *) 0; \
	BIT_RESET((rt)->rt_state, RTS_ACTIVE|RTS_PENDING); \
	rtaf_info[socktype((rt)->rt_dest)].rtaf_actives--; \
    } while (0)

#define	rt_set_hidden(rt) \
    do { \
	BIT_SET((rt)->rt_state, RTS_HIDDEN); \
	rtaf_info[socktype((rt)->rt_dest)].rtaf_hiddens++; \
    } while (0)
#define	rt_reset_hidden(rt) \
    do { \
	BIT_RESET((rt)->rt_state, RTS_HIDDEN); \
	rtaf_info[socktype((rt)->rt_dest)].rtaf_hiddens--; \
    } while (0)

#define	rt_error(trp, name) \
    do { \
	trace_log_tp(trp, 0, LOG_ERR, ("rt_event_%s: fatal state error", name)); \
	task_quit(EINVAL); \
    } while (0)
#define	rt_set_flash(rt) \
    BIT_SET((rt)->rt_head->rth_state, RTS_FLASH)
#define	rt_reset_flash(rt) \
    BIT_RESET((rt)->rt_head->rth_state, RTS_FLASH)
#define	rt_assert_noflash() \
    assert(!BIT_TEST(task_state, TASKS_FLASH|TASKS_NEWPOLICY))
#define	rt_set_onlist(rt) \
    BIT_SET((rt)->rt_head->rth_state, RTS_ONLIST); RTLIST_ADD(rt_change_list, (rt)->rt_head)

/* Select the active route and return a pointer to it or a NULL pointer */

static inline rt_entry *
rt_select_active  __PF1(rth, rt_head *)
{
    register rt_entry *rt = rth->rt_forw;
	
    if (rt == (rt_entry *) &rth->rt_forw) {
	/* No routes to become active */

	return (rt_entry *) 0;
    }

    if (BIT_TEST(rt->rt_state, RTS_DELETE|RTS_RELEASE|RTS_HIDDEN)) {
	/* This route is scheduled for delete, release or is hidden */

	return (rt_entry *) 0;
    }

    /* If this candidate for the active route is from a holddown */
    /* protocol, and the old active route is being announced by a */
    /* holddown protocol, set the RTS_PENDING flag to prevent this */
    /* route from being announced until the formerly active route */
    /* leaves holddown.  This does not prevent this route from */
    /* becoming active or being installed in the forwarding table, */
    /* it only prevents it from being announced to other protocols */
    /* because of the chance that it is an echo of an announced route.  */

    if (BIT_TEST(rt->rt_gwp->gw_flags, GWF_NEEDHOLD)
	&& rth->rth_n_announce) {
	register rt_entry *old_rt;

	RT_ALLRT(old_rt, rth) {
	    if (old_rt != rt
		&& old_rt->rt_n_bitsset) {
#if	RTBIT_SIZE > 1
		register int i = RTBIT_SIZE;

		while (i--) {
		    if (BIT_TEST(rt_holddown_bits[i], old_rt->rt_bits[i])) {
			rt_set_pending(rt);
			return rt;
		    }
		}
#else	/* RTBIT_SIZE > 1 */
		if (BIT_TEST(*rt_holddown_bits, *old_rt->rt_bits)) {
		    rt_set_pending(rt);
		    return rt;
		}
#endif	/* RTBIT_SIZE > 1 */
	    }
	} RT_ALLRT_END(old_rt, rth) ;
    }

    return rt;
}


/*
 *	Remove an rt_entry structure from the doubly linked list
 *	pointed to by it's rt_head
 */

static void
rt_remove  __PF1(remove_rt, rt_entry *)
{
    if (!--remove_rt->rt_head->rth_entries) {
	rtaf_info[socktype(remove_rt->rt_dest)].rtaf_routes--;
    }
    REMQUE(remove_rt);
}


/*	Insert an rt_entry structure in preference order in the doubly linked	*/
/*	list pointed to by it's rt_head.  If two routes with identical		*/
/*	preference are found, the one witht he shorter as path length is used.	*/
/*	If the as path lengths are the same, the route with the lower next-hop	*/
/*	IP address is prefered. This insures that the selection of the prefered	*/
/*	route is deterministic.							*/
static void
rt_insert __PF1(insert_rt, rt_entry *)
{
    rt_entry *insert_rt1;
    rt_head *insert_rth = insert_rt->rt_head;

    /*
     * Sort usable routes before hidden routes, which are sorted before
     * deleted routes.  Deleted and hidden routes are attached at the
     * end of their respective lists, usable routes are sorted into
     * preference order.
     *
     * If this is a delete, just hook it to the end and return.  If this
     * is a hidden route search the list from the end to find the spot
     * to insert it.
     */
    if (BIT_TEST(insert_rt->rt_state, (RTS_DELETE|RTS_HIDDEN))) {
	if (BIT_TEST(insert_rt->rt_state, RTS_DELETE)) {
	    /* Insert delete routes at the end of the list */
	    
	    INSQUE(insert_rt, insert_rth->rt_back);
	} else {
	    /* Insert hidden routes before the last deleted route */
	    
	    RT_ALLRT_REV(insert_rt1, insert_rth) {
		if (!BIT_TEST(insert_rt1->rt_state, RTS_DELETE)) {
		    break;
		}
	    } RT_ALLRT_REV_END(insert_rt1, insert_rth);

	    INSQUE(insert_rt, insert_rt1 ? insert_rt1 : (rt_entry *) &insert_rth->rt_forw);
	}
    } else {
	/* Neither deleted or hidden, do the check the long way */

	RT_ALLRT(insert_rt1, insert_rth) {
	    if (BIT_TEST(insert_rt1->rt_state, (RTS_DELETE|RTS_HIDDEN))) {
		break;
	    }
	    if (insert_rt->rt_preference < insert_rt1->rt_preference) {
		/* This preference is better */

		break;
	    } else if (insert_rt->rt_preference == insert_rt1->rt_preference) {
#ifdef	PROTO_ASPATHS
		int path_which;
#endif	/* PROTO_ASPATHS */

		/*
		 * Break ties with the second preference if they differ.
		 */
		if (insert_rt->rt_preference2 < insert_rt1->rt_preference2) {
		    break;
		}
		if (insert_rt->rt_preference2 > insert_rt1->rt_preference2) {
		    continue;
		}

		/*
		 * Prefer strictly interior routes over anything.  Prefer
		 * strictly exterior routes over exterior routes received
		 * on an interior session.  I.e. prefer state bits in this
		 * order:
		 *		RTS_INTERIOR
		 *		RTS_EXTERIOR
		 *		RTS_INTERIOR|RTS_EXTERIOR
		 */
		if (!BIT_MASK_MATCH(insert_rt->rt_state,
				    insert_rt1->rt_state,
				    (RTS_INTERIOR|RTS_EXTERIOR))) {
		    if (!BIT_TEST(insert_rt->rt_state, RTS_EXTERIOR)) {
			break;		/* new route is better */
		    }
		    if (!BIT_TEST(insert_rt1->rt_state, RTS_EXTERIOR)) {
			continue;	/* current route is better */
		    }
		    if (!BIT_TEST(insert_rt->rt_state, RTS_INTERIOR)) {
			break;		/* new route is better */
		    }
		    continue;		/* current route is better */
		}
#ifdef	PROTO_ASPATHS
		/* See if the AS path provides any hints about which route to use */
		/* XXX Need to do something similar for ISO */
		path_which = aspath_prefer(insert_rt, insert_rt1);

		if (path_which < 0) {
		    break;
		}
		if (path_which == 0) {
#endif	/* PROTO_ASPATHS */
		    if (insert_rt->rt_gwp->gw_proto == insert_rt1->rt_gwp->gw_proto
			&& insert_rt->rt_gwp->gw_peer_as == insert_rt1->rt_gwp->gw_peer_as) {
			/* Same protocol and AS */

			if (insert_rt->rt_metric < insert_rt1->rt_metric) {
			    /* Use the lower metric */ 
			    break;
			}
			if (insert_rt->rt_metric > insert_rt1->rt_metric) {
			    /* Current one is more attractive */
			    continue;
			}
		    }

		    /* Default to comparing the router address to be deterministic */
		    if (!insert_rt->rt_n_gw || !insert_rt1->rt_n_gw) {
			/* Only one has a next hop */

			if (insert_rt->rt_n_gw) {
			    /* This is the one with the next hop, use it */

			    break;
			}
		    } else if (sockaddrcmp2(RT_ROUTER(insert_rt), RT_ROUTER(insert_rt1)) < 0) {
			/* This router address is lower, use it */
			break;
		    }
#ifdef	PROTO_ASPATHS
		}
#endif	/* PROTO_ASPATHS */
	    }
	} RT_ALLRT_END(insert_rt1, insert_rth);

	/* Insert prior to element if found, or behind the element at the end of a list. */
	/* For an empty list this ends up being behind the first element. */
	INSQUE(insert_rt, insert_rt1 ? insert_rt1->rt_back : insert_rth->rt_back);
    }

    if (!insert_rth->rth_entries++) {
	rtaf_info[socktype(insert_rt->rt_dest)].rtaf_routes++;
    }
}


static void
rt_event_active  __PF2(rt, rt_entry *,
		       log, int)
{
    
    switch (rt->rt_state & RTS_STATEMASK) {

    case RTS_INITIAL:
	rt_error(rt_opentask, "active");
	break;
	    
    case RTS_ELIGIBLE:
	rt_set_active(rt);
	rt_reset_holddown(rt);
	break;
	    
    case RTS_HIDDEN:
	rt_error(rt_opentask, "active");
	break;
	    
    case RTS_ACTIVE:
	rt_reset_pending(rt);
	break;
	    
    case RTS_DELETE:
	rt_error(rt_opentask, "active");
	break;
	    
    case RTS_HIDDEN|RTS_DELETE:
	rt_error(rt_opentask, "active");
	break;
	    
    default:
	rt_error(rt_opentask, "active");
    }
    
    rt_set_flash(rt);

    if (log && TRACE_TP(rt_opentask, TR_ROUTE)) {
	rt_trace(rt_opentask, rt, log_change);
    }
}


static void
rt_event_inactive __PF1(rt, rt_entry *)
{
    switch (rt->rt_state & RTS_STATEMASK) {
	
    case RTS_INITIAL:
	rt_error(rt_opentask, "inactive");
	break;
	    
    case RTS_ELIGIBLE:
    case RTS_HIDDEN:
	rt_error(rt_opentask, "inactive");
	break;
	    
    case RTS_ACTIVE:
	rt_reset_active(rt);
	if (rt->rt_n_bitsset) {
	    rt_set_holddown(rt);
	    rt_set_flash(rt);
	}
	break;
	    
    case RTS_DELETE:
    case RTS_HIDDEN|RTS_DELETE:
    default:
	rt_error(rt_opentask, "inactive");
    }

    if (TRACE_TP(rt_opentask, TR_ROUTE)) {
	rt_trace(rt_opentask, rt, log_change);
    }
}


static void
rt_event_preference __PF2(pref_rt, rt_entry *,
			  gateway_changed, int)
{
    rt_entry *new_active;
    
    switch (pref_rt->rt_state & RTS_STATEMASK) {

    case RTS_INITIAL:
	rt_error(rt_opentask, "preference");
	break;
	    
    case RTS_ELIGIBLE:
	rt_remove(pref_rt);
	if (pref_rt->rt_preference >= 0) {
	    rt_insert(pref_rt);
	    if (rt_select_active(pref_rt->rt_head) == pref_rt) {
		if (pref_rt->rt_active) {
		    rt_event_inactive(pref_rt->rt_active);
		}
		rt_event_active(pref_rt, FALSE);
	    }
	} else {
	    rt_set_hidden(pref_rt);
	    rt_insert(pref_rt);
	}	
	break;
	    
    case RTS_HIDDEN:
	rt_remove(pref_rt);
	if (pref_rt->rt_preference >= 0) {
	    rt_reset_hidden(pref_rt);
	    rt_insert(pref_rt);
	    if (rt_select_active(pref_rt->rt_head) == pref_rt) {
		if (pref_rt->rt_active) {
		    rt_event_inactive(pref_rt->rt_active);
		}
		rt_event_active(pref_rt, FALSE);
	    }
	} else {
	    rt_insert(pref_rt);
	}
	break;
	
    case RTS_ACTIVE:
	rt_remove(pref_rt);
	if (pref_rt->rt_preference >= 0) {
	    rt_insert(pref_rt);
	    new_active = rt_select_active(pref_rt->rt_head);
	    if (new_active != pref_rt) {
		rt_reset_active(pref_rt);
		if (pref_rt->rt_n_bitsset) {
		    rt_set_holddown(pref_rt);
		    rt_set_flash(pref_rt);
		}
		if (new_active) {
		    rt_event_active(new_active, TRUE);
		}
	    } else if (gateway_changed) {
		rt_set_flash(pref_rt);
	    }
	} else {
	    rt_set_hidden(pref_rt);
	    rt_reset_active(pref_rt);
	    if (pref_rt->rt_n_bitsset) {
		rt_set_holddown(pref_rt);
		rt_set_flash(pref_rt);
	    }
	    rt_insert(pref_rt);
	    if ((new_active = rt_select_active(pref_rt->rt_head))) {
		rt_event_active(new_active, TRUE);
	    }
	}
	break;
	    
    case RTS_DELETE:
	rt_reset_delete(pref_rt);
	rt_remove(pref_rt);
	if (pref_rt->rt_preference >= 0) {
	    rt_insert(pref_rt);
	    if (rt_select_active(pref_rt->rt_head) == pref_rt) {
		if (pref_rt->rt_active) {
		    rt_event_inactive(pref_rt->rt_active);
		}
		rt_event_active(pref_rt, FALSE);
	    }
	} else {
	    rt_set_hidden(pref_rt);
	    rt_insert(pref_rt);
	}
	break;
	
    case RTS_HIDDEN|RTS_DELETE:
	rt_remove(pref_rt);
	rt_reset_delete(pref_rt);
	if (pref_rt->rt_preference >= 0) {
	    rt_reset_hidden(pref_rt);
	    rt_insert(pref_rt);
	    if (rt_select_active(pref_rt->rt_head) == pref_rt) {
		if (pref_rt->rt_active) {
		    rt_event_inactive(pref_rt->rt_active);
		}
		rt_event_active(pref_rt, FALSE);
	    }
	} else {
	    rt_insert(pref_rt);
	}
	break;
	
    default:
	rt_error(rt_opentask, "preference");
    }	
    
    if (BIT_COMPARE(pref_rt->rt_head->rth_state, RTS_FLASH|RTS_ONLIST, RTS_FLASH)) {
	rt_assert_noflash();
	rt_set_onlist(pref_rt);
    }
    rt_reset_flash(pref_rt);
    
    if (TRACE_TP(rt_opentask, TR_ROUTE)) {
	rt_trace(rt_opentask, pref_rt, log_change);
    }
}	


static void
rt_event_unreachable __PF1(rt, rt_entry *)
{
    const char *log_type = (const char *) 0;
    rt_entry *new_active;
    
    switch (rt->rt_state & RTS_STATEMASK) {
	
    case RTS_INITIAL:
	rt_error(rt_opentask, "unreachable");
	break;
	
    case RTS_ELIGIBLE:
	rt_remove(rt);
	if (rt->rt_n_bitsset) {
	    rt_set_delete(rt);
	    rt_insert(rt);
	    log_type = log_change;
	} else {
	    rt_set_release(rt);
	    log_type = log_release;
	}
	break;
	
    case RTS_HIDDEN:
	rt_remove(rt);
	if (rt->rt_n_bitsset) {
	    rt_set_delete(rt);
	    rt_insert(rt);
	    log_type = log_change;
	} else {
	    rt_reset_hidden(rt);
	    rt_set_release(rt);
	    log_type = log_release;
	}
	break;
	
    case RTS_ACTIVE:
	rt_remove(rt);
	rt_reset_active(rt);
	if (rt->rt_n_bitsset) {
	    rt_set_holddown(rt);
	    rt_set_flash(rt);
	    rt_set_delete(rt);
	    rt_insert(rt);
	    log_type = log_change;
	} else {
	    rt_set_release(rt);
	    log_type = log_release;
	}
	if ((new_active = rt_select_active(rt->rt_head))) {
	    rt_event_active(new_active, TRUE);
	}
	break;
	    
    case RTS_DELETE:
	rt_error(rt_opentask, "unreachable");	/* XXX - ignore */
	break;
	
    case RTS_HIDDEN|RTS_DELETE:
	rt_error(rt_opentask, "unreachable");	/* XXX - ignore */
	break;
	
    default:
	rt_error(rt_opentask, "unreachable");
    }	
    
    if (BIT_COMPARE(rt->rt_head->rth_state, RTS_FLASH|RTS_ONLIST, RTS_FLASH)) {
	rt_assert_noflash();
	rt_set_onlist(rt);
    }
    rt_reset_flash(rt);
    
    if (log_type && TRACE_TP(rt_opentask, TR_ROUTE)) {
	rt_trace(rt_opentask, rt, log_type);
    }
}	


static inline void
rt_event_bit_set __PF2(rt, rt_entry *,
		       bit, u_int)
{
    RTBIT_SET(bit, rt->rt_bits);
    
    switch (rt->rt_state & RTS_STATEMASK) {
	    
    case RTS_INITIAL:
    case RTS_ELIGIBLE:
    case RTS_HIDDEN:
	rt_error(rt_opentask, "bit_set");
	break;
	
    case RTS_ACTIVE:
	if (!rt->rt_n_bitsset++) {
	    rt->rt_head->rth_n_announce++;
	}
	break;
	
    case RTS_DELETE:
    case RTS_HIDDEN|RTS_DELETE:
    default:
	rt_error(rt_opentask, "bit_set");
    }
}


static inline void
rt_event_bit_reset  __PF3(rt, rt_entry *,
			  bit, u_int,
			  pending, int)
{
    const char *log_type = (const char *) 0;
    
    RTBIT_CLR(bit, rt->rt_bits);
    
    switch (rt->rt_state & RTS_STATEMASK) {
	
    case RTS_INITIAL:
	rt_error(rt_opentask, "bit_reset");
	break;
	
    case RTS_ELIGIBLE:
    case RTS_HIDDEN:
	if (!--rt->rt_n_bitsset) {
	    rt->rt_head->rth_n_announce--;
	    rt_reset_holddown(rt);
#ifdef	notdef
	    rt_remove(rt);
	    rt_insert(rt);
#endif	/* notdef */
	    if (rt->rt_active
		&& (pending
		    || BIT_TEST(rt->rt_active->rt_state, RTS_PENDING))) {
		if (BIT_TEST(task_state, TASKS_NEWPOLICY|TASKS_FLASH)) {
		    BIT_SET(rt->rt_head->rth_state, RTS_PENDING);
		} else {
		    rt_event_active(rt->rt_active, TRUE);
		}
	    }
	}
	break;
	
    case RTS_ACTIVE:
	if (!--rt->rt_n_bitsset) {
	    rt->rt_head->rth_n_announce--;
	}
	break;
	
    case RTS_DELETE:
	if (!--rt->rt_n_bitsset) {
	    rt->rt_head->rth_n_announce--;
	    rt_reset_holddown(rt);
	    rt_set_release(rt);
	    rt_remove(rt);
	    if (rt->rt_active
		&& (pending
		    || BIT_TEST(rt->rt_active->rt_state, RTS_PENDING))) {
		if (BIT_TEST(task_state, TASKS_NEWPOLICY|TASKS_FLASH)) {
		    BIT_SET(rt->rt_head->rth_state, RTS_PENDING);
		} else {
		    rt_event_active(rt->rt_active, TRUE);
		}
	    }
	    log_type = log_release;
	}
	break;
	
    case RTS_HIDDEN|RTS_DELETE:
	if (!--rt->rt_n_bitsset) {
	    rt->rt_head->rth_n_announce--;
	    rt_reset_holddown(rt);
	    rt_reset_hidden(rt);
	    rt_set_release(rt);
	    rt_remove(rt);
	    if (rt->rt_active
		&& (pending
		    || BIT_TEST(rt->rt_active->rt_state, RTS_PENDING))) {
		if (BIT_TEST(task_state, TASKS_NEWPOLICY|TASKS_FLASH)) {
		    BIT_SET(rt->rt_head->rth_state, RTS_PENDING);
		} else {
		    rt_event_active(rt->rt_active, TRUE);
		}
	    }
	    log_type = log_release;
	}
	break;
	
    default:
	rt_error(rt_opentask, "bit_reset");
    }		
    
    if (log_type && TRACE_TP(rt_opentask, TR_ROUTE)) {
	rt_trace(rt_opentask, rt, log_type);
    }

    if (BIT_COMPARE(rt->rt_head->rth_state, RTS_FLASH|RTS_ONLIST, RTS_FLASH) &&
	!BIT_TEST(task_state, TASKS_NEWPOLICY)) {
	rt_set_onlist(rt);
	rt_n_changes++;
    }
    rt_reset_flash(rt);
}


static void
rt_event_initialize __PF1(rt, rt_entry *)
{
    rt_entry *new_active = (rt_entry *) 0;

    switch (rt->rt_state & RTS_STATEMASK) {
    case RTS_INITIAL:
	BIT_RESET(rt->rt_state, RTS_INITIAL);
	if (rt->rt_preference >= 0) {
	    rt_insert(rt);
	    new_active = rt_select_active(rt->rt_head);
	    if (new_active != rt->rt_active) {
		if (rt->rt_active) {
		    rt_event_inactive(rt->rt_active);
		}
		if (new_active) {
		    rt_event_active(new_active, FALSE);
		}
	    }
	} else {
	    rt_set_hidden(rt);
	    rt_insert(rt);
	}
	break;
	    
    case RTS_ELIGIBLE:
    case RTS_HIDDEN:
    case RTS_ACTIVE:
    case RTS_DELETE:
    case RTS_HIDDEN|RTS_DELETE:
    default:
	rt_error(rt_opentask, "initialize");
    }

    if (BIT_COMPARE(rt->rt_head->rth_state, RTS_FLASH|RTS_ONLIST, RTS_FLASH)) {
	rt_assert_noflash();
	rt_set_onlist(rt);
    }
    rt_reset_flash(rt);
    
    if (TRACE_TP(rt_opentask, TR_ROUTE)) {
	rt_trace(rt_opentask, rt, "ADD");
    }
}


static int
rt_flash_cleanup __PF1(list, rt_list *)
{
    register rt_head *rth;
    int resched = 0;

    rt_open(rt_task);
    
    RT_LIST(rth, list, rt_head) {
	/* Indicate no longer on list */
	BIT_RESET(rth->rth_state, RTS_ONLIST);

	if (!rth->rth_entries) {
	    /* Nothing in this head, free it */

	    rth_remove(rth);
	} else {
	    /* Free up change block */
	    if (rth->rth_changes) {
		/* Clean up rt_change block */
		rtchanges_free(rth);
	    }

	    /* Reset last active pointer */
	    rth->rth_last_active = rth->rth_active;

	    if (BIT_TEST(rth->rth_state, RTS_PENDING)) {
		/* Need to reflash a pending route */

		assert(rth->rth_active && BIT_TEST(rth->rth_state, RTS_PENDING));
		rt_event_active(rth->rth_active, TRUE);
		BIT_RESET(rth->rth_state, RTS_PENDING);
		resched++;
	    }
	}
    } RT_LIST_END(rth, list, rt_head) ;

    rt_close(rt_task, (gw_entry *) 0, resched, NULL);
    
    return resched;
}


/*
 *
 */
void
rt_new_policy __PF0(void)
{
    register rt_list *list = rt_change_list;
    register rt_head *rth;
	
    if (list) {
	/* Discard the flash list */
	
	/* Get the root of the list */
	list = list->rtl_root;

	RTLIST_RESET(list);

	rt_change_list = (rt_list *) 0;
    }

    /* Get a full list */
    list = rthlist_all(AF_UNSPEC);

    if (list) {

	/* Update the  protocols */
	if (TRACE_TF(trace_global, TR_ROUTE)) {
	    trace_only_tf(trace_global,
			  TRC_NL_BEFORE,
			  ("rt_new_policy: new policy started with %d entries",
			   list->rtl_count));
	}

	/* Flag the routes as being on the list in case any are deleted */
	RT_LIST(rth, list, rt_head) {
	    BIT_SET(rth->rth_state, RTS_ONLIST);
	} RT_LIST_END(rth, list, rt_head) ;

	/* Recalculate aggregates based on policy */
	rt_aggregate_flash(list, 0);

	/* Let the kernel see everything */
	krt_flash(list);

	/* Now flash the protocols */
	task_newpolicy(list);

	/* Make sure no one changed anything while we were flashing */
	assert(!rt_change_list);

	if (TRACE_TF(trace_global, TR_ROUTE)) {
	    trace_only_tf(trace_global,
			  TRC_NL_AFTER,
			  ("rt_new_policy: new policy ended with %d entries",
			   list->rtl_count));
	}

	if (list->rtl_count
	    && rt_flash_cleanup(list)) {
	    
	    rt_flash_job = task_job_create(rt_task,
					   TASK_JOB_PRIO_FLASH,
					   "flash_update",
					   rt_flash_update,
					   (void_t) 0);
	}

	/* And reset this list */
	RTLIST_RESET(list);
    }
}


/*
 * Cause a flash update to happen
 */
void
rt_flash_update __PF1(jp, task_job *)
{
    int delete = TRUE;

    rt_list *list = rt_change_list;
    
    if (list) {
	/* Get the root of the list */
	list = list->rtl_root;

	/* Run aggregation policy */
	rt_aggregate_flash(list, 0);

	/* Reset the change list */
	rt_change_list = (rt_list *) 0;
	
	/* Update the kernel */
	if (TRACE_TF(trace_global, TR_ROUTE)) {
	    trace_only_tf(trace_global,
			  TRC_NL_BEFORE,
			  ("rt_flash_update: updating kernel with %d entries",
			   list->rtl_count));
	}
	krt_flash(list);

	if (!BIT_TEST(task_state, TASKS_TERMINATE)) {
	    /* Update the protocols */

	    if (TRACE_TF(trace_global, TR_ROUTE)) {
		trace_only_tf(trace_global,
			      TRC_NL_BEFORE,
			      ("rt_flash_update: flash update started with %d entries",
			       list->rtl_count));
	    }

	    task_flash(list);

	    if (TRACE_TF(trace_global, TR_ROUTE)) {
		trace_only_tf(trace_global,
			      TRC_NL_AFTER,
			      ("rt_flash_update: flash update ended with %d entries",
			       list->rtl_count));
	    }
	}

	/* Make sure no one changed anything while we were flashing */
	assert(!rt_change_list);

	if (list->rtl_count
	    && rt_flash_cleanup(list)) {
	    delete = FALSE;
	}
		    
	/* And reset this list */
	RTLIST_RESET(list);
    }

    if (jp && delete) {
	task_job_delete(jp);
	rt_flash_job = (task_job *) 0;
    }
}


/**/
/* Allocate a bit for the protocol specific bit in the routing table */
u_int
rtbit_alloc(tp, holddown, size, data, dump)
task *tp;
int holddown;
u_int size;
void_t data;
_PROTOTYPE(dump,
	   void,
	   (FILE *,
	    rt_head *,
	    void_t,
	    const char *));
{
    u_int bit;
    rtbit_info *ip = rtbit_map;

    for (bit = 1; bit <= RTBIT_NBITS; ip++, bit++) {
	if (!ip->rtb_task) {
	    break;
	}
    }

    assert(bit <= RTBIT_NBITS);

    /* Indicate this bit has been allocated */
    ip->rtb_task = tp;
    ip->rtb_data = data;
    ip->rtb_dump = dump;

    if (size) {
	ip->rtb_length = size;
	rttsi_alloc(ip);
    }

    /* If this protocol does holddowns we must remember it's bits */
    if (holddown) {
	RTBIT_SET(bit, rt_holddown_bits);
    }
    return bit;
}

/* Free an allocated bit */
void
rtbit_free(tp, bit)
task *tp;
u_int bit;
{
    register rtbit_info *ip = &rtbit_map[bit-1];

    assert(ip->rtb_task == tp);

    ip->rtb_task = (task *) 0;
    ip->rtb_data = (void_t) 0;
    ip->rtb_dump = 0;

    /* Indicate that this bit no longer does holddowns */
    RTBIT_CLR(bit, rt_holddown_bits);
    
    if (ip->rtb_length) {
	rttsi_free(ip);
    }
}


/* Set the announcement bit for this network */
void
rtbit_set __PF2(set_rt, rt_entry *,
		set_bit, u_int)
{
    rt_check_open(set_rt->rt_gwp->gw_proto, "rtbit_set");

    if (!RTBIT_ISSET(set_bit, set_rt->rt_bits)) {
	rt_event_bit_set(set_rt, set_bit);
    }

#ifdef	RT_SANITY
    rt_sanity();
#endif	/* RT_SANITY */
    return;
}


/* Reset the announcement bit for this network */
rt_entry *
rtbit_reset __PF2(reset_rt, rt_entry *,
		  reset_bit, u_int)
{
    rt_check_open(reset_rt->rt_gwp->gw_proto, "rtbit_reset");

    if (RTBIT_ISSET(reset_bit, reset_rt->rt_bits)) {
	rt_event_bit_reset(reset_rt, reset_bit, FALSE);

	if (BIT_TEST(reset_rt->rt_state, RTS_RELEASE)) {
	    reset_rt = rt_free(reset_rt);
	}
    }
    
#ifdef	RT_SANITY
    rt_sanity();
#endif	/* RT_SANITY */
    return reset_rt;
}


/* Reset announcment bit and cause the active route to be put on a new */
/* list */
rt_entry *
rtbit_reset_pending __PF2(reset_rt, rt_entry *,
			  reset_bit, u_int)
{
    rt_check_open(reset_rt->rt_gwp->gw_proto, "rtbit_reset_pending");

    if (RTBIT_ISSET(reset_bit, reset_rt->rt_bits)) {
	rt_event_bit_reset(reset_rt, reset_bit, TRUE);

	if (BIT_TEST(reset_rt->rt_state, RTS_RELEASE)) {
	    reset_rt = rt_free(reset_rt);
	}
    }
    
#ifdef	RT_SANITY
    rt_sanity();
#endif	/* RT_SANITY */
    return reset_rt;
}


/* Reset the bits on all routes and free the bit */
void
rtbit_reset_all(tp, bit, gwp)
task *tp;
u_int bit;
gw_entry *gwp;
{
    int changes = 0;
    register rt_entry *rt;
    register rt_list *rtl = rtlist_all(AF_UNSPEC);
    
    rt_open(tp);

    RT_LIST(rt, rtl, rt_entry) {
	if (rtbit_isset(rt, bit)) {
	    changes++;

	    /* Clear the TSI field */
	    rttsi_reset(rt->rt_head, bit);

	    /* Reset this bit */
	    (void) rtbit_reset(rt, bit);
	}
    } RT_LIST_END(rt, rtl, rt_entry);

    RTLIST_RESET(rtl) ;
    
    rt_close(tp, gwp, changes, NULL);
    
    rtbit_free(tp, bit);
}


static void
rtbit_dump __PF1(fd, FILE *)
{
    u_int bit;
    rtbit_info *ip = rtbit_map;

    (void) fprintf(fd,
		   "\tBit allocations:\n");

    for (bit = 1; bit <= RTBIT_NBITS; ip++, bit++) {
	if (ip->rtb_task) {
	    (void) fprintf(fd,
			   "\t\t%d\t%s",
			   bit,
			   task_name(ip->rtb_task));
	    if (ip->rtb_length) {
		(void) fprintf(fd,
			       "\tbyte index: %d\tlength: %d",
			       ip->rtb_index,
			       ip->rtb_length);
	    }
	    (void) fprintf(fd, "\n");
	}
    }
    (void) fprintf(fd, "\n");
}


/**/

int rt_n_changes = 0;		/* Number of changes to routing table */
rt_list *rt_change_list = (rt_list *) 0;

/*
 *	rt_open: Make table available for updating
 */
void
rt_open __PF1(tp, task *)
{
    assert(!rt_opentask);
    rt_opentask = tp;
    rt_n_changes = 0;
}


/*
 *	rt_close: Clean up after table updates
 */
void
rt_close __PF4(tp, task *,
	       gwp, gw_entry *,
	       changes, int,
	       message, char *)
{
    assert(rt_opentask == tp);

    rt_opentask = (task *) 0;
    if (rt_n_changes) {
	if (TRACE_TP(tp, TR_ROUTE)) {
	    tracef("rt_close: %d",
		   rt_n_changes);
	    if (changes) {
		tracef("/%d",
		       changes);
	    }
	    tracef(" route%s proto %s",
		   rt_n_changes > 1 ? "s" : "",
		   task_name(tp));
	    if (gwp && gwp->gw_addr) {
		tracef(" from %A",
		       gwp->gw_addr);
	    }
	    if (message) {
		tracef(" %s",
		       message);
	    }
	    trace_only_tp(tp,
			  TRC_NL_AFTER,
			  (NULL));
	}
	rt_n_changes = 0;
    }

    /* Create a flash job */
    if (rt_change_list
	&& !rt_flash_job
	&& !BIT_TEST(task_state, TASKS_INIT|TASKS_RECONFIG|TASKS_TERMINATE)) {
	/* Schedule a flash update */

	rt_flash_job = task_job_create(rt_task,
				       TASK_JOB_PRIO_FLASH,
				       "flash_update",
				       rt_flash_update,
				       (void_t) 0);
    }

    return;
}


/**/

/* Looks up a destination network route with a specific protocol mask. */
/* Specifying a protocol of zero will match all protocols. */
rt_entry *
rt_locate __PF4(state, flag_t,
		dst, sockaddr_un *,
		mask, sockaddr_un *,
		proto_mask, flag_t)
{
    register rt_head *rth = rt_table_locate(dst, mask);

    if (rth) {
	register rt_entry *rt;

	RT_ALLRT(rt, rth) {
	    if (!BIT_TEST(rt->rt_state, RTS_DELETE)
		&& rt->rt_state & state & (RTS_NETROUTE|RTS_GROUP)
		&& BIT_TEST(proto_mask, RTPROTO_BIT(rt->rt_gwp->gw_proto))) {
		return rt;
	    }
	} RT_ALLRT_END(rt, rth);
    }

    return (rt_entry *) 0;
}


/* Look up a route with a destination address, protocol and source gateway */
rt_entry *
rt_locate_gw __PF4(state, flag_t,
		   dst, sockaddr_un *,
		   mask, sockaddr_un *,
		   gwp, gw_entry *)
{
    register rt_head *rth = rt_table_locate(dst, mask);

    if (rth) {
	register rt_entry *rt;
	
	RT_ALLRT(rt, rth) {
	    if (!BIT_TEST(rt->rt_state, RTS_DELETE)
		&& rt->rt_state & state & (RTS_NETROUTE|RTS_GROUP) 
		&& (rt->rt_gwp == gwp)) {
		return rt;
	    }
	} RT_ALLRT_END(rt, rth);
    }

    return (rt_entry *) 0;
}



/*
 *	Look up the most specific route that matches the supplied
 *	criteria
 */  
rt_entry *
rt_lookup __PF4(good, flag_t,
		bad, flag_t,
		dst, sockaddr_un *,
		proto_mask, flag_t)
{
    register rt_list *rtl = rthlist_match(dst);
    register rt_head *rth;
    register rt_entry *rt = (rt_entry *) 0;

    RT_LIST(rth, rtl, rt_head) {

	RT_ALLRT(rt, rth) {
	    if (!BIT_TEST(rt->rt_state, bad) &&
		BIT_TEST(rt->rt_state, good) &&
		BIT_TEST(proto_mask, RTPROTO_BIT(rt->rt_gwp->gw_proto))) {
		/* Found it */

		goto Return;
	    }
	} RT_ALLRT_END(rt, rth) ;
    } RT_LIST_END(rth, rtl, rt_head) ;
 Return:
    RTLIST_RESET(rtl);
    return rt;
}


/**/
#if	RT_N_MULTIPATH > 1
/*
 *	Do a linear sort of the routers to get them in ascending order
 */
static inline void
rt_routers_sort __PF2(routers, sockaddr_un **,
		      nrouters, int)
{
    register int i, j;
    
    for (i = 0; i < (nrouters-1); i++) {
	for (j = i+1; j < nrouters; j++) {
	    if (sockaddrcmp2(routers[i], routers[j]) > 0) {
		register sockaddr_un *swap = routers[i];

		routers[i] = routers[j];
		routers[j] = swap;
	    }
	}
    }
}

int
rt_routers_compare __PF2(rt, rt_entry *,
			 routers, sockaddr_un **)
{
    register int i = rt->rt_n_gw;

    /* Get them into the same order */
    rt_routers_sort(routers, i);
		
    while (i--) {
	if (!sockaddrcmp(routers[i], rt->rt_routers[i])) {
	    /* Found one that was different */
	    return FALSE;
	}
    }

    return TRUE;
}
#endif	/* RT_N_MULTIPATH > 1 */


/*  Add a route to the routing table after some checking.  The route	*/
/*  is added in preference order.  If the active route changes, the	*/
/*  kernel routing table is updated.					*/
rt_entry *
rt_add __PF1(pp, register rt_parms *)
{
    int i;
    const char *errmsg = (char *) 0;
    rt_entry *rt = (rt_entry *) 0;
    task *tp = pp->rtp_gwp->gw_task;

    rt_check_open(pp->rtp_gwp->gw_proto, "rt_add");

    /* Allocate an entry */
    rt = (rt_entry *) task_block_alloc(rt_block_index);

    /* Locate the head for this entry */
    rt->rt_head = rth_locate(pp->rtp_dest, pp->rtp_dest_mask, &pp->rtp_state, &errmsg);
    if (errmsg) {
	goto Error;
    }

    for (i = 0; i < pp->rtp_n_gw; i++) {
	rt->rt_routers[i] = sockdup(pp->rtp_routers[i]);

	/* Clean up the address */
	switch (socktype(rt->rt_routers[i])) {
#ifdef	PROTO_INET
	case AF_INET:
	    sock2port(rt->rt_routers[i]) = 0;
	    break;
#endif	/* PROTO_INET */

#ifdef	PROTO_ISO
	case AF_ISO:
	    /* XXX - What do we need here? */
	    break;
#endif	/* PROTO_ISO */
	}
    }
    rt->rt_n_gw = pp->rtp_n_gw;
#if	RT_N_MULTIPATH > 1
    rt_routers_sort(rt->rt_routers, (int) rt->rt_n_gw);
    rt->rt_gw_sel = (pp->rtp_n_gw > 1) ? (short) grand((u_int32) rt->rt_n_gw) : 0;
#endif	/* RT_N_MULTIPATH > 1 */

    rt->rt_gwp = pp->rtp_gwp;
    rt->rt_metric = pp->rtp_metric;
    rt->rt_metric2 = pp->rtp_metric2;
    rt->rt_tag = pp->rtp_tag;

    /* Add the route to the gateway queue and count */
    INSQUE(&rt->rt_rtq, rt->rt_gwp->gw_rtq.rtq_back);
    rt->rt_gwp->gw_n_routes++;
    rt->rt_time = time_sec;

    rt->rt_state = pp->rtp_state | RTS_INITIAL | (rt->rt_head->rth_state & ~(RTS_ONLIST));
    rt->rt_preference = pp->rtp_preference;
    rt->rt_preference2 = pp->rtp_preference2;
    if (!BIT_MATCH(rt->rt_state, rt->rt_head->rth_state)) {
	/* XXX - this route does not match */
    }

    /* Set the gateway flags */
    if (BIT_TEST(rt->rt_state, RTS_EXTERIOR)) {
	BIT_SET(rt->rt_state, RTS_GATEWAY);
    } else {
	switch (rt->rt_gwp->gw_proto) {
	    case RTPROTO_KERNEL:
	    case RTPROTO_STATIC:
	    case RTPROTO_DIRECT:
		break;

	    default:
		BIT_SET(rt->rt_state, RTS_GATEWAY);
		break;
	}
    }

    /* Check for martians */
    if (is_martian(pp->rtp_dest, pp->rtp_dest_mask)
	&& !BIT_TEST(pp->rtp_state, RTS_NOADVISE)) {
	/* It's a martian!  Make sure it does not get used */

	BIT_SET(rt->rt_state, RTS_NOTINSTALL|RTS_NOADVISE);
	errmsg = "MARTIAN will not be propagated";
    }

    for (i = 0; i < pp->rtp_n_gw; i++) {
	IFA_ALLOC(rt->rt_ifaps[i] = if_withroute(rt->rt_dest, rt->rt_routers[i], rt->rt_state));
	if (!rt->rt_ifaps[i]) {
	    /* This is an off-net gateway */

	    if (BIT_TEST(rt->rt_state, RTS_NOTINSTALL)) {
		/* Allow if flaged as not installable */

		continue;
	    }

	    /* XXX - which interface */
	    errmsg = "interface not found for";
	    goto Error;
	}

	/* If this is not an interface route, ignore routes to the destination(s) of this interface */
	if (BIT_TEST(rt->rt_state, RTS_GATEWAY)
	    && !BIT_MATCH(rt->rt_state, RTS_NOTINSTALL|RTS_NOADVISE|RTS_REJECT)
	    && if_myaddr(rt->rt_ifaps[i], rt->rt_dest, rt->rt_dest_mask)
	    && (!BIT_TEST(rt->rt_ifaps[i]->ifa_state, IFS_POINTOPOINT)
		|| !sockaddrcmp(rt->rt_dest, rt->rt_ifaps[i]->ifa_addr_local))) {
	    /* Make it unusable */

	    BIT_SET(rt->rt_state, RTS_NOTINSTALL|RTS_NOADVISE);
	    if (rt->rt_preference > 0) {
		rt->rt_preference = -rt->rt_preference;
	    }
	}
    }

    rt->rt_data = pp->rtp_rtd;
    
    rt_n_changes++;

#ifdef	PROTO_ASPATHS
    /* Create an AS path for this route */
    aspath_rt_build(rt, pp->rtp_asp);
#endif	/* PROTO_ASPATHS */

    rt_event_initialize(rt);

 Message:
    if (errmsg) {
	tracef("rt_add: %s ",
	       errmsg);
	if (BIT_TEST(pp->rtp_state, RTS_GROUP)) {
	    tracef("group %A",
		   pp->rtp_dest);
	} else {
	    tracef("%A",
		   pp->rtp_dest);
	    if (pp->rtp_dest_mask) {
		tracef("/%A",
		       pp->rtp_dest_mask);
	    }
	}
	tracef(" gw");
	for (i 	= 0; i < pp->rtp_n_gw; i++) {
	    tracef("%c%A",
		   i ? ',' : ' ',
		   pp->rtp_routers[i]);
	}

	if (pp->rtp_gwp->gw_addr && (pp->rtp_n_gw > 1 || !sockaddrcmp(pp->rtp_routers[0], pp->rtp_gwp->gw_addr))) {
	    tracef(" from %A",
		   pp->rtp_gwp->gw_addr);
	}
	
	tracef(" %s",
	       trace_state(rt_proto_bits, pp->rtp_gwp->gw_proto));
	if (pp->rtp_gwp->gw_peer_as) {
	    tracef(" AS %d",
		   pp->rtp_gwp->gw_peer_as);
	}

	trace_log_tp(tp,
		     0,
		     LOG_WARNING,
		     (NULL));
    }

#ifdef	RT_SANITY
    rt_sanity();
#endif	/* RT_SANITY */
    
    return rt;

 Error:
    if (rt) {
	if (rt->rt_rtq.rtq_forw) {
	    /* We added it to the queue, remove it */

	    REMQUE(&rt->rt_rtq);
	    rt->rt_gwp->gw_n_routes--;
	}
	(void) rt_free(rt);
	rt = (rt_entry *) 0;
    }
    goto Message;
}



 /* rt_change() changes a route &/or notes that an update was received.	*/
 /* returns 1 if change made.  Updates the kernel's routing table if	*/
 /* the router has changed, or a preference change has made another	*/
 /* route active							*/
rt_entry *
#ifdef	PROTO_ASPATHS
rt_change_aspath __PF9(rt, rt_entry *,
		       metric, metric_t,
		       metric2, metric_t,
		       tag, metric_t,
		       preference, pref_t,
		       preference2, pref_t,
		       n_gw, int,
		       gateway, sockaddr_un **,
		       asp, as_path *)
#else	/* PROTO_ASPATHS */
rt_change __PF8(rt, rt_entry *,
		metric, metric_t,
		metric2, metric_t,
		tag, metric_t,
		preference, pref_t,
		preference2, pref_t,
		n_gw, int,
		gateway, sockaddr_un **)
#endif	/* PROTO_ASPATHS */
{
    rt_changes *rtcp;
    int gateway_changed = FALSE;
    int preference_changed = FALSE;

    rt_check_open(rt->rt_gwp->gw_proto, "rt_change");

    /* Put at the end of the gateway queue and timestamp the route */
    if (!rt->rt_rtq.rtq_forw) {
	/* It was not on the list, put it on and count new route */
	
	rt->rt_gwp->gw_n_routes++;
	INSQUE(&rt->rt_rtq, rt->rt_gwp->gw_rtq.rtq_back);
	rt->rt_time = time_sec;
    }

    /* Allocate a change block if necessary */
    if (rt == rt->rt_head->rth_last_active) {
	rtcp = rt->rt_head->rth_changes;
	if (!rtcp && rt->rt_n_bitsset) {
	    rtcp = rt->rt_head->rth_changes = (rt_changes *) task_block_alloc(rtchange_block_index);
	}
    } else {
	rtcp = (rt_changes *) 0;
    }

    if (n_gw != rt->rt_n_gw
	|| gateway != rt->rt_routers) {
	int should_copy = rtcp && !BIT_TEST(rtcp->rtc_flags, RTCF_NEXTHOP);
	if_addr *ifap;
#if	RT_N_MULTIPATH > 1
	sockaddr_un **routers = gateway;
	sockaddr_un *sorted_routers[RT_N_MULTIPATH];
	if_addr *sorted_ifaps[RT_N_MULTIPATH];
	register int i;

	if (n_gw > 1) {
	    for (i = 0; i < n_gw; i++) {
		sorted_routers[i] = *gateway++;
	    }
	    rt_routers_sort(sorted_routers, n_gw);
	    routers = sorted_routers;
	}

	/*
	 * Check to see if anything has changed.
	 */
	if ((i = n_gw) == rt->rt_n_gw) {
	    while (i--) {
		if (!sockaddrcmp(rt->rt_routers[i], routers[i])) {
		    break;
		}
	    }
	}

	/*
	 * Copy his routers.  If we need to save them, do that too.
	 */
	if (i >= 0) {
	    /* Compute the ifap's for the new routes */
	    for (i = 0; i < n_gw; i++) {
		sorted_routers[i] = sockdup(*routers);
		sockclean(sorted_routers[i]);
		routers++;
		ifap = if_withroute(rt->rt_dest, sorted_routers[i], rt->rt_state);
		if (!ifap) {
		    int j;

		    trace_log_tp(rt->rt_gwp->gw_task,
				 0,
				 LOG_WARNING,
				 ("rt_change: interface not found for net %-15A gateway %A",
				  rt->rt_dest,
				  sorted_routers[i]));
		    for (j = 0; j < i; j++) {
			sockfree(sorted_routers[j]);
			IFA_FREE(sorted_ifaps[j]);
		    }
		    sockfree(sorted_routers[i]);
		    return (rt_entry *) 0;
		}
		IFA_ALLOC(sorted_ifaps[i] = ifap);
	    }

	    for (i = 0; i < n_gw; i++) {
		if (i < rt->rt_n_gw) {
		    if (should_copy) {
			rtcp->rtc_routers[i] = rt->rt_routers[i];
			rtcp->rtc_ifaps[i] = rt->rt_ifaps[i];
		    } else {
			sockfree(rt->rt_routers[i]);
			IFA_FREE(rt->rt_ifaps[i]);
		    }
		}
		rt->rt_routers[i] = sorted_routers[i];
		rt->rt_ifaps[i] = sorted_ifaps[i];
	    }
	    for ( ; i < rt->rt_n_gw; i++) {
		if (should_copy) {
		    rtcp->rtc_routers[i] = rt->rt_routers[i];
		    rtcp->rtc_ifaps[i] = rt->rt_ifaps[i];
		} else {
		    sockfree(rt->rt_routers[i]);
		    IFA_FREE(rt->rt_ifaps[i]);
		}
		rt->rt_routers[i] = NULL;
	    }
	    if (should_copy) {
		rtcp->rtc_n_gw = rt->rt_n_gw;
		rtcp->rtc_gw_sel = rt->rt_gw_sel;
		rt->rt_gw_sel = (n_gw > 1) ? (short) grand((u_int32) n_gw) : 0;
		BIT_SET(rtcp->rtc_flags, RTCF_NEXTHOP);
	    } else if (rtcp) {
		/*
		 * Check to see if we have changed the next hop back to what it was
		 * previously.  If so, delete the change information.
		 */
		if (rtcp->rtc_n_gw == n_gw) {
		    for (i = 0; i < n_gw; i++) {
			if (!sockaddrcmp(rtcp->rtc_routers[i], rt->rt_routers[i])) {
			    break;
			}
		    }
		    if (i == n_gw) {
			/*
			 * Same as before, delete change info.
			 */
			BIT_RESET(rtcp->rtc_flags, RTCF_NEXTHOP);
			rt->rt_gw_sel = rtcp->rtc_gw_sel;
			for (i = 0; i < n_gw; i++) {
			    sockfree(rtcp->rtc_routers[i]);
			    IFA_FREE(rtcp->rtc_ifaps[i]);
			    rtcp->rtc_routers[i] = NULL;
			}
			rtcp->rtc_n_gw = 0;
			rtcp->rtc_gw_sel = 0;
		    }
		}
		if (BIT_TEST(rtcp->rtc_flags, RTCF_NEXTHOP)) {
		    rt->rt_gw_sel = (n_gw > 1) ? (short) grand((u_int32) n_gw) : 0;
		}
	    }

	    rt->rt_gw_sel = (n_gw > 1) ? (short) grand((u_int32) n_gw) : 0;
	    rt->rt_n_gw = n_gw;
	    gateway_changed = TRUE;
	}
#else	/* RT_N_MULTIPATH == 1 */
	if (n_gw != rt->rt_n_gw ||
	  (n_gw > 0 && !sockaddrcmp(RT_ROUTER(rt), *gateway))) {
	    sockaddr_un *newrouter;

	    /* XXX - We check for interface change only in this case? */

	    if (n_gw > 0) {
		newrouter = sockdup(*gateway);
		sockclean(newrouter);
		ifap = if_withroute(rt->rt_dest, newrouter, rt->rt_state);
		if (!ifap) {
		    trace_log_tp(rt->rt_gwp->gw_task,
				 0,
				 LOG_WARNING,
				 ("rt_change: interface not found for net %-15A gateway %A",
				  rt->rt_dest,
				  newrouter));
		    return (rt_entry *) 0;
		}
		IFA_ALLOC(ifap);
	    } else {
		newrouter = (sockaddr_un *) 0;
		ifap = (if_addr *) 0;
	    }

	    if (should_copy) {
		if ((rtcp->rtc_n_gw = rt->rt_n_gw) > 0) {
		    rtcp->rtc_routers[0] = RT_ROUTER(rt);
		    rtcp->rtc_ifaps[0] = RT_IFAP(rt);
		}
		BIT_SET(rtcp->rtc_flags, RTCF_NEXTHOP);
	    } else {
		if (rt->rt_n_gw) {
		    sockfree(RT_ROUTER(rt));
		    IFA_FREE(RT_IFAP(rt));
		}
		if (rtcp
		  && BIT_TEST(rtcp->rtc_flags, RTCF_NEXTHOP)
		  && n_gw == rtcp->rtc_n_gw) {
		    /*
		     * If the new router is the same as the changed
		     * version, deleted the changed.
		     */
		    if (n_gw == 0) {
			BIT_RESET(rtcp->rtc_flags, RTCF_NEXTHOP);
		    } else if (sockaddrcmp(RT_ROUTER(rt),
					   rtcp->rtc_routers[0])) {
			BIT_RESET(rtcp->rtc_flags, RTCF_NEXTHOP);
			sockfree(rtcp->rtc_routers[0]);
			IFA_FREE(rtcp->rtc_ifaps[0]);
			rtcp->rtc_routers[0] = (sockaddr_un *) 0;
			rtcp->rtc_ifaps[0] = (if_addr *) 0;
			rtcp->rtc_n_gw = 0;
		    }
		}
	    }
	    RT_ROUTER(rt) = newrouter;
	    RT_IFAP(rt) = ifap;
	    rt->rt_n_gw = n_gw;
	    gateway_changed = TRUE;
	}
#endif	/* RT_N_MULTIPATH */
    }

    if (preference != rt->rt_preference) {
	rt->rt_preference = preference;
	preference_changed = TRUE;
    }
    if (preference2 != rt->rt_preference2) {
	rt->rt_preference2 = preference2;
	preference_changed = TRUE;
    }

    if (metric != rt->rt_metric) {
	if (rtcp) {
	    if (!BIT_TEST(rtcp->rtc_flags, RTCF_METRIC)) {
		BIT_SET(rtcp->rtc_flags, RTCF_METRIC);
		rtcp->rtc_metric = rt->rt_metric;
	    } else if (rtcp->rtc_metric == metric) {
		BIT_RESET(rtcp->rtc_flags, RTCF_METRIC);
	    }
	}
	rt->rt_metric = metric;
	gateway_changed = TRUE;
    }

    if (metric2 != rt->rt_metric2) {
	if (rtcp) {
	    if (!BIT_TEST(rtcp->rtc_flags, RTCF_METRIC2)) {
		BIT_SET(rtcp->rtc_flags, RTCF_METRIC2);
		rtcp->rtc_metric2 = rt->rt_metric2;
	    } else if (rtcp->rtc_metric2 == metric2) {
		BIT_RESET(rtcp->rtc_flags, RTCF_METRIC2);
	    }
	}
	rt->rt_metric2 = metric2;
	gateway_changed = TRUE;
    }

    if (tag != rt->rt_tag) {
	if (rtcp) {
	    if (!BIT_TEST(rtcp->rtc_flags, RTCF_TAG)) {
		BIT_SET(rtcp->rtc_flags, RTCF_TAG);
		rtcp->rtc_tag = rt->rt_tag;
	    } else if (rtcp->rtc_tag == tag) {
		BIT_RESET(rtcp->rtc_flags, RTCF_TAG);
	    }
	}
	rt->rt_tag = tag;
	gateway_changed = TRUE;
    }

#ifdef	PROTO_ASPATHS
    if (asp != rt->rt_aspath) {
	/* Path has changed */

	if (rtcp) {
	    if (!BIT_TEST(rtcp->rtc_flags, RTCF_ASPATH)) {
		/* Save the path, don't unlink it */
		rtcp->rtc_aspath = rt->rt_aspath;
		BIT_SET(rtcp->rtc_flags, RTCF_ASPATH);
		rt->rt_aspath = (as_path *) 0;
	    } else if (asp == rtcp->rtc_aspath) {
		/* AS path changed back.  Unlink the route's AS path then */
		/* simply transfer the changed path back to the route */
		/* This assumes an INTERNAL_IGP route will never have been active */
		if (rt->rt_aspath) {
		    aspath_rt_free(rt);
		}
		rt->rt_aspath = asp;
		rtcp->rtc_aspath = NULL;
		BIT_RESET(rtcp->rtc_flags, RTCF_ASPATH);
	    }
	}

	if (rtcp == NULL || BIT_TEST(rtcp->rtc_flags, RTCF_ASPATH)) {
	    if (rt->rt_aspath) {
		aspath_rt_free(rt);
	    }
	    aspath_rt_build(rt, asp);
	}
	    gateway_changed = TRUE;
    }
#endif	/* PROTO_ASPATHS */

    if (preference_changed || gateway_changed
	|| BIT_TEST(rt->rt_state, RTS_DELETE)) {
	rt_event_preference(rt, gateway_changed);
	rt_n_changes++;
    }

    if (rtcp && !rtcp->rtc_flags) {
	/* No changes - release */
	rtchanges_free(rt->rt_head);
    }
    
#ifdef	RT_SANITY
    rt_sanity();
#endif	/* RT_SANITY */
    return rt;
}


/* User has declared a route unreachable.  */
void
rt_delete __PF1(delete_rt, rt_entry *)
{
    rt_check_open(delete_rt->rt_gwp->gw_proto, "rt_delete");

    rt_event_unreachable(delete_rt);

    /* Remove from the queue and timestamp */
    if (delete_rt->rt_rtq.rtq_forw) {
	REMQUE(&delete_rt->rt_rtq);
	delete_rt->rt_rtq.rtq_forw = delete_rt->rt_rtq.rtq_back = (rtq_entry *) 0;
	delete_rt->rt_gwp->gw_n_routes--;
    }
    delete_rt->rt_time = time_sec;

    if (BIT_TEST(delete_rt->rt_state, RTS_RELEASE)) {
	(void) rt_free(delete_rt);
    }
    rt_n_changes++;
    
#ifdef	RT_SANITY
    rt_sanity();
#endif	/* RT_SANITY */
}


#ifdef	PROTO_INET
/**/

/*
 * Handle the internally generated default route.
 */

int rt_default_active = 0;		/* Number of requests to install default */
int rt_default_needed = 0;		/* TRUE if we need to generate a default */
static rt_entry *rt_default_rt;		/* Pointer to the default route */
rt_parms rt_default_rtparms = RTPARMS_INIT(1,
					   (metric_t) 0,
					   RTS_INTERIOR,
					   RTPREF_DEFAULT);

/*
 * This is only a pseudo route so use the loopback interface,
 * it should not go down and produce
 * undesirable side effects.
 */  
#define	RT_DEFAULT_ADD	rt_default_rt = rt_add(&rt_default_rtparms)

#define	RT_DEFAULT_DELETE	(void) rt_delete(rt_default_rt); \
    				rt_default_rt = (rt_entry *) 0

static void
rt_default_reinit  __PF1(tp, task *)
{
    if (!rt_default_rtparms.rtp_gwp) {
	rt_default_rtparms.rtp_gwp = gw_init((gw_entry *) 0,
					     RTPROTO_DEFAULT,
					     tp,
					     (as_t) 0,
					     (as_t) 0,
					     (sockaddr_un *) 0,
					     GWF_NOHOLD);
	rt_default_rtparms.rtp_dest = inet_addr_default;
	rt_default_rtparms.rtp_dest_mask = inet_mask_default;
    }
    /* Make sure we have a next hop */
    if (!rt_default_rtparms.rtp_router) {
	rt_default_rtparms.rtp_router = sockdup(inet_addr_loopback);
    }
    /* Do not install if our next hop is the loopback interface */
    if (sockaddrcmp(rt_default_rtparms.rtp_router, inet_addr_loopback)) {
	BIT_SET(rt_default_rtparms.rtp_state, RTS_NOTINSTALL);
    }

    rt_open(tp);
    
    if (rt_default_needed) {
	/* Default is enabled */

	if (rt_default_active) {
	    /* Should be installed */

	    if (!rt_default_rt) {
		/* It is not, add it */

		RT_DEFAULT_ADD;
	    } else {
		/* It exists, verify parameters */

		if (rt_default_rt->rt_preference != rt_default_rtparms.rtp_preference
		    || !sockaddrcmp(RT_ROUTER(rt_default_rt),
				    rt_default_rtparms.rtp_router)) {
		    /* Parmeters are not correct, delete and re-add */
		    
		    RT_DEFAULT_DELETE;
		    RT_DEFAULT_ADD;

		}
	    }
	}
    } else {
	/* Default is disabled */

	if (rt_default_rt) {
	    /* Get rid of current default */

	    RT_DEFAULT_DELETE;
	}
    }

    rt_close(tp, (gw_entry *) 0, 0, NULL);
}


void
rt_default_add __PF0(void)
{

    if (!rt_default_active++ && rt_default_needed) {
	/* First request to add and default is enabled, add it */

	rt_open(rt_task);
	RT_DEFAULT_ADD;
	rt_close(rt_task, (gw_entry *) 0, 0, NULL);
    }
}


void
rt_default_delete __PF0(void)
{

    if (!--rt_default_active && rt_default_rt) {
	/* Last request to delete and default is installed, remove it */

	rt_open(rt_task);
	RT_DEFAULT_DELETE;
	rt_close(rt_task, (gw_entry *) 0, 0, NULL);
    }
}


static void
rt_default_cleanup __PF1(tp, task *)
{
    rt_default_needed = FALSE;

    rt_default_rtparms.rtp_preference = RTPREF_DEFAULT;
    if (rt_default_rtparms.rtp_router) {
	sockfree(rt_default_rtparms.rtp_router);
	rt_default_rtparms.rtp_router = (sockaddr_un *) 0;
    }
    BIT_RESET(rt_default_rtparms.rtp_state, RTS_NOTINSTALL);
}


#undef	RT_DEFAULT_ADD
#undef	RT_DEFAULT_DELETE
#endif	/* PROTO_INET */


/**/
/*
 *	In preparation for a re-parse, reset the NOAGE flags on static routes
 *	so they will be deleted if they are not refreshed.
 */
/*ARGSUSED*/
static void
rt_cleanup __PF1(tp, task *)
{

    rt_static_cleanup(tp);

#ifdef	PROTO_INET
    /* Cleanup default route */
    rt_default_cleanup(tp);
#endif	/* PROTO_INET */

    /* Cleanup our tracing */
    trace_freeup(rt_task->task_trace);
}


/*
 *	Delete any static routes that do not have the NOAGE flag.
 */
/*ARGSUSED*/
static void
rt_reinit __PF1(tp, task *)
{
    /* Update tracing */
    trace_freeup(rt_task->task_trace);
    rt_task->task_trace = trace_set_global((bits *) 0, (flag_t) 0);
    
    rt_static_reinit(tp);

#ifdef	PROTO_INET
    /* Make sure default route is in the right state */
    rt_default_reinit(tp);
#endif	/* PROTO_INET */

#ifdef	PROTO_SNMP
    /* Make sure the MIB is registered */
    rt_init_mib(TRUE);
#endif	/* PROTO_SNMP */
}


/*
 *	Deal with an interface state change
 */
static void
rt_ifachange __PF2(tp, task *,
		   ifap, if_addr *)
{
    rt_static_ifachange(tp);
}


/*
 *	Do this just before shutdown
 */
static void
rt_shutdown __PF1(tp, task *)
{
    /* Update the kernel with any changes made */
    rt_flash_update((task_job *) 0);

    /* Cleanup our tracing */
    trace_freeup(rt_task->task_trace);
    
    /* And we're outa here... */
    task_delete(tp);
}


/*
 *	Terminating - clean up
 */
static void
rt_terminate __PF1(tp, task *)
{

    rt_static_terminate(tp);

#ifdef	PROTO_INET
    /* Remove the internal default route */
    rt_default_reset();
#endif	/* PROTO_INET */
}


/*
 *	Dump routing table to dump file
 */
static void
rt_dump  __PF2(tp, task *,
	       fd, FILE *)
{
    int af = 0;
    register rt_head *rth;
    register rt_list *rtl = rthlist_all(AF_UNSPEC);

    /*
     * Dump the static routes
     */
    rt_static_dump(tp, fd);
    
    /*
     * Dump the static gateways
     */
    if (rt_gw_list) {
	(void) fprintf(fd, "\tGateways referenced by static routes:\n");

	gw_dump(fd,
		"\t\t",
		rt_gw_list,
		RTPROTO_STATIC);

	(void) fprintf(fd, "\n");
    }
    (void) fprintf(fd, "\n");

    /* Print the bit allocation info */
    rtbit_dump(fd);
    
    /* Print netmasks */
    mask_dump(fd);

    /* Routing table information */
    (void) fprintf(fd, "Routing Tables:\n");

    rt_table_dump(tp, fd);
    
#ifdef	PROTO_INET
    (void) fprintf(fd,
		   "\tGenerate Default: %s\n",
		   rt_default_needed ? "yes" : "no");
#endif	/* PROTO_INET */
    (void) fprintf(fd, "\n");

    /*
     * Dump all the routing information
     */

    (void) fprintf(fd,
		   "\n\t%s\n%s\n",
		   "+ = Active Route, - = Last Active, * = Both",
#if	RT_N_MULTIPATH > 1
		   "\t* = Next hop in use\n");
#else	/* RT_N_MULTIPATH */
		   "");
#endif	/* RT_N_MULTIPATH */
    
    RT_LIST(rth, rtl, rt_head) {
	register u_int i;
	register rt_entry *rt;

	if (socktype(rth->rth_dest) != af) {
	    af = socktype(rth->rth_dest);

	    (void) fprintf(fd, "\n\tRouting table for %s (%d):\n",
			   gd_lower(trace_value(task_domain_bits, af)),
			   af);
	    (void) fprintf(fd, "\t\tDestinations: %d\tRoutes: %d\n",
			   rtaf_info[af].rtaf_dests,
			   rtaf_info[af].rtaf_routes);
	    (void) fprintf(fd, "\t\tHolddown: %d\tDelete: %d\tHidden: %d\n\n",
			   rtaf_info[af].rtaf_holddowns,
			   rtaf_info[af].rtaf_deletes,
			   rtaf_info[af].rtaf_hiddens);
	}
	
	(void) fprintf(fd,
		  "\t%-15A",
		       rth->rth_dest);
	if (!BIT_TEST(rth->rth_state, RTS_GROUP)) {
	    (void) fprintf(fd,
			   "\tmask %-15A",
			   rth->rth_dest_mask);
	}
	(void) fprintf(fd,
		  "\n\t\t\tentries %d\tannounce %d",
		       rth->rth_entries,
		       rth->rth_n_announce);
	if (rth->rth_state) {
	    (void) fprintf(fd,
			   "\tstate <%s>",
			   trace_bits(rt_state_bits, rth->rth_state));
	}
	(void) fprintf(fd, "\n");

	/* Print change information */
	if (rth->rth_changes) {
	    rt_changes *rtcp = rth->rth_changes;

	    (void) fprintf(fd, "\t\tPrevious state: %s\n",
			   trace_bits(rt_change_bits, rtcp->rtc_flags));
	    
	    if (BIT_TEST(rtcp->rtc_flags, RTCF_NEXTHOP)) {
		if (rtcp->rtc_n_gw) {
		    for (i = 0; i < rtcp->rtc_n_gw; i++) {
			if (rtcp->rtc_ifaps[i]) {
			    (void) fprintf(fd,
					   "\t\t\t%sNextHop: %-15A\tInterface: %A(%s)\n",
#if	RT_N_MULTIPATH > 1
					   i == rtcp->rtc_gw_sel ? "*" :
#endif	/* RT_N_MULTIPATH > 1 */
					   "",
					   rtcp->rtc_routers[i],
					   rtcp->rtc_ifaps[i]->ifa_addr,
					   rtcp->rtc_ifaps[i]->ifa_link->ifl_name);
			} else {
			    (void) fprintf(fd,
					   "\t\t\t%sNextHop: %-15A\n",
#if	RT_N_MULTIPATH > 1
					   i == rtcp->rtc_gw_sel ? "*" :
#endif	/* RT_N_MULTIPATH > 1 */
					   "",
					   rtcp->rtc_routers[i]);
			}
		    }
		} else {
		    (void) fprintf(fd, "\t\t\tNextHop: none\tInterface: none\n");
		}
	    }

	    if (BIT_TEST(rtcp->rtc_flags, RTCF_METRIC|RTCF_METRIC2|RTCF_TAG)) {
		(void) fprintf(fd, "\t\t");
		if (BIT_TEST(rtcp->rtc_flags, RTCF_METRIC)) {
		    (void) fprintf(fd,
				   "\tMetric: %u",
				   rtcp->rtc_metric);
		}
		if (BIT_TEST(rtcp->rtc_flags, RTCF_METRIC2)) {
		    (void) fprintf(fd,
				   "\tMetric2: %u",
				   rtcp->rtc_metric2);
		}
		if (BIT_TEST(rtcp->rtc_flags, RTCF_TAG)) {
		    (void) fprintf(fd,
				   "\tTag: %u\n",
				   rtcp->rtc_tag);
		}
		(void) fprintf(fd, "\n");
	    }
#ifdef	PROTO_ASPATHS
	    /* Format AS path */
	    if (BIT_TEST(rtcp->rtc_flags, RTCF_ASPATH) && rtcp->rtc_aspath) {
		aspath_dump(fd, rtcp->rtc_aspath, "\t\t\t", "\n");
	    }
#endif	/* PROTO_ASPATHS */
	}

	rt_aggregate_rth_dump(fd, rth);

	rttsi_dump(fd, rth);

	(void) fprintf(fd, "\n");

	RT_ALLRT(rt, rth) {
	    const char *active;

	    if (rt == rth->rth_active && rt == rth->rth_last_active) {
		active = "*";
	    } else if (rt == rth->rth_active) {
		active = "+";
	    } else if (rt == rth->rth_last_active) {
		active = "-";
	    } else {
		active = "";
	    }
	    (void) fprintf(fd,
			   "\t\t%s%s\tPreference: %3d",
			   active,
			   trace_state(rt_proto_bits, rt->rt_gwp->gw_proto),
			   rt->rt_preference);
	    if (rt->rt_preference2) {
		(void) fprintf(fd, "/%d", rt->rt_preference2);
	    }
	    if (rt->rt_gwp->gw_addr && (rt->rt_n_gw == 0
	      || !sockaddrcmp(rt->rt_gwp->gw_addr, RT_ROUTER(rt)))) {
		(void) fprintf(fd,
			       "\t\tSource: %A\n",
			       rt->rt_gwp->gw_addr);
	    } else {
		(void) fprintf(fd,
			       "\n");
	    }

	    for (i = 0; i < rt->rt_n_gw; i++) {
		if (rt->rt_ifaps[i]) {
		    (void) fprintf(fd,
				   "\t\t\t%sNextHop: %-15A\tInterface: %A(%s)\n",
#if	RT_N_MULTIPATH > 1
				   i == rt->rt_gw_sel ? "*" :
#endif	/* RT_N_MULTIPATH > 1 */
				   "",
				   rt->rt_routers[i],
				   rt->rt_ifaps[i]->ifa_addr,
				   rt->rt_ifaps[i]->ifa_link->ifl_name);
		} else {
		    (void) fprintf(fd,
				   "\t\t\t%sNextHop: %-15A\n",
#if	RT_N_MULTIPATH > 1
				   i == rt->rt_gw_sel ? "*" :
#endif	/* RT_N_MULTIPATH > 1 */
				   "",
				   rt->rt_routers[i]);
		}
	    }

	    (void) fprintf(fd,
			   "\t\t\tState: <%s>\n",
			   trace_bits(rt_state_bits, rt->rt_state));

	    if (rt->rt_gwp->gw_peer_as || rt->rt_gwp->gw_local_as) {
		(void) fprintf(fd,
			       "\t\t");
		if (rt->rt_gwp->gw_local_as) {
		    (void) fprintf(fd,
				   "\tLocal AS: %5u",
				   rt->rt_gwp->gw_local_as);
		}
		if (rt->rt_gwp->gw_peer_as) {
		    (void) fprintf(fd,
				   "\tPeer AS: %5u",
				   rt->rt_gwp->gw_peer_as);
		}
		(void) fprintf(fd,
			       "\n");
	    }
	    (void) fprintf(fd,
			   "\t\t\tAge: %#T",
			   rt_age(rt));
	    (void) fprintf(fd,
			   "\tMetric: %d\tMetric2: %d\tTag: %u\n",
			   rt->rt_metric,
			   rt->rt_metric2,
			   rt->rt_tag);

	    if (rt->rt_gwp->gw_task) {
		(void) fprintf(fd,
			       "\t\t\tTask: %s\n",
			       task_name(rt->rt_gwp->gw_task));
	    }

	    if (rt->rt_n_bitsset) {
		(void) fprintf(fd,
			       "\t\t\tAnnouncement bits(%d):",
			       rt->rt_n_bitsset);
		for (i = 1; i <= RTBIT_NBITS; i++)  {
		    if (rtbit_isset(rt, i)) {
			(void) fprintf(fd,
				       " %d-%s",
				       i,
				       task_name(rtbit_map[i-1].rtb_task));
		    }
		}
		(void) fprintf(fd,
			       "\n");
	    }

#ifdef	PROTO_ASPATHS
	    /* Format AS path */
	    if (rt->rt_aspath) {
		aspath_dump(fd, rt->rt_aspath, "\t\t\t", "\n");
	    }
#endif	/* PROTO_ASPATHS */

	    /* Format protocol specific data */
	    if (rt->rt_data && rt->rt_gwp->gw_rtd_dump) {
		rt->rt_gwp->gw_rtd_dump(fd, rt);
	    }

	    rt_aggregate_rt_dump(fd, rt);
	    
	    (void) fprintf(fd, "\n");
	} RT_ALLRT_END(rt, rth);

	(void) fprintf(fd, "\n");

    } RT_LIST_END(rth, rtl, rt_head) ;
}


/*
 *  Initialize the routing table.
 *
 *  Also creates a timer and task for the job of aging the routing table
 */
void
rt_family_init __PF0(void)
{

    /* Init the routing table */
    rt_table_init();
    
    /* Allocate the routing table task */
    rt_task = task_alloc("RT",
			 TASKPRI_RT,
			 trace_set_global((bits *) 0, (flag_t) 0));
    task_set_cleanup(rt_task, rt_cleanup);
    task_set_reinit(rt_task, rt_reinit);
    task_set_dump(rt_task, rt_dump);
    task_set_terminate(rt_task, rt_terminate);
    task_set_shutdown(rt_task, rt_shutdown);
    task_set_ifachange(rt_task, rt_ifachange);
    if (!task_create(rt_task)) {
	task_quit(EINVAL);
    }

    rt_block_index = task_block_init(sizeof (rt_entry), "rt_entry");
    rth_block_index = task_block_init(sizeof (rt_head), "rt_head");
    rtchange_block_index = task_block_init(sizeof (rt_changes), "rt_changes");
    rtlist_block_index = task_block_init((size_t) RTL_SIZE, "rt_list");
    rttsi_block_index = task_block_init(sizeof (struct _rt_tsi), "rt_tsi");

    rt_static_init(rt_task);

#ifdef	PROTO_INET
    rt_default_cleanup(rt_task);
#endif	/* PROTO_INET */

    rt_aggregate_init();

    redirect_init();
}
