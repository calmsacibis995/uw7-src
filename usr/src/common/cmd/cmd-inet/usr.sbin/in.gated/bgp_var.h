#ident	"@(#)bgp_var.h	1.3"
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


/*
 *	BGP Private Definitions
 */

/*
 * Define for debugging
 */
#define	BGPDEBUG	/* nothing */

/*
 * Assert replacement for debugging inline macros
 */
#ifdef	BGPDEBUG
#define	BGP_BLEWIT()	(bgp_blewit(__FILE__, __LINE__))
#endif	/* BGPDEBUG */

/*
 * The default size we set receive and send buffers to, before we have
 * figured this out by looking at the peer/group configuration
 */
#define	BGP_RECV_BUFSIZE	((size_t)(2*BGPMAXPACKETSIZE))
#define	BGP_SEND_BUFSIZE	((size_t)(2*BGPMAXPACKETSIZE))

/*
 * The number of bytes we read out of the TCP socket at once before
 * allowing other things to happen.  I think we can get 20kb out on
 * an RT in a few seconds if we're not tracing, so this may be okay.
 *
 * I dropped this to 18kb to try to keep things a bit more chipper.
 * I drop it to 8kb when we are tracing packets to try to keep the
 * time we spend in the routine below a couple of seconds.
 */
#define	BGPMAXREAD		18432
#define	BGPMAXREADTRACEPKTS	8192

/*
 * XXX I'm not sure what this is for.  Oh, well.
 */
#define	BGP_CLOSE_TIMER		45

/*
 * We schedule connect attempts into slots to try to minimize the number
 * of peers which we attempt to connect to simultaneously.  The slot size
 * is the basic unit of time granularity for specifying connect times.
 */
#define	BGPCONN_SLOTSIZE	4	/* 4 second slots, must be power of 2 */
#define	BGPCONN_SLOTSHIFT	2	/* 4 == 2^2 */
#define	BGPCONN_INSLOTMASK	(BGPCONN_SLOTSIZE - 1)
#define	BGPCONN_N_SLOTS		64	/* 256 seconds worth of slots */
#define	BGPCONN_SLOTMASK	(BGPCONN_N_SLOTS - 1)
#define	BGPCONN_SLOTDELAY	5	/* delay by up to 5 slots (20 sec) */

#define	BGPCONN_SLOT(interval) \
    ((((interval)+bgp_time_sec) >> BGPCONN_SLOTSHIFT) & BGPCONN_SLOTMASK)
#define	BGPCONN_NEXTSLOT(slot) (((slot) + 1) & BGPCONN_SLOTMASK)
#define	BGPCONN_DONE(bnp) \
    do { \
	if (bnp->bgp_connect_slot != 0) { \
	    bgp_connect_slots[bnp->bgp_connect_slot - 1]--; \
	    bnp->bgp_connect_slot = 0; \
	} \
    } while (0)

/*
 * How long to wait before retrying connect (should be multple of slot size)
 */
#define	BGPCONN_SHORT		32
#define	BGPCONN_MED		64
#define	BGPCONN_LONG		148
#define	BGPCONN_INIT		12	/* after initialization */
#define	BGPCONN_IFUP		4	/* after interface comes up */

#define	BGPCONN_F_SHORT		2
#define	BGPCONN_F_MED		5

#define	BGPCONN_INTERVAL(bnp) \
    (((bnp)->bgp_connect_failed <= BGPCONN_F_SHORT) ? BGPCONN_SHORT : \
    (((bnp)->bgp_connect_failed <= BGPCONN_F_MED) ? BGPCONN_MED : BGPCONN_LONG))

/*
 * Where BGP gets the time.
 */
#define	bgp_time_sec	time_sec	/* use system time */

/*
 * Timeout for open message reception.  Set this equal to the holdtime for now.
 */
#define	BGP_OPEN_TIMEOUT	BGP_HOLDTIME

/*
 * Timeout for listening after initialization.  Should be
 * shorter than BGPCONN_INIT
 */
#define	BGP_LISTEN_TIMEOUT	10

/*
 * Best and worst versions, for version negotiation.
 */
#define	BGP_BEST_VERSION	BGP_VERSION_4
#define	BGP_WORST_VERSION	BGP_VERSION_2

/*
 * Macroes to determine if a peer group is a type which requires a
 * shared network, or if it is a group which can use this information
 */
#define	BGP_NEEDS_SHAREDIF(bgp) \
    ((bgp)->bgpg_type == BGPG_EXTERNAL || (bgp)->bgpg_type == BGPG_INTERNAL)

#define	BGP_USES_SHAREDIF(bgp) \
    ((bgp)->bgpg_type != BGPG_INTERNAL_RT \
      && (bgp)->bgpg_type != BGPG_INTERNAL_IGP)

#define	BGP_OPTIONAL_SHAREDIF(bgp) \
    ((bgp)->bgpg_type == BGPG_TEST)

/*
 * Macros to determine if a peer group is an external or internal type.
 */
#define	BGP_GROUP_EXTERNAL(bgp) \
    ((bgp)->bgpg_type == BGPG_EXTERNAL || (bgp)->bgpg_type == BGPG_TEST)

#define	BGP_GROUP_INTERNAL(bgp)	(!BGP_GROUP_EXTERNAL((bgp)))

/*
 * Initialize the adv queue.
 */
#define	BGP_ADVQ_INIT(vqp) \
    ((vqp)->bgpv_next = (vqp)->bgpv_prev = (vqp))

/*
 * True when the adv queue is empty
 */
#define	BGP_ADVQ_EMPTY(vqp)	((vqp)->bgpv_next == (vqp))

/*
 * Find the first entry on the queue
 */
#define	BGP_ADV_FIRST(vqp)	(((vqp)->bgpv_next == (vqp)->bgpv_prev) \
    ? ((bgp_adv_entry *) 0) : ((bgp_adv_entry *)((vqp)->bgpv_next)))

/*
 * Find the next entry on the queue
 */
#define	BGP_ADV_NEXT(vqp, entp)	(((entp)->bgpe_next == (vqp)) \
    ? ((bgp_adv_entry *) 0) : ((bgp_adv_entry *)((entp)->bgpe_next)))

/*
 * Add an adv entry to a queue after the pointer
 */
#ifdef	BGPDEBUG
#define	BGP_ADV_ADD_AFTER(vqp, entp) \
    do { \
	register bgp_adv_queue *Xvqp = (vqp); \
	register bgp_adv_queue *Xentp = &((entp)->bgpe_q_entry); \
	assert(!Xentp->bgpv_next && !Xentp->bgpv_prev); \
	Xentp->bgpv_prev = Xvqp; \
	(Xentp->bgpv_next = Xvqp->bgpv_next)->bgpv_prev = Xentp; \
	Xvqp->bgpv_next = Xentp; \
    } while (0)
#else	/* BGPDEBUG */
    do { \
	register bgp_adv_queue *Xvqp = (vqp); \
	register bgp_adv_queue *Xentp = &((entp)->bgpe_q_entry); \
	Xentp->bgpv_prev = Xvqp; \
	(Xentp->bgpv_next = Xvqp->bgpv_next)->bgpv_prev = Xentp; \
	Xvqp->bgpv_next = Xentp; \
    } while (0)
#endif	/* BGPDEBUG */

/*
 * Add an adv entry to a queue before the pointer
 */
#ifdef	BGPDEBUG
#define	BGP_ADV_ADD_BEFORE(vqp, entp) \
    do { \
	register bgp_adv_queue *Xvqp = (vqp); \
	register bgp_adv_queue *Xentp = &((entp)->bgpe_q_entry); \
	assert(!Xentp->bgpv_next && !Xentp->bgpv_prev); \
	Xentp->bgpv_next = Xvqp; \
	(Xentp->bgpv_prev = Xvqp->bgpv_prev)->bgpv_next = Xentp; \
	Xvqp->bgpv_prev = Xentp; \
    } while (0)
#else	/* BGPDEBUG */
    do { \
	register bgp_adv_queue *Xvqp = (vqp); \
	register bgp_adv_queue *Xentp = &((entp)->bgpe_q_entry); \
	Xentp->bgpv_next = Xvqp; \
	(Xentp->bgpv_prev = Xvqp->bgpv_prev)->bgpv_next = Xentp; \
	Xvqp->bgpv_prev = Xentp; \
    } while (0)
#endif	/* BGPDEBUG */

/*
 * Dequeue an adv entry
 */
#ifdef	BGPDEBUG
#define	BGP_ADV_DEQUEUE(entp) \
    do { \
	register bgp_adv_queue *Xentp = &((entp)->bgpe_q_entry); \
	assert(Xentp->bgpv_prev->bgpv_next == Xentp \
	    && Xentp->bgpv_next->bgpv_prev == Xentp); \
	Xentp->bgpv_next->bgpv_prev = Xentp->bgpv_prev; \
	Xentp->bgpv_prev->bgpv_next = Xentp->bgpv_next; \
	Xentp->bgpv_prev = Xentp->bgpv_next = (bgp_adv_queue *) 0; \
    } while (0)
#else	/* BGPDEBUG */
    do { \
	register bgp_adv_queue *Xentp = &((entp)->bgpe_q_entry); \
	Xentp->bgpv_next->bgpv_prev = Xentp->bgpv_prev; \
	Xentp->bgpv_prev->bgpv_next = Xentp->bgpv_next; \
    } while (0)
#endif	/* BGPDEBUG */


/*
 * Macro to unlink an rti entry
 */
#ifdef	BGPDEBUG
#define	BGP_RTI_UNLINK(rti) \
    do { \
	register bgp_rti_entry *Xrti = (rti); \
	assert(Xrti->bgpi_prev->bgpi_next == Xrti \
	    && Xrti->bgpi_next->bgpi_prev == Xrti); \
	Xrti->bgpi_prev->bgpi_next = Xrti->bgpi_next; \
	Xrti->bgpi_next->bgpi_prev = Xrti->bgpi_prev; \
	Xrto->bgpi_next = Xtri->bgpi_prev = (bgp_rti_entry *) 0; \
    } while (0)
#else	/* BGPDEBUG */
    do { \
	register bgp_rti_entry *Xrti = (rti); \
	Xrti->bgpi_prev->bgpi_next = Xrti->bgpi_next; \
	Xrti->bgpi_next->bgpi_prev = Xrti->bgpi_prev; \
    } while (0)
#endif	/* BGPDEBUG */

/*
 * Macro to unlink an rto entry
 */
#ifdef	BGPDEBUG
#define	BGP_RTO_UNLINK(rto) \
    do { \
	register bgp_rto_entry *Xrto = (rto); \
	assert(Xrto->bgpo_prev->bgpo_next == Xrto \
	    && Xrto->bgpo_next->bgpo_prev == Xrto); \
	Xrto->bgpo_prev->bgpo_next = Xrto->bgpo_next; \
	Xrto->bgpo_next->bgpo_prev = Xrto->bgpo_prev; \
	Xrto->bgpo_next = Xrto->bgpo_prev = (bgp_rto_entry *) 0; \
    } while (0)
#else	/* BGPDEBUG */
#define	BGP_RTO_UNLINK(rto) \
    do { \
	register bgp_rto_entry *Xrto = (rto); \
	Xrto->bgpo_prev->bgpo_next = Xrto->bgpo_next; \
	Xrto->bgpo_next->bgpo_prev = Xrto->bgpo_prev; \
    } while (0)
#endif	/* BGPDEBUG */


/*
 * Macro to unlink a grto entry
 */
#ifdef	BGPDEBUG
#define	BGP_GRTO_UNLINK(grto) \
    do { \
	register bgpg_rto_entry *Xgrto = (grto); \
	assert(Xgrto->bgpgo_prev->bgpgo_next == Xgrto \
	    && Xgrto->bgpgo_next->bgpgo_prev == Xgrto); \
	Xgrto->bgpgo_prev->bgpgo_next = Xgrto->bgpgo_next; \
	Xgrto->bgpgo_next->bgpgo_prev = Xgrto->bgpgo_prev; \
	Xgrto->bgpgo_next = Xgrto->bgpgo_prev = (bgpg_rto_entry *) 0; \
    } while (0)
#else	/* BGPDEBUG */
    do { \
	register bgpg_rto_entry *Xgrto = (grto); \
	Xgrto->bgpgo_prev->bgpgo_next = Xgrto->bgpgo_next; \
	Xgrto->bgpgo_next->bgpgo_prev = Xgrto->bgpgo_prev; \
    } while (0)
#endif	/* BGPDEBUG */

/*
 * Macro to remove a group rtinfo entry
 */
#ifdef	BGPDEBUG
#define	BGP_RTINFO_UNLINK(grto, rtinfo) \
    do { \
	register bgpg_rtinfo_entry *Xinfo = (rtinfo); \
	register bgpg_rto_entry *Xgrto = (grto); \
	if (Xgrto->bgpgo_info == Xinfo) { \
	    Xgrto->bgpgo_info = Xinfo->bgp_info_next; \
	} else { \
	    register bgpg_rtinfo_entry *Xinfo_prev = Xgrto->bgpgo_info; \
	    do { \
		if (Xinfo_prev->bgp_info_next == Xinfo) { \
		    Xinfo_prev->bgp_info_next = Xinfo->bgp_info_next; \
		    break; \
		} \
	    } while ((Xinfo_prev = Xinfo_prev->bgp_info_next)); \
	    assert(Xinfo_prev); \
	} \
    } while (0)
#else	/* BGPDEBUG */
    do { \
	register bgpg_rtinfo_entry *Xinfo = (rtinfo); \
	register bgpg_rto_entry *Xgrto = (grto); \
	if (Xgrto->bgpgo_info == Xinfo) { \
	    Xgrto->bgpgo_info = Xinfo->bgp_info_next; \
	} else { \
	    register bgpg_rtinfo_entry *Xinfo_prev = Xgrto->bgpgo_info; \
	    do { \
		if (Xinfo_prev->bgp_info_next == Xinfo) { \
		    Xinfo_prev->bgp_info_next = Xinfo->bgp_info_next; \
		    break; \
		} \
	    } while ((Xinfo_prev = Xinfo_prev->bgp_info_next)); \
	} \
    } while (0)
#endif	/* BGPDEBUG */




/*
 * Given an rt queue pointer, determine the asp list address
 */
#define	ASPL_FROM_QUEUE(qp) \
    ((bgp_asp_list *)((void_t)(((byte *)(qp)) \
    - offsetof(bgp_asp_list, bgpl_asp_queue))))

/*
 * Initialize the queue head for the AS path list
 */
#define	BGP_RTQ_INIT(qp) \
    do { \
	register bgp_rt_queue *Xqp = (qp); \
	Xqp->bgpq_prev = Xqp->bgpq_next = Xqp; \
    } while (0);

/*
 * True when the asp route queue is empty
 */
#define	BGP_RTQ_EMPTY(qp)	((qp)->bgpq_next == (qp))

/*
 * Initialize an asp list
 */
#define	BGP_ASPL_INIT(aspl) \
    do { \
	register bgp_asp_list *Xaspl = (aspl); \
	Xaspl->bgpl_rto_prev = Xaspl->bgpl_rto_next \
	  = (bgp_rto_entry *)Xaspl; \
    } while (0)

/*
 * Add an AS path list before the specified queue entry
 */
#ifdef	BGPDEBUG
#define	BGP_ASPL_ADD_BEFORE(qp, aspl) \
    do { \
	register bgp_rt_queue *Xqp = (qp); \
	register bgp_rt_queue *Xaqp = &((aspl)->bgpl_asp_queue); \
	assert(!Xaqp->bgpq_next && !Xaqp->bgpq_prev); \
	Xaqp->bgpq_next = Xqp; \
	(Xaqp->bgpq_prev = Xqp->bgpq_prev)->bgpq_next = Xaqp; \
	Xqp->bgpq_prev = Xaqp; \
    } while (0)
#else	/* BGPDEBUG */
    do { \
	register bgp_rt_queue *Xqp = (qp); \
	register bgp_rt_queue *Xaqp = &((aspl)->bgpl_asp_queue); \
	Xaqp->bgpq_next = Xqp; \
	(Xaqp->bgpq_prev = Xqp->bgpq_prev)->bgpq_next = Xaqp; \
	Xqp->bgpq_prev = Xaqp; \
    } while (0)
#endif	/* BGPDEBUG */

/*
 * Add an AS path list after the specified queue entry
 */
#ifdef	BGPDEBUG
#define	BGP_ASPL_ADD_AFTER(qp, aspl) \
    do { \
	register bgp_rt_queue *Xqp = (qp); \
	register bgp_rt_queue *Xaqp = &((aspl)->bgpl_asp_queue); \
	assert(!Xaqp->bgpq_next && !Xaqp->bgpq_prev); \
	Xaqp->bgpq_prev = Xqp; \
	(Xaqp->bgpq_next = Xqp->bgpq_next)->bgpq_prev = Xaqp; \
	Xqp->bgpq_next = Xaqp; \
    } while (0)
#else	/* BGPDEBUG */
#define	BGP_ASPL_ADD_AFTER(qp, aspl) \
    do { \
	register bgp_rt_queue *Xqp = (qp); \
	register bgp_rt_queue *Xaqp = &((aspl)->bgpl_asp_queue); \
	Xaqp->bgpq_prev = Xqp; \
	(Xaqp->bgpq_next = Xqp->bgpq_next)->bgpq_prev = Xaqp; \
	Xqp->bgpq_next = Xaqp; \
    } while (0)
#endif	/* BGPDEBUG */

/*
 * Remove an AS path list from the queue, making sure the hash
 * is up to date.
 */
#ifdef	BGPDEBUG
#define	BGP_ASPL_REMOVE(qp, aspl) \
    do { \
	register bgp_rt_queue *Xqp = (qp); \
	register bgp_asp_list *Xaspl = (aspl); \
	assert(Xaspl->bgpl_q_prev->bgpq_next == &Xaspl->bgpl_asp_queue \
	    && Xaspl->bgpl_q_next->bgpq_prev == &Xaspl->bgpl_asp_queue); \
	if (Xaspl->bgpl_asp) { \
	    register int Xhash = Xaspl->bgpl_asp->path_hash; \
	    assert(Xaspl->bgpl_asp_hash_check == Xhash \
		&& (Xqp->bgpq_asp_hash)[Xhash]); \
	    if ((Xqp->bgpq_asp_hash)[Xhash] == Xaspl) { \
		if (Xaspl->bgpl_q_next == Xqp \
		  || Xaspl->bgpl_q_next->bgpq_asp->path_hash != Xhash) { \
		    (Xqp->bgpq_asp_hash)[Xhash] = (bgp_asp_list *) 0; \
		} else { \
		    assert(Xaspl->bgpl_q_next->bgpq_asp_hash_check == Xhash); \
		    (Xqp->bgpq_asp_hash)[Xhash] \
		        = ASPL_FROM_QUEUE(Xaspl->bgpl_q_next); \
		} \
	    } \
	} else { \
	    assert(Xaspl->bgpl_asp_hash_check == (-1)); \
	} \
	Xaspl->bgpl_q_next->bgpq_prev = Xaspl->bgpl_q_prev; \
	Xaspl->bgpl_q_prev->bgpq_next = Xaspl->bgpl_q_next; \
	Xaspl->bgpl_q_prev = Xaspl->bgpl_q_next = (bgp_rt_queue *) 0; \
    } while (0)
#else	/* BGPDEBUG */
#define	BGP_ASPL_REMOVE(qp, aspl) \
    do { \
	register bgp_rt_queue *Xqp = (qp); \
	register bgp_asp_list *Xaspl = (aspl); \
	if (Xaspl->bgpl_asp) { \
	    register int Xhash = Xaspl->bgpl_asp->path_hash; \
	    if ((Xqp->bgpq_asp_hash)[Xhash] == Xaspl) { \
		if (Xaspl->bgpl_q_next == Xqp \
		  || Xaspl->bgpl_q_next->bgpq_asp->path_hash != Xhash) { \
		    (Xqp->bgpq_asp_hash)[Xhash] = (bgp_asp_list *) 0; \
		} else { \
		    (Xqp->bgpq_asp_hash)[Xhash] \
		        = ASPL_FROM_QUEUE(Xaspl->bgpl_q_next); \
		} \
	    } \
	} \
	Xaspl->bgpl_q_next->bgpq_prev = Xaspl->bgpl_q_prev; \
	Xaspl->bgpl_q_prev->bgpq_next = Xaspl->bgpl_q_next; \
    } while (0)
#endif	/* BGPDEBUG */

/*
 * True when an asp list has no queued routes
 */
#define	BGP_ASPL_EMPTY(aspl) \
	((aspl)->bgpl_rto_next == ((bgp_rto_entry *)(aspl)))

/*
 * Return the first AS path list on a queue, null if none
 */
#ifdef	BGPDEBUG
#define	BGP_ASPL_FIRST(qp)	(((qp)->bgpq_next == (qp)) \
    ? ((bgp_asp_list *) 0) : (((qp)->bgpq_prev && \
    (qp)->bgpq_next->bgpq_prev == (qp)) \
    ? ASPL_FROM_QUEUE((qp)->bgpq_next) : ((bgp_asp_list *)BGP_BLEWIT())))
#else	/* BGPDEBUG */
#define	BGP_ASPL_FIRST(qp)	(((qp)->bgpq_next == (qp)) \
    ? ((bgp_asp_list *) 0) : ASPL_FROM_QUEUE((qp)->bgpq_next))
#endif	/* BGPDEBUG */

/*
 * Return the next AS path list on a queue, null if none
 */
#ifdef	BGPDEBUG
#define	BGP_ASPL_NEXT(qp, aspl)	(((aspl)->bgpl_q_next == (qp)) \
    ? ((bgp_asp_list *) 0) : (((aspl)->bgpl_q_prev \
    && ASPL_FROM_QUEUE((aspl)->bgpl_q_next->bgpq_prev) == (aspl)) \
    ? ASPL_FROM_QUEUE((aspl)->bgpl_q_next) : ((bgp_asp_list *)BGP_BLEWIT())))
#else	/* BGPDEBUG */
#define	BGP_ASPL_NEXT(qp, aspl)	(((aspl)->bgpl_q_next == (qp)) \
    ? ((bgp_asp_list *) 0) : ASPL_FROM_QUEUE((aspl)->bgpl_q_next))
#endif	/* BGPDEBUG */


/*
 * Add an rto entry to the end of the aspl list.
 */
#ifdef	BGPDEBUG
#define	BGP_RTO_ADD_END(aspl, rto) \
    do { \
	register bgp_asp_list *Xaspl = (aspl); \
	register bgp_rto_entry *Xrto = (rto); \
	assert(!Xrto->bgpo_prev && !Xrto->bgpo_next); \
	Xrto->bgpo_next = (bgp_rto_entry *)Xaspl; \
	(Xrto->bgpo_prev = Xaspl->bgpl_rto_prev)->bgpo_next = Xrto; \
	Xaspl->bgpl_rto_prev = Xrto; \
    } while (0)
#else	/* BGPDEBUG */
    do { \
	register bgp_asp_list *Xaspl = (aspl); \
	register bgp_rto_entry *Xrto = (rto); \
	Xrto->bgpo_next = (bgp_rto_entry *)Xaspl; \
	(Xrto->bgpo_prev = Xaspl->bgpl_rto_prev)->bgpo_next = Xrto; \
	Xaspl->bgpl_rto_prev = Xrto; \
    } while (0)
#endif	/* BGPDEBUG */

/*
 * Add a grto entry to the end of the aspl list.
 */
#ifdef	BGPDEBUG
#define	BGP_GRTO_ADD_END(aspl, grto) \
    do { \
	register bgp_asp_list *Xaspl = (aspl); \
	register bgpg_rto_entry *Xgrto = (grto); \
	assert(!Xgrto->bgpgo_prev && !Xgrto->bgpgo_next); \
	Xgrto->bgpgo_next = (bgpg_rto_entry *)Xaspl; \
	(Xgrto->bgpgo_prev = Xaspl->bgpl_grto_prev)->bgpgo_next = Xgrto; \
	Xaspl->bgpl_grto_prev = Xgrto; \
    } while (0)
#else	/* BGPDEBUG */
#define	BGP_GRTO_ADD_END(aspl, grto) \
    do { \
	register bgp_asp_list *Xaspl = (aspl); \
	register bgpg_rto_entry *Xgrto = (grto); \
	Xgrto->bgpgo_next = (bgpg_rto_entry *)Xaspl; \
	(Xgrto->bgpgo_prev = Xaspl->bgpl_grto_prev)->bgpgo_next = Xgrto; \
	Xaspl->bgpl_grto_prev = Xgrto; \
    } while (0)
#endif	/* BGPDEBUG */

/*
 * Add an rto entry to the head of the aspl list.
 */
#ifdef	BGPDEBUG
#define	BGP_RTO_ADD_HEAD(aspl, rto) \
    do { \
	register bgp_asp_list *Xaspl = (aspl); \
	register bgp_rto_entry *Xrto = (rto); \
	assert(!Xrto->bgpo_prev && !Xrto->bgpo_next); \
	Xrto->bgpo_prev = (bgp_rto_entry *)Xaspl; \
	(Xrto->bgpo_next = Xaspl->bgpl_rto_next)->bgpo_prev = Xrto; \
	Xaspl->bgpl_rto_next = Xrto; \
    } while (0)
#else	/* BGPDEBUG */
#define	BGP_RTO_ADD_HEAD(aspl, rto) \
    do { \
	register bgp_asp_list *Xaspl = (aspl); \
	register bgp_rto_entry *Xrto = (rto); \
	Xrto->bgpo_prev = (bgp_rto_entry *)Xaspl; \
	(Xrto->bgpo_next = Xaspl->bgpl_rto_next)->bgpo_prev = Xrto; \
	Xaspl->bgpl_rto_next = Xrto; \
    } while (0)
#endif	/* BGPDEBUG */

/*
 * Add a grto entry to the head of the aspl list.
 */
#ifdef	BGPDEBUG
#define	BGP_GRTO_ADD_HEAD(aspl, grto) \
    do { \
	register bgp_asp_list *Xaspl = (aspl); \
	register bgpg_rto_entry *Xgrto = (grto); \
	assert(!Xgrto->bgpgo_prev && !Xgrto->bgpgo_next); \
	Xgrto->bgpgo_prev = (bgpg_rto_entry *)Xaspl; \
	(Xgrto->bgpgo_next = Xaspl->bgpl_grto_next)->bgpgo_prev = Xgrto; \
	Xaspl->bgpl_grto_next = Xgrto; \
    } while (0)
#else	/* BGPDEBUG */
#define	BGP_GRTO_ADD_HEAD(aspl, grto) \
    do { \
	register bgp_asp_list *Xaspl = (aspl); \
	register bgpg_rto_entry *Xgrto = (grto); \
	Xgrto->bgpgo_prev = (bgpg_rto_entry *)Xaspl; \
	(Xgrto->bgpgo_next = Xaspl->bgpl_grto_next)->bgpgo_prev = Xgrto; \
	Xaspl->bgpl_grto_next = Xgrto; \
    } while (0)
#endif	/* BGPDEBUG */

/*
 * Add an rto entry after the current entry
 */
#ifdef	BGPDEBUG
#define	BGP_RTO_ADD_AFTER(rtoprev, rto) \
    do { \
	register bgp_rto_entry *Xrto = (rto); \
	register bgp_rto_entry *Xrtoprev = (rtoprev); \
	assert(!Xrto->bgpo_prev && !Xrto->bgpo_next && Xrtoprev->bgpo_prev); \
	Xrto->bgpo_prev = Xrtoprev; \
	(Xrto->bgpo_next = Xrtoprev->bgpo_next)->bgpo_prev = Xrto; \
	Xrtoprev->bgpo_next = Xrto; \
    } while (0)
#else	/* BGPDEBUG */
#define	BGP_RTO_ADD_AFTER(rtoprev, rto) \
    do { \
	register bgp_rto_entry *Xrto = (rto); \
	register bgp_rto_entry *Xrtoprev = (rtoprev); \
	Xrto->bgpo_prev = Xrtoprev; \
	(Xrto->bgpo_next = Xrtoprev->bgpo_next)->bgpo_prev = Xrto; \
	Xrtoprev->bgpo_next = Xrto; \
    } while (0)
#endif	/* BGPDEBUG */


/*
 * Add a grto entry after the current entry
 */
#ifdef	BGPDEBUG
#define	BGP_GRTO_ADD_AFTER(grtoprev, grto) \
    do { \
	register bgpg_rto_entry *Xgrto = (grto); \
	register bgpg_rto_entry *Xgrtoprev = (grtoprev); \
	assert(!Xgrto->bgpgo_prev \
	    && !Xgrto->bgpgo_next \
	    && Xgrtoprev->bgpgo_prev); \
	Xgrto->bgpgo_prev = Xgrtoprev; \
	(Xgrto->bgpgo_next = Xgrtoprev->bgpgo_next)->bgpgo_prev = Xgrto; \
	Xgrtoprev->bgpgo_next = Xgrto; \
    } while (0)
#else	/* BGPDEBUG */
#define	BGP_GRTO_ADD_AFTER(grtoprev, grto) \
    do { \
	register bgpg_rto_entry *Xgrto = (grto); \
	register bgpg_rto_entry *Xgrtoprev = (grtoprev); \
	Xgrto->bgpgo_prev = Xgrtoprev; \
	(Xgrto->bgpgo_next = Xgrtoprev->bgpgo_next)->bgpgo_prev = Xgrto; \
	Xgrtoprev->bgpgo_next = Xgrto; \
    } while (0)
#endif	/* BGPDEBUG */



/*
 * Peer bitmask processing.  We use bitmasks to keep track of which
 * peers in non-external groups need to receive which routes, and when.
 */
#define	BGP_MAXBITS	64	/* enough for 2000 peers in a group */

#define	BGPB_NBBY	8			/* XXX assumes NBBY >= 8 */
#define	BGPB_BITSBITS	(sizeof(bgp_bits) * BGPB_NBBY)
#define	BGPB_MASK	(BGPB_BITSBITS - 1)	/* XXX bits/long power of 2 */
#define	BGPB_DIV	(BGPB_BITSBITS)		/* hope it turns / into >> */

#define	BGPB_WORD(bit)		((bit) / BGPB_DIV)
#define	BGPB_ENT(bits, bit)	((bits)[BGPB_WORD(bit)])
#define	BGPB_WBIT(bit)		(((u_long)(1)) << ((bit) & BGPB_MASK))

/*
 * Versions to use when you know the word and the wbit
 */
#define	BGPB_WB_TEST(bits, word, wbit) \
    BIT_TEST(((bits)[(word)]), (wbit))

#define	BGPB_WB_SET(bits, word, wbit) \
    BIT_SET(((bits)[(word)]), (wbit))

#define	BGPB_WB_RESET(bits, word, wbit) \
    BIT_RESET(((bits)[(word)]), (wbit))

/*
 * Versions to use when you only know the bit number
 */
#define	BGPB_BTEST(bits, bit) \
    BGPB_WB_TEST((bits), BGPB_WORD((bit)), BGPB_WBIT((bit)))

#define	BGPB_BSET(bits, bit) \
    BGPB_WB_SET((bits), BGPB_WORD((bit)), BGPB_WBIT((bit)))

#define	BGPB_BRESET(bits, bit) \
    BGPB_WB_RESET((bits), BGPB_WORD((bit)), BGPB_WBIT((bit)))

/*
 * See if two bit sets match exactly.  Evaluates the len argument twice.
 */
#define	BGPB_MATCH(bits1, bits2, len, ismatch) \
    do { \
	if ((len) == 1) { \
	    (ismatch) = (*(bits1) == *(bits2)); \
	} else { \
	    register bgp_bits *Xb1 = (bits1); \
	    register bgp_bits *Xb2 = (bits2); \
	    register bgp_bits *Xbend = Xb1 + (len); \
	    do { \
		if (*Xb1 != *Xb2) { \
		    break; \
		} \
		Xb1++; \
		Xb2++; \
	    } while (Xb1 < Xbend); \
	    (ismatch) = (Xb1 == Xbend); \
	} \
    } while (0)

/*
 * See if two bit sets have bits in common.  Evaluates the len argument twice
 */
#define	BGPB_COMMON(bits1, bits2, len, hascommonbits) \
    do { \
	if ((len) == 1) { \
	    (hascommonbits) = ((*(bits1) & *(bits2)) != 0); \
	} else { \
	    register bgp_bits *Xb1 = (bits1); \
	    register bgp_bits *Xb2 = (bits2); \
	    register bgp_bits *Xbend = Xb1 + (len); \
	    do { \
		if (((*Xb1) & (*Xb2)) != 0) { \
		    break; \
		} \
		Xb1++; \
		Xb2++; \
	    } while (Xb1 < Xbend); \
	    (hascommonbits) = (Xb1 != Xbend); \
	} \
    } while (0)

/*
 * Clear the bits set in bits2clear from bits.  Return an indication
 * if bits is entirely clear afterwards.  Evaluates len twice.
 */
#define	BGPB_CLEAR(bits, bits2clear, len, allclear) \
    do { \
	if ((len) == 1) { \
	    (allclear) = ((*(bits) &= ~(*(bits2clear))) == 0); \
	} else { \
	    register bgp_bits *Xb = (bits); \
	    register bgp_bits *Xb2c = (bits2clear); \
	    register bgp_bits *Xbend = Xb + (len); \
	    register int Xstillset = 0; \
	    while (Xb < Xbend) { \
		BIT_RESET(*Xb, *Xb2c); \
		if (*Xb != 0) { \
		    Xstillset++; \
		} \
		Xb++; \
		Xb2c++; \
	    } \
	    (allclear) = (Xstillset == 0); \
	} \
    } while (0)

/*
 * Check to see if a set of bits is all zero
 */
#define	BGPB_CHECK_CLEAR(bits, len, allclear) \
    do { \
	if ((len) == 1) { \
	    (allclear) = (*(bits) == 0); \
	} else { \
	    register bgp_bits *Xb = (bits); \
	    register bgp_bits *Xbend = Xb + (len); \
	    register int Xres = 1; \
	    do { \
		if (*Xb++ != 0) { \
		    Xres = 0; \
		    break; \
		} \
	    } while (Xb < Xbend); \
	    (allclear) = Xres; \
	} \
    } while (0)

/*
 * Determine if all the bits in bitsmbset are set in bits.  Len evaluated twice.
 */
#define	BGPB_MBSET(bits, bitsmbset, len, allset) \
    do { \
	register bgp_bits *Xbmbs = (bitsmbset); \
	if ((len) == 1) { \
	    (allset) = ((*(bits) & *Xbmbs) == *Xbmbs); \
	} else { \
	    register bgp_bits *Xb = (bits); \
	    register bgp_bits *Xbend = Xb + (len); \
	    do { \
		if ((*Xb & *Xbmbs) != *Xbmbs) { \
		    break; \
		} \
		Xb++; \
		Xbmbs++; \
	    } while (Xb < Xbend); \
	    (allset) = (Xb == Xbend); \
	} \
    } while (0)

/*
 * Or a set of bits into another set of bits
 */
#define	BGPB_SET(bits, bits2set, len) \
    do { \
	if ((len) == 1) { \
	    *(bits) |= *(bits2set); \
	} else { \
	    register bgp_bits *Xb = (bits); \
	    register bgp_bits *Xb2set = (bits2set); \
	    register int Xlen = (len); \
	    do { \
		*Xb++ |= *Xb2set++; \
	    } while (--Xlen > 0); \
	} \
    } while (0)

/*
 * Reset in bits the bits occuring in bits2reset
 */
#define	BGPB_RESET(bits, bits2reset, len) \
    do { \
	if ((len) == 1) { \
	    *(bits) &= ~(*(bits2reset)); \
	} else { \
	    register bgp_bits *Xb = (bits); \
	    register bgp_bits *Xb2reset = (bits2reset); \
	    register int Xlen = (len); \
	    do { \
		*Xb++ &= ~(*Xb2reset++); \
	    } while (--Xlen > 0); \
	} \
    } while (0)

/*
 * And a set of bits with another set of bits.  Return true if there
 * are any matching bits.
 */
#define	BGPB_AND_CHECK(bits, bits2and, len, someset) \
    do { \
	if ((len) == 1) { \
	    (someset) = ((*(bits) &= *(bits2and)) != 0); \
	} else { \
	    register bgp_bits *Xb = (bits); \
	    register bgp_bits *Xb2and = (bits2and); \
	    register int Xlen = (len); \
	    register int Xset = 0; \
	    do { \
		if ((*Xb++ &= *Xb2and++) != 0) { \
		    Xset = 1; \
		} \
	    } while (--Xlen > 0); \
	    (someset) = Xset; \
	} \
    } while (0)

/*
 * Copy one set of bits to another
 */
#define	BGPB_COPY(ibits, obits, len) \
    do { \
	if ((len) == 1) { \
	    *(obits) = *(ibits); \
	} else { \
	    bcopy((void_t)(ibits), (void_t)(obits), (len) * sizeof(bgp_bits)); \
	} \
    } while (0)

/*
 * Zero a set of bits
 */
#define	BGPB_ZERO(bits, len) \
    do { \
	if ((len) == 1) { \
	    *(bits) = 0; \
	} else { \
	    bzero((void_t)(bits), (size_t) ((len) * sizeof(bgp_bits))); \
	} \
    } while (0)

/*
 * Collect all the bits in the rtinfo entries connected to a group
 * rto entry.
 */
#define	BGPB_INFOBITS(bits, rtop, bitlen) \
    do { \
	register bgpg_rtinfo_entry *Xinfop = (rtop)->bgpgo_info; \
	register bgp_bits *Xbits = (bits); \
	if ((bitlen) == 1) { \
	    *Xbits = *(Xinfop->bgp_info_bits); \
	    while ((Xinfop = Xinfop->bgp_info_next) \
	      != (bgpg_rtinfo_entry *) 0) { \
		*Xbits |= *(Xinfop->bgp_info_bits); \
	    } \
	} else { \
	    register bgp_bits *Xb2add; \
	    register bgp_bits *Xb; \
	    register bgp_bits *Xbend = Xbits + (bitlen); \
	    Xb = Xbits; \
	    Xb2add = Xinfop->bgp_info_bits; \
	    do { \
		*Xb++ = *Xb2add++; \
	    } while (Xb < Xbend); \
	    while ((Xinfop = Xinfop->bgp_info_next) \
	      != (bgpg_rtinfo_entry *) 0) { \
		Xb = Xbits; \
		Xb2add = Xinfop->bgp_info_bits; \
		do { \
		    *Xb++ |= *Xb2add++; \
		} while (Xb < Xbend); \
	    } \
	} \
    } while (0)




/*
 * Initialize the rti queue pointers.
 */
#define	BGP_RTI_INIT(bnp) \
    do { \
	register bgpPeer *Xbnp = (bnp); \
	Xbnp->bgp_rti_prev = Xbnp->bgp_rti_next \
	  = (bgp_rti_entry *)&(Xbnp->bgp_rti_prev); \
    } while (0)

/*
 * Add an rti to the tail of the rti queue.
 */
#define	BGP_RTI_PUT_TAIL(bnp, rti) \
    do { \
	register bgpPeer *Xbnp = (bnp); \
	register bgp_rti_entry *Xrti = (rti); \
	Xrti->bgpi_prev = Xbnp->bgp_rti_prev; \
	Xbnp->bgp_rti_prev->bgpi_next = Xrti; \
	Xrti->bgpi_next = (bgp_rti_entry *)&(Xbnp->bgp_rti_prev); \
	Xbnp->bgp_rti_prev = Xtri; \
    } while (0)

/*
 * Remove the head of the rti queue.
 */
#define	BGP_RTI_GET_HEAD(bnp, rti) \
    do { \
	register bgpPeer *Xbnp = (bnp); \
	Xbnp->bgp_rti_next = ((rti) = Xbnp->bgp_rti_next)->bgpi_next; \
	Xbnp->bgp_rti_next->bgpi_prev \
	  = (bgp_rti_entry *)&(Xbnp->bgp_rti_prev); \
    } while (0)


/*
 * Get a set of group bits
 */

#define	BGPG_GETBITS(bgpg_bits, size) \
    (((size) == 1) ? &((bgpg_bits).bgp_gr_bits) : (bgpg_bits).bgp_gr_bitptr)


/*
 * Buffer space computations
 */
#define	BGPBUF_SPACE(bnp)	((bnp)->bgp_endbuf - (bnp)->bgp_readptr)
#define	BGPBUF_LEFT(bnp, cp)	((bnp)->bgp_readptr - (byte *)(cp))
#define	BGPBUF_DATA(bnp)	BGPBUF_LEFT((bnp), (bnp)->bgp_bufpos)
#define	BGPBUF_FULL(bnp)	((bnp)->bgp_readptr == (bnp)->bgp_endbuf)

/*
 * Macro to sqeeze data to the beginning of the buffer.  This recognizes
 * most special cases, and so may be used for routine cleanup.
 */
#define	BGPBUF_COMPACT(bnp, cp)	\
	do { \
		if ((cp) != (bnp)->bgp_buffer) { \
			if ((cp) == (bnp)->bgp_readptr) { \
				(bnp)->bgp_readptr = (bnp)->bgp_buffer; \
			} else { \
				register int Xlen = BGPBUF_LEFT((bnp), (cp)); \
				bcopy((cp), (bnp)->bgp_buffer, (size_t)Xlen); \
				(bnp)->bgp_readptr = (bnp)->bgp_buffer + Xlen; \
			} \
		} \
	} while (0)

/*
 * Quicker authentication checking for the default type
 */
#define	BGP_CHECK_AUTH(bap, tp, pkt, pktlen) \
    ((((bap) != NULL && (bap)->bgpa_type != BGP_AUTH_NONE) \
      || (bcmp((caddr_t)bgp_default_auth_info, (caddr_t)(pkt), \
	BGP_HEADER_MARKER_LEN) != 0)) \
    ? bgp_check_auth((bap), (tp), (pkt), (pktlen)) : (1))

#define	BGP_ADD_AUTH(bap, pkt, pktlen) \
    if ((bap) == NULL || (bap)->bgpa_type == BGP_AUTH_NONE) {\
        bcopy((caddr_t)bgp_default_auth_info, (caddr_t)(pkt), BGP_HEADER_MARKER_LEN); \
    } else { \
        bgp_add_auth((bap), (pkt), (pktlen)); \
    }

/*
 * I don't like trace_state much because it doesn't do bounds checking.
 * Try this instead.  Should use token pasting for the macro.
 */
typedef struct _bgp_code_string {
    u_int bgp_n_codes;
    const bits *bgp_code_bits;
} bgp_code_string;

#define	BGP_MAKE_CODES(codevar, bitvar) \
    const bgp_code_string codevar = { (sizeof(bitvar)/sizeof(bits))-1, bitvar }

#define	bgp_code(codes, code) \
    (((code) >= (codes).bgp_n_codes) ? "invalid" : \
      trace_state((codes).bgp_code_bits, (code)))

/*
 * Metric translations for BGP4/BGP2or3 peers.
 *
 * XXX This is a huge crock of shit.  This needs to be replaced by
 * (1) making metrics unsigned 32 bit numbers, with a separate value for "none"
 * (2) making BGP3 internal metrics better bigger, when rcp_routed is gone
 * (3) allowing preference/metric translations to be specified from policy.
 */
#define	BGP_LOCALPREF_TO_GATED(met)	((met) & 0x7fffffff)
#define	BGP_V4METRIC_TO_GATED(met)	((met) & 0x7fffffff)

#define	BGP_V3METRIC_TO_PREF(met, setpref, defpref) \
    (((met) == BGP_METRIC_NONE) ? (defpref) : \
    ((((met) + (setpref)) > 254) ? 254 : ((met) + (setpref))))

#define	BGP_METRIC_3TO4(met) \
    (((met) >= 254 || (met) == BGP_METRIC_NONE) ? 0 : (254 - (met)))

#define	BGP_METRIC_4TO3(met) \
    (((met) == BGP_METRIC_NONE) ? 254 : \
    (((met) >= 254) ? 0 : (254 - (met))))

#define	BGP_LOCALPREF_TO_PREF(met, setpref, defpref) \
    (((met) == BGP_METRIC_NONE) ? (defpref) : \
    (((met) <= (254 - (setpref))) ? 254 : \
    (((met) >= 254) ? (setpref) : ((setpref) + 254 - (met)))))

#define	BGP_PREF_TO_LOCALPREF(pref, setpref) \
    (((pref) <= (setpref)) ? 254 : (254 + (setpref) - (pref)))

#define	BGP_PREF_TO_V3METRIC(pref, setpref) \
    (((pref) <= (setpref) ? 0 : ((pref) - (setpref)))

#define	BGP_DEF_LOCALPREF	100
#define	BGP_DEF_V3METRIC	(254 - 100)

/*
 * tracing/debugging variables, in bgp_init.c
 */
extern const bits bgp_flag_bits[];
extern const bits bgp_group_flag_bits[];
extern const bits bgp_option_bits[];
extern const bits bgp_state_bits[];
extern const bits bgp_event_bits[];
extern const bits bgp_message_type_bits[];
extern const bits bgp_error_bits[];
extern const bits bgp_header_error_bits[];
extern const bits bgp_open_error_bits[];
extern const bits bgp_update_error_bits[];
extern const bits bgp_group_bits[];

extern const bgp_code_string bgp_state_codes;
extern const bgp_code_string bgp_event_codes;
extern const bgp_code_string bgp_message_type_codes;
extern const bgp_code_string bgp_error_codes;
extern const bgp_code_string bgp_header_error_codes;
extern const bgp_code_string bgp_open_error_codes;
extern const bgp_code_string bgp_update_error_codes;
extern const bgp_code_string bgp_group_codes;

/*
 * Variables in bgp_init.c
 */
extern bgpPeerGroup *bgp_groups;	/* group list */
extern int bgp_n_groups;		/* number of groups in list */
extern int bgp_n_peers;			/* total number of peers */
extern int bgp_n_unconfigured;		/* number of unconfigured peers */
extern int bgp_n_established;		/* number of established peers */

/*
 * Variables from bgp.c
 */
extern byte bgp_default_auth_info[];	/* 16 bytes of 1's, should be const */


/*
 * Routines in bgp_rt.c
 */
PROTOTYPE(bgp_rt_send_init,
	  extern void,
	  (bgpPeer *));
PROTOTYPE(bgp_aux_flash,
	  extern void,
	  (task *,
	   rt_list *));
PROTOTYPE(bgp_aux_newpolicy,
	  extern void,
	  (task *,
	  rt_list *));
PROTOTYPE(bgp_rt_terminate,
	  extern void,
	  (bgpPeer *));
PROTOTYPE(bgp_rt_send_ready,
	  extern void,
	  (bgpPeer *));
PROTOTYPE(bgp_rt_init,
	  extern void,
	  (void));
PROTOTYPE(bgp_rt_peer_delete,
	  extern void,
	  (bgpPeer *));
PROTOTYPE(bgp_rt_group_delete,
	  extern void,
	  (bgpPeerGroup *));
PROTOTYPE(bgp_rt_unsync,
	  extern void,
	  (bgpPeer *));
PROTOTYPE(bgp_rt_sync,
	  extern void,
	  (bgpPeer *));
PROTOTYPE(bgp_rt_peer_timer,
	  extern void,
	  (bgpPeer *));
PROTOTYPE(bgp_rt_group_timer,
	  extern void,
	  (bgpPeerGroup *));
PROTOTYPE(bgp_rt_if_terminate,
	  extern void,
	  (bgpPeerGroup *,
	   if_addr *));
#ifdef	BGPDEBUG
PROTOTYPE(bgp_blewit,
	  extern void_t,
	  (const char *,
	   int));
#endif	/* BGPDEBUG */

/*
 * Routines in bgp_init.c
 */
PROTOTYPE(bgp_pp_delete,
	  extern void,
	  (bgpProtoPeer *));
PROTOTYPE(bgp_recv_change,
	  extern void,
	  (bgpPeer *,
	  _PROTOTYPE(recv_rtn,
		     void,
		     (task *)),
	  const char *));
PROTOTYPE(bgp_write_message,
	  extern void,
	  (bgpPeer *,
	   byte *,
	   size_t,
	   int));
PROTOTYPE(bgp_force_write,
	  extern int,
	  (bgpPeer *));
PROTOTYPE(bgp_set_write,
	  extern void,
	  (bgpPeer *));
PROTOTYPE(bgp_set_flash,
	  extern void,
	  (task *,
	   _PROTOTYPE(flash_rtn,
		      void,
		      (task *,
		       rt_list *)),
	   _PROTOTYPE(newpolicy_rtn,
		      void,
		      (task *,
		       rt_list *))));
PROTOTYPE(bgp_reset_flash,
	  extern void,
	  (task *));
PROTOTYPE(bgp_set_reinit,
	  extern void,
	  (task *,
	   _PROTOTYPE(reinit_rtn,
		      void,
		      (task *))));
PROTOTYPE(bgp_reset_reinit,
	  extern void,
	  (task *));
PROTOTYPE(bgp_find_group,
	  extern bgpPeerGroup *,
	  (sockaddr_un *,
	   sockaddr_un *,
	   as_t,
	   as_t,
	   int,
	   byte *,
	   int));
PROTOTYPE(bgp_find_peer,
	  extern bgpPeer *,
	  (bgpPeerGroup *,
	   sockaddr_un *,
	   sockaddr_un *));
PROTOTYPE(bgp_find_group_by_addr,
	  extern bgpPeerGroup *,
	  (sockaddr_un *,
	   sockaddr_un *));
PROTOTYPE(bgp_new_peer,
	  extern bgpPeer *,
	  (bgpPeerGroup *,
	   bgpProtoPeer *,
	   size_t));
PROTOTYPE(bgp_use_protopeer,
	  extern void,
	  (bgpPeer *,
	   bgpProtoPeer *,
	   size_t));
PROTOTYPE(bgp_peer_close,
	  extern void,
	  (bgpPeer *,
	   int));
PROTOTYPE(bgp_peer_established,
	  extern void,
	  (bgpPeer *));
PROTOTYPE(bgp_route_timer_set,
	  extern void,
	  (bgpPeer *));
PROTOTYPE(bgp_group_route_timer_set,
	  extern void,
	  (bgpPeerGroup *));

#define	bgp_find_peer_by_addr(bgp, addr, lcladdr) \
	bgp_find_peer((bgp), (addr), (lcladdr))

/*
 * Routines in bgp.c
 */
PROTOTYPE(bgp_event, extern void, (bgpPeer *, int, int));
PROTOTYPE(bgp_add_auth,
	  extern void,
	  (bgpAuthinfo *,
	   byte *,
	   size_t));
PROTOTYPE(bgp_check_auth,
	  extern int,
	  (bgpAuthinfo *,
	   task *,
	   byte *,
	   size_t));
PROTOTYPE(bgp_group_auth,
	  extern int,
	  (bgpPeerGroup *,
	   int,
	   byte *,
	   int));
PROTOTYPE(bgp_open_auth,
	  extern int,
	  (char *,
	  bgpAuthinfo *,
	  int,
	  byte *,
	  int));
PROTOTYPE(bgp_trace,
	  extern void,
	 (bgpPeer *,
	  bgpProtoPeer *,
	  const char *,
	  int,
	  byte *,
	  int));
PROTOTYPE(bgp_log_notify,
	  extern void,
	  (trace *,
	   char *,
	   byte *,
	   size_t,
	   int));
PROTOTYPE(bgp_send,
	  extern int,
	  (bgpPeer *,
	   byte *,
	   size_t,
	   int));
PROTOTYPE(bgp_send_open,
	  extern int,
	  (bgpPeer *,
	   int));
PROTOTYPE(bgp_send_keepalive,
	  extern int,
	  (bgpPeer *,
	   int));
PROTOTYPE(bgp_send_notify,
	  extern void,
	  (bgpPeer *,
	   int,
	   int,
	   byte *,
	   int));
PROTOTYPE(bgp_send_notify_none, extern void, (bgpPeer *, int, int));
PROTOTYPE(bgp_send_notify_byte, extern void, (bgpPeer *, int, int, int));
PROTOTYPE(bgp_send_notify_word, extern void, (bgpPeer *, int, int, int));
PROTOTYPE(bgp_send_notify_aspath, extern void,
	 (bgpPeer *, int, int, as_path *));
PROTOTYPE(bgp_path_attr_error, extern void, (bgpPeer *, int, byte *, const char *));
PROTOTYPE(bgp_pp_notify_none, extern void,
	 (bgpProtoPeer *, bgpPeerGroup *, bgpPeer *, int, int));
PROTOTYPE(bgp_recv, extern int, (task *, bgpBuffer *, int, char *));
PROTOTYPE(bgp_recv_open, extern void, (task *));
PROTOTYPE(bgp_pp_recv, extern void, (task *));

/*
 * Stuff in bgp_sync.c
 */
PROTOTYPE(bgp_sync_rt_add,
	  extern rt_entry *,
	  (bgp_sync *,
	   bgpPeer *,
	   rt_head *,
	   rt_parms *));
PROTOTYPE(bgp_sync_rt_change,
	  extern rt_entry *,
	  (bgp_sync *,
	   bgpPeer *,
	   rt_entry *,
	   metric_t,
	   metric_t,
	   tag_t,
	   pref_t,
	   pref_t,
	   int,
	   sockaddr_un **,
	   as_path *));
PROTOTYPE(bgp_sync_rt_delete,
	  extern void,
	  (bgp_sync *,
	   rt_entry *));
PROTOTYPE(bgp_sync_igp_rt,
	  extern void,
	  (bgp_sync *,
	   rt_head *));
PROTOTYPE(bgp_sync_terminate,
	  extern void,
	  (bgp_sync *));
PROTOTYPE(bgp_sync_init,
	  extern bgp_sync *,
	  (bgpPeerGroup *));

/*
 * Stuff in bgp_mib.c, for SNMP
 */

#ifdef  PROTO_SNMP
PROTOTYPE(bgp_init_mib, extern void, (int));
PROTOTYPE(bgp_trap_established, extern void, (bgpPeer *));
PROTOTYPE(bgp_trap_backward, extern void, (bgpPeer *));
PROTOTYPE(bgp_sort_add, extern void, (bgpPeer *));
PROTOTYPE(bgp_sort_remove, extern void, (bgpPeer *));
#endif  /* PROTO_SNMP */
