#ident	"@(#)ospf_tqhandle.c	1.3"
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
 * Call down event on virtual interfaces associated with intf
 */
static void
virtual_intf_down  __PF1(intf, struct INTF *)
{

    /* If virtual links exist that use this interface, kill it */
    if (IAmBorderRtr &&
	(intf->area->area_id) != OSPF_BACKBONE &&
	ospf.vcnt) {
	struct INTF *vintf;

	VINTF_LIST(vintf) {
	    if (vintf->state > IDOWN &&
		vintf->ifap == intf->ifap) {
		(*if_trans[INTF_DOWN][vintf->state]) (vintf);
		/* schedule a build_rtr for the backbone */
		set_rtr_sched(&ospf.backbone);
	    }
	    /* set_rtr_sched will delay building */
	    ospf.backbone.build_rtr = FALSE;
	} VINTF_LIST_END(vintf) ;
    }
}


/*
 *	Interface is down
 */
void
ospf_ifdown __PF1(intf, struct INTF *)
{
    struct AREA *area = intf->area;
    struct ospf_lsdb_list *txq = LLNULL;

    trace_only_tf(ospf.trace_options,
		  0,
		  ("ospf_ifdown: Interface %s (%s) DOWN",
		   intf->ifap->ifa_link->ifl_name,
		   intf->ifap->ifa_link->ifl_name));

    (*if_trans[INTF_DOWN][intf->state]) (intf);
    virtual_intf_down(intf);

    if (BIT_TEST(intf->flags, OSPF_INTFF_NBR_CHANGE)) {
	(*(if_trans[NBR_CHANGE][intf->state])) (intf);
    }

    /* Received from outside source */
    if (area->build_rtr) {
	area->spfsched |= build_rtr_lsa(area, &txq, 0);
	area->build_rtr = FALSE;
    }
    /* build net and rtr lsa if necessary */
    if (BIT_TEST(intf->flags, OSPF_INTFF_BUILDNET)) {
	area->spfsched |= build_net_lsa(intf, &txq, 0);
	BIT_RESET(intf->flags, OSPF_INTFF_BUILDNET);
    }
    if (txq != LLNULL) {	/* may be locked out */
	self_orig_area_flood(area, txq, LS_RTR);
	ospf_freeq((struct Q **)&txq, ospf_lsdblist_index);
    }
    if (area->spfsched)
	ospf_spf_sched();
}


/*
 *	Interface is up
 */
void
ospf_ifup __PF1(intf, struct INTF *)
{
    struct ospf_lsdb_list *txq = LLNULL;
    struct AREA *area = intf->area;

    (*(if_trans[INTF_UP][IDOWN])) (intf);

    /* Received from outside source (GATED) */
    if (area->build_rtr) {
	area->spfsched |= build_rtr_lsa(area, &txq, 0);
	area->build_rtr = FALSE;
    }
    /* build net and rtr lsa if necessary */
    if (BIT_TEST(intf->flags, OSPF_INTFF_BUILDNET)) {
	area->spfsched |= build_net_lsa(intf, &txq, 0);
	BIT_RESET(intf->flags, OSPF_INTFF_BUILDNET);
    }
    if (txq != LLNULL) {	/* may be locked out */
	self_orig_area_flood(area, txq, LS_RTR);
	ospf_freeq((struct Q **)&txq, ospf_lsdblist_index);
    }
    if (area->spfsched)
	ospf_spf_sched();
}


/*ARGSUSED*/
void
tq_hellotmr __PF2(tip, task_timer *,
		  interval, time_t)
{
    struct INTF *intf = (struct INTF *) tip->task_timer_data;

    TRAP_REF_UPDATE;	/* Update the trap event counter */

    if (intf->state <= IDOWN) {
	return;
    }

    send_hello(intf, 0, FALSE);

    if (intf->type == NONBROADCAST) {
	if ((time_t) (intf->pollmod * intf->hello_timer) >= intf->poll_timer) {
	    struct NBR *nbr;
		
	    NBRS_LIST(nbr, intf) {	    
		/* Poll timer */
		if (nbr->state == NDOWN) {
		    send_hello(intf, nbr, FALSE);
		}
	    } NBRS_LIST_END(nbr, intf) ;

	    intf->pollmod = 1;
	} else {
	    intf->pollmod++;
	}
    }
}


/*ARGSUSED*/
void
tq_adjtmr __PF2(tip, task_timer *,
		interval, time_t)
{
    struct INTF *intf = (struct INTF *) tip->task_timer_data;
    time_t next_time = intf->dead_timer;

    TRAP_REF_UPDATE;	/* Update the trap event counter */

    if (intf->state > IDOWN) {
	struct NBR *nbr;
	struct ospf_lsdb_list *txq = LLNULL;
	struct AREA *area = intf->area;
	struct NBR *next_nbr;

	for (nbr = FirstNbr(intf); nbr != NBRNULL; nbr = next_nbr) {
	    next_nbr = nbr->next;
	    
	    /* Inactivity timer */
	    if (nbr->state >= NATTEMPT
		&& nbr->last_hello) {
		time_t left = intf->dead_timer - (time_sec - nbr->last_hello);

		if (left <= 0) {
		    nbr->last_hello = 0;
		    if (intf->type == VIRTUAL_LINK) {
			(*if_trans[INTF_DOWN][intf->state]) (intf);
			goto virtual_bail_out;
		    } else {
			(*(nbr_trans[INACT_TIMER][nbr->state])) (intf, nbr);
		    }
		} else if (left < next_time) {
		    next_time = left;
		}
	    }
	    
	    /*
	     * Handle holding interval for Slave mode - nbr state is Loading
	     * or Full
	     */
	    if (nbr->mode == SLAVE_HOLD
		&& nbr->last_exch) {
		time_t left = intf->dead_timer - (time_sec - nbr->last_exch);

		if (left <= 0) {
		    nbr->mode = SLAVE;
		    if (!(nbr->dbsum == LSDB_SUM_NULL)) {
			dbsum_free(nbr->dbsum);
			nbr->dbsum = LSDB_SUM_NULL;
		    }
		} else if (left < next_time) {
		    next_time = left;
		}
	    }
 	}

	/* Wait timer */
	if (intf->state == IWAITING
	    && intf->wait_time) {
	    time_t left = intf->wait_time - (time_sec - intf->dead_timer);

	    if (left <= 0) {
		(*if_trans[WAIT_TIMER][intf->state]) (intf);
	    } else if (left < next_time) {
		next_time = left;
	    }
	}

	if (BIT_TEST(intf->flags, OSPF_INTFF_NBR_CHANGE)) {
	    (*(if_trans[NBR_CHANGE][intf->state])) (intf);
	}

	/* build net and rtr lsa if necessary */
	if (BIT_TEST(intf->flags, OSPF_INTFF_BUILDNET)) {
	    area->spfsched |= build_net_lsa(intf, &txq, 0);
	    BIT_RESET(intf->flags, OSPF_INTFF_BUILDNET);
	}

      virtual_bail_out:
	if (area->build_rtr) {
	    area->spfsched |= build_rtr_lsa(area, &txq, 0);
	    area->build_rtr = FALSE;
	}
	if (txq != LLNULL) {	/* may be locked out */
	    self_orig_area_flood(area, txq, LS_RTR);
	    ospf_freeq((struct Q **)&txq, ospf_lsdblist_index);
	}
	if (area->spfsched)
	    ospf_spf_sched();
    }

    /* Reschedule */
    task_timer_set(tip, (time_t) 0, next_time);
}


/*
 *	LSA Lock one shot timer - value is MinLSInterval
 *	- can't generate an LSA within MinLSInterval
 */
/*ARGSUSED*/
void
tq_lsa_lock __PF2(tip, task_timer *,
		  interval, time_t)
{
    struct AREA *a = (struct AREA *) tip->task_timer_data;
    struct INTF *intf;
    struct ospf_lsdb_list *txq = LLNULL;
    struct LSDB *db;

    TRAP_REF_UPDATE;	/* Update the trap event counter */
    /*
     * Check to-be-free list
     */
    DB_RUNQUE(ospf.db_free_list, db) {
	if (DB_CAN_BE_FREED(db)) {
	    db_free(db, LS_TYPE(db));
	}
    } DB_RUNQUE_END(ospf.db_free_list, db) ;

    /* MODIFIED 1/22/92 */
    /* If LSDB limit is exceeded send trap */
    if (ospf.lsdb_limit && 
	ospf.db_cnt > ospf.lsdb_limit &&
    	!ospf.lsdb_overflow) {
	ospf.lsdb_overflow++;
	ospf.lsdb_hiwater_exceeded++;
#ifdef NOTYETDEF
	ospf_lsdb_trap(ospfLSDBOverflowTrap);
#endif
    } else if (ospf.lsdb_hiwater && 
	ospf.db_cnt > ospf.lsdb_hiwater &&
    	!ospf.lsdb_hiwater_exceeded) {
	ospf.lsdb_hiwater_exceeded++;
#ifdef NOTYETDEF
	ospf_lsdb_trap(ospfLSDBHiWaterExceededTrap);
#endif
    }

    INTF_LIST(intf, a) {
	if (intf->lock_time && (time_sec - intf->lock_time >= MinLSInterval)) {
	    if (BIT_TEST(intf->flags, OSPF_INTFF_NETSCHED)) {
		reset_net_sched(intf);	/* turn off sched */
		reset_net_lock(intf);
		if (intf->state == IDr)
		    a->spfsched |= build_net_lsa(intf, &txq, TRUE);
	    } else
		reset_net_lock(intf);
	}
    } INTF_LIST_END(intf, a) ;
    if (a->lock_time && (time_sec - a->lock_time >= MinLSInterval)) {
	if ((a->lsalock & RTRSCHED)) {
	    reset_rtr_sched(a);	/* turn off sched */
	    reset_rtr_lock(a);
	    a->spfsched |= build_rtr_lsa(a, &txq, 1);
	} else
	    reset_rtr_lock(a);
    }
    if (txq != LLNULL) {	/* may not be adjacent to anyone */
	self_orig_area_flood(a, txq, LS_NET);
	ospf_freeq((struct Q **)txq, ospf_lsdblist_index);
    }
    /*
     * Check for a spf sched event having been run - if out of memory during
     * spf, spf may have scheduled
     */
    if (a->spfsched)
	ospf_spf_sched();
}


/*
 *	LSA update interval timer for rtr and net LSAs
 *	- generate for all areas
 */
/*ARGSUSED*/
void
tq_IntLsa __PF2(tip, task_timer *,
		interval, time_t)
{
    int schedule = 0;
    struct AREA *a;
    struct INTF *intf;
    struct ospf_lsdb_list *txq = LLNULL;

    TRAP_REF_UPDATE;	/* Update the trap event counter */

    AREA_LIST(a) {
	a->spfsched |= build_rtr_lsa(a, &txq, 1);

	INTF_LIST(intf, a) {
	    if (intf->state == IDr)
		a->spfsched |= build_net_lsa(intf, &txq, TRUE);
	} INTF_LIST_END(intf, a) ;

	if (txq != LLNULL) {	/* may be locked out */
	    self_orig_area_flood(a, txq, LS_RTR);
	    ospf_freeq((struct Q **)&txq, ospf_lsdblist_index);
	}
	if (a->spfsched)
	    schedule++;
    } AREA_LIST_END(a) ;

    if (schedule) {
	ospf_spf_sched();
    }
    
    task_timer_set_interval(tip, LSRefreshTime);
}


/*
 *	LSA update interval timer for summary LSAs
 *	- generate for all areas
 */
/*ARGSUSED*/
void
tq_SumLsa __PF2(tip, task_timer *,
		interval, time_t)
{
    int reset = LSRefreshTime;

    TRAP_REF_UPDATE;	/* Update the trap event counter */

    /* will send to all areas */

    /* build_sum check buffers */
    if (build_sum())
	reset = 40;
    /* If ran out of buffers - try again in 40 seconds */

    task_timer_set_interval(tip, reset);
}


/*
 *	retransmit timer for this interface
 *		what to retransmit is based on state
 */
/*ARGSUSED*/
void
tq_retrans __PF2(tip, task_timer *,
		 interval, time_t)
{
    struct INTF *intf = (struct INTF *) tip->task_timer_data;
    struct NBR *n;

    TRAP_REF_UPDATE;	/* Update the trap event counter */

    NBRS_LIST(n, intf) {
	if (n->state < NEXSTART)
	    continue;

	if (n->state == NEXSTART) {
	    /* retransmit dbsum if any on queue */
	    if (n->dbsum != LSDB_SUM_NULL)
		send_dbsum(intf, n, IS_RETRANS);
	    else
		send_dbsum(intf, n, NOT_RETRANS);
	    continue;
	}
	if ((n->state == NEXCHANGE) &&
	    (n->mode == MASTER) &&
	    (n->dbsum != LSDB_SUM_NULL))
	    send_dbsum(intf, n, IS_RETRANS);

	if (n->state >= NEXCHANGE) {
	    if (!(NO_REQ(n)))
		send_req(intf, n, IS_RETRANS);

	    if (n->rtcnt) {
		send_lsu(n->retrans, OSPF_HASH_QUEUE, n, intf, IS_RETRANS);
	    }
	}
    } NBRS_LIST_END(n, intf) ;
}


/*
 *	Ack timer for this interface
 *		Send delayed ack if there area any
 */
/*ARGSUSED*/
void
tq_ack __PF2(tip, task_timer *,
	     interval, time_t)
{
    int acks = 0;
    struct AREA *area;
    struct INTF *intf;

    TRAP_REF_UPDATE;	/* Update the trap event counter */

    /* Send acks for all interfaces */
    AREA_LIST(area) {
	INTF_LIST(intf, area) {
	    if (intf->acks.ptr[NEXT]) {
		acks += send_ack(intf, NBRNULL, &intf->acks);	/* non-direct ack */
	    }
	} INTF_LIST_END(intf, area) ;
    } AREA_LIST_END(area) ;

    /* Send acks for all virtual interfaces */
    if (IAmBorderRtr && ospf.vcnt) {
	VINTF_LIST(intf) {
	    if (intf->acks.ptr[NEXT]) {
		acks += send_ack(intf, NBRNULL, &intf->acks);	/* non-direct ack */
	    }
	} VINTF_LIST_END(intf) ;
    }

    if (!acks) {
	/* No acks left, stop timer */

	task_timer_reset(tip);
    }
}


static int
dbage  __PF5(a, struct AREA *,
	     type, int,
	     trans, struct ospf_lsdb_list **,
	     start, int,
	     stop, int)
{
    register struct LSDB_HEAD *hp;
    struct ospf_lsdb_list *ll;
    u_int16 age;
    int spfsched = 0;

    TRAP_REF_UPDATE;	/* Update the trap event counter */

    /*
     * Age DB from start to stop
     * - Ase can age pieces at a time to avoid major floods
     */
    LSDB_HEAD_LIST(a->htbl[type], hp, start, stop) {
	register struct LSDB *db;

	LSDB_LIST(hp, db) {
	    /* MaxSeq and no acks, generate a new one */
	    if (db->lsdb_seq_max
		&& !db->lsdb_retrans
		&& !DB_FREEME(db)) {
		/* Need to re-originate */
		
		spfsched |= beyond_max_seq(a, (struct INTF *) 0, db, trans, trans, TRUE);
	    } else if (DB_WHERE(db) != ON_ASE_LIST
		       && ADV_AGE(db) >= MaxAge
		       && !db->lsdb_seq_max) {
		/* Currently active route? */
		
		if (db->lsdb_route || db->lsdb_asb_rtr) {
		    if ((LS_TYPE(db) != LS_SUM_NET
			 && LS_TYPE(db) != LS_SUM_ASB)
			|| !IAmBorderRtr
			|| a->area_id == OSPF_BACKBONE) {
			spfsched |= SCHED_BIT(type);
		    }

		    LS_AGE(db) = MaxAge;
		    if (DB_FREEME(db) != TRUE) {
			DB_REMQUE(db);
			DB_FREEME(db) = TRUE;
			DB_ADDQUE(ospf.db_free_list, db);
		    }
		    ll = (struct ospf_lsdb_list *) task_block_alloc(ospf_lsdblist_index);
		    ll->lsdb = db;
		    EN_Q((*trans), ll);
		} else if (DB_FREEME(db)) {
		    /* was set to be freed, free if conditions are met */

		    if (DB_CAN_BE_FREED(db)) {
			db_free(db, LS_TYPE(db));
		    }
		} else {
		    /* db->freeme hasn't been set yet */

		    LS_AGE(db) = MaxAge;
		    DB_REMQUE(db);
		    DB_FREEME(db) = TRUE;
		    DB_ADDQUE(ospf.db_free_list, db);
		    ll = (struct ospf_lsdb_list *) task_block_alloc(ospf_lsdblist_index);
		    ll->lsdb = db;
		    EN_Q((*trans), ll);
		}
		continue;
	    }

	    /* Rerun LS checksum */
	    age = LS_AGE(db);
	    LS_AGE(db) = 0;
	    if (ospf_checksum_bad(DB_RTR(db), ntohs(LS_LEN(db)))) {
		trace_log_tf(ospf.trace_options, 0, LOG_WARNING,
		     ("dbage: checksum failed: area %A type %d id %A adv %A",
			      sockbuild_in(0, a->area_id),
			      type,
			      sockbuild_in(0, LS_ID(db)),
			      sockbuild_in(0, ADV_RTR(db))));
		assert(FALSE);
	    }
	    LS_AGE(db) = age;
	} LSDB_LIST_END(hp, db) ;
    } LSDB_HEAD_LIST_END(a->htbl[type], hp, start, stop) ;

    return spfsched;
}



/*
 * tq_int_dbage
 *   	- Run on LS_RTR and LS_NET for all areas
 * 	- Recalculate DB checksum
 * 	- Check for MaxAge
 */
/*ARGSUSED*/
void
tq_int_age __PF2(t, task_timer *,
		 interval, time_t)
{
    int schedule = 0;
    struct AREA *a;
    /* list of lsdbs to be sent */
    struct ospf_lsdb_list *trans = LLNULL;

    TRAP_REF_UPDATE;	/* Update the trap event counter */

    AREA_LIST(a) {
	a->spfsched |= dbage(a, LS_RTR, &trans, 0, HTBLSIZE);
	a->spfsched |= dbage(a, LS_NET, &trans, 0, HTBLSIZE);

	/* something with a valid route has aged out */
	/*
	 * this should only happen when a router goes brain dead, shouldn't
	 * even happen
	 */
	/* if a db entry was newly discovered to be MaxAge, flood it */
	if (trans != LLNULL) {
	    self_orig_area_flood(a, trans, LS_RTR);
	    ospf_freeq((struct Q **)&trans, ospf_lsdblist_index);
	}
	if (a->spfsched)
	    schedule++;
    } AREA_LIST_END(a) ;

    if (schedule) {
	ospf_spf_sched();
    }
}

/*
 * tq_sum_age
 *   	- Run on LS_SUM_NET and LS_SUM_ASB for all areas
 * 	- Recalculate DB checksum
 * 	- Check for MaxAge
 */
/*ARGSUSED*/
void
tq_sum_age __PF2(t, task_timer *,
		 interval, time_t)
{
    int schedule = 0;
    struct AREA *a;
    struct ospf_lsdb_list *trans = LLNULL;

    TRAP_REF_UPDATE;	/* Update the trap event counter */

    AREA_LIST(a) {
	a->spfsched |= dbage(a, LS_SUM_NET, &trans, 0, HTBLSIZE);
	a->spfsched |= dbage(a, LS_SUM_ASB, &trans, 0, HTBLSIZE);
	/*
	 * if a db entry was newly discovered to be MaxAge, flood it
	 */
	if (trans != LLNULL) {
	    self_orig_area_flood(a, trans, LS_SUM_NET);
	    ospf_freeq((struct Q **)&trans, ospf_lsdblist_index);
	}
	if (a->spfsched)
	    schedule++;
    } AREA_LIST_END(a) ;

    if (schedule) {
	ospf_spf_sched();
    }
}


/*
 * tq_ase_age
 * 	- Age ase LSAs
 */
/*ARGSUSED*/
void
tq_ase_age __PF2(t, task_timer *,
		 interval, time_t)
{
    struct AREA *area = ospf.area.area_forw;
    struct ospf_lsdb_list *trans = LLNULL;
    int ase_age_end = ospf.ase_age_ndx + ASE_AGE_NDX_ADD;
    int ase_age_start = ospf.ase_age_ndx;

    TRAP_REF_UPDATE;	/* Update the trap event counter */

    if (ase_age_end >= HTBLSIZE) {
        ase_age_end = HTBLSIZE;
        ospf.ase_age_ndx = 0;
    } else {
        ospf.ase_age_ndx = ase_age_end;
    }

    area->spfsched |= dbage(area,
			    LS_ASE,
			    &trans,
			    ase_age_start,
			    ase_age_end);

    /*
     * if a db entry was newly discovered to be MaxAge, flood it
     */
    if (trans != LLNULL) {
	struct AREA *a;

	AREA_LIST(a) {
	    if (!BIT_TEST(a->area_flags, OSPF_AREAF_STUB)) {
                self_orig_area_flood(a, trans, LS_ASE);
	    }
	} AREA_LIST_END(a) ;

	ospf_freeq((struct Q **) &trans, ospf_lsdblist_index);
    }
    /*
     * something with a valid route has aged out
     */
    if (area->spfsched)
	ospf_spf_sched();
}
