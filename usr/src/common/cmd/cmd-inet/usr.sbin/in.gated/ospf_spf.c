#ident	"@(#)ospf_spf.c	1.3"
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
 * Add next hop entry to nh_block
 */
struct NH_BLOCK *
ospf_nh_add __PF3(ifap, if_addr *,
		  nh_addr, u_int32,
		  type, int)
{
    register struct NH_BLOCK *np;

    GNTOHL(nh_addr);

    NH_LIST(np) {
	if (nh_addr < ntohl(np->nh_addr)) {
	    /* They are in order so we must have passed it */

	    break;
	} else if (nh_addr == ntohl(np->nh_addr) &&
	    type == np->nh_type &&
	    ifap == np->nh_ifap) {
	    /* We have a match */

	    goto Return;
	}
    } NH_LIST_END(np) ;

    /* Allocate a new one */
    /* Insert before the current element */
    INSQUE(task_block_alloc(ospf_nh_block_index), np->nh_back);
    np = np->nh_back;
    assert(!np->nh_addr);

    /* Fill it in */
    np->nh_addr = htonl(nh_addr);
    np->nh_type = type;
    IFA_ALLOC(np->nh_ifap = ifap);

#ifdef	DEBUG
    trace_tf(ospf.trace_options,
	     TR_OSPF_DEBUG,
	     0,
	     ("ospf_nh_add: ADD %A type %d interface %A (%s)",
	      sockbuild_in(0, np->nh_addr),
	      np->nh_type,
	      np->nh_ifap->ifa_addr,
	      np->nh_ifap->ifa_link->ifl_name));
#endif	/* DEBUG */

 Return:
    return np;
}


/*
 *	Clean dead wood out of NH cache
 */
int
ospf_nh_collect __PF0(void)
{
    int count = 0;
    register struct NH_BLOCK *np;

    NH_LIST(np) {
	if (!np->nh_refcount) {
#ifdef	DEBUG
	    trace_tf(ospf.trace_options,
		     TR_OSPF_DEBUG,
		     0,
		     ("ospf_nh_collect: DELETE %A type %d interface %A (%s)",
		      sockbuild_in(0, np->nh_addr),
		      np->nh_type,
		      np->nh_ifap->ifa_addr,
		      np->nh_ifap->ifa_link->ifl_name));
#endif	/* DEBUG */

	    IFA_FREE(np->nh_ifap);

	    REMQUE(np);
	
	    task_block_free(ospf_nh_block_index, (void_t) np);

	    count++;
	}
    } NH_LIST_END(np) ;

    return count;
}


/*
 * Find appropriate non-virtual nh_block entry
 */
static struct NH_BLOCK *
ospf_nh_find __PF2(v, struct LSDB *,
		   if_ip_addr, u_int32)
{
    register struct NH_BLOCK *np = &ospf.nh_list;

    switch (LS_TYPE(v)) {
    case LS_STUB:
	/* either host or point-to-point interface */
	if (DB_NET(v)->net_mask == HOST_NET_MASK) {
	    NH_LIST(np) {
		if (BIT_TEST(np->nh_ifap->ifa_state, IFS_POINTOPOINT|IFS_LOOPBACK) &&
		    np->nh_addr == LS_ID(v))
		    break;
	    } NH_LIST_END(np) ;
	} else {
	    NH_LIST(np) {
		if (!BIT_TEST(np->nh_ifap->ifa_state, IFS_POINTOPOINT|IFS_LOOPBACK) &&
		    NH_NET(np) == DB_NETNUM(v))
		    break;
	    } NH_LIST_END(np) ;
	}
#ifdef NOTDEF
	/* go through host list, if should be configured */
	/* need to look up IF_NDX of host??? */
	for (host = ospf.area[i].hosts.ptr[NEXT],
	     host != HOSTNULL;
	     host = host->ptr[NEXT])
	    if (host->ipaddr == LS_ID(v))
		break;
#endif
	break;

    case LS_NET:
	NH_LIST(np) {
	    if (!BIT_TEST(np->nh_ifap->ifa_state, IFS_POINTOPOINT|IFS_LOOPBACK) &&
		NH_NET(np) == DB_NETNUM(v))
		break;
	} NH_LIST_END(np) ;
	break;

    case LS_RTR:
	/* Find adjacent neighbor */
	NH_LIST(np) {
	    if (np->nh_addr == if_ip_addr)
		break;
	} NH_LIST_END(np) ;
	break;
    }

    if (np == &ospf.nh_list) {
	trace_log_tf(ospf.trace_options,
		     0,
		     LOG_ERR,
		     ("ospf_nh_find: FAILED!  LinkStateType %s LinkStateID %A AdvRtr %A NextHop %A",
		      trace_state(ospf_ls_type_bits, LS_TYPE(v)),
		      sockbuild_in(0, LS_ID(v)),
		      sockbuild_in(0, ADV_RTR(v)),
		      sockbuild_in(0, if_ip_addr)));

	assert(FALSE);
    }

    return np;
}


/*
 * Combine the next hop lists of parents in sorted order
 */
u_int
ospf_nh_merge __PF4(v_nhcnt, u_int,
		    v_nh, struct NH_BLOCK **,
		    p_nhcnt, u_int,
		    p_nh, struct NH_BLOCK **)
{
    struct NH_BLOCK *p[RT_N_MULTIPATH * 2];
    register u_int i;
    int tot = v_nhcnt + p_nhcnt;
    int need_alloc[RT_N_MULTIPATH * 2];
    u_int vndx = 0, pndx = 0;

    /* merge lists into p based on nh_addr */
    for (i = 0; i < tot; i++) {
	if (vndx >= v_nhcnt) {
	    p[i] = p_nh[pndx];
	    need_alloc[i] = TRUE;
	    pndx++;
	} else if (pndx >= p_nhcnt) {
	    p[i] = v_nh[vndx];
	    need_alloc[i] = FALSE;
	    vndx++;
	} else if (v_nh[vndx] == p_nh[pndx]) {
	    /* same next hop */
	    p[i] = v_nh[vndx];
	    need_alloc[i] = FALSE;
	    vndx++;
	    pndx++;
	    tot--;
	} else if (ntohl(v_nh[vndx]->nh_addr) <= ntohl(p_nh[pndx]->nh_addr)) {
	    p[i] = v_nh[vndx];
	    need_alloc[i] = FALSE;
	    vndx++;
	} else {
	    p[i] = p_nh[pndx];
	    need_alloc[i] = TRUE;
	    pndx++;
	}
    }

    /* Copy the maximum number that will fit */
    v_nhcnt = MIN(RT_N_MULTIPATH, tot);
    for (i = 0; i < v_nhcnt; i++) {
	v_nh[i] = p[i];
	if (need_alloc[i]) {
	    OSPF_NH_ALLOC(v_nh[i]);
	}
    }

    /* Free any remaining from the old list */
    for (; i < tot; i++) {
	if (!need_alloc[i]) {
	    ospf_nh_free(&p[i]);
	}
    }

    return v_nhcnt;
}


/**/

/*
 *      Spf(a) has just been run - cleanup unused stubs
 */
static void
stub_cleanup __PF1(a, struct AREA *)
{
    register struct LSDB_HEAD *hp;

    LSDB_HEAD_LIST(a->htbl[LS_STUB], hp, 0, HTBLSIZE) {
	register struct LSDB *db;

	LSDB_LIST(hp, db) {
	    if (!db->lsdb_route) {
		db_free(db, LS_STUB);
	    }
	} LSDB_LIST_END(hp, db) ;
    } LSDB_HEAD_LIST_END(a->htbl[LS_STUB], hp, 0, HTBLSIZE) ;
}


/*
 *  spfinit()    free parent list and reset vertex variables
 */
static void
spfinit __PF1(a, struct AREA *)
{
    int type;

    for (type = LS_STUB; type <= LS_NET; type++) {
	register struct LSDB_HEAD *hp;
    
	LSDB_HEAD_LIST(a->htbl[type], hp, 0, HTBLSIZE) {
	    register struct LSDB *db;

	    LSDB_LIST(hp, db) {
		if (DB_FREEME(db)) {
		    continue;
		}
		DB_WHERE(db) = UNINITIALIZED;
		DB_VIRTUAL(db) = FALSE;
		DB_REMQUE(db);
		switch (type) {
		case LS_NET:
		case LS_STUB:
		    if (db->lsdb_route) {
			ORT_CHANGE(db->lsdb_route) = E_UNCHANGE;
		    }
		    break;

		default:
		    if (db->lsdb_ab_rtr) {
			RRT_CHANGE(db->lsdb_ab_rtr) = E_UNCHANGE;
		    }
		    if (db->lsdb_asb_rtr) {
			RRT_CHANGE(db->lsdb_asb_rtr) = E_UNCHANGE;
		    }
		    break;
		}
		db->lsdb_dist = SUMLSInfinity;
		ospf_nh_free_list(db->lsdb_nhcnt, db->lsdb_nh);
	    } LSDB_LIST_END(hp, db) ;
	} LSDB_HEAD_LIST_END(a->htbl[type], hp, 0, HTBLSIZE) ;
    }
}

/*
 * Resolve virtual next hops
 */
static int
resolve_virtual __PF2(v, struct LSDB *,
		      parent, struct LSDB *)
{
    int hash;
    struct LSDB *db;
    struct AREA *trans_area = v->lsdb_trans_area;
    struct OSPF_ROUTE *rr;
    u_int32 newmetric = SUMLSInfinity;
   
    switch (LS_TYPE(v)) {
    case LS_NET:
    case LS_STUB:
	/* Zip through sum nets for trans_area */
	hash = XHASH(DB_NETNUM(v), 0);
	LSDB_LIST(&trans_area->htbl[LS_SUM_NET][hash], db) {
	    /* or mania */
	    if ((BIG_METRIC(db) == SUMLSInfinity) ||
		(DB_FREEME(db)) ||
		(LS_AGE(db) >= MaxAge) ||
		(ADV_RTR(db) == MY_ID) ||
		(DB_NETNUM(v) != DB_NETNUM(db))) {
		continue;
	    }
	    if (!(rr = rtr_findroute(trans_area,
				     ADV_RTR(db),
				     DTYPE_ABR,
				     PTYPE_INTRA))) {
		continue;
	    }
	    if ((RRT_COST(rr) + BIG_METRIC(db)) < newmetric) {
		/* MODIFIED 8/19/92 - check for DB_NH_CNT */
		if (parent->lsdb_nhcnt) {
		    v->lsdb_dist = (newmetric = RRT_COST(rr) + BIG_METRIC(db));
		    ospf_nh_set(v->lsdb_nhcnt, v->lsdb_nh,
				RRT_NH_CNT(rr), RRT_NH(rr));
		}
	    }
	} LSDB_LIST_END(&trans_area->htbl[LS_SUM_NET][hash], db) ;
	break;
	
    case LS_RTR:
	hash = XHASH(LS_ID(v), 0);
	/*
	 * resolve only if AS Border route
	 */
	if ((ntohs(DB_RTR(v)->E_B) & bit_E)) {
	    /*
	     * First check for intra-area route
	     */
	    rr = rtr_findroute(trans_area,
			      LS_ID(v),
			      DTYPE_ASBR,
			      PTYPE_INTRA);
	    if (rr) {
		v->lsdb_dist = RRT_COST(rr);
		ospf_nh_set(v->lsdb_nhcnt, v->lsdb_nh,
			    RRT_NH_CNT(rr), RRT_NH(rr));
		return TRUE;
	    }
	    /*
	     * Zip through sum ASBs for trans_area
	     */
	    LSDB_LIST(&trans_area->htbl[LS_SUM_ASB][hash], db) {
		/* or mania */
		if ((BIG_METRIC(db) == SUMLSInfinity) ||
		    (DB_FREEME(db)) ||
		    (LS_AGE(db) >= MaxAge) ||
		    (ADV_RTR(db) == MY_ID) ||
		    (LS_ID(db) != LS_ID(v))) {
		    continue;
		}
		if (!(rr = rtr_findroute(trans_area,
					 ADV_RTR(db),
					 DTYPE_ABR,
					 PTYPE_INTRA))) {
		    continue;
		}
		if ((RRT_COST(rr) + BIG_METRIC(db)) < newmetric) {
		    v->lsdb_dist = (newmetric = RRT_COST(rr) + BIG_METRIC(db));
		    ospf_nh_set(v->lsdb_nhcnt, v->lsdb_nh,
				RRT_NH_CNT(rr), RRT_NH(rr));
		}
	    } LSDB_LIST_END(&trans_area->htbl[LS_SUM_ASB][hash], db) ;
	} else {
	    /*
	     * Non-asb router link
	     */
	    DB_VIRTUAL(v) = TRUE;
	    v->lsdb_trans_area = trans_area;
	    v->lsdb_nhcnt = ospf_nh_merge(v->lsdb_nhcnt, v->lsdb_nh,
					  parent->lsdb_nhcnt, parent->lsdb_nh);
	}
	break;

    default:
	break;
    }

    /* MODIFIED 8/19/92 - check for DB_NH_CNT */
    if (v->lsdb_nhcnt) {
	return TRUE;
    }

    return FALSE;
}


/*
 *	Add parent to vertex
 *	- used by all entries except for ase() who rolls its own
 */
int
ospf_add_parent __PF7(v, struct LSDB *,
		      parent, struct LSDB *,
		      newdist, u_int32,
		      a, struct AREA *,
		      nh, u_int32,
		      vr, struct OSPF_ROUTE *,
		      trans_area, struct AREA *)
{
    int ret = TRUE;
    
    if (v == DB_FIRSTQ(a->spf))
	return ret;

    /*
     * If there are no parents or this is an Intra area route
     * place on candidate list
     */
    /* MODIFIED 7/24/92 */
    v->lsdb_dist = newdist;
    if (!v->lsdb_nhcnt && LS_TYPE(v) < LS_SUM_NET) {
	/*
	 * put it in order on the candidate list
	 */
	if (LS_TYPE(v) != LS_STUB) {
	    struct LSDB *nextv;

	    /* Remove from previous queue */
	    DB_REMQUE(v);

	    DB_RUNQUE(a->candidates, nextv) {
		if (nextv->lsdb_dist < v->lsdb_dist) {
		    continue;
		}
		if (nextv->lsdb_dist > v->lsdb_dist
		    || LS_TYPE(nextv) != LS_NET) {
		    break;
		}
	    } DB_RUNQUE_END(a->candidates, nextv) ;

	    /* Insert before this element */
	    DB_INSQUE(v, nextv->lsdb_back);
	}
	DB_WHERE(v) = ON_CLIST;
    }

    /*
     * set rtr if addr for virtual link dest ip addr
     */
    if (trans_area) {
	/*
	 * virtual link
	 */
	ospf_nh_set(v->lsdb_nhcnt, v->lsdb_nh,
		    RRT_NH_CNT(vr), RRT_NH(vr));
	DB_VIRTUAL(v) = TRUE;
	v->lsdb_trans_area = trans_area;
    } else if ((DB_VIRTUAL(parent) == TRUE) && (LS_TYPE(v) < LS_SUM_NET)) {
	/*
	 * Flag for children of this virtual vertex
	 */
	DB_VIRTUAL(v) = TRUE;
	v->lsdb_trans_area = parent->lsdb_trans_area;
	/*
	 * If transit area hasn't come around yet
	 * remove from candidate list
	 */
	/* MODIFIED 8/19/92  */
	if (!resolve_virtual(v, parent)) {
	    if (LS_TYPE(v) != LS_STUB) {
		DB_REMQUE(v);
	    }
	    ret = FALSE;
	}
    } else if (ADV_RTR(parent) == MY_ID) {
	struct NH_BLOCK *nhndx;

	/* if adjacent to this rtr, first hop is direct */
	nhndx = ospf_nh_find(v, nh);
	v->lsdb_nhcnt = ospf_nh_merge(v->lsdb_nhcnt, v->lsdb_nh,
				      1, &nhndx);
	DB_DIRECT(v) = TRUE;
    } else if (DB_DIRECT(parent) && LS_TYPE(parent) == LS_NET) {
	struct NH_BLOCK *nhndx;

	/* if this rtr is DR (parent) - is also direct */

	if (ADV_RTR(v) == MY_ID) {
	    nhndx = ospf_nh_find(v, nh);
	} else {
	    nhndx = ospf_nh_add(parent->lsdb_nh[0]->nh_ifap,
				nh,
				NH_NBR);
	}

	v->lsdb_nhcnt = ospf_nh_merge(v->lsdb_nhcnt, v->lsdb_nh,
				      1, &nhndx);
    } else {
	/*
	 * bind to parent's next hop interface addr
	 */
	v->lsdb_nhcnt = ospf_nh_merge(v->lsdb_nhcnt, v->lsdb_nh,
				      parent->lsdb_nhcnt, parent->lsdb_nh);
	if (LS_TYPE(v) == LS_STUB
	    && v->lsdb_nh[0]->nh_type == NH_LOCAL) {
	    /* This is local end of P2P interface */
	    DB_DIRECT(v) = TRUE;
	}
    }

    return ret;
}


/*
 * Verify that the next router vertex in the Dijkstra
 * directed graph has a specific transit network in
 * its list of router links. If found, return the
 * next hop in next_hop_address.
 */
static int
ospf_rtr_netbacklink __PF3(next_router_vertex, struct LSDB *,
			   current_network_vertex, struct LSDB *,
			   next_hop_address, u_int32 *)
{
    int i;
    struct RTR_LA_PIECES *linkp;
    int cnt = ntohs(DB_RTR(next_router_vertex)->lnk_cnt);

    *next_hop_address = 0;
    if (cnt == 0) {
        return FALSE;
    }
  
    for (i = 0, linkp = &DB_RTR(next_router_vertex)->link;
         i < cnt;
         i++, linkp = (struct RTR_LA_PIECES *) ((long) linkp +
						RTR_LA_PIECES_SIZE +
						((linkp->metric_cnt) * RTR_LA_METRIC_SIZE))) {
        if (linkp->lnk_id == LS_ID(current_network_vertex) &&
	    linkp->con_type == RTR_IF_TYPE_TRANS_NET) {
	    *next_hop_address = linkp->lnk_data;
	    return TRUE;
	}
    }

    return FALSE;
}


/*
 * Verify that the next router vertex in the Dijkstra
 * directed graph has a router in its list of router
 * links. For point-to-point interfaces, also verify
 * that a link data for the current link corresponds
 * to the next stub network.
 *
 * N.B. The P2P checking is dependent on GateD's implementation
 * of router links LSAs. Since the checking was added to allow
 * direct next hops to be selected correctly this dependency
 * accepted.
 */
static int
ospf_rtr_rtrbacklink __PF6(next_router_vertex, struct LSDB *,
		      current_router_vertex, struct LSDB *,
		      current_linkp, struct RTR_LA_PIECES *,
		      current_nextlinkp, struct RTR_LA_PIECES *,
		      a, struct AREA *, 
		      next_hop_address, u_int32 *)
{
    int i;
    struct RTR_LA_PIECES *linkp;
    int cnt = ntohs(DB_RTR(next_router_vertex)->lnk_cnt);
    u_int32 metric = RTRLSInfinity;
 
    *next_hop_address = 0;
    if (cnt == 0) {
	return FALSE;
    }
 
    for (i = 0, linkp = &DB_RTR(next_router_vertex)->link;
	 i < cnt;
	 i++, linkp = (struct RTR_LA_PIECES *) ((long) linkp +
						RTR_LA_PIECES_SIZE +
						((linkp->metric_cnt) * RTR_LA_METRIC_SIZE))) {
	if (linkp->lnk_id == LS_ID(current_router_vertex) &&
	    linkp->con_type == current_linkp->con_type) {
	    if (linkp->con_type == RTR_IF_TYPE_RTR) {
		/*
		 * Check the next link to determine if it is a stub network corresponding
		 * to the router's P2P interface. The additional checks are required to
		 * assure a match is not found in the case of an unnumbered link or a
		 * Point-to-Multipoint link (currently an Internet Draft).
		 */
		if (current_nextlinkp &&
		    current_nextlinkp->con_type == RTR_IF_TYPE_STUB_NET &&
		    current_nextlinkp->lnk_id == linkp->lnk_data) {
		    *next_hop_address = linkp->lnk_data;

		    return TRUE;
		} else {
		    if (linkp->tos0_metric < metric ||
			(linkp->tos0_metric == metric &&
			 linkp->lnk_data > *next_hop_address)) {
			*next_hop_address = linkp->lnk_data;
			metric = linkp->tos0_metric;
		    }
		}
	    } else {
		/*
		 * Ideally, we would match up virtual links with
		 * transit areas. However, the information is not
		 * available to do this so we return success if
		 * any virtual link is found. For virtual links, the
		 * link data should not be used as a next hop (hence,
		 * it is not returned).
		 */

		return TRUE;
	    }
	}
    }

/*
* Unnumbered point-to-point link support.
* If the next router vertex is using unnumbered point-to-point
* interfaces, then its Link Data will contain an ifIndex, not an 
* Ip address (see RFC 1583, 12.4.1). But we need the IP interface
* address, so our only choice is to use the Link ID, which may
* be mapped to a neighbor IP address on direct attachments.
* Non-direct attachments don't really need a real nexthop address, anyway.
*/
#define IS_UNNUMBERED_ADDR(addr) (((addr) & htonl(IN_CLASSA_NET)) == 0)

if(*next_hop_address && IS_UNNUMBERED_ADDR(*next_hop_address)
&& (ADV_RTR(current_router_vertex) == MY_ID)) {
  struct INTF *intf;
  struct NBR *n;
  u_int32 neighbor_id = current_linkp->lnk_id;

INTF_LIST(intf,a) {
NBRS_LIST(n,intf) {
if(neighbor_id == NBR_ID(n)) {
  *next_hop_address = NBR_ADDR(n);
return TRUE;
}
} NBRS_LIST_END(n,intf);
} INTF_LIST_END(intf,a);

/* if there's no matching neighbor, we smell trouble */
*next_hop_address = 0;
}

    return (*next_hop_address) ? TRUE : FALSE;
}


/*
 *  	Check net vertex to see if there is a link back to id
 */
static int
ospf_net_backlink __PF2(id, u_int32,
			db, struct LSDB *)
{
    int i;
    u_int cnt = (u_int) (ntohs(LS_LEN(db)) - NET_LA_HDR_SIZE) / 4;
    struct NET_LA_PIECES *att_rtr;

    for (i = 0, att_rtr = &DB_NET(db)->att_rtr; i < cnt; i++, att_rtr++) {
	if (att_rtr->lnk_id == id)
	    return TRUE;
    }

    return FALSE;
}


/*
 * Update intra area routing table updates
 */
static int
rtr_update __PF1(area, struct AREA *)
{
    struct OSPF_ROUTE *rr, *next;
    area->asbr_cnt = 0;
    area->abr_cnt = 0;

    /*
     * Update as border rtrs
     */
    for (rr = area->asbrtab.ptr[NEXT];
	 rr;
	 rr = next) {
	next = RRT_NEXT(rr);
	/*
	 * Add Asb rtr summary info to other areas - prepare to flood
	 */
	if ((IAmBorderRtr) && (RRT_ADVRTR(rr) != MY_ID)) {
	    if (build_sum_asb(area, rr, area)) {
		area->spfsched |= INTRASCHED;
		return FLAG_NO_BUFS;
	    }
	}
	if (RRT_REV(rr) != RTAB_REV) {   /* no longer valid route *//*
					 * If not marked invalid, flag build_sum_asb to remove
					 * for now  unbind from lsdb entry
					 */
	    if (RRT_V(rr)) {
		RRT_V(rr)->lsdb_asb_rtr = (struct OSPF_ROUTE *) 0;
		DB_DIRECT(RRT_V(rr)) = FALSE;
		ospf_nh_free_list(RRT_V(rr)->lsdb_nhcnt, RRT_V(rr)->lsdb_nh);	/* free parent vertex list */
	    }
	    /*
	     * delete from routing table
	     */
	    ospf_nh_free_list(RRT_NH_CNT(rr), RRT_NH(rr));
	    DEL_Q(rr, ospf_route_index);
	} else
	    area->asbr_cnt++;
    }

    /*
     * Update area border rtrs
     * These are entered into the routing tble if this area isn't the
     * backbone for virtual endpoints to exchange routing info
     */
    for (rr = area->abrtab.ptr[NEXT];
	 rr;
	 rr = next) {
	next = RRT_NEXT(rr);
	if (RRT_REV(rr) != RTAB_REV) {
	    /*
	     * no longer valid route
	     * unbind from lsdb entry
	     */
	    if (RRT_V(rr)) {
		RRT_V(rr)->lsdb_ab_rtr = (struct OSPF_ROUTE *) 0;
		/* free parent vertex list */
		DB_DIRECT(RRT_V(rr)) = FALSE;
		ospf_nh_free_list(RRT_V(rr)->lsdb_nhcnt, RRT_V(rr)->lsdb_nh);
	    }
	    ospf_nh_free_list(RRT_NH_CNT(rr), RRT_NH(rr));
	    DEL_Q(rr, ospf_route_index);
	} else if (area->area_id != OSPF_BACKBONE && RRT_ADVRTR(rr) != MY_ID) {
	    RRT_CHANGE(rr) = E_UNCHANGE;
	    area->abr_cnt++;
	}
    }
    return FLAG_NO_PROBLEM;
}


/*
 * get_trans_area() - return area if nbr_id of virtual link == nh_addr
 */
static struct AREA *
get_trans_area __PF1(virt_id, u_int32)
{
    struct INTF *intf;

    VINTF_LIST(intf) {
	if (NBR_ID(&intf->nbr) == virt_id) {
	    return intf->trans_area;
	}
    } VINTF_LIST_END(intf) ;
    return AREANULL;
}


/*
 *	set_virtual_addr() - called by intra when
 *		1) virtual links are configured,
 * 		2) v is a rtr adv with the ABR bit set
 *		3) area is transit area
 *	set up ifspfndx and virtual nbr's ip address
 */
static void
set_virtual_addr __PF3(v, struct LSDB *,
		       area, struct AREA *,
		       if_ip_addr, u_int32)
{
    struct INTF *intf;

    VINTF_LIST(intf) {
	if (ADV_RTR(v) == NBR_ID(&intf->nbr) &&
	    intf->trans_area == area) {
	    if (intf->nbr.nbr_addr) {
		sockfree(intf->nbr.nbr_addr);
	    }
	    intf->nbr.nbr_addr = sockdup(sockbuild_in(0, if_ip_addr));
	    IFA_ALLOC(intf->ifap = v->lsdb_nh[0]->nh_ifap);
	    intf->nbr.intf = intf;
	    break;
	}
    } VINTF_LIST_END(intf) ;
}


/*
 * First handle all intra-area routes
 */
static int
intra __PF1(a, struct AREA *)
{
    int i, cnt;
    u_int32 newdist;
    int foundlsa, was_added;
    int nomem = 0;		/* if out of memory will run again in
				 * tq_lsa_lock */
    u_int lookup;			/* RTR or NET LSA lookup in LSDB */
    struct LSDB *v, *nextv;	/* current spfnode */
    struct NET_LA_PIECES *att_rtr;
    struct RTR_LA_PIECES *linkp, *nextlinkp;
    struct _qelement stublist;
    struct LSDB *stub;
    u_int32 nh;			/* for discovering next hop address */

    trace_tf(ospf.trace_options,
	     TR_OSPF_SPF,
	     0,
	     ("OSPF SPF Area %A running Intra",
	      sockbuild_in(0, a->area_id)));

    DB_INITQ(stublist);
    
    v = FindLSA(a, MY_ID, MY_ID, LS_RTR);
    assert(v);

    v->lsdb_dist = 0;
    DB_WHERE(v) = ON_RTAB;
    DB_ADDQUE(a->spf, v);
    for (;
	 v != (struct LSDB *) &a->candidates;
	 v = DB_FIRSTQ(a->candidates)) {

	if (v == DB_FIRSTQ(a->candidates)) {
	    DB_REMQUE(v);
	}
	
	/* if no candidates left we are finished */
	if (LS_TYPE(v) != LS_STUB)
	    nomem = addroute(a, v, INTRASCHED, a);
	if (nomem) {
	    a->spfsched |= INTRASCHED;
	    return FLAG_NO_BUFS;
	}
	was_added = 0;
	
	if ((ADV_AGE(v) >= MaxAge) || DB_FREEME(v))
	    continue;

	switch (LS_TYPE(v)) {
	case LS_RTR:
	    /* first handle ls rtr update */
	    /*
	     * check all links
	     */
	    cnt = ntohs(DB_RTR(v)->lnk_cnt);
	    for (i = 1, linkp = (struct RTR_LA_PIECES *) &DB_RTR(v)->link;
		 i <= cnt;
		 i++, linkp = nextlinkp) {
		struct AREA *trans_area = AREANULL;	/* transit area for virtual route */
		struct OSPF_ROUTE *vr = (struct OSPF_ROUTE *) 0;	/* for virtual route */
		nextlinkp = (i<cnt) ? (struct RTR_LA_PIECES *) ((long) linkp + RTR_LA_PIECES_SIZE +
								((linkp->metric_cnt) * RTR_LA_METRIC_SIZE)) :
								    (struct RTR_LA_PIECES *) 0;

		newdist = ntohs(linkp->tos0_metric) + v->lsdb_dist;

		/*
		 * if it is a transit vertex check other side
		 */
		switch (linkp->con_type) {
		case RTR_IF_TYPE_VIRTUAL:
		    if (linkp->tos0_metric == RTRLSInfinity)
			continue;
		    lookup = LS_RTR;
		    /*
		     * check for virtual link
		     */
		    if (a->area_id == OSPF_BACKBONE && ADV_RTR(v) == MY_ID) {
			trans_area = get_trans_area(linkp->lnk_id);
			if (trans_area == AREANULL) {
			    continue;
			}
			/* lookup area bdr rtr for virtual link */
			vr = rtr_findroute(trans_area,
					   linkp->lnk_id,
					   DTYPE_ABR,
					   PTYPE_INTRA);
			if (!vr) {
			    continue;
			}
		    }
		    goto common;
		    
		case RTR_IF_TYPE_RTR:
		    if (linkp->tos0_metric == RTRLSInfinity)
			continue;
		    lookup = LS_RTR;
		    goto common;

		case RTR_IF_TYPE_TRANS_NET:
		    lookup = LS_NET;
		    /* Fall through */

		common:
		    /*
		     * if not in lsdb, next vertex
		     */
		    if (!(nextv = FindLSA(a, linkp->lnk_id, linkp->lnk_id, lookup))) {
			continue;
		    }

		    /*
		     * if it is too old, or on spf tree get next vertex
		     */
		    if ((DB_WHERE(nextv) == ON_RTAB) ||
			(ADV_AGE(nextv) >= MaxAge) ||
			(DB_FREEME(nextv))) {
			continue;
		    }
		    /*
		     * if no backlink, next vertex
		     */
		    switch (linkp->con_type) {
		    case RTR_IF_TYPE_RTR:
		    case RTR_IF_TYPE_VIRTUAL:
			if (!ospf_rtr_rtrbacklink(nextv,
						  v,
						  linkp,
						  nextlinkp,
						  a,
						  &nh)) {
			    continue;
			}
			/*
			 * This rtrs link data holds nbr's ip address
			 */
			break;

		    case RTR_IF_TYPE_TRANS_NET:
			if (!ospf_net_backlink(LS_ID(v),
					       nextv)) {
			    continue;
			}
			nh = linkp->lnk_data;
			break;

		    default:
			break;
		    }
		    /*
		     * if newdist is > than current, next vertex
		     */
		    if (newdist > nextv->lsdb_dist) {
			continue;
		    }
		    /*
		     * add equal cost route
		     */
		    if (newdist == nextv->lsdb_dist) {
			ospf_add_parent(nextv,
					v,
					newdist,
					a,
					nh,
					vr, trans_area);
			if (ntohs(DB_RTR(nextv)->E_B) & bit_B &&
			    BIT_TEST(a->area_flags, OSPF_AREAF_TRANSIT) &&
			    linkp->con_type == RTR_IF_TYPE_RTR) {
			    set_virtual_addr(nextv, a, nh);
			}
		    } else if (newdist < nextv->lsdb_dist) {
			ospf_nh_free_list(nextv->lsdb_nhcnt, nextv->lsdb_nh);
			DB_DIRECT(nextv) = FALSE;
			DB_VIRTUAL(nextv) = FALSE;
			ospf_add_parent(nextv,
					v,
					newdist,
					a,
					nh,
					vr, trans_area);
			/* Potential virtual neighbor */
			if (ntohs(DB_RTR(nextv)->E_B) & bit_B &&
			    BIT_TEST(a->area_flags, OSPF_AREAF_TRANSIT) &&
			    linkp->con_type == RTR_IF_TYPE_RTR) {
			    set_virtual_addr(nextv, a, nh);
			}
		    }
		    break;

		case RTR_IF_TYPE_STUB_NET:
		    /*
		     * stub -  will handle after spf algorithm
		     */
		    foundlsa = ospf_add_stub_lsa(&stub,
						 a,
						 RTR_ADV_NETNUM(linkp),
						 ADV_RTR(v),
						 linkp->lnk_data);
		    if (!foundlsa) {
			DBADV_ALLOC(stub, NET_LA_HDR_SIZE);
			LS_ID(stub) = linkp->lnk_id;
			ADV_RTR(stub) = ADV_RTR(v);
			DB_MASK(stub) = linkp->lnk_data;
			LS_LEN(stub) = htons(NET_LA_HDR_SIZE);
			stub->lsdb_time = v->lsdb_time;
			/* add ls checksum */
			ospf_checksum(DB_NET(stub), NET_LA_HDR_SIZE);
			was_added = ospf_add_parent(stub,
						    v,
						    newdist,
						    a,
						    linkp->lnk_id,
						    vr, trans_area);

			/* Trace this build */
			if (TRACE_TF(ospf.trace_options, TR_OSPF_LSA_BLD)) {
			    ospf_trace_build(a, a, stub->lsdb_adv, FALSE);
			}
		    } else {
			stub->lsdb_time = v->lsdb_time;
			if (newdist == stub->lsdb_dist)
			    was_added = ospf_add_parent(stub,
							v,
							newdist,
							a,
							linkp->lnk_id,
							vr, trans_area);
			if (newdist < stub->lsdb_dist) {
			    ospf_nh_free_list(stub->lsdb_nhcnt, stub->lsdb_nh);
			    DB_DIRECT(stub) = FALSE;
			    was_added = ospf_add_parent(stub,
							v,
							newdist,
							a,
							linkp->lnk_id,
							vr, trans_area);
			}
		    }
		    if (was_added) {
			/* Remove just in case... */
			DB_REMQUE(stub);
                        DB_ADDQUE(stublist, stub);
		    }
		    continue;

		default:
		    break;
		}
	    }			/* end of router links loops */
	    break;

	case LS_NET:
	    /* now zip through the net links */
	    cnt = ntohs(LS_LEN(v)) - NET_LA_HDR_SIZE;
	    for (att_rtr = &DB_NET(v)->att_rtr, i = 0;
		 i < cnt;
		 att_rtr++, i += 4) {

		/*
		 * check other side
		 */
		if (!(nextv = FindLSA(a, att_rtr->lnk_id, att_rtr->lnk_id, LS_RTR))) {
		    continue;
		}
		/*
		 * if it is on spftree, too old or no back link,
		 *      go to next vertex
		 */
		if (DB_WHERE(nextv) == ON_RTAB ||
		    ADV_AGE(nextv) >= MaxAge ||
		    DB_FREEME(nextv) ||
		    !ospf_rtr_netbacklink(nextv,
					  v,
					  &nh)) {
		    continue;
		}

		/*
		 * 0 cost on same net
		 */
		if (v->lsdb_dist > nextv->lsdb_dist) {
		    continue;
		} else if (v->lsdb_dist == nextv->lsdb_dist) {
		    ospf_add_parent(nextv,
				    v,
				    v->lsdb_dist,
				    a,
				    nh,
				    (struct OSPF_ROUTE *) 0, (struct AREA *) 0);
		    /*
		     * Potential virtual neighbor
		     */
		    if (ntohs(DB_RTR(nextv)->E_B) & bit_B &&
			BIT_TEST(a->area_flags, OSPF_AREAF_TRANSIT)) {
			set_virtual_addr(nextv, a, nh);
		    }
		} else if (v->lsdb_dist < nextv->lsdb_dist) {
		    DB_DIRECT(nextv) = FALSE;
		    ospf_nh_free_list(nextv->lsdb_nhcnt, nextv->lsdb_nh);
		    if (DB_WHERE(nextv) == ON_CLIST) {
			DB_REMQUE(nextv);
		    }
		    ospf_add_parent(nextv,
				    v,
				    v->lsdb_dist,
				    a,
				    nh,
				    (struct OSPF_ROUTE *) 0, (struct AREA *) 0);
		    /*
		     * Potential virtual neighbor
		     */
		    if (ntohs(DB_RTR(nextv)->E_B) & bit_B &&
			BIT_TEST(a->area_flags, OSPF_AREAF_TRANSIT)) {
			set_virtual_addr(nextv, a, nh);
		    }
		}
	    }
	    break;

	default:
	    break;
	}
    }

    /* now do stub nets; at this point shortest intra routes are known */
    while (!DB_EMPTYQ(stublist)
	   && (stub = DB_FIRSTQ(stublist))) {

	/* Remove from the list */
	DB_REMQUE(stub) ;

	/* check for an existing net - could have more than one entry */
	if (addroute(a, stub, INTRASCHED, a)) {
	    a->spfsched |= INTRASCHED;
	    return FLAG_NO_BUFS;
	    /* XXX - freeq? */
	}
    }

    return 0;
}


/*
 * Check virtual links to see if they have come up
 *	- if we have an intra area route to the virtual nbr, bring it up
 */
static void
virtual_check __PF1(area, struct AREA *)
{
    int match = FALSE;
    struct INTF *intf;

    VINTF_LIST(intf) {
	if (intf->trans_area == area) {
	    struct OSPF_ROUTE *rr = rtr_findroute(area,
						  NBR_ID(&intf->nbr),
						  DTYPE_ABR,
						  PTYPE_INTRA);

	    if (!rr ||
		RRT_COST(rr) >= RTRLSInfinity) {
		/* No route - bring link down */
		
		if (intf->state != IDOWN) {
		    match++;
		    (*(if_trans[INTF_DOWN][IDOWN])) (intf);
		}
	    } else if (intf->state == IDOWN) {
		/* We have a route and link is down - bring it up */

		match++;
		intf->cost = RRT_COST(rr);
		(*(if_trans[INTF_UP][IDOWN])) (intf);
	    } else if (intf->cost != RRT_COST(rr)) {
		/* Metric has changed and link is not down - rebuild router links */
		
		match++;
		intf->cost = RRT_COST(rr);
		set_rtr_sched(&ospf.backbone);
	    }
	}
    } VINTF_LIST_END(intf) ;
    
    /*
     * have to schedule backbone to rerun if there is a virtual link
     * if there was a match, building the rtr lsa will rerun the dijkstra
     */
    if (!match && BIT_TEST(area->area_flags, OSPF_AREAF_VIRTUAL_UP))
	ospf.backbone.spfsched |= SCHED_BIT(LS_RTR);
}


/*
 * Run Dijkstra on this area
 */
static void
spf __PF1(area, struct AREA *)
{
    struct AREA *sum_area = (IAmBorderRtr && ospf.backbone.ifUcnt) ? &ospf.backbone : area;

    area->spfcnt++;
    RTAB_REV++;

    /*
     * run spf on this area
     */
    spfinit(area);		/* clear spf tree */

    DB_INITQ(area->candidates);

    if (intra(area))
	return;			/* Out of memory */
    /*
     * update asbr and abr routing tables for this area
     */
    rtr_update(area);

    /*
     * Run sum on backbone if this rtr is area border router
     * else run on first (and only) area
     */
    if (netsum(sum_area, ALLSCHED, area, 0))
        return;                 /* Out of memory */
    if (asbrsum(sum_area, ALLSCHED, area, 0))
        return;                 /* Out of memory */

    /*
     * Run ASE on global exteral as LSAs
     */
    if (ase(area, ALLSCHED, 0))
	return;			/* Out o'memory */
    /*
     * Update network routing table
     */
    ntab_update(area, ALLSCHED);

    /*
     * Area Border routers must determine whether or not summary nets
     * for the area have changed due to intra-area route changes. 
     *  - If so, new advertisements may need to be injected into other
     *    area.
     *  - Additionally, if a network summary has been flushed in build_sum_net,
     *    the network summary portion of the SPF must be re-run to assure that:
     *     1) If it has become reachable, a previous route corresponding to a summary
     *        advertisement from a different ABR is deleted from the route table.
     *     2) If it has become unreachable, accessiblity via the backbone or other areas
     *        is exploited.
     *  - If a memory allocation failure occurs, simply return.
     * 
     */
    if (IAmBorderRtr) {
	switch (build_sum_net(area)) {
	case FLAG_NO_BUFS:
	    return;

	case FLAG_RERUN_NETSUM:
	    area->spfsched |= SUMNETSCHED;
	    break;
	    
	default:
	    break;
	}
    }
	    

    /*
     * cleanup no longer valid stub network lsdb entries
     */
    stub_cleanup(area);

    /*
     * check virtual links - bring up adjacencies if we have intra route
     */
    if (BIT_TEST(area->area_flags, OSPF_AREAF_TRANSIT)) {
	virtual_check(area);
    }
}


/*
 *  After the SPF is run, a timer is started to insure that
 *  the SPF is not run again in a certain time period.  When
 *  the timer fires we check to see if any areas require an SPF, if so
 *  schedule the foreground job.
 */
static void
ospf_spf_timer __PF2(tip, task_timer *,
		     interval, time_t)
{
    register struct AREA *area;
    
    AREA_LIST(area) {
	if (area->spfsched) {
	    ospf_spf_sched();
	    break;
	}
    } AREA_LIST_END(area) ;

    ospf.timer_spf = (task_timer *) 0;
}


/*
 * A call to run the correct level of the spf algorithm
 * - called by Rx routines and tq_lsa_lock
 */
void
ospf_spf_run __PF2(area, struct AREA *,
		   partial, int)
{
    struct AREA *a;

    trace_tf(ospf.trace_options,
	     TR_OSPF_SPF,
	     0,
	     ("OSPF SPF Area %A Scheduled: %s",
	      sockbuild_in(0, area->area_id),
	      trace_bits(ospf_sched_bits, (flag_t) area->spfsched)));

    /*
     * If we've lost the backbone, assure the network summary SPF is run for non-backbone areas.  
     */
    if (area == &ospf.backbone 
	&& !ospf.backbone.ifUcnt) {
	AREA_LIST(a) {
	    if (a != &ospf.backbone) {
		a->spfsched |= NETSCHED;
	    }
	} AREA_LIST_END(a) ;
    }

    /* Open the routing table */
    rt_open(ospf.task);

    /*
     * Check for a spf sched event having been run - if out of memory during
     * spf process may have scheduled
     */
    if (area->spfsched & INTSCHED) {
	area->spfsched = 0;
	spf(area);
	partial = 0;
    }
    if (area->spfsched) {
	int bad_run = FALSE;
	int from = 0;

	/*
	 * Only will run sum if area border rtr and area is backbone or not
	 * area border rtr
	 */
	if (area->spfsched & SUMSCHED) {
	    struct AREA *sum_area = (IAmBorderRtr && ospf.backbone.ifUcnt) ? &ospf.backbone : area;

	    /*
	     * Run either or both
	     */
	     if (!(area->spfsched & SCHED_BIT(LS_SUM_ASB))) {
		 area->spfsched = 0;
		 bad_run = netsum(sum_area, SUMNETSCHED, area, partial);
		 from = (ASESCHED | SUMNETSCHED);
	     } else if (!(area->spfsched & SCHED_BIT(LS_SUM_NET))) {
		 area->spfsched = 0;
		 bad_run = asbrsum(sum_area, SUMASBSCHED, area, partial);
		 from = (ASESCHED | SUMASBSCHED);
	     } else {
		 area->spfsched = 0;
		 bad_run = netsum(sum_area, SUMASESCHED, area, partial);
		 bad_run |= asbrsum(sum_area, SUMASESCHED, area, partial);
		 from = SUMASESCHED;
	     }
	     /* Turn partial off for ASE's */
	     partial = 0;
	} else if (area->spfsched & ASESCHED) {
	    from = ASESCHED;
	    area->spfsched = 0;
	}
	if (!bad_run && from && !BIT_TEST(area->area_flags, OSPF_AREAF_STUB)) {
	    bad_run = ase(area, from, partial);
	}

	/*
	 * Update the network routing table
	 */
	if (!bad_run && from) {
	    /*
	     * Partial deals with updating the routing table
	     */
	    if (!partial)
		ntab_update(area, from);
	}
    }
    /*
     * new sum routes may be on txq or new inter routes may have become
     * available, inject into areas
     */
    AREA_LIST(a) {
	if (a->txq != LLNULL) {
	    self_orig_area_flood(a, a->txq, LS_SUM_NET);
	    ospf_freeq((struct Q **) &a->txq, ospf_lsdblist_index);
	}
    } AREA_LIST_END(a) ;

    rt_close(ospf.task, (gw_entry *) 0, 0, NULL);
}


static void
ospf_spf_job __PF1(jp, task_job *)
{
    int ran = 0;
    int deleted;
    register struct AREA *area;
    
    trace_tf(ospf.trace_options,
	     TR_OSPF_SPF,
	     TRC_NL_BEFORE,
	     ("OSPF SPF Start"));

    AREA_LIST(area) {
	if (area->spfsched
	    && !BIT_TEST(area->lsalock, RTRSCHED)) {
	    ospf_spf_run(area, 0);
	    ran++;
	}
    } AREA_LIST_END(area) ;

    deleted = ospf_nh_collect();

    trace_tf(ospf.trace_options,
	     TR_OSPF_SPF,
	     0,
	     ("OSPF SPF - NH Collection - %u entries removed",
	      deleted));
    
    trace_tf(ospf.trace_options,
	     TR_OSPF_SPF,
	     TRC_NL_AFTER,
	     ("OSPF SPF End"));

    if (ran) {
	/* Start a timer to prevent SPF runs in the next 5 seconds */
	ospf.timer_spf = task_timer_create(ospf.task,	
					   "SPF",
					   TIMERF_DELETE,
					   (time_t) 0,
					   MinLSInterval,
					   ospf_spf_timer,
					   (void_t) 0);
    }

    task_job_delete(jp);
    ospf.spf_job = (task_job *) 0;
}


void
ospf_spf_sched __PF0(void)
{
    if (ospf.timer_spf || ospf.spf_job) {
	/* Too soon to run SPF, or SPF already scheduled */
	return;
    }

    ospf.spf_job = task_job_create(ospf.task,
				   TASK_JOB_PRIO_SPF,
				   "SPF",
				   ospf_spf_job,
				   (void_t) 0);
    
}

