#ident	"@(#)bgp_mib.c	1.3"
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
#define	INCLUDE_RT_VAR
#include "include.h"
#include "inet.h"
#include "bgp_proto.h"
#include "bgp.h"
#include "bgp_var.h"
#include "snmp_isode.h"

PROTOTYPE(o_bgp,
	  static int,
	  (OI,
	   struct type_SNMP_VarBind *,
	   int));
PROTOTYPE(o_bgp_peer,
	  static int,
	  (OI,
	   struct type_SNMP_VarBind *,
	   int));
PROTOTYPE(o_bgp_path,
	  static int,
	  (OI,
	   struct type_SNMP_VarBind *,
	   int));

static struct object_table bgp_objects[] = {
#define	IbgpVersion			0
#define	IbgpLocalAs			1
#define	IbgpIdentifier			2
    OTE(bgpVersion, o_bgp, NULL),
    OTE(bgpLocalAs, o_bgp, NULL),
    OTE(bgpIdentifier, o_bgp, NULL),

#define	IbgpPeerIdentifier		3
#define	IbgpPeerState			4
#define	IbgpPeerAdminStatus		5
#define	IbgpPeerNegotiatedVersion	6
#define	IbgpPeerLocalAddr		7
#define	IbgpPeerLocalPort		8
#define	IbgpPeerRemoteAddr		9
#define	IbgpPeerRemotePort		10
#define	IbgpPeerRemoteAs		11
#define	IbgpPeerInUpdates		12
#define	IbgpPeerOutUpdates    		13
#define	IbgpPeerInTotalMessages		14
#define	IbgpPeerOutTotalMessages	15
#define	IbgpPeerLastError		16
    OTE(bgpPeerIdentifier, o_bgp_peer, NULL),
    OTE(bgpPeerState, o_bgp_peer, NULL),
#define	PSTATE_IDLE		1
#define	PSTATE_CONNECT		2
#define	PSTATE_ACTIVE		3
#define	PSTATE_OPENSENT		4
#define	PSTATE_OPENCONFIRM	5
#define	PSTATE_ESTABLISHED	6
    OTE(bgpPeerAdminStatus, o_bgp_peer, NULL),
    OTE(bgpPeerNegotiatedVersion, o_bgp_peer, NULL),
    OTE(bgpPeerLocalAddr, o_bgp_peer, NULL),
    OTE(bgpPeerLocalPort, o_bgp_peer, NULL),
    OTE(bgpPeerRemoteAddr, o_bgp_peer, NULL),
    OTE(bgpPeerRemotePort, o_bgp_peer, NULL),
    OTE(bgpPeerRemoteAs, o_bgp_peer, NULL),
    OTE(bgpPeerInUpdates, o_bgp_peer, NULL),
    OTE(bgpPeerOutUpdates, o_bgp_peer, NULL),
    OTE(bgpPeerInTotalMessages, o_bgp_peer, NULL),
    OTE(bgpPeerOutTotalMessages, o_bgp_peer, NULL),
    OTE(bgpPeerLastError, o_bgp_peer, NULL),

#define	IbgpPathAttrPeer		17
#define	IbgpPathAttrDestNetwork		18
#define	IbgpPathAttrOrigin		19
#define	IbgpPathAttrASPath		20
#define	IbgpPathAttrNextHop		21
#define	IbgpPathAttrInterASMetric	22
    OTE(bgpPathAttrPeer, o_bgp_path, NULL),
    OTE(bgpPathAttrDestNetwork, o_bgp_path, NULL),
    OTE(bgpPathAttrOrigin, o_bgp_path, NULL),
#define	PATH_ORIGIN_IGP		1
#define	PATH_ORIGIN_EGP		2
#define	PATH_ORIGIN_INCOMPLETE	3
    OTE(bgpPathAttrASPath, o_bgp_path, NULL),
    OTE(bgpPathAttrNextHop, o_bgp_path, NULL),
    OTE(bgpPathAttrInterASMetric, o_bgp_path, NULL),

    { NULL }
};

#define	MIB_ENABLED	1
#define	MIB_DISABLED	2

static struct snmp_tree bgp_mib_tree = {
    NULL, NULL,
    "bgp",
    NULLOID,
    readWrite,
    bgp_objects,
    0
};


/*
 *	Sort list maintenance
 */

static bgpPeer *bgp_sort_list = NULL;		/* List of sorted peers, for SNMP */
static bgpPeer *bgp_mib_last_bnp;
static unsigned int *bgp_mib_last;

/*
 * bgp_sort_add - add a peer to the sort list
 */
void
bgp_sort_add __PF1(add_bnp, register bgpPeer *)
{
    register u_long rmt;

    /*
     *  If there is any cached peer, reset the cache since with the
     *  get-next operator any addition may invalidate the cache.
     */
    if (bgp_mib_last) {
        snmp_last_free(&bgp_mib_last);
        bgp_mib_last_bnp = (bgpPeer *) 0;
    }

    /*
     * Load up the local rmt address.  This is an efficiency hack, since
     * most peers will be at different addresses.
     */
    rmt = ntohl(sock2ip(add_bnp->bgp_addr));

    /*
     * See if this goes first in the list
     */
    if (bgp_sort_list == NULL
	|| BGPSORT_LT(add_bnp, rmt, bgp_sort_list)) {
	add_bnp->bgp_sort_next = bgp_sort_list;
	bgp_sort_list = add_bnp;
    } else {
        register bgpPeer *bnpprev, *bnp;

	/*
	 * Not first, run through list
	 */
	for (bnpprev = bgp_sort_list;
	     (bnp = bnpprev->bgp_sort_next) != NULL;
	     bnpprev = bnp) {
	    if (BGPSORT_LT(add_bnp, rmt, bnp)) {
		break;
	    }
	}

	/* Already on sort list */
	assert(add_bnp != bnpprev &&
	       add_bnp != bnp);

	add_bnp->bgp_sort_next = bnp;
	bnpprev->bgp_sort_next = add_bnp;
    }
}


/*
 * bgp_sort_remove - remove a peer from the sort list
 */
void
bgp_sort_remove __PF1(rem_bnp, register bgpPeer *)
{
    /* If this is the cached peer, reset the cache */
    if (rem_bnp == bgp_mib_last_bnp) {
	snmp_last_free(&bgp_mib_last);
	bgp_mib_last_bnp = (bgpPeer *) 0;
    }
    
    /*
     * See if it is first on the list
     */

    /* Sort list empty */
    assert(bgp_sort_list);

    if (bgp_sort_list == rem_bnp) {
	bgp_sort_list = rem_bnp->bgp_sort_next;
    } else {
	register bgpPeer *bnp;

	/*
	 * Not first, must be further in
	 */
	for (bnp = bgp_sort_list;
	     bnp->bgp_sort_next != NULL;
	     bnp = bnp->bgp_sort_next) {
	    if (bnp->bgp_sort_next == rem_bnp) {
		break;
	    }
	}

	/* Not on sort list */
	assert(bnp->bgp_sort_next);

	bnp->bgp_sort_next = rem_bnp->bgp_sort_next;
    }
    rem_bnp->bgp_sort_next = NULL;
}


/* Since we support multiple connections per peer but the MIB does not, */
/* we only acknowledge the first non-test peer with a given address */

#undef	BGP_SORT_LIST
#undef	BGP_SORT_LIST_END

#define	BGP_SORT_LIST(bnp) \
    for (bnp = bgp_sort_list; bnp; bnp = bnp->bgp_sort_next) { \
	if (bnp->bgp_type != BGPG_TEST)
				
#define	BGP_SORT_LIST_END(bnp) \
	while (bnp->bgp_sort_next && \
	    sockaddrcmp_in(bnp->bgp_addr, bnp->bgp_sort_next->bgp_addr)) { \
	    bnp = bnp->bgp_sort_next; \
	} \
    }

static int
o_bgp __PF3(oi, OI,
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
    case IbgpVersion:
        {
#define	VSTRING_LENGTH	(((BGP_BEST_VERSION - 1) / NBBY) + 1)
	    register int vers;
	    byte versions[VSTRING_LENGTH];

	    for (vers = 0; vers < sizeof versions; vers++) {
		versions[vers] = (byte) 0;
	    }
	    
	    for (vers = BGP_WORST_VERSION; vers <= BGP_BEST_VERSION; vers++) {
		if (BGP_KNOWN_VERSION(vers)) {
		    /* v is a supported version */
		    register int v1 = vers - 1;

		    versions[v1 / NBBY] |= 0x80 >> (v1 % NBBY);
		}
	    }

	    return o_string(oi, v, (caddr_t) versions, VSTRING_LENGTH);
#undef	VSTRING_LENGTH
	}

    case IbgpLocalAs:
	return o_integer(oi, v, inet_autonomous_system);

    case IbgpIdentifier:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(inet_routerid,
				  (int *) 0));
    }

    return int_SNMP_error__status_noSuchName;
}

/**/


static bgpPeer *
o_bgp_get_peer __PF3(ip, register unsigned int *,
		     len, u_int,
		     isnext, int)
{

    if (snmp_last_match(&bgp_mib_last, ip, len, isnext)) {
	return bgp_mib_last_bnp;
    }

    if (len) {
	u_long bnp_addr;
	register bgpPeer *p;

	oid2ipaddr(ip, &bnp_addr);

	GNTOHL(bnp_addr);

	if (isnext) {
	    register bgpPeer *new = (bgpPeer *) 0;
	    register u_long new_addr = 0;

	    BGP_SORT_LIST(p) {
		register u_long cur_addr = ntohl(sock2ip(p->bgp_addr));

		if (cur_addr > bnp_addr &&
		    (!new || cur_addr < new_addr)) {
		    new = p;
		    new_addr = cur_addr;
		}
	    } BGP_SORT_LIST_END(p) ;

	    bgp_mib_last_bnp = new;
	} else {
	    bgp_mib_last_bnp = (bgpPeer *) 0;
	    

	    BGP_SORT_LIST(p) {
		register u_long cur_addr = ntohl(sock2ip(p->bgp_addr));
		
		if (cur_addr == bnp_addr) {
		    bgp_mib_last_bnp = p;
		    break;
		} else if (cur_addr > bnp_addr) {
		    break;
		}
	    } BGP_SORT_LIST_END(p) ;
	}
    } else {
	bgp_mib_last_bnp = bgp_n_peers ? bgp_sort_list : (bgpPeer *) 0;
    }

    return bgp_mib_last_bnp;
}


static int
o_bgp_peer_info __PF4(oi, OI,
		      v, register struct type_SNMP_VarBind *,
		      ifvar, int,
		      vp, void_t)
{
    register bgpPeer *bnp = (bgpPeer *) vp;

    switch (ifvar) {
    case IbgpPeerIdentifier:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(sockbuild_in(0, bnp->bgp_id),
				  (int *) 0));
	
    case IbgpPeerState:
    {
	int state;

	switch (bnp->bgp_state) {
	case BGPSTATE_IDLE:
	    state = PSTATE_IDLE;
	    break;
		
	case BGPSTATE_CONNECT:
	    state = PSTATE_CONNECT;
	    break;
		
	case BGPSTATE_ACTIVE:
	    state = PSTATE_ACTIVE;
	    break;
		
	case BGPSTATE_OPENSENT:
	    state = PSTATE_OPENSENT;
	    break;
		
	case BGPSTATE_OPENCONFIRM:
	    state = PSTATE_OPENCONFIRM;
	    break;
		
	case BGPSTATE_ESTABLISHED:
	    state = PSTATE_ESTABLISHED;
	    break;

	default:
	    state = -1;
	}

	return o_integer(oi, v, state);
    }
	
    case IbgpPeerAdminStatus:
	return o_integer(oi, v, MIB_ENABLED/*XXX*/);
	
    case IbgpPeerNegotiatedVersion:
	return o_integer(oi, v, bnp->bgp_version);

    case IbgpPeerLocalAddr:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(bnp->bgp_myaddr ? bnp->bgp_myaddr : inet_addr_default,
				  (int *) 0));

    case IbgpPeerLocalPort:
	return o_integer(oi, v, bnp->bgp_myaddr ? ntohs(sock2port(bnp->bgp_myaddr)) : 0);

    case IbgpPeerRemoteAddr:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(bnp->bgp_addr,
				  (int *) 0));

    case IbgpPeerRemotePort:
	return o_integer(oi, v, ntohs(sock2port(bnp->bgp_addr)));

    case IbgpPeerRemoteAs:
	return o_integer(oi, v, bnp->bgp_peer_as);
	
    case IbgpPeerInUpdates:
	return o_integer(oi, v, bnp->bgp_in_updates);
	
    case IbgpPeerOutUpdates:
	return o_integer(oi, v, bnp->bgp_out_updates);
	
    case IbgpPeerInTotalMessages:
	return o_integer(oi, v, bnp->bgp_in_updates + bnp->bgp_in_notupdates);
	
    case IbgpPeerOutTotalMessages:
	return o_integer(oi, v, bnp->bgp_out_updates + bnp->bgp_out_notupdates);
	
    case IbgpPeerLastError:
	return o_string(oi, v, (caddr_t) &bnp->bgp_lasterror, sizeof bnp->bgp_lasterror);
    }

    return int_SNMP_error__status_noSuchName;
}


static int
o_bgp_peer __PF3(oi, OI,
		 v, register struct type_SNMP_VarBind *,
		 offset, int)
{
    register int    i;
    register unsigned int *ip,
			  *jp;
    register bgpPeer *bnp;
    register OID    oid = oi->oi_name;
    register OT	    ot = oi->oi_type;
    OID		    new;

    /* INDEX { bgpPeerRemoteAddr } */
#define	NDX_SIZE	(sizeof (struct in_addr))

    switch (offset) {
    case type_SNMP_SMUX__PDUs_get__request:
	if (oid->oid_nelem != ot->ot_name->oid_nelem + NDX_SIZE) {
		return int_SNMP_error__status_noSuchName;
	    }
	bnp = o_bgp_get_peer(oid->oid_elements + oid->oid_nelem - NDX_SIZE,
			     sizeof (struct in_addr),
			     0);
	if (!bnp) {
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
	    bnp = o_bgp_get_peer((unsigned int *) 0,
				 0,
				 TRUE);
	    if (!bnp) {
		return NOTOK;
	    }

	    if ((new = oid_extend(oid, NDX_SIZE)) == NULLOID) {
		return int_SNMP_error__status_genErr;
	    }

	    ip = new->oid_elements + new->oid_nelem - NDX_SIZE;
	    STR_OID(ip, &sock2ip(bnp->bgp_addr), sizeof (sock2ip(bnp->bgp_addr)));
		
	    if (v->name) {
		free_SNMP_ObjectName(v->name);
	    }
	    v->name = new;
	} else {
	    int j;

	    bnp = o_bgp_get_peer(ip = oid->oid_elements + ot->ot_name->oid_nelem,
				 (u_int) (j = oid->oid_nelem - ot->ot_name->oid_nelem),
				 TRUE);
	    if (!bnp) {
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
	    STR_OID(ip, &sock2ip(bnp->bgp_addr), sizeof (sock2ip(bnp->bgp_addr)));
	}
	break;

    default:
	return int_SNMP_error__status_genErr;
    }
#undef	NDX_SIZE

    return o_bgp_peer_info(oi, v, ot2object(ot)->ot_info, (void_t) bnp);
}


static rt_entry *
o_bgp_get_match __PF2(rth, register rt_head *,
		      data, void_t)
{
    register bgpPeer *p;
    register u_long this_peer;

    if (data) {
	this_peer = *((u_long *)data);
    } else {
	this_peer = 0;
    }
 
    BGP_SORT_LIST(p) {
	if (!this_peer || ntohl(sock2ip(p->bgp_addr)) > this_peer) {
	    register rt_entry *rt;
	    register gw_entry *gwp = &p->bgp_gw;

	    RT_ALLRT(rt, rth) {
		if (BIT_TEST(rt->rt_state, RTS_DELETE)) {
		    break;
		}
		if (rt->rt_gwp == gwp) {
		    return rt;
		}
	    } RT_ALLRT_END(rt, rth) ;
	}
    } BGP_SORT_LIST_END(p) ;

    return (rt_entry *) 0;
}

static rt_entry *
o_bgp_get_match_exact __PF2(rth, register rt_head *,
			    data, void_t)
{
    register rt_entry *rt;
    register gw_entry *gwp = (gw_entry *)(data);

    RT_ALLRT(rt, rth) {
	if (BIT_TEST(rt->rt_state, RTS_DELETE)) {
	    break;
	}
	if (rt->rt_gwp == gwp) {
	    return rt;
	}
    } RT_ALLRT_END(rt, rth);

    return (rt_entry *) 0;
}


static rt_entry *
o_bgp_get_path __PF3(ip, register unsigned int *,
		     len, u_int,
		     isnext, int)    
{
    static rt_entry *last_rt = (rt_entry *) 0;
    static unsigned int *last = (unsigned int *) 0;
    static int last_quantum = 0;

    if (last_quantum != snmp_quantum) {
	last_quantum = snmp_quantum;

	if (last) {
	    task_mem_free((task *) 0, (caddr_t) last);
	    last = (unsigned int *) 0;
	}
    }
    if (snmp_last_match(&last, ip, len, isnext)) {
	return last_rt;
    }

    if (len) {
	u_long peer;
	sockaddr_un *dest;
	register bgpPeer *p;

	dest = sockbuild_in(0, 0);
	oid2ipaddr(ip, &sock2ip(dest));

	oid2ipaddr(&ip[sizeof (struct in_addr)], &peer);

	if (isnext) {
	    /* Find the next peer in the next valid route */

	    GNTOHL(peer);

	    /* First look up this route and see if there is a next peer */
	    last_rt = rt_table_get(dest,
				   (sockaddr_un *) 0,
				   o_bgp_get_match,
				   (void_t)(&peer));

	    /* That didn't work, find the first peer in the next route */
	    if (!last_rt) {
		last_rt = rt_table_getnext(dest,
					   (sockaddr_un *) 0,
					   AF_INET,
					   o_bgp_get_match,
					   (void_t) 0);
	    }
	} else {
	    /* Find the route with this peer */

	    /* First find the peer */
	    BGP_SORT_LIST(p) {
		if (peer == sock2ip(p->bgp_addr)) {

		    /* Now look up the route */
		    last_rt = rt_table_get(dest,
					   (sockaddr_un *) 0,
					   o_bgp_get_match_exact,
					   (void_t)&(p->bgp_gw));
		    break;
		}
	    } BGP_SORT_LIST_END(p) ;
	}
    } else {
	/* Find the first peer of the first route */

	last_rt = rt_table_getnext((sockaddr_un *) 0,
				   (sockaddr_un *) 0,
				   AF_INET,
				   o_bgp_get_match,
				   (void_t) 0);
    }

    return last_rt;
}


static int
o_bgp_path __PF3(oi, OI,
		 v, register struct type_SNMP_VarBind *,
		 offset, int)
{
    register int    i;
    register unsigned int *ip,
			  *jp;
    register OID    oid = oi->oi_name;
    register OT	    ot = oi->oi_type;
    OID		    new;
    register rt_entry *rt;
    bgpPeer *bnp;

    /* INDEX { bgpPathAttrDestNetwork, bgpPathAttrPeer } */
#define	NDX_SIZE	(sizeof (struct in_addr) + sizeof (struct in_addr))
    
    switch (offset) {
    case type_SNMP_SMUX__PDUs_get__request:
	if (oid->oid_nelem - ot->ot_name->oid_nelem != NDX_SIZE) {
	    return int_SNMP_error__status_noSuchName;
	}
	rt = o_bgp_get_path(oid->oid_elements + oid->oid_nelem - NDX_SIZE,
			    NDX_SIZE,
			    0);
	if (!rt) {
	    return int_SNMP_error__status_noSuchName;
	}
	bnp = (bgpPeer *) rt->rt_gwp->gw_task->task_data;
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

	/* Next request with no instance? */
	if (oid->oid_nelem == ot->ot_name->oid_nelem) {
	    rt = o_bgp_get_path((unsigned int *) 0,
				0,
				TRUE);
	    if (!rt) {
		return NOTOK;
	    }
	    bnp = (bgpPeer *) rt->rt_gwp->gw_task->task_data;
	    
	    if ((new = oid_extend(oid, NDX_SIZE)) == NULLOID) {
		return int_SNMP_error__status_genErr;
	    }

	    ip = new->oid_elements + new->oid_nelem - NDX_SIZE;
	    STR_OID(ip, &sock2ip(rt->rt_dest), sizeof (sock2ip(rt->rt_dest)));
	    STR_OID(ip, &sock2ip(bnp->bgp_addr), sizeof (sock2ip(bnp->bgp_addr)));
		
	    if (v->name) {
		free_SNMP_ObjectName(v->name);
	    }
	    v->name = new;
	} else {
	    int j;

	    rt = o_bgp_get_path(ip = oid->oid_elements + ot->ot_name->oid_nelem,
				(u_int) (j = oid->oid_nelem - ot->ot_name->oid_nelem),
				TRUE);
	    if (!rt) {
		return NOTOK;
	    }
	    bnp = (bgpPeer *) rt->rt_gwp->gw_task->task_data;

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
	    STR_OID(ip, &sock2ip(rt->rt_dest), sizeof (sock2ip(rt->rt_dest)));
	    STR_OID(ip, &sock2ip(bnp->bgp_addr), sizeof (sock2ip(bnp->bgp_addr)));
	}
	break;

    default:
	return int_SNMP_error__status_genErr;
    }
#undef	NDX_SIZE

    switch (ot2object(ot)->ot_info) {
    case IbgpPathAttrPeer:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(bnp->bgp_addr,
				  (int *) 0));

    case IbgpPathAttrDestNetwork:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(rt->rt_dest,
				  (int *) 0));
    
    case IbgpPathAttrOrigin:
        {
	    int origin;

	    switch (rt->rt_aspath->path_origin) {
	    case PATH_ORG_IGP:
		origin = PATH_ORIGIN_IGP;
		break;

	    case PATH_ORG_EGP:
		origin = PATH_ORIGIN_EGP;
		break;

	    case PATH_ORG_XX:
		origin = PATH_ORIGIN_INCOMPLETE;
		break;

	    default:
		origin = -1;
	    }
	    
	    return o_integer(oi, v, origin);
	}

    case IbgpPathAttrASPath:
	/* The path is stored in network byte order and that is basically how the MIB wants to see it */
	/* XXX - what about a zero length AS path? */
	return o_string(oi, v, (caddr_t) PATH_PTR(rt->rt_aspath), rt->rt_aspath->path_len);

    case IbgpPathAttrNextHop:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(RT_ROUTER(rt),
				  (int *) 0));
	
    case IbgpPathAttrInterASMetric:
	return o_integer(oi, v, rt->rt_metric);
    }

    return int_SNMP_error__status_noSuchName;
}


/**/
/* Traps */

#define	bgpEstablished		1
#define	bgpBackwardTransition	2

#define	TRAP_NUM_VARS	3

static int bgp_trap_varlist[TRAP_NUM_VARS] = {
    IbgpPeerRemoteAddr,
    IbgpPeerLastError,
    IbgpPeerState
};


void
bgp_trap_established __PF1(bnp, bgpPeer *)
{
    struct type_SNMP_VarBindList bindlist[TRAP_NUM_VARS];
    struct type_SNMP_VarBind var[TRAP_NUM_VARS];

    if (snmp_varbinds_build(TRAP_NUM_VARS,
			    bindlist,
			    var,
			    bgp_trap_varlist,
			    &bgp_mib_tree,
			    o_bgp_peer_info,
			    (void_t) bnp) == NOTOK) {
	trace_log_tf(snmp_trace_options,
		     0,
		     LOG_ERR,
		     ("bgp_trap_established: snmp_varbinds_build failed"));
	goto out;
    }

    snmp_trap("bgpEstablished",
	      bgp_mib_tree.r_name,
	      int_SNMP_generic__trap_enterpriseSpecific,
	      bgpEstablished,
	      bindlist);

out:;
    snmp_varbinds_free(bindlist);
}


void
bgp_trap_backward __PF1(bnp, bgpPeer *)
{
    struct type_SNMP_VarBindList bindlist[TRAP_NUM_VARS];
    struct type_SNMP_VarBind var[TRAP_NUM_VARS];

    if (snmp_varbinds_build(TRAP_NUM_VARS,
			    bindlist,
			    var,
			    bgp_trap_varlist,
			    &bgp_mib_tree,
			    o_bgp_peer_info,
			    (void_t) bnp) == NOTOK) {
	trace_log_tf(snmp_trace_options,
		     0,
		     LOG_ERR,
		     ("bgp_trap_backward: snmp_varbinds_build failed"));
	goto out;
    }

    snmp_trap("bgpBackwardTransition",
	      bgp_mib_tree.r_name,
	      int_SNMP_generic__trap_enterpriseSpecific,
	      bgpBackwardTransition,
	      bindlist);

out:;
    snmp_varbinds_free(bindlist);
}

/**/

void
bgp_init_mib __PF1(enabled, int)
{
    if (enabled) {
	snmp_tree_register(&bgp_mib_tree);
    } else {
	snmp_tree_unregister(&bgp_mib_tree);
    }
}
