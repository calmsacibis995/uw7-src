#ident	"@(#)egp_rt.c	1.4"
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


#include "include.h"
#include "inet.h"
#include "egp.h"

static block_t blockindex_gateway;
static block_t blockindex_distance;
static block_t blockindex_route;


/* Insert this route into the network list for this peer */
static void
net_insert __PF4(ngp, egp_neighbor *,
		 new_rp, egp_route *,
		 ifap, if_addr *,
		 distance, metric_t)
{
    egp_dist *dp;
    egp_gwentry *gwp, *gw_head;
    sockaddr_un *gateway;
    rt_entry *rt = new_rp->route_rt;
    int exterior = 0;
    int gw_is_me = 0;
    int add_gateway = 0;
    int add_distance = 0;

    /* XXX - What about OSPF ASEs?  Should I look at the tag field */

    /* Figure out the gateway and whether it is interior or exterior */
    if (!ifap || RT_IFAP(rt) == ifap) {
	if (RT_ROUTER(rt)) {
	    /* gateway is on polled net */
	    
	    gateway = RT_ROUTER(rt);
	} else {
	    /* gateway should be myself, use default until we have a polled network */

	    gateway = inet_addr_default;
	}

	if (rt->rt_gwp->gw_peer_as && (rt->rt_gwp->gw_peer_as != inet_autonomous_system)) {
	    /* Use exterior list */

	    exterior++;
	}
    } else {
	/* Use our address on polled net */
	gateway = ifap->ifa_addr_local;
	gw_is_me++;
    }

    /* Make sure there is a head pointer */
    gw_head = ngp->ng_net_gw[exterior];
    if (!gw_head) {
	gw_head = ngp->ng_net_gw[exterior] = (egp_gwentry *) task_block_alloc(blockindex_gateway);

	/* Setup announcement list head pointers */
	gw_head->gwe_forw = gw_head->gwe_back = gw_head;
    }

    if (gw_is_me) {
	if (gw_head->gwe_forw->gwe_addr.s_addr == sock2ip(gateway)) {
	    /* We are installed at the beginning of the list */

	    gwp = gw_head->gwe_forw;
	    goto found_gateway;
	}

	/* Insert ourselves at the beginning of the list */
	gwp = gw_head;
    } else {
	/* Look for our gateway */

	EGP_GW_LIST(gwp, gw_head) {
	    if (gwp->gwe_addr.s_addr == sock2ip(gateway)) {
		/* We found our gateway */

		goto found_gateway;
	    }
	} EGP_GW_LIST_END(gwp, gw_head) ;

	/* Insert at end of list */
	gwp = gwp->gwe_back;
    }

    /* New gateway */
    INSQUE(task_block_alloc(blockindex_gateway), gwp);
    gwp = gwp->gwe_forw;
	   
    /* Init distance head pointer */
    gwp->gwe_distances.dist_forw = gwp->gwe_distances.dist_back = &gwp->gwe_distances;
    gwp->gwe_addr = sock2in(gateway);	/* struct copy */

    /* Remember to add the size of our gateway */
    add_gateway++;

 found_gateway:
    /* Try to locate the distance for this gateway */
    EGP_DIST_LIST(dp, &gwp->gwe_distances) {
	if (dp->dist_distance > distance) {
	    /* Insert before this one */
	    break;
	} else if (dp->dist_distance == distance) {
	    /* Found an empty distance */

	    goto found_distance;
	}
    } EGP_DIST_LIST_END(dp, &gwp->gwe_distances) ;

    /* Insert before this one */
    dp = dp->dist_back;

    /* Our distance does not exist;  make a new distance */
    INSQUE(task_block_alloc(blockindex_distance), dp);
    dp = dp->dist_forw;

    /* Init route head pointer */
    dp->dist_routes.route_forw = dp->dist_routes.route_back = &dp->dist_routes;
    dp->dist_gwentry = gwp;
    dp->dist_distance = distance;

    /* Remember to add length of distance */
    add_distance++;

    
 found_distance:
    new_rp->route_dist = dp;

    /* Add length of network */
    switch (inet_class_of(rt->rt_dest)) {
    case INET_CLASSC_C:
    case INET_CLASSC_E:
    case INET_CLASSC_C_SHARP:
	ngp->ng_length += 3;
	break;
				
    case INET_CLASSC_B:
	ngp->ng_length += 2;
	break;
				
    case INET_CLASSC_A:
	ngp->ng_length += 1;
	break;
    }

    /* Add this route and check for route overflow */
    if (!(++dp->dist_n_routes % EGP_ROUTES_MAX)) {
	add_distance++;
    }

    /* Insert at the end of the list */
    INSQUE(new_rp, dp->dist_routes.route_back);

    /* And fix up the counts */
    if (add_distance) {

	/* Add this distance and check for distance overflow */
	if (!(++gwp->gwe_n_distances % EGP_DISTANCE_MAX)) {
	    add_gateway++;
	}

	/* Add distance and net count to length */
	ngp->ng_length += 2;
    }
    
    if (add_gateway) {
	/* Add size of gateway and distance count to length */
	switch (inet_class_of(gateway)) {
	case INET_CLASSC_A:
	    ngp->ng_length += 4;
	    break;
		    
	case INET_CLASSC_B:
	case INET_CLASSC_E:
	case INET_CLASSC_C_SHARP:
	    ngp->ng_length += 3;
	    break;

	case INET_CLASSC_C:
	    ngp->ng_length += 2;
	    break;
	}
    }

#ifdef	DEBUG
    trace_tp(ngp->ng_task,
	     TR_EGP_DEBUG,
	     0,
	     ("net_insert: peer %A AS %u net %A proto %s distance %d gateway %A length %d",
	      ngp->ng_addr,
	      ngp->ng_peer_as,
	      new_rp->route_rt->rt_dest,
	      trace_state(rt_proto_bits, new_rp->route_rt->rt_gwp->gw_proto),
	      dp->dist_distance,
	      sockbuild_in(0, gwp->gwe_addr.s_addr),
	      ngp->ng_length));
#endif	/* DEBUG */
}


/* Find and remove the entry for this route.  If we remove the last entry */
/* for a distance, remove that distance.  If we remove the last entry for a */
 /* gateway, remove that gateway */
static void
net_remove __PF2(ngp, egp_neighbor *,
		 rp, egp_route *)
{
    egp_dist *dp = rp->route_dist;
    egp_gwentry *gwp = dp->dist_gwentry;
    int sub_distance = 0;

    assert(dp && gwp);

#ifdef	DEBUG
    trace_tp(ngp->ng_task,
	     TR_EGP_DEBUG,
	     0,
	     ("net_remove: peer %A AS %u net %A proto %s distance %d gateway %A",
	      ngp->ng_addr,
	      ngp->ng_peer_as,
	      rp->route_rt->rt_dest,
	      trace_state(rt_proto_bits, rp->route_rt->rt_gwp->gw_proto),
	      dp->dist_distance,
	      sockbuild_in(0, gwp->gwe_addr.s_addr)));
#endif	/* DEBUG */

    /* Remove route from queue for this distance */
    REMQUE(rp);

    /* Remove length of network */
    switch (inet_class_of(rp->route_rt->rt_dest)) {
    case INET_CLASSC_C:
    case INET_CLASSC_E:
    case INET_CLASSC_C_SHARP:
	ngp->ng_length -= 3;
	break;
				
    case INET_CLASSC_B:
	ngp->ng_length -= 2;
	break;
				
    case INET_CLASSC_A:
	ngp->ng_length -= 1;
	break;
    }

    if (dp->dist_n_routes == 1) {
	/* This distance is now empty, remove it */

	REMQUE(dp);

	/* Remember to remove distance and net count from length */
	sub_distance++;
	
	task_block_free(blockindex_distance, (void_t) dp);
	dp = (egp_dist *) 0;
    } else {
	/* There are still routes left */

	/* Subtract this route and check for overflow */
	if (!(dp->dist_n_routes-- % EGP_ROUTES_MAX)) {
	    sub_distance++;
	}
    }
    
    /* And fix up the counts */
    if (sub_distance) {
	int sub_gateway = 0;
	byte gw_class = *((byte *) &gwp->gwe_addr.s_addr);

	/* Subtract distance and net count from length */
	ngp->ng_length -= 2;

	if (gwp->gwe_n_distances == 1) {
	    /* This gateway has no distances, remove it */

	    REMQUE(gwp);

	    /* Remember to remove gateway length and distance count */
	    /* from length */
	    sub_gateway++;

	    task_block_free(blockindex_gateway, (void_t) gwp);
	    gwp = (egp_gwentry *) 0;
	} else {
	    /* There are still distances left */

	    /* Subtract this distance and check for overflow */
	    if (!(gwp->gwe_n_distances-- % EGP_DISTANCE_MAX)) {
		/* Just crossed back over overflow threshold adjust count */

		sub_gateway++;
	    }
	}

	if (sub_gateway) {
	    /* Remove length of gateway */

	    switch (inet_class_of_byte(&gw_class)) {
	    case INET_CLASSC_A:
		ngp->ng_length -= 4;
		break;
		    
	    case INET_CLASSC_B:
	    case INET_CLASSC_E:
	    case INET_CLASSC_C_SHARP:
		ngp->ng_length -= 3;
		break;

	    case INET_CLASSC_C:
		ngp->ng_length -= 2;
		break;
	    }
	}
    }
   
}


/*  */

/* Find the gateway address to use on the polled network */
static if_addr *
egp_rt_pollnet __PF2(ngp, egp_neighbor *,
		     dest, sockaddr_un *)
{
    if_addr *ifap = ngp->ng_ifap;

    if (ifap) {
	/* See if this relates to our interface */
	if (ifap->ifa_net) {
	    /* Multi-access interface */

	    if (sock2ip(dest) == inet_net_natural(ifap->ifa_net)
		|| sockaddrcmp_in(dest, ifap->ifa_net)) {
		/* We have a match */

		return ifap;
	    }
	} else {
	    /* Point2point interface, check network of local side */

	    if (sock2ip(dest) == inet_net_natural(ifap->ifa_addr_local)) {
		/* We have a match */

		return ifap;
	    }
	}
    }
    
    /* First try a destination/subnet */
    if ((ifap = if_withdst(dest))) {
	return ifap;
    }

    /* Finally try a network */
    if ((ifap = inet_ifwithnet(dest))) {
	return ifap;
    }
	
    trace_log_tf(ngp->ng_trace_options,
		 0,
		 LOG_ERR,
		 ("egp_rt_pollnet: no route to polled net %A",
		  dest));

    return ifap;
}


/* Rebuild the announcement list with the new polled address */
int
egp_rt_newaddr(ngp, paddr)
egp_neighbor *ngp;
sockaddr_un *paddr;
{
    egp_gwentry *gws[2];
    int exterior = 0;
    int new_address = FALSE;
    if_addr *ifap = (if_addr *) 0;

    if (paddr) {
	/* Locate the interface route to this new network */
	if (!(ifap = egp_rt_pollnet(ngp, paddr))) {
	    return TRUE;
	}

	new_address = !ngp->ng_paddr || !sockaddrcmp_in(ngp->ng_paddr, paddr);
    }

    trace_tp(ngp->ng_task,
	     TR_STATE,
	     0,
	     ("egp_rt_newaddr: neighbor %s polled address: old: %A new: %A",
	      ngp->ng_name,
	      ngp->ng_paddr,
	      paddr));

    if (new_address) {
	if (ngp->ng_paddr) {
	    sockfree(ngp->ng_paddr);
	}
	ngp->ng_paddr = sockdup(paddr);
    }

    if (!paddr || new_address) {
	/* If the old and new addresses are not the same, recreate the policy for this peer */
	if (new_address) {
	    trace_tp(ngp->ng_task,
		     TR_STATE,
		     0,
		     ("egp_rt_newaddr: neighbor %s rebuilding for new address",
		      ngp->ng_name));
	} else {
	    trace_tp(ngp->ng_task,
		     TR_STATE,
		     0,
		     ("egp_rt_newaddr: neighbor %s freeing announcement list",
		      ngp->ng_name));
	}
	    
	/* Scan the complete announcement list for this peer and re-insert each route with it's new gateway */
	for (exterior = 0; exterior < 2; exterior++) {
	    /* Copy the head */
	    gws[exterior] = ngp->ng_net_gw[exterior];

	    /* And clear the old pointer */
	    ngp->ng_net_gw[exterior] = (egp_gwentry *) 0;
	}

	/* Reset packet length */
	ngp->ng_length = sizeof (egp_packet_nr) + IP_MAXHDRLEN;

	for (exterior = 0; exterior < 2; exterior ++) {
	    egp_gwentry *gwp;

	    if (!gws[exterior]) {
		continue;
	    }
	    
	    EGP_GW_LIST(gwp, gws[exterior]) {
		egp_dist *dp;
		egp_gwentry *old_gwp = gwp;

		gwp = gwp->gwe_back;
		REMQUE(old_gwp);

		EGP_DIST_LIST(dp, &old_gwp->gwe_distances) {
		    egp_route *rp;
		    egp_dist *old_dp = dp;

		    dp = dp->dist_back;
		    REMQUE(old_dp);

		    EGP_ROUTE_LIST(rp, &old_dp->dist_routes) {
			egp_route *old_rp = rp;

			rp = rp->route_back;
			REMQUE(old_rp);

			if (ifap) {
			    /* Re-insert this element into the new list */

			    net_insert(ngp, old_rp, ifap, old_dp->dist_distance);
			} else {
			    /* Unchain and free this element */

			    rttsi_reset(old_rp->route_rt->rt_head, ngp->ng_task->task_rtbit);
			    (void) rtbit_reset(old_rp->route_rt, ngp->ng_task->task_rtbit);
			    task_block_free(blockindex_route, (void_t) old_rp);
			}
		    } EGP_ROUTE_LIST_END(rp, &old_dp->dist_routes) ;

		    /* Free this element */
		    task_block_free(blockindex_distance, (void_t) old_dp);
		} EGP_DIST_LIST_END(dp, &old_gwp->gwe_distances) ;

		/* Free this element */
		task_block_free(blockindex_gateway, (void_t) old_gwp);
	    } EGP_GW_LIST_END(gwp, gws[exterior]) ;

	    /* Free head pointer */
	    task_block_free(blockindex_gateway, (void_t) gws[exterior]);
	}

	trace_tp(ngp->ng_task,
		 TR_NORMAL,
		 0,
		 ("egp_rt_newaddr: neighbor %s new length %d",
		  ngp->ng_name,
		  ngp->ng_length));
    }
    
    return FALSE;
}


/* Reinit after a parse */
void
egp_rt_reinit(ngp)
egp_neighbor *ngp;
{
    int entries = 0;
    rt_entry *rt;

    if (!blockindex_gateway) {
	blockindex_gateway = task_block_init(sizeof (egp_gwentry), "egp_gwentry");
	blockindex_distance = task_block_init(sizeof (egp_dist), "egp_dist");
	blockindex_route = task_block_init(sizeof (egp_route), "egp_route");
    }

    /* Locate new policy */
    BIT_SET(ngp->ng_flags, NGF_POLICY);
    ngp->ng_import = control_exterior_locate(egp_import_list, ngp->ng_peer_as);
    ngp->ng_export = control_exterior_locate(egp_export_list, ngp->ng_peer_as);
    
    /* Open the routing table */
    rt_open(ngp->ng_task);

    RTQ_LIST(&ngp->ng_gw.gw_rtq, rt) {
	pref_t preference = ngp->ng_preference;

	/* Calculate preference of this route */
	(void) import(rt->rt_dest,
		      rt->rt_dest_mask,
		      ngp->ng_import,
		      (adv_entry *) 0,
		      (adv_entry *) 0,
		      &preference,
		      ngp->ng_ifap,
		      (void_t) 0);

	if (rt->rt_preference != preference
	  || rt->rt_preference2 != ngp->ng_preference2) {
	    /* The preference has changed, change the route */
	    (void) rt_change(rt,
			     rt->rt_metric,
			     rt->rt_metric2,
			     rt->rt_tag,
			     preference,
			     ngp->ng_preference2,
			     rt->rt_n_gw, rt->rt_routers);
	}
	entries++;
    } RTQ_LIST_END(&ngp->ng_gw.gw_rtq, rt) ;

    /* Close the routing table */
    rt_close(ngp->ng_task, (gw_entry *) 0, entries, NULL);
}


/*
 *	Release the announcement list resources
 */
void
egp_rt_terminate(ngp)
egp_neighbor *ngp;
{
    rt_entry *rt;

    rt_open(ngp->ng_task);

    /* Release all our routes */
    RTQ_LIST(&ngp->ng_gw.gw_rtq, rt) {
	rt_delete(rt);
    } RTQ_LIST_END(&ngp->ng_gw.gw_rtq, rt) ;

    /* Free the announcement lists */
    (void) egp_rt_newaddr(ngp, (sockaddr_un *) 0);
    if (ngp->ng_paddr) {
	sockfree(ngp->ng_paddr);
	ngp->ng_paddr = (sockaddr_un *) 0;
    }

    if (ngp->ng_task->task_rtbit) {
	/* Free the bit */
	rtbit_free(ngp->ng_task, ngp->ng_task->task_rtbit);
    }

    rt_close(ngp->ng_task, &ngp->ng_gw, 0, NULL);
}


/* Dump the announcement list for this peer */
void
egp_rt_dump(fd, ngp)
FILE *fd;
egp_neighbor *ngp;
{
    int exterior = 0;
    
    for (exterior = 0; exterior < 2; exterior ++) {
	egp_gwentry *gwp;

	if (!ngp->ng_net_gw[exterior]) {
	    /* No gateways */
	    continue;
	}
	
	fprintf(fd, "\t\t%s gateways:\n",
		exterior ? "Exterior" : "Interior");
	
	EGP_GW_LIST(gwp, ngp->ng_net_gw[exterior]) {
	    egp_dist *dp;

	    fprintf(fd, "\t\t\tGateway: %A  %d distances\n",
		    sockbuild_in(0, gwp->gwe_addr.s_addr),
		    gwp->gwe_n_distances);
	    
	    EGP_DIST_LIST(dp, &gwp->gwe_distances) {
		int n_nets = 0;
		egp_route *rp;

		fprintf(fd, "\t\t\t\tDistance: %d  %d networks\n",
			dp->dist_distance,
			dp->dist_n_routes);
		
		EGP_ROUTE_LIST(rp, &dp->dist_routes) {

		    if (++n_nets == 5) {
			n_nets = 0;
		    }
		    
		    fprintf(fd, "%s%-15A%c",
			    n_nets == 1 ? "\t\t\t\t\t" : "",
			    rp->route_rt->rt_dest,
			    n_nets ? ' ' : '\n');
		    
		} EGP_ROUTE_LIST_END(rp, &dp->dist_routes) ;

		if (n_nets) {
		    fprintf(fd, "\n");
		}
		
	    } EGP_DIST_LIST_END(dp, gwp->gwe_distances) ;
	    
	} EGP_GW_LIST_END(gwp, ngp->ng_net_gw[exterior]) ;
    }
}


/* Calculate the list of networks we will announce to this peer */
void
egp_rt_policy(ngp, rtl)
egp_neighbor *ngp;
rt_list *rtl;
{
    rt_head *rth;
    if_addr *ifap = (if_addr *) 0;
    task *tp = ngp->ng_task;

    trace_tp(ngp->ng_task,
	     TR_STATE,
	     0,
	     ("egp_rt_policy: neighbor %s polled net %A",
	      ngp->ng_name,
	      ngp->ng_paddr));

    /* Locate the interface route to the old network */
    if (ngp->ng_paddr) {
	if (!(ifap = egp_rt_pollnet(ngp, ngp->ng_paddr))) {
	    return;
	}
    }

    rt_open(tp);

    RT_LIST(rth, rtl, rt_head) {
	rt_entry *new_rt = rth->rth_active;
	rt_entry *old_rt;
	adv_results result;
	egp_route *rp = (egp_route *) 0;

	result.res_metric = egp_default_metric;

	if (rth->rth_last_active
	    && rtbit_isset(rth->rth_last_active, tp->task_rtbit)) {
	    /* We were announcing the last active route */
	    
	    old_rt = rth->rth_last_active;
	} else if (rth->rth_n_announce) {
	    /* If we have a route in holddown, the above might not catch it */
 
	    RT_ALLRT(old_rt, rth) {
		if (rtbit_isset(old_rt, tp->task_rtbit)) {
		    break;
		}
	    } RT_ALLRT_END(old_rt, rth) ;
	} else {
	    /* No old route */

	    old_rt = (rt_entry *) 0;
	}

	/* Can we announce the new route (if there is one)? */
	if (new_rt) {
	    /* Figure out if we are allowed to announce this route */

	    if (socktype(new_rt->rt_dest) != AF_INET) {
		goto no_export;
	    }
	    
	    if (TRACE_TP(tp, TR_POLICY)) {
		tracef("\t%-15A %-8s\t",
		       rth->rth_dest,
		       trace_state(rt_proto_bits, new_rt->rt_gwp->gw_proto));
	    }

	    if (new_rt->rt_dest_mask != inet_mask_natural(new_rt->rt_dest)
		|| BIT_TEST(new_rt->rt_state, RTS_NOADVISE|RTS_PENDING|RTS_GROUP)) {
		/* don't send subnets or hosts */
		trace_tp(tp,
			 TR_POLICY,
			 0,
			 ("subnet, host or bogus"));
		goto no_export;
	    }

	    /* Ignore nets that are not Class A, B, C or E */
	    
	    switch (BIT_TEST(inet_class_flags(rth->rth_dest), INET_CLASSF_DEFAULT|INET_CLASSF_NETWORK)) {
	    case INET_CLASSF_DEFAULT|INET_CLASSF_NETWORK:
		/* Don't allow the DEFAULT net through, unless we are allowed to */
		/* send DEFAULT and this is the internally generated default. */
		if (!BIT_TEST(ngp->ng_options, NGO_DEFAULTOUT)) {
		    trace_tp(tp,
			     TR_POLICY,
			     0,
			     ("default"));
		    goto no_export;
		}
		break;

	    case INET_CLASSF_NETWORK:
		switch (inet_class_of(rth->rth_dest)) {
		case INET_CLASSC_A:
		case INET_CLASSC_B:
		case INET_CLASSC_C:
		case INET_CLASSC_E:
		case INET_CLASSC_C_SHARP:
		    break;
		default:
		    goto bad_class;
		}
		break;

	    bad_class:
	    default:
		trace_tp(tp,
			 TR_POLICY,
			 0,
			 ("unsupported class"));
		trace_log_tp(tp,
			     TRC_LOGONLY,
			     LOG_ERR,
			     ("egp_rt_policy: net class not valid for export: %A",
			      rth->rth_dest));
		goto no_export;
	    }

	    if (RT_IFAP(new_rt)
		&& !BIT_TEST(RT_IFAP(new_rt)->ifa_state, IFS_UP)) {
		/* Interface not up */
		goto no_export;
	    }
	    
	    if (export(new_rt,
		       (proto_t) 0,
		       ngp->ng_export,
		       (adv_entry *) 0,
		       (adv_entry *) 0,
		       &result)) {
		/* Exportable */
		
		if (BIT_TEST(ngp->ng_options, NGO_METRICOUT)) {
		    /* Fixed distance */
		    result.res_metric = ngp->ng_metricout;
		}

		trace_tp(tp,
			 TR_POLICY,
			 0,
			 ("distance %d",
			  result.res_metric));
		if (new_rt != old_rt) {
		    rtbit_set(new_rt, tp->task_rtbit);
		}
	    } else {
		/* Not exportable */
		
		trace_tp(tp,
			 TR_POLICY,
			 0,
			 ("not exportable"));

	    no_export:
		new_rt = (rt_entry *) 0;
	    }
	}

	/* Reset old announcment and remove the entry from the list */
	if (old_rt) {
	    /* Remove the old network from the list */

	    /* Get pointer to block */
	    rttsi_get(rth, tp->task_rtbit, (byte *) &rp);

	    net_remove(ngp, rp);
	    assert(rp && rp->route_rt == old_rt);

	    if (old_rt != new_rt) {
		if (!new_rt) {
		    /* Reset the TSI field */
		    rttsi_reset(rth, tp->task_rtbit);

		    /* And free the entry */
		    task_block_free(blockindex_route, (void_t) rp);
		}

		/* Reset announcement indication */
		(void) rtbit_reset(old_rt, tp->task_rtbit);
	    }
	}

	if (new_rt) {
	    /* Insert this new network into the list */

	    if (!rp) {
		rp = (egp_route *) task_block_alloc(blockindex_route);

		/* Set TSI to point to route entry */
		rttsi_set(rth, tp->task_rtbit, (byte *) &rp);
	    }

	    /* Point entry at this route and insert it into the list */
	    rp->route_rt = new_rt;
	    net_insert(ngp, rp, ifap, result.res_metric);
	}
    } RT_LIST_END(rth, rtl, rt_head) ;

    rt_close(tp, &ngp->ng_gw, 0, NULL);

    trace_tp(tp,
	     TR_NORMAL,
	     0,
	     ("egp_rt_policy: neighbor %s length %d",
	      ngp->ng_name,
	      ngp->ng_length));
    
    return;
}


/*
 * Evaluate the policy for a peer, called when the AS is not
 * configured and we learn his AS from an acquire packet
 */
void
egp_rt_newpolicy(ngp)
egp_neighbor *ngp;
{
    rt_list *rtl = rthlist_active(AF_INET);

    /* Pickup policy and re-evaluate input policy */
    egp_rt_reinit(ngp);
    
    /* Cause all the policy to be re-evaluated for this peer */
    egp_rt_policy(ngp, rtl);

    RTLIST_RESET(rtl);
}


/**/

/* Build an NR message from the network list from this peer */
#define	overflow_check(nrp, limit, Return)	if (nrp > limit) goto Return
#define	put_down(nrp, data)	*(nrp)++ = data
    /* Take advantage of the fact that in_addr's are in network byte order */
#define	put_gateway(nrp, gateway, n_gateways, n_distances) \
{ \
    register byte *cp = (byte *) &(gateway)->s_addr; \
    if (*n_gateways == EGP_GATEWAY_MAX) { \
	goto Gateways; \
    } \
    (*(n_gateways))++; \
    switch (inet_class_of_byte(cp)) { \
    case INET_CLASSC_A: \
	cp += 1; \
	put_down(nrp, *cp++); put_down(nrp, *cp++); put_down(nrp, *cp++); \
	break; \
    case INET_CLASSC_B: \
	cp += 2; \
	put_down(nrp, *cp++); put_down(nrp, *cp++); \
	break; \
    case INET_CLASSC_C: \
	cp += 3; \
	put_down(nrp, *cp++); \
	break; \
    case INET_CLASSC_E: \
    case INET_CLASSC_C_SHARP: \
	cp += 2; \
	put_down(nrp, *cp++ & 0x0f); put_down(nrp, *cp++); \
	break; \
    default: \
	assert(FALSE); \
    } \
    /* Save a pointer to the distance count */ \
    n_distances = nrp++; \
    *n_distances = 0; \
}	
#define	put_distance(nrp, distance)	put_down(nrp, distance)
#define	put_network(nrp, addr) \
{ \
    register byte *cp = (byte *) &sock2ip(addr); \
    switch(inet_class_of_byte(cp)) { \
    case INET_CLASSC_C: \
	put_down(nrp, *cp++); \
	/* Fall through */ \
    case INET_CLASSC_B: \
	put_down(nrp, *cp++); \
	/* Fall though */ \
    case INET_CLASSC_A: \
	put_down(nrp, *cp++); \
	break; \
    case INET_CLASSC_E: \
    case INET_CLASSC_C_SHARP: \
	put_down(nrp, *cp++); put_down(nrp, *cp++ & 0xf0); \
	break; \
    } \
}

int
egp_rt_send(ngp, pkt)
egp_neighbor *ngp;
egp_packet_nr *pkt;
{
    register byte *nrp = (byte *) (pkt + 1);		/* next octet of NR message */
    byte *limit = (byte *) pkt + ngp->ng_length - IP_MAXHDRLEN;
    int exterior;
    size_t length;

    /* Since the output buffer includes the size of the IP packet we have a bit of slop */

    for (exterior = 0; exterior < 2; exterior++) {
	u_int8 *n_gateways = exterior ? &pkt->en_egw : &pkt->en_igw;
	egp_gwentry *gwsp = ngp->ng_net_gw[exterior];
	egp_gwentry *gwp;

	*n_gateways = 0;
	
	if (!gwsp) {
	    /* No gateways for this class */

	    continue;
	}

	EGP_GW_LIST(gwp, gwsp) {
	    egp_dist *dp;
	    byte *n_distances;

	    /* Put down gateway address */
	    put_gateway(nrp, &gwp->gwe_addr, n_gateways, n_distances);
	    overflow_check(nrp, limit, Overflow);

	    EGP_DIST_LIST(dp, &gwp->gwe_distances) {
		egp_route *rp = dp->dist_routes.route_forw;
		byte *n_nets;

	    do_distance:
		/* Reset net count */

		if (*n_distances == EGP_DISTANCE_MAX) {
		    /* This gateway has all the distances it can handle */

		    /* Make a new gateway entry */
		    put_gateway(nrp, &gwp->gwe_addr, n_gateways, n_distances);
		    overflow_check(nrp, limit, Overflow);
		}

		/* Increment count of distances */
		(*n_distances)++;

		/* Save this distance */
		put_distance(nrp, dp->dist_distance);
		overflow_check(nrp, limit, Overflow);

		/* Save a pointer to the network count */
		*(n_nets = nrp++) = 0;

		do {
		    /* Count this network */
		    if (*n_nets == EGP_ROUTES_MAX) {
			*n_nets = EGP_ROUTES_MAX;
			goto do_distance;
		    }
		    (*n_nets)++;

		    /* Put down the network */
		    put_network(nrp, rp->route_rt->rt_dest);
		    overflow_check(nrp, limit, Overflow);
		} while ((rp = rp->route_forw) != &dp->dist_routes) ;

	    } EGP_DIST_LIST_END(dp, &gwp->gew_distances) ;

	} EGP_GW_LIST_END(gwp, gwps) ;
    }

    length = nrp - (byte *) pkt;
    if (length + IP_MAXHDRLEN != ngp->ng_length) {
	trace_log_tf(ngp->ng_trace_options,
		     0,
		     LOG_WARNING,
		     ("egp_rt_send: warning: neighbor %s AS %d NR message size (%d) different than calculated (%d)",
		      ngp->ng_name,
		      ngp->ng_peer_as,
		      length + IP_MAXHDRLEN,
		      ngp->ng_length));
    }
    return length;

 Gateways:
    trace_log_tf(ngp->ng_trace_options,
		 0,
		 LOG_WARNING,
		 ("egp_rt_send: neighbor %s AS %d NR contains too many gateways",
		  ngp->ng_name,
		  ngp->ng_peer_as));
    return EGP_ERROR;

 Overflow:
    trace_log_tf(ngp->ng_trace_options,
		 0,
		 LOG_WARNING,
		 ("egp_rt_send: neighbor %s AS %d NR message exceeds output buffer (%d)",
		  ngp->ng_name,
		  ngp->ng_peer_as,
		  ngp->ng_length));
    return EGP_ERROR;
}


/*  */
/*
 * egp_rt_recv() updates the exterior routing tables on receipt of an NR
 * update message from an EGP neighbor. It first checks for valid NR counts
 * before updating the routing tables.
 *
 * EGP Updates are used to update the exterior routing table if one of the
 * following is satisfied:
 *   - No routing table entry exists for the destination network and the
 *     metric indicates the route is reachable (< 255).
 *   - The advised gateway is the same as the current route.
 *   - The advised distance metric is less than the current metric.
 *   - The current route is older (plus a margin) than the maximum poll
 *     interval for all acquired EGP neighbors. That is, the route was
 *     omitted from the last Update.
 *
 * Returns 1 if there is an error in NR message data, 0 otherwise.
 */

int
egp_rt_recv(ngp, pkt, egplen)
egp_neighbor *ngp;			/* pointer to neighbor state table */
egp_packet *pkt;
size_t egplen;				/* length EGP NR packet */
{
    int rc = EGP_NOERROR;
    int gw_class;
    int checkingNR;
    int NR_nets = 0;
    register byte *nrb;
    register byte *cp;
    rt_entry *rt;
    egp_packet_nr *nrp = (egp_packet_nr *) ((void_t) pkt);
    rt_parms rtparms;
    rtq_entry rtq;

    bzero((caddr_t) &rtparms, sizeof (rtparms));
    rtparms.rtp_dest = sockdup(sockbuild_in(0, 0));
    rtparms.rtp_n_gw = 1;
    rtparms.rtp_router = sockdup(sockbuild_in(0, 0));
    rtparms.rtp_gwp = &ngp->ng_gw;
    rtparms.rtp_preference2 = ngp->ng_preference2;

    /* Figure out gateway class and verify it is valid */
    if (!inet_class_valid_byte((byte *) &nrp->en_net)) {
	rc = EGP_REASON_BADDATA;		/* NR message error */
	goto Return;
    }
    gw_class = inet_class_of_byte((byte *) &nrp->en_net);

    if (BIT_TEST(ngp->ng_options, NGO_GATEWAY)) {
	sock2in(rtparms.rtp_router) = sock2in(ngp->ng_gateway);	/* struct copy */
    } else {
	sock2ip(rtparms.rtp_router) = nrp->en_net;	/* struct copy */
    }

    /*
     * First check NR message for valid counts, then repeat and update routing
     * tables
     */
    for (checkingNR = 1; checkingNR >= 0; checkingNR--) {
	u_int n_gw = nrp->en_igw + nrp->en_egw;

	if (!checkingNR) {
	    /* Save queue */
	    RTQ_MOVE(ngp->ng_gw.gw_rtq, rtq);
	    
	    rt_open(ngp->ng_task);
	}

	nrb = (byte *) nrp + sizeof(egp_packet_nr);	/* start first gw */

	while (n_gw--) {
	    int n_dist;

	    rtparms.rtp_state = (n_gw < nrp->en_egw ? RTS_INTERIOR : RTS_EXTERIOR);

	    if (!BIT_TEST(ngp->ng_options, NGO_GATEWAY)) {
		/* Pickup gateway address */
		cp = (byte *) &sock2ip(rtparms.rtp_router);
		switch (gw_class) {
		case INET_CLASSC_A:
		    cp += 1;
		    *cp++ = *nrb++;
		    *cp++ = *nrb++;
		    *cp++ = *nrb++;
		    break;

		case INET_CLASSC_B:
		    cp += 2;
		    *cp++ = *nrb++;
		    *cp++ = *nrb++;
		    break;
		    
		case INET_CLASSC_C:
		    cp += 3;
		    *cp++ = *nrb++;
		    break;
		    
		case INET_CLASSC_E:
		case INET_CLASSC_C_SHARP:
		    sock2ip(rtparms.rtp_router) &= INET_CLASSE_NET;
		    if (*nrb & 0xf0) {
			rc = EGP_REASON_BADDATA;
			goto Return;
		    }
		    cp += 2;
		    *cp++ = (*cp & 0x0f) | *nrb++;
		    *cp++ = *nrb++;
		    break;
		}

		if (!if_withdst(rtparms.rtp_router) ||
		    if_withlcladdr(rtparms.rtp_router, TRUE)) {
		    /* Invalid gateway */
		    rc = EGP_REASON_BADDATA;
		    goto Return;
		}
	    } else {
		switch (gw_class) {
		case INET_CLASSC_A:
		    nrb += 3;
		    break;

		case INET_CLASSC_B:
		case INET_CLASSC_E:
		case INET_CLASSC_C_SHARP:
		    nrb += 2;
		    break;

		case INET_CLASSC_C:
		    nrb += 1;
		}
	    }

	    /* Pickup number of distances */
	    n_dist = *nrb++;

	    while (n_dist--) {
		int n_net;

		/* Pickup distance */
		rtparms.rtp_metric = *nrb++;

		/* Pickup number of networks */
		n_net = *nrb++;

		if (!checkingNR) {
		    NR_nets += n_net;
		}

		while (n_net--) {
		    rtparms.rtp_preference = ngp->ng_preference;

		    /* Pickup network */
		    sock2ip(rtparms.rtp_dest) = 0;
		    cp = (byte *) &sock2ip(rtparms.rtp_dest);

		    switch (inet_class_of_byte(nrb)) {
		    default:
			/* Bogus */
			if (checkingNR) {
			    trace_log_tf(ngp->ng_trace_options,
					 0,
					 LOG_ERR,
					 ("egp_rt_recv: net %d class not valid from %s via %A",
					  *nrb,
					  ngp->ng_name,
					  rtparms.rtp_router));
			}

			/* We assume that the length of an invalid */
			/* network is three.  If this asumption is bad */
			/* we could end up creating alot of bogus */
			/* routes. */
			nrb += 3;
			continue;
			
		    case INET_CLASSC_E:
		    case INET_CLASSC_C_SHARP:
			*cp++ = *nrb++;
			*cp++ = *nrb++;
			if ((*cp++ = *nrb++) & 0x0f) {
			    continue;
			}
			break;

		    case INET_CLASSC_C:
			*cp++ = *nrb++;
			*cp++ = *nrb++;
			*cp++ = *nrb++;
			break;

		    case INET_CLASSC_B:
			*cp++ = *nrb++;
			*cp++ = *nrb++;
			break;
			
		    case INET_CLASSC_A:
			*cp++ = *nrb++;

 			if (BIT_TEST(inet_class_flags_byte(nrb), INET_CLASSF_DEFAULT)
 			    && !BIT_TEST(ngp->ng_options, NGO_DEFAULTIN)) {
 			    if (checkingNR) {
				trace_log_tf(ngp->ng_trace_options,
					     0,
					     LOG_WARNING,
					     ("egp_rt_recv: ignoring net %A from %s via %A",
					      rtparms.rtp_dest,
					      ngp->ng_name,
					      rtparms.rtp_router));
 			    }
 			    continue;
			}
		    }

		    /* We only deal in REAL networks so we only need a REAL netmask */
		    rtparms.rtp_dest_mask = inet_mask_natural(rtparms.rtp_dest);
		    
		    if (checkingNR) {
			/* first check counts only */
			if (nrb > (byte *) nrp + egplen + 1) {
			    /* erroneous counts in NR */
			    rc = EGP_REASON_BADDATA;
			    goto Return;
			} else {
			    continue;
			}
		    }

		    rt = rt_locate_gw(RTS_NETROUTE,
				      rtparms.rtp_dest,
				      rtparms.rtp_dest_mask,
				      rtparms.rtp_gwp);
		    if (!rt) {
			/* new route */
			if (rtparms.rtp_metric == EGP_DISTANCE_MAX) {
			    /* Unreachable */

			    rt = (rt_entry *) 0;
			} else if (ngp->ng_ifap->ifa_net
				   && sock2ip(rtparms.rtp_dest) == inet_net_natural(ngp->ng_ifap->ifa_net)) {
			    /* Ignore route to shared network */

			    rtparms.rtp_preference = -1;
			} else {
			    (void) import(rtparms.rtp_dest,
					  rtparms.rtp_dest_mask,
					  ngp->ng_import,
					  (adv_entry *) 0,
					  (adv_entry *) 0,
					  &rtparms.rtp_preference,
					  ngp->ng_ifap,
					  (void_t) 0);
			}
		    } else if (BIT_TEST(rt->rt_state, RTS_INTERIOR|RTS_EXTERIOR) !=
			       BIT_TEST(rtparms.rtp_state, RTS_INTERIOR|RTS_EXTERIOR)) {
			/* interior/exterior change */
			rtparms.rtp_preference = rt->rt_preference;
			rt_delete(rt);
			rt = (rt_entry *) 0;
		    } else if (rtparms.rtp_metric != rt->rt_metric) {
			/* Metric change */
			if (rtparms.rtp_metric == EGP_DISTANCE_MAX) {
			    /* Unreachable */
			    rt_delete(rt);
			    rt = (rt_entry *) 0;
			} else {
			    (void) rt_change(rt,
					     rtparms.rtp_metric,
					     rtparms.rtp_metric2,
					     rtparms.rtp_tag,
					     rt->rt_preference,
					     rt->rt_preference2,
					     1, rtparms.rtp_routers);
			    rt_refresh(rt);
			}
		    } else if (!sockaddrcmp_in(rtparms.rtp_router, RT_ROUTER(rt))) {
			/* Gateway change */
			(void) rt_change(rt,
					 rtparms.rtp_metric,
					 rtparms.rtp_metric2,
					 rtparms.rtp_tag,
					 rt->rt_preference,
					 rt->rt_preference2,
					 1, rtparms.rtp_routers);
			rt_refresh(rt);
		    } else {
			/* No change */
			rt_refresh(rt);
		    }
		    if (!rt && rtparms.rtp_metric != EGP_DISTANCE_MAX) {
			/* New route or gateway/metric change */
			rt = rt_add(&rtparms);
		    }
		}			/* end for all nets */
	    }				/* end for all distances */
	}				/* end for all gateways */

	if (checkingNR && (nrb > (byte *) nrp + egplen)) {
	    rc = EGP_REASON_BADDATA;		/* erroneous counts */
	    goto Return;
	}
    }

    /* If there are any routes in the table that were not in this update, delete them */
    RTQ_LIST(&rtq, rt) {
	rt_delete(rt);
    } RTQ_LIST_END(&rtq, rt);

    rt_close(ngp->ng_task, &ngp->ng_gw, NR_nets, NULL);

    /*
     * Generate default if not prohibited and the NR packet
     * contains a route
     */
    if (!BIT_TEST(ngp->ng_options, NGO_NOGENDEFAULT) &&
	!BIT_TEST(ngp->ng_flags, NGF_GENDEFAULT) &&
	NR_nets) {
	rt_default_add();
	BIT_SET(ngp->ng_flags, NGF_GENDEFAULT);
    }

 Return:
    sockfree(rtparms.rtp_dest);
    sockfree(rtparms.rtp_router);
    
    return rc;
}
