#ident	"@(#)ospf_rt.c	1.3"
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

static block_t ospf_rtinfo_block_index;

static void
ospf_rt_dump  __PF2(fp, FILE *,
		    rt, rt_entry *)
{
    struct LSDB *db = ORT_V(rt);
    
    fprintf(fp,
	    "\t\t\tCost: %d\tArea: %A\tType: %s\tAdvRouter: %A\n",
	    ORT_COST(rt),
	    sockbuild_in(0, ORT_AREA(rt)->area_id),
	    trace_state(ospf_ls_type_bits, ORT_PTYPE(rt)),
	    sockbuild_in(0, ORT_ADVRTR(rt)));
    if (LS_TYPE(db) == LS_ASE) {
	fprintf(fp,
		"\t\t\tType: %d\t\tForward: %A\tTag: %A\n",
		ASE_TYPE1(db) ? 1 : 2,
		sockbuild_in(0, DB_ASE_FORWARD(db)),
		ospf_path_tag_dump(inet_autonomous_system, rt->rt_tag));
	if (ASE_TYPE2(db)) {
	    fprintf(fp,
		    "\t\t\tExternal Cost (Metric): %d\tInternal Cost (Metric2): %d\n",
		    rt->rt_metric,
		    rt->rt_metric2);
	}
    }
}


static void
ospf_rt_free __PF2(rt, rt_entry *,
		   rtd, void_t)
{
    OSPF_RT_INFO *rtinfo = (OSPF_RT_INFO *) rtd;

    ospf_nh_free_list(rtinfo->nh_cnt, rtinfo->nh_ndx);

    task_block_free(ospf_rtinfo_block_index, rtd);
}


void
ospf_route_update __PF3(rt, rt_entry *,
			area, struct AREA *,
			level, int)
{
    struct NET_RANGE *foundnr = NRNULL, *nr;
    struct LSDB *db = ORT_V(rt);
    int should_run_build_inter = FALSE;
    int rt_change_flag = FALSE;

    /* A few checks - some paranoid */
    if (!ORT_INFO(rt) || 
	!db ||
	DB_WHERE(db) == ON_ASE_LIST ||
	!(PTYPE_BIT(ORT_PTYPE(rt)) & level) ||
	((PTYPE_BIT(ORT_PTYPE(rt)) & PTYPE_INTRA) && area != ORT_AREA(rt))) {

	return;
    }

    /*
     * if internal to this area and this is a border rtr,
     * set cost - build_ls will figure out what to do
     */
    if (IAmBorderRtr) {
	if (PTYPE_BIT(ORT_PTYPE(rt)) & PTYPE_INTRA ||
	    ORT_CHANGE(rt) == E_WAS_INTRA_NOW_ASE || 
	    ORT_CHANGE(rt) == E_WAS_INTER_NOW_INTRA) {
	    /* Should only summarize if no virtual links */

	    RANGE_LIST(nr, area) {
	    	/* check for net to be summarized */
		if ((nr->mask & RT_DEST(rt)) == (nr->mask & nr->net)) {
		    foundnr = nr;
		    break;
		}
	    } RANGE_LIST_END(nr, area) ;
	    if (!foundnr) {
	    	should_run_build_inter = TRUE;
	    }
    	} else if (((PTYPE_BIT(ORT_PTYPE(rt)) & PTYPE_INTER) && 
		    area->area_id == OSPF_BACKBONE) ||
		   ORT_CHANGE(rt) == E_WAS_INTRA_NOW_INTER) {
	    should_run_build_inter = TRUE;
	}
    }

    /*
     * Check for invalid route:
     * - Revision != current revision &&
     * - Intra route && area that is currently running dijkstra == rt's area ||
     * - Inter or Ase route - correct level (inter or ase) is checked above
     */
    if ((ORT_REV(rt) != RTAB_REV) &&
	(((PTYPE_BIT(ORT_PTYPE(rt)) & PTYPE_INTRA) && area == ORT_AREA(rt)) ||
	 (PTYPE_BIT(ORT_PTYPE(rt)) & PTYPE_LEAVES))) {
	if (should_run_build_inter) {
	    /* delete inter-area route from other areas */
	    /* May be out of memory */
	    if (build_inter(db, area, E_DELETE)) {
		return;
	    }
	}

	/* remove route from routing table */
	/* free parent list */
	ospf_nh_free_list(db->lsdb_nhcnt, db->lsdb_nh);
	DB_DIRECT(db) = FALSE;
	db->lsdb_route = (rt_entry *) 0;

	ospf_rt_free(rt, rt->rt_data);
	rt->rt_data = (void_t) 0;
	rt_delete(rt);
	return;
    }

    /*
     * Set high cost for net range per post-RFC 1583 OSPF v2 draft.
     *
     * We use SUMLSInfinity as a magic cookie to indicate that the range's
     * cost hasn't been set.
     */
    if ((foundnr && ((foundnr->cost == SUMLSInfinity) || (ORT_COST(rt) > foundnr
->cost))) &&
        /* if ((foundnr && ORT_COST(rt) > foundnr->cost)) && */
        ORT_CHANGE(rt) != E_WAS_INTRA_NOW_ASE) {
        foundnr->cost = ORT_COST(rt);
    }

    /* Handle rtab changes */
    switch (ORT_CHANGE(rt)) {
    case E_NEXTHOP:
    case E_METRIC:
	rt_change_flag = TRUE;
	/* Fall through */

    case E_NEW:
	/* if border rtr and bb, add intra routes to all areas */
	if (should_run_build_inter) {
	    build_inter(db, area, E_NEW);
	}
	break;

    case E_WAS_INTER_NOW_INTRA:
	BIT_RESET(rt->rt_state, RTS_EXTERIOR);
	BIT_SET(rt->rt_state, RTS_INTERIOR);
	if (should_run_build_inter) {
	    build_inter(db, area, E_NEW);
	} else if (IAmBorderRtr && 
		   foundnr && 
		   (foundnr->net != DB_NETNUM(db) ||
		    foundnr->mask != DB_MASK(db))) {
	    /* Have to delete more specific inter-area route */
	    build_inter(db, area, E_DELETE);
	}
	rt_change_flag = TRUE;
	break;

    case E_WAS_INTRA_NOW_ASE:
    case E_WAS_INTER_NOW_ASE:
	if (should_run_build_inter) {
	    build_inter(db, area, E_DELETE);
	}
	break;

    case E_WAS_ASE:
	if (ORT_AREA(rt) == area && should_run_build_inter) {
	    build_inter(db, area, E_WAS_ASE);
	}
	break;

    case E_WAS_INTRA_NOW_INTER:
	if (should_run_build_inter) {
	    build_inter(db, area, E_DELETE);
	}
	/* Fall through */

    case E_ASE_METRIC:
    case E_ASE_TYPE:
    case E_ASE_TAG:
	/* External changes require a new route because of policy */
	ospf_build_route(area, ORT_V(rt), rt, E_NEW);
	rt_delete(rt);
	return;

    case E_UNCHANGE:
	/* Nothing to do */
	return;
	
    default:
	assert(FALSE);
    }

    if (rt_change_flag) {
	register int i = ORT_NH_CNT(rt);
	sockaddr_un *routers[RT_N_MULTIPATH];
	pref_t preference = rt->rt_preference;

	if ((DB_DIRECT(db) == FALSE) != (preference > 0)) {
	    /* We can't change the flags on a route */

	    ospf_build_route(area, ORT_V(rt), rt, E_NEW);
	    rt_delete(rt);
	    return;
	}
	
    	while (i--) {
	    routers[i] = sockbuild_in(0, ORT_NH(rt, i));
    	}
    	(void) rt_change(rt,
			 db->lsdb_dist,
			 (metric_t) -1,
			 rt->rt_tag,
			 preference,
			 rt->rt_preference2,
			 (int) ORT_NH_CNT(rt), routers);
	rt_refresh(rt);
    }

    ORT_CHANGE(rt) = E_UNCHANGE;
    return;
}


/*
 * ntab_update()
 *	- called by routine who knows which levels have been run before
 * 	- process all routes from this level up
 *      - notify changed or new net routes to real routing table
 *	- notify real routing table of deleted net routes
 *        routes are deletable if:
 *		intra && area == r->area && r->spfcnt != RTAB_REV
 *		inter && r->spfcnt != RTAB_REV
 *		ase && r->spfcnt != RTAB_REV
 */
void
ntab_update __PF2(a, struct AREA *,
		  lvl, int)
{
    struct NET_RANGE *nr1;
    rt_entry *rt;

    /* MODIFIED 5/1/92 */
    if (IAmBorderRtr && (lvl & PTYPE_INTRA))		/* set up for net_sum */
	RANGE_LIST(nr1, a) {
	    nr1->cost = SUMLSInfinity;
	} RANGE_LIST_END(nr1, a) ;

    RTQ_LIST(&ospf.gwp->gw_rtq, rt) {
	if (!BIT_TEST(rt->rt_state, RTS_REJECT)) {
	    ospf_route_update(rt, a, lvl);
	}
    } RTQ_LIST_END(&ospf.gwp->gw_rtq, rt) ;

    RTQ_LIST(&ospf.gwp_ase->gw_rtq, rt) {
	if (!BIT_TEST(rt->rt_state, RTS_REJECT)) {
	    ospf_route_update(rt, a, lvl);
	}
    } RTQ_LIST_END(&ospf.gwp_ase->gw_rtq, rt) ;
}


/*
 * Allocate route and install it in the routing table
 */
int
ospf_build_route __PF4(a, struct AREA *,
		       v, struct LSDB *,
		       old_rt, rt_entry *,
		       change, int)
{
    u_int i;
    rt_entry *rt;
    OSPF_RT_INFO *rtinfo;
    rt_parms rtparms;

    bzero((caddr_t) &rtparms, sizeof (rtparms));

    rtparms.rtp_dest = sockbuild_in(0, DB_NETNUM(v));
    rtparms.rtp_dest_mask = inet_mask_locate(DB_MASK(v));
    rtparms.rtp_n_gw = v->lsdb_nhcnt;
    /* Build the next-hop list */
    i = v->lsdb_nhcnt;
    assert(i && i <= RT_N_MULTIPATH);
    while (i--) {
	register u_int32 addr = v->lsdb_nh[i]->nh_addr;

	assert(addr);

	rtparms.rtp_routers[i] = sockbuild_in(0, addr);
    }
    rtparms.rtp_gwp = ospf.gwp;
    rtparms.rtp_metric = v->lsdb_dist;
    rtparms.rtp_metric2 = (metric_t) -1;	/* For MIB */
    rtparms.rtp_state = RTS_INTERIOR;
    rtparms.rtp_preference = ospf.preference;

    switch (LS_TYPE(v)) {
    case LS_SUM_NET:
	/* Summary Networks (Inter-Area routes) */
	break;

    case LS_STUB:
    case LS_NET:
	/* Intra-Area routes */
	break;
	
    case LS_ASE:
	/* AS External routes */

	rtparms.rtp_preference = ospf.preference_ase;
	rtparms.rtp_gwp = ospf.gwp_ase;
	if (ospf.asbr) {
	    /* For debugging purposes, we do not allow import */
	    /* filtering unless we are an AS border router */
	    
	    (void) import(rtparms.rtp_dest,
			  rtparms.rtp_dest_mask,
			  ospf.import_list,
			  (adv_entry *) 0,
			  (adv_entry *) 0,
			  &rtparms.rtp_preference,
			  (if_addr *) 0,
			  (void_t) v);
	}

	if (ASE_TYPE2(v)) {
	    BIT_SET(rtparms.rtp_state, RTS_EXTERIOR);
	    if (v->lsdb_dist < ASELSInfinity) {
		rtparms.rtp_metric = BIG_METRIC(v);
		rtparms.rtp_metric2 = v->lsdb_dist;
	    }
	}
	break;
    }

    if (DB_DIRECT(v)) {
	/* We need this route, but gated shouldn't see or use it */

	rtparms.rtp_preference = -rtparms.rtp_preference;
	BIT_SET(rtparms.rtp_state, RTS_NOADVISE|RTS_NOTINSTALL);
    }

    if (old_rt && old_rt->rt_data) {
	/* Use old data area */

	rtparms.rtp_rtd = old_rt->rt_data;
	old_rt->rt_data = (void_t) 0;
    } else {
	/* Allocate a new data area */

	rtparms.rtp_rtd = task_block_alloc(ospf_rtinfo_block_index);
    }

    /* Build the OSPF specific extension */
    rtinfo = (OSPF_RT_INFO *) rtparms.rtp_rtd;

    rtinfo->change = change;
    rtinfo->v = v;
    rtinfo->revision = RTAB_REV;
    rtinfo->area = a;
    rtinfo->cost = v->lsdb_dist;
    rtinfo->ptype = LS_TYPE(v);
    rtinfo->advrtr = ADV_RTR(v);
    ospf_nh_set(rtinfo->nh_cnt, rtinfo->nh_ndx,
		v->lsdb_nhcnt, v->lsdb_nh);
    rtinfo->etype = FALSE;
    if (LS_TYPE(v) == LS_ASE) {
	if (ASE_TYPE2(v)) {
	    rtinfo->etype = TRUE;	/* XXX - INTERIOR/EXTERIOR */
	}
	rtparms.rtp_tag = ntohl(LS_ASE_TAG(v));
    }

    rt = rt_add(&rtparms);
    v->lsdb_route = rt;
    if (!rt) {
	ospf_rt_free((rt_entry *) 0, rtparms.rtp_rtd);
	rtparms.rtp_rtd = (void_t) 0;

	/* XXX - Couldn't add the route, but if I return FALSE the SPF will not finish */
    }

    return TRUE;
}



/*
 * Bind this route to this vertex
 */
void
rvbind __PF3(rt, rt_entry *,
	     v, struct LSDB *,
	     a, struct AREA *)
{
#define TYPE_CHANGE(old, new)	((old << 8) | new)
    int change = TYPE_CHANGE(ORT_PTYPE(rt), LS_TYPE(v));
    time_t timestamp = (time_t) 0;
    int nh_is_diff = FALSE;
    u_int i;

    if (ORT_NH_CNT(rt) != v->lsdb_nhcnt) {
	nh_is_diff = TRUE;
    } else {
    	for (i = 0; i < ORT_NH_CNT(rt); i++)
    	    if (ORT_NH_NDX(rt, i) != v->lsdb_nh[i]) {
	    	nh_is_diff = TRUE;
		break;
	    }
    }

    /* break link from LSDB to routing table */
    if (ORT_V(rt)) {
	timestamp = ORT_V(rt)->lsdb_time;
	ORT_V(rt)->lsdb_route = (rt_entry *) 0;
    }

    /* Lets assume no change */
    ORT_CHANGE(rt) = E_UNCHANGE;

    /* Figure out what (if anything) has changed */
    switch (change) {
    case TYPE_CHANGE(LS_STUB, LS_NET):
    case TYPE_CHANGE(LS_NET, LS_STUB):
    case TYPE_CHANGE(LS_NET, LS_NET):
    case TYPE_CHANGE(LS_STUB, LS_STUB):
    case TYPE_CHANGE(LS_SUM_NET, LS_SUM_NET):
	if (nh_is_diff || (v != ORT_V(rt) && timestamp == v->lsdb_time)) {
	    ORT_CHANGE(rt) = E_NEXTHOP;
	} else if (v->lsdb_dist != ORT_COST(rt)) {
	    ORT_CHANGE(rt) = E_METRIC;
	}
	break;
	
    /*
     * The next two cases are a bit weird since they imply that
     * there is an inter-area net with the same net number as an intra-area net 
     */
    case TYPE_CHANGE(LS_STUB, LS_SUM_NET):
    case TYPE_CHANGE(LS_NET, LS_SUM_NET):
	ORT_CHANGE(rt) = E_WAS_INTRA_NOW_INTER;
	break;

    case TYPE_CHANGE(LS_SUM_NET, LS_STUB):
    case TYPE_CHANGE(LS_SUM_NET, LS_NET):
	ORT_CHANGE(rt) = E_WAS_INTER_NOW_INTRA;
	break;
	
    case TYPE_CHANGE(LS_STUB, LS_ASE):
    case TYPE_CHANGE(LS_NET, LS_ASE):
	ospf_build_route(a, v, rt, E_WAS_INTRA_NOW_ASE);
	rt_delete(rt);
	return;

    case TYPE_CHANGE(LS_SUM_NET, LS_ASE):
	ospf_build_route(a, v, rt, E_WAS_INTER_NOW_ASE);
	rt_delete(rt);
	return;

    case TYPE_CHANGE(LS_ASE, LS_STUB):
    case TYPE_CHANGE(LS_ASE, LS_NET):
    case TYPE_CHANGE(LS_ASE, LS_SUM_NET):
	ospf_build_route(a, v, rt, E_WAS_ASE);
	rt_delete(rt);
	return;

    case TYPE_CHANGE(LS_ASE, LS_ASE):
	if (nh_is_diff) {
	    /* Next hop change */
	    ORT_CHANGE(rt) = E_NEXTHOP;
	} else if (ASE_TYPE2(v) != ORT_ETYPE(rt)) {
	    ORT_CHANGE(rt) = E_ASE_TYPE;
	} else if (ntohl(LS_ASE_TAG(v)) != rt->rt_tag) {
	    ORT_CHANGE(rt) = E_ASE_TAG;
	} else if (v->lsdb_dist != ORT_COST(rt)
		   || (ASE_TYPE2(v)
		       && BIG_METRIC(v) != rt->rt_metric)) {
	    /* Metric change */
	    ORT_CHANGE(rt) = E_ASE_METRIC;
        }
	break;
    }

    ORT_REV(rt) = RTAB_REV;

    /* Link routing table entry to LSDB */
    ORT_V(rt) = v;
    v->lsdb_route = rt;

    /* Make sure info is up to date */
    ORT_AREA(rt) = a;
    ORT_COST(rt) = v->lsdb_dist;
    ORT_PTYPE(rt) = LS_TYPE(v);
    ORT_ADVRTR(rt) = ADV_RTR(v);
    ospf_nh_set(ORT_NH_CNT(rt), ORT_INFO(rt)->nh_ndx,
		v->lsdb_nhcnt, v->lsdb_nh);
    ORT_ETYPE(rt) = FALSE;
    if (LS_TYPE(v) == LS_ASE) {
	if (ASE_TYPE2(v)) {
	    ORT_ETYPE(rt) = TRUE;	/* XXX - INTERIOR/EXTERIOR */
	}
	rt->rt_tag = ntohl(LS_ASE_TAG(v));
    }
#undef	TYPE_CHANGE
}


/**/
/*
 *  Exportation of routes into OSPF.
 */

/*
 *  For exporting gated routes to OSPF.
 */
static block_t ospf_export_block_index;

/*
 * Macros to dig change and LSDB pointers out of the tsi field, and stuff
 * them back in the same way.
 */
#define	OSPF_TSI_NONE	0
#define	OSPF_TSI_LSDB	1
#define	OSPF_TSI_CE	2

#define	OSPF_TSI_DATA_SIZE	(sizeof(void_t) + sizeof(byte))

struct ospf_tsi_data {
    void_t ptr;
    byte type;		/* XXX assume no hole */
};

#define	OSPF_TSI_GET(rth, rtbit, cept, lsdbpt) \
    do { \
	struct ospf_tsi_data Xtsi; \
	rttsi_get((rth), (rtbit), (byte *)&Xtsi); \
	switch (Xtsi.type) { \
	case OSPF_TSI_LSDB: \
	    (cept) = NULL; \
	    (lsdbpt) = (struct LSDB *)Xtsi.ptr; \
	    break; \
	case OSPF_TSI_CE: \
	    (cept) = (ospf_export_entry *)Xtsi.ptr; \
	    (lsdbpt) = ((ospf_export_entry *)Xtsi.ptr)->db; \
	    break; \
	default: \
	    (cept) = NULL; \
	    (lsdbpt) = NULL; \
	    break; \
	} \
    } while (0)

#define	OSPF_TSI_PUT(rth, rtbit, cept, lsdbpt) \
    do { \
	struct ospf_tsi_data Xtsi; \
	if ((Xtsi.ptr = (void_t)(cept)) != NULL) { \
	    Xtsi.type = OSPF_TSI_CE; \
	    ((ospf_export_entry *)Xtsi.ptr)->db = (lsdbpt); \
	} else if ((Xtsi.ptr = (void_t)(lsdbpt)) != NULL) { \
	    Xtsi.type = OSPF_TSI_LSDB; \
	} else { \
	    Xtsi.type = OSPF_TSI_NONE; \
	} \
	rttsi_set((rth), (rtbit), (byte *)&Xtsi); \
    } while (0)

/*
 * Macros to enqueue/dequeue export entries.  The problem with delaying
 * ASE exports is that, if you are a transit AS and a delete is long
 * delayed, you can potentially be black-holing a neighbour's traffic
 * for as long as it takes to get the delete to them during a time when
 * they could be using alternate routes.  Because of this it is slightly
 * better to get deletes out faster.  Also, during changes, BGP may
 * be telling lies to your neighbours, so it is desireable to minimize
 * this as well.
 *
 * These macros maintain the export queue such that deletes are queued
 * before changes, which are queued before adds.  This is probably slightly
 * less efficient from OSPF's point-of-view, but is somewhat more satisfying
 * from the point-of-view of the transient.
 */
#define	OSPF_EXPORT_Q_DELETE(ospf, export_ent) \
    do { \
	register struct OSPF *Xospf = (ospf); \
	register ospf_export_entry *Xce = (export_ent); \
	if (Xospf->export_queue_change == Xospf->export_queue_delete) { \
	    Xospf->export_queue_change = Xce; \
	} \
	INSQUE(Xce, Xospf->export_queue_delete); \
	Xospf->export_queue_delete = Xce; \
	Xospf->export_queue_size++; \
    } while (0)

#define	OSPF_EXPORT_Q_CHANGE(ospf, export_ent) \
    do { \
	register struct OSPF *Xospf = (ospf); \
	register ospf_export_entry *Xce = (export_ent); \
	INSQUE(Xce, Xospf->export_queue_change); \
	Xospf->export_queue_change = Xce; \
	Xospf->export_queue_size++; \
    } while (0)

#define	OSPF_EXPORT_Q_ADD(ospf, export_ent) \
    do { \
	register struct OSPF *Xospf = (ospf); \
	INSQUE(export_ent, Xospf->export_queue.back); \
	Xospf->export_queue_size++; \
    } while (0)

#define	OSPF_EXPORT_DEQUEUE(ospf, export_ent) \
    do { \
	register struct OSPF *Xospf = (ospf); \
	register ospf_export_entry *Xce = (export_ent); \
	if (Xospf->export_queue_delete == Xce) { \
	    Xospf->export_queue_delete = Xce->back; \
	} \
	if (Xospf->export_queue_change == Xce) { \
	    Xospf->export_queue_change = Xce->back; \
	} \
	REMQUE(Xce); \
	Xospf->export_queue_size--; \
    } while (0)


static void
ospf_tsi_dump __PF4(fp, FILE *,
		    rth, rt_head *,
		    data, void_t,
		    pfx, const char *)
{
    task *tp = (task *)data;
    ospf_export_entry *ce;
    struct LSDB *db;

    OSPF_TSI_GET(rth, tp->task_rtbit, ce, db);
    if (ce) {
	(void) fprintf(fp,
		       "%s %s export entry\n",
		       pfx,
		       tp->task_name);
    } else if (db) {
	(void) fprintf(fp,
		       "%s%s LSDB seq %8x\n",
		       pfx,
		       tp->task_name,
		       ntohl(LS_SEQ(db)));
    }
}


static void
ospf_ase_export __PF2(tip, task_timer *,
		      interval, time_t)
{
    struct ospf_lsdb_list *txq = LLNULL;
    int i = ospf.export_limit;
    register ospf_export_entry *ce;
    task *tp;
    u_int rtbit;

    if (ospf.export_queue.forw == &ospf.export_queue) {
	/* No work to do, reset timer and quit */
	task_timer_reset(tip);
	return;
    }

    tp = tip->task_timer_task;
    rtbit = tp->task_rtbit;
    rt_open(tp);

    while ((ce = ospf.export_queue.forw) != &ospf.export_queue &&
	   i--) {
        struct LSDB *db = ce->db;
	struct ospf_lsdb_list *ll;

	/*
	 * If we're in here we're going to do something to this
	 * route.  Remove any old db pointer from all retransmission
	 * lists if there are any.
	 */
	if (ce->old_rt) {
	    assert(db && DB_WHERE(db) == ON_ASE_LIST);

	    if (db->lsdb_retrans) {
		rem_db_retrans(db);
	    }
	} else {
	    assert(db == NULL);
	}

	if (ce->old_rt && !ce->new_rt) {
	    /* Here because we are deleting one we originated */

	    /* Timestamp new origination */
	    db->lsdb_time = time_sec;

	    /* Calculate checksum and update checksumsum */
	    LS_AGE(db) = 0;
	    ospf.db_chksumsum -= LS_CKS(db);
	    ospf_checksum_sum(DB_ASE(db), ASE_LA_HDR_SIZE, ospf.db_chksumsum);
	    LS_AGE(db) = MaxAge;

	    /* Log this new ASE */
	    if (TRACE_TF(ospf.trace_options, TR_OSPF_LSA_BLD)) {
		ospf_trace_build((struct AREA *) 0, (struct AREA *) 0, db->lsdb_adv, FALSE);
	    }

	    /* Remove from ASE list */
	    DB_REMQUE(db);

	    /* Add to free list */
	    DB_WHERE(db) = ON_ASE_INFINITY;
	    DB_FREEME(db) = TRUE;
	    DB_ADDQUE(ospf.db_free_list, db);

	    /* Add to retransmit queue */
	    ll = (struct ospf_lsdb_list *) task_block_alloc(ospf_lsdblist_index);
	    ll->lsdb = db;
	    EN_Q(txq, ll);

	    /* Set TSI to null and reset bit on old route */
	    db->lsdb_route = (rt_entry *) 0;
	    rttsi_reset(ce->old_rt->rt_head, rtbit);
	    (void) rtbit_reset(ce->old_rt, rtbit);
	} else {
	    struct ASE_LA_HDR *as_hdr;
	    u_int32 age = 0;
	    int newseq = FALSE;
	    const char *action;

	    /* Either a route change or a new route */
	    if (ce->old_rt) {
		/* Make a change to the existing ASE */

		action = "Change";
	    
		as_hdr = DB_ASE(db);
		if (ce->new_rt != ce->old_rt) {
	    	    (void) rtbit_reset(ce->old_rt, rtbit);
		}

		/* Force it to the end of the queue */
		DB_REMQUE(db);
		DB_ADDQUE(ospf.my_ase_list, db);
		
		/* Indicate we need to bump the sequence number */
		newseq++;
	    } else {
		u_int32 lsa_id;
                int foundlsa;

		lsa_id = LS_ID_NORMALIZE_RT(ce->new_rt);
		/* New route.  Create an LS db entry for it */
		foundlsa = AddLSA(&db,
				      ospf.area.area_forw,
				      lsa_id,
				      MY_ID,
				      LS_ASE);

		if (foundlsa) {
		    /* Previous one was in the process of being flushed */

		    action = "Existing";

		    as_hdr = DB_ASE(db);

                    trace_log_tp(tp,
                                 0,
                                 LOG_WARNING,
                                 ("Conflict between LSDB %A Mask %A and route %A Mask %A - Export to OSPF ASE Bypassed.",
                                 sockbuild_in(0, LS_ID(db)),
                                 inet_mask_locate(DB_MASK(db)),
                                 ce->new_rt->rt_dest,
                                 ce->new_rt->rt_dest_mask));
                                 rttsi_reset(ce->new_rt->rt_head, rtbit);
                    (void) rtbit_reset(ce->new_rt, rtbit);
                    goto free_change_entry;


		    DB_REMQUE(db);
		    DB_FREEME(db) = FALSE;

		    if (db->lsdb_retrans) {
			rem_db_retrans(db);
		    }

		    /* Indicate we need to bump the sequence number */
		    newseq++;
		} else {
		    action = "Add";

		    DBADV_ALLOC(db, ASE_LA_HDR_SIZE);
		    as_hdr = DB_ASE(db);
		    as_hdr->ls_hdr.ls_seq = FirstSeq;
		}

		/* Add to the ASE list */
		DB_ADDQUE(ospf.my_ase_list, db);
		DB_WHERE(db) = ON_ASE_LIST;

		/* Init ASE fields */
		as_hdr->ls_hdr.ls_type = LS_ASE;
		as_hdr->ls_hdr.adv_rtr = MY_ID;
		as_hdr->ls_hdr.length = htons(ASE_LA_HDR_SIZE);
		as_hdr->ls_hdr.ls_id = lsa_id;
		as_hdr->net_mask = RT_MASK(ce->new_rt);
	    }

	    if (newseq) {
		/* We need to bump the sequence number */

		if (!db->lsdb_seq_max &&
		    as_hdr->ls_hdr.ls_seq == MaxSeqNum) {
		    /* Maximum sequence number - Flush from everyone's db */

		    age = MaxAge;
		    as_hdr->ls_hdr.ls_seq = MaxSeqNum;
		    db->lsdb_seq_max = TRUE;
		} else {
		    /* Increment sequence number */
		
		    as_hdr->ls_hdr.ls_seq = NEXTNSEQ(as_hdr->ls_hdr.ls_seq);
		}
	    }

	    /* Fill in the fields we calculated in ospf_policy */
	    as_hdr->tos0.ExtRtTag = ce->tag;
	    as_hdr->tos0.tos_metric = ce->metric;
	    as_hdr->tos0.ForwardAddr = ce->forward.s_addr;

	    /* Recalculate checksum and update checksumsum */
	    if (as_hdr->ls_hdr.ls_chksum) {
		ospf.db_chksumsum -= as_hdr->ls_hdr.ls_chksum;
	    }
	    as_hdr->ls_hdr.ls_age = 0;
	    ospf_checksum_sum(as_hdr, ASE_LA_HDR_SIZE, ospf.db_chksumsum);
	    as_hdr->ls_hdr.ls_age = age;

	    /* Add to transmit queue */
	    db->lsdb_time = time_sec;
	    ll = (struct ospf_lsdb_list *) task_block_alloc(ospf_lsdblist_index);
	    ll->lsdb = db;
	    EN_Q(txq, ll);

	    ospf.orig_new_lsa++;
	    ospf.orig_lsa_cnt[LS_ASE]++;

	    /* Log this change */
	    /* Log this new ASE */
	    if (TRACE_TF(ospf.trace_options, TR_OSPF_LSA_BLD)) {
		ospf_trace_build((struct AREA *) 0, (struct AREA *) 0, db->lsdb_adv, FALSE);
	    }

	    /* Store the db pointer in the TSI field */
	    db->lsdb_route = ce->new_rt;
	    OSPF_TSI_PUT(ce->new_rt->rt_head,
			 rtbit,
			 (ospf_export_entry *)0,
			 db);

	    if (TRACE_TP(tp, TR_POLICY)) {
		rt_entry *rt = ce->old_rt ? ce->old_rt : ce->new_rt;
		
		trace_tp(tp,
			 TR_POLICY,
			 0,
			 ("ospf_ase_export: export %s %A/%A",
			  action,
			  rt->rt_dest,
			  rt->rt_dest_mask));
	    }
	}

free_change_entry:
	/* Free the change entry */
	OSPF_EXPORT_DEQUEUE(&ospf, ce);
	task_block_free(ospf_export_block_index, (void_t) ce);
    }

    if (i != ospf.export_limit && !interval) {
	task_timer_set(tip, ospf.export_interval, 0);
    }

    if (txq) {
	/* Flood all the changes I've made */

	struct AREA *a;

	AREA_LIST(a) {
	    if (!BIT_TEST(a->area_flags, OSPF_AREAF_STUB)) {
		self_orig_area_flood(a, txq, LS_ASE);
	    }
	} AREA_LIST_END(a) ;

	ospf_freeq((struct Q **)&txq, ospf_lsdblist_index);
    }

    rt_close(tp, (gw_entry *) 0, ospf.export_limit - i, NULL);

    trace_tp(tp,
	     TR_ROUTE,
	     0,
	     ("ospf_ase_export: processed %d ASEs %d remaining",
	      ospf.export_limit - i,
	      ospf.export_queue_size));
}


/*
 * ospf_policy - flash/newpolicy update routine for
 */
static void
ospf_policy __PF2(tp, task *,
		  change_list, rt_list *)
{
    int changes = 0;
    rt_head *rth;
    u_int rtbit = tp->task_rtbit;

    rt_open(tp);

    RT_LIST(rth, change_list, rt_head) {
	register rt_entry *new_rt;
	register rt_entry *old_rt;
	register ospf_export_entry *ce;
	struct LSDB *db;
	adv_results result;

	if (socktype(rth->rth_dest) != AF_INET) {
	    continue;
	}

	/* Read the tsi field to see what the state of this route is */
	OSPF_TSI_GET(rth, rtbit, ce, db);
#ifdef	notdef
	TRACE_TF(ospf.trace_options,
		 TR_ALL,
		 ("ce = %x, db = %x",
		  (u_int)ce,
		  (u_int)db));
#endif	/* notdef */
	new_rt = rth->rth_active;

	/* If this is one which has already changed, use old_rt from there */
	if (ce) {
	    old_rt = ce->old_rt;
	    if (ce->new_rt &&
		ce->new_rt != new_rt &&
		ce->new_rt != old_rt) {
		/* Reset routing bit on previous new route */
		(void) rtbit_reset(ce->new_rt, rtbit);
	    }

	    /*
	     * Remove it from the linked list.  It'll get put back on
	     * at the end later.
	     */
	    OSPF_EXPORT_DEQUEUE(&ospf, ce);
	} else {
	    /*
	     * See if we were announcing another route.  The only route
	     * this could be is the last_active route.  If there is a bit
	     * set on the last_active route the db pointer had better
	     * be non-zero, otherwise it had better be zero.
	     */
	    if (rth->rth_last_active &&
		rtbit_isset(rth->rth_last_active, rtbit)) {
		assert(db);
		old_rt = rth->rth_last_active;
	    } else {
		assert(db == NULL);
		old_rt = (rt_entry *) 0;
	    }
	}

	/* See if we can announce the new route */
	if (new_rt) {
	    if (BIT_TEST(new_rt->rt_state, RTS_NOADVISE|RTS_PENDING)) {
		new_rt = (rt_entry *) 0;
	    } else {
		switch (new_rt->rt_gwp->gw_proto) {
		case RTPROTO_OSPF:
		case RTPROTO_OSPF_ASE:
		    /* Our own route - ignore */
		    new_rt = (rt_entry *) 0;
		    break;

		case RTPROTO_DIRECT:
		    /* Ignore pseudo interface routes and interfaces running OSPF */
		    if (BIT_TEST(new_rt->rt_state, RTS_NOTINSTALL) ||
			IF_INTF(RT_IFAP(new_rt)) ||
			BIT_TEST(RT_IFAP(new_rt)->ifa_state, IFS_LOOPBACK)) {
			new_rt = (rt_entry *) 0;
		    }
		    break;

		default:
#ifdef	notdef
		    if (new_rt->rt_ifap
			&& BIT_TEST(RT_IFAP(new_rt)->ifa_state, IFS_LOOPBACK)) {
			/* Route is via the loopback interface */
			new_rt = (rt_entry *) 0;
		    }
#endif	/* notdef */
		    break;
		}
	    }

	    if (new_rt) {
		/* See if policy will allow us to export this route */
		result.res_metric = ospf.export_metric;
		result.res_metric2 = ospf.export_tag;
		result.res_flag = ospf.export_type;
		if (!export(new_rt,
			    (proto_t) 0,
			    ospf.export_list,
			    (adv_entry *) 0,
			    (adv_entry *) 0,
			    &result)) {
		    new_rt = (rt_entry *) 0;
		}
	    }
	}

	/*
	 * So far, so good.  If we haven't got either a new route
	 * or an old route at this point, continue.
	 */
	if (!new_rt && !old_rt) {
	    if (ce) {
		task_block_free(ospf_export_block_index, (void_t) ce);
		OSPF_TSI_PUT(rth,
			     rtbit,
			     (ospf_export_entry *) 0,
			     (struct LSDB *) 0);
	    }
	    continue;
	}

	/*
	 * If we haven't got an export entry we'll probably need one
	 */
	if (!ce) {
	    ce = (ospf_export_entry *) task_block_alloc(ospf_export_block_index);
	    ce->old_rt = old_rt;
	}

	/*
	 * If we haven't got a new route just queue the delete.
	 * If we've got a new route, record the export data for comparison.
	 * If we've also got an old route, check whether the old route is
	 * the same.  If so we can forget about this altogether.
	 */
	if (!new_rt) {
	    OSPF_EXPORT_Q_DELETE(&ospf, ce);
	} else {
	    /*
	     * Set the bit on the new route if it isn't there already
	     */
	    if (new_rt != old_rt &&
		new_rt != ce->new_rt) {
		rtbit_set(new_rt, rtbit);
	    }

	    ce->metric = htonl((BIT_TEST(result.res_flag, OSPF_EXPORT_TYPE2) ? ASE_bit_E : 0) | result.res_metric);
	    ce->tag =
#ifdef	PROTO_ASPATHS
		BIT_TEST(result.res_metric2, PATH_OSPF_TAG_TRUSTED) ?
		    aspath_tag_ospf(inet_autonomous_system, new_rt, result.res_metric2) :
#endif	/* PROTO_ASPATHS */
			htonl(result.res_metric2);

	    /* Calculate forwarding address */
	    if (RT_IFAP(new_rt)
		&& IF_INTF(RT_IFAP(new_rt))
		&& IF_INTF(RT_IFAP(new_rt))->type != POINT_TO_POINT) {
		/*
		 * Only set forwarding address if OSPF is running on
		 * said interface.  Should not point to a router that
		 * runs OSPF, but that is too difficult to calculate
		 */
		ce->forward.s_addr = RT_NEXTHOP(new_rt);
	    } else {
		ce->forward.s_addr = 0;
	    }

	    /*
	     * Compare what we've got now to the old route.  If they're
	     * the same, forget about this.  In any case, add this to
	     * the appropriate part of the list
	     */
	    if (old_rt) {
	        struct ASE_LA_HDR *as_hdr = DB_ASE(db);

		if (ce->forward.s_addr == as_hdr->tos0.ForwardAddr &&
		    ce->metric == as_hdr->tos0.tos_metric &&
		    ce->tag == as_hdr->tos0.ExtRtTag) {
		    if (old_rt != new_rt) {
			rtbit_reset(old_rt, rtbit);
		    }
		    task_block_free(ospf_export_block_index, (void_t) ce);
		    OSPF_TSI_PUT(rth, rtbit, (ospf_export_entry *) 0, db);
		    continue;
		}
		OSPF_EXPORT_Q_CHANGE(&ospf, ce);
	    } else {
		OSPF_EXPORT_Q_ADD(&ospf, ce);
	    }
	}

	/*
	 * Okdk.  By the time we're here we've got an exportable new
	 * route and/or an old route we're not using any more.  Queue it
	 * at the end of the list.
	 */
	ce->new_rt = new_rt;
	OSPF_TSI_PUT(rth, rtbit, ce, db);
	changes++;
    } RT_LIST_END(rth, change_list, rt_head) ;

    rt_close(tp, (gw_entry *) 0, 0, NULL);

    if (changes &&
	!BIT_TEST(task_state, TASKS_TEST) &&
	!ospf.timer_ase->task_timer_interval) {
	/* We added something to the queue and the timer is not running, start running the queue */

	ospf_ase_export(ospf.timer_ase, (time_t) 0);
    }
}


/**/


/*
 *  Refresh ASE LSAs
*/
/*ARGSUSED*/
static void
ospf_ase_refresh __PF2(tip, task_timer *,
			interval, time_t)
{
    struct ospf_lsdb_list *txq = LLNULL;
    struct LSDB_HEAD *hp = &ospf.ase[ospf.ase_refresh_bucket];
    register struct LSDB *db;
    time_t min_age = time_sec - MinLSInterval;

    if (++ospf.ase_refresh_bucket == HTBLSIZE) {
	ospf.ase_refresh_bucket = 0;
    }

    LSDB_LIST(hp, db) {
	u_int16 age;
	struct ospf_lsdb_list *ll;

	if (db->lsdb_time > min_age) {
	    /* Just refreshed */
	    continue;
	}

	if (DB_WHERE(db) != ON_ASE_LIST) {
	    /* Not one I originated */
	    continue;
	}

	
	/* This LSA needs to be refreshed */
	    
	if (db->lsdb_retrans != NLNULL) {
	    /* Remove from retrahs queue */
	    
	    rem_db_retrans(db);
	}

	LS_AGE(db) = 0;
#ifdef NOTYETDEF
	/* MODIFIED 1/22/92 */
	if (ospf.db_overflow) {
	    age = MaxAge;
	} else
#endif
	    if (LS_SEQ(db) == MaxSeqNum
		&& db->lsdb_seq_max == FALSE
		&& ospf.nbrEcnt) {
		/* Not noted - flush from everyone's LSDB */
		db->lsdb_seq_max = TRUE;
		age = MaxAge;
	    } else {
		db->lsdb_seq_max = FALSE;
		LS_SEQ(db) = NEXTNSEQ(LS_SEQ(db));
		age = 0;
	    }
	ospf.db_chksumsum -= LS_CKS(db);
	ospf_checksum_sum(DB_ASE(db), ASE_LA_HDR_SIZE, ospf.db_chksumsum);
	ospf.orig_new_lsa++;
	ospf.orig_lsa_cnt[LS_ASE]++;
	db->lsdb_time = time_sec;
	LS_AGE(db) = age;

	/* Add to transmit queue */
	db->lsdb_time = time_sec;
	ll = (struct ospf_lsdb_list *) task_block_alloc(ospf_lsdblist_index);
	ll->lsdb = db;
	EN_Q(txq, ll);

	/* Make sure we are at the end of the queue */
	DB_REMQUE(db);
	DB_ADDQUE(ospf.my_ase_list, db);
	
	if (TRACE_TF(ospf.trace_options, TR_OSPF_LSA_BLD)) {
	    ospf_trace_build((struct AREA *) 0, (struct AREA *) 0, db->lsdb_adv, FALSE);
	}
    } LSDB_LIST_END(hp, db) ;

    if (txq) {
	/* Flood all the changes I've made */

	struct AREA *a;

	AREA_LIST(a) {
	    if (!BIT_TEST(a->area_flags, OSPF_AREAF_STUB)) {
		self_orig_area_flood(a, txq, LS_ASE);
	    }
	} AREA_LIST_END(a) ;

	ospf_freeq((struct Q **)&txq, ospf_lsdblist_index);
    }
}


/**/

/*
 *	Flash update when aux protocol is enabled
 */
static void
ospf_aux_flash __PF2(tp, task *,
		     change_list, rt_list *)
{
    register aux_proto *auxp = tp->task_aux;
    
    ospf_policy(tp, change_list);

    task_aux_flash(auxp, change_list);
}


/*
 *	New policy when aux protocol is enabled
 */
static void
ospf_aux_newpolicy __PF2(tp, task *,
			 change_list, rt_list *)
{
    register aux_proto *auxp = tp->task_aux;
    
    ospf_policy(tp, change_list);

    task_aux_newpolicy(auxp, change_list);
}


/*
 *	Initiate Aux connection
 */
static void
ospf_aux_register __PF2(tp, task *,
			auxp, register aux_proto *)
{
    if (auxp) {
	/* Register */

	if (ospf.asbr) {
	    /* Set intercept routines */

	    task_set_flash(tp, ospf_aux_flash);
	    task_set_newpolicy(tp, ospf_aux_newpolicy);
	}
    
	task_aux_initiate(auxp, tp->task_rtbit);
    } else {
	/* Un-Register */

	if (tp->task_flash_method == ospf_aux_flash) {
	    /* Reset intercept routines */

	    task_set_flash(tp, ospf_policy);
	    task_set_newpolicy(tp, ospf_policy);
	}
    }
}


/**/


void
ospf_policy_init __PF1(tp, task *)
{
    tp->task_aux_register = ospf_aux_register;
    tp->task_aux_proto = RTPROTO_OSPF_ASE;

    if (ospf.asbr) {
	task_set_flash(tp, ospf_policy);
	task_set_newpolicy(tp, ospf_policy);
	tp->task_rtbit = rtbit_alloc(tp,
				     FALSE,
				     OSPF_TSI_DATA_SIZE,
				     (void_t) tp,
				     ospf_tsi_dump);

	/* Create ASE timers */
	ospf.timer_ase = task_timer_create(ospf.task,
					   "AseQueue",
					   0,
					   0,
					   0,
					   ospf_ase_export,
					   (void_t) 0);

	(void) task_timer_create(ospf.task,	
				 "LSAGenAse",
				 0,
				 LSRefreshTime / HTBLSIZE,
				 (time_t) 0,
				 ospf_ase_refresh,
				 (void_t) 0);
    }	

    /* Initiate protocol interaction */
    task_aux_lookup(tp);

    if (!ospf_export_block_index) {
	ospf_export_block_index = task_block_init(sizeof (ospf_export_entry),
						  "ospf_export_entry");
	ospf_rtinfo_block_index = task_block_init(sizeof (OSPF_RT_INFO),
						  "OSPF_RT_INFO");
    }

    ospf.gwp->gw_rtd_dump = ospf.gwp_ase->gw_rtd_dump = ospf_rt_dump;
    ospf.gwp->gw_rtd_free = ospf.gwp_ase->gw_rtd_free = ospf_rt_free;
}


void
ospf_policy_cleanup __PF1(tp, task *)
{
    ospf.asbr = FALSE;

    if (tp && tp->task_rtbit) {
	
	/* Terminate Aux connection */
	task_aux_terminate(tp);
	tp->task_aux_register = 0;
	tp->task_aux_proto = (proto_t) 0;

	rt_open(tp);
	
	/* Clean up ASEs we have originated */
        {
	    struct LSDB *db;

	    while (!DB_EMPTYQ(ospf.my_ase_list)
		   && (db = DB_FIRSTQ(ospf.my_ase_list))) {
		/* Reset TSI, release route and reset route pointer */
		rttsi_reset(db->lsdb_route->rt_head, tp->task_rtbit);
		(void) rtbit_reset(db->lsdb_route, tp->task_rtbit);
		db->lsdb_route = (rt_entry *) 0;

		/* Remove from our list and add to the free list */
		DB_REMQUE(db);
		DB_FREEME(db) = TRUE;
		DB_ADDQUE(ospf.db_free_list, db);
	    }
	}
	
	/* Delete any export queue entries */
	if (ospf.export_queue.forw != &ospf.export_queue) {
	    register ospf_export_entry *ce;

	    while ((ce = ospf.export_queue.forw) != &ospf.export_queue) {
		if (ce->new_rt && ce->new_rt != ce->old_rt) {
		    if (!ce->old_rt) {
			rttsi_reset(ce->new_rt->rt_head, tp->task_rtbit);
		    }
		    (void) rtbit_reset(ce->new_rt, tp->task_rtbit);
		}
		REMQUE(ce);
		task_block_free(ospf_export_block_index, (void_t) ce);
	    }
	}

	/* Free our bits */
	rtbit_free(tp, tp->task_rtbit);

	rt_close(tp, (gw_entry *) 0, 0, NULL);
    }
}


#ifdef	notdef
void
ospf_discard_delete __PF1(nr, struct NET_RANGE *)
{
    if (nr->rt) {
	rt_delete(nr->rt);
    }
    nr->rt = (rt_entry *) 0;
}

/* jgsXXX this routine is never called -- two calls are ifdef'd out */
int
ospf_discard_add __PF1(nr, struct NET_RANGE *)
{
    rt_parms rtp;

    bzero((caddr_t) &rtp, sizeof (rtp));

    rtp.rtp_dest = sockbuild_in(0, nr->net);
    rtp.rtp_dest_mask = inet_mask_locate(nr->mask);
    rtp.rtp_n_gw = 1;
    rtp.rtp_router = inet_addr_loopback;
    rtp.rtp_gwp = ospf.gwp;
    rtp.rtp_metric = SUMLSInfinity;
    rtp.rtp_state = RTS_REJECT|RTS_INTERIOR;
    rtp.rtp_preference = ospf.preference;

    nr->rt = rt_add(&rtp);

    return FLAG_NO_PROBLEM;
}
#endif	/* notdef */
