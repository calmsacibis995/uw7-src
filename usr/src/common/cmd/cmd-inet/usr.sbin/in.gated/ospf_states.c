#ident	"@(#)ospf_states.c	1.3"
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


/******************************************************************************
 *
 *			STATE TRANSITION SUPPORT ROUTINES
 *
 ******************************************************************************/



/******************************************************************************
 *
 *			NEIGHBOR STATE TRANSITIONS
 *
 ******************************************************************************/

static const char *ospf_nbr_events[] = {
    "Hello Received",
    "Start",
    "Two Way Received",
    "Adjacency OK",
    "Negotiation Done",
    "Exchange Done",
    "Sequence # Mismatch",
    "Bad LS Request",
    "Loading Done",
    "One way",
    "Reset Adjacency",
    "Kill Neighbor",
    "Inactivity Timer",
    "Lower Level Down"
};

const char *ospf_nbr_states[] = {
    "Down",
    "Attempt",
    "Init",
    "Two Way",
    "Exch Start",
    "Exchange",
    "Loading",
    "Full",
    "SCVirtual"
};

#define	msg_event_nbr(nbr, event, old) \
	if (TRACE_TF(ospf.trace_options, TR_STATE)) { \
	    trace_only_tf(ospf.trace_options, \
			  TRC_NL_AFTER,\
			  ("OSPF TRANSITION	Neighbor %A  EVENT %s  %10s -> %-10s", \
			   (nbr)->nbr_addr, \
			   ospf_nbr_events[event], \
			   ospf_nbr_states[old], \
			   ospf_nbr_states[(nbr)->state])); \
	}


static void
NErr __PF2(intf, struct INTF *,
	   nbr, struct NBR *)
{
}

/*
 *	NHello - received a hello packet
 *		current state: NATTEMPT or NDOWN; new state: NINIT
 */

static void
NHello __PF2(intf, struct INTF *,
	     nbr, struct NBR *)
{
    u_int oldstate = nbr->state;
    struct AREA *area = intf->area;
    /* Unnumbered ptop link? */
    u_int32 nh_addr = (NBR_ADDR(nbr)) ? NBR_ADDR(nbr) : NBR_ID(nbr);

    /*
     * fire up dead timer alarm
     */
    reset_inact_tmr(nbr);
    nbr->state = NINIT;
    intf->nbrIcnt++;
    ospf.nbrIcnt++;
    area->nbrIcnt++;
    OSPF_NH_ALLOC(nbr->nbr_nh = ospf_nh_add(nbr->intf->ifap,
					    nh_addr,
					    NH_NBR));

    msg_event_nbr(nbr, HELLO_RX, oldstate);
    nbr->events++;
}

/*
 * Non-broadcast interface is up - current state = NDOWN
 *	start inactivity timer and start sending hellos
 */
static void
NStart __PF2(intf, struct INTF *,
	     nbr, struct NBR *)
{
    u_int oldstate = nbr->state;

    nbr->state = NATTEMPT;
    if (intf->pri)
	send_hello(intf, nbr, FALSE);
    reset_inact_tmr(nbr);

    msg_event_nbr(nbr, START, oldstate);
    nbr->events++;
}


static void
N2Way __PF2(intf, struct INTF *,
	    nbr, struct NBR *)
{

    u_int oldstate = nbr->state;

    nbr->state = N2WAY;

    if ((intf->type <= NONBROADCAST) && (intf->state > IPOINT_TO_POINT)) {
	(*(if_trans[NBR_CHANGE][intf->state])) (intf);
    }

    msg_event_nbr(nbr, TWOWAY, oldstate);
    nbr->events++;
    /*
     * if type is PTP or VIRTUAL or we are still in N2WAY and
     * the neighbor is DR or BDR - establish an adjacency
     */
    if ((intf->type > NONBROADCAST && intf->type <= VIRTUAL_LINK) ||
	((nbr->state == N2WAY && intf->type < POINT_TO_POINT) &&
	 (intf->dr == nbr || intf->bdr == nbr ||
	  intf->dr == &intf->nbr || intf->bdr == &intf->nbr)))
	(*(nbr_trans[ADJ_OK][nbr->state])) (intf, nbr);
    /*
     * choose dr will have reset adjacencies or established
     * adjacencies if we are dr or bdr
     */
}


static void
NAdjOk __PF2(intf, struct INTF *,
	     nbr, struct NBR *)
{
    u_int oldstate = nbr->state;

    nbr->state = NEXSTART;
    /*
     * set initial seq number
     */
    nbr->seq = time_sec;
    nbr->I_M_MS = (bit_I | bit_M | bit_MS);
    send_dbsum(intf, nbr, 0);

    msg_event_nbr(nbr, ADJ_OK, oldstate);
    nbr->events++;
}


static void
NNegDone __PF2(intf, struct INTF *,
	       nbr, struct NBR *)
{
    u_int oldstate = nbr->state;
    struct AREA *area = intf->area;

    /*
     * stop sending db negotiation pkt
     */
    intf->nbrEcnt++;
    area->nbrEcnt++;
    ospf.nbrEcnt++;
    /*
     * build a summary of all of the LSDB
     * return if build_dbsum is out of bufs
     */
    if (build_dbsum(intf, nbr)) {
	intf->nbrEcnt--;
	area->nbrEcnt--;
	ospf.nbrEcnt--;
	return;
    }

    nbr->state = NEXCHANGE;
    msg_event_nbr(nbr, NEGO_DONE, oldstate);
    nbr->events++;
}


static void
NExchDone __PF2(intf, struct INTF *,
		nbr, struct NBR *)

{
    u_int oldstate = nbr->state;

    /*
     * if master free last dbsum else leave it hanging around for a while
     */
    if (nbr->mode == MASTER) {
	if (nbr->dbsum != LSDB_SUM_NULL)
	    freeDbSum(nbr);
    } else {				/* mode == SLAVE */
	/*
 	 * put nbr in SLAVE_HOLDING mode until timer has expired
	 */
	nbr->mode = SLAVE_HOLD;
	set_hold_tmr(nbr);
    }

    nbr->state = NLOADING;
    msg_event_nbr(nbr, EXCH_DONE, oldstate);
    nbr->events++;

    if (NO_REQ(nbr))
	(*(nbr_trans[LOAD_DONE][nbr->state])) (intf, nbr);
}


static void
NBadReq __PF2(intf, struct INTF *,
	      nbr, struct NBR *)
{
    u_int oldstate = nbr->state;
    struct AREA *area = intf->area;

    /*
     * Free all lists
     */
    freeDbSum(nbr);			/* summary of area db */
    REM_NBR_RETRANS(nbr);
    freeLsReq(nbr);			/* list of requests from this nbr */
    if (intf->nbrIcnt == 1) {
    	freeAckList(intf);		/* list of acks to send to this nbr */
    }

    if (nbr->state == NFULL) {
	intf->nbrFcnt--;		/* Another one bites the dust */
	area->nbrFcnt--;
	ospf.nbrFcnt--;
	/* 
	 * Schedule nbr change or build_rtr 
	 */
	if ((intf->type <= NONBROADCAST) && (intf->state >= IDr))
	    BIT_SET(intf->flags, OSPF_INTFF_NBR_CHANGE);
	else if (intf->type == POINT_TO_POINT || intf->type == VIRTUAL_LINK)
	    area->build_rtr = TRUE;

    }
    if (nbr->state >= NEXCHANGE) {
	intf->nbrEcnt--;		/* Another one bites the dust */
	area->nbrEcnt--;
	ospf.nbrEcnt--;
    }

    nbr->state = NEXSTART;
#ifdef	notdef
    intf->nbrEcnt--;
    area->nbrEcnt--;
    ospf.nbrEcnt--;
#endif	/* notdef */
    /*
     * set initial seq number and start all over
     */
    nbr->seq = time_sec;
    nbr->I_M_MS = (bit_I | bit_M | bit_MS);
    send_dbsum(intf, nbr, 0);

    msg_event_nbr(nbr, BAD_LS_REQ, oldstate);
    nbr->events++;
}


static void
NBadSq __PF2(intf, struct INTF *,
	     nbr, struct NBR *)
{
    u_int oldstate = nbr->state;
    struct AREA *area = intf->area;

    /*
     * Free all lists
     */
    freeDbSum(nbr);			/* summary of area db */
    REM_NBR_RETRANS(nbr);
    freeLsReq(nbr);			/* list of requests from this nbr */

    if (intf->nbrIcnt == 1) {
    	freeAckList(intf);		/* list of acks to send to this nbr */
    }

    if (nbr->state == NFULL) {
	intf->nbrFcnt--;		/* Another one bites the dust */
	area->nbrFcnt--;
	ospf.nbrFcnt--;
	/* 
	 * Schedule nbr change or build_rtr 
	 */
	if ((intf->type <= NONBROADCAST) && (intf->state >= IDr))
	    BIT_SET(intf->flags, OSPF_INTFF_NBR_CHANGE);
	else if (intf->type == POINT_TO_POINT || intf->type == VIRTUAL_LINK)
	    area->build_rtr = TRUE;
    }
    if (nbr->state >= NEXCHANGE) {
	intf->nbrEcnt--;		/* Another one bites the dust */
	area->nbrEcnt--;
	ospf.nbrEcnt--;
    }
    nbr->state = NEXSTART;
    /*
     * set initial seq number and start all over
     */
    nbr->seq = time_sec;
    nbr->I_M_MS = (bit_I | bit_M | bit_MS);
    send_dbsum(intf, nbr, 0);

    msg_event_nbr(nbr, SEQ_MISMATCH, oldstate);
    nbr->events++;

}

static void
NLoadDone __PF2(intf, struct INTF *,
		nbr, struct NBR *)
{
    u_int oldstate = nbr->state;
    struct AREA *area = intf->area;

    /*
     * Another Full neighbor
     */
    nbr->state = NFULL;
    intf->nbrFcnt++;
    area->nbrFcnt++;
    ospf.nbrFcnt++;

    area->build_rtr = TRUE;
    if (intf->state == IDr)
	BIT_SET(intf->flags, OSPF_INTFF_BUILDNET);

    msg_event_nbr(nbr, LOAD_DONE, oldstate);
    nbr->events++;
}

/*
 *	Can no longer see ourself in nbr's hello pkt
 *		- revert back to INIT
 */
static void
N1Way __PF2(intf, struct INTF *,
	    nbr, struct NBR *)
{
    u_int oldstate = nbr->state;
    struct AREA *area = intf->area;

    rem_hold_tmr(nbr);
    /*
     * Another neighbor bites the dust
     */
    if (nbr->state == NFULL) {
	intf->nbrFcnt--;
	area->nbrFcnt--;
	ospf.nbrFcnt--;
	/* 
	 * Schedule nbr change or build_rtr 
	 */
	if ((intf->type <= NONBROADCAST) && (intf->state >= IDr))
	    BIT_SET(intf->flags, OSPF_INTFF_NBR_CHANGE);
	else if (intf->type == POINT_TO_POINT || intf->type == VIRTUAL_LINK)
	    area->build_rtr = TRUE;

    }
    if (nbr->state >= NEXCHANGE) {
	intf->nbrEcnt--;
	area->nbrEcnt--;
	ospf.nbrEcnt--;
    }
    nbr->state = NINIT;
    nbr->mode = 0;
    nbr->seq = 0;
    nbr->dr = 0;
    nbr->bdr = 0;

    /*
     * Free all lists
     */
    freeDbSum(nbr);			/* summary of area db */
    REM_NBR_RETRANS(nbr);
    freeLsReq(nbr);			/* list of requests from this nbr */
    if (intf->nbrIcnt == 1) {
    	freeAckList(intf);		/* list of acks to send to this nbr */
    }

    msg_event_nbr(nbr, ONEWAY, oldstate);
    nbr->events++;

}

static void
NRstAd __PF2(intf, struct INTF *,
	     nbr, struct NBR *)
{
    u_int oldstate = nbr->state;
    struct AREA *area = intf->area;

    rem_hold_tmr(nbr);
    /*
     * Another nbr bites the dust
     */
    if (nbr->state == NFULL) {
	intf->nbrFcnt--;
	area->nbrFcnt--;
	ospf.nbrFcnt--;
    }
    if (nbr->state >= NEXCHANGE) {
	intf->nbrEcnt--;
	area->nbrEcnt--;
	ospf.nbrEcnt--;
    }
    nbr->state = N2WAY;
    nbr->mode = 0;
    nbr->seq = 0;
    /*
     * Free all lists
     */
    freeDbSum(nbr);			/* summary of area db */
    REM_NBR_RETRANS(nbr);
    freeLsReq(nbr);			/* list of requests from this nbr */

    if (intf->nbrIcnt == 1) {
    	freeAckList(intf);		/* list of acks to send to this nbr */
    }

    msg_event_nbr(nbr, RST_ADJ, oldstate);
    nbr->events++;

    /*
     * if type is PTP or VIRTUAL || (since we are still in N2WAY)
     * the neighbor is DR or BDR - establish an adjacency
     */
    if ((intf->type > NONBROADCAST && intf->type <= VIRTUAL_LINK) ||
	((nbr->state == N2WAY && intf->type < POINT_TO_POINT) &&
	 (intf->dr == nbr || intf->bdr == nbr ||
	  intf->dr == &intf->nbr || intf->bdr == &intf->nbr)))
	(*(nbr_trans[ADJ_OK][nbr->state])) (intf, nbr);
}

/*
 * Neighbor has gone away -
 */
static void
NDown __PF2(intf, struct INTF *,
	    nbr, struct NBR *)
{
    u_int oldstate = nbr->state;
    struct NBR *n;
    struct AREA *area = intf->area;

    nbr->state = NDOWN;
    msg_event_nbr(nbr, INACT_TIMER, oldstate);
    nbr->events++;

    /* disable all timers associated with this neighbor */
    rem_hold_tmr(nbr);
    rem_inact_tmr(nbr);

    /*
     * Free all lists
     */
    freeDbSum(nbr);			/* summary of area db */
    REM_NBR_RETRANS(nbr);
    freeLsReq(nbr);			/* list of requests from this nbr */
    if (intf->nbrIcnt == 1) {
    	freeAckList(intf);		/* list of acks to send to this nbr */
    }

    /*
     * Another nbr bytes the dust
     */
    if (oldstate > NATTEMPT) {
	intf->nbrIcnt--;
	ospf.nbrIcnt--;
	area->nbrIcnt--;
    }
    if (oldstate == NFULL) {
	intf->nbrFcnt--;
	area->nbrFcnt--;
	ospf.nbrFcnt--;
    }
    if (oldstate >= NEXCHANGE) {
	intf->nbrEcnt--;
	area->nbrEcnt--;
	ospf.nbrEcnt--;
    }

    ospf_nh_free(&nbr->nbr_nh);

    /*
     * For types other than BROADCAST we want to keep the nbr and the
     * initialized info around
     */
    if (intf->type > BROADCAST) {
	nbr->mode = 0;
	nbr->seq = 0;
	if (intf->type != VIRTUAL_LINK) {
	    if (nbr->nbr_id) {
		sockfree(nbr->nbr_id);
		nbr->nbr_id = (sockaddr_un *) 0;
	    }
	} else {		/* virtual link */
	    if (nbr->nbr_addr) {
		sockfree(nbr->nbr_addr);
		nbr->nbr_addr = (sockaddr_un *) 0;
	    }
	}
	nbr->dr = nbr->bdr = 0;
	if (nbr == intf->dr) {
	    intf->dr = NBRNULL;
	    intf->nbr.dr = 0;
	} 
	if (nbr == intf->bdr) {
	    intf->bdr = NBRNULL;
	    intf->nbr.bdr = 0;
	}
    } else {
	/*
	 * Broadcast - free the nbr structure
	 */
	/* Johnathan's fix */
	if (nbr == intf->dr) {
	    intf->dr = NBRNULL;
	    intf->nbr.dr = 0;
	}
	if (nbr == intf->bdr) {
	    intf->bdr = NBRNULL;
	    intf->nbr.bdr = 0;
	}
	for (n = &intf->nbr; n != NBRNULL; n = n->next) {
	    if (n->next == nbr) {	/* found it! */
		n->next = nbr->next;
		ospf_nbr_delete(intf, nbr);
		nbr = NBRNULL;
#ifdef	notdef
		ospf.nbr_sb_not_valid = TRUE;
#endif	/* notdef */
		break;
	    }
	}
	assert(nbr == NBRNULL);
    }

    if ((intf->type <= NONBROADCAST) && (intf->state >= IDr))
	BIT_SET(intf->flags, OSPF_INTFF_NBR_CHANGE);
    else if (intf->type == POINT_TO_POINT || intf->type == VIRTUAL_LINK)
	area->build_rtr = TRUE;
}


_PROTOTYPE(nbr_trans[NNBR_EVENTS][NNBR_STATES],
	  void,
	  (struct INTF *,
	   struct NBR *)) = {
/*
 * event/state		NDOWN	NATTEMPT NINIT	N2WAY	NEXSTART NEXCH	NLOAD	NFULL
 */
/* HELLO_RX  */ {	NHello,	NHello,	NErr,	NErr,	NErr,	NErr,	NErr,	NErr },
/* START     */ {	NStart,	NErr,	NErr,	NErr,	NErr,	NErr,	NErr,	NErr },
/* 2WAY      */ {	NErr,	NErr,	N2Way,	NErr,	NErr,	NErr,	NErr,	NErr },
/* ADJ_OK    */ {	NErr,	NErr,	NErr,	NAdjOk,	NErr,	NErr,	NErr,	NErr },
/* NEGO_DONE */ {	NErr,	NErr,	NErr,	NErr,	NNegDone, NErr,	NErr,	NErr },
/* EXCH_DONE */ {	NErr,	NErr,	NErr,	NErr,	NErr,	NExchDone, NErr, NErr },
/* SEQ_MSMTCH*/ {	NErr,	NErr,	NErr,	NBadSq,	NBadSq,	NBadSq,	NBadSq,	NBadSq },
/* BAD_LSREQ */ {	NErr,	NErr,	NErr,	NErr,	NErr,	NBadReq, NBadReq, NBadReq },
/* LOAD_DONE */ {	NErr,	NErr,	NErr,	NErr,	NErr,	NErr,	NLoadDone, NErr },
/* 1WAY      */ {	NErr,	NErr,	NErr,	N1Way,	N1Way,	N1Way,	N1Way,	N1Way },
/* RST_ADJ   */ {	NErr,	NErr,	NErr,	NErr,	NRstAd,	NRstAd,	NRstAd,	NRstAd },
/* KILL_NBR  */ {	NDown,	NDown,	NDown,	NDown,	NDown,	NDown,	NDown,	NDown },
/* INACT_TMR */ {	NDown,	NDown,	NDown,	NDown,	NDown,	NDown,	NDown,	NDown },
/* LLDOWN    */ {	NDown,	NDown,	NDown, 	NDown,	NDown,	NDown,	NDown,	NDown },
};


/******************************************************************************
 *
 *			INTERFACE STATE TRANSITIONS
 *
 ******************************************************************************/

static const char *ospf_intf_types[] = {
    "",
    "Broadcast",
    "Nonbroadcast",
    "Point To Point",
    "Virtual"
};

static const char *ospf_intf_events[] = {
    "Interface Up",
    "Wait Timer",
    "Backup Seen",
    "Neighbor Change",
    "Loop Indication",
    "Unloop Indication",
    "Interface Down"
};


const char *ospf_intf_states[] = {
    "Down",
    "Loopback",
    "Waiting",
    "P To P",
    "DR",
    "BackupDR",
    "DR Other"
};


#define	msg_event_intf(intf, event, cur) \
	if (TRACE_TF(ospf.trace_options, TR_STATE)) { \
	    trace_only_tf(ospf.trace_options, \
			  TRC_NL_AFTER,\
			  ("OSPF TRANSITION %s Interface %A  EVENT %s  %-8s -> %8s", \
			   ospf_intf_types[(intf)->type], \
			   (intf)->type == VIRTUAL_LINK ? (intf)->nbr.nbr_addr: (intf)->ifap->ifa_addr, \
			   ospf_intf_events[event], \
			   ospf_intf_states[cur],\
			   ospf_intf_states[(intf)->state]); )\
	}

static void
IErr __PF1(intf, struct INTF *)
{
}

/*
 * IUp: in state: IDOWN, event: INTF_UP
 */
static void
IUp __PF1(intf, struct INTF *)
{

    u_int oldstate = intf->state;
    struct AREA *a = intf->area;
    struct NBR *n;

    switch (intf->type) {
    case BROADCAST:
	intf->state = IWAITING;
	send_hello(intf, 0, FALSE);
	start_wait_tmr(intf);
	break;

    case POINT_TO_POINT:
	/*
	 * fire up dead timer alarm
	 */
	reset_inact_tmr(&intf->nbr);
	intf->state = IPOINT_TO_POINT;
	send_hello(intf, 0, FALSE);
	break;
	
    case VIRTUAL_LINK:
	intf->state = IPOINT_TO_POINT;
	BIT_SET(intf->trans_area->area_flags, OSPF_AREAF_VIRTUAL_UP);
	ospf.vUPcnt++;
	send_hello(intf, 0, FALSE);
	break;

    case NONBROADCAST:
	if (intf->pri) {
	    intf->state = IWAITING;
	    start_wait_tmr(intf);
	} else
	    intf->state = IDrOTHER;
	/*
	 * Fire up all NBMA neighbors
	 */
	for (n = intf->nbr.next; n != NBRNULL; n = n->next)
	    (*(nbr_trans[START][n->state])) (intf, n);
	break;
    }

    /*
     * If virtual link has come up, schedule a build_rtr_lsa
     */
    if (intf->type != VIRTUAL_LINK)
	a->build_rtr = TRUE;
    else
	set_rtr_sched(&ospf.backbone);

    intf->events++;
    a->ifUcnt++;
    msg_event_intf(intf, INTF_UP, oldstate);

    /* Record up time */
    intf->up_time = time_sec;
}


/*
 * Interface has gone down - tq_ifchk has been called
 */
static void
IDown __PF1(intf, struct INTF *)
{
    u_int oldstate = intf->state;
    struct NBR *n, *next_nbr;
    struct AREA *a = intf->area;

    intf->state = IDOWN;
    /*
     * reset intf variables
     */
    rem_wait_tmr(intf);
    reset_net_sched(intf);
    intf->nbr.dr = 0;
    intf->nbr.bdr = 0;
    intf->dr = NBRNULL;
    intf->bdr = NBRNULL;

    freeAckList(intf);		/* list of acks to send to this nbr */

    msg_event_intf(intf, INTF_DOWN, oldstate);

    for (n = FirstNbr(intf); n != NBRNULL; n = next_nbr) {
	next_nbr = n->next;
	(*(nbr_trans[KILL_NBR][n->state])) (intf, n);
    }

    /*
     * Net has left us for the great beyond
     */
    if (intf->type <= NONBROADCAST && intf->nbrIcnt == 0) {
	(*(if_trans[NBR_CHANGE][intf->state])) (intf);
	a->build_rtr = TRUE;
    }
    if (intf->type == VIRTUAL_LINK) {
	BIT_SET(intf->trans_area->area_flags, OSPF_AREAF_VIRTUAL_UP);
	ospf.vUPcnt--;
    }
    a->ifUcnt--;
    intf->events++;
}


static void
ILoop __PF1(intf, struct INTF *)
{
}


static void
IUnLoop __PF1(intf, struct INTF *)
{
}

/*
 * Wait timer has gone off from state IWAIT
 *		time to elect BDR and DR
 */
static void
IWaitTmr __PF1(intf, struct INTF *)
{
    u_int oldstate = intf->state;
    struct AREA *a = intf->area;

    /*
     * if we are elig and there are neighbors
     * reform adjacencies
     */
    ospf_choose_dr(intf);

    /*
     * flag to build new router lsa
     */
    a->build_rtr = TRUE;

    msg_event_intf(intf, WAIT_TIMER, oldstate);

    if (oldstate != intf->state)
    	intf->events++;
}



/*
 * Backup has been seen from state IWAIT
 *		time to elect BDR and DR
 */
static void
IBackUp __PF1(intf, struct INTF *)
{
    u_int oldstate = intf->state;
    struct AREA *a = intf->area;

    /*
     * nuke the wait timer
     */
    rem_wait_tmr(intf);

    /*
     * if we are elig and there are neighbors
     * reform adjacencies
     */
    ospf_choose_dr(intf);

    /*
     * flag to build new router lsa
     */
    a->build_rtr = TRUE;

    msg_event_intf(intf, BACKUP_SEEN, oldstate);

    if (oldstate != intf->state)
    	intf->events++;
}


static void
INbrCh __PF1(intf, struct INTF *)
{
    u_int oldstate = intf->state;
    struct AREA *a = intf->area;

    BIT_RESET(intf->flags, OSPF_INTFF_NBR_CHANGE);

    if (intf->type > NONBROADCAST)
	return;
    /*
     * if we are elig and there are neighbors
     * reform adjacencies
     */
    ospf_choose_dr(intf);

    /*
     * flag to build new router lsa
     */
    a->build_rtr = TRUE;

    msg_event_intf(intf, NBR_CHANGE, oldstate);

    if (oldstate != intf->state)
    	intf->events++;
}


_PROTOTYPE(if_trans[NINTF_EVENTS][NINTF_STATES],
	  void,
	  (struct INTF *)) = {
/*
 * event/state  	IDOWN	ILOOPBACK IWAIT	IPtoP	IDR	IBACKUP	IDROTHER
 */
/* UP      */ 	{	IUp,	IErr,	IErr,	IErr,	IErr,	IErr,	IErr },
/* WAIT_TM */   {	IErr,	IErr,	IWaitTmr, IErr,	IErr,	IErr,	IErr },
/* BACKUP  */   {	IErr,	IErr,	IBackUp, IErr,	IErr,	IErr,	IErr },
/* NBR_CH  */   {	IErr,	IErr,	IErr,	IErr,	INbrCh,	INbrCh,	INbrCh },
/* LOOP    */   {	ILoop,	IErr,	IErr,	IErr,	IErr,	IErr,	IErr },
/* UNLOOP  */   {	IErr,	IUnLoop,IErr,	IErr,	IErr,	IErr,	IErr },
/* DOWN    */   {	IErr,	IDown,	IDown,	IDown,	IDown,	IDown,	IDown }
};
