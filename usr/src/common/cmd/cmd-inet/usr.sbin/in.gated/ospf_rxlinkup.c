#ident	"@(#)ospf_rxlinkup.c	1.3"
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


static int force_flood;

/*
 * Information about a specific received LSA
 */
struct DB_INFO {
    size_t len;			/* Length of new adv */
    int foundlsa;		/* If found in lsdb or not */
    struct LSDB *db; 		/* Current or to be installed db struct */
    struct AREA *area;	 	/* Associated area */
    struct INTF *intf; 		/* Interface received on */
    struct NBR *nbr;		/* Neighbor received on */
    union LSA_PTR adv; 		/* New advertisement */
    u_int diff;			/* Difference between sec and db orig rx time */
    sockaddr_un *src, *dst;
};

/* Direct ack queue */
static struct LS_HDRQ acks;

/* Newer LSAs */
static struct ospf_lsdb_list *rejects;

/*
 * Compare LSAs for changes
 * - schedule partial update
 * - return spfsched
 */
static int
lsacmp  __PF3(area, struct AREA *,
	      adv, union LSA_PTR,
	      db, struct LSDB *)
{
    int spfsched = 0;

    if ((ADV_AGE(db) >= MaxAge && adv.rtr->ls_hdr.ls_age < MaxAge) ||
	(ADV_AGE(db) < MaxAge && adv.rtr->ls_hdr.ls_age >= MaxAge)) {
	if (LS_TYPE(db) > LS_NET) {
	    DBH_RERUN(&area->htbl[LS_TYPE(db)][db->lsdb_hash]) = TRUE;
	    /*
	    * If virtual link transit area rerun backbone
	     */
	    if ((LS_TYPE(db) != LS_ASE) && BIT_TEST(area->area_flags, OSPF_AREAF_VIRTUAL_UP)) {
		ospf.backbone.spfsched |= SCHED_BIT(LS_NET);
	    }
	}
	return SCHED_BIT(LS_TYPE(db));
    }

    switch (adv.rtr->ls_hdr.ls_type) {
    case LS_RTR:
	if ((adv.rtr->ls_hdr.length != LS_LEN(db)) ||
	    (adv.rtr->lnk_cnt != DB_RTR(db)->lnk_cnt) ||
	    (adv.rtr->E_B != DB_RTR(db)->E_B) ||
	    (RTR_LINK_CMP(adv.rtr, DB_RTR(db),
			  (ntohs(adv.rtr->ls_hdr.length) - RTR_LA_HDR_SIZE))))
	    spfsched = SCHED_BIT(LS_RTR);
	break;

    case LS_NET:
	if ((adv.net->ls_hdr.length != LS_LEN(db)) ||
	    (NET_ATTRTR_CMP(adv.net, DB_NET(db),
			    ntohs(adv.net->ls_hdr.length) - (NET_LA_HDR_SIZE))))
	    spfsched = SCHED_BIT(LS_NET);
	break;

    case LS_SUM_NET:
	/*
	 * Just check metric
	 */
	if (adv.sum->tos0.tos_metric != (DB_SUM(db)->tos0.tos_metric)) {
	    /*
	     * If there is a virtual link schedule backbone spf
	     */
	    if (BIT_TEST(area->area_flags, OSPF_AREAF_VIRTUAL_UP))
		ospf.backbone.spfsched |= SCHED_BIT(LS_NET);

	    /*
	     * schedule netsum if not bdr rtr or bdr rtr and
	     * area is backbone
	     */
	    if ((IAmBorderRtr && area->area_id == OSPF_BACKBONE) ||
		(!(IAmBorderRtr))) {
		spfsched = SCHED_BIT(LS_SUM_NET);
		DBH_RERUN(&area->htbl[LS_SUM_NET][db->lsdb_hash]) = TRUE;
	    }
	}
	break;

    case LS_SUM_ASB:
	if (adv.sum->tos0.tos_metric != (DB_SUM(db)->tos0.tos_metric)) {
	    /*
	     * If there is a virtual link schedule backbone spf
	     */
	    if (BIT_TEST(area->area_flags, OSPF_AREAF_VIRTUAL_UP)) {
		ospf.backbone.spfsched |= SCHED_BIT(LS_NET);
	    }
	    /*
	     * schedule asbrsum if not bdr rtr or bdr rtr and
	     * area is backbone - will have to rerun sum and ase
	     */
	    if ((IAmBorderRtr && area->area_id == OSPF_BACKBONE) ||
		(!(IAmBorderRtr))) {
		spfsched = SCHED_BIT(LS_SUM_ASB);
		DBH_RERUN(&area->htbl[LS_SUM_ASB][db->lsdb_hash]) = TRUE;
	    }
	}
	break;

    case LS_ASE:
	if (ASE_TOS_CMP(&DB_ASE(db)->tos0, &adv.ase->tos0)) {
	    spfsched = SCHED_BIT(LS_ASE);
	    DBH_RERUN(&area->htbl[LS_ASE][db->lsdb_hash]) = TRUE;
	}
	break;
    }

    return spfsched;
}

/*
 * Remove request from nbr request list
 */
int
nbr_rem_req __PF2(nbr, struct NBR *,
		  adv, union LSA_PTR)
{
    struct LS_REQ *ls_req;

    for (ls_req = nbr->ls_req[adv.rtr->ls_hdr.ls_type];
	 ls_req != LS_REQ_NULL; ls_req = ls_req->ptr[NEXT])
	if ((adv.rtr->ls_hdr.ls_id == ls_req->ls_id) &&
	    (adv.rtr->ls_hdr.adv_rtr == ls_req->adv_rtr)) {
	    int ret = REQ_LESS_RECENT;
	    
	    if (MORE_RECENT(ls_req, &adv.rtr->ls_hdr, 0)) {
		return REQ_MORE_RECENT;
	    }
	    nbr->reqcnt--;
	    if (SAME_INSTANCE(ls_req, &adv.rtr->ls_hdr, 0))
		ret =  REQ_SAME_INSTANCE;
	    REM_Q(nbr->ls_req[adv.rtr->ls_hdr.ls_type],
		  ls_req,
		  ospf_lsreq_index);
	    return ret;
	}
    return REQ_NOT_FOUND;
}

/*
 * Handle self-originated LSA
 */
static int
rx_self_orig  __PF3(dbi, struct DB_INFO *,
		    trans, struct ospf_lsdb_list **,
		    asetrans, struct ospf_lsdb_list **)
{
    int spfsched = 0;
    int implied_ack = FALSE;
    struct ospf_lsdb_list *ll;
    struct INTF *db_intf;
    u_int16 age = 0;
    u_int32 new_seq;
    int flushit = FALSE;


    if ((!dbi->foundlsa) ||
	MORE_RECENT(&dbi->adv.rtr->ls_hdr,
		    &DB_RTR(dbi->db)->ls_hdr,
		    dbi->diff)) {
	/*
	 * newer one has come in, we have an old one floating around
	 */

	/* 
	 * if there is a retrans list  free it 
	 */
	if ((dbi->foundlsa) && (dbi->db->lsdb_retrans != NLNULL))
	    rem_db_retrans(dbi->db);

	/*
	 * build new LSA and send
	 */
	switch (dbi->adv.rtr->ls_hdr.ls_type) {
	case LS_RTR:
	    age = LS_AGE(dbi->db);
	    LS_AGE(dbi->db) = 0;
	    LS_SEQ(dbi->db) = dbi->adv.rtr->ls_hdr.ls_seq;
	    dbi->area->db_chksumsum -= LS_CKS(dbi->db);
	    ospf_checksum_sum(DB_RTR(dbi->db), dbi->len, dbi->area->db_chksumsum);
	    LS_AGE(dbi->db) = age;
	    force_flood |= PTYPE_BIT(LS_RTR);
	    dbi->area->build_rtr = TRUE;
	    break;

	case LS_NET:
	    /*
	     * Find interface associated with this net lsa
	     */
	    INTF_LIST(db_intf, dbi->area) {
		if (db_intf->type != POINT_TO_POINT
		    && ADV_NETNUM(dbi->adv.net) == INTF_NET(db_intf)) {
		    break;
		}
	    } INTF_LIST_END(db_intf, dbi->area) ;
	    if ((!dbi->foundlsa) ||
		(db_intf == &dbi->area->intf) ||
		(db_intf->state != IDr) ||
		((db_intf->state == IDr) && (!db_intf->nbrFcnt))) {
		/*
		 * nbr has an out of date net lsa around
		 */
		if (!dbi->foundlsa) {
		    DBADV_ALLOC(dbi->db, dbi->len);
		    if (!DB_NET(dbi->db)) {
			db_free(dbi->db, LS_NET);
			return FLAG_NO_BUFS;
		    }
		    ADV_COPY(dbi->adv.net, DB_NET(dbi->db), (size_t) dbi->len);
		} else {
		    LS_SEQ(dbi->db) = dbi->adv.net->ls_hdr.ls_seq;
		    dbi->area->db_chksumsum -= LS_CKS(dbi->db);
		}
		/*
		 * MaxAge so everyone deletes it
		 */
		age = MaxAge;
		flushit = TRUE;
		ll = (struct ospf_lsdb_list *) task_block_alloc(ospf_lsdblist_index);
		ll->lsdb = dbi->db;
		EN_Q((*trans), ll);
	    } else {
		/*
		 * have one in lsdb and this rtr is Dr
		 */
		dbi->area->db_chksumsum -= LS_CKS(dbi->db);
		LS_SEQ(dbi->db) = dbi->adv.net->ls_hdr.ls_seq;
		age = LS_AGE(dbi->db);
		if (db_intf)
		    BIT_SET(db_intf->flags, OSPF_INTFF_BUILDNET);
	    }
	    dbi->db->lsdb_time = time_sec;
	    LS_AGE(dbi->db) = 0;
	    ospf_checksum_sum(DB_NET(dbi->db), dbi->len, dbi->area->db_chksumsum);
	    LS_AGE(dbi->db) = age;
	    force_flood |= SCHED_BIT(LS_NET);
	    break;

	case LS_SUM_ASB:
	case LS_SUM_NET:
	    ll = (struct ospf_lsdb_list *) task_block_alloc(ospf_lsdblist_index);
	    /*
	     * An old sum hangin' around
	     */
	    if (!dbi->foundlsa) {
		DBADV_ALLOC(dbi->db, dbi->len);
		if (!DB_SUM(dbi->db)) {
		    db_free(dbi->db, dbi->adv.rtr->ls_hdr.ls_type);
		    return FLAG_NO_BUFS;
		}
		ADV_COPY(dbi->adv.sum, DB_SUM(dbi->db), dbi->len);
		DB_SUM(dbi->db)->tos0.tos_metric = htonl(SUMLSInfinity);
	    	/* MODIFIED 5/1/92 */
		DB_WHERE(dbi->db) = ON_SUM_INFINITY;
		flushit = TRUE;
		age = MaxAge;	/* let everyone know to free it */
	    	/* MODIFIED 5/1/92 */
		new_seq = NEXTNSEQ(dbi->adv.sum->ls_hdr.ls_seq);
	    } else if (!dbi->db->lsdb_seq_max
		       && LS_SEQ(dbi->db) == MaxSeqNum) {
		/*
		 * Flush from everyone's db
		 */
		age = MaxAge;
		new_seq = MaxSeqNum;
		dbi->db->lsdb_seq_max = TRUE;
		dbi->area->db_chksumsum -= LS_CKS(dbi->db);
	    } else {
		if (DB_WHERE(dbi->db) == ON_SUM_INFINITY) {
		    flushit = TRUE;
		    age = MaxAge;
		} else {
		    age = 0;
		}
		new_seq = NEXTNSEQ(dbi->adv.sum->ls_hdr.ls_seq);
		dbi->area->db_chksumsum -= LS_CKS(dbi->db);
	    }

	    LS_AGE(dbi->db) = 0;
	    LS_SEQ(dbi->db) = new_seq;
	    dbi->db->lsdb_time = time_sec;
	    /*
	     * add new chksum
	     */
	    ospf_checksum_sum(DB_SUM(dbi->db), dbi->len, dbi->area->db_chksumsum);
	    LS_AGE(dbi->db) = age;
	    ll->lsdb = dbi->db;
	    EN_Q((*trans), ll);
	    break;

	case LS_ASE:
	    ll = (struct ospf_lsdb_list *) task_block_alloc(ospf_lsdblist_index);
	    /*
	     * An old ase hangin' around
	     */
	    if (!dbi->foundlsa) {
		/*
		 * Save ASE type bit
		 */
		u_int32 bitE = htonl(ASE_bit_E);

		DBADV_ALLOC(dbi->db, dbi->len);
		if (!DB_ASE(dbi->db)) {
		    db_free(dbi->db, LS_ASE);
		    return FLAG_NO_BUFS;
		}
		ADV_COPY(dbi->adv.ase, DB_ASE(dbi->db), dbi->len);
		age = MaxAge;
	    	DB_WHERE(dbi->db) = ON_ASE_INFINITY;
		flushit = TRUE;
	    	/* MODIFIED 5/1/92 */
		if (!(bitE & DB_ASE(dbi->db)->tos0.tos_metric))
		    bitE = 0;
		DB_ASE(dbi->db)->tos0.tos_metric = htonl(ASELSInfinity);
		DB_ASE(dbi->db)->tos0.tos_metric |= htonl(bitE);
		new_seq = NEXTNSEQ(dbi->adv.ase->ls_hdr.ls_seq);
	    } else if (!dbi->db->lsdb_seq_max && 
		       LS_SEQ(dbi->db) == MaxSeqNum) {
		/*
		 * Flush from everyone's db
		 */
		age = MaxAge;
		new_seq = MaxSeqNum;
		dbi->db->lsdb_seq_max = TRUE;
		ospf.db_chksumsum -= LS_CKS(dbi->db);
	    } else {
		if (DB_WHERE(dbi->db) != ON_ASE_LIST) {
		    flushit = TRUE;
		    age = MaxAge;
		} else {
		    age = 0;
		}
		new_seq = NEXTNSEQ(dbi->adv.sum->ls_hdr.ls_seq);
		ospf.db_chksumsum -= LS_CKS(dbi->db);
	    }
	    LS_SEQ(dbi->db) = new_seq;
	    dbi->db->lsdb_time = time_sec;
	    LS_AGE(dbi->db) = 0;
	    /*
	     * add new chksum
	     */
	    ospf_checksum_sum(DB_ASE(dbi->db), dbi->len, ospf.db_chksumsum);
	    LS_AGE(dbi->db) = age;
	    ll->lsdb = dbi->db;
	    EN_Q((*asetrans), ll);
	    break;
	}

	/*
	 * If it is to be flushed, put on free list
	 */
	if (flushit) {
	    DB_REMQUE(dbi->db);
	    DB_FREEME(dbi->db) = TRUE;
	    DB_ADDQUE(ospf.db_free_list, dbi->db);
	}
    } else if (SAME_INSTANCE(&dbi->adv.rtr->ls_hdr,
		&DB_RTR(dbi->db)->ls_hdr,
		dbi->diff)) {
	if (dbi->nbr->state < NFULL) {
	    /*
	     * Out of sync with nbr's state machine
	     */
	    if (nbr_rem_req(dbi->nbr, dbi->adv)) {
		(*(nbr_trans[BAD_LS_REQ][dbi->nbr->state]))(dbi->intf,dbi->nbr);
		return FLAG_BAD_REQ;
	    }
	}
	/*
         * remove from retransmit lists
         */
	(void) rem_nbr_ptr(dbi->db, dbi->nbr);
	implied_ack |= rem_db_ptr(dbi->nbr, dbi->db);
	/*
         * in this case, implied ack means that the lsa wasn't
         * on the retrans list so send direct ack
         */
	if (!implied_ack) {
	    ADD_ACK(&acks, dbi->db);
	}

	/*
         * If seqnum is MaxSeq and this is last ack, generate a new one
         */
	if ((dbi->db->lsdb_seq_max == TRUE) &&
	    (dbi->db->lsdb_retrans == NLNULL) &&
	    (DB_FREEME(dbi->db) != TRUE))
	    beyond_max_seq(dbi->area,dbi->intf,dbi->db,trans,asetrans,TRUE);
    } else {
	/*
	 * Less recent
	*/
	if (dbi->nbr->state < NFULL) {
	    /*
	     * Out of sync with nbr's state machine
	     */
	    if (nbr_rem_req(dbi->nbr, dbi->adv)) {
		(*(nbr_trans[BAD_LS_REQ][dbi->nbr->state]))(dbi->intf,dbi->nbr);
		return FLAG_BAD_REQ;
	    }
	} else {
	    OSPF_LOG_RX_LSA2(OSPF_ERR_UPD_OLDER,
			     dbi->intf,
			     dbi->src,
			     dbi->dst,
			     &dbi->adv.rtr->ls_hdr,
			     dbi->db);
	}
    }
    return spfsched;
}


/*
 * Handle non self-originated LSA
 */
static int
not_my_lsa  __PF3(dbi, struct DB_INFO *,
		  trans, struct ospf_lsdb_list **,
		  asetrans, struct ospf_lsdb_list **)
{
    int spfsched = 0;
    int implied_ack = FALSE;
    struct ospf_lsdb_list *ll;

    /*
     * if found and not the same or if not found
     */
    if (!dbi->foundlsa ||
	MORE_RECENT(&dbi->adv.rtr->ls_hdr,
		    &DB_RTR(dbi->db)->ls_hdr,
		    dbi->diff)) {
	if (dbi->foundlsa && dbi->diff < MinLSInterval) {
	    return FLAG_NO_PROBLEM;		/* The Tsuchiya fix */
	}

	/*
	 * Keep track of received new instances for MIBness
	 */
	ospf.rx_new_lsa++;

	/*
         * install in lsdb - may have to recalculate routing table
         */
	if (!dbi->foundlsa) {
	    /*
	     * allocate a new one and assign the lsa info to it
	     */
	    DBADV_ALLOC(dbi->db, dbi->len);
	    if (!DB_RTR(dbi->db)) {
		/* No memory - undo a couple of things... */
		ospf.rx_new_lsa--;
		db_free(dbi->db, dbi->adv.rtr->ls_hdr.ls_type);
		return FLAG_NO_BUFS;
	    }

	    ADV_COPY(dbi->adv.rtr, DB_NET(dbi->db), dbi->len);
	    switch (LS_TYPE(dbi->db)) {
	    case LS_RTR:
	    case LS_NET:
		/*
		 * will rerun intra()
		 */
		spfsched |= (SCHED_BIT(LS_TYPE(dbi->db)));
		break;

	    case LS_SUM_NET:
		/* 
		 * add net 
		 */
		if ((IAmBorderRtr && dbi->area->area_id == OSPF_BACKBONE) ||
		    (!(IAmBorderRtr))) {
		    spfsched = SCHED_BIT(LS_SUM_NET);
		    DBH_RERUN(&dbi->area->htbl[LS_SUM_NET][dbi->db->lsdb_hash]) = TRUE;
		}
		if (BIT_TEST(dbi->area->area_flags, OSPF_AREAF_VIRTUAL_UP))
		    ospf.backbone.spfsched |= SCHED_BIT(LS_NET);
		break;

	    case LS_SUM_ASB:
		/* add as border rtr and schedule ase -
		   ls_ase may have become reachable */
		if ((IAmBorderRtr && dbi->area->area_id == OSPF_BACKBONE) ||
		    (!(IAmBorderRtr))) {
		    spfsched = SCHED_BIT(LS_SUM_ASB);
		    DBH_RERUN(&dbi->area->htbl[LS_SUM_ASB][dbi->db->lsdb_hash]) = TRUE;
		}
		if (BIT_TEST(dbi->area->area_flags, OSPF_AREAF_VIRTUAL_UP))
		    ospf.backbone.spfsched |= SCHED_BIT(LS_RTR);
		break;

	    case LS_ASE:
		/* new ase has come in */
		spfsched = SCHED_BIT(LS_ASE);
		DBH_RERUN(&dbi->area->htbl[LS_ASE][dbi->db->lsdb_hash]) = TRUE;
		break;
	    }
	} else {
	    u_int16 old_age;
	    u_int32  old_cks;

	    old_age = ADV_AGE(dbi->db);
	    old_cks = LS_CKS(dbi->db);

	    /*
	     * found lsa
	     */

	    /*
	     * see if they're the same - if not schedule spf
	     */
	    spfsched |= lsacmp(dbi->area, dbi->adv, dbi->db);

	    /*
	     * save an alloc
     	     */
	    DBADV_FREE(dbi->db);
	    DBADV_ALLOC(dbi->db, dbi->len);
	    ADV_COPY(dbi->adv.rtr, DB_RTR(dbi->db), dbi->len);

	    /*
	     * if there is a retrans list free it
	     */
	    if (dbi->db->lsdb_retrans != NLNULL)
		rem_db_retrans(dbi->db);

	    /*
	     * if new one is no longer maxage, remove from free list
	     */
	    if ((old_age >= MaxAge) &&
		(LS_AGE(dbi->db) < MaxAge)) {
		if (DB_FREEME(dbi->db)) {
		    DB_REMQUE(dbi->db);
		    DB_FREEME(dbi->db) = FALSE;
		}
	    } else if ((LS_AGE(dbi->db) >= MaxAge) &&
		       (DB_FREEME(dbi->db) == FALSE)) {
		/*
		 * set it up so when retrans list is empty, free
		 */

		DB_REMQUE(dbi->db);
		DB_FREEME(dbi->db) = TRUE;
		DB_ADDQUE(ospf.db_free_list, dbi->db);
	    }

	    /*
	     * Subtract chksum for MIBness
	     */
	    if (LS_TYPE(dbi->db) < LS_ASE)
	    	dbi->area->db_chksumsum -= old_cks;
	    else 
	    	ospf.db_chksumsum -= old_cks;

	}

	ll = (struct ospf_lsdb_list *) task_block_alloc(ospf_lsdblist_index);
	ll->lsdb = dbi->db;
	/*
         * put on appropriate list for flooding and update new checksum sum
         */
	if (LS_TYPE(dbi->db) < LS_ASE) {
	    EN_Q((*trans), ll)
	    /*
	     * Update area chksumsum for MIB
	     */
	    dbi->area->db_chksumsum += LS_CKS(dbi->db);
	} else {
	    EN_Q((*asetrans), ll);
	    ospf.db_chksumsum += LS_CKS(dbi->db);
	}

	/*
         * note when it came it
         */
	dbi->db->lsdb_time = time_sec;

	/*
         * will add non-direct acks in flood()
         */
    } else if (SAME_INSTANCE(&dbi->adv.rtr->ls_hdr,
			     &DB_RTR(dbi->db)->ls_hdr,
			     dbi->diff)) {
	if (dbi->nbr->state < NFULL) {	/* remove from req list */
	    /* Out of sync with nbr's state machine */
	    if (nbr_rem_req(dbi->nbr, dbi->adv)) {
		(*(nbr_trans[BAD_LS_REQ][dbi->nbr->state]))(dbi->intf,dbi->nbr);
		return FLAG_BAD_REQ;
	    }
	}
	/* count same instance as ack, remove from nbrs list */
	/* remove from db */
	(void) rem_nbr_ptr(dbi->db, dbi->nbr);
	/* remove from nbr */
	implied_ack = rem_db_ptr(dbi->nbr, dbi->db);
	if (implied_ack &&
	    (dbi->intf->state == IBACKUP &&
	     NBR_ADDR(dbi->nbr) == NBR_ADDR(dbi->intf->dr))) {
	    ADD_ACK_INTF(dbi->intf, dbi->db);
	} else if (implied_ack &&
		   DB_CAN_BE_FREED(dbi->db) &&
		   ADV_AGE(dbi->db) >= MaxAge) {
	    /*
	     * if MaxAge and not retrans queue, remove from lsdb
	     */
	    db_free(dbi->db, LS_TYPE(dbi->db));
	} else if (!implied_ack) {
	    /* send direct ack */
	    ADD_ACK(&acks, dbi->db);
	}
    } else {				/* Less recent LSA */
	if (dbi->nbr->state < NFULL) {	/* remove from req list */
	    /* Out of sync with nbr's state machine */
	    if (nbr_rem_req(dbi->nbr, dbi->adv)) {
		(*(nbr_trans[BAD_LS_REQ][dbi->nbr->state]))(dbi->intf,dbi->nbr);
		return FLAG_BAD_REQ;
	    }
	} else {
	    /* Send more recent LSA back to this neighbor */

	    /* Make sure we are not already on his queue */
	    if (!dbi->db->lsdb_retrans
		|| !find_db_ptr(dbi->nbr, dbi->db)) {

		/* Add to the transmission queue */
		ll = (struct ospf_lsdb_list *) task_block_alloc(ospf_lsdblist_index);
		ll->lsdb = dbi->db;
		ll->flood = FLOOD;
		EN_Q(rejects, ll);

		/* Add to the retransmission queue for this neighbor */
		add_nbr_retrans(dbi->nbr, ll->lsdb);
		add_db_retrans(ll->lsdb, dbi->nbr);
	    }
	}

	OSPF_LOG_RX_LSA2(OSPF_ERR_UPD_OLDER,
			 dbi->intf,
			 dbi->src,
			 dbi->dst,
			 &dbi->adv.rtr->ls_hdr,
			 dbi->db);
    }
    return spfsched;
}


int
ospf_rx_lsupdate  __PF7(lsup, struct LS_UPDATE_HDR *,
			nbr, struct NBR *,
			intf, struct INTF *,
			src, sockaddr_un *,
			dst, sockaddr_un *,
			router_id, sockaddr_un *,
			olen, size_t)
{
    struct DB_INFO dbi;
    int i;			/* length of adv */
    int spfsched = 0;
    u_int reqcnt;		/* current number of requests on nbr->ls_req */
    /* list of lsdbs to be sent */
    struct ospf_lsdb_list *trans = LLNULL, *asetrans = LLNULL;
    struct ospf_lsdb_list *src_trans = LLNULL, *src_asetrans = LLNULL;
    struct AREA *a;
    struct INTF *intf1;

    if (nbr->state < NEXCHANGE) {
	return OSPF_ERR_UPD_STATE;
    }

    dbi.intf = intf;
    dbi.foundlsa = TRUE;
    dbi.area = intf->area; /* area received from */
    dbi.nbr = nbr;
    dbi.src = src;
    dbi.dst = dst;

    /* log count of ls_reqests */
    reqcnt = dbi.nbr->reqcnt;
    force_flood = 0;		/* use if self_orig and build rtr or net */

    GNTOHL(lsup->adv_cnt);

    for (i = 0, dbi.adv.rtr = (struct RTR_LA_HDR *) &lsup->adv.rtr;
	 i < lsup->adv_cnt && olen;
	 i++,
	 olen -= dbi.len,
	 dbi.adv.rtr = (struct RTR_LA_HDR *) ((long) dbi.adv.rtr + dbi.len)) {
	u_int newage = ntohs(dbi.adv.rtr->ls_hdr.ls_age);

	dbi.len = ntohs(dbi.adv.rtr->ls_hdr.length);
	dbi.adv.rtr->ls_hdr.ls_age = 0;

	if (ospf_checksum_bad(dbi.adv.rtr, dbi.len)) {
	    OSPF_LOG_RX_LSA1(OSPF_ERR_UPD_CHKSUM,
			     dbi.intf,
			     src,
			     dst,
			     &dbi.adv.rtr->ls_hdr,
			     "	CHKSUM",
			     (time_t) newage);
	    continue;
	}

	switch (dbi.adv.rtr->ls_hdr.ls_type) {
	case LS_RTR:
	case LS_NET:
	case LS_SUM_NET:
	case LS_SUM_ASB:
	case LS_ASE:
	    /* OK */
	    break;

	case LS_GM:
	case LS_NSSA:
	    /* Ignore */
	    continue;

	default:
	    OSPF_LOG_RX_LSA1(OSPF_ERR_UPD_TYPE,
			     dbi.intf,
			     src,
			     dst,
			     &dbi.adv.rtr->ls_hdr,
			     "	BAD TYPE",
			     (time_t) newage);
	    continue;
	}

	/* If stub area and type is ASE continue */
	if (BIT_TEST(dbi.area->area_flags, OSPF_AREAF_STUB) && 
	    dbi.adv.rtr->ls_hdr.ls_type == LS_ASE)
	    continue;

	/* put back age in host order */
	dbi.adv.rtr->ls_hdr.ls_age = newage;

	if (!(XAddLSA(&dbi.db, dbi.area, dbi.adv.rtr))) {
	    dbi.foundlsa = FALSE;
	    dbi.diff = 0;
	    /* 
	     * not found, if MaxAge remove from req lists and drop 
	     */
	    if (dbi.adv.rtr->ls_hdr.ls_age >= MaxAge) {
		if (!dbi.db) {
	    	    goto nobufs;
		}

		DBADV_ALLOC(dbi.db, dbi.len);

		/* 
		 * No memory - undo a couple of things... 
		 */
		if (!DB_NET(dbi.db)) {
		    db_free(dbi.db, dbi.adv.rtr->ls_hdr.ls_type);
		    goto nobufs;
		}

		ADV_COPY(dbi.adv.net, DB_NET(dbi.db), dbi.len);
		dbi.db->lsdb_time = time_sec;
		ADD_ACK(&acks, dbi.db);

		/*
		 * put on appropriate list for flooding and update new checksum sum
		 */
		if (LS_TYPE(dbi.db) < LS_ASE) {
		    /*
		     * Update area chksumsum for MIB
		     */
		    dbi.area->db_chksumsum += LS_CKS(dbi.db);
		} else {
		    ospf.db_chksumsum += LS_CKS(dbi.db);
		}

		if (dbi.nbr->state < NFULL) {	/* remove from req list */

		    /* Add to the free list */
		    DB_FREEME(dbi.db) = TRUE;
		    DB_ADDQUE(ospf.db_free_list, dbi.db);

		    nbr_rem_req(dbi.nbr, dbi.adv);
		    if (NO_REQ(dbi.nbr))
		  	(*(nbr_trans[LOAD_DONE][dbi.nbr->state]))(intf,dbi.nbr);
		} else {
		    db_free(dbi.db, dbi.adv.rtr->ls_hdr.ls_type);
		}
		continue;
	    }
	} else {
	    dbi.foundlsa = TRUE;
	    dbi.diff = time_sec - dbi.db->lsdb_time;	/* For the NEW AGE */
	}
	/* check for self originated lsa */
	if (dbi.adv.rtr->ls_hdr.adv_rtr == MY_ID) {
	    spfsched |= rx_self_orig(&dbi, &src_trans, &src_asetrans);
	} else {
	    spfsched |= not_my_lsa(&dbi, &trans, &asetrans);
	}

	if (BIT_TEST(spfsched, FLAG_NO_BUFS | FLAG_BAD_REQ)) {
	    break;
	}
    }

  nobufs:				/* have run out of buffers... */

    /*
     * Flood new self originated ls_ase
     */
    if (src_asetrans != LLNULL) {
	AREA_LIST(a) {
	    if (!BIT_TEST(a->area_flags, OSPF_AREAF_STUB)) {
		self_orig_area_flood(a, src_asetrans, LS_ASE);
	    }
	} AREA_LIST_END(a) ;

	ospf_freeq((struct Q **) &src_asetrans, ospf_lsdblist_index);
    }
    /*
     * Flood local to this area
     */
    if (trans != LLNULL) {
	area_flood(dbi.area,
		   trans,
		   intf,
		   dbi.nbr,
		   LS_RTR);
	ospf_freeq((struct Q **) &trans, ospf_lsdblist_index);
    }
    /*
     * Flood other than this RTR's LS_ASEs
     */
    if (asetrans != LLNULL) {
	AREA_LIST(a) {
	    if (!BIT_TEST(a->area_flags, OSPF_AREAF_STUB)) {
		area_flood(a,
			   asetrans,
			   intf,
			   dbi.nbr,
			   LS_ASE);
	    }
	} AREA_LIST_END(a) ;

	ospf_freeq((struct Q **) &asetrans, ospf_lsdblist_index);
    }
    /*
     * Flood (more recent) LSAs back to originator
     */
    if (rejects != LLNULL) {
	send_lsu(rejects,
		 1,
		 dbi.nbr,
		 dbi.intf,
		 0);
	ospf_freeq((struct Q **)rejects, ospf_lsdblist_index);
	rejects = LLNULL;
    }
    /*
     * load done event has occurred, build_rtr LSA and net LSA
     */
    if (dbi.area->build_rtr) {
	dbi.area->build_rtr = FALSE;
	spfsched |= build_rtr_lsa(dbi.area,
				  &src_trans,
				  (PTYPE_BIT(LS_RTR) & force_flood));
    }
    /*
     * Build net LSAs for intfs scheduled to do so while parsing this pkt
     */
    INTF_LIST(intf1, dbi.area) {
	if (BIT_TEST(intf1->flags, OSPF_INTFF_BUILDNET))
	    spfsched |= build_net_lsa(intf1,
				      &src_trans,
				      (PTYPE_BIT(LS_NET) & force_flood));
    } INTF_LIST_END(intf1, dbi.area) ;

    /*
     * flood new self-originated LSAs
     */
    if (src_trans != LLNULL) {
	self_orig_area_flood(dbi.area, src_trans, LS_NET);
	ospf_freeq((struct Q **) &src_trans, ospf_lsdblist_index);
    }

    /* Send direct ack */
    if (acks.ptr[NEXT]) {
	(void) send_ack(intf, dbi.nbr, &acks);
    }

    /* Send an ack packet if we have enough for a full one */
    if (!ACK_QUEUE_FULL(intf) || send_ack(intf, NBRNULL, &intf->acks)) {
	/* There is less than one full packet left, make sure the Ack timer is running */

	task_timer_set_interval(ospf.timer_ack, OSPF_T_ACK);
    }
    
    /*
     * Topology change in area or load done has occurred
     */
    dbi.area->spfsched |= (spfsched & ALLSCHED);

    /*
     * if any requests were acked, send new request pkt
     */
    if (dbi.nbr->state > NEXSTART && dbi.nbr->reqcnt < reqcnt)
	send_req(intf, dbi.nbr, 0);

    return GOOD_RX;
}
