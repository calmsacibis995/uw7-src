#ident	"@(#)targets.c	1.3"
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


#include "include.h"
#include "targets.h"

/**/

static const bits target_flag_bits[] = {
    { TARGETF_BROADCAST,	"Broadcast" },
    { TARGETF_SOURCE,		"Source" },
    { TARGETF_SUPPLY,		"Supply" },
    { TARGETF_ALLINTF,		"AllInterfaces" },
    { TARGETF_POLICY,		"Policy" },
    { 0, NULL },
};

const bits target_entry_bits[] = {
    { TDF_CHANGED,	"Changed" },
    { TDF_HOLDDOWN,	"Holddown" },
    { TDF_POISON,	"Poison" },
    { 0, NULL },
} ;

static block_t target_block_index;
block_t target_td_block;


/* Free any routes we were announcing */
static void
target_release __PF1(tlp, target *)
{
    register td_entry *tdp;
    /* Release any routes we have announced */

    rt_open(tlp->target_task);

    TD_LIST(tdp, &tlp->target_td) {
	/* Reset the bit and tsi then dequeue and free the entry */
	TD_CLEANUP(tlp, tdp, FALSE);
    } TD_LIST_END(tdp, &tlp->target_td) ;

    rt_close(tlp->target_task, (gw_entry *) 0, 0, NULL);

    /* And finally free the bit */
    rtbit_free(tlp->target_task, tlp->target_rtbit);

    tlp->target_rtbit = 0;
}


/* Free a target list */
static void
target_free __PF2(tp, task *,
		  tlp, target *)
{

    if (TRACE_TP(tp, TR_POLICY)) {
	tracef("target_free: FREE %A -> %A",
	       *tlp->target_src,
	       *tlp->target_dst);
	if (tlp->target_ifap) {
	    tracef(" interface %A(%s)",
		   tlp->target_ifap->ifa_addr,
		   tlp->target_ifap->ifa_link->ifl_name);
	}
	if (tlp->target_gwp) {
	    tracef(" gateway %A",
		   tlp->target_gwp->gw_addr);
	}
	trace_only_tp(tp,
		      0,
		      (" flags <%s>",
		       trace_bits(target_flag_bits, tlp->target_flags)));
    }

    /* Invoke protocol specific free routine */
    if (tlp->target_reset) {
	tlp->target_reset(tp, tlp);
    }
	
    /* Free the bit if allocated */
    if (tlp->target_rtbit) {
	target_release(tlp);
    }
	
    if (tlp->target_ifap) {
	/* Free the interface */
	    
	BIT_RESET(tlp->target_ifap->ifa_rtactive, RTPROTO_BIT(tp->task_rtproto));

	IFA_FREE(tlp->target_ifap);
    }

    /* Free the target entry */
    REMQUE(tlp);
    task_block_free(target_block_index, (void_t) tlp);
}


void
target_free_list __PF2(tp, task *,
		       list, target *)
{
    register target *tlp;

    while ((tlp = list->target_forw) != list) {
	target_free(tp, tlp);
    }
}


target *
target_locate __PF3(list, target *,
		    ifap, if_addr *,
		    gwp, gw_entry *)
{
    target *tlp;
    
    TARGET_LIST(tlp, list) {
	if ((BIT_TEST(tlp->target_flags, TARGETF_BROADCAST)
	     && tlp->target_ifap == ifap)
	    || (BIT_TEST(tlp->target_flags, TARGETF_SOURCE)
		&& tlp->target_gwp == gwp)) {

	    return tlp;
	}
    } TARGET_LIST_END(tlp, list) ;

    return (target *) 0;
}


PROTOTYPE(target_alloc,
	  static inline target *,
	  (task *,
	   target *,
	   target *,
	   if_addr *,
	   gw_entry *,
	   int,
	   _PROTOTYPE(ta_dump,
		      void,
		      (FILE *,
		       rt_head *,
		       void_t,
		       const char *))));	   
static inline target *
target_alloc(ta_tp, old_list, new_list, ifap, gwp, alloc, ta_dump)
task *ta_tp;
target *old_list;
target *new_list;
if_addr *ifap;
gw_entry *gwp;
int alloc;
_PROTOTYPE(ta_dump,
	   void,
	   (FILE *,
	    rt_head *,
	    void_t,
	    const char *));
{
    target *ta_tlp;

    /* Locate this target on the old list */
    TARGET_LIST(ta_tlp, old_list) {
	if (ta_tlp->target_ifap == ifap
	    && ta_tlp->target_gwp == gwp) {
	    REMQUE(ta_tlp)
	    break;
	}
    } TARGET_LIST_END(ta_tlp, old_list) ;

    /* If not on the old list, allocate a new one */
    if (!ta_tlp) {
	/* Allocate our block index */
	if (!target_block_index) {
	    target_block_index = task_block_init(sizeof (target), "target");
	    target_td_block = task_block_init(sizeof (td_entry), "target_dest");
	}

	/* Allocate this block */
    	ta_tlp = (target *) task_block_alloc(target_block_index);
	ta_tlp->target_task = ta_tp;
	ta_tlp->target_td.td_forw = ta_tlp->target_td.td_back = &ta_tlp->target_td;
    }

    if (alloc) {
	if (!BIT_TEST(ta_tlp->target_flags, TARGETF_SUPPLY)) {
	    /* Allocate a bit for this target */

	    ta_tlp->target_rtbit = rtbit_alloc(ta_tp,
					       TRUE,
					       sizeof (td_entry *),
					       (void_t) ta_tlp,
					       ta_dump);

	    /* Indicate we supply to this guy */
	    BIT_SET(ta_tlp->target_flags, TARGETF_SUPPLY);
	}
    } else if (BIT_TEST(ta_tlp->target_flags, TARGETF_SUPPLY)) {

	/* Release routes and free the bit */
	target_release(ta_tlp);

	/* Indicate that we don't want to supply any more */
	BIT_RESET(ta_tlp->target_flags, TARGETF_SUPPLY);
    }
    
    /* Append to the end of the new list */
    INSQUE(ta_tlp, new_list->target_back);

    return ta_tlp;
}


/* Allocate and build a target list for the given parameters */
int
target_build(tp, list, gw_list, flags, dump)
task *tp;
target *list;
gw_entry *gw_list;
flag_t flags;
_PROTOTYPE(dump,
	   void,
	   (FILE *,
	    rt_head *,
	    void_t,
	    const char *));
{
    int targets = 0;
    if_addr *ifap;
    target old, *tlp;

    /* Copy the root of the list so we can build a new one */
    old = *list;
    if (old.target_forw != list) {
	old.target_forw->target_back = &old;
	old.target_back->target_forw = &old;
    } else {
	old.target_forw = old.target_back = &old;
    }
    list->target_forw = list->target_back = list;

    /* Reset the active bits on any interfaces */
    TARGET_LIST(tlp, &old) {
	BIT_RESET(tlp->target_ifap->ifa_rtactive, RTPROTO_BIT(tp->task_rtproto));
    } TARGET_LIST_END(tlp, &old) ;

    /* First add interfaces */
    if (BIT_TEST(flags, TARGETF_ALLINTF|TARGETF_BROADCAST)) {
	IF_ADDR(ifap) {
	    int alloc;
	    sockaddr_un **dest;

	    if (socktype(ifap->ifa_addr) != AF_INET
		|| !BIT_TEST(ifap->ifa_state, IFS_UP)
		|| BIT_TEST(ifap->ifa_ps[tp->task_rtproto].ips_state, IFPS_NOOUT)) {
		/* Wrong protocol, interface is down, */
		/* no announcements allowed or policy has not seen this interface yet */

	    Continue:
		continue;
	    }

	    switch (BIT_TEST(ifap->ifa_state, IFS_BROADCAST|IFS_POINTOPOINT|IFS_LOOPBACK)) {
	    case IFS_LOOPBACK:
		/* The default is not to send packets to the loopback */
		/* interface.  This can be overridden by specifying the */
		/* loopback address in the sourcegateway clause */
		continue;

	    case IFS_BROADCAST:
		/* On broadcast interfaces, send to the broadcast address */
		
		dest = &ifap->ifa_addr_broadcast;

		/* Verify that we do not have another interface on this network */
		/* XXX - We should open one socket per local address and bind it. */
		TARGET_LIST(tlp, list) {
		    if (BIT_TEST(tlp->target_flags, TARGETF_BROADCAST)
			&& BIT_TEST(tlp->target_ifap->ifa_state, IFS_BROADCAST)
			&& sockaddrcmp(*dest, *tlp->target_dst)) {
			/* This is a duplicate */

			goto Continue;
		    }
		} TARGET_LIST_END(tlp, list) ;
		break;

	    case IFS_POINTOPOINT:
		/* On P2P interfaces, send to the destination address. */

		dest = &ifap->ifa_addr;
		break;

	    default:
		/* On NBMA interfaces we send packets to our self in order */
		/* to test that the interface is working.  This assumes that */
		/* packets send to myself over that interface will actually */
		/* get looped back by the hardware */

		if (BIT_TEST(ifap->ifa_state, IFS_NOAGE)) {
		    /* The test is not desired */

		    continue;
		}
		dest = &ifap->ifa_addr;
		break;
	    }

	    /* Locate old or allocate new */
	    tlp = target_alloc(tp,
			       &old,
			       list,
			       ifap,
			       (gw_entry *) 0,
			       BIT_TEST(flags, TARGETF_BROADCAST) ? TRUE : FALSE,
			       dump);

	    alloc = tlp->target_ifap != ifap;

	    /* Fill in the information */
	    tlp->target_dst = dest;
	    tlp->target_src = &ifap->ifa_addr_local;
	    tlp->target_gwp = (gw_entry *) 0;
	    if (tlp->target_ifap != ifap) {
		IFA_ALLOC(ifap);
		if (tlp->target_ifap) {
		    IFA_FREE(tlp->target_ifap);
		}
		tlp->target_ifap = ifap;
	    }
	    BIT_RESET(tlp->target_flags, TARGETF_SOURCE);
	    BIT_SET(tlp->target_flags, TARGETF_BROADCAST);
	    if (BIT_TEST(flags, TARGETF_BROADCAST)) {
		/* Indicate we are active on this interface */

		BIT_SET(ifap->ifa_rtactive, RTPROTO_BIT(tp->task_rtproto));

		/* And count it */
		targets++;
	    }

	    if (TRACE_TP(tp, TR_POLICY)) {
		tracef("target_build: %s %A -> %A",
		       alloc ? "ALLOC" : "REUSE",
		       *tlp->target_src,
		       *tlp->target_dst);
		if (tlp->target_ifap) {
		    tracef(" interface %A(%s)",
			   tlp->target_ifap->ifa_addr,
			   tlp->target_ifap->ifa_link->ifl_name);
		}
		trace_only_tp(tp,
			      0,
			      (" flags <%s>",
			       trace_bits(target_flag_bits, tlp->target_flags)));
	    }
	} IF_ADDR_END(ifap) ;
    }

    /* Then add the source gateways if any */
    if (BIT_TEST(flags, TARGETF_SOURCE)) {
	gw_entry *gwp;

	GW_LIST(gw_list, gwp) {
	    if (!BIT_TEST(gwp->gw_flags, GWF_SOURCE)) {
		continue;
	    }

	    if (!(ifap = if_withdst(gwp->gw_addr))) {
		if (BIT_TEST(task_state, TASKS_STRICTIFS)) {
		    trace_log_tp(tp,
				 0,
				 LOG_INFO,
				 ("target_build: Ignoring source gateway %A not on attached net",
				  gwp->gw_addr));
		}
		continue;
	    }

	    if (!BIT_TEST(ifap->ifa_state, IFS_UP)) {
		continue;
	    }

	    /* Look to see if this destination is the remote end of */
	    /* a P2P link I am already sending to */
	    TARGET_LIST(tlp, list) {
		if (sockaddrcmp(gwp->gw_addr, *tlp->target_dst)) {
		    if (!BIT_TEST(tlp->target_flags, TARGETF_SUPPLY)) {
			/* Allocate a bit for this target */

			tlp->target_rtbit = rtbit_alloc(tp,
							TRUE,
							sizeof (td_entry *),
							(void_t) tlp,
							dump);

			/* Indicate we supply to this guy */
			BIT_SET(tlp->target_flags, TARGETF_SUPPLY);
		    }
		    tlp->target_gwp = gwp;
		    break;
		}
	    } TARGET_LIST_END(tlp, list) ;

	    if (!tlp) {
		/* Locate old or allocate new */
		tlp = target_alloc(tp,
				   &old,
				   list,
				   ifap,
				   gwp,
				   TRUE,
				   dump);

		/* Fill in the information */
		tlp->target_dst = &gwp->gw_addr;
		tlp->target_src = &ifap->ifa_addr_local;
		if (tlp->target_ifap != ifap) {
		    IFA_ALLOC(ifap);
		    if (tlp->target_ifap) {
			IFA_FREE(tlp->target_ifap);
		    }
		}
		tlp->target_ifap = ifap;
		tlp->target_gwp = gwp;
		BIT_RESET(tlp->target_flags, TARGETF_BROADCAST);
	    }

	    /* Indicate we are supplying to this gateway */
	    BIT_SET(tlp->target_flags, TARGETF_SOURCE);

	    /* Indicate we are active on this interface */
	    BIT_SET(ifap->ifa_rtactive, RTPROTO_BIT(tp->task_rtproto));

	    /* And count it */
	    targets++;
	    
	} GW_LIST_END(gw_list, gwp);
    }

    /* Finally, free any remaining targets */
    if (old.target_forw != &old) {
	target_free_list(tp, &old);
    }

    return targets;
}


/*
 *	Dump a target list
 */
void
target_dump __PF3(fp, FILE *,
		  list, target *,
		  bp, const bits *)
{
    target *tlp;

    if (list) {
	(void) fprintf(fp, "\n\tTargets:\n");
	
	TARGET_LIST(tlp, list) {
	    (void) fprintf(fp, "\t\t%-15A -> %-15A\tInterface: %A(%s)\n",
			   *tlp->target_src,
			   *tlp->target_dst,
			   tlp->target_ifap->ifa_addr,
			   tlp->target_ifap->ifa_link->ifl_name);
	    (void) fprintf(fp, "\t\t\tFlags: <%s>\n",
			   trace_bits2(bp, target_flag_bits, tlp->target_flags));
	    if (BIT_TEST(tlp->target_flags, TARGETF_SUPPLY)) {
		(void) fprintf(fp, "\t\t\tBit: %d\n",
			       tlp->target_rtbit);
	    }
	    (void) fprintf(fp, "\n");

	    /* Dump the routes */
	    if (tlp->target_td.td_forw != &tlp->target_td) {
		td_entry *tdp;

		(void) fprintf(fp, "\t\t\tRoutes:\n");
		TD_LIST(tdp, &tlp->target_td) {
		    if (BIT_TEST(tdp->td_flags, TDF_HOLDDOWN|TDF_POISON)) {
			(void) fprintf(fp, "\t\t\t\t%A/%A <%s> count left %u\n",
				       tdp->td_rt->rt_dest,
				       tdp->td_rt->rt_dest_mask,
				       trace_bits(target_entry_bits, tdp->td_flags),
				       tdp->td_metric);
		    } else {
			(void) fprintf(fp, "\t\t\t\t%A/%A <%s> metric  %u\n",
				       tdp->td_rt->rt_dest,
				       tdp->td_rt->rt_dest_mask,
				       trace_bits(target_entry_bits, tdp->td_flags),
				       tdp->td_metric);
		    }
		} TD_LIST_END(tdp, &tlp->target_td) ;
	    }
	} TARGET_LIST_END(tlp, list) ;

	(void) fprintf(fp, "\n");
    }
}
