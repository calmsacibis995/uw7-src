#ident	"@(#)rip_mib.c	1.5"
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

#if	defined(PROTO_SNMP) && defined(MIB_RIP)
#include "inet.h"
#include "targets.h"
#include "rip.h"
#include "snmp_isode.h"


PROTOTYPE(o_rip,
	  static int,
	  (OI,
	   struct type_SNMP_VarBind *,
	   int));
PROTOTYPE(o_rip_ifstat,
	  static int,
	  (OI,
	   struct type_SNMP_VarBind *,
	   int));
PROTOTYPE(o_rip_ifconf,
	  static int,
	  (OI,
	   struct type_SNMP_VarBind *,
	   int));
PROTOTYPE(o_rip_peer,
	  static int,
	  (OI,
	   struct type_SNMP_VarBind *,
	   int));

#define	DOMAIN_LENGTH	2
 
static struct object_table rip_objects[] = {
#define rip2GlobalRouteChanges	0
#define rip2GlobalQueries	1
    { "rip2GlobalRouteChanges",		o_rip,		NULL,			rip2GlobalRouteChanges },
    { "rip2GlobalQueries",		o_rip,		NULL,			rip2GlobalQueries },

#define rip2IfStatAddress	2
#define rip2IfStatRcvBadPackets	3
#define rip2IfStatRcvBadRoutes	4
#define rip2IfStatSentUpdates	5
#define rip2IfStatStatus	6
    { "rip2IfStatAddress",		o_rip_ifstat,	NULL,			rip2IfStatAddress },
    { "rip2IfStatRcvBadPackets",	o_rip_ifstat,	NULL,			rip2IfStatRcvBadPackets },
    { "rip2IfStatRcvBadRoutes",		o_rip_ifstat,	NULL,			rip2IfStatRcvBadRoutes },
    { "rip2IfStatSentUpdates",		o_rip_ifstat,	NULL,			rip2IfStatSentUpdates },
    { "rip2IfStatStatus",		o_rip_ifstat,	NULL,			rip2IfStatStatus },
#define	STATUS_Valid		1
#define	STATUS_Invalid		2

#define rip2IfConfAddress	7
#define rip2IfConfDomain	8
#define rip2IfConfAuthType	9
#define rip2IfConfAuthKey	10
#define rip2IfConfSend		11
#define rip2IfConfReceive	12
#define rip2IfConfDefaultMetric	13
#define rip2IfConfStatus	14
#define rip2IfConfSrcAddress	15
    { "rip2IfConfAddress",		o_rip_ifconf,	NULL,			rip2IfConfAddress },
    { "rip2IfConfDomain",		o_rip_ifconf,	NULL,			rip2IfConfDomain },
    { "rip2IfConfAuthType",		o_rip_ifconf,	NULL,			rip2IfConfAuthType },
#define	AUTHTYPE_NoAuthentication	1
#define	AUTHTYPE_SimplePassword		2
    { "rip2IfConfAuthKey",		o_rip_ifconf,	NULL,			rip2IfConfAuthKey },
    { "rip2IfConfSend",			o_rip_ifconf,	NULL,			rip2IfConfSend },
#define	SEND_DoNotSend		1
#define	SEND_RipVersion1	2
#define	SEND_Rip1Compatible	3
#define	SEND_RipVersion2	4    
#define	SEND_RipV1Demand	5    
#define	SEND_RipV2Demand	6    
    { "rip2IfConfReceive",		o_rip_ifconf,	NULL,			rip2IfConfReceive },
#define	RECEIVE_Rip1	1
#define	RECEIVE_Rip2	2
#define	RECEIVE_Rip1OrRip2	3
#define RECEIVE_DoNotReceive	4    
    { "rip2IfConfDefaultMetric",	o_rip_ifconf,	NULL,			rip2IfConfDefaultMetric },
    { "rip2IfConfStatus",		o_rip_ifconf,	NULL,			rip2IfConfStatus },
    { "rip2IfConfSrcAddress",		o_rip_ifconf,	NULL,			rip2IfConfSrcAddress },

#define rip2PeerAddress		16
#define rip2PeerDomain		17
#define rip2PeerLastUpdate	18
#define rip2PeerVersion		19
#define rip2PeerRcvBadPackets	20
#define rip2PeerRcvBadRoutes	21
    { "rip2PeerAddress",		o_rip_peer,	NULL,			rip2PeerAddress },
    { "rip2PeerDomain",			o_rip_peer,	NULL,			rip2PeerDomain },
    { "rip2PeerLastUpdate",		o_rip_peer,	NULL,			rip2PeerLastUpdate },
    { "rip2PeerVersion",		o_rip_peer,	NULL,			rip2PeerVersion },
    { "rip2PeerRcvBadPackets",		o_rip_peer,	NULL,			rip2PeerRcvBadPackets },
    { "rip2PeerRcvBadRoutes",		o_rip_peer,	NULL,			rip2PeerRcvBadRoutes },

    { NULL }
};


static struct snmp_tree rip_mib_tree = {
    NULL, NULL,
    "rip2",
    NULLOID,
    readWrite,
    rip_objects,
    0
};

static int
o_rip __PF3(oi, OI,
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
    case rip2GlobalRouteChanges:
	return o_integer(oi, v, rip_global_changes);

    case rip2GlobalQueries:
	return o_integer(oi, v, rip_global_responses);
    }

    return int_SNMP_error__status_noSuchName;
}
/* 
 * Routines for handling rip2IfStatTable and rip2IfConfTable. 
 *
 */
/*
 * A sorted list of rip interfaces will be maintained to facilitate retrieval and 
 * implementation of unnumbered point-to-point links. 
 */
struct rip_intf_entry	{
    struct rip_intf_entry *forw;
    struct rip_intf_entry *back;
    u_int32 rip_intf_ip_addr;
    if_addr *rip_ifap;
};

static struct rip_intf_entry rip_intf_list = 	{&rip_intf_list, 
						  &rip_intf_list, 
						  (u_int32) 0,
						  (if_addr *) 0};
#define RIP_INTF_LIST(intfp) for (intfp = rip_intf_list.forw; \
				  intfp != &rip_intf_list; \
				  intfp = intfp->forw) 
#define RIP_INTF_LIST_END(intfp)

static int rip_intf_cnt;		/* Count of rip interfaces.	*/
static block_t rip_intf_block_index;	/* Storage allocation block index for rip interface list.	*/ 
static unsigned int *rip_intf_last;	/* Pointer to last interface.	*/

/*
 * Function which will build a sorted list of all the GateD interfaces configured for rip. 
 * It is invoked during protocol initialization and when interfaces changes are detected.
 */

void
o_rip_intf_get __PF0(void)
{
    register struct rip_intf_entry *rip_intfp; 	/* Pointer to rip interface. 	*/
    register if_addr 	*ifap; 			/* Pointer to GateD interface.	*/
/*    register int unnumbered_index = 0;		/* Index for unnumbered interfaces. */
    /* 
     * Free storage allocated to the prior rip inteface list successively removing
     * the first element of the queue until only the head-tail entry remains.
     */
    RIP_INTF_LIST(rip_intfp) {
	remque((struct qelem *) rip_intfp);
	task_block_free(rip_intf_block_index, (void_t) rip_intfp);
	rip_intfp = &rip_intf_list;
    } RIP_INTF_LIST_END(rip_intfp);
    
    snmp_last_free(&rip_intf_last);
    rip_intf_cnt = 0;
    /*
     * Scan the list of interface currently known to GateD and build a local list sorted by
     * IP Address or interface-index (for configurations with more than one interface having the 
     * same local IP address).
     */
    IF_ADDR(ifap) {
	register u_int32 current_intf_addr; 
	register u_int32 save_intf_addr = 0; 
	if (!BIT_TEST(ifap->ifa_state, (IFS_LOOPBACK|IFS_DELETE))) { 
	    /*
	     * Set interface address dependent on whether or not the 
	     * current interface is a Frame Relay Point-to-Point interface,
	     * normal Point-to-Point, or non-Point-to-Point interface.
	     */
	    if (BIT_TEST(ifap->ifa_state, IFS_POINTOPOINT) && 
		BIT_TEST(ifap->ifa_link->ifl_state, IFS_BROADCAST)) {
		current_intf_addr = (ifap->ifa_link->ifl_index << 8);
	    } else {
		current_intf_addr = (BIT_TEST(ifap->ifa_state, IFS_POINTOPOINT)) ?
		    sock2ip(ifap->ifa_addr_local) :
			sock2ip(ifap->ifa_addr);
		save_intf_addr = current_intf_addr;  /* Save the current interface address
							for duplicate handling.         */
	    }
	    /*
	     * Insert an interface entry onto the rip interface list sorted by
	     * IP address. If the IP address is the same as that of a previous interface,
	     * use the interface index. The case of multiple PPP links originating from 
	     * the same frame relay physical interface is already handled above.
	     */
	    /* Find the correct position in the list to insert the current interface.	*/
	    RIP_INTF_LIST(rip_intfp) {
		if (current_intf_addr < rip_intfp->rip_intf_ip_addr) {
		    break;
		} else if (current_intf_addr == rip_intfp->rip_intf_ip_addr) {
		    if (current_intf_addr == save_intf_addr) {
			current_intf_addr = ifap->ifa_link->ifl_index; 
			rip_intfp = &rip_intf_list; /* Re-insert from the list start.	*/
		    } else {
			current_intf_addr++;
		    }
		}	
	    } RIP_INTF_LIST_END(rip_intfp);

	    /* Actually insert and build the interface entry in our list of rip interfaces. */
	    insque((struct qelem *) task_block_alloc(rip_intf_block_index), rip_intfp->back);
	    rip_intfp->back->rip_ifap = ifap;
	    rip_intfp->back->rip_intf_ip_addr = current_intf_addr;
	    rip_intf_cnt++;
	}
    } IF_ADDR_END(ifap);
}

static struct rip_intf_entry *
o_rip_intf_lookup __PF3(ip, register unsigned int *,
			len, int,
			isnext, int)
{
    static struct rip_intf_entry *last_rip_intfp;
    static int last_quantum;

    if (last_quantum != snmp_quantum) {
	last_quantum = snmp_quantum;

	if (rip_intf_last) {
	    task_mem_free((task *) 0, (caddr_t) rip_intf_last);
	    rip_intf_last = (unsigned int *) 0;
	}
    }
    /*
     * Determine if the last interface is being requested again. 
     */
    if (snmp_last_match(&rip_intf_last, ip, len, isnext)) {
	return last_rip_intfp;
    }
    /* Return a null pointer if no rip interfaces exist. */
    if (!rip_intf_cnt) {
	return last_rip_intfp = (struct rip_intf_entry *) 0;
    }
    if (len) {
	u_int32 requested_intf_ip_addr = (u_int32)  0;
	int requested_intf_index;
	register struct rip_intf_entry *rip_intfp;
	
	oid2ipaddr(ip, &requested_intf_ip_addr);
#ifndef	SCO_GEMINI
	GNTOHL(requested_intf_ip_addr);
#endif	/*SCO_GEMINI*/
	/*
	 * The following statement will be needed when the interface lists 
	 * in the rip mib are indexed by ip address and interface index. For now,
	 * retrieve the first interface with the specified ip address.
	 *
	 *	intf_index = ip[sizeof (struct in_addr)];
	 */
	requested_intf_index = 0;
	RIP_INTF_LIST(rip_intfp) {
	    /*
	     * If the requested address and index match the current
	     * entry's values and an SNMP Get operation is being processed, 
	     * return the current entry. 
	     */
	    if (rip_intfp->rip_intf_ip_addr == requested_intf_ip_addr) {
		if (!isnext) {
		    return last_rip_intfp = rip_intfp;
		}
		/*
		 * If the current entry's address surpasses the requested 
		 * values and the powerful SNMP Get-Next operation is being process,
		 * return the current entries. For the SNMP Get operation, the search
		 * has failed and a no interface will be returned.
		 */
	    } else if (rip_intfp->rip_intf_ip_addr > requested_intf_ip_addr) {
#ifdef	SCO_GEMINI
		return (last_rip_intfp != rip_intfp && isnext) ?
			rip_intfp : (struct rip_intf_entry *) 0;
#else	/*SCO_GEMINI*/
		return last_rip_intfp = isnext ? rip_intfp : (struct rip_intf_entry *) 0;
#endif	/*SCO_GEMINI*/
	    }
	} RIP_INTF_LIST_END(rip_intfp);
	/* All interfaces have been scanned without success, a null value will be returned. */
	return last_rip_intfp = (struct rip_intf_entry *) 0;
    }
    /* 
     *  Return the first interface since no search critera were specified. This implies an 
     *  SNMP Get to a ancestoral MIB element or the first SNMP Get-Next after processing the
     *  prior object in the rip II MIB tree. 
     */
    return last_rip_intfp = rip_intf_list.forw;
}

/*
 * Return objects in the rip2IfStatTable.  
 */
static int 
o_rip_ifstat __PF3(oi, OI,
		  v, register struct type_SNMP_VarBind *,
		  offset, int)
{
    register int    i;
    register unsigned int *ip,
			  *jp;
    struct rip_intf_entry *rip_intfp;
    register OID    oid = oi->oi_name;
    register OT	    ot = oi->oi_type;
    OID		    new;
    /*
     * The rip2IfStatTable is indexed by rip2IfStatAddress (i.e., IP Address).
     * Account for enough tokens to hold an IP address in the Object Identifier
     * calculations. The MIB will need to be updated to support an index containing
     * both the ip address and interface index if unnumbered interfaces are to be  
     * supported correctly.
     */ 
#define	NDX_SIZE	sizeof(struct in_addr)
    switch (offset) {
    case type_SNMP_SMUX__PDUs_get__request:
        trace_tf(trace_global, TR_USER_1, 0,
           ("SNMP rip2ifstat - Get"));
	if (oid->oid_nelem - ot->ot_name->oid_nelem != NDX_SIZE) {
        trace_tf(trace_global, TR_USER_1, 0,
           ("SNMP rip2ifstat - Get elem err %d",oid->oid_nelem));
	    return int_SNMP_error__status_noSuchName;
	}
	rip_intfp = o_rip_intf_lookup(oid->oid_elements + oid->oid_nelem - NDX_SIZE,
				      NDX_SIZE,
				      0);
	if (!rip_intfp) {
        trace_tf(trace_global, TR_USER_1, 0,
           ("SNMP rip2ifstat - Get not found - %d",oid->oid_nelem));
	    return int_SNMP_error__status_noSuchName;
	}
	break;

    case type_SNMP_SMUX__PDUs_get__next__request:
        trace_tf(trace_global, TR_USER_1, 0,
           ("SNMP rip2ifstat - GetNext"));
	/* next request with incomplete instance? */
	if ((i = oid->oid_nelem - ot->ot_name->oid_nelem) != 0 && i < (int) NDX_SIZE) {
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
	} else
            if (i > (int)  NDX_SIZE)
               oid->oid_nelem = ot->ot_name->oid_nelem + NDX_SIZE;


	/* next request with no instance? */
	if (oid->oid_nelem == ot->ot_name->oid_nelem) {
	    rip_intfp = o_rip_intf_lookup((unsigned int *) 0,
					   0,
					   TRUE);
	    if (!rip_intfp) {
		return NOTOK;
	    }
	    
	    if ((new = oid_extend(oid, NDX_SIZE)) == NULLOID) {
		return NOTOK;
	    }

	    ip = new->oid_elements + new->oid_nelem - NDX_SIZE;
	    STR_OID(ip,
		    &(rip_intfp->rip_intf_ip_addr),
		    sizeof( rip_intfp->rip_intf_ip_addr));
	    if (v->name) {
		free_SNMP_ObjectName(v->name);
	    }
	    v->name = new;
	} else {
	    int j;

	    rip_intfp = o_rip_intf_lookup(ip = oid->oid_elements + ot->ot_name->oid_nelem,
					  j = oid->oid_nelem - ot->ot_name->oid_nelem,
					  TRUE);
	    if (!rip_intfp) {
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
	    STR_OID(ip,
		    &(rip_intfp->rip_intf_ip_addr),
		    sizeof( rip_intfp->rip_intf_ip_addr));
   	}
	break;

    default:
	return int_SNMP_error__status_genErr;
    }
#undef	NDX_SIZE
    /*
     *  Return the requested object from the selected interface entry. 
     *
     */
    switch (ot2object(ot)->ot_info) {
    case rip2IfStatAddress:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(sockbuild_in(0, rip_intfp->rip_intf_ip_addr),
			(int *) 0));	  

    case rip2IfStatRcvBadPackets:
	return o_integer(oi,
			 v,
			 (u_int) rip_intfp->rip_ifap->ifa_rip_bad_packets);

    case rip2IfStatRcvBadRoutes:
	return o_integer(oi,
			 v, 
			 (u_int) rip_intfp->rip_ifap->ifa_rip_bad_routes);

    case rip2IfStatSentUpdates:
	return o_integer(oi, 
			 v,     
			 (u_int) rip_intfp->rip_ifap->ifa_rip_triggered_updates);

    case rip2IfStatStatus:
	return o_integer(oi, v, STATUS_Valid);
    }

    return int_SNMP_error__status_noSuchName;
}

/*
 * Return Objects in the rip2IfConfTable.
 */

static int
o_rip_ifconf __PF3(oi, OI,
		  v, register struct type_SNMP_VarBind *,
		  offset, int)
{
    register int    i;
    register unsigned int *ip,
			  *jp;
    as_t domain = 0;
    register OID    oid = oi->oi_name;
    register OT	    ot = oi->oi_type;
    OID		    new;
    struct rip_intf_entry *rip_intfp;  		/* Pointer to rip interface entry. */
/*
 * Constants for rip2IfConfSend and rip2ifConfReceive
 */
#define SEND_doNotSend 1
#define SEND_ripVersion1 2
#define SEND_rip1Compatible 3
#define SEND_ripVersion2 4
#define RECEIVE_rip1 1
#define RECEIVE_rip2 2
#define RECEIVE_rip1OrRip2 3
#define RECEIVE_NoRipIn 4
    /*
     * The rip2IfConfTable is indexed by rip2IfConfAddress (i.e., IP Address).
     * Account for enough tokens to hold an IP address in the Object Identifier
     * calculations. 
     */ 
#define	NDX_SIZE	sizeof(struct in_addr)
    switch (offset) {
    case type_SNMP_SMUX__PDUs_get__request:
        trace_tf(trace_global, TR_USER_1, 0,
           ("SNMP rip2ifconf - Get"));
	if (oid->oid_nelem - ot->ot_name->oid_nelem != NDX_SIZE) {
	    return int_SNMP_error__status_noSuchName;
	}
	rip_intfp = o_rip_intf_lookup(oid->oid_elements + oid->oid_nelem - NDX_SIZE,
				 NDX_SIZE,
				 0);
	if (!rip_intfp) {
	    return int_SNMP_error__status_noSuchName;
	}
	break;

    case type_SNMP_SMUX__PDUs_get__next__request:
        trace_tf(trace_global, TR_USER_1, 0,
           ("SNMP rip2ifconf - GetNext"));
	/* next request with incomplete instance? */
	if ((i = oid->oid_nelem - ot->ot_name->oid_nelem) != 0 && i < (int) NDX_SIZE) {
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
	    rip_intfp = o_rip_intf_lookup((unsigned int *) 0,
					  0,
					  TRUE);
	    if (!rip_intfp) {
		return NOTOK;
	    }
	    
	    if ((new = oid_extend(oid, NDX_SIZE)) == NULLOID) {
		return int_SNMP_error__status_genErr;
	    }

	    ip = new->oid_elements + new->oid_nelem - NDX_SIZE;
	    STR_OID(ip,
		    &(rip_intfp->rip_intf_ip_addr),
		    sizeof( rip_intfp->rip_intf_ip_addr));
	    if (v->name) {
		free_SNMP_ObjectName(v->name);
	    }
	    v->name = new;
	} else {
	    int j;

	    rip_intfp = o_rip_intf_lookup(ip = oid->oid_elements + ot->ot_name->oid_nelem,
					  j = oid->oid_nelem - ot->ot_name->oid_nelem,
					  TRUE);
	    if (!rip_intfp) {
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
	    STR_OID(ip,
		    &(rip_intfp->rip_intf_ip_addr),
		    sizeof( rip_intfp->rip_intf_ip_addr));
	   	}
	break;

    default:
	return int_SNMP_error__status_genErr;
    }
#undef	NDX_SIZE
    /*
     *  Return the requested object from the selected interface entry. 
     */
    switch (ot2object(ot)->ot_info) {
    case rip2IfConfAddress:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(sockbuild_in(0, rip_intfp->rip_intf_ip_addr),
			(int *) 0));	  

    case rip2IfConfDomain:
	return o_string(oi, v, (caddr_t) &domain, DOMAIN_LENGTH);

    case rip2IfConfAuthType:
	return o_integer(oi, v, (struct rip_authinfo *) rip_intfp->rip_ifap->ifa_rip_auth ? 
			 ((struct rip_authinfo *) rip_intfp->rip_ifap->ifa_rip_auth)->auth_type : 
			 AUTHTYPE_NoAuthentication);

    case rip2IfConfAuthKey:
	return o_string(oi, v, (caddr_t) 0, 0);

    case rip2IfConfSend:	{
	u_int32 conf_send;
	if (BIT_TEST(rip_intfp->rip_ifap->ifa_ps[RTPROTO_RIP].ips_state,IFPS_NOOUT) ||
	    !BIT_TEST(rip_intfp->rip_ifap->ifa_rtactive, RTPROTO_BIT(RTPROTO_RIP))) {
	    conf_send = SEND_doNotSend;
	} else	{
	    if (BIT_TEST(rip_intfp->rip_ifap->ifa_ps[RTPROTO_RIP].ips_state,RIP_IFPS_V2)) {
		if (BIT_TEST(rip_intfp->rip_ifap->ifa_ps[RTPROTO_RIP].ips_state,RIP_IFPS_V2MC)) {
		    conf_send = SEND_ripVersion2;
		} else {
		    conf_send = SEND_rip1Compatible;
		}
       	    } else {
		conf_send = SEND_ripVersion1;
	    }
	}
	return o_integer(oi, 
			 v,
			 conf_send);
    }

    case rip2IfConfReceive:
	return o_integer(oi,
			 v,
 	 		 (BIT_TEST(rip_intfp->rip_ifap->ifa_ps[RTPROTO_RIP].ips_state,IFPS_NOIN) ?
	 		  RECEIVE_DoNotReceive :  RECEIVE_Rip1OrRip2));
	 
    case rip2IfConfDefaultMetric:
	return o_integer(oi, v, 0);

    case rip2IfConfStatus:
	return o_integer(oi, v, STATUS_Valid);

    case rip2IfConfSrcAddress:
	return o_ipaddr(oi,
			v,
			BIT_TEST(rip_intfp->rip_ifap->ifa_state, IFS_POINTOPOINT) ?
			(caddr_t) sock2unix(rip_intfp->rip_ifap->ifa_addr_local,
				  (int *) 0) :
			(caddr_t) sock2unix(rip_intfp->rip_ifap->ifa_addr, 
				  (int *) 0));
    }	
    return int_SNMP_error__status_noSuchName;
}

/* 
 * rip2PeerTable Processing routines.
 */
#define RIP_GW_LIST(p) for (p = rip_gw_list; p != (gw_entry *) 0; p = p->gw_next)
#define RIP_GW_LIST_END(p)

/*
 * Routine to find the specified or next peer gateway. 
 */
static gw_entry *
o_rip_get_peer __PF3(ip, register unsigned int *,
		     len, int,
		     isnext, int)
{
    static gw_entry *last_gwp;
    static unsigned int *last;
    static int last_quantum;
    u_int32 rip_peer_addr;
    register gw_entry *p;
    struct timezone time_zone;
    struct timeval current_time;

    trace_tf(trace_global, TR_USER_1, 0,
             ("o_rip_get_peer - entered"));

    if (last_quantum != snmp_quantum) {
	last_quantum = snmp_quantum;

	if (last) {
	    task_mem_free((task *) 0, (caddr_t) last);
	    last = (unsigned int *) 0;
	}
    }

    trace_tf(trace_global, TR_USER_1, 0,
             ("o_rip_get_peer - snmp_last_match"));

    if (snmp_last_match(&last, ip, len, isnext)) {
	return last_gwp;
    }

    trace_tf(trace_global, TR_USER_1, 0,
             ("o_rip_get_peer - oid2ipaddr"));

    if (ip != (unsigned int *) 0) {
	oid2ipaddr(ip, &rip_peer_addr);
	GNTOHL(rip_peer_addr);
    } else
	rip_peer_addr = (u_int32) 0;

    /* 
     * Get the current time of day so that gateways from which we have not heard
     * from can be ommitted from the gateways returned.
     */
    gettimeofday(&current_time,
		 &time_zone);

    if (isnext) {
	register gw_entry *new = (gw_entry *) 0;
	register u_int32 new_addr = 0;
	/*
	 * Traverse Rip Gateway List and in one pass find the next element
	 * by finding the Gateway with lowest IP address which exceeds the
	 * the last IP address. This logic is in support of the powerful
	 * SNMP Get Next operation.
	 *
	 * Note that the gateway is not cleaned up as interfaces come and go.
	 * Consequently, we must assure that the gateway is still on an 
	 * attached network and is not a local interface. Additionally, we 
	 * assure that we have heard from the gateway in the last rip expiration 
	 * interval (currently 180 seconds).
	 */

        trace_tf(trace_global, TR_USER_1, 0,
             ("o_rip_get_peer - Traversing GW list"));

	RIP_GW_LIST(p) {
	    if (!if_withlcladdr(p->gw_addr, FALSE)  &&
		if_withdst(p->gw_addr) &&
		((current_time.tv_sec - p->gw_last_update_time.tv_sec) < RIP_T_EXPIRE)){
		register u_int32 cur_addr = ntohl(sock2ip(p->gw_addr));

                trace_tf(trace_global, TR_USER_1, 0,
                  ("o_rip_get_peer - cur_add is %X",cur_addr));

	        if (cur_addr > rip_peer_addr &&
		    (!new || cur_addr < new_addr)) {
		    new = p;
		    new_addr = cur_addr;
		}
	    }	
	} RIP_GW_LIST_END(p) ;

        trace_tf(trace_global, TR_USER_1, 0,
             ("o_rip_get_peer - GW list finished"));

	last_gwp = new;
    } else {
	/*
	 * Traverse Rip Gateway List simply search for the specified element. 
	 * This logic is in support of the SNMP Get operation. 
	 *
	 * Note that the gateway is not cleaned up as interfaces come and go.
	 * Consequently, we must assure that the gateway is still on an 
	 * attached network and is not a local interface. Additionally, we 
	 * assure that we have heard from the gateway in the last rip expiration 
	 * interval (currently 180 seconds).
	 */
	last_gwp = (gw_entry *) 0;
	RIP_GW_LIST(p) {
	    if (!if_withlcladdr(p->gw_addr, FALSE) &&
		if_withdst(p->gw_addr) &&
		((current_time.tv_sec - p->gw_last_update_time.tv_sec) < RIP_T_EXPIRE)) {
		register u_int32 cur_addr = ntohl(sock2ip(p->gw_addr));
		
		if (cur_addr == rip_peer_addr) {
		    last_gwp = p;
		    break;
		}
	    } 
	} RIP_GW_LIST_END(p) ;
    }
   return last_gwp;
}

/*
 * Return the requested object for the rip2PeerTable. 
 */
static int
o_rip_peer __PF3(oi, OI,
		  v, register struct type_SNMP_VarBind *,
		  offset, int)
{
    register int    i;
    register unsigned int *ip,
			  *jp;
    gw_entry *gwp;
    as_t domain = 0;
    register OID    oid = oi->oi_name;
    register OT	    ot = oi->oi_type;
    OID		    new;
    /*
     * The rip2PeerTable is indexed by rip2PeerAddress (i.e., IP Address).
     * Account for enough tokens to hold an IP address in the Object Identifier
     * calculations.
     */ 
#define	NDX_SIZE	sizeof(struct in_addr)
    switch (offset) {
    case type_SNMP_SMUX__PDUs_get__request:
        trace_tf(trace_global, TR_USER_1, 0,
           ("SNMP rip2peer - Get"));
	if (oid->oid_nelem - ot->ot_name->oid_nelem != NDX_SIZE) {
	    return int_SNMP_error__status_noSuchName;
	}
	gwp = o_rip_get_peer(oid->oid_elements + oid->oid_nelem - NDX_SIZE,
			      NDX_SIZE,
			      0);
	if (!gwp) {
	    return int_SNMP_error__status_noSuchName;
	}
	break;

    case type_SNMP_SMUX__PDUs_get__next__request:
        trace_tf(trace_global, TR_USER_1, 0,
           ("SNMP rip2peer - GetNext"));
	/* next request with incomplete instance? */
	if ((i = oid->oid_nelem - ot->ot_name->oid_nelem) != 0 && i < (int) NDX_SIZE) {
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
	} else
            if (i > (int) NDX_SIZE)
               oid->oid_nelem = ot->ot_name->oid_nelem + NDX_SIZE;


	/* next request with no instance? */
	if (oid->oid_nelem == ot->ot_name->oid_nelem) {
           trace_tf(trace_global, TR_USER_1, 0,
               ("SNMP rip2peer - GetNext no instance"));
	    gwp = o_rip_get_peer((unsigned int *) 0,
				  0,
				  TRUE);
	    if (!gwp) {
              trace_tf(trace_global, TR_USER_1, 0,
               ("SNMP rip2peer - GetNext no instance - no peers"));
		return NOTOK;
	    }
	    
	    if ((new = oid_extend(oid, NDX_SIZE)) == NULLOID) {
              trace_tf(trace_global, TR_USER_1, 0,
               ("SNMP rip2peer - GetNext no instance - oid_extend failed"));
		return int_SNMP_error__status_genErr;
	    }

	    ip = new->oid_elements + new->oid_nelem - NDX_SIZE;
            trace_tf(trace_global, TR_USER_1, 0,
               ("SNMP rip2peer - GetNext no instance - STR_OID"));
	    STR_OID(ip,
		    &sock2ip(gwp->gw_addr),
		    sizeof(sock2ip(gwp->gw_addr)));
	    if (v->name) {
		free_SNMP_ObjectName(v->name);
	    }
	    v->name = new;
	} else {
	    int j;

            trace_tf(trace_global, TR_USER_1, 0,
               ("SNMP rip2peer - GetNext with instance"));
	    gwp = o_rip_get_peer(ip = oid->oid_elements + ot->ot_name->oid_nelem,
				  j = oid->oid_nelem - ot->ot_name->oid_nelem,
				  TRUE);
	    if (!gwp) {
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
	    STR_OID(ip,
		    &sock2ip(gwp->gw_addr),
		    sizeof(sock2ip(gwp->gw_addr)));
	}  
	break;

    default:
	return int_SNMP_error__status_genErr;
    }
#undef	NDX_SIZE
    /*
     * Return the requested object for the specified rip2PeerEntry 
     */ 
    switch (ot2object(ot)->ot_info) {
    case rip2PeerAddress:
        trace_tf(trace_global, TR_USER_1, 0,
             ("SNMP rip2peer - GetNext rip2PeerAddress"));
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(gwp->gw_addr, 
				  (int *) 0));

    case rip2PeerDomain:
        trace_tf(trace_global, TR_USER_1, 0,
             ("SNMP rip2peer - GetNext rip2PeerDomain"));
	return o_string(oi, v, (caddr_t) &domain, DOMAIN_LENGTH);

    case rip2PeerLastUpdate: {
	/* 
	 * RFC 1155 Timeticks are defined as the number of hundreths of a second
	 * since some epoch. In the case of rip2PeerLastUpdate, this epoch is 
	 * sysUpTime. Since sysUpTime is not available, we use GateD start time
	 * which should be close to sysUpTime.         
	 */
#define CENTI_SECONDS_IN_SECOND 100
#define NANO_SECOND_ROUNDING_CONSTANT 500000
	u_int32 normalized_last_update_time;
	if (gwp->gw_last_update_time.tv_sec) {
	    normalized_last_update_time = 
		((gwp->gw_last_update_time.tv_sec - time_boot) * 
		 CENTI_SECONDS_IN_SECOND) +
	     (gwp->gw_last_update_time.tv_usec > NANO_SECOND_ROUNDING_CONSTANT);
	} else {
	    normalized_last_update_time = 0;
	}
 	return o_integer(oi,
			 v,
			 normalized_last_update_time);
    }

    case rip2PeerVersion:
	return o_integer(oi,
			 v,
			 gwp->gw_last_version_received);

    case rip2PeerRcvBadPackets:
	return o_integer(oi,
			 v,
			 gwp->gw_bad_packets);

    case rip2PeerRcvBadRoutes:
	return o_integer(oi,
			 v,
			 gwp->gw_bad_routes);
    }

    return int_SNMP_error__status_noSuchName;
}

/*
 * Rip MIB Initialization.
 */  
void
rip_init_mib __PF1(enabled, int)
{
    if (enabled) {
	if (!rip_intf_block_index) {
	    rip_intf_block_index = task_block_init(sizeof (struct rip_intf_entry),
						   "rip_intf_entry");
	}
	snmp_tree_register(&rip_mib_tree);
    } else {
	snmp_tree_unregister(&rip_mib_tree);
    }
}

#endif	/* PROTO_SNMP && MIB_RIP  */
