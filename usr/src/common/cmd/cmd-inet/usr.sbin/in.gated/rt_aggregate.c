#ident	"@(#)rt_aggregate.c	1.3"
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
#include "parse.h"
#ifdef	PROTO_INET
#include "inet.h"
#endif	/* PROTO_INET */
#ifdef	PROTO_ISO
#include "iso.h"
#endif	/* PROTO_ISO */

/**/


/* Aggregate routes */


#ifdef	PROTO_INET
adv_entry *aggregate_list_inet;		/* Aggregation policy */
#endif	/* PROTO_INET */
#ifdef	PROTO_ISO
adv_entry *aggregate_list_iso;		/* Aggregation policy */
#endif	/* PROTO_ISO */

struct rt_aggregate_family {
    u_int rtaf_family;
    adv_entry **rtaf_list;
    u_int rtaf_depth;
};

static struct rt_aggregate_family rt_aggregate_families[] = {
#ifdef	PROTO_INET
    { AF_INET, &aggregate_list_inet },
#endif	/* PROTO_INET */
#ifdef	PROTO_ISO
    { AF_ISO, &aggregate_list_iso },
#endif	/* PROTO_ISO */
    { 0 }
};

#define	AGGR_FAMILIES(afp) \
	{ \
	     struct rt_aggregate_family *(afp) = rt_aggregate_families; \
	     do

#define	AGGR_FAMILIES_END(afp) \
	     while ((++afp)->rtaf_family); \
	}

static block_t rt_aggregate_entry_block = (block_t) 0;
static block_t rt_aggregate_head_block = (block_t) 0;
static gw_entry *rt_aggregate_gwp = (gw_entry *) 0;
static task *rt_aggregate_task = (task *) 0;
static rt_parms rt_aggregate_rtparms = { 0 } ;

static const bits rt_aggregate_flag_bits[] = {
    { RTAHF_BRIEF,	"Brief" },
    { RTAHF_CHANGED,	"Changed" },
    { RTAHF_ASPCHANGED,	"ASPathChanged" },
    { RTAHF_ONLIST,	"OnList" },
    { RTAHF_GENERATE,	"Generate" },
    { 0, NULL }
};


/**/

#ifndef	PROTO_ASPATHS
#define	aspath_aggregate_changed(rtah, oasp, nasp)	0
#endif	/* PROTO_ASPATHS */

#define	RT_CHANGE(art, rtah, old_asp, new_asp, list) \
	do { \
	     if (aspath_aggregate_changed((rtah), (old_asp), (new_asp)) \
		 || (art)->rt_preference != (rtah)->rtah_rta_forw->rta_preference) { \
		 BIT_SET((rtah)->rtah_flags, RTAHF_CHANGED); \
	     } else if (BIT_COMPARE((rtah)->rtah_flags, RTAHF_GENERATE|RTAHF_CHANGED, RTAHF_GENERATE)) { \
	         register rt_entry *Xrt = (rtah)->rtah_rta_forw->rta_rt; \
		 register int Xi = (art)->rt_n_gw; \
		 while (Xi--) { \
		     if (!sockaddrcmp((art)->rt_routers[Xi], Xrt->rt_routers[Xi])) { \
			 BIT_SET((rtah)->rtah_flags, RTAHF_CHANGED); \
			 break; \
		     } \
		 } \
	     } \
	     if (BIT_COMPARE((rtah)->rtah_flags, RTAHF_CHANGED|RTAHF_ONLIST, RTAHF_CHANGED)) { \
		 RTLIST_ADD(list, (art)->rt_head); \
		 BIT_SET((rtah)->rtah_flags, RTAHF_ONLIST); \
	     } \
	 } while(0)


/* This here routine re-evaluates routes that may contribute to aggregate routes. */
/* It is a bit too complex in order to avoid any unnecessary routine calls and be */
/* as fast as possible.  It does recurse, but only only level deep. */
void
rt_aggregate_flash __PF2(list, rt_list *,
			 starting_depth, u_int)
{
    register rt_head *rth;
    static rt_list *new;
    task *tp = rt_aggregate_task;
    register u_int depth = starting_depth;

    if (!tp) {
	/* We must be shutting down */
	return;
    }

    if (!starting_depth) {
	new = (rt_list *) 0;
	rt_open(tp);
    }
    
    RT_LIST(rth, list, rt_head) {

	if (!starting_depth && rth->rth_aggregate_depth) {
	    rt_entry *aggr_rt;
	    rt_aggr_head *rtah = (rt_aggr_head *) 0;

	    /* Find our aggregate */
	    RT_ALLRT(aggr_rt, rth) {
		if (aggr_rt->rt_gwp == rt_aggregate_gwp) {
		    /* Found it */

		    rtah = rt_aggregate_head(aggr_rt);
		    break;
		}
	    } RT_ALLRT_END(aggr_rt, rth) ;
	    assert(rtah);

	    /* Add to our private list */

	    if (!BIT_TEST(rtah->rtah_flags, RTAHF_ONLIST)) {
		RTLIST_ADD(new, rth);
		BIT_SET(rtah->rtah_flags, RTAHF_ONLIST);
	    }
	} else {
	    /* If this is an aggregate, check to see if it has changed */
	    if (starting_depth
		&& depth == rth->rth_aggregate_depth) {
		rt_entry *aggr_rt;
		rt_aggr_head *rtah = (rt_aggr_head *) 0;

		/* Find our aggregate */
		RT_ALLRT(aggr_rt, rth) {
		    if (aggr_rt->rt_gwp == rt_aggregate_gwp) {
			/* Found it */

			rtah = rt_aggregate_head(aggr_rt);
			break;
		    }
		} RT_ALLRT_END(aggr_rt, rth) ;
		assert(rtah && BIT_TEST(rtah->rtah_flags, RTAHF_ONLIST));

		if (BIT_TEST(rtah->rtah_flags, RTAHF_CHANGED)) {
		    int n_gw;
		    sockaddr_un **routers = (sockaddr_un **) 0;
		    rt_entry *rt = rtah->rtah_rta_forw->rta_rt;
#ifdef	PROTO_ASPATHS
		    as_path *asp;
#endif	/* PROTO_ASPATHS */

		    if (BIT_TEST(rtah->rtah_flags, RTAHF_GENERATE)) {
			/* A generated route needs next hops */
			
			if (rt == aggr_rt) {
			    n_gw = 0;
			} else if (!BIT_TEST(rt->rt_state, RTS_GATEWAY)) {
			    /* Interface routes are special */

			    if (rt->rt_n_gw
				&& BIT_TEST(RT_IFAP(rt)->ifa_state, IFS_POINTOPOINT)) {
				/* On a P2P interface we need to point at the remote address */

				n_gw = 1;
				routers = &RT_IFAP(rt)->ifa_addr;
			    } else {
				/* Other interfaces just end up with an aggregate route */

				n_gw = 0;
			    }
			} else {
			    /* Not an interface route, use it's next hops */
			    
			    n_gw = rt->rt_n_gw;
			    routers = rt->rt_routers;
			}
		    } else {
			n_gw = 0;
		    }
#ifdef	PROTO_ASPATHS
		    asp = aspath_do_aggregation(rtah);
#endif	/* PROTO_ASPATHS */

		    if ((n_gw != 0) != (aggr_rt->rt_n_gw != 0)) {
			rt_entry *old_rt = aggr_rt;

			/* Changing to and from a REJECT route requires a delete and re-add */

			/* Set REJECT flag */
			if (n_gw) {
			    BIT_RESET(rt_aggregate_rtparms.rtp_state, RTS_REJECT);
			} else {
			    BIT_SET(rt_aggregate_rtparms.rtp_state, RTS_REJECT);
			}

			rt_aggregate_rtparms.rtp_dest = aggr_rt->rt_dest;
			rt_aggregate_rtparms.rtp_dest_mask = aggr_rt->rt_dest_mask;
			rt_aggregate_rtparms.rtp_rtd = aggr_rt->rt_data;
			rt_aggregate_rtparms.rtp_n_gw = n_gw;
			bzero((caddr_t) rt_aggregate_rtparms.rtp_routers, sizeof rt_aggregate_rtparms.rtp_routers);
			if (n_gw) {
#if	RT_N_MULTIPATH > 1
			    register int i = n_gw - 1;

			    do {
				rt_aggregate_rtparms.rtp_routers[i] = routers[i];
			    } while (i--) ;
#else	/* RT_N_MULTIPATH == 1 */
			    rt_aggregate_rtparms.rtp_router = *routers;
#endif	/* RT_N_MULTIPATH */
			}
			rt_aggregate_rtparms.rtp_tag = aggr_rt->rt_tag;
			rt_aggregate_rtparms.rtp_metric = aggr_rt->rt_metric;
			rt_aggregate_rtparms.rtp_metric2 = aggr_rt->rt_metric2;
			rt_aggregate_rtparms.rtp_preference =
				rtah->rtah_rta_forw->rta_preference;
			rt_aggregate_rtparms.rtp_preference2 =
				aggr_rt->rt_preference2;
#ifdef	PROTO_ASPATHS
			rt_aggregate_rtparms.rtp_asp = asp;
#endif	/* PROTO_ASPATHS */
			
			aggr_rt = rtah->rtah_rta_rt =
				rt_add(&rt_aggregate_rtparms);
			assert(aggr_rt);

                        /*
                         * Since we are deleting and re-adding the route, we
			 * must update the corresponding policy (adv_entry)
			 * to point to the new route.
                         */
                        AGGR_FAMILIES(afp) {
                            /* Find aggregate route address family. */
                            if (afp->rtaf_family == socktype(old_rt->rt_dest)) {
                                register dest_mask_internal *dmi;

                                if (!*afp->rtaf_list) {
                                    continue;
                                }
                                /* Walk the aggregate list looking for adv
				 * entries matching the old route
				 */
                                DMI_WALK_ALL( (*afp->rtaf_list)->adv_dm.dm_internal, dmi, aggr_adv) {
                                    if (aggr_adv->adv_result.res_void ==
							(void_t) old_rt) {
                                        aggr_adv->adv_result.res_void =
						(void_t) aggr_rt;
                                        break;
                                     }
                                 }
                                 DMI_WALK_ALL_END( (*afp->rtaf_list)->adv_dm.dm_internal, dmi, aggr_adv) ;
                            }
                        } AGGR_FAMILIES_END(afp) ;

			rt_delete(old_rt);
		    } else {

			aggr_rt = rt_change_aspath(aggr_rt,
						   aggr_rt->rt_metric,
						   aggr_rt->rt_metric2,
						   aggr_rt->rt_tag,
						   rtah->rtah_rta_forw->rta_preference,
						   (pref_t) 0,
						   n_gw, routers,
						   asp);
			rt_refresh(aggr_rt);
		    }
#ifdef	PROTO_ASPATHS
		    if (asp) {
			ASPATH_FREE(asp);
		    }
#endif	/* PROTO_ASPATHS */
		    assert(aggr_rt);
		}

		/* Zero this pointer so we will skip this route later */
		BIT_RESET(rtah->rtah_flags, RTAHF_ONLIST|RTAHF_CHANGED);
		RTLIST_REMOVE(list);
	    }

	    /* See if this route contributes to any aggregates */
	    if (!starting_depth
		|| depth == rth->rth_aggregate_depth) {
		rt_entry *new_rt = rth->rth_active;
		rt_entry *old_rt = rth->rth_last_active;
#ifdef	PROTO_ASPATHS
		as_path *new_asp, *old_asp;
#endif	/* PROTO_ASPATHS */
		rt_entry *aggr_rt = (rt_entry *) 0;
		dest_mask *dm;
		pref_t preference = (pref_t) RTPREF_AGGREGATE;
		int add, delete;

		/* Check for this one as a contributor */

		/* We only consider the old route if we set our mark on it */
		if (old_rt
		    && !rtbit_isset(old_rt, tp->task_rtbit)) {
		    old_rt = (rt_entry *) 0;
		}

		/* If we have a new route, search policy for a match */
		if (new_rt
		    && (!new_rt->rt_n_gw
			|| !BIT_TEST(RT_IFAP(new_rt)->ifa_state, IFS_LOOPBACK))) {
		    adv_entry *aggr;

		    AGGR_FAMILIES(afp) {
			/* Skip families that don't apply */
			if (afp->rtaf_family == socktype(rth->rth_dest)
			    && (aggr = adv_aggregate_match(*afp->rtaf_list, new_rt, &preference))) {
			    /* Found one */
			    
			    dm = &aggr->adv_dm;
			    aggr_rt = (rt_entry *) aggr->adv_result.res_void;
#ifdef	PROTO_ASPATHS
			    new_asp = new_rt->rt_aspath;
#endif	/* PROTO_ASPATHS */
			    goto Done;
			}
		    } AGGR_FAMILIES_END(afp) ;
		}

		/* No match */
		dm = (dest_mask *) 0;
#ifdef	PROTO_ASPATHS
		new_asp = (as_path *) 0;
#endif	/* PROTO_ASPATHS */

	    Done: ;
#ifdef	PROTO_ASPATHS
		if (old_rt) {
		    rt_changes *rtcp = rth->rth_changes;

		    if (rtcp && BIT_TEST(rtcp->rtc_flags, RTCF_ASPATH)) {
			old_asp = rtcp->rtc_aspath;
		    } else {
			old_asp = old_rt->rt_aspath;
		    }
		} else {
		    old_asp = (as_path *) 0;
		}
#endif	/* PROTO_ASPATHS */

		if (!old_rt) {
		    /* New route */
		    
		    add = TRUE;
		    delete = FALSE;
		} else if (!new_rt || !dm) {
		    /* Old route, no new route, or no policy */

		    add = FALSE;
		    delete = TRUE;
		} else {
		    rt_aggr_entry *rta = rth->rth_aggregate;

		    /* Change */

		    if (aggr_rt == rta->rta_head->rtah_rta_rt) {
			/* Same aggregate */

			if (old_rt != new_rt) {
			    /* Need to set bit in correct route and fix */
			    /* router pointer in aggregate structure */
			
			    rtbit_set(new_rt, tp->task_rtbit);
			    rtbit_reset(old_rt, tp->task_rtbit);
			    rta->rta_rt = new_rt;
			}

			/* Update the preference */
			rta->rta_preference = preference;

			RT_CHANGE(aggr_rt, rt_aggregate_head(aggr_rt), old_asp, new_asp, new);
			add = delete = FALSE;
		    } else {
			/* Delete the old one and add a new one */

			delete = add = TRUE;
		    }
		}

		if (delete) {
		    rt_aggr_entry *rta = rth->rth_aggregate;
		
		    /* Delete this route from contributor list */

		    /* Release the route */
		    rtbit_reset(old_rt, tp->task_rtbit);

		    /* Remove this entry from the queue */
		    REMQUE(rta);

		    /* Update the aggregate route */
		    RT_CHANGE(rta->rta_head->rtah_rta_rt, rta->rta_head,
			old_asp, 0, new);

		    /* Free the block and reset pointers to it */
		    task_block_free(rt_aggregate_entry_block, (void_t) rta);
		    rth->rth_aggregate = (rt_aggr_entry *) 0;
		}

		if (add && dm) {
		    register rt_aggr_entry *rta1;
		    rt_aggr_entry *rta;
		    rt_aggr_head *rtah;

		    /* Add this route to the contributor list */

		    /* We stored the aggregate route pointer in the policy structure */
		    assert(aggr_rt);

		    /* Get aggregate head pointer */
		    rtah = rt_aggregate_head(aggr_rt);

		    /* Head better be there and this route must not be a contributor */
		    assert(rtah && !rth->rth_aggregate);

		    /* Get a block */
		    rth->rth_aggregate = rta = (rt_aggr_entry *) task_block_alloc(rt_aggregate_entry_block);

		    /* Set our bit on this route so it does not go away */
		    /* without us noticing */
		    rtbit_set(new_rt, tp->task_rtbit);

		    /* Link the list entry to us */
		    rta->rta_rt = new_rt;

		    /* Set the preference */
		    rta->rta_preference = preference;

		    /* Find a good place, insert us and point back to the head */
		    for (rta1 = rtah->rtah_rta.rta_back; rta1 != &rtah->rtah_rta; rta1 = rta1->rta_back) {
			if (rta1->rta_preference < preference
			    || (rta1->rta_preference == preference
				&& rta1->rta_rt->rt_preference < new_rt->rt_preference)) {
			    /* Insert after this one */

			    break;
			}
		    }
		    INSQUE(rta, rta1);
		    rta->rta_head = rtah;

		    RT_CHANGE(aggr_rt, rtah, 0, new_asp, new);
		}
	    }
	}
    } RT_LIST_END(rth, list, rt_head) ;

    /* If not recursing, process any aggregates we have on our */
    /* private list.  Then close the routing table. */
    if (!starting_depth) {

	if (new) {
	    while (new->rtl_root->rtl_count) {
		/* At end of flash list, go down a level */

		rt_aggregate_flash(new->rtl_root, ++depth);
	    }

	    /* Free the list */
	    RTLIST_RESET(new);
	}

	rt_close(tp, (gw_entry *) 0, 0, NULL);
    }
}


static void
rt_aggregate_delete __PF1(aggr_rt, rt_entry *)
{
    register rt_aggr_head *rtah = (rt_aggr_head *) rt_aggregate_head(aggr_rt);
    register rt_aggr_entry *rta;
    task *tp = rt_aggregate_task;
    
    /* Remove all routes from list */
    AGGR_LIST(&rtah->rtah_rta, rta) {
	rt_entry *rt = rta->rta_rt;
	rt_head *rth = rt->rt_head;

	rtbit_reset(rt, tp->task_rtbit);
	REMQUE(rta);
	task_block_free(rt_aggregate_entry_block, (void_t) rta);
	(rth)->rth_aggregate = (rt_aggr_entry *) 0;
    } AGGR_LIST_END(&rtah->rtah_rta, rta) ;

    /* Indicate that there is no longer an aggregate for this destination */
    aggr_rt->rt_head->rth_aggregate_depth = 0;

#ifdef	PROTO_ASPATHS
    aspath_aggregate_free(rtah);
#endif	/* PROTO_ASPATHS */

    task_block_free(rt_aggregate_head_block, aggr_rt->rt_data);
    aggr_rt->rt_data = (void_t) 0;

    rt_delete(aggr_rt);
}


static void
rt_aggregate_reinit __PF1(tp, task *)
{
    register rt_entry *rt;
    rtq_entry rtq;

    /* Update tracing */
    trace_freeup(tp->task_trace);
    tp->task_trace = trace_set_global((bits *) 0, (flag_t) 0);

    /* Save the list of routes */
    RTQ_MOVE(rt_aggregate_gwp->gw_rtq, rtq);

    /* Re-init the rt_add parameters; rt_aggregate_init() is only called once
     */
    bzero(&rt_aggregate_rtparms, sizeof(rt_aggregate_rtparms));
    rt_aggregate_rtparms.rtp_gwp = rt_aggregate_gwp;
    rt_aggregate_rtparms.rtp_state = RTS_INTERIOR | RTS_REJECT;
    rt_aggregate_rtparms.rtp_preference = (pref_t) -1;

    rt_open(tp);

    /* Verify that the routes we have all have policy */
    AGGR_FAMILIES(afp) {
	register dest_mask_internal *dmi;
	
	if (!*afp->rtaf_list) {
	    continue;
	}
	
	/* Calculate the depths of all nodes on the tree */
	adv_destmask_depth(*afp->rtaf_list);

	/* Walk the aggregate list and look up the route */
	DMI_WALK_ALL((*afp->rtaf_list)->adv_dm.dm_internal, dmi, aggr) {
	    u_int depth = aggr->adv_result.res_metric;

	    afp->rtaf_depth = MAX(afp->rtaf_depth, depth);
	    
	    rt = rt_locate_gw(RTS_NETROUTE,
			      aggr->adv_dm.dm_dest,
			      aggr->adv_dm.dm_mask,
			      rt_aggregate_rtparms.rtp_gwp);
	    if (rt) {
		flag_t flags = 0;
		rt_aggr_head *rtah = rt_aggregate_head(rt);
		
		/* Update the route */

		/* A refresh will move it off our list */
		rt_refresh(rt);
		
		/* Update a few things */
		if (BIT_TEST(aggr->adv_flag, ADVF_AGGR_BRIEF)) {
		    BIT_SET(flags, RTAHF_BRIEF);
		}
		if (BIT_TEST(aggr->adv_flag, ADVF_AGGR_GENERATE)) {
		    BIT_SET(flags, RTAHF_GENERATE);
		}

		if (!BIT_MASK_MATCH(flags, rtah->rtah_flags, RTAHF_BRIEF|RTAHF_GENERATE)) {
		    BIT_RESET(rtah->rtah_flags, RTAHF_BRIEF|RTAHF_GENERATE);
		    BIT_SET(rtah->rtah_flags, RTAHF_CHANGED|flags);
		}
	    } else {	    
		rt_aggr_head *rtah;

		/* Need to add a route */

		rt_aggregate_rtparms.rtp_dest = aggr->adv_dm.dm_dest;
		rt_aggregate_rtparms.rtp_dest_mask = aggr->adv_dm.dm_mask;

		/* Allocate rt_data info plus head of list */
		rtah = (rt_aggr_head *) (rt_aggregate_rtparms.rtp_rtd = task_block_alloc(rt_aggregate_head_block));
		rtah->rtah_rta_forw = rtah->rtah_rta_back = &rtah->rtah_rta;
		rtah->rtah_rta_preference = (pref_t) -1;	/* So aggr_rt becomes hidden */
		if (BIT_TEST(aggr->adv_flag, ADVF_AGGR_BRIEF)) {
		    BIT_SET(rtah->rtah_flags, RTAHF_BRIEF);
		}
		if (BIT_TEST(aggr->adv_flag, ADVF_AGGR_GENERATE)) {
		    BIT_SET(rtah->rtah_flags, RTAHF_GENERATE);
		}

		/* Now add it to the routing table */
		rt = rtah->rtah_rta_rt = rt_add(&rt_aggregate_rtparms);
		assert(rt);
	    }

	    /* Save the pointer to the route */
	    aggr->adv_result.res_void = (void_t) rt;

	    /* Save the depth */
	    rt->rt_head->rth_aggregate_depth = depth;
	} DMI_WALK_ALL_END((*afp->rtaf_list)->adv_dm.dm_internal, dmi, aggr) ;

    } AGGR_FAMILIES_END(afp) ;

    /* Now delete any routes that were not refreshed */
    RTQ_LIST(&rtq, rt) {
	rt_aggregate_delete(rt);
    } RTQ_LIST_END(&rtq, rt);

    rt_close(tp, (gw_entry *) 0, 0, NULL);
}


static void
rt_aggregate_cleanup __PF1(tp, task *)
{

    AGGR_FAMILIES(afp) {
	if (*afp->rtaf_list) {
	    adv_free_list(*afp->rtaf_list);
	    *afp->rtaf_list = (adv_entry *) 0;
	}
	afp->rtaf_depth = 0;
    } AGGR_FAMILIES_END(afp) ;

    
    /* Cleanup our tracing */
    trace_freeup(tp->task_trace);
}


static void
rt_aggregate_terminate __PF1(tp, task *)
{
    register rt_entry *rt;

    rt_open(tp);
    
    /* Delete all our routes */
    RTQ_LIST(&rt_aggregate_gwp->gw_rtq, rt) {
	rt_aggregate_delete(rt);
    } RTQ_LIST_END(&rt_aggregate_gwp->gw_rtq, rt) ;

    rt_close(tp, (gw_entry *) 0, 0, NULL);

    /* Free policy and other cleanup */
    rt_aggregate_cleanup(tp);
    
    task_delete(tp);
    rt_aggregate_task = (task *) 0;
}


/**/

static void
rt_aggregate_dump __PF2(tp, task *,
			fp, FILE *)
{

    AGGR_FAMILIES(afp) {
	register dest_mask_internal *dmi;

	if (!*afp->rtaf_list) {
	    continue;
	}
	
	(void) fprintf(fp, "\tAggregation policy for %s, maximum depth %u:\n\n",
		       trace_state(task_domain_bits, afp->rtaf_family),
		       afp->rtaf_depth);

	DMI_WALK_ALL((*afp->rtaf_list)->adv_dm.dm_internal, dmi, aggr) {
	    adv_entry *proto;

	    (void) fprintf(fp, "\t\t%A/%A",
			   aggr->adv_dm.dm_dest,
			   aggr->adv_dm.dm_mask);
	    if (BIT_TEST(aggr->adv_flag, ADVF_AGGR_BRIEF)) {
		(void) fprintf(fp, " brief");
	    }
	    if (BIT_TEST(aggr->adv_flag, ADVF_AGGR_GENERATE)) {
		(void) fprintf(fp, " generate");
	    }
	    if (BIT_TEST(aggr->adv_flag, ADVF_NO)) {
		(void) fprintf(fp,
			       " restrict\n");
	    } else if (BIT_TEST(aggr->adv_flag, ADVFOT_PREFERENCE)) {
		(void) fprintf(fp,
			       " preference %d\n",
			       aggr->adv_result.res_preference);
	    } else {
		(void) fprintf(fp, "\n");
	    }

	    ADV_LIST(aggr->adv_list, proto) {
		(void) fprintf(fp, "\t\t\tproto %s",
			       trace_state(rt_proto_bits, proto->adv_proto));

		if (BIT_TEST(proto->adv_flag, ADVF_NO)) {
		    (void) fprintf(fp,
				   " restrict\n");
		} else if (BIT_TEST(proto->adv_flag, ADVFOT_PREFERENCE)) {
		    (void) fprintf(fp,
				   " preference %d\n",
				   proto->adv_result.res_preference);
		} else {
		    (void) fprintf(fp, "\n");
		}

		control_dmlist_dump(fp,
				    4,
				    proto->adv_list,
				    (adv_entry *) 0,
				    (adv_entry *) 0);

	    } ADV_LIST_END(aggr->adv_list, proto) ;
	} DMI_WALK_ALL_END((*afp->rtaf_list)->adv_dm.dm_internal, dmi, aggr) ;

	fprintf(fp, "\n");

    } AGGR_FAMILIES_END(afp) ;

    fprintf(fp, "\n");
}


void
rt_aggregate_rth_dump __PF2(fp, FILE *,
			    rth, rt_head *)
{
    if (rth->rth_aggregate_depth) {
	(void) fprintf(fp,
		       "\t\t\tAggregate Depth: %u\n",
		       rth->rth_aggregate_depth);
    }
    if (rth->rth_aggregate) {
	rt_entry *aggr_rt = rth->rth_aggregate->rta_head->rtah_rta_rt;
		
	(void) fprintf(fp,
		       "\t\t\tAggregate: %A mask %A metric %u preference %d\n",
		       aggr_rt->rt_dest,
		       aggr_rt->rt_dest_mask,
		       aggr_rt->rt_metric,
		       aggr_rt->rt_preference);
    }
}


void
rt_aggregate_rt_dump __PF2(fp, FILE *,
			   rt, rt_entry *)
{
    if (rt->rt_gwp == rt_aggregate_gwp) {
	rt_aggr_entry *rta;
	rt_aggr_head *rtah = rt_aggregate_head(rt);
	int first = TRUE;

	if (rtah->rtah_flags) {
	    (void) fprintf(fp,
			   "\t\t\tFlags: %s\n",
			   trace_bits(rt_aggregate_flag_bits, rtah->rtah_flags));
	}

#ifdef	PROTO_ASPATHS
	if (rtah->rtah_aplp) {
	    aspath_list_dump(fp, rtah);
	}
#endif	/* PROTO_ASPATHS */

	AGGR_LIST(&rtah->rtah_rta, rta) {
	    if (first) {
		first = FALSE;
		(void) fprintf(fp,
			       "\t\t\tContributing Routes:\n");
	    }

	    fprintf(fp,
		    "\t\t\t\t%-15A mask %-15A  proto %s  metric %d preference %d\n",
		    rta->rta_rt->rt_dest,
		    rta->rta_rt->rt_dest_mask,
		    trace_state(rt_proto_bits, rta->rta_rt->rt_gwp->gw_proto),
		    rta->rta_rt->rt_metric,
		    rta->rta_rt->rt_head->rth_aggregate->rta_preference);
	} AGGR_LIST_END(&rtah->rtah_rta, rta);
    }
}

/**/


void
rt_aggregate_init __PF0(void)
{
    task *tp;
    
    /* Allocate the routing table task */
    tp = task_alloc("Aggregate",
		    TASKPRI_RT,
		    trace_set_global((bits *) 0, (flag_t) 0));
    task_set_cleanup(tp, rt_aggregate_cleanup);
    task_set_reinit(tp, rt_aggregate_reinit);
    task_set_dump(tp, rt_aggregate_dump);
    task_set_terminate(tp, rt_aggregate_terminate);
    tp->task_rtbit = rtbit_alloc(tp,
				 FALSE,
				 (size_t) 0,
				 (void_t) 0,
				 (PROTOTYPE((*),
					    void,
					    (FILE *,
					     rt_head *,
					     void_t,
					     const char *))) 0);
    if (!task_create(tp)) {
	task_quit(EINVAL);
    }

    rt_aggregate_task = tp;

    rt_aggregate_gwp = gw_init((gw_entry *) 0,
			       RTPROTO_AGGREGATE,
			       tp,
			       (as_t) 0,
			       (as_t) 0,
			       (sockaddr_un *) 0,
			       GWF_NOHOLD);
    
    /* Init the rt_add init parameters */
    rt_aggregate_rtparms.rtp_n_gw = 0;
    rt_aggregate_rtparms.rtp_gwp = rt_aggregate_gwp;
    rt_aggregate_rtparms.rtp_metric = (metric_t) 0;
    rt_aggregate_rtparms.rtp_tag = (metric_t) 0;
    rt_aggregate_rtparms.rtp_state = RTS_INTERIOR | RTS_REJECT;
    rt_aggregate_rtparms.rtp_preference = (pref_t) -1;

    rt_aggregate_entry_block = task_block_init(sizeof (rt_aggr_entry), "rt_aggr_entry");
    rt_aggregate_head_block = task_block_init(sizeof (rt_aggr_head), "rt_aggr_head");
}


