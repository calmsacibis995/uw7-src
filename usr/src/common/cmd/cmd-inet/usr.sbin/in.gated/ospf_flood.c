#ident	"@(#)ospf_flood.c	1.3"
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
 *	Point-to-point interface flood routine
 */
static void
ptp_flood __PF4(ll, struct ospf_lsdb_list *,
		intf, struct INTF *,
		src_intf, struct INTF *,
		src_nbr, struct NBR *)
{
    struct NBR *n = &intf->nbr;
    struct ospf_lsdb_list *l;
    int floodit = FALSE;
    int req_found;

    /* if < exchange don't use */
    if (n->state < NEXCHANGE)
	return;
    /* examine all new advertisements */
    for (l = ll; l != LLNULL; l = l->ptr[NEXT]) {
	if (n->state == NEXCHANGE || n->state == NLOADING) {

	    /* check to see if any on this list are on request list */
	    req_found = nbr_rem_req(n, l->lsdb->lsdb_adv);

	    if (n == src_nbr) {
		/* send delayed ack */
		if (req_found > REQ_LESS_RECENT) {
		    ADD_ACK_INTF(intf, l->lsdb);
		}
	    } else {
		/* Not src nbr case */
		/* do the right thing per version on the req list */
		if (req_found == REQ_LESS_RECENT) {
		    add_nbr_retrans(n, l->lsdb);
		    add_db_retrans(l->lsdb, n);
		    l->flood = FLOOD;
		    floodit = TRUE;
		}
	    }
	    if (req_found)
		continue;
	}			/* end of request */
	if (n == src_nbr) {
	    /* send delayed ack */
	    ADD_ACK_INTF(intf, l->lsdb);
	} else {		/* newer, so flood */
	    add_nbr_retrans(n, l->lsdb);
	    add_db_retrans(l->lsdb, n);
	    floodit = TRUE;
	    l->flood = FLOOD;
	}
    }				/* end of ll loop */
    if ((n->state == NEXCHANGE || n->state == NLOADING) && NO_REQ(n))
	(*(nbr_trans[LOAD_DONE][n->state])) (intf, n);
    if (floodit && ll) {
	send_lsu(ll, 1, 0, intf, 0);
    }
}


/*
 *	Multi-access interface flood routine
 */
static void
ma_flood __PF4(ll, struct ospf_lsdb_list *,
	       intf, struct INTF *,
	       src_intf, struct INTF *,
	       src_nbr, struct NBR *)
{
    int floodit = FALSE;
    int req_found;
    struct NBR *n = intf->nbr.next;
    struct ospf_lsdb_list *l;

    /* examine all neighbors */
    for (; n != NBRNULL; n = n->next) {
	/* if < exchange don't use */
	if (n->state < NEXCHANGE)
	    continue;

	/* examine all new advertisements */
	for (l = ll; l != LLNULL; l = l->ptr[NEXT]) {
	    if (n->state == NEXCHANGE || n->state == NLOADING) {
		req_found = nbr_rem_req(n, l->lsdb->lsdb_adv);
		if (n == src_nbr) {
		    if (src_intf->dr == n || src_intf->bdr == n) {
			/* received from DR or BDR */
			/* delayed ack */
			if (req_found > REQ_LESS_RECENT) {
			    ADD_ACK_INTF(intf, l->lsdb);
			}
		    } else {	/* received from IDROTHER */
			/*
			 * if this intf state is bdr ack, else if dr flood
			 * without adding it to retrans list
			 */
			if (src_intf->state == IBACKUP) {
			    /* delayed ack */
			    ADD_ACK_INTF(intf, l->lsdb);
			} else {
			    l->flood = FLOOD;
			    floodit = TRUE;
			}
		    }
		} else {	/* Not src nbr case */
		    /* we are requesting a newer version */
		    if (req_found == REQ_LESS_RECENT) {
			/* more recent has come in - flood to nbr */
			if (intf == src_intf && (intf->state == IDr ||
						 intf->state == IBACKUP)) {
			    add_nbr_retrans(n, l->lsdb);
			    add_db_retrans(l->lsdb, n);
			    if (intf->state != IBACKUP) {
				l->flood = FLOOD;
				floodit = TRUE;
			    }
			} else if (intf != src_intf) {
			    add_nbr_retrans(n, l->lsdb);
			    add_db_retrans(l->lsdb, n);
			    l->flood = FLOOD;
			    floodit = TRUE;
			}
		    }
		}		/* end of non-src nbr case */
		if (req_found)
		    continue;
	    }
	    /* state == NFULL || request not found */
	    if (n == src_nbr) {
		if (src_intf->dr == n || src_intf->bdr == n) {
		    /* received from DR or BDR */
		    ADD_ACK_INTF(intf, l->lsdb);	/* delayed ack */
		} else {
		    /* received from IDROTHER */
		    /*
		     * if this intf state is bdr ack else if dr flood without
		     * adding it to retrans list
		     */
		    if (src_intf->state == IBACKUP) {
			/* delayed ack */

			ADD_ACK_INTF(intf, l->lsdb);
		    } else {
			l->flood = FLOOD;
			floodit = TRUE;
		    }
		}
	    } else {
		if (intf == src_intf &&
		    (intf->state == IDr || intf->state == IBACKUP)) {
		    add_nbr_retrans(n, l->lsdb);
		    add_db_retrans(l->lsdb, n);
		    if (intf->state != IBACKUP) {
			l->flood = FLOOD;
			floodit = TRUE;
		    }
		} else if (intf != src_intf) {
		    add_nbr_retrans(n, l->lsdb);
		    add_db_retrans(l->lsdb, n);
		    l->flood = FLOOD;
		    floodit = TRUE;
		}
	    }
	}			/* end o ll loop */
    }				/* end o nbr loop */
    for (n = intf->nbr.next; n != NBRNULL; n = n->next)
	if ((n->state == NEXCHANGE || n->state == NLOADING) && NO_REQ(n)) {
	    (*(nbr_trans[LOAD_DONE][n->state])) (intf, n);
	}
    if (floodit && ll) {
	send_lsu(ll, 1, 0, intf, 0);
    }
}

/*
 * flood non-self originated LSAs to this area
 * - called by rxlinkup
 */
void
area_flood __PF5(a, struct AREA *,
		 trans, struct ospf_lsdb_list *,
		 src_intf, struct INTF *,
		 src_nbr, struct NBR *,
		 type, int)
{

    struct INTF *intf;
    struct ospf_lsdb_list *l;

    INTF_LIST(intf, a) {
	for (l = trans;
	     l != LLNULL; l = l->ptr[NEXT])
	    l->flood = DONTFLOOD;
	if (intf->type == POINT_TO_POINT)
	    ptp_flood(trans, intf, src_intf, src_nbr);
	else			/* Multi-access flood */
	    ma_flood(trans, intf, src_intf, src_nbr);
    } INTF_LIST_END(intf, a) ;

    if (type != LS_ASE &&
	a->area_id == OSPF_BACKBONE &&
	ospf.vcnt) {

	VINTF_LIST(intf) {
	    /* assume don't, set to flood otherwise */
	    for (l = trans;
		 l != LLNULL; l = l->ptr[NEXT])
		l->flood = DONTFLOOD;
	    ptp_flood(trans, intf, src_intf, src_nbr);
	} VINTF_LIST_END(intf) ;
    }
}


/*
 * flood self-originated lsa's to this area
 */
int
self_orig_area_flood __PF3(a, struct AREA *,
			   trans, struct ospf_lsdb_list *,
			   type, int)
{
    int floodit;
    int no_bufs;
    struct INTF *intf;
    struct NBR *n;
    struct ospf_lsdb_list *l;

    INTF_LIST(intf, a) {
	floodit = FALSE;
	for (l = trans; l != LLNULL; l = l->ptr[NEXT])
	    l->flood = FLOOD;
	NBRS_LIST(n, intf) {
	    if (n->state >= NEXCHANGE) {
		floodit++;	/* ok to send */
		if (n->state < NFULL) {
		    for (l = trans; l != LLNULL; l = l->ptr[NEXT])
			nbr_rem_req(n, l->lsdb->lsdb_adv);
		    if (NO_REQ(n))
			(*(nbr_trans[LOAD_DONE][n->state])) (intf, n);
		}
		/* if recvd newer self-gen still add to retrans */
		for (l = trans; l != LLNULL; l = l->ptr[NEXT]) {
		    add_nbr_retrans(n, l->lsdb);
		    add_db_retrans(l->lsdb, n);
		}
	    }
	} NBRS_LIST_END(n, intf) ;
	if (floodit && trans) {
	    no_bufs = send_lsu(trans, 1, 0, intf, 0);
	    if (no_bufs)
		return FLAG_NO_BUFS;
	}
    } INTF_LIST_END(intf, a) ;

    if (a->area_id == OSPF_BACKBONE &&
	ospf.vcnt &&
	type != LS_ASE) {

	VINTF_LIST(intf) {
	    n = &intf->nbr;
	    if (n->state >= NEXCHANGE) {
		if (n->state < NFULL) {
		    for (l = trans; l != LLNULL; l = l->ptr[NEXT])
			nbr_rem_req(n, l->lsdb->lsdb_adv);
		    if (NO_REQ(n))
			(*(nbr_trans[LOAD_DONE][n->state])) (intf, n);
		}
		for (l = trans; l != LLNULL; l = l->ptr[NEXT]) {
		    add_nbr_retrans(n, l->lsdb);
		    add_db_retrans(l->lsdb, n);
		    l->flood = FLOOD;
		}
		if (trans) {
		    no_bufs = send_lsu(trans, 1, 0, intf, 0);
		    if (no_bufs)
			return FLAG_NO_BUFS;
		}
	    }
	} VINTF_LIST_END(intf) ;
    }

    return FLAG_NO_PROBLEM;
}
