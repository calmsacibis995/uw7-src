#ident	"@(#)ospf_spf_leaves.c	1.3"
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
 * ------------------------------------------------------------------------
 * 
 *                 U   U M   M DDDD     OOOOO SSSSS PPPPP FFFFF
 *                 U   U MM MM D   D    O   O S     P   P F
 *                 U   U M M M D   D    O   O  SSS  PPPPP FFFF
 *                 U   U M M M D   D    O   O     S P     F
 *                  UUU  M M M DDDD     OOOOO SSSSS P     F
 * 
 *     		          Copyright 1989, 1990, 1991
 *     	       The University of Maryland, College Park, Maryland.
 * 
 * 			    All Rights Reserved
 * 
 *      The University of Maryland College Park ("UMCP") is the owner of all
 *      right, title and interest in and to UMD OSPF (the "Software").
 *      Permission to use, copy and modify the Software and its documentation
 *      solely for non-commercial purposes is granted subject to the following
 *      terms and conditions:
 * 
 *      1. This copyright notice and these terms shall appear in all copies
 * 	 of the Software and its supporting documentation.
 * 
 *      2. The Software shall not be distributed, sold or used in any way in
 * 	 a commercial product, without UMCP's prior written consent.
 * 
 *      3. The origin of this software may not be misrepresented, either by
 *         explicit claim or by omission.
 * 
 *      4. Modified or altered versions must be plainly marked as such, and
 * 	 must not be misrepresented as being the original software.
 * 
 *      5. The Software is provided "AS IS". User acknowledges that the
 *         Software has been developed for research purposes only. User
 * 	 agrees that use of the Software is at user's own risk. UMCP
 * 	 disclaims all warrenties, express and implied, including but
 * 	 not limited to, the implied warranties of merchantability, and
 * 	 fitness for a particular purpose.
 * 
 *     Royalty-free licenses to redistribute UMD OSPF are available from
 *     The University Of Maryland, College Park.
 *       For details contact:
 * 	        Office of Technology Liaison
 * 		4312 Knox Road
 * 		University Of Maryland
 * 		College Park, Maryland 20742
 * 		     (301) 405-4209
 * 		FAX: (301) 314-9871
 * 
 *     This software was written by Rob Coltun
 *      rcoltun@ni.umd.edu
 */


#define	INCLUDE_TIME
#include "include.h"
#include "inet.h"
#include "ospf.h"


/*
 * These routines deal with the SUMMARY and AS External part of the
 * 	Dijkstra algorithm
 */

/* MODIFIED 5/2/92 - should make nr a hash table */
static int
area_nr_check __PF1(nr_net, u_int32)
{
    struct NET_RANGE *nr;
    struct AREA *dst;

    AREA_LIST(dst) {
	RANGE_LIST(nr, dst) {
	    if (nr->net == nr_net) {
		if (nr->cost < SUMLSInfinity) {
		    return TRUE;
		}
	    } else if (ntohl(nr_net) > ntohl(nr->net)) {
		break;
	    }
	} RANGE_LIST_END(nr, dst) ;
    } AREA_LIST_END(dst) ;

    return FALSE;
}


/*
 * Handle network routes imported from another area
 * 	- called by spf() or rxlinkup()
 *   	- intra or sum may have been run before, but it ain't necessarily so
 *	- if called by rxlinkup can just do partial updates
 */
int
netsum __PF4(a, struct AREA *,
	     from, int,			/* level was run from */
	     from_area, struct AREA *,	/* area running the algorithm */
	     partial, int)		/* True if called from rxlinkup */
{
    register struct LSDB_HEAD *hp;
    struct OSPF_ROUTE *rr;
    rt_entry *old_route = (rt_entry *) 0;
    u_int32 lowcost = SUMLSInfinity, cost;

    trace_tf(ospf.trace_options,
	     TR_OSPF_SPF,
	     0,
	     ("OSPF SPF Area %A running Network Summary",
	      sockbuild_in(0, a->area_id)));

    if (!(from & INTRASCHED)) {
	/* intra hasn't been run */
	a->spfcnt++;
	RTAB_REV++;
    }

    LSDB_HEAD_LIST(a->htbl[LS_SUM_NET], hp, 0, HTBLSIZE) {
	struct LSDB *db, *low = LSDBNULL;

	if (partial && !DBH_RERUN(hp)) {
	    /* routes not included in partial update */
	    LSDB_LIST(hp, db) {
		/* running partial update - just mark current */
		if (db->lsdb_route) {
		    ORT_CHANGE(db->lsdb_route) = E_UNCHANGE;
		    ORT_REV(db->lsdb_route) = RTAB_REV;
		}
	    }  LSDB_LIST_END(hp, DB) ;
	} else if ((partial && DBH_RERUN(hp)) || !(partial)) {
	    /* non-partial update or partial update sched for this row */
	    DBH_RERUN(hp) = FALSE;
	    LSDB_LIST(hp, db) {
		if ((BIG_METRIC(db) != SUMLSInfinity) &&
		    (!DB_FREEME(db)) &&
		    (LS_AGE(db) < MaxAge) &&
		    (ADV_RTR(db) != MY_ID) && 
		    (!area_nr_check(LS_ID(db)))) {

		    rr = (struct OSPF_ROUTE *) 0;

		    /* check to see if we know about this ABR */
		    if (GOT_A_BDR(db) && ABRTR_ACTIVE(db)) {
			rr = db->lsdb_border->lsdb_ab_rtr;
		    } else {
			rr = rtr_findroute(a,
					   ADV_RTR(db),
					   DTYPE_ABR,
					   PTYPE_INTRA);
			if (rr) {
			    db->lsdb_border = RRT_V(rr);
			} else {
			    db->lsdb_border = (struct LSDB *) 0;
			}
		    }

		    /* Have a valid border router? */
		    if ((rr) &&
			db->lsdb_border &&
			((cost = (RRT_COST(rr) + BIG_METRIC(db))) <
			 SUMLSInfinity)) {
			if (cost == lowcost) {
			    ospf_add_parent(low,
					    db->lsdb_border,
					    cost,
					    a,
					    RRT_NH_ADDR(rr),
					    (struct OSPF_ROUTE *) 0, (struct AREA *) 0);
			} else if (cost < lowcost) {
			    /* free old parent list */
			    ospf_nh_free_list(db->lsdb_nhcnt, db->lsdb_nh);
			    low = db;
			    lowcost = cost;
			    ospf_add_parent(low,
					    low->lsdb_border,
					    cost,
					    a,
					    RRT_NH_ADDR(rr),
					    (struct OSPF_ROUTE *) 0, (struct AREA *) 0);
			}
		    }
		}
		/* Grab old route for deletion with partial update */
		if ((db->lsdb_route))
		    old_route = db->lsdb_route;

		/* MODIFIED 5/1/92 - removed ase rerun flag */
		if (!db->lsdb_next ||
		    (low && (LS_ID(db->lsdb_next) != LS_ID(low))) ||
		    (old_route && (LS_ID(db->lsdb_next) != RT_DEST(old_route)))) {
		    /* Nexthops were added? */
		    if (low && low->lsdb_nhcnt) {
		    	if ( addroute(a, low, from, from_area) )
			    return FLAG_NO_BUFS;
		    }
		    /* Modify partial update */
		    if (partial) {
			if (low && low->lsdb_route)
			    old_route = low->lsdb_route;
			if (old_route)
			    ospf_route_update(old_route, a, SUMASESCHED);
		    }
		    low = LSDBNULL;
		    lowcost = SUMLSInfinity;
		    old_route = (rt_entry *) 0;
		}
	    } LSDB_LIST_END(hp, db) ;
	}
    } LSDB_HEAD_LIST_END(a->htbl[LS_SUM_NET], hp, 0, HTBLSIZE) ;

    BIT_RESET(from_area->spfsched, SUMNETSCHED);
    BIT_RESET(a->spfsched, SUMNETSCHED);

    return 0;
}


/*
 * Handle all network as border rtrs imported from another area
 * 	- called by spf() or rxlinkup()
 *   	- intra or sum may have been run before, but it ain't necessarily so
 */
int
asbrsum __PF4(a, struct AREA *,
	      from, int,		/* 0 if called from spf() else 1 */
	      from_area, struct AREA *,
	      partial, int)
{
    register struct LSDB_HEAD *hp;
    struct OSPF_ROUTE *rr, *rrt_next;
    u_int32 lowcost = SUMLSInfinity, cost;
    u_int32 last_reachable_dest = 0;

    trace_tf(ospf.trace_options,
	     TR_OSPF_SPF,
	     0,
	     ("OSPF SPF Area %A running ASBR Summary",
	      sockbuild_in(0, a->area_id)));

    /*
     * For now will not do partial updates for asbrsum because there probably
     * won't be more than a few asbr routes
     */

    /* if intra and netsum haven't been run incriment spfcnt */
    if (!(from & SUMNETSCHED)) {
	a->spfcnt++;
	RTAB_REV++;
    }

    LSDB_HEAD_LIST(a->htbl[LS_SUM_ASB], hp, 0, HTBLSIZE) {
	register struct LSDB *db, *low = LSDBNULL;

	LSDB_LIST(hp, db) {
	    if ((BIG_METRIC(db) != SUMLSInfinity) &&
		(!DB_FREEME(db)) &&
		(LS_AGE(db) < MaxAge) &&
		(ADV_RTR(db) != MY_ID)) {
		/* First check for intra */
		if ((rr = rtr_findroute(0,
					LS_ID(db),
					DTYPE_ASBR,
					PTYPE_INTRA)))
		    goto next_db;

		/* check to see if we know about this ABR */
		if (GOT_A_BDR(db) && ABRTR_ACTIVE(db)) {
		    rr = db->lsdb_border->lsdb_ab_rtr;
		} else {
		    rr = rtr_findroute(a,
				       ADV_RTR(db),
				       DTYPE_ABR,
				       PTYPE_INTRA);
		    if (rr) {
			db->lsdb_border = RRT_V(rr);
		    } else {
			db->lsdb_border = (struct LSDB *) 0;
		    }
		}
		/* Have a valid border router? */
		if ((rr) &&
		    db->lsdb_border &&
		    ((cost = (RRT_COST(rr) + BIG_METRIC(db))) < SUMLSInfinity)) {
		    if (cost == lowcost) {
			ospf_add_parent(low,
					db->lsdb_border,
					cost,
					a,
					RRT_NH_ADDR(rr),
					(struct OSPF_ROUTE *) 0, (struct AREA *) 0);
		    } else if (cost < lowcost) {
			/* free old parent list */
			ospf_nh_free_list(db->lsdb_nhcnt, db->lsdb_nh);
			low = db;
			lowcost = cost;
			ospf_add_parent(low,
					low->lsdb_border,
					cost,
					a,
					RRT_NH_ADDR(rr),
					(struct OSPF_ROUTE *) 0, (struct AREA *) 0);
		    }
		}
	    }			/* Reasonable LS_SUM_ASB */
	  next_db:
	    if (low &&
		(!db->lsdb_next ||
		 (LS_ID(db->lsdb_next) != LS_ID(low)))) {
		/* Nexthops were added? */
		if (low->lsdb_nhcnt) {
		    /* add route to ospf's routing table */
		    if (addroute(a, low, from, from_area))
			return FLAG_NO_BUFS;
		}
		low = LSDBNULL;
		lowcost = SUMLSInfinity;
	    }
	} LSDB_LIST_END(hp, db) ;
    } LSDB_HEAD_LIST_END(a->htbl[LS_SUM_ASB], hp, 0, HTBLSIZE) ;

    /* modify asb routes */
    for (rr = ospf.sum_asb_rtab.ptr[NEXT];
	 rr;
	 rr = rrt_next) { 
	rrt_next = RRT_NEXT(rr);
	
	if (RRT_REV(rr) != RTAB_REV) {
	    /*
	     * For ASB Summary routes which are no longer valid, the self-originated
	     * summaries previously injected into other areas must be flushed. This 
	     * should only be done by Area Border routers and only if a new ASB Summary
	     * LSA has not already been generated for this AS Border router.
	     */
 	    if (RRT_V(rr)) {
		if ((IAmBorderRtr) &&
		    (a->area_id == OSPF_BACKBONE) &&
		    (ADV_RTR(RRT_V(rr)) != MY_ID) &&
		    (RRT_DEST(rr) != last_reachable_dest)) {
		    build_sum_asb(a, rr, from_area);
		}

		RRT_V(rr)->lsdb_asb_rtr = (struct OSPF_ROUTE *) 0;
		DB_DIRECT(RRT_V(rr)) = FALSE;
		ospf_nh_free_list(RRT_V(rr)->lsdb_nhcnt, RRT_V(rr)->lsdb_nh);
	    }
	    ospf_nh_free_list(RRT_NH_CNT(rr), RRT_NH(rr));
	    DEL_Q(rr, ospf_route_index);
	} else {
	    last_reachable_dest = RRT_DEST(rr);		/* Save last reachable ASB Address. 	*/
	    if (RRT_CHANGE(rr) != E_UNCHANGE) {
		/* 
		 * If the route is new or changed, inject new self-originated ASB Summary LSAs
		 * into non-backbone area.
		 */
		if ((IAmBorderRtr) && (a->area_id == OSPF_BACKBONE) &&
		    (ADV_RTR(RRT_V(rr)) != MY_ID)) {
		    build_sum_asb(a, rr, from_area);
		}
		RRT_CHANGE(rr) = E_UNCHANGE;
	    }
	}
    }

    BIT_RESET(a->spfsched, SUMASBSCHED);

    return FLAG_NO_PROBLEM;
}

/**/

/* Support for ASEs */

struct FORWARD_CACHE {
    u_int32 fwd_addr;
    u_int32 fwd_use;
    struct LSDB *fwd_par;
    struct NH_BLOCK *fwd_nh;
};

/*
 * Handle all routes imported from another AS
 * 	- called by spf() or rxlinkup()
 *   	- intra or sum may have been run before, but it ain't necessarily so
 *	- will just keep one equal cost route per border rtr/forwarding addr
 */
int
ase __PF3(area, struct AREA *,	/* area resulting in ase() being called */
	  from, int,		/* what was (intra or sum run before ase) flag */
	  partial, int)		/* True if called for rxlinkup */
{
    int rc = FLAG_NO_PROBLEM;
    rt_entry *old_route = (rt_entry *) 0;
    register struct LSDB_HEAD *hp;
    struct FORWARD_CACHE forward_cache[FORWARD_CACHE_SIZE];
    struct FORWARD_CACHE *forward_cache_end = forward_cache;

    if (!(from & INTSCHED)) {
	area->spfcnt++;
	RTAB_REV++;		/* intra and sum haven't been run */
    }

    trace_tf(ospf.trace_options,
	     TR_OSPF_SPF,
	     0,
	     ("OSPF SPF Area %A running ASE",
	      sockbuild_in(0, area->area_id)));

    /* The fowarding cache should be clear */
    bzero((caddr_t) forward_cache, sizeof forward_cache);

    LSDB_HEAD_LIST(area->htbl[LS_ASE], hp, 0, HTBLSIZE) {
	register struct LSDB *db;
	register struct LSDB *low = LSDBNULL;
	register int low_etype = TRUE;
	register u_int32 low_type2cost = ASELSInfinity;

	if ((partial && DBH_RERUN(hp)) || !(partial)) {
	    /* non-partial update or partial update sched for this row */
	    DBH_RERUN(hp) = FALSE;
	    LSDB_LIST(hp, db) {

		/* Reasonable looking DB? */
		if (BIG_METRIC(db) != ASELSInfinity
		    && !DB_FREEME(db)
		    && LS_AGE(db) < MaxAge
		    && ADV_RTR(db) != MY_ID) {
		    struct OSPF_ROUTE *rr = (struct OSPF_ROUTE *) 0;

		    /* check to see if we know about this ASBR */
		    /* ASBR may have changed so if not active lookup.. */
		    if (GOT_A_BDR(db) && ASBRTR_ACTIVE(db)) {
			rr = db->lsdb_border->lsdb_asb_rtr;
		    } else {
			rr = rtr_findroute(0,
					   ADV_RTR(db),
					   DTYPE_ASBR,
					   PTYPE_INT);
			if (rr) {
			    db->lsdb_border = RRT_V(rr);
			} else {
			    db->lsdb_border = (struct LSDB *) 0;
			}
		    }
		    if (rr && db->lsdb_border) {
			register struct LSDB *par;
			register u_int32 cost, type2cost;
			register int etype;
			u_int nh_cnt;
			struct NH_BLOCK **nh;

			/* We have an AS border router */

			if (DB_ASE_FORWARD(db)) {
			    register struct FORWARD_CACHE *fp;

			    /* Forwarding address cache */

			    /* See if this next hop is in our cache */
			    for (fp = forward_cache; fp < forward_cache_end; fp++) {
				if (fp->fwd_addr == DB_ASE_FORWARD(db)) {
				    /* Found it */

				    fp->fwd_use++;
				    if (!fp->fwd_par) {
					/* Negative cache entry */

					goto next_db;
				    }
				    break;
				}
			    }

			    if (fp == forward_cache_end) {
				register rt_entry *rt;

				rt = rt_lookup(RTS_NETROUTE,
					       RTS_DELETE,
					       sockbuild_in(0, DB_ASE_FORWARD(db)),
					       RTPROTO_BIT(RTPROTO_OSPF));

				/* Add to forward cache */
				if (forward_cache_end - forward_cache < FORWARD_CACHE_SIZE) {
				    /* Because of lookup above, fp points to next entry to use */

				    forward_cache_end++;
				} else {
				    register struct FORWARD_CACHE *fp1 = (fp = forward_cache);
				    
				    /* Reuse the least used entry */

				    while (++fp1 < forward_cache_end) {
					if (fp1->fwd_use < fp->fwd_use) {
					    fp = fp1;
					}
				    }
				}

				fp->fwd_use = 1;
				fp->fwd_addr = DB_ASE_FORWARD(db);

				if (rt
				    && rt->rt_dest_mask != inet_mask_default
				    && sock2ip(RT_IFAP(rt)->ifa_addr) != DB_ASE_FORWARD(db)
				    && (from == PTYPE_EXT
					|| ospf_int_active(from, area, rt))) {
				    /* This is a positive cache entry */

				    fp->fwd_par = ORT_V(rt);
				    if (DB_DIRECT(fp->fwd_par)) {
					fp->fwd_nh = ospf_nh_add(ORT_IO_NDX(rt, 0),
								 DB_ASE_FORWARD(db),
								 NH_DIRECT_FORWARD);
				    } else {
					fp->fwd_nh = (struct NH_BLOCK *) 0;
				    }
				} else {
				    /* This is a negative cache entry */

				    fp->fwd_par = (struct LSDB *) 0;
				    trace_tf(ospf.trace_options,
					     TR_ALL,
					     0,
					     ("ase: forwarding address lookup failed for ASE %A RTRID %A FORWARD %A",
					      sockbuild_in(0, LS_ID(db)),
					      sockbuild_in(0, ADV_RTR(db)),
					      sockbuild_in(0, DB_ASE_FORWARD(db))));
				    goto next_db;
				}
			    }

			    par = fp->fwd_par;
			    cost = par->lsdb_dist;
			    if (fp->fwd_nh) {
				nh = &fp->fwd_nh;
				nh_cnt = 1;
			    } else {
				nh = par->lsdb_nh;
				nh_cnt = par->lsdb_nhcnt;
			    }
			} else {
			    par = RRT_V(rr);
			    cost = RRT_COST(rr);
			    nh = par->lsdb_nh;
			    nh_cnt = par->lsdb_nhcnt;
			}

			/* Set metric for external type */
			if ((etype = ASE_TYPE2(db))) {
			    type2cost = BIG_METRIC(db);
			} else {
			    cost += BIG_METRIC(db);
			    type2cost = 0;
			}

			/* Have a valid parent? */
			if (par
			    && cost < ASELSInfinity) {
			    /* Add equal cost route */

			    if (low
				&& (etype > low_etype
				    || (etype == low_etype
					&& (type2cost > low_type2cost
					    || (type2cost == low_type2cost
						&& cost > low->lsdb_dist))))) {
				/* Not a better route */

				goto next_db;
			    }

			    if (!low
				|| etype != low_etype
				|| type2cost != low_type2cost
				|| cost != low->lsdb_dist) {
				/* New or better route */

				if (low) {
				    /* Free old list */
				
				    ospf_nh_free_list(low->lsdb_nhcnt, low->lsdb_nh);
				}
			    
				low = db;
				low_etype = etype;
				low_type2cost = type2cost;
				low->lsdb_dist = cost;
				ospf_nh_free_list(low->lsdb_nhcnt, low->lsdb_nh);
			    }

			    /* Merge next hops */
			    low->lsdb_nhcnt = ospf_nh_merge(low->lsdb_nhcnt, low->lsdb_nh,
							    nh_cnt, nh);
			}
		    }
		next_db: ;
		}

		/* Grab old route for deletion with partial update */
		if (DB_WHERE(db) != ON_ASE_LIST
		    && db->lsdb_route) {
		    old_route = db->lsdb_route;
		}
		if (!db->lsdb_next ||
		    (low && ((LS_ID(db->lsdb_next) != LS_ID(low)))) ||
 		    (old_route && (LS_ID(db->lsdb_next) != RT_DEST(old_route)))) {
		    /* Nexthops were added? */
		    if (low && low->lsdb_nhcnt) {
			if (addroute(area, low, from, area)) {
			    rc = FLAG_NO_BUFS;
			    goto Return;
			}
		    }
		    /* Modify partial update */
		    if (partial) {
			if (low
			    && DB_WHERE(db) != ON_ASE_LIST
			    && low->lsdb_route) {
			    old_route = low->lsdb_route;
			}
			if (old_route) {
			    ospf_route_update(old_route, area, ASESCHED);
			}
		    }
		    low = LSDBNULL;
		    low_type2cost = ASELSInfinity;
		    low_etype = 1;
		    old_route = (rt_entry *) 0;
		}
	    } LSDB_LIST_END(hp, db) ;
	}			/* Partial */

    } LSDB_HEAD_LIST_END(area->htbl[LS_ASE], hp, 0, HTBLSIZE) ;

    BIT_RESET(area->spfsched, ASESCHED);

 Return:
    return rc;
}
