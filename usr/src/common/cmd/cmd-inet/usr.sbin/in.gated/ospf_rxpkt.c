#ident	"@(#)ospf_rxpkt.c	1.3"
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


/* LS Ack packet */
static int
ospf_rx_lsack  __PF7(ack, struct LS_ACK_HDR *,
		     nbr, struct NBR *,
		     intf, struct INTF *,
		     src, sockaddr_un *,
		     dst, sockaddr_un *,
		     router_id, sockaddr_un *,
		     olen, size_t)
{
    struct LS_HDR *ap = &ack->ack_piece;
    struct LSDB *db = LSDBNULL;
    struct AREA *a, *area = intf->area;
    struct ospf_lsdb_list *trans = LLNULL, *asetrans = LLNULL;
    struct ospf_lsdb_list *lp;

    if (nbr->state < NEXCHANGE) {
	return OSPF_ERR_ACK_STATE;
    }

    /* list of db entries to be freed */
    for (olen -= OSPF_HDR_SIZE;
	 olen > 0;
	 olen -= ACK_PIECE_SIZE, ap++) {
	u_int diff;
	
	switch (ap->ls_type) {
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
	    OSPF_LOG_RX_LSA1(OSPF_ERR_ACK_TYPE,
			     intf,
			     src,
			     dst,
			     ap,
			     "	BAD TYPE",
			     ntohs(ap->ls_age));
	    continue;
	}
	if (!(db = FindLSA(area, ap->ls_id, ap->adv_rtr, ap->ls_type))) {
	    continue;
	}
	if (!db->lsdb_retrans
	    || !(lp = find_db_ptr(nbr, db))) {
	    continue;
	}
	diff = db->lsdb_time ? time_sec - db->lsdb_time : 0;
 	GNTOHS(ap->ls_age);
	if (SAME_INSTANCE(ap, &DB_RTR(db)->ls_hdr, diff)) {
	    if (lp) {	/* on retrans list */
		/* remove from lsdb */
		(void) rem_nbr_ptr(db, nbr);
		/* remove from nbr */
		REM_DB_PTR(nbr, lp);

		/* If MaxSeq and this is last ack, generate a new one */
		if (db->lsdb_seq_max == TRUE &&
		    db->lsdb_retrans == NLNULL &&
		    DB_FREEME(db) != TRUE)
		    area->spfsched |= beyond_max_seq(area,
						     intf,
						     db,
						     &trans,
						     &asetrans,
						     0);

		/*
		 * if MaxAge we have flooded this one - check for empty
		 * retrans list - if empty, remove from lsdb
		 */
		if (DB_CAN_BE_FREED(db)) {
		    db_free(db, LS_TYPE(db));
		}
	    }
	} else if (MORE_RECENT(ap, &DB_RTR(db)->ls_hdr, diff)) {
	    OSPF_LOG_RX_LSA2(OSPF_ERR_ACK_BAD,
			     intf,
			     src,
			     dst,
			     ap,
			     db);
	} else {
	    OSPF_LOG_RX_LSA2(OSPF_ERR_ACK_DUP,
			     intf,
			     src,
			     dst,
			     ap,
			     db);
	}
    }

    if (asetrans != LLNULL) {	/* new self originated ls_ase */
	AREA_LIST(a) {
	    if (!BIT_TEST(a->area_flags, OSPF_AREAF_STUB)) {
		self_orig_area_flood(a, asetrans, LS_ASE);
	    }
	} AREA_LIST_END(a) ;
	ospf_freeq((struct Q **)&asetrans, ospf_lsdblist_index);
    }
    if (trans != LLNULL) {
	self_orig_area_flood(area, trans, LS_RTR);
	ospf_freeq((struct Q **)&trans, ospf_lsdblist_index);
    }

    return GOOD_RX;
}

/**/

/* Database Description packet */

/*
 * parse packet and put on the nbrs link state req list
 */
static int
db_parse __PF6(dbh, struct DB_HDR *,
	       nbr, struct NBR *,
	       intf, struct INTF *,
	       src, sockaddr_un *,
	       dst, sockaddr_un *,
	       len, size_t)
{
    struct LS_HDR *dbp;
    struct LSDB *db;
    struct AREA *a = intf->area;
    struct LS_REQ *lr;
    u_int diff;		/* to calculate db elapsed time */

    dbp = &dbh->dbp;

    for (; len; len -= DB_PIECE_SIZE) {
	/* If we don't have a copy or if nbrs info is newer */

	switch (dbp->ls_type) {
	case LS_RTR:
	case LS_NET:
	case LS_SUM_NET:
	case LS_SUM_ASB:
	case LS_ASE:
	    /* OK */
	    break;

	case LS_GM:
	case LS_NSSA:
	    /* Fall through */

	default:
	    OSPF_LOG_RX_LSA1(OSPF_ERR_DD_TYPE,
			     intf,
			     src,
			     dst,
			     dbp,
			     "	BAD TYPE",
			     ntohs(dbp->ls_age));
	    (*(nbr_trans[SEQ_MISMATCH][nbr->state])) (intf, nbr);
	    return 1;
	}
	if ((db = FindLSA(a, dbp->ls_id, dbp->adv_rtr, dbp->ls_type))) {
	    diff = time_sec - db->lsdb_time;
	} else {
	    diff = 0;
	}
	dbp->ls_age = ntohs(dbp->ls_age);
	if (!db
	    || MORE_RECENT(dbp, &DB_RTR(db)->ls_hdr, diff)) {
	    /* piece is newer or we don't have it - add to list */
	    lr = (struct LS_REQ *) task_block_alloc(ospf_lsreq_index);
	    lr->ls_id = dbp->ls_id;
	    lr->adv_rtr = dbp->adv_rtr;
	    lr->ls_age = htons(dbp->ls_age);
	    lr->ls_seq = dbp->ls_seq;
	    lr->ls_chksum = dbp->ls_chksum;
	    EN_Q((nbr->ls_req[dbp->ls_type]), lr);
	    nbr->reqcnt++;
	}
	dbp++;
    }
    return FLAG_NO_PROBLEM;
}


static int
ospf_rx_db  __PF7(dbh, struct DB_HDR *,
		  nbr, struct NBR *,
		  intf, struct INTF *,
		  src, sockaddr_un *,
		  dst, sockaddr_un *,
		  router_id, sockaddr_un *,
		  olen, size_t)
{
    int	sched = 0;
    int ret = GOOD_RX;
    int reqcnt;
    struct DB_HDR *d;
    struct LSDB_SUM *nextds;
    struct ospf_lsdb_list *txq = LLNULL;
    struct AREA *area = intf->area;	/* area received from */

    if (BIT_MATCH(dbh->options, OPT_E_bit) == BIT_MATCH(area->area_flags, OSPF_AREAF_STUB))
	return OSPF_ERR_DD_E;

    olen -= (OSPF_HDR_SIZE + DB_HDR_SIZE);
    GNTOHL(dbh->seq);
    reqcnt = nbr->reqcnt;

    switch (nbr->state) {
    case NDOWN:
    case NATTEMPT:
	return OSPF_ERR_DD_STATE;

    case N2WAY:
	return GOOD_RX;

    case NINIT:
	(*(nbr_trans[TWOWAY][nbr->state])) (intf, nbr);
	/* if nbr didn't make it to NEXTSTART chuckit */
	if (nbr->state != NEXSTART)
	    goto we_are_through;
	send_dbsum(intf, nbr, NOT_RETRANS);
	/* if forming adjacency continue processing */

    case NEXSTART:
	/* if first exchange pkt and this nbr's id is > than ours  */
	if ((BIT_MATCH(dbh->I_M_MS, bit_I | bit_M | bit_MS) && !olen) &&
	    ntohl(NBR_ID(nbr)) > ntohl(MY_ID)) {
	    /* We are now in SLAVE mode */

	    nbr->mode = SLAVE;
	    nbr->I_M_MS = 0;
	    nbr->seq = dbh->seq;
	    (*(nbr_trans[NEGO_DONE][nbr->state])) (intf, nbr);
	    /* did we make it to NEXCHANGE? (out of bufs?) */
	    if (nbr->state != NEXCHANGE) {
		nbr->I_M_MS = (bit_I | bit_M | bit_MS);
		nbr->mode = 0;
		goto we_are_through;
	    }
	    /* Slave sends in response */
	    send_dbsum(intf, nbr, NOT_RETRANS);
	} else if (!BIT_MATCH(dbh->I_M_MS, bit_I | bit_MS) &&
		   (nbr->seq == dbh->seq) &&
		   (ntohl(NBR_ID(nbr)) < ntohl(MY_ID))) {
	    /* check to see if it was an ack (we are master) */
	    nbr->mode = MASTER;
	    nbr->I_M_MS = bit_MS;	/* set to master */
	    (*(nbr_trans[NEGO_DONE][nbr->state])) (intf, nbr);
	    /* did we make it to NEXCHANGE? (out of bufs?) */
	    if (nbr->state != NEXCHANGE) {
		nbr->I_M_MS = (bit_I | bit_M | bit_MS);
		nbr->mode = 0;
		goto we_are_through;
	    }
	    if (olen) {
		if (db_parse(dbh, nbr, intf, src, dst, olen))	/* out of bufs */
		    goto we_are_through;
	    }
	    nbr->seq++;
	    send_dbsum(intf, nbr, NOT_RETRANS);
	} else if (NBR_ID(nbr) == MY_ID) {
	    ret = OSPF_ERR_DD_RTRID;
	    goto we_are_through;
	}
	break;

    case NEXCHANGE:
	/* check for master mismatch */
	if ((BIT_MATCH(dbh->I_M_MS, bit_MS) == (nbr->mode == MASTER)) ||
	    BIT_TEST(dbh->I_M_MS, bit_I)) {
	    (*(nbr_trans[SEQ_MISMATCH][nbr->state])) (intf, nbr);
	    goto we_are_through;
	}
	if (nbr->mode == MASTER) {

	    /* Validate sequence number */
	    switch (nbr->seq - dbh->seq) {
	    case 0:
		/* Expected sequence number */
		break;

	    case 1:
		/* Duplicate - ignore */
		goto we_are_through;

	    default:
		/* Sequence mis-match */
		(*(nbr_trans[SEQ_MISMATCH][nbr->state])) (intf, nbr);
		goto we_are_through;
	    }
	    
	    if (nbr->dbsum == LSDB_SUM_NULL)
		d = (struct DB_HDR *) 0;
	    else
		d = &nbr->dbsum->dbpkt->ospfh_un.database;

	    /* if it's SLAVE's last and my last */
	    nbr->seq++;
	    /* parse packet and put on the nbrs link state req list */
	    if (olen) {
		if (db_parse(dbh, nbr, intf, src, dst, olen))
		    goto we_are_through;
	    }
	    if (!BIT_TEST(dbh->I_M_MS, bit_M) &&	/* slave's last */
		(d != (struct DB_HDR *) 0) &&	/* pkts left */
		!BIT_TEST(d->I_M_MS, bit_M)) {
		/* then, we're up to loading state */
		(*(nbr_trans[EXCH_DONE][nbr->state])) (intf, nbr);
		goto we_are_through;
	    }
	    /* free head of list f more bit was set */
	    if ((d != (struct DB_HDR *) 0) &&
		BIT_TEST(d->I_M_MS, bit_M)) {
		/* One less dbsum pkt */
		nbr->dbcnt -= nbr->dbsum->cnt;
		nextds = nbr->dbsum->next;
		dbsum_free(nbr->dbsum);
		nbr->dbsum = nextds;
	    }
	    send_dbsum(intf, nbr, NOT_RETRANS);
	} else {		/* we are in SLAVE mode  */

	    /* Validate sequence number */
	    switch (dbh->seq - nbr->seq) {
	    case 1:
		/* Expected sequence number */
		break;

	    case 0:
		/* Duplicate - resent last packet */
		send_dbsum(intf, nbr, NOT_RETRANS);
		goto we_are_through;

	    default:
		/* Sequence mis-match */
		(*(nbr_trans[SEQ_MISMATCH][nbr->state])) (intf, nbr);
		goto we_are_through;
	    }
	    
	    nbr->seq = dbh->seq;

	    /* remove last sent */
	    if (nbr->dbsum != (struct LSDB_SUM *) 0) {
		/* One less dbsum pkt */
		nbr->dbcnt -= nbr->dbsum->cnt;
		nextds = nbr->dbsum->next;
		dbsum_free(nbr->dbsum);
		nbr->dbsum = nextds;
	    }
	    if (nbr->dbsum != (struct LSDB_SUM *) 0)
		d = &nbr->dbsum->dbpkt->ospfh_un.database;
	    else
		d = (struct DB_HDR *) NULL;

	    /* if it's MASTER's last and my last */
	    if (!BIT_TEST(dbh->I_M_MS, bit_M) &&	/* master's last */
		((d == (struct DB_HDR *) NULL) ||	/* no pkts left */
		 !BIT_TEST(d->I_M_MS, bit_M)))	/* my more bit is off */
		sched++;	/* schedule an exchange done event */

	    if (db_parse(dbh, nbr, intf, src, dst, olen))	/* run out of bufs */
		goto we_are_through;
	    send_dbsum(intf, nbr, NOT_RETRANS);

	    if (sched) {	/* up to loading state */
		(*(nbr_trans[EXCH_DONE][nbr->state])) (intf, nbr);
	    }
	}
	break;

    case NLOADING:
    case NFULL:
	/* check for master mismatch */
	if((BIT_MATCH(dbh->I_M_MS, bit_MS) == (nbr->mode == MASTER)) ||
	BIT_TEST(dbh->I_M_MS, bit_I)) {
		(*(nbr_trans[SEQ_MISMATCH][nbr->state]))(intf,nbr);
		goto we_are_through;
	}
	if (nbr->mode == SLAVE_HOLD && nbr->seq == dbh->seq)
	    /* haven't timed out yet */
	    send_dbsum(intf, nbr, NOT_RETRANS);
	else if ((nbr->mode == MASTER && nbr->seq - 1 == dbh->seq) ||
		 (nbr->mode == SLAVE && nbr->seq == dbh->seq))
	    goto we_are_through;
	else {
	    if (nbr->state == NFULL) {
		trace_log_tf(ospf.trace_options,
			     0,
			     LOG_WARNING,
			     ("Lost %s Neighbor %A with address %A due to sequence mismatch (%d versus %d).",
			      ((nbr->mode == MASTER) ? "Master" : "Slave"),
			      nbr->nbr_id, 
			      nbr->nbr_addr,
			      ((nbr->mode == MASTER) ? (nbr->seq-1) : nbr->seq),
			      dbh->seq));
	    }
	    (*(nbr_trans[SEQ_MISMATCH][nbr->state])) (intf, nbr);
	}

	break;
    }

  we_are_through:
    if (BIT_TEST(intf->flags, OSPF_INTFF_NBR_CHANGE)) {
	(*(if_trans[NBR_CHANGE][intf->state])) (intf);
    }

    /* build net and rtr lsa if necessary */
    if (BIT_TEST(intf->flags, OSPF_INTFF_BUILDNET)) {
	area->spfsched |= build_net_lsa(intf, &txq, 0);
	BIT_RESET(intf->flags, OSPF_INTFF_BUILDNET);
    }
    if (area->build_rtr) {
	area->spfsched |= build_rtr_lsa(area, &txq, 0);
	area->build_rtr = FALSE;
    }
    if (txq != LLNULL) {	/* may be locked out */
	self_orig_area_flood(area, txq, LS_RTR);
	ospf_freeq((struct Q **)&txq, ospf_lsdblist_index);
    }

    /* Can we send req pkt? */
    if ((nbr->state >= NEXCHANGE) &&
	(reqcnt == 0) &&
	(nbr->reqcnt > 0))
	send_req(intf, nbr, 0);

    return ret;
}


/**/

/* Hello packet */


static int
ospf_rx_hello  __PF8(hello, struct HELLO_HDR *,
		     newnbr, struct NBR *,
		     intf, struct INTF *,
		     src, sockaddr_un *,
		     dst, sockaddr_un *,
		     router_id, sockaddr_un *,
		     olen, size_t,
		     sequence, u_int32)
{
    int 	nbr_is_dr = FALSE, nbr_is_bdr = FALSE;
    int		rtr_heard_from = FALSE;
    int		backup_seen = FALSE;
    byte *	limit = (byte *) hello + olen - OSPF_HDR_SIZE;
    struct 	RHF *rhf;
    struct 	ospf_lsdb_list *txq = LLNULL;
    struct 	AREA *area = intf->area;	/* area received from */

    if (intf->state == IDOWN) {
	/* wait until virtual nbr is brought up by trans area running SPF */
	if (intf->type == VIRTUAL_LINK)
	    return OSPF_ERR_HELLO_VIRT;
	ospf_ifup(intf);
	return OSPF_ERR_OSPF_IFDOWN;
    }

    /* 
     * a few validity checks 
     */
    if (intf->type != VIRTUAL_LINK
	&& intf->type != POINT_TO_POINT
	&& hello->netmask != INTF_MASK(intf)) {
	return OSPF_ERR_HELLO_MASK;
    }
    if (ntohs(hello->HelloInt) != intf->hello_timer) {
	return OSPF_ERR_HELLO_TIMER;
    }
    if (ntohl(hello->DeadInt) != intf->dead_timer) {
	return OSPF_ERR_HELLO_DEAD;
    }
    if (BIT_MATCH(hello->options, OPT_E_bit) == BIT_MATCH(area->area_flags, OSPF_AREAF_STUB)) {
	return OSPF_ERR_HELLO_E;
    }
    if (sockaddrcmp_in(router_id, ospf.router_id)) {
	return OSPF_ERR_HELLO_ID;
    }

    switch (intf->type) {
    case POINT_TO_POINT:
	if (!newnbr) {
	    newnbr = &intf->nbr;
	    assert(sockaddrcmp_in(src, newnbr->nbr_addr));
#ifdef	notdef
	    if (!sockaddrcmp_in(src, newnbr->nbr_addr)) {
		if (newnbr->nbr_addr) {
		    sockfree(newnbr->nbr_addr);
		}
		newnbr->nbr_addr = sockdup(src);
	    }
#endif	/* notdef */
	    newnbr->nbr_sequence = sequence;
	    newnbr->intf = intf;
	    newnbr->pri = hello->rtr_priority;
	    if (newnbr->nbr_id) {
		sockfree(newnbr->nbr_id);
	    }
	    newnbr->nbr_id = sockdup(router_id);
	}
	break;

    case VIRTUAL_LINK:
	assert(newnbr);		/* Neighbor is found when we find virtual link */
	break;
	
    case NONBROADCAST:
    case BROADCAST:
	if (newnbr) {
	    /* Update neighbor ID */

	    if (!newnbr->nbr_id
		|| !sockaddrcmp_in(newnbr->nbr_id, router_id)) {
		if (newnbr->nbr_id) {
		    sockfree(newnbr->nbr_id);
		}
		newnbr->nbr_id = sockdup(router_id);
	    }
	} else {
	    /* Allocate a new neighbor */

	    if (intf->type == NONBROADCAST && intf->pri) {
		/* NBMA DR elig nbrs should be configured */

		OSPF_LOG_RX(OSPF_ERR_HELLO_NBMA,
			    intf,
			    src,
			    dst);
	    }

	    newnbr = (struct NBR *) task_block_alloc(ospf_nbr_index);
	    newnbr->nbr_id = sockdup(router_id);
	    newnbr->nbr_addr = sockdup(src);
	    newnbr->nbr_sequence = sequence;
	    newnbr->intf = intf;
	    newnbr->pri = hello->rtr_priority;
	    ospf_nbr_add(intf,newnbr);
	}
	break;
    }

    reset_inact_tmr(newnbr);		/* if not set don't set */

    /* 
     * have already reset rtr dead timer or will start in NHello 
     */
    switch (intf->type) {
    case VIRTUAL_LINK:
    case NONBROADCAST:
	switch (newnbr->state) {
	case NATTEMPT:
	case NDOWN:
	    (*(nbr_trans[HELLO_RX][newnbr->state])) (intf, newnbr);
	    break;

	default:
	    break;
	}
	break;

    default:
	if (newnbr->state == NDOWN) {
	    (*(nbr_trans[HELLO_RX][newnbr->state])) (intf, newnbr);
	}
	break;
    }

    /* 
     * see if my router_id appears in the router-heard-from list 
     */
    for (rhf = &hello->rhf; (byte *) rhf < limit; rhf++) {
	if (rhf->rtr == MY_ID) {
	    if (newnbr->state < N2WAY) {
		(*(nbr_trans[TWOWAY][newnbr->state])) (intf, newnbr);
	    }
	    rtr_heard_from++; 		/* flag that we found it */
	    break;
	}
    }

    /* drop back to one way until we can see ourself in this nbr's hello */
    if (!rtr_heard_from) {
	if (newnbr->state == NFULL) {
	    trace_log_tf(ospf.trace_options,
			 0,
			 LOG_WARNING,
			 ("Lost Neighbor %A with address %A due to HELLO received without my ID.",
			  newnbr->nbr_id, 
			  newnbr->nbr_addr));
	}
	(*(nbr_trans[ONEWAY][newnbr->state])) (intf, newnbr);
    } else if (intf->type <= NONBROADCAST) {

	if (hello->dr == NBR_ADDR(newnbr))
	    nbr_is_dr++;

	if (hello->bdr == NBR_ADDR(newnbr))
	    nbr_is_bdr++;

	/* 
	 * if this nbr is bdr or it's dr and there is no bdr 
	 */
	if (intf->state == IWAITING
	    && ((nbr_is_dr && !hello->bdr)
		|| nbr_is_bdr)) {
		newnbr->dr = hello->dr;
                newnbr->bdr = hello->bdr;
		backup_seen = TRUE;
	} else if (hello->rtr_priority != newnbr->pri) /* priority change? */ {
	    newnbr->pri = hello->rtr_priority;
	    if (intf->state >= IDr)
		BIT_SET(intf->flags, OSPF_INTFF_NBR_CHANGE);	/* schedule nbr change */
    	} else {	/* dr/bdr changeola? */
    	    if (hello->dr != newnbr->dr
		&& (newnbr->dr == NBR_ADDR(newnbr)
		    || nbr_is_dr)) {
		BIT_SET(intf->flags, OSPF_INTFF_NBR_CHANGE);
    	    } else if (hello->bdr != newnbr->bdr
		       && (newnbr->bdr == NBR_ADDR(newnbr)
			   || nbr_is_bdr)) {
		BIT_SET(intf->flags, OSPF_INTFF_NBR_CHANGE);
	    }
	}

	newnbr->dr = hello->dr;
	newnbr->bdr = hello->bdr;
    }


    /* If not elig send hello to elig nbr if nbr isn't dr or bdr */
    if (intf->type == NONBROADCAST
	&& !intf->pri
	&& newnbr->pri
	&& intf->dr != newnbr
	&& intf->bdr != newnbr) {

	send_hello(intf, newnbr, FALSE);
    }

    if (backup_seen) {
	(*(if_trans[BACKUP_SEEN][intf->state])) (intf);
    } else if (BIT_TEST(intf->flags, OSPF_INTFF_NBR_CHANGE)) {
	(*(if_trans[NBR_CHANGE][intf->state])) (intf);
    }

    /* build net and rtr lsa if necessary */
    if (BIT_TEST(intf->flags, OSPF_INTFF_BUILDNET)) {
	area->spfsched |= build_net_lsa(intf, &txq, 0);
	BIT_RESET(intf->flags, OSPF_INTFF_BUILDNET);
    }
    if (area->build_rtr) {
	area->spfsched |= build_rtr_lsa(area, &txq, 0);
	area->build_rtr = FALSE;
    }
    if (txq != LLNULL) {		/* may be locked out */
	self_orig_area_flood(area, txq, LS_RTR);
	ospf_freeq((struct Q **) &txq, ospf_lsdblist_index);
    }

    return GOOD_RX;
}


/**/

/* LS Request */


/*
 * ospf_rx_lsreq - receive an ls request
 *	   - while parsing build and send ls pkt
 */
/* ospf_rxreq.c */
static int
ospf_rx_lsreq  __PF7(ls_req, struct LS_REQ_HDR *,
		     nbr, struct NBR *,
		     intf, struct INTF *,
		     src, sockaddr_un *,
		     dst, sockaddr_un *,
		     router_id, sockaddr_un *,
		     olen, size_t)
{
    struct OSPF_HDR *pkt;
    struct LS_REQ_PIECE *req = &ls_req->req;
    struct AREA *a = intf->area;
    struct LSDB *db;
    struct ospf_lsdb_list *txq = LLNULL;
    u_int32 *adv_cnt;
    union ADV *adv;
    size_t len = OSPF_HDR_SIZE + 4;/* max len of pkt is INTF_MTU */
    size_t intf_mtu = INTF_MTU(intf);
    int ret = GOOD_RX;

    if (nbr->state < NEXCHANGE) {
	return OSPF_ERR_REQ_STATE;
    }

    olen -= OSPF_HDR_SIZE;
    if (!olen)
	return OSPF_ERR_REQ_EMPTY;

    NEW_LSU(pkt, adv_cnt, adv);
    /* Out of pkt buffers? */
    if (!pkt)
	return GOOD_RX;

    for (; olen; olen -= LS_REQ_PIECE_SIZE) {
	if ((req->ls_type < LS_RTR || req->ls_type > LS_ASE) ||
	    !(db = FindLSA(a, req->ls_id, req->adv_rtr, req->ls_type))) {
	    if (nbr->state == NFULL) {
		trace_log_tf(ospf.trace_options,
			     0,
			     LOG_WARNING,
			     ("Lost Neighbor %A with address %A due to bad LS Request (%A %A %d).",
			      nbr->nbr_id, 
			      nbr->nbr_addr,
			      sockbuild_in(0, req->ls_id),
			      sockbuild_in(0, req->adv_rtr),
			      req->ls_type));
	    }
	    (*(nbr_trans[BAD_LS_REQ][nbr->state])) (intf, nbr);
	    ret = OSPF_ERR_REQ_BOGUS;
	    goto we_are_through;
	}
	if ((len + ntohs(LS_LEN(db))) > intf_mtu) {
	    GHTONL(*adv_cnt);
	    ospf_txpkt(pkt, intf, OSPF_PKT_LSU, len, NBR_ADDR(nbr), NOT_RETRANS);
	    len = OSPF_HDR_SIZE + 4;	/* 4 for adv_cnt */
	    NEW_LSU(pkt, adv_cnt, adv);
	}
	(*adv_cnt)++;
	ADV_COPY(DB_RTR(db), adv, ntohs(LS_LEN(db)));
	adv->rtr.ls_hdr.ls_age = htons((u_int16) MIN(ADV_AGE(db) + intf->transdly, MaxAge));
	adv = (union ADV *) ((long) adv + ntohs(LS_LEN(db)));
	len += ntohs(LS_LEN(db));
	req++;
    }

    if (*adv_cnt) {		/* any left to send? */
	GHTONL(*adv_cnt);
	ospf_txpkt(pkt, intf, OSPF_PKT_LSU, len, NBR_ADDR(nbr), FALSE);
    }

  we_are_through:

    if (BIT_TEST(intf->flags, OSPF_INTFF_NBR_CHANGE)) {
	(*(if_trans[NBR_CHANGE][intf->state])) (intf);
    }

    /* build net and rtr lsa if necessary */
    if (BIT_TEST(intf->flags, OSPF_INTFF_BUILDNET)) {
	a->spfsched |= build_net_lsa(intf, &txq, 0);
	BIT_RESET(intf->flags, OSPF_INTFF_BUILDNET);
    }
    if (a->build_rtr) {
	a->spfsched |= build_rtr_lsa(a, &txq, 0);
	a->build_rtr = FALSE;
    }
    if (txq != LLNULL) {		/* may be locked out */
	self_orig_area_flood(a, txq, LS_RTR);
	ospf_freeq((struct Q **) &txq, ospf_lsdblist_index);
    }

    return ret;
}


/**/

void
ospf_rxpkt __PF4(ip, struct ip *,
		 o_hdr, struct OSPF_HDR *,
		 src, sockaddr_un *,
		 dst, sockaddr_un *)
{
    size_t len;
    int ret = 0;
    ospf_auth *oaps[3], **oapp = oaps, *oap;
    u_int32 sequence;
    sockaddr_un *router_id = sockbuild_in(0, o_hdr->ospfh_rtr_id);
    struct INTF *intf;
    struct NBR *nbr = (struct NBR *) 0;
#define	bad_packet(x)	{ ret = x; goto Return; }

    /* Log current time */
    ospf_get_sys_time();

    TRAP_REF_UPDATE;	/* Update the trap event counter */


    if (o_hdr->ospfh_type == OSPF_PKT_MON) {
	/* Monitor packet */

	intf = (struct INTF *) 0;
    } else {
	if_addr *ifap = if_withdst(src);

	/* check that IP destination is to interface or IP multicast addr */

	intf = ifap ? IF_INTF(ifap) : (struct INTF *) 0;

#ifdef	IP_MULTICAST
	if (sockaddrcmp_in(dst, ospf_addr_allspf)) {
	    /* Multicast to All SPF routers */

	    if (!intf) {
		/* Not on attached interface running OSPF */
		bad_packet(OSPF_ERR_IP_DEST);
	    }

	    switch (intf->type) {
	    case POINT_TO_POINT:
	    case BROADCAST:
		break;

	    default:
		bad_packet(OSPF_ERR_IP_DEST);
	    }
	} else if (sockaddrcmp_in(dst, ospf_addr_alldr)) {
	    /* Multicast to All DR routers */

	    if (!intf
		|| !BIT_TEST(ifap->ifa_ps[ospf.task->task_rtproto].ips_state, OSPF_IFPS_ALLDR)) {
		/* Not on an attached OSPF interface with group reception enabled */

		bad_packet(OSPF_ERR_IP_DEST);
	    }

	    switch (intf->state) {
	    case IDr:
	    case IBACKUP:
		if (intf->type == BROADCAST && intf->pri) {
		    break;
		}
		/* Fall through */

	    default:
		bad_packet(OSPF_ERR_IP_DEST);
	    }
	} else {
#endif	/* IP_MULTICAST */
	    /* Unicast packet */
	
	    if (intf) {
		/* Local source */

		if (!sockaddrcmp_in(ifap->ifa_addr_local, dst)) {
		    /* Not addressed directly to us */
		    bad_packet(OSPF_ERR_IP_DEST);
		}
	    } else if (IAmBorderRtr) {
		/* Non-local source, could be a virtual link */

		if (!(ifap = if_withlcladdr(dst, FALSE))
		    || !(intf = IF_INTF(ifap))) {
		    /* Not addressed to us */
		    bad_packet(OSPF_ERR_IP_DEST);
		}
	    } else {
		/* Non-local source, should not be a virtual link */

		bad_packet(OSPF_ERR_IP_DEST);
	    }
#ifdef	IP_MULTICAST
	}
#endif	/* IP_MULTICAST */

	if (sockaddrcmp_in(src, ifap->ifa_addr_local)) {
	    /* Echo of my own multicast */
	    bad_packet(OSPF_ERR_IP_ECHO);
	}

	/* Check for a down to up transition of the interface */
	if_rtupdate(ifap);

    }

    if (IP_PROTOCOL(ip) != ospf.task->task_proto) {
	bad_packet(OSPF_ERR_IP_PROTO);
    }

    len = ntohs(o_hdr->ospfh_length);
    if (len < OSPF_HDR_SIZE) {
	bad_packet(OSPF_ERR_OSPF_SHORT);
    }
    if (len > IP_LENGTH(ip)) {
	bad_packet(OSPF_ERR_OSPF_LONG);
    }

    /* OSPF hdr */
    if (o_hdr->ospfh_version != OSPF_VERSION) {
	bad_packet(OSPF_ERR_OSPF_VERSION);
    }

    /* Checksum the packet, exclusive of the authentication field */
    if (ntohs(o_hdr->ospfh_auth_type) == OSPF_AUTH_MD5) {
	/* With MD5 the checksum field must be zero */
	
	if (o_hdr->ospfh_checksum) {
	    bad_packet(OSPF_ERR_OSPF_CHKSUM);
	}
    } else {
	struct iovec v[2], *vp = v;

	/* Verify the INET checksum */
	
	/* First the packet header not including the authentication */
	vp->iov_base = (caddr_t) o_hdr;
	vp->iov_len = OSPF_HDR_SIZE - OSPF_AUTH_SIMPLE_SIZE;
	vp++;

	if (len > OSPF_HDR_SIZE) {
	    /* Then the rest of the packet */

	    vp->iov_base = (caddr_t) o_hdr->ospfh_auth_key + OSPF_AUTH_SIMPLE_SIZE;
	    vp->iov_len = len - OSPF_HDR_SIZE;
	    vp++;
	} else {
	    /* Just a header */

	    vp->iov_base = (caddr_t) 0;
	    vp->iov_base = 0;
	}

	if (inet_cksumv(v, vp - v, len - OSPF_AUTH_SIMPLE_SIZE)) {
	    bad_packet(OSPF_ERR_OSPF_CHKSUM);
	}
    }

    if (o_hdr->ospfh_type == OSPF_PKT_MON) {
	*oapp++ = &ospf.mon_auth;
	nbr = (struct NBR *) 0;
    } else {
	/* the area id check stuff - VIRTUAL or Standard link? */
	if (intf
	    && o_hdr->ospfh_area_id == intf->area->area_id) {
	    /* NOT virtual */

	    if ((sock2ip(src) & INTF_MASK(intf)) != (INTF_ADDR(intf) & INTF_MASK(intf)))
		bad_packet(OSPF_ERR_OSPF_AREAID);

	    /* Find the neighbor */
	    switch (intf->type) {
	    case BROADCAST:
	    case NONBROADCAST:
		/* Look up neighbor in tree */
	    
		OSPF_NBR_LOOKUP(nbr, intf, src);
		break;

	    case POINT_TO_POINT:
		nbr = FirstNbr(intf);
		if (!nbr->nbr_id
		    || !sockaddrcmp_in(nbr->nbr_id, router_id)) {
		    /* Unknown router ID, or unknown address */

		    nbr = (struct NBR *) 0;
		}
		break;

	    default:
		assert(FALSE);
	    }

	    if (!nbr && o_hdr->ospfh_type != OSPF_PKT_HELLO) {
		/* Unknown neighbor */
	    
		if (OSPF_LOG_TIME(intf)) {
		    bad_packet(OSPF_ERR_OSPF_NBR);
		} else {
		    return;
		}
	    }
	} else {
	    struct INTF *vintf;

	    /* pkt should be from virtual link */

	    if (!IAmBorderRtr)
		bad_packet(OSPF_ERR_OSPF_ABR);

	    VINTF_LIST(vintf) {
		/*
		 * if VL 1) should be from configured nbr 2) nbr is area border
		 * rtr 3) trans area is same as receiving IF's area
		 */
		if (sockaddrcmp_in(vintf->nbr.nbr_id, router_id) &&
		    o_hdr->ospfh_area_id == OSPF_BACKBONE &&	/* from backbone */
		    vintf->trans_area == intf->area) {
		    intf = vintf;
		    nbr = &intf->nbr;
		    goto found_intf;
		}
	    } VINTF_LIST_END(vintf) ;

	    /* Not found */
	    bad_packet(OSPF_ERR_OSPF_VL);

	found_intf: ;
	}

	*oapp++ = &intf->auth;
	if (BIT_TEST(intf->flags, OSPF_INTFF_SECAUTH)) {
	    *oapp++ = &intf->auth2;
	}
    }

    *oapp = (ospf_auth *) 0;

    {
	size_t md5_done = 0;
	u_int32 md5_temp[OSPF_AUTH_SIZE];
	
	for (oapp = oaps; (oap = *oapp); oapp++ ) {
	
	    /* Check the authentication */
	    if (ntohs(o_hdr->ospfh_auth_type) != oap->auth_type) {
		continue;
	    }

	    switch (oap->auth_type) {
	    case OSPF_AUTH_NONE:
		sequence = (u_int32) 0;
		goto auth_ok;

	    case OSPF_AUTH_SIMPLE:
		if (o_hdr->ospfh_auth_key[0] == oap->auth_key[0]
		    && o_hdr->ospfh_auth_key[1] == oap->auth_key[1]) {
		    sequence = (u_int32) 0;
		    goto auth_ok;
		}
		break;

	    case OSPF_AUTH_MD5:
		if (htons(o_hdr->ospfh_md5_offset) + OSPF_AUTH_MD5_SIZE <= (size_t) IP_LENGTH(ip)
		    && !o_hdr->ospfh_md5_instance) {
		    u_int32 *dp = (u_int32 *) ((void_t) ((byte *) o_hdr + ntohs(o_hdr->ospfh_md5_offset)));
		    u_int32 digest[OSPF_AUTH_SIZE];

		    /* Save the digest */
		    digest[0] = dp[0]; digest[1] = dp[1]; digest[2] = dp[2]; digest[3] = dp[3];

		    /* Checksum the packet up to the digest */
		    if (!md5_done) {
			md5_done = md5_cksum_partial(o_hdr,
						     dp,
						     TRUE,
						     md5_temp);
		    }

		    /* Put the secret in the packet */
		    dp[0] = oap->auth_key[0]; dp[1] = oap->auth_key[1];
		    dp[2] = oap->auth_key[2]; dp[3] = oap->auth_key[3];

		    md5_cksum((byte *) o_hdr + md5_done, 
			      len + OSPF_AUTH_MD5_SIZE - md5_done,
			      len + OSPF_AUTH_MD5_SIZE,
			      dp,
			      md5_temp);

		    if (digest[0] == dp[0]
			&& digest[1] == dp[1]
			&& digest[2] == dp[2]
			&& digest[3] == dp[3]) {
			/* Verify that the sequence is not less than and save the */
			/* new sequence number */

			/* XXX - Handle wrap? */
			sequence = ntohl(o_hdr->ospfh_md5_sequence);
			if (nbr) {
			    if (sequence < nbr->nbr_sequence) {
				/* Sequence not valid */
				
				continue;
			    }
			    nbr->nbr_sequence = sequence;
			}
			goto auth_ok;
		    }
		}
		break;
	
	    default:
		bad_packet(OSPF_ERR_OSPF_AUTH_TYPE);
	    }
	}

	/* Search of keys has failed */
	bad_packet(OSPF_ERR_OSPF_AUTH_KEY);

    auth_ok: ;
    }
    
    /* Log the packet */
    if (TRACE_PACKET_RECV(ospf.trace_options,
			  o_hdr->ospfh_type,
			  OSPF_PKT_MAX,
			  ospf_trace_masks)) {
	ospf_trace(o_hdr,
		   len,
		   (u_int) o_hdr->ospfh_type,
		   TRUE,
		   intf,
		   src,
		   dst,
		   TRACE_DETAIL_RECV(ospf.trace_options,
				     o_hdr->ospfh_type,
				     OSPF_PKT_MAX,
				     ospf_trace_masks));
    }

    /* Handle the packet */
    switch (o_hdr->ospfh_type) {
    case OSPF_PKT_MON:
	ret = ospf_rx_mon(&o_hdr->ospfh_un.mon, intf, src, router_id, len);
	break;

    case OSPF_PKT_HELLO:
	ret = ospf_rx_hello(&o_hdr->ospfh_un.hello,
			    nbr,
			    intf,
			    src,
			    dst,
			    router_id,
			    len,
			    sequence);
	break;
	    
    case OSPF_PKT_DD:
	ret = ospf_rx_db(&o_hdr->ospfh_un.database, nbr, intf, src, dst, router_id, len);
	break;
	
    case OSPF_PKT_LSR:
	ret = ospf_rx_lsreq(&o_hdr->ospfh_un.ls_req, nbr, intf, src, dst, router_id, len);
	break;
	
    case OSPF_PKT_LSU:
	ret = ospf_rx_lsupdate(&o_hdr->ospfh_un.ls_update, nbr, intf, src, dst, router_id, len);
	break;

    case OSPF_PKT_ACK:
	ret = ospf_rx_lsack(&o_hdr->ospfh_un.ls_ack, nbr, intf, src, dst, router_id, len);
	break;

    default:
	bad_packet(OSPF_ERR_OSPF_TYPE);
    }
	
    if (ret == GOOD_RX) {
	ret = o_hdr->ospfh_type;
    }

 Return:
    OSPF_LOG_RX(ret, intf, src, dst);

    if (ospf_log_last_lsa) {
	ospf_log_last_lsa = 0;
	trace_only_tf(ospf.trace_options,
		      0,
		      (NULL));
    }
}
