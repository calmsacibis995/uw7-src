#ident	"@(#)egp_mib.c	1.3"
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


#define	INCLUDE_ISODE_SNMP
#include "include.h"

#if	defined(PROTO_EGP) && defined(PROTO_SNMP)
#include "inet.h"
#include "egp.h"
#include "snmp_isode.h"


PROTOTYPE(o_egp,
	  static int,
	  (OI,
	   struct type_SNMP_VarBind *,
	   int));
PROTOTYPE(o_egp_neigh,
	  static int,
	  (OI,
	   struct type_SNMP_VarBind *,
	   int));
PROTOTYPE(s_egpNeighEventTrigger,
	  static int,
	  (OI,
	   struct type_SNMP_VarBind *,
	   int));

static struct object_table egp_objects[] = {
#define	IegpInMsgs		0
#define	IegpInErrors		1
#define	IegpOutMsgs		2
#define	IegpOutErrors		3
#define	IegpAs			4
    OTE(egpInMsgs, o_egp, NULL),
    OTE(egpInErrors, o_egp, NULL),
    OTE(egpOutMsgs, o_egp, NULL),
    OTE(egpOutErrors, o_egp, NULL),
    OTE(egpAs, o_egp, NULL),

#define	IegpNeighState		5
#define	IegpNeighAddr		6
#define	IegpNeighAs		7
#define	IegpNeighInMsgs		8
#define	IegpNeighInErrs		9
#define	IegpNeighOutMsgs	10
#define	IegpNeighOutErrs	11
#define	IegpNeighInErrMsgs	12
#define	IegpNeighOutErrMsgs	13
#define	IegpNeighStateUps	14
#define	IegpNeighStateDowns	15
#define	IegpNeighIntervalHello	16
#define	IegpNeighIntervalPoll	17
#define	IegpNeighMode		18
#define	IegpNeighEventTrigger	19
    OTE(egpNeighState, o_egp_neigh, NULL),
#define STATE_IDLE		1
#define STATE_ACQUISITION	2
#define STATE_DOWN		3
#define STATE_UP		4
#define STATE_CEASE		5
    OTE(egpNeighAddr, o_egp_neigh, NULL),
    OTE(egpNeighAs, o_egp_neigh, NULL),
    OTE(egpNeighInMsgs, o_egp_neigh, NULL),
    OTE(egpNeighInErrs, o_egp_neigh, NULL),
    OTE(egpNeighOutMsgs, o_egp_neigh, NULL),
    OTE(egpNeighOutErrs, o_egp_neigh, NULL),
    OTE(egpNeighInErrMsgs, o_egp_neigh, NULL),
    OTE(egpNeighOutErrMsgs, o_egp_neigh, NULL),
    OTE(egpNeighStateUps, o_egp_neigh, NULL),
    OTE(egpNeighStateDowns, o_egp_neigh, NULL),
    OTE(egpNeighIntervalHello, o_egp_neigh, NULL),
    OTE(egpNeighIntervalPoll, o_egp_neigh, NULL),
    OTE(egpNeighMode, o_egp_neigh, NULL),
    OTE(egpNeighEventTrigger, o_egp_neigh, s_egpNeighEventTrigger),

    { NULL }
};


static struct snmp_tree egp_mib_tree = {
    NULL, NULL,
    "egp",
    NULLOID,
    readWrite,
    egp_objects,
    0
};


static int
o_egp __PF3(oi, OI,
	    v, register struct type_SNMP_VarBind *,
	    offset, int)
{
    register OID    oid = oi->oi_name;
    register OT	    ot = oi->oi_type;

    switch (offset) {
    case type_SNMP_SMUX__PDUs_get__request:
	if (oid->oid_nelem != ot->ot_name->oid_nelem + 1
	    || oid->oid_elements[oid->oid_nelem - 1]) {
	    return int_SNMP_error__status_noSuchName;
	}
	break;

    case type_SNMP_SMUX__PDUs_get__next__request:
	if (oid->oid_nelem == ot->ot_name->oid_nelem) {
	    OID new;

	    if ((new = oid_extend(oid, 1)) == NULLOID) {
		return int_SNMP_error__status_genErr;
	    }
	    new->oid_elements[new->oid_nelem - 1] = 0;

	    if (v->name) {
		free_SNMP_ObjectName(v->name);
	    }
	    v->name = new;
	} else {
	    return NOTOK;
	}
	break;

    default:
	return int_SNMP_error__status_genErr;
    }

    switch (ot2object(ot)->ot_info) {
    case IegpInMsgs:
	return o_integer(oi, v, egp_stats.inmsgs);

    case IegpInErrors:
	return o_integer(oi, v, egp_stats.inerrors);

    case IegpOutMsgs:
	return o_integer(oi, v, egp_stats.outmsgs);

    case IegpOutErrors:
	return o_integer(oi, v, egp_stats.outerrors);

    case IegpAs:
	return o_integer(oi, v, inet_autonomous_system);
    }

    return int_SNMP_error__status_noSuchName;
}

/**/

static egp_neighbor  **egp_sort;	/* Sorted list of pointers to neighbors */
static u_int egp_sort_size = 0;
static egp_neighbor *egp_mib_last_ngp;
static unsigned int *egp_mib_last;


/* Collect the list of egp neighbors and make a sorted list */
void
egp_sort_neighbors __PF1(old_ngp, egp_neighbor *)
{

    /* XXX - do this with a chain through the peers */

    if (old_ngp
	&& old_ngp == egp_mib_last_ngp) {
	snmp_last_free(&egp_mib_last);
	egp_mib_last_ngp = (egp_neighbor *) 0;
    }

    /* Build a sorted list of neighbors for network monitoring */
    if (egp_sort_size < egp_neighbors) {
	if (egp_sort) {
	    task_mem_free((task *) 0, (void_t) egp_sort);
	}

	egp_sort_size = egp_neighbors;
	egp_sort = (egp_neighbor **) task_mem_calloc((task *) 0, (u_int) (egp_sort_size + 1), sizeof(egp_neighbor *));
    }
    if (egp_neighbors) {
	register egp_neighbor *ngp;
	register egp_neighbor **pl = egp_sort;

	EGP_LIST(ngp) {
	    u_int32 dst = ntohl(sock2ip(ngp->ng_addr));
	    register egp_neighbor **p;

	    for (p = egp_sort;
		 p < pl;
		 p++) {
		if (dst < ntohl(sock2ip((*p)->ng_addr))) {
		    register egp_neighbor **q = pl;

		    /* Copy the list */
		    do {
			*q = *(q - 1);
		    } while (q-- > p) ;

		    break;
		}
	    }

	    *p = ngp;
	    pl++;
	} EGP_LIST_END(ngp);
    }
}


static egp_neighbor *
egp_get_neigh __PF3(ip, register unsigned int *,
		    len, u_int,
		    isnext, int)
{
    u_int32 ngp_addr;

    if (snmp_last_match(&egp_mib_last, ip, len, isnext)) {
	return egp_mib_last_ngp;
    }

    if (len) {
	register egp_neighbor **p = egp_sort;
	register egp_neighbor **pl = egp_sort + egp_neighbors;

	oid2ipaddr(ip, &ngp_addr);

	GNTOHL(ngp_addr);

	if (isnext) {
	    register egp_neighbor *new = (egp_neighbor *) 0;
	    register u_int32 new_addr = 0;

	    for (; p < pl; p++) {
		register u_int32 cur_addr = ntohl(sock2ip((*p)->ng_addr));

		if (cur_addr > ngp_addr &&
		    (!new || cur_addr < new_addr)) {
		    new = *p;
		    new_addr = cur_addr;
		}
	    }

	    egp_mib_last_ngp = new;
	} else {
	    for (; p < pl; p++) {
		register u_int32 cur_addr = ntohl(sock2ip((*p)->ng_addr));
		
		if (cur_addr == ngp_addr) {
		    egp_mib_last_ngp = *p;
		    break;
		} else if (cur_addr > ngp_addr) {
		    egp_mib_last_ngp = (egp_neighbor *) 0;
		    break;
		}
	    }
	}
    } else {
	egp_mib_last_ngp = egp_neighbors ? *egp_sort : (egp_neighbor *) 0;
    }

    return egp_mib_last_ngp;
}


static int
o_egp_neigh_info __PF4(oi, OI,
		       v, register struct type_SNMP_VarBind *,
		       ifvar, int,
		       vp, void_t)
{
    register egp_neighbor *ngp = ((egp_neighbor *) vp);

    switch (ifvar) {
    case IegpNeighState:
        {
	    int state;

	    switch (ngp->ng_state) {
	    case NGS_IDLE:
		state = STATE_IDLE;
		break;
		
	    case NGS_ACQUISITION:
		state = STATE_ACQUISITION;
		break;
		
	    case NGS_DOWN:
		state = STATE_DOWN;
		break;
		
	    case NGS_UP:
		state = STATE_UP;
		break;
		
	    case NGS_CEASE:
		state = STATE_CEASE;
		break;

	    default:
		state = -1;
	    }
	    
	    return o_integer(oi, v, state);
	}

    case IegpNeighAddr:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(ngp->ng_addr,
				  (int *) 0));

    case IegpNeighAs:
	return o_integer(oi, v, ngp->ng_peer_as);

    case IegpNeighInMsgs:
	return o_integer(oi, v, ngp->ng_stats.inmsgs);

    case IegpNeighInErrs:
	return o_integer(oi, v, ngp->ng_stats.inerrors);

    case IegpNeighOutMsgs:
	return o_integer(oi, v, ngp->ng_stats.outmsgs);

    case IegpNeighOutErrs:
	return o_integer(oi, v, ngp->ng_stats.outerrors);

    case IegpNeighInErrMsgs:
	return o_integer(oi, v, ngp->ng_stats.inerrmsgs);

    case IegpNeighOutErrMsgs:
	return o_integer(oi, v, ngp->ng_stats.outerrmsgs);

    case IegpNeighStateUps:
	return o_integer(oi, v, ngp->ng_stats.stateups);

    case IegpNeighStateDowns:
	return o_integer(oi, v, ngp->ng_stats.statedowns);

    case IegpNeighIntervalHello:
	return o_integer(oi, v, ngp->ng_T1);

    case IegpNeighIntervalPoll:
	return o_integer(oi, v, ngp->ng_T2);

    case IegpNeighMode:
	return o_integer(oi, v, ngp->ng_M);

    case IegpNeighEventTrigger:
	return o_integer(oi, v, ngp->ng_stats.trigger);
    }

    return int_SNMP_error__status_noSuchName;
}


static int
o_egp_neigh __PF3(oi, OI,
		  v, register struct type_SNMP_VarBind *,
		  offset, int)
{
    register int    i;
    register unsigned int *ip,
			  *jp;
    register egp_neighbor *ngp;
    register OID    oid = oi->oi_name;
    register OT	    ot = oi->oi_type;
    OID		    new;

    /* INDEX   { egpNeighAddr } */
#define	NDX_SIZE	(sizeof (struct in_addr))
    
    switch (offset) {
    case type_SNMP_SMUX__PDUs_get__request:
	if (oid->oid_nelem != ot->ot_name->oid_nelem + NDX_SIZE) {
		return int_SNMP_error__status_noSuchName;
	    }
	ngp = egp_get_neigh(oid->oid_elements + oid->oid_nelem - NDX_SIZE,
			    NDX_SIZE,
			    0);
	if (!ngp) {
	    return int_SNMP_error__status_noSuchName;
	}
	break;

    case type_SNMP_SMUX__PDUs_get__next__request:
	/* next request with incomplete instance? */
	if ((i = oid->oid_nelem - ot->ot_name->oid_nelem) != 0 && i < NDX_SIZE) {
	    for (jp = (ip = oid->oid_elements + 
		       ot->ot_name->oid_nelem - 1) + i;
		 jp > ip;
		 jp--) {
		if (*jp != 0) {
		    break;
		}
	    }
	    if (jp == ip) {
		oid->oid_nelem = ot->ot_name->oid_nelem;
	    } else {
		if ((new = oid_normalize(oid, NDX_SIZE - i, 256)) == NULLOID) {
			return NOTOK;
		    }
		if (v->name) {
		    free_SNMP_ObjectName(v->name);
		}
		v->name = oid = new;
	    }
	}

	/* next request with no instance? */
	if (oid->oid_nelem == ot->ot_name->oid_nelem) {
	    ngp = egp_get_neigh((unsigned int *) 0,
				0,
				TRUE);
	    if (!ngp) {
		return NOTOK;
	    }

	    if ((new = oid_extend(oid, NDX_SIZE)) == NULLOID) {
		return int_SNMP_error__status_genErr;
	    }

	    ip = new->oid_elements + new->oid_nelem - NDX_SIZE;
	    STR_OID(ip, &sock2ip(ngp->ng_addr), sizeof (sock2ip(ngp->ng_addr)));
		
	    if (v->name) {
		free_SNMP_ObjectName(v->name);
	    }
	    v->name = new;
	} else {
	    int j;

	    ngp = egp_get_neigh(ip = oid->oid_elements + ot->ot_name->oid_nelem,
				     (u_int) (j = oid->oid_nelem - ot->ot_name->oid_nelem),
				     TRUE);
	    if (!ngp) {
		return NOTOK;
	    }

	    if ((i = j - NDX_SIZE) < 0) {
		if ((new = oid_extend(oid, -i)) == NULLOID) {
		    return int_SNMP_error__status_genErr;
		}
		if (v->name) {
		    free_SNMP_ObjectName(v->name);
		}
		v->name = oid = new;
	    } else if (i > 0) {
		oid->oid_nelem -= i;
	    }
		
	    ip = oid->oid_elements + ot->ot_name->oid_nelem;
	    STR_OID(ip, &sock2ip(ngp->ng_addr), sizeof (sock2ip(ngp->ng_addr)));
	}
	break;

    default:
	return int_SNMP_error__status_genErr;
    }

    return o_egp_neigh_info(oi, v, ot2object(ot)->ot_info, (void_t) ngp);
}


static int
s_egpNeighEventTrigger __PF3(oi, OI,
			     v, register struct type_SNMP_VarBind *,
			     offset, int)
{
    register OID oid = oi->oi_name;
    register OT ot = oi->oi_type;
    register OS os = ot->ot_syntax;
    int	trigger;
    register egp_neighbor *ngp;

    switch (offset) {
    case type_SNMP_SMUX__PDUs_set__request:
    case type_SNMP_PDUs_commit:
    case type_SNMP_PDUs_rollback:
	if (oid->oid_nelem != ot->ot_name->oid_nelem + sizeof (struct in_addr)) {
	    return int_SNMP_error__status_noSuchName;
	}
	ngp = egp_get_neigh(oid->oid_elements + oid->oid_nelem - sizeof (struct in_addr),
				 sizeof (struct in_addr),
				 0);
	if (!ngp) {
	    return int_SNMP_error__status_noSuchName;
	}
	break;

    default:
	trace_tf(snmp_trace_options,
		 TR_ALL,
		 0,
		 ("s_egpNeighEventTrigger: GENERR: bad offset %d",
		  offset));
	return int_SNMP_error__status_genErr;
    }

    if (os == NULLOS) {
	trace_tf(snmp_trace_options,
		 TR_ALL,
		 0,
		 ("s_egpNeighEventTrigger: GENERR: no syntax defined for \"%s\"",
		  ot->ot_text));
	return int_SNMP_error__status_genErr;
    }

    switch (offset) {
    case type_SNMP_SMUX__PDUs_set__request:
	if (ngp->ng_stats.trigger_save) {
	    (*os->os_free)(ngp->ng_stats.trigger_save);
	    ngp->ng_stats.trigger_save = NULL;
	}
	if ((*os->os_decode)(&ngp->ng_stats.trigger_save, v->value) == NOTOK) {
	    return int_SNMP_error__status_badValue;
	}
	trigger = *((int *)(ngp->ng_stats.trigger_save));
	switch (trigger) {
	case EGP_TRIGGER_START:
	case EGP_TRIGGER_STOP:
	    break;

	default:
	    return int_SNMP_error__status_badValue;
	}
	break;

    case type_SNMP_PDUs_commit:
	/* 
	 * NOTE: calling egp_event_stop() will cause an egpNeighLoss
	 * 	trap to be generated, before the responsePDU is sent.
	 *  Isode snmpd rejects the trap, and closes the smux-agent.
	 *  To prevent this, we assign a one-shot job later in time
	 *  to actually do the commit.
	 */
	ngp->ng_stats.trigger = *((int *)(ngp->ng_stats.trigger_save));

	switch (ngp->ng_stats.trigger) {
	case EGP_TRIGGER_START:
	    egp_event_start(ngp->ng_task);
	    break;

	case EGP_TRIGGER_STOP:
	    egp_event_stop(ngp, EGP_STATUS_GOINGDOWN);
	    break;
	}

	(*os->os_free) (ngp->ng_stats.trigger_save);
	ngp->ng_stats.trigger_save = NULL;
	break;

    case type_SNMP_PDUs_rollback:
	if (ngp->ng_stats.trigger_save) {
	    (*os->os_free) (ngp->ng_stats.trigger_save);
	    ngp->ng_stats.trigger_save = NULL;
	}
	break;
    }

    return int_SNMP_error__status_noError;
}


/**/

/* Traps */

#define	TRAP_NUM_VARS	1

static int egp_trap_varlist[TRAP_NUM_VARS] = { IegpNeighAddr };

void
egp_trap_neighbor_loss __PF1(ngp, egp_neighbor *)
{
    struct type_SNMP_VarBindList bindlist[TRAP_NUM_VARS];
    struct type_SNMP_VarBind var[TRAP_NUM_VARS];

    if (snmp_varbinds_build(TRAP_NUM_VARS,
			    bindlist,
			    var,
			    egp_trap_varlist,
			    &egp_mib_tree,
			    o_egp_neigh_info,
			    (void_t) ngp) == NOTOK) {
	trace_log_tf(snmp_trace_options,
		     0,
		     LOG_ERR,
		     ("egp_trap_neighbor_loss: snmp_varbinds_build failed"));
	goto out;
    }

    snmp_trap("egpNeighborLoss",
	      NULLOID,
	      int_SNMP_generic__trap_egpNeighborLoss,
	      0,
	      bindlist);

out:;
    snmp_varbinds_free(bindlist);
}


/**/

void
egp_mib_init __PF1(enabled, int)
{
    if (enabled) {
	snmp_tree_register(&egp_mib_tree);
    } else {
	snmp_tree_unregister(&egp_mib_tree);
    }
}

#endif	/* defined(PROTO_EGP) && defined(PROTO_SNMP) */
