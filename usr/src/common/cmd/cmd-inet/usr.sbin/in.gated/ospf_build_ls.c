#ident	"@(#)ospf_build_ls.c	1.3"
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
 * add db to list to be flooded by calling routine
 */
static void
txadd __PF5(db, struct LSDB *,
	    txq, struct ospf_lsdb_list **,
	    len, size_t,
	    age, u_int16,
	    area, struct AREA *)
{
    struct ospf_lsdb_list *ll = LLNULL;

    /*
     * if there is a retrans list, free it
     */
    if (db->lsdb_retrans != NLNULL)
	rem_db_retrans(db);
    LS_AGE(db) = 0;
    ll = (struct ospf_lsdb_list *) task_block_alloc(ospf_lsdblist_index);
    if (ll == LLNULL)
	return;
    ll->lsdb = db;
    ll->flood = FLOOD;
    ospf_checksum_sum(DB_RTR(db), len, area->db_chksumsum);
    LS_AGE(db) = age;
    /* 
     * put lsa on queue to be flooded 
     */
    EN_Q((*txq), ll);

    /* 
     * Increase count of self originated LSAs for MIBness 
     */
    ospf.orig_new_lsa++;
    ospf.orig_lsa_cnt[LS_TYPE(db)]++;
}


/*
 * build ls updates - three cases where these are called
 *		   1) tq has fired
 *		   2) nbr change
 *		   3) received self orig with earlier seq number
 */
int
build_rtr_lsa __PF3(a, struct AREA *,
		    txq, struct ospf_lsdb_list **,
		    force_flood, int)	/* if true always flood */
{
    int foundlsa;			/* true if lsa was found when adding */
    int spfsched = 0;
    int addnet = 0;
    union LSA_PTR adv;
    struct RTR_LA_PIECES *linkp;
    struct OSPF_HOSTS *host;
    struct INTF *intf;
    struct LSDB *db;
    block_t r_index = (block_t) 0;
    size_t len;
    u_int32 new_seq;

    if (RTR_LSA_LOCK(a)) {
	/* schedule one to send */
	set_rtr_sched(a);
	return FLAG_NO_PROBLEM;
    }

    foundlsa = AddLSA(&db, a, MY_ID, MY_ID, LS_RTR);


    /* Check seq number for max */
    if (foundlsa) {
	if ((LS_SEQ(db) == MaxSeqNum)) {
	    if (db->lsdb_seq_max == TRUE && db->lsdb_retrans != NLNULL) {
		/*
		 * Will reoriginate when the acks come home
		 */
		return FLAG_NO_PROBLEM;
		/* If no nbrs >= exchange there will be no acks */
	    } else if (db->lsdb_seq_max == FALSE && a->nbrEcnt) {
		/*
		 * Not already noted - flush from everyone's LSDB
		 */
		db->lsdb_seq_max = TRUE;
		txadd(db, txq, (size_t) ntohs(LS_LEN(db)), MaxAge, a);
		rtr_lsa_lockout(a);
		db->lsdb_time = time_sec;
		return FLAG_NO_PROBLEM;
	    }
	    /* have receive acks */
	}
	db->lsdb_seq_max = FALSE;
	new_seq = NEXTNSEQ(LS_SEQ(db));
    } else {
        /*
         * Enough mem for DB?
         */
        if (db == LSDBNULL) {
	    set_rtr_sched(a);
	    return FLAG_NO_PROBLEM;
        }
	new_seq = FirstSeq;
	spfsched = RTRSCHED;
    }

    /*
     * Allocate enough for all links
     */
    len = MY_RTR_ADV_SIZE(a);
    ADV_ALLOC(adv, r_index, len);
    if (!adv.rtr) {
	set_rtr_sched(a);
	return FLAG_NO_PROBLEM;
    }

    adv.rtr->ls_hdr.ls_type = LS_RTR;
    adv.rtr->ls_hdr.ls_id = MY_ID;
    adv.rtr->ls_hdr.adv_rtr = MY_ID;
    if (IAmBorderRtr)
	adv.rtr->E_B = bit_B;
    if (ospf.asbr)
	adv.rtr->E_B |= bit_E;
    GHTONS(adv.rtr->E_B);
    len = RTR_LA_HDR_SIZE;
    linkp = &adv.rtr->link;

    INTF_LIST(intf, a) {
	if (intf->state == IDOWN)
	    continue;
	addnet = 0;

	switch (intf->state) {
	case IPOINT_TO_POINT:
	    if (intf->nbr.state == NFULL) {
		adv.rtr->lnk_cnt++;
		/* len += RTR_LA_METRIC_SIZE - add metric lengths */
		len += RTR_LA_PIECES_SIZE;
		/* add for TOS here */
		linkp->con_type = RTR_IF_TYPE_RTR;
		linkp->lnk_id = NBR_ID(&intf->nbr);
		linkp->lnk_data = INTF_LCLADDR(intf);
		linkp->metric_cnt = 0;
		linkp->tos0_metric = htons(intf->cost);
		/* add metric lengths here if necessary */
		linkp = (struct RTR_LA_PIECES *) ((long) linkp +
						 RTR_LA_PIECES_SIZE +
						 ((linkp->metric_cnt) * RTR_LA_METRIC_SIZE));

		/* Add a stub link to the remote address */
		adv.rtr->lnk_cnt++;
		len += RTR_LA_PIECES_SIZE;
		/* add for TOS here */
		linkp->con_type = RTR_IF_TYPE_HOST;
		linkp->lnk_data = HOST_NET_MASK;
		linkp->lnk_id = NBR_ADDR(&intf->nbr);
		linkp->metric_cnt = 0;
		linkp->tos0_metric = htons(intf->cost);
		linkp = (struct RTR_LA_PIECES *) ((long) linkp +
						 RTR_LA_PIECES_SIZE +
						 ((linkp->metric_cnt) * RTR_LA_METRIC_SIZE));
	    }
	    break;
	    
	case ILOOPBACK:
	    /*
	     * Need ip address for this part...
	     */
	    if (intf->ifap->ifa_addr) {
		adv.rtr->lnk_cnt++;
		len += RTR_LA_PIECES_SIZE;
		/* add for TOS here */
		linkp->con_type = RTR_IF_TYPE_HOST;
		linkp->lnk_data = HOST_NET_MASK;
		linkp->lnk_id = INTF_ADDR(intf);
		linkp->metric_cnt = 0;
		linkp->tos0_metric = 0;
		linkp = (struct RTR_LA_PIECES *) ((long) linkp +
						 RTR_LA_PIECES_SIZE +
						 ((linkp->metric_cnt) * RTR_LA_METRIC_SIZE));
	    }
	    break;

	case IDr:			/* have DR selected cases */
	case IBACKUP:
	case IDrOTHER:
	    /*
	     * If there are full nbrs and we are adjacent or are DR
	     * it is OK to add a network link
	     */
	    if (intf->nbrFcnt) {
		if ((intf->state == IDr) ||
		    ((intf->dr) && (intf->dr->state == NFULL)))
		    addnet++;
	    }

	    if (addnet) {
		adv.rtr->lnk_cnt++;
		len += RTR_LA_PIECES_SIZE;
		/* add for TOS here */
		linkp->con_type = RTR_IF_TYPE_TRANS_NET;
		linkp->lnk_id = NBR_ADDR(intf->dr);
		linkp->lnk_data = INTF_ADDR(intf);
		linkp->metric_cnt = 0;
		linkp->tos0_metric = htons(intf->cost);
		linkp = (struct RTR_LA_PIECES *) ((long) linkp +
						 RTR_LA_PIECES_SIZE +
						 ((linkp->metric_cnt) * RTR_LA_METRIC_SIZE));
		break;
	    }
	    /*
	     * else fall through
	     */
	case IWAITING:
	    /*
	     * still determining DR
	     */
	    adv.rtr->lnk_cnt++;
	    len += RTR_LA_PIECES_SIZE;
	    /* add for TOS here */
	    linkp->con_type = RTR_IF_TYPE_STUB_NET;
	    linkp->lnk_data = INTF_MASK(intf);
	    linkp->lnk_id = INTF_NET(intf);
	    linkp->metric_cnt = 0;
	    linkp->tos0_metric = htons(intf->cost);
	    linkp = (struct RTR_LA_PIECES *) ((long) linkp +
					     RTR_LA_PIECES_SIZE +
					     ((linkp->metric_cnt) * RTR_LA_METRIC_SIZE));
	    break;
	}
    } INTF_LIST_END(intf, a) ;
    
    /*
     * add virtual links
     */
    if (a->area_id == OSPF_BACKBONE && ospf.vcnt) {
	struct INTF *vintf;

	VINTF_LIST(vintf) {
	    if ((vintf->state == IPOINT_TO_POINT) &&
		(vintf->nbr.state == NFULL)) {
		adv.rtr->lnk_cnt++;
		len += RTR_LA_PIECES_SIZE;
		/* add for TOS here */
		linkp->con_type = RTR_IF_TYPE_VIRTUAL;
		linkp->lnk_id = NBR_ID(&vintf->nbr);
		linkp->lnk_data = INTF_LCLADDR(vintf);
		linkp->metric_cnt = 0;
		linkp->tos0_metric = htons(vintf->cost);
		linkp = (struct RTR_LA_PIECES *) ((long) linkp +
						 RTR_LA_PIECES_SIZE +
			      ((linkp->metric_cnt) * RTR_LA_METRIC_SIZE));
	    }
	} VINTF_LIST_END(vintf) ;
    }
    /*
     * add host links
     */
    for (host = a->hosts.ptr[NEXT]; host != HOSTSNULL; host = host->ptr[NEXT]) {
	if_addr *ifap = if_withdstaddr(sockbuild_in(0, host->host_if_addr));

	if (!ifap) {
	  ifap = if_withdstaddr(sockbuild_in(0,host->host_if_addr));
	  
	  if(!ifap) {
	    /* No interface for this host */
	    continue;
	  }
	}
	switch (ifap->ifa_state & (IFS_POINTOPOINT|IFS_LOOPBACK)) {
	    struct NH_BLOCK *np;
	    
	case IFS_POINTOPOINT:
	    if ((np = (struct NH_BLOCK *) ifap->ifa_ospf_nh)
		&& np->nh_addr == host->host_if_addr) {
		/* Matches on remote address */

		break;
	    }
	    /* Fall through */

	case IFS_LOOPBACK:
	default:
	    if (!(np = (struct NH_BLOCK *) ifap->ifa_ospf_nh_lcl)
		|| np->nh_addr != host->host_if_addr) {
		/* No next hop block or no match */

		continue;
	    }
	    break;
	}
	
	adv.rtr->lnk_cnt++;
	len += RTR_LA_PIECES_SIZE;
	/* add for TOS here */
	linkp->con_type = RTR_IF_TYPE_HOST;
	linkp->lnk_data = HOST_NET_MASK;
	linkp->lnk_id = host->host_if_addr;
	linkp->metric_cnt = 0;
	linkp->tos0_metric = htons((s_int16) host->host_cost);
	linkp = (struct RTR_LA_PIECES *) ((long) linkp +
					  RTR_LA_PIECES_SIZE +
					  ((linkp->metric_cnt) * RTR_LA_METRIC_SIZE));
    }

    adv.rtr->ls_hdr.length = htons((u_int16) len);

    if (foundlsa) {
	/*
	 * compare to see if it's the same, if not rerun spf
	 */
	if ((adv.rtr->ls_hdr.length != LS_LEN(db)) ||
	    (adv.rtr->E_B != DB_RTR(db)->E_B) ||
	    (LS_AGE(db) == MaxAge) ||
	    (adv.rtr->lnk_cnt != ntohs(DB_RTR(db)->lnk_cnt)) ||
	    RTR_LINK_CMP(adv.rtr, DB_RTR(db),
			 (size_t) (ntohs(adv.rtr->ls_hdr.length) - RTR_LA_HDR_SIZE)))
	    spfsched = RTRSCHED;
	if (DB_FREEME(db) == TRUE) {
	    DB_FREEME(db) = FALSE;
	    DB_REMQUE(db);		/* remove from db_free_list */
	}
    }

    /*
     * if found or changed, replace with new one and flood
     */
    if (force_flood ||
	spfsched ||
	new_seq == FirstSeq ||
	ospf.nbrEcnt != ospf.nbrFcnt) {

	if (foundlsa)
	    a->db_chksumsum -= LS_CKS(db);

	if (DB_RTR(db))
	    DBADV_FREE(db);
	DB_RTR(db) = adv.rtr;
	db->lsdb_index = r_index;
	LS_SEQ(db) = new_seq;

	GHTONS(adv.rtr->lnk_cnt);

	if (TRACE_TF(ospf.trace_options, TR_OSPF_LSA_BLD)) {
	    ospf_trace_build(a, a, db->lsdb_adv, FALSE);
	}

	/*
	 * lock out new instansiation for MinLSInterval
	 */
	rtr_lsa_lockout(a);
	db->lsdb_time = time_sec;
	/*
	 * put lsa on queue to be flooded
	 */
	txadd(db, txq, len, 0, a);
    } else {
	ADV_FREE(adv, r_index);
    }

    /*
     * let calling routine know to rerun spf or not
     */
    return spfsched;
}

/*
 * build net link state advertisement - this rtr is DR
 */
int
build_net_lsa __PF3(intf, struct INTF *,
		    txq, struct ospf_lsdb_list **,
		    force_flood, int)	/* if true always flood */
{
    int foundlsa;			/* true if lsa was found when adding */
    int spfsched = 0;
    struct AREA *a = intf->area;
    union LSA_PTR nh;
    block_t nh_index = (block_t) 0;
    struct NET_LA_PIECES *att_rtr;
    struct NBR *nbr;
    struct LSDB *db;
    u_int32 new_seq;
    size_t len;

    if (intf->state != IDr)
	return FALSE;

    if (NET_LSA_LOCK(intf)) {
	/* schedule one to send */
	set_net_sched(intf);
	return FLAG_NO_PROBLEM;
    }
    /*
     * Are we adjacent to any neighbors?
     */
    if ((!intf->nbrFcnt))
	return FALSE;

    foundlsa = AddLSA(&db, a, INTF_ADDR(intf), MY_ID, LS_NET);
    /*
     * Check seq number for max
     */
    if (foundlsa) {
	if (DB_FREEME(db)) {
	    DB_FREEME(db) = FALSE;
	    DB_REMQUE(db);
	}
	if ((LS_SEQ(db) == MaxSeqNum)) {
	    if (db->lsdb_seq_max == TRUE && db->lsdb_retrans != NLNULL) {
		/* Will re originate when the acks come home */
		return FALSE;
		/* If no nbrs >= exchange there will be no acks */
	    } else if (db->lsdb_seq_max == FALSE && a->nbrEcnt) {
		/* Not already noted - flush from everyone's LSDB */
		db->lsdb_seq_max = TRUE;
		txadd(db, txq, (size_t) ntohs(LS_LEN(db)), MaxAge, a);
		net_lsa_lockout(intf);
		db->lsdb_time = time_sec;
		return FALSE;
	    }
	}
	/*
         * have received acks
         */
	db->lsdb_seq_max = FALSE;
	new_seq = NEXTNSEQ(LS_SEQ(db));
    } else {
	if (!db) {
	    set_net_sched(intf);
	    return FLAG_NO_PROBLEM;
	}
	new_seq = FirstSeq;
	spfsched = NETSCHED;
    }


    /*
     * Allocate enough for all links
     */
    len = MY_NET_ADV_SIZE(intf);
    ADV_ALLOC(nh, nh_index, len);
    if (!nh.net) {
    	set_net_sched(intf);
	if (!foundlsa) {
	    db_free(db, LS_NET);
	}
	return FLAG_NO_PROBLEM;
    }
    len = NET_LA_HDR_SIZE + NET_LA_PIECES_SIZE;	/* one for this rtr */
    nh.net->ls_hdr.ls_type = LS_NET;
    nh.net->ls_hdr.ls_id = INTF_ADDR(intf);
    nh.net->ls_hdr.adv_rtr = MY_ID;
    nh.net->net_mask = INTF_MASK(intf);
    att_rtr = &nh.net->att_rtr;
    /*
     * set the first one to this rtr
     */
    att_rtr->lnk_id = MY_ID;
    att_rtr++;
    for (nbr = intf->nbr.next; nbr != NBRNULL; nbr = nbr->next) {
	if (nbr->state == NFULL) {
	    att_rtr->lnk_id = NBR_ID(nbr);
	    len += NET_LA_PIECES_SIZE;
	    att_rtr++;
	}
    }
    nh.net->ls_hdr.length = htons((u_int16) len);
    if (foundlsa) {
	/*
  	 * compare to see if it's the same, if not rerun spf
	 */
	if ((nh.net->ls_hdr.length != LS_LEN(db)) ||
	    (LS_AGE(db) == MaxAge) ||
	    NET_ATTRTR_CMP(nh.net, DB_NET(db), (len - NET_LA_HDR_SIZE)) ||
	    nh.net->net_mask != DB_MASK(db))
	    spfsched = NETSCHED;
	if (DB_FREEME(db) == TRUE) {
	    DB_FREEME(db) = FALSE;
	    DB_REMQUE(db);		/* remove from db_free_list */
	}
    }
    /*
     * if !found, not the same or force flood, replace with new one
     */
    if (force_flood ||
	spfsched ||
	new_seq == FirstSeq ||
	ospf.nbrEcnt != ospf.nbrFcnt) {
	if (foundlsa)
	    a->db_chksumsum -= LS_CKS(db);
	if (DB_NET(db))
	    DBADV_FREE(db);
	DB_NET(db) = nh.net;
	db->lsdb_index = nh_index;
	LS_SEQ(db) = new_seq;
	/*
	 * lock out new instansiation for MinLSInterval
	 */
	net_lsa_lockout(intf);
	db->lsdb_time = time_sec;
	txadd(db, txq, len, 0, a);

	if (TRACE_TF(ospf.trace_options, TR_OSPF_LSA_BLD)) {
	    ospf_trace_build(a, a, db->lsdb_adv, FALSE);
	}
    } else {
	ADV_FREE(nh, nh_index);
    }

    /* A done deal */
    BIT_RESET(intf->flags, OSPF_INTFF_BUILDNET);

    /* rerun spf? */
    return spfsched;
}


/*
 * Update seq number
 * - if current seq number == MaxSeqNum set to max age and add to queue
 */
static void
sum_next_seq __PF4(db, struct LSDB *,
		   a, struct AREA *,
		   metric, u_int32,
		   sec, time_t)
{
    /* MODIFIED 5/1/92 */
    u_int16 age = (metric == ntohl(SUMLSInfinity)) ? MaxAge : 0;

    DB_SUM(db)->tos0.tos_metric = metric;
    if (LS_SEQ(db) == MaxSeqNum) {
	if (db->lsdb_seq_max == TRUE && db->lsdb_retrans != NLNULL)
	    return;
	if ((db->lsdb_seq_max == FALSE) && (a->nbrEcnt)) {
	    /* Not noted - flush from everyone's LSDB */
	    db->lsdb_seq_max = TRUE;
	    age = MaxAge;
	} else {
	    db->lsdb_seq_max = FALSE;
	    LS_SEQ(db) = NEXTNSEQ(LS_SEQ(db));
	}
    } else {
	LS_SEQ(db) = NEXTNSEQ(LS_SEQ(db));
    }
    db->lsdb_time = sec;
    a->db_chksumsum -= LS_CKS(db);
    txadd(db, &a->txq, (size_t) SUM_LA_HDR_SIZE, age, a);

}

/*
 * Build network summary link-state advertisements
 *    called by spf - just build the ones that have changed
 *    - Loop through all area and store in dst's lsdb where dst != orig area
 *    - Put on dst area's->sumnetlst
 */
int
build_sum_net __PF1(area, struct AREA *)
{
    int foundlsa;			/* true if lsa was found when adding */
    struct AREA *dst;			/* injecting from a - into area dst  */
    struct LSDB *db;
    struct SUM_LA_HDR *s;
    struct NET_RANGE *nr;
    int return_flag = FLAG_NO_PROBLEM;	/* Return flag to indicate no problem,
					   out of memory, or need to re-run net
					   summary.			     */

    RANGE_LIST(nr, area) {
	/* Assure the net range should be advertised outside the area. */
	if (nr->status == DoNotAdvertise) {
	    continue;
	}
	/*
	 * 	Network Range is Unreachable from originating Area - flush 
	 *	advertisement from other areas.
	 */
	if (nr->cost == SUMLSInfinity) {
	    AREA_LIST(dst) {		/* Loop through Areas.			*/  
		if (area == dst)
		    continue;
		db = FindLSA(dst, nr->net, MY_ID, LS_SUM_NET);
		if (db == LSDBNULL)
		    continue;		/* Net Range was previously unreachable. */
		if (DB_WHERE(db) != ON_SUM_INFINITY &&
			 db->lsdb_net_range) {
		    /* has become unreachable */
		    DB_WHERE(db) = ON_SUM_INFINITY;
	    	    DB_REMQUE(db);
	    	    DB_FREEME(db) = TRUE;
	    	    DB_ADDQUE(ospf.db_free_list, db);
		    return_flag = FLAG_RERUN_NETSUM;
#ifdef	notdef
		    ospf_discard_delete(nr);
#endif	/* notdef */
		    /* add to txq */
		    sum_next_seq(db, dst, htonl(SUMLSInfinity), time_sec);
		    if (TRACE_TF(ospf.trace_options, TR_OSPF_LSA_BLD)) {
			trace_only_tf(ospf.trace_options,
				      0,
				      ("build_sum_net: Unreachable Summary Net Range %A flushed from Area %A",
				       sockbuild_in(0, nr->net),
				       sockbuild_in(0, dst->area_id)));
			ospf_trace_build(area, dst, db->lsdb_adv, FALSE);
		    }
		}			/* End flush unreachable Network Range. */
	    } AREA_LIST_END(dst) ;	/* End of Area list.			*/
	} else {			/* Network Range is now reachable.	*/
	    /* 
	     * Network Range is now reachable - generate LSAs which will 
	     * supercede any existing LSAs.
	     */
	    AREA_LIST(dst) {		/* Loop through area list.		*/
		if (area == dst) { 	/* Flush Net Range LSA from origin area.*/
		    /* 
		     * If it exists, Flush the LSA from the origin area.
		     */
		    db = FindLSA(dst, nr->net, MY_ID, LS_SUM_NET);
		    if ((db) && (DB_WHERE(db) != ON_SUM_INFINITY)) {
			DB_WHERE(db) = ON_SUM_INFINITY;
			DB_REMQUE(db);
			DB_FREEME(db) = TRUE;
			/*
			 * Indicate the LSDB is now associated with a Network Range and 
			 * remove any previous association with a route.
			 */ 
			db->lsdb_net_range = TRUE;
			db->lsdb_route = (rt_entry *) 0;
			db->lsdb_border = LSDBNULL;
			DB_ADDQUE(ospf.db_free_list, db);
			return_flag = FLAG_RERUN_NETSUM;
#ifdef	notdef
			ospf_discard_delete(nr);
#endif	/* notdef */
			sum_next_seq(db, dst, htonl(SUMLSInfinity), time_sec);
			trace_tf(ospf.trace_options,
				 TR_OSPF_LSA_BLD,
				 0,
				 ("build_sum_net: Summary Net Range %A flushed from origin Area %A",
				  sockbuild_in(0, nr->net),
				  sockbuild_in(0, dst->area_id)));
		    }			/* End LSA exists in origin area.	*/
		} else {		/* Not originating area.		*/
		    foundlsa = AddLSA(&db, dst, nr->net, MY_ID, LS_SUM_NET);
		    if (!foundlsa) {		/* Net Range now accessible.    */
			if (!db)		/* Out of memory?		*/
			    return FLAG_NO_BUFS;
			DBADV_ALLOC(db, SUM_LA_HDR_SIZE);
			if (!(s = DB_SUM(db))			/* Out of memory?		*/
#ifdef	notdef
			    || ospf_discard_add(nr) == FLAG_NO_BUFS
#endif	/* notdef */
			    ) {
			    db_free(db, LS_SUM_NET);
			    return FLAG_NO_BUFS;
			}			/* End out of memory.		*/
			/*
			 * Build a new Network Summary LSA for the Net Range to 
			 *  be flooded into the area.
			 */
			s->ls_hdr.ls_type = LS_SUM_NET;
			s->ls_hdr.ls_id = nr->net;
			s->ls_hdr.adv_rtr = MY_ID;
			s->ls_hdr.ls_seq = FirstSeq;
			s->ls_hdr.length = htons(SUM_LA_HDR_SIZE);
			s->net_mask = nr->mask;
			s->tos0.tos_metric = htonl(nr->cost);
			/* 
			 * add to this areas net sum list 
			 */
			DB_WHERE(db) = ON_SUMNET_LIST;
			DB_ADDQUE(dst->sumnetlst, db);
			db->lsdb_time = time_sec;
			/* add to area's transmission queue for flooding.	*/
			txadd(db, &dst->txq, SUM_LA_HDR_SIZE, 0, dst);
			trace_tf(ospf.trace_options,
				 TR_OSPF_LSA_BLD,
				 0,
				 ("build_sum_net: New Summary Net Range %A generated for Area %A",
				  sockbuild_in(0, nr->net),
				  sockbuild_in(0, dst->area_id)));
		    } else {		/* LSA found - Previously existed	*/
			/*
			 * Net Range is now accessible - regenerate.
			 */
			if (DB_WHERE(db) == ON_SUM_INFINITY) { 
#ifdef	notdef
			    if (ospf_discard_add(nr) == FLAG_NO_BUFS)
				return FLAG_NO_BUFS;
#endif	/* notdef */
			    DB_MASK(db) = nr->mask;
			    sum_next_seq(db, dst, htonl(nr->cost), time_sec);
			    if (DB_FREEME(db) == TRUE) {
				DB_FREEME(db) = FALSE;
				DB_REMQUE(db);	/* remove from db_free_list 	*/
			    }
			    /* 
			     * add to this area's net sum list 
			     */
			    DB_WHERE(db) = ON_SUMNET_LIST;
			    DB_ADDQUE(dst->sumnetlst, db);
			    trace_tf(ospf.trace_options,
				     TR_OSPF_LSA_BLD,
				     0,
				     ("build_sum_net: Now-reachable Summary Net Range %A re-generated for Area %A",
				      sockbuild_in(0, nr->net),
				      sockbuild_in(0, dst->area_id)));
			} else {	/* Previously on area summary list.	*/
			    /* 
			     * Had LSDB on area network summary list - if the cost
			     * or net mask has changed - regenerate the LSDB. 
			     */
			    if (DB_SUM(db)->tos0.tos_metric != htonl(nr->cost) ||
				DB_MASK(db) != nr->mask) {
				DB_MASK(db) = nr->mask;
				sum_next_seq(db, dst, htonl(nr->cost), time_sec);
				trace_tf(ospf.trace_options,
					 TR_OSPF_LSA_BLD,
					 0,
					 ("build_sum_net: Summary Net Range %A re-generated for Area %A with cost %d mask %A",
					  sockbuild_in(0, nr->net),
					  sockbuild_in(0, dst->area_id),
					  nr->cost,
					  sockbuild_in(0, nr->mask)));
			    }			/* End regenerate LSDB.		*/
			}		/* End LSDB previously on Area summary
					   network list.			*/
			/*
			 * Indicate the LSDB is now associated with a Network Range and 
			 * remove any previous association with a route.
			 */ 
			db->lsdb_net_range = TRUE;
			db->lsdb_route = (rt_entry *) 0;
			db->lsdb_border = LSDBNULL;
		    }			/* End Summary LSA found in Area summary
					   network list.			*/
		    if (TRACE_TF(ospf.trace_options, TR_OSPF_LSA_BLD)) {
			ospf_trace_build(area, dst, db->lsdb_adv, FALSE);
		    }
		}			/* End reachable Network Range within 
					   non-originating area.		*/
	    } AREA_LIST_END(dst) ;	/* End area list.			*/
	}				/* End reachable Net Range.		*/
    } RANGE_LIST_END(nr, area) ;
    return return_flag;
}


/*
 * build_sum_asb: Build Summary AS (Autonomous System) Border Router Advertisements
 *    
 *	This function will build AS Border Router Summary Advertisments and inject 
 *      them into non-backbone areas which do not have a local route to the AS border 
 *	router. It is called both from asbsum (the SPF phase for AS Border Router
 *	Advertisements) and from rtr_update (an SPF catcher function to update OSPF
 *	router route tables). 
 *
 *
 */
int
build_sum_asb __PF3(area, struct AREA *,	/* Only used for logging. 	*/
		    r, struct OSPF_ROUTE *,	/* OSPF Route pointer. 		*/
		    from_area, struct AREA *)	/* Area running SPF.		*/
{
    int foundlsa;			/* LSA found in current area. 		*/
    struct AREA *dst;			/* Used to step through areas.		*/
    struct LSDB *db;			/* LSDB entry pointer for new entries.  */
    struct SUM_LA_HDR *s;		/* LSA Header pointer for new LSAs.	*/
    struct OSPF_ROUTE *rr;		/* Existing OSPF Route to AS Border
					   router.				*/
    /*
     * Loop through accessible areas determining whether an AS Border Router 
     * Summary is required.
     */
    AREA_LIST(dst) {
	int lsa_generated = FALSE;		/* Indicates whether or not an LSA has
						   been generated.			*/
	/*
         * Don't re-flood into the backbone since this function is only called
	 * for non-self-originated AS Border Router LSAs. 
         */

	if ((RRT_PTYPE(r) == LS_SUM_ASB) && (dst->area_id == OSPF_BACKBONE))
	    continue;
	/*
	 * Determine if there is currently a valid intra-area route to the AS Border
	 * router for the current area.
	 */
	rr = rtr_findroute( dst,
			    RRT_DEST(r),
			    DTYPE_ASBR,
			    PTYPE_INTRA );
	/*
	 * If there is a currently valid intra-area route and:
	 *  - The current route is a summary ASB route or
	 *  - The current route is a router route wihin another area or a current route for 
	 *    current area.
	 */
	if ((rr) && 
	    (   (RRT_PTYPE(r) == LS_SUM_ASB) ||
	        ((RRT_PTYPE(r) == LS_RTR) &&
		 ((dst != from_area) || ((dst == from_area) && (RRT_REV(rr) == RTAB_REV)))) ) )
	{
	    /*
     	     * Flush (if it exists) our ASB Summary LSA from the current area.
	     */
	    db = FindLSA(dst, RRT_DEST(r), MY_ID, LS_SUM_ASB);
	    if (db != LSDBNULL) {
		if ((db) && (DB_WHERE(db) == ON_SUMASB_LIST)) {
		    DB_WHERE(db) = ON_SUM_INFINITY;
	    	    DB_REMQUE(db);
	    	    DB_FREEME(db) = TRUE;
	    	    DB_ADDQUE(ospf.db_free_list, db);
		    sum_next_seq(db, dst, htonl(SUMLSInfinity), time_sec);
		    trace_tf(ospf.trace_options,
			     TR_OSPF_LSA_BLD,
			     0,
			     ("build_sum_asb: Locally Reachable ASB Summary %A flushed for Area %A. SPF/Sum Area %A/%A",
			      sockbuild_in(0, RRT_DEST(r)),
			      sockbuild_in(0, dst->area_id),
			      sockbuild_in(0, from_area->area_id),
			      sockbuild_in(0, area->area_id)));
		    lsa_generated = TRUE;		/* Indicate LSA generated and can be traced. 	*/
		}
	    }
	} else {					/* No Valid Intra-area route to ASB router.	*/
	    /*
	     * No intra-area route for current area. Determine if an LSA exists, 
	     */
	    foundlsa = AddLSA(&db, dst, RRT_DEST(r), MY_ID, LS_SUM_ASB);

	    if (!foundlsa) {
		/* 
		 * Summary ASB LSA did not exist for area:
		 * 	- If the route is valid for the current SPF run, build a new LSA for flooding.
		 *	- If the route not valid, undo the Addition of the area LSDB entry.
		 */
		if (!db)
		    return FLAG_NO_BUFS;		/* No memory - should not happen.		*/
		if(!BIT_TEST(dst->area_flags, OSPF_AREAF_STUB) &&
		  ((RRT_REV(r) == RTAB_REV) ||
		    (RRT_AREA(r) != from_area))) {
		    /*
		     * Valid Route - build new LSA and flood into area. 
		     */
		    DBADV_ALLOC(db, SUM_LA_HDR_SIZE);
		    if (!(s = DB_SUM(db))) {
			db_free(db, LS_SUM_ASB);
			return FLAG_NO_BUFS;		/* No memory - should not happen.		*/
		    }
		    s->ls_hdr.ls_type = LS_SUM_ASB;
		    s->ls_hdr.ls_id = RRT_DEST(r);
		    s->ls_hdr.adv_rtr = MY_ID;
		    s->ls_hdr.ls_seq = FirstSeq;
		    s->ls_hdr.length = htons(SUM_LA_HDR_SIZE);
		    s->tos0.tos_metric = htonl(RRT_COST(r));
		    DB_ADDQUE(dst->asblst, db);
		    DB_WHERE(db) = ON_SUMASB_LIST;
		    db->lsdb_time = time_sec;
		    txadd(db, &dst->txq, SUM_LA_HDR_SIZE, 0, dst);
		    trace_tf(ospf.trace_options,
			     TR_OSPF_LSA_BLD,
			     0,
			     ("build_sum_asb: ASB Summary %A generated for Area %A. From/Sum Area %A/%A",
			      sockbuild_in(0, RRT_DEST(r)),
			      sockbuild_in(0, dst->area_id),
			      sockbuild_in(0, from_area->area_id),
			      sockbuild_in(0, area->area_id)));
		    lsa_generated = TRUE;		/* Indicate LSA generated and can be traced. 	*/
		} else {				/* Invalid Route for SPF run.			*/			
		    /*
		     * Invalid route for current SPF - Undo area LSDB entry addition.
		     */
		    db_free(db, LS_SUM_ASB);
		}					/* End of invalid route for SPF run.		*/
	    } else {					/* Existing LSDB for AREA.			*/
		/*
		 * Summary ASB LSA existed for Area:
		 *	- If the route is valid for the current SPF run, assure it being advertised and
		 *	  determine whether or not it has changed. 
		 * 	- If the route is invalid for the current SPF run and it is being advertised, flush
		 * 	  it from the current area.
		 */
		if ((RRT_REV(r) == RTAB_REV) ||
		    (RRT_AREA(r) != from_area)) {
		    if (DB_WHERE(db) == ON_SUMASB_LIST) {
			/* 
			 * LSA is currently being advertised, determine whether it has changed and, if so,
			 * re-flood it into the current area with an updated sequence number.
			 */
			DB_FREEME(db) = FALSE;
			if (DB_SUM(db)->tos0.tos_metric != htonl(RRT_COST(r))) {
			    sum_next_seq(db, dst, htonl(RRT_COST(r)), time_sec);
			    trace_tf(ospf.trace_options,
				     TR_OSPF_LSA_BLD,
				     0,
				     ("build_sum_asb: ASB Summary %A re-generated for Area %A with metric of %d. From/Sum Area %A/%A",
				      sockbuild_in(0, RRT_DEST(r)),
				      sockbuild_in(0, dst->area_id),
				      RRT_COST(r),
				      sockbuild_in(0, from_area->area_id),
				      sockbuild_in(0, area->area_id)));
			    lsa_generated = TRUE;	/* Indicate LSA generated and can be traced. 	*/
			}
		    } else {				/* Not currently being advertised.		*/
			/*
			 * It is not on the list of summary ASBs being advertised. At this point, it should be 
			 * deducible that it is on the summary infinity list indicating that an unreachable 
			 * AS Border Router had become reachable. However, there is no check for the LSDB  
			 * being on the summary infinity list since, in any case, we need a new ASB Summary LSA. 
			 */
			if (DB_FREEME(db) == TRUE) {	/* Remove from the list to be freed.		*/
			    DB_FREEME(db) = FALSE;
			    DB_REMQUE(db);		
			}
			DB_WHERE(db) = ON_SUMASB_LIST;
			DB_ADDQUE(dst->asblst, db);
			sum_next_seq(db, dst, htonl(RRT_COST(r)), time_sec);
			trace_tf(ospf.trace_options,
				 TR_OSPF_LSA_BLD,
				 0,
				 ("build_sum_asb: ASB Summary %A for Area %A cost %d moved to SUMASB list for flooding. From/Sum Area %A/%A",
				  sockbuild_in(0, RRT_DEST(r)),
				  sockbuild_in(0, dst->area_id),
				  RRT_COST(r),
				  sockbuild_in(0, from_area->area_id),
				  sockbuild_in(0, area->area_id)));
			lsa_generated = TRUE;		/* Indicate LSA generated and can be traced. 	*/
		    }					/* End Valid Route not advertised.		*/
		} else {				/* Invalid Route for current SPF Run.		*/ 
		    /* 
		     * The route is invalid for the current SPF run. If it is not the summary infinity list, 
		     * put it there for flushing from the current area. Note that it will remain on the
		     * summary infinity list untill it has been flushed and conditions are such that it may
		     * be freed. 
		     */
		    if (DB_WHERE(db) != ON_SUM_INFINITY) {
			DB_WHERE(db) = ON_SUM_INFINITY;
			DB_REMQUE(db);
			DB_FREEME(db) = TRUE;
			DB_ADDQUE(ospf.db_free_list, db);
			sum_next_seq(db, dst, htonl(SUMLSInfinity), time_sec);
			trace_tf(ospf.trace_options,
				 TR_OSPF_LSA_BLD,
				 0,
				 ("build_sum_asb: ASB Summary %A flushed from Area %A due to invalid route. From/Sum Area %A/%A",
				  sockbuild_in(0, RRT_DEST(r)),
				  sockbuild_in(0, dst->area_id),
				  sockbuild_in(0, from_area->area_id),
				  sockbuild_in(0, area->area_id)));
			lsa_generated = TRUE;		/* Indicate LSA generated and can be traced. 	*/
		    }					/* End Invalid Route and advertised LSA.	*/
		}					/* End Invalid Route for current SPF run.	*/
	    }						/* End Existing LSA for Area.			*/
	}						/* End No Valid Intra-area route to ASB router.	*/
	if (lsa_generated
	    && TRACE_TF(ospf.trace_options, TR_OSPF_LSA_BLD)) {
	    ospf_trace_build(area, dst, db->lsdb_adv, FALSE);
	}
    } AREA_LIST_END(dst) ;				/* End of stepping through areas.		*/

    return FLAG_NO_PROBLEM;
}

/*
 * build_sum() builds all inter area routes
 * - changes have been sent when changes have been seen
 *    as a result of running spf or receiving a new sum pkt
 * - this routine just updates the sequence numbers and sends out new sum
 *    pkts called by tq_SumLsa
 */
int
build_sum __PF0(void)
{
    struct LSDB *db;
    struct AREA *a;
    int	ret_flag = FLAG_NO_PROBLEM;

    AREA_LIST(a) {
	/* new sequence number only */

	DB_RUNQUE(a->asblst, db) {
	    if ((time_sec - db->lsdb_time) < MinLSInterval)
		continue;
	    sum_next_seq(db, a, DB_SUM(db)->tos0.tos_metric, time_sec);

	    if (TRACE_TF(ospf.trace_options, TR_OSPF_LSA_BLD)) {
		ospf_trace_build(a, a, db->lsdb_adv, FALSE);
	    }

	} DB_RUNQUE_END(a->asblst, db) ;

	DB_RUNQUE(a->sumnetlst, db) {
	    if ((time_sec - db->lsdb_time) < MinLSInterval)
		continue;
	    sum_next_seq(db, a, DB_SUM(db)->tos0.tos_metric, time_sec);

	    if (TRACE_TF(ospf.trace_options, TR_OSPF_LSA_BLD)) {
		trace_only_tf(ospf.trace_options,
			      0,
			      ("build_sum: Summary %A re-generated for Area %A due to timer",
			       sockbuild_in(0, DB_NETNUM(db)),
			       sockbuild_in(0, a->area_id)));
		ospf_trace_build(a, a, db->lsdb_adv, FALSE);
	    }
	} DB_RUNQUE_END(a->sumnetlst, db) ;

	if (BIT_TEST(a->area_flags, OSPF_AREAF_STUB_DEFAULT)) {
	    /* Update stub area default route */

	    db = a->dflt_sum;
	    if ((time_sec - db->lsdb_time) >= MinLSInterval) {
	    	sum_next_seq(db, a, a->dflt_metric, time_sec);

		if (TRACE_TF(ospf.trace_options, TR_OSPF_LSA_BLD)) {
		    ospf_trace_build(a, a, db->lsdb_adv, FALSE);
		}

	    }
	}

	/* 
	 * Inter area routes determined from routing table 
	 */
	DB_RUNQUE(a->interlst, db) {
	    if ((time_sec - db->lsdb_time) < MinLSInterval)
		continue;
	    sum_next_seq(db, a, DB_SUM(db)->tos0.tos_metric, time_sec);

	    if (TRACE_TF(ospf.trace_options, TR_OSPF_LSA_BLD)) {
		ospf_trace_build(a, a, db->lsdb_adv, FALSE);
	    }
	} DB_RUNQUE_END(a->interlst, db) ;

	/* 
	 * Send the whole shebangee 
 	 */
	if (a->txq != LLNULL) {
	    self_orig_area_flood(a, a->txq, LS_SUM_NET);
	    ospf_freeq((struct Q **) &a->txq, ospf_lsdblist_index);
	}
	if (ret_flag)
	    return FLAG_NO_BUFS;
    } AREA_LIST_END(area) ;

    /* If they make it on the retrans list, that's good enough */
    return FLAG_NO_PROBLEM;
}

/*
 * build_inter: Build or Flush Intra Area Network Summary LSAs 
 *
 * 	This function from hell do one of the following:
 *
 *	- Flush self-originated Network Summary LSAs corresponding to
 *	  the LSDB entry for every AREA. This is done when a route does
 *	  not match the current SPF revision, is superceded by a network
 *	  range, has gone from being an Intra-Area or Inter-Area route to
 *        being an ASE route, or had gone from being an Intra-area route to
 *	  being an Inter-Area route.
 *	- Generate new Network Summary LSAs for other areas when a new Intra-area
 * 	  route is found, or an Intra-area routes metric and/or Next Hop changes, 
 *	  or when a route changes from being an ASE route to being an Intra-Area
 *	  or Inter-Area route.
 *
 * The function returns an indication if it cannot allocate storage. However, indication
 * is not interrogated in the invoking procedure (ospf_route_update).   
 */
int
build_inter __PF3(v, struct LSDB *,     /* LSDB entry from existing route.	*/
		  area, struct AREA *,	/* Originating Area.			*/
		  what, int)		/* Delete or Generate Flag.		*/
{
    struct LSDB *db;			/* Local LSDB entry for new LSAs.	*/
    struct AREA *dst;			/* Area to flush/generate LSA.		*/
    struct SUM_LA_HDR *s;		/* LSA Header for new LSAs.		*/
    int foundlsa = 0;			/* LSDB exists in AREA database flag.	*/
    /*
     * Deletion of existing LSAs
     */
    if (what == E_DELETE) { 		/* Deletion of summary LSA.		*/
	AREA_LIST(dst) {		/* Loop through areas for deletion.	*/
	    /* 
	     * Find the LSA and delete it if it is on the summary list and is not
	     * associated with a locally generated net range advertisement.
	     */
	    db = FindLSA(dst, DB_NETNUM(v), MY_ID, LS_SUM_NET);
	    if ((db) &&
		(DB_WHERE(db) != ON_SUM_INFINITY) &&
		!db->lsdb_net_range) {
		DB_WHERE(db) = ON_SUM_INFINITY;
	    	DB_REMQUE(db);
	    	DB_FREEME(db) = TRUE;
	    	DB_ADDQUE(ospf.db_free_list, db);
		sum_next_seq(db, dst, htonl(SUMLSInfinity), time_sec);
		trace_tf(ospf.trace_options,
			 TR_OSPF_LSA_BLD,
			 0,
			 ("build_inter: Summary Net %A flushed from Area %A due to E_DELETE",
			  sockbuild_in(0, DB_NETNUM(v)),
			  sockbuild_in(0, dst->area_id)));
	    }
	} AREA_LIST_END(dst) ;          /* End of areas for deletion.		*/
    /*
     * Addition of New LSAs
     */
    } else {				/* Not deletion - New Summary LSAs.	*/
	assert(v->lsdb_route);		/* LSA should point to route which
					   points to LSA.			*/
	AREA_LIST(dst) {		/* Loop through areas for addition.	*/
	    /* 
	     * Find the LSA and delete it if it is on the summary list for the
	     * originating area.
	     */
	    if (area == dst) {		/* On originating area??		*/
		db = FindLSA(dst, DB_NETNUM(v), MY_ID, LS_SUM_NET);
		if ((db) &&
		    (DB_WHERE(db) != ON_SUM_INFINITY) &&
		    !db->lsdb_net_range) {    /* Should not happen.		*/
		    DB_WHERE(db) = ON_SUM_INFINITY;
		    DB_REMQUE(db);
		    DB_FREEME(db) = TRUE;
		    DB_ADDQUE(ospf.db_free_list, db);
		    sum_next_seq(db, dst, htonl(SUMLSInfinity), time_sec);
		    trace_tf(ospf.trace_options,
			     TR_OSPF_LSA_BLD,
			     0,
			     ("build_inter: Summary Net %A flushed from Origination Area %A",
			      sockbuild_in(0, DB_NETNUM(v)),
			      sockbuild_in(0, dst->area_id)));
		}
	    } else {			/* Not originating area - generate LSA. */
		/*
		 * Add the LSDB to the area's data base if it does not exist.
		 */
		foundlsa = AddLSA(&db, dst, DB_NETNUM(v), MY_ID, LS_SUM_NET);
		if (!foundlsa) {	/* No LSA currently in DB for Area.     */
		    u_int16 age = 0;

		    if (db == LSDBNULL)	{	/* Out of Memory??		*/
			return FLAG_NO_BUFS;
		    }
		    DBADV_ALLOC(db, SUM_LA_HDR_SIZE);
		    if (!(s = DB_SUM(db))) {		/* Out of Memory?			*/
			db_free(db, LS_SUM_NET);
			return FLAG_NO_BUFS;
		    }			/* End out of memory.			*/
		    /*
		     * Build new Network Summary LSA for Area.
		     */
		    s->ls_hdr.ls_type = LS_SUM_NET;
		    s->ls_hdr.ls_id = DB_NETNUM(v);
		    s->ls_hdr.adv_rtr = MY_ID;
		    s->ls_hdr.ls_seq = FirstSeq;
		    s->ls_hdr.length = htons(SUM_LA_HDR_SIZE);
		    s->net_mask = DB_MASK(v);
		    s->tos0.tos_metric = htonl(ORT_COST(v->lsdb_route));
		    DB_ADDQUE(dst->interlst, db);
		    DB_WHERE(db) = ON_INTER_LIST;
		    db->lsdb_time = time_sec;
		    txadd(db, &dst->txq, SUM_LA_HDR_SIZE, age, dst);
		    trace_tf(ospf.trace_options,
			     TR_OSPF_LSA_BLD,
			     0,
			     ("build_inter: Summary Net %A generated for Area %A",
			      sockbuild_in(0, DB_NETNUM(v)),
			      sockbuild_in(0, dst->area_id)));
		} else {		/* LSA already existed. 		*/
		    /* 
		     * Regenerate the LSA with a new sequence number as long as it does not
		     * interfere with an LSA associated with a reachable net range - Note that
		     * ospf_route_update checking should prevent this.
		     */
		    if (!db->lsdb_net_range ||
			DB_WHERE(db) == ON_SUM_INFINITY) {	
			DB_MASK(db) = sock2ip(v->lsdb_route->rt_dest_mask);
			DB_FREEME(db) = FALSE;
			DB_REMQUE(db);	/* remove from db_free_list.		 */
			DB_ADDQUE(dst->interlst, db);
			DB_WHERE(db) = ON_INTER_LIST;
			sum_next_seq(db, dst, htonl(ORT_COST(v->lsdb_route)), time_sec);
			trace_tf(ospf.trace_options,
				 TR_OSPF_LSA_BLD,
				 0,
				 ("build_inter: Summary Net %A re-generated for Area %A with cost %d and mask %A",
				  sockbuild_in(0, DB_NETNUM(v)),
				  sockbuild_in(0, dst->area_id),
				  htonl(ORT_COST(v->lsdb_route)),
				  v->lsdb_route->rt_dest_mask));
		    }
		}			/* End Re-generate existing LSA.	*/

		if (TRACE_TF(ospf.trace_options, TR_OSPF_LSA_BLD)) {
		    ospf_trace_build(&ospf.backbone, dst, db->lsdb_adv, FALSE);
		}
	    }
	} AREA_LIST_END(dst) ;          /* End of areas for addition.		*/
    }					/* End of new summary LSAs.		*/
    
    return FLAG_NO_PROBLEM;		/* Successful completion.		*/
}					/* End of the infamous build_inter.	*/

/*
 * Build default route for area border router
 * - called by init
 */
void
build_sum_dflt __PF1(a, struct AREA *)
{
    struct SUM_LA_HDR *s;
    struct LSDB *db;

    AddLSA(&db, a, 0, MY_ID, LS_SUM_NET);
    DBADV_ALLOC(db, SUM_LA_HDR_SIZE);
    s = DB_SUM(db);
    s->ls_hdr.ls_type = LS_SUM_NET;
    s->ls_hdr.ls_id = 0;
    s->ls_hdr.adv_rtr = MY_ID;
    s->ls_hdr.ls_seq = FirstSeq;
    s->ls_hdr.length = htons(SUM_LA_HDR_SIZE);
    s->net_mask = 0;
    s->tos0.tos_metric = a->dflt_metric;
    /* 
     * add to this area's net sum list 
     */
    DB_WHERE(db) = ON_SUMNET_LIST;
    a->dflt_sum = db;
    db->lsdb_time = time_sec;
    ospf_checksum_sum(DB_SUM(db), SUM_LA_HDR_SIZE, a->db_chksumsum);
    a->db_cnts[LS_SUM_NET]++;

    if (TRACE_TF(ospf.trace_options, TR_OSPF_LSA_BLD)) {
	ospf_trace_build(a, a, db->lsdb_adv, FALSE);
    }
}

/*
 * Acks have come home, generate new LSA with seq FirstSeq, put on txq
 */
int
beyond_max_seq __PF6(a, struct AREA *,
		     intf, struct INTF *,
		     db, struct LSDB *,
		     txq, struct ospf_lsdb_list **,
		     asetxq, struct ospf_lsdb_list **,
		     from_rxlinkup, int)
{

    if (LS_TYPE(db) == LS_RTR) {
	if (from_rxlinkup) {
	    a->build_rtr = TRUE;
	    return FLAG_NO_PROBLEM;
	}
	return build_rtr_lsa(a, txq, TRUE);
    }

    if (LS_TYPE(db) == LS_NET) {
	if (!intf) {
	    INTF_LIST(intf, a) {
		if (INTF_LCLADDR(intf) == LS_ID(db)) {
		    break;
		}
	    } INTF_LIST_END(intf, a) ;

	    if (intf == &a->intf) {
		return FLAG_NO_PROBLEM;		/* something has gone weird */
	    }
	}
	if (from_rxlinkup && intf->state == IDr) {
	    BIT_SET(intf->flags, OSPF_INTFF_BUILDNET);
	    return FLAG_NO_PROBLEM;
	}
	return build_net_lsa(intf, txq, TRUE);
    }


    db->lsdb_seq_max = FALSE;
    LS_SEQ(db) = NEXTNSEQ(LS_SEQ(db));
    if (LS_TYPE(db) == LS_ASE) {
    	ospf.db_chksumsum -= LS_CKS(db);
	txadd(db, asetxq, (size_t) ntohs(LS_LEN(db)), 0, a);
    } else {
    	a->db_chksumsum -= LS_CKS(db);
	txadd(db, txq, (size_t) ntohs(LS_LEN(db)), 0, a);
    }
    return FLAG_NO_PROBLEM;
}
