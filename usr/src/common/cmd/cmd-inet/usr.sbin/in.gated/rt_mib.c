#ident	"@(#)rt_mib.c	1.3"
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
#ifdef	PROTO_OSPF
#include "ospf.h"
#endif	/* PROTO_OSPF */

#include "snmp_isode.h"

/**/

/* IP routine table (MIB-II) */

/* INDEX   { ipRouteDest } */
#define	NDX_SIZE	(sizeof (struct in_addr))

PROTOTYPE(o_ip_route,
	  static int,
	  (OI,
	   struct type_SNMP_VarBind *,
	   int));

static struct object_table rt_iproute_objects[] = {
#define	IipRouteDest	0
#define	IipRouteIfIndex	1
#define	IipRouteMetric1	2
#define	IipRouteMetric2	3
#define	IipRouteMetric3	4
#define	IipRouteMetric4	5
#define	IipRouteNextHop	6
#define	IipRouteType	7
#define	IipRouteProto	8
#define	IipRouteAge	9
#define	IipRouteMask	10
#define IipRouteMetric5	11
#define IipRouteInfo	12
    OTE(ipRouteDest, o_ip_route, NULL),
    OTE(ipRouteIfIndex, o_ip_route, NULL),
    OTE(ipRouteMetric1, o_ip_route, NULL),
#define METRIC_NONE	(-1)
    OTE(ipRouteMetric2, o_ip_route, NULL),
    OTE(ipRouteMetric3, o_ip_route, NULL),
    OTE(ipRouteMetric4, o_ip_route, NULL),
    OTE(ipRouteNextHop, o_ip_route, NULL),
    OTE(ipRouteType, o_ip_route, NULL),
#define RTYPE_OTHER	1
#define RTYPE_INVALID	2
#define	RTYPE_DIRECT	3
#define RTYPE_INDIRECT	4
    OTE(ipRouteProto, o_ip_route, NULL),
#define SPROTO_OTHER	1
#define SPROTO_LOCAL	2
#define SPROTO_NETMGMT	3
#define	SPROTO_ICMP	4
#define	SPROTO_EGP	5
#define	SPROTO_GGP	6
#define	SPROTO_HELLO	7
#define SPROTO_RIP	8
#define	SPROTO_ISIS	9
#define	SPROTO_ESIS	10
#define	SPROTO_CISCO	11
#define	SPROTO_BBN	12
#define	SPROTO_OSPF	13
#define	SPROTO_BGP	14
    OTE(ipRouteAge, o_ip_route, NULL),
    OTE(ipRouteMask, o_ip_route, NULL),
    OTE(ipRouteMetric5, o_ip_route, NULL),
    OTE(ipRouteInfo, o_ip_route, NULL),

    { NULL }
};


static struct snmp_tree rt_iproute_tree = {
    NULL, NULL,
    "ipRouteTable",
    NULLOID,
    readWrite,
    rt_iproute_objects,
    0
};


static rt_entry *
o_ip_route_match __PF2(rth, rt_head *,
		       data, void_t)
{
    return rth->rth_active;
}


static inline rt_entry *
o_rt_lookup __PF2(l_dst, sockaddr_un *,
		  l_isnext, int)
{
    rt_entry *rt;
    
    if (l_isnext) {
	rt = rt_table_getnext(l_dst,
			       (sockaddr_un *) 0,
			       AF_INET,
			       o_ip_route_match,
			       (void_t) 0);
    } else {
	rt = rt_table_get(l_dst,
			  (sockaddr_un *) 0,
			  o_ip_route_match,
			  (void_t) 0);
    }

    return rt;
}


static rt_entry *o_ip_route_last_rt;
static unsigned int *o_ip_route_last;

static rt_entry *
o_ip_route_lookup __PF3(ip, register unsigned int *,
			len, u_int,
			isnext, int)
{

    if (snmp_last_match(&o_ip_route_last, ip, len, isnext)) {
	return o_ip_route_last_rt;
    }

    if (len) {
	sockaddr_un *dst;
	
	dst = sockbuild_in(0, 0);
	oid2ipaddr(ip, &sock2in(dst));

	o_ip_route_last_rt = o_rt_lookup(dst, isnext);
    } else {
	o_ip_route_last_rt = o_rt_lookup((sockaddr_un *) 0, TRUE);
    }

    return o_ip_route_last_rt;
}


static int
o_ip_route __PF3(oi, OI,
		 v, register struct type_SNMP_VarBind *,
		 offset, int)
{
    register int i;
    register unsigned int *ip, *jp;
    register rt_entry *rt;
    register OID oid = oi->oi_name;
    register OT	ot = oi->oi_type;
    OID	new;

    switch (offset) {
    case type_SNMP_SMUX__PDUs_get__request:
	if (oid->oid_nelem != ot->ot_name->oid_nelem + NDX_SIZE) {
	    return int_SNMP_error__status_noSuchName;
	}
	
	if (!(rt = o_ip_route_lookup(oid->oid_elements + oid->oid_nelem - NDX_SIZE, NDX_SIZE, FALSE))) {
	    return int_SNMP_error__status_noSuchName;
	}
	break;

    case type_SNMP_SMUX__PDUs_get__next__request:
	/* next request with incomplete instance? */
	if ((i = oid->oid_nelem - ot->ot_name->oid_nelem) &&
	    i < NDX_SIZE) {
	    for (jp = (ip = oid->oid_elements + ot->ot_name->oid_nelem - 1) + i;
		 jp > ip;
		 jp--) {
		if (*jp != 0) {
		    break;
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
	}

	/* next request with no instance? */
	if (oid->oid_nelem == ot->ot_name->oid_nelem) {
	    if (!(rt = o_ip_route_lookup(oid->oid_elements + oid->oid_nelem,
					 0,
					 TRUE))) {
		return NOTOK;
	    }

	    if ((new = oid_extend(oid, NDX_SIZE)) == NULLOID) {
		return int_SNMP_error__status_genErr;
	    }

	    ip = new->oid_elements + new->oid_nelem - NDX_SIZE;
	    STR_OID(ip, &sock2ip(rt->rt_dest), sizeof (sock2ip(rt->rt_dest)));
		
	    if (v->name) {
		free_SNMP_ObjectName(v->name);
	    }
	    v->name = new;
	} else {
	    int j;

	    if (!(rt = o_ip_route_lookup(ip = oid->oid_elements + ot->ot_name->oid_nelem,
					 (u_int) (j = oid->oid_nelem - ot->ot_name->oid_nelem),
					 TRUE))) {
		return NOTOK;
	    }

	    if ((i = j - NDX_SIZE) < 0) {
		if ((new = oid_extend(oid, -i)) == NULLOID)
		    return int_SNMP_error__status_genErr;
		if (v->name) {
		    free_SNMP_ObjectName(v->name);
		}
		v->name = oid = new;
	    } else if (i > 0) {
		oid->oid_nelem -= i;
	    }
		
	    ip = oid->oid_elements + ot->ot_name->oid_nelem;
	    STR_OID(ip, &sock2ip(rt->rt_dest), sizeof (sock2ip(rt->rt_dest)));
	}
	break;

    default:
	return int_SNMP_error__status_genErr;
    }

    switch (ot2object(ot)->ot_info) {
	int value;
	
    case IipRouteDest:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(rt->rt_dest,
				  (int *) 0));

    case IipRouteIfIndex:
	if (BIT_TEST(rt->rt_state, RTS_REJECT)
	    || !RT_IFAP(rt)) {
	    value = 0;
	} else {
	    value = RT_IFAP(rt)->ifa_link->ifl_index;
	}
	return o_integer(oi, v, value);
	
    case IipRouteMetric1:
	return o_integer(oi, v, rt->rt_metric);

    case IipRouteMetric2:
	return o_integer(oi, v, rt->rt_metric2);

    case IipRouteMetric3:
    case IipRouteMetric4:
    case IipRouteMetric5:
	return o_integer(oi, v, METRIC_NONE);

    case IipRouteNextHop:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix((!RT_ROUTER(rt)
				   || BIT_TEST(rt->rt_state, RTS_REJECT))
				  ? inet_addr_default : RT_ROUTER(rt),
				  (int *) 0));

    case IipRouteType:
	if (BIT_TEST(rt->rt_state, RTS_REJECT)) {
	    value = RTYPE_OTHER;
	} else if (BIT_TEST(rt->rt_state, RTS_GATEWAY)) {
	    value = RTYPE_INDIRECT;
	} else {
	    value = RTYPE_DIRECT;
	}
	return o_integer(oi, v, value);

    case IipRouteProto:
	switch (rt->rt_gwp->gw_proto) {
	default:
	case RTPROTO_DIRECT:
	    value = SPROTO_LOCAL;
	    break;

	case RTPROTO_STATIC:
	    value = SPROTO_LOCAL;
	    break;

	case RTPROTO_KERNEL:
	    if (BIT_TEST(rt->rt_state, RTS_NOADVISE)) {
		value = SPROTO_OTHER;
	    } else {
		value = SPROTO_LOCAL;
	    }
	    break;

	case RTPROTO_AGGREGATE:
	    value = SPROTO_OTHER;
	    break;
	    
	case RTPROTO_SNMP:
	    value = SPROTO_NETMGMT;
	    break;
		
	case RTPROTO_REDIRECT:
	    value = SPROTO_ICMP;
	    break;

	case RTPROTO_EGP:
	    value = SPROTO_EGP;
	    break;

	case RTPROTO_HELLO:
	    value = SPROTO_HELLO;
	    break;

	case RTPROTO_RIP:
	    value = SPROTO_RIP;
	    break;

	case RTPROTO_IGRP:
	    value = SPROTO_CISCO;
	    break;

	case RTPROTO_OSPF:
	case RTPROTO_OSPF_ASE:
	    value = SPROTO_OSPF;
	    break;

	case RTPROTO_BGP:
	    value = SPROTO_BGP;
	    break;

	case RTPROTO_ISIS:
	    value = SPROTO_ISIS;
	    break;
	}
	return o_integer(oi, v, value);

    case IipRouteAge:
	return o_integer(oi, v, rt_age(rt));
	
    case IipRouteMask:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(rt->rt_dest_mask,
				  (int *) 0));
	
    case IipRouteInfo:
	return o_specific(oi, v, (caddr_t) snmp_nullSpecific);
    }
	
    return int_SNMP_error__status_noSuchName;
}

#undef	NDX_SIZE


/**/

/* IP Forwarding table */

PROTOTYPE(o_ip_forward_number,
	  static int,
	  (OI,
	   struct type_SNMP_VarBind *,
	   int));
PROTOTYPE(o_ip_forward,
	  static int,
	  (OI,
	   struct type_SNMP_VarBind *,
	   int));

/* INDEX {ipForwardDest, ipForwardProto, ipForwardPolicy, ipForwardNextHop} */
#define	NDX_SIZE	(sizeof (struct in_addr) + 1 + 1 + sizeof (struct in_addr))

static struct object_table rt_ipforward_objects[] = {
#define	IipForwardNumber		0
    OTE(ipForwardNumber, o_ip_forward_number, NULL),
#define	IipForwardDest		1
#define	IipForwardMask		2
#define	IipForwardPolicy	3
#define	IipForwardNextHop	4
#define	IipForwardIfIndex	5
#define	IipForwardType		6
#define	IipForwardProto		7
#define	IipForwardAge		8
#define	IipForwardInfo		9
#define	IipForwardNextHopAS	10
#define	IipForwardMetric1	11
#define	IipForwardMetric2	12
#define	IipForwardMetric3	13
#define	IipForwardMetric4	14
#define IipForwardMetric5	15
    OTE(ipForwardDest, o_ip_forward, NULL),
    OTE(ipForwardMask, o_ip_forward, NULL),
    OTE(ipForwardPolicy, o_ip_forward, NULL),
    OTE(ipForwardNextHop, o_ip_forward, NULL),
    OTE(ipForwardIfIndex, o_ip_forward, NULL),
    OTE(ipForwardType, o_ip_forward, NULL),
#define FTYPE_OTHER	1
#define FTYPE_INVALID	2
#define	FTYPE_LOCAL	3
#define FTYPE_REMOTE	4
    OTE(ipForwardProto, o_ip_forward, NULL),
#define FPROTO_OTHER	1
#define FPROTO_LOCAL	2
#define FPROTO_NETMGMT	3
#define	FPROTO_ICMP	4
#define	FPROTO_EGP	5
#define	FPROTO_GGP	6
#define	FPROTO_HELLO	7
#define FPROTO_RIP	8
#define	FPROTO_ISIS	9
#define	FPROTO_ESIS	10
#define	FPROTO_CISCO	11
#define	FPROTO_BBN	12
#define	FPROTO_OSPF	13
#define	FPROTO_BGP	14
#define	FPROTO_IDPR	15
    OTE(ipForwardAge, o_ip_forward, NULL),
    OTE(ipForwardInfo, o_ip_forward, NULL),
    OTE(ipForwardNextHopAS, o_ip_forward, NULL),
    OTE(ipForwardMetric1, o_ip_forward, NULL),
#define METRIC_NONE	(-1)
    OTE(ipForwardMetric2, o_ip_forward, NULL),
    OTE(ipForwardMetric3, o_ip_forward, NULL),
    OTE(ipForwardMetric4, o_ip_forward, NULL),
    OTE(ipForwardMetric5, o_ip_forward, NULL),

    { NULL }
};


static struct snmp_tree rt_ipforward_tree = {
    NULL, NULL,
    "ipForward",
    NULLOID,
    readWrite,
    rt_ipforward_objects,
    0
};


static inline int
o_ip_forward_proto __PF1(p_rt, rt_entry *)
{
    int proto;
    
    switch (p_rt->rt_gwp->gw_proto) {
    default:
    case RTPROTO_DIRECT:
	proto = FPROTO_LOCAL;
	break;

    case RTPROTO_STATIC:
	proto = FPROTO_NETMGMT;
	break;
	
    case RTPROTO_KERNEL:
	if (BIT_TEST(p_rt->rt_state, RTS_NOADVISE)) {
	    proto = FPROTO_OTHER;
	} else {
	    proto = FPROTO_NETMGMT;
	}
	break;

    case RTPROTO_AGGREGATE:
	proto = FPROTO_OTHER;
	break;
	
    case RTPROTO_SNMP:
	proto = FPROTO_NETMGMT;
	break;
		
    case RTPROTO_REDIRECT:
	proto = FPROTO_ICMP;
	break;

    case RTPROTO_EGP:
	proto = FPROTO_EGP;
	break;

    case RTPROTO_HELLO:
	proto = FPROTO_HELLO;
	break;

    case RTPROTO_RIP:
	proto = FPROTO_RIP;
	break;

    case RTPROTO_IGRP:
	proto = FPROTO_CISCO;
	break;

    case RTPROTO_OSPF:
    case RTPROTO_OSPF_ASE:
	proto = FPROTO_OSPF;
	break;

    case RTPROTO_BGP:
	proto = FPROTO_BGP;
	break;

    case RTPROTO_IDPR:
	proto = FPROTO_IDPR;
	break;

    case RTPROTO_ISIS:
	proto = FPROTO_ISIS;
	break;
    }

    return proto;
}


static int
o_ip_forward_number __PF3(oi, OI,
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
	int forward_number;
	sockaddr_un *dst;
	rt_entry *rt;
	
    case IipForwardNumber:
#if   RT_N_MULTIPATH > 1
	/* XXX - This is slow, but is the only way to get a correct result
	 * right now
	 */
	for (forward_number = 0, dst = (sockaddr_un *) 0;
	     (rt = o_rt_lookup(dst, TRUE));
	     dst = rt->rt_dest) {
	    forward_number += rt->rt_n_gw;
	}
	return o_integer(oi, v, forward_number);
#else /* RT_N_MULTIPATH > 1 */
	return o_integer(oi, v, rtaf_info[AF_INET].rtaf_actives);
#endif        /* RT_N_MULTIPATH */
    }

    return int_SNMP_error__status_noSuchName;
}


static rt_entry *o_ip_forward_last_rt;
static unsigned int *o_ip_forward_last;

static rt_entry *
o_ip_forward_lookup __PF4(ip, register unsigned int *,
			  len, u_int,
			  isnext, int,
			  router_index, int *)
{
    static int last_index;
    sockaddr_un *dst;

    if (snmp_last_match(&o_ip_forward_last, ip, len, isnext)) {
	goto Return;
    }

    if (len) {
	int proto;
	u_int32 addr;
	sockaddr_un *gw;

	dst = sockbuild_in(0, 0);
	oid2ipaddr(ip, &addr);
	dst = sockbuild_in(0, addr);
	proto = ip[sizeof (struct in_addr)];
	if (ip[sizeof (struct in_addr) + 1]) {
	    /* We don't support TOS */
	    *router_index = last_index = 0;
	    o_ip_forward_last_rt = (rt_entry *) 0;
	    goto Return;
	}
	oid2ipaddr(ip + sizeof (struct in_addr) + 1 + 1, &addr);
	gw = sockbuild_in(0, addr);
	if (!sock2ip(gw)) {
	    goto Next;
	}

	o_ip_forward_last_rt = o_rt_lookup(dst, FALSE);
	if (o_ip_forward_last_rt) {
	    register int i;
	    
	    for (i = 0; i < o_ip_forward_last_rt->rt_n_gw; i++) {
		switch (sockaddrcmp2(o_ip_forward_last_rt->rt_routers[i], gw)) {
		case 0:
		    if (!isnext) {
			last_index = i;
			goto Return;
		    }
		    break;

		case 1:
		    if (isnext) {
			last_index = i;
			goto Return;
		    } else {
			goto Next;
		    }
		}
	    }
	}

    Next: ;
    } else {
	dst = (sockaddr_un *) 0;
    }

    o_ip_forward_last_rt = o_rt_lookup(dst, TRUE);
    last_index = 0;

 Return:
    *router_index = last_index;
    return o_ip_forward_last_rt;
}


static int
o_ip_forward __PF3(oi, OI,
		   v, register struct type_SNMP_VarBind *,
		   offset, int)
{
    int router_index;
    register int i;
    register unsigned int *ip, *jp;
    register rt_entry *rt;
    register OID oid = oi->oi_name;
    register OT	ot = oi->oi_type;
    OID	new;

    switch (offset) {
    case type_SNMP_SMUX__PDUs_get__request:
	if (oid->oid_nelem != ot->ot_name->oid_nelem + NDX_SIZE) {
	    return int_SNMP_error__status_noSuchName;
	}
	
	if (!(rt = o_ip_forward_lookup(oid->oid_elements + oid->oid_nelem - NDX_SIZE,
				       (int) NDX_SIZE,
				       FALSE,
				       &router_index))) {
	    return int_SNMP_error__status_noSuchName;
	}
	break;

    case type_SNMP_SMUX__PDUs_get__next__request:
	/* next request with incomplete instance? */
	if ((i = oid->oid_nelem - ot->ot_name->oid_nelem) &&
	    i < NDX_SIZE) {
	    for (jp = (ip = oid->oid_elements + ot->ot_name->oid_nelem - 1) + i;
		 jp > ip;
		 jp--) {
		if (*jp != 0) {
		    break;
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
	}

	/* next request with no instance? */
	if (oid->oid_nelem == ot->ot_name->oid_nelem) {
	    if (!(rt = o_ip_forward_lookup(oid->oid_elements + oid->oid_nelem,
					   0,
					   TRUE,
					   &router_index))) {
		return NOTOK;
	    }

	    if ((new = oid_extend(oid, NDX_SIZE)) == NULLOID) {
		return int_SNMP_error__status_genErr;
	    }

	    ip = new->oid_elements + new->oid_nelem - NDX_SIZE;
	    STR_OID(ip, &sock2ip(rt->rt_dest), sizeof (sock2ip(rt->rt_dest)));
	    INT_OID(ip, o_ip_forward_proto(rt));
	    INT_OID(ip, 0);	/* No support for TOS */
	    if (BIT_TEST(rt->rt_state, RTS_GATEWAY)) {
		STR_OID(ip, &sock2ip(rt->rt_routers[router_index]), sizeof (sock2ip(rt->rt_routers[router_index])));
	    } else {
		STR_OID(ip, &sock2ip(inet_addr_default), sizeof (sock2ip(inet_addr_default)));
	    }
		
	    if (v->name) {
		free_SNMP_ObjectName(v->name);
	    }
	    v->name = new;
	} else {
	    int j;

	    if (!(rt = o_ip_forward_lookup(ip = oid->oid_elements + ot->ot_name->oid_nelem,
					   (u_int) (j = oid->oid_nelem - ot->ot_name->oid_nelem),
					   TRUE,
					   &router_index))) {
		return NOTOK;
	    }

	    if ((i = j - NDX_SIZE) < 0) {
		if ((new = oid_extend(oid, -i)) == NULLOID)
		    return int_SNMP_error__status_genErr;
		if (v->name) {
		    free_SNMP_ObjectName(v->name);
		}
		v->name = oid = new;
	    } else if (i > 0) {
		oid->oid_nelem -= i;
	    }
		
	    ip = oid->oid_elements + ot->ot_name->oid_nelem;
	    STR_OID(ip, &sock2ip(rt->rt_dest), sizeof (sock2ip(rt->rt_dest)));
	    INT_OID(ip, o_ip_forward_proto(rt));
	    INT_OID(ip, 0);	/* No support for TOS */
	    if (rt->rt_routers[router_index]
		&& !BIT_TEST(rt->rt_state, RTS_REJECT)) {
		STR_OID(ip, &sock2ip(rt->rt_routers[router_index]),
			sizeof (sock2ip(rt->rt_routers[router_index])));
	    } else {
		STR_OID(ip, &sock2ip(inet_addr_default),
			sizeof (sock2ip(inet_addr_default)));
	    }
	}
	break;

    default:
	return int_SNMP_error__status_genErr;
    }

    switch (ot2object(ot)->ot_info) {
	int value;
	
    case IipForwardDest:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(rt->rt_dest,
				  (int *) 0));

    case IipForwardMask:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix(rt->rt_dest_mask,
				  (int *) 0));
	
    case IipForwardPolicy:
	/* No TOS support */
	return o_integer(oi, v, 0);

    case IipForwardNextHop:
	return o_ipaddr(oi,
			v,
			(caddr_t) sock2unix((rt->rt_routers[router_index]
				&& !BIT_TEST(rt->rt_state, RTS_REJECT))?
				  	rt->rt_routers[router_index] :
					inet_addr_default,
				(int *) 0));

    case IipForwardIfIndex:
	if (BIT_TEST(rt->rt_state, RTS_REJECT)
	    || !rt->rt_ifaps[router_index]) {
	    value = 0;
	} else {
	    value = rt->rt_ifaps[router_index]->ifa_link->ifl_index;
	}
	return o_integer(oi, v, value);

    case IipForwardType:
	if (BIT_TEST(rt->rt_state, RTS_REJECT)) {
	    value = FTYPE_OTHER;
	} else if (BIT_TEST(rt->rt_state, RTS_GATEWAY)) {
	    value = FTYPE_REMOTE;
	} else {
	    value = FTYPE_LOCAL;
	}
	return o_integer(oi, v, value);


    case IipForwardProto:
	return o_integer(oi, v, o_ip_forward_proto(rt));

    case IipForwardAge:
	return o_integer(oi, v, rt_age(rt));
	
    case IipForwardInfo:
	/* XXX - Need to ask the protocols */
	return o_specific(oi, v, (caddr_t) snmp_nullSpecific);

    case IipForwardNextHopAS:
	return o_integer(oi, v, rt->rt_gwp->gw_peer_as);

    case IipForwardMetric1:
	return o_integer(oi, v, rt->rt_metric);

    case IipForwardMetric2:
	switch (rt->rt_gwp->gw_proto) {
#ifdef	PROTO_OSPF
	case RTPROTO_OSPF_ASE:
	    if (ORT_ETYPE(rt)) {
		value = ORT_COST(rt);
		break;
	    }
	    /* Fall through */
#endif	/* PROTO_OSPF */
	    
	default:
	    value = METRIC_NONE;
	}
	return o_integer(oi, v, value);
	
    case IipForwardMetric3:
    case IipForwardMetric4:
    case IipForwardMetric5:
	return o_integer(oi, v, METRIC_NONE);

    }
	
    return int_SNMP_error__status_noSuchName;
}


/**/

void
rt_mib_free_rt __PF1(rt, rt_entry *)
{
    if (o_ip_route_last_rt == rt) {
	o_ip_route_last_rt = (rt_entry *) 0;
	snmp_last_free(&o_ip_route_last);
    }
    if (o_ip_forward_last_rt == rt) {
	o_ip_forward_last_rt = (rt_entry *) 0;
	snmp_last_free(&o_ip_forward_last);
    }
}


void
rt_init_mib __PF1(enabled, int)
{
    if (enabled) {
	snmp_tree_register(&rt_iproute_tree);
	snmp_tree_register(&rt_ipforward_tree);
    } else {
	snmp_tree_unregister(&rt_iproute_tree);
	snmp_tree_unregister(&rt_ipforward_tree);
    }
}
