#ident	"@(#)krt_rt_sock.c	1.4"
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


#define	INCLUDE_ROUTE
#define	INCLUDE_IF
#ifdef	SCO_GEMINI
#define	INCLUDE_IOCTL
#endif	/* SCO_GEMINI */
#include "include.h"
#ifdef	PROTO_INET
#include "inet.h"
#endif	/* PROTO_INET */
#ifdef	PROTO_ISO
#include "iso.h"
#endif	/* PROTO_ISO */
#include "krt.h"
#include "krt_var.h"


const bits rtm_type_bits[] =
{
    {RTM_ADD, "ADD"},
    {RTM_DELETE, "DELETE"},
    {RTM_CHANGE, "CHANGE"},
    {RTM_GET, "GET"},
    {RTM_LOSING, "LOSING"},
    {RTM_REDIRECT, "REDIRECT"},
    {RTM_MISS, "MISS"},
    {RTM_LOCK, "LOCK"},
    {RTM_OLDADD, "OLDADD"},
    {RTM_OLDDEL, "OLDDEL"},
    {RTM_RESOLVE, "RESOLVE"},
#ifdef	RTM_NEWADDR
    {RTM_NEWADDR,	"NEWADDR"},
#endif	/* RTM_NEWADDR */
#ifdef	RTM_DELADDR
    {RTM_DELADDR,	"DELADDR"},
#endif	/* RTM_DELADDR */
#ifdef	RTM_IFINFO
    {RTM_IFINFO,	"IFINFO"},
#endif	/* RTM_IFINFO */
    {0, NULL}
};

static const bits rtm_lock_bits[] =
{
    {RTV_MTU, "MTU"},
    {RTV_HOPCOUNT, "HOPCOUNT"},
    {RTV_EXPIRE, "EXPIRE"},
    {RTV_RPIPE, "RPIPE"},
    {RTV_SPIPE, "SPIPE"},
    {RTV_SSTHRESH, "SSTHRESH"},
    {RTV_RTT, "RTT"},
    {RTV_RTTVAR, "RTTVAR"},
    {0, NULL}
};

static const bits rtm_sock_bits[] =
{
    {RTA_DST,		"Dest"},
    {RTA_GATEWAY,	"Gateway"},
    {RTA_NETMASK,	"NetMask"},
    {RTA_GENMASK,	"GenMask"},
    {RTA_IFP,		"IFP"},
    {RTA_IFA,		"IFA"},
    {RTA_AUTHOR,	"Author"},
#ifdef	RTA_BRD
    {RTA_BRD,		"Broadcast"},
#endif	/* RTA_BRD */
    {0, NULL}
};


#ifdef	KRT_IFREAD_KINFO
static const bits krt_if_flag_bits[] =
{
    {IFF_UP,		"UP" },
    {IFF_BROADCAST,	"BROADCAST" },
    {IFF_DEBUG,		"DEBUG" },
    {IFF_LOOPBACK,	"LOOPBACK" },
    {IFF_POINTOPOINT,	"POINTOPOINT" },
#ifdef	IFF_NOTRAILERS
    {IFF_NOTRAILERS,	"NOTRAILERS" },
#endif	/* IFF_NOTRAILERS */
    {IFF_RUNNING,	"RUNNING" },
#ifdef	IFF_NOARP
    {IFF_NOARP,		"NOARP" },
#endif	/* IFF_NOARP */
#ifndef SCO_GEMINI
    {IFF_PROMISC,	"PROMISC" },
#endif /*SCO_GEMINI*/
    {IFF_ALLMULTI,	"ALLMULTI" },
#ifdef IFF_OACTIVE
    {IFF_OACTIVE,	"OACTIVE" },
#endif  /* IFF_OACTIVE */
#ifdef IFF_SIMPLEX
    {IFF_SIMPLEX,	"SIMPLEX" },
#endif  /* IFF_SIMPLEX */
#ifdef	IFF_LINK0
    {IFF_LINK0,		"LINK0" },
    {IFF_LINK1,		"LINK1" },
    {IFF_LINK2,		"LINK2" },
#endif	/* IFF_LINK0 */
    {0, NULL}
};
#endif	/* KRT_IFREAD_KINFO */

static const flag_t krt_trace_masks[] = {
    TR_ALL,			/* 0 - Invalid */
    TR_KRT_PACKET_ROUTE,	/* 1 - RTM_ADD */
    TR_KRT_PACKET_ROUTE,	/* 2 - RTM_DELETE */
    TR_KRT_PACKET_ROUTE,	/* 3 - RTM_CHANGE */
    TR_KRT_PACKET_OTHER,	/* 4 - RTM_GET */
    TR_KRT_PACKET_OTHER,	/* 5 - RTM_LOSING */
    TR_KRT_PACKET_REDIRECT,	/* 6 - RTM_REDIRECT */
    TR_KRT_PACKET_OTHER,	/* 7 - RTM_MISS */
    TR_KRT_PACKET_OTHER,	/* 8 - RTM_LOCK */
    TR_KRT_PACKET_ROUTE,	/* 9 - RTM_OLDADD */
    TR_KRT_PACKET_ROUTE,	/* 10 - RTM_OLDDEL */
    TR_KRT_PACKET_OTHER,	/* 11 - RTM_RESOLVE */
    TR_KRT_PACKET_INTERFACE,	/* 12 - RTM_NEWADDR */
    TR_KRT_PACKET_INTERFACE,	/* 13 - RTM_DELADDR */
    TR_KRT_PACKET_INTERFACE,	/* 14 - RTM_IFINFO */
} ;    


flag_t krt_rt_support = KRTS_HOST
#ifdef	VARIABLE_MASKS
	| KRTS_VAR_MASK
#endif	/* VARIABLE_MASKS */
	;

/*
 * Pickup all the address from the message
 */
krt_addrinfo *
krt_xaddrs __PF2(rtp, register struct rt_msghdr *,
		 len, size_t)
{
    register int i;
    register int family = AF_UNSPEC;
    register struct sockaddr *ap;
    caddr_t limit = (caddr_t) rtp + len;
    static krt_addrinfo addrinfo;

    if (rtp->rtm_msglen != len) {
	trace_log_tp(krt_task,
		     0,
		     LOG_ERR,
		     ("krt_xaddrs: length mismatch!  Reported: %d, actual %d",
		      rtp->rtm_msglen,
		      len));
	return (krt_addrinfo *) 0;
    }
    
    if (rtp->rtm_version != RTM_VERSION) {
	trace_log_tp(krt_task,
		     0,
		     LOG_ERR,
		     ("krt_xaddrs: version mismatch!  Expected %d, received %d",
		      RTM_VERSION,
		      rtp->rtm_version));
	return (krt_addrinfo *) 0;
    }

    /* Locate address bits and start of addresses */
    switch (rtp->rtm_type) {
    case RTM_ADD:
    case RTM_DELETE:
    case RTM_CHANGE:
    case RTM_GET:
    case RTM_LOSING:
    case RTM_REDIRECT:
    case RTM_MISS:
    case RTM_LOCK:
    case RTM_OLDADD:
    case RTM_OLDDEL:
    case RTM_RESOLVE:
	ap = (struct sockaddr *) (rtp + 1);
	addrinfo.rti_addrs = rtp->rtm_addrs;
	break;

#ifdef	KRT_IFREAD_KINFO
    case RTM_IFINFO:
	ap = (struct sockaddr *) (((struct if_msghdr *) rtp) + 1);
	addrinfo.rti_addrs = ((struct if_msghdr *) rtp)->ifm_addrs;
	break;

    case RTM_NEWADDR:
    case RTM_DELADDR:
	ap = (struct sockaddr *) (((struct ifa_msghdr *) rtp) + 1);
	addrinfo.rti_addrs = ((struct ifa_msghdr *) rtp)->ifam_addrs;
	break;
#endif	/* KRT_IFREAD_KINFO */

    default:
	return (krt_addrinfo *) 0;
    }


    /* Grab all the addresses */
    RTAX_LIST(i) {
	if (BIT_TEST(addrinfo.rti_addrs, 1 << i)) {
	    if ((caddr_t) ap >= limit) {
		trace_log_tp(krt_task,
			     0,
			     LOG_ERR,
			     ("krt_xaddrs: out of data fetching %s address",
			      trace_state(rtm_sock_bits, i)));
		return (krt_addrinfo *) 0;
	    }
	    
	    switch (i) {
	    case RTAX_DST:
	    case RTAX_IFA:
		family = ap->sa_family;
		/* Fall through */
		
	    case RTAX_GATEWAY:
	    case RTAX_IFP:
	    case RTAX_AUTHOR:
	    case RTAX_BRD:
		if (unix_socksize(ap)) {
		    addrinfo.rti_info[i] = sock2gated(ap, (size_t) unix_socksize(ap));
		}
		break;

	    case RTAX_NETMASK:
	    case RTAX_GENMASK:
		addrinfo.rti_info[i] = (sockaddr_un *) ap;
		break;
	    }

	    RTM_ADDR(ap);
	} else {
	    addrinfo.rti_info[i] = (sockaddr_un *) 0;
	}
    } RTAX_LIST_END(i) ;

    /* Now that we have the family, convert the masks */
    for (i = RTAX_NETMASK; i <= RTAX_GENMASK; i++) {
	if ((ap = (struct sockaddr *) addrinfo.rti_info[i])) {
#ifdef SOCKET_LENGTHS
	    if (ap->sa_len < 2) {
		/* Zero mask, fudge it so we can translate it */

		*((u_long *) ap) = 0;
		ap->sa_len = 2;	/* Minimum length */
	    }
#endif

	    /* Set the family and translate */
	    ap->sa_family = family;
	    switch (family) {
#ifdef	PROTO_INET
	    case AF_INET:
	        {
		    struct sockaddr_in *sinp = (struct sockaddr_in *) ap;
#ifdef SOCKET_LENGTHS
		    if (sinp->sin_len > sizeof *sinp) {
			sinp->sin_len = sizeof *sinp;
		    }
#endif
		    sinp->sin_port = 0;
		}
		break;
#endif	/* PROTO_INET */

		default:
		    break;
	    }
	    addrinfo.rti_info[i] = sock2gated(ap, (size_t) unix_socksize(ap));
	}
    }

    return &addrinfo;
}


/*
 * Common logic for controling routes
 */
int
krt_rtaddrs __PF4(adip, krt_addrinfo *,
		  rtp, rt_parms *,
		  author, sockaddr_un **,
		  flags, flag_t)    
{
    int rc = KRT_ADDR_OK;
    
    rtp->rtp_state = krt_flags_to_state(flags);

    rtp->rtp_dest = adip->rti_info[RTAX_DST];
    if (!rtp->rtp_dest) {
	return KRT_ADDR_IGNORE;
    }

    switch (socktype(rtp->rtp_dest)) {
#ifdef	PROTO_INET
    case AF_INET:
	if (BIT_TEST(flags, RTF_LLINFO)
#ifdef RTF_CLONING
	    || BIT_MATCH(flags, RTF_CLONING|RTF_GATEWAY)) {
#else
	    || BIT_MATCH(flags, RTF_GATEWAY)) {
#endif
	    /* Skip ARP cache entries and cloning masks that are not interface routes */
	    return KRT_ADDR_IGNORE;
	}
	break;
#endif	/* PROTO_INET */
#ifdef	PROTO_ISO
    case AF_ISO:
        if (BIT_TEST(flags, RTF_DYNAMIC)) {
	    /* Delete redirects */
            rc = KRT_ADDR_BOGUS;
	    break;
        }
        if (BIT_TEST(flags, RTF_LLINFO|RTF_CLONING)) {
	    /* Skip ES-IS routes and cloning masks */
            return KRT_ADDR_IGNORE;
        }
        break;
#endif	/* PROTO_ISO */

    default:
	break;
    }

    *author = adip->rti_info[RTAX_AUTHOR];

    rtp->rtp_router = adip->rti_info[RTAX_GATEWAY];
    if (rtp->rtp_router) {
	/*
	 * Kernel gateway is possibly an AF_LINK.  If so, look up the interface
	 * with the destination address and figure out the gateway from there.
	 * When installing the interface route the kernel is supposed to put what
	 * it wants so we can just keep it as the gateway.
	 */
	switch (socktype(rtp->rtp_router)) {
	    register if_addr *ifap;

	case AF_LINK:
	    if (BIT_TEST(flags, RTF_GATEWAY)) {
		/* Not an interface route */

		return KRT_ADDR_IGNORE;
	    }

	    /* Later versions of the BSD code will return interface */
	    /* information.  RTAX_BRD indicates the destination address of */
	    /* a P2P interface, RTAX_IFA indicates the local address */
	    /* of other interface types. */

	    if (adip->rti_info[RTAX_BRD]) {
		ifap = if_withdstaddr(adip->rti_info[RTAX_BRD]);
	    } else if (adip->rti_info[RTAX_IFA]) {
		ifap = if_withlcladdr(adip->rti_info[RTAX_IFA], FALSE);
	    } else {
		ifap = (if_addr *) 0;
	    }

	    if (!ifap) {
		ifap = if_withsubnet(rtp->rtp_dest);
	    }

	    if (ifap) {
		if (ifap->ifa_link->ifl_index == rtp->rtp_router->dl.gdl_index) {
		    rtp->rtp_router = ifap->ifa_addr;
		}
	    } else {
		/* Bogus, delete it */

		rc = KRT_ADDR_BOGUS;
	    }
	    break;
		
	default:
	    break;
	}
    }

    rtp->rtp_dest_mask = adip->rti_info[RTAX_NETMASK];
    if (rtp->rtp_dest_mask) {
	/* Have a netmask */

	if (!mask_contig(rtp->rtp_dest_mask)) {
	    return KRT_ADDR_BOGUS;
	}
	
	rtp->rtp_dest_mask = mask_locate(rtp->rtp_dest_mask);
    } else {
	/* Figure out a mask */
	    
	switch (socktype(rtp->rtp_dest)) {
#ifdef	PROTO_INET
	case AF_INET:
	    rtp->rtp_dest_mask = BIT_TEST(flags, RTF_HOST) ? inet_mask_host : inet_mask_natural(rtp->rtp_dest);
	    break;
#endif	/* PROTO_INET */

#ifdef	PROTO_ISO
	case AF_ISO:
	    rtp->rtp_dest_mask = iso_mask_natural(rtp->rtp_dest);
	    break;
#endif	/* PROTO_ISO */

	default:
	    assert(TRUE);
	}
    }

    /* If we have a mask, mask off potential garbage in the address */
    if (rtp->rtp_dest_mask) {
	sockmask(rtp->rtp_dest, rtp->rtp_dest_mask);
    }
	
    /* Check to see if the address is valid */
    switch (krt_addrcheck(rtp)) {
    case KRT_ADDR_OK:
	/* Address is OK */
	break;

    case KRT_ADDR_IGNORE:
	/* Ignore it */
	return KRT_ADDR_IGNORE;

    case KRT_ADDR_BOGUS:
	/* Bogus, delete it */
	rc = KRT_ADDR_BOGUS;
	break;

#ifdef	IP_MULTICAST
    case KRT_ADDR_MC:
	/* Multicast group */
	rc = KRT_ADDR_MC;
	break;
#endif	/* IP_MULTICAST */
    }

    if (BIT_TEST(rtp->rtp_state, RTS_GATEWAY)) {
	BIT_SET(rtp->rtp_state, RTS_EXTERIOR);
    } else {
	BIT_SET(rtp->rtp_state, RTS_INTERIOR);
    }

    return rc;
}


/* Trace a routing socket packet */
/*ARGSUSED*/
void
krt_trace_msg __PF7(tp, task *,
		    direction, const char *,
		    rtp, struct rt_msghdr *,
		    len, size_t,
		    adip, krt_addrinfo *,
		    pri, int,
		    detail, int)
{
    if (!adip) {
	adip = krt_xaddrs(rtp, len);
	if (!adip) {
	    /* Could not parse message */
	    return;
	}
    }

    if (detail) {
	/* Long form */

	tracef("KRT %s len %d ver %d type %s(%d)",
	       direction,
	       rtp->rtm_msglen,
	       rtp->rtm_version,
	       trace_state(rtm_type_bits, rtp->rtm_type - 1),
	       rtp->rtm_type);
    } else {
	/* Short form */

	tracef("KRT %s type %s(%d)",
	       direction,
	       trace_state(rtm_type_bits, rtp->rtm_type - 1),
	       rtp->rtm_type);
    }

    switch (rtp->rtm_type) {
    case RTM_ADD:
    case RTM_DELETE:
    case RTM_CHANGE:
    case RTM_GET:
    case RTM_LOSING:
    case RTM_REDIRECT:
    case RTM_MISS:
    case RTM_LOCK:
    case RTM_OLDADD:
    case RTM_OLDDEL:
    case RTM_RESOLVE:
	if (detail) {
	    /* Long form */
	    tracef(" addrs <%s>(%x) pid %d seq %d",
		   trace_bits(rtm_sock_bits, (flag_t) rtp->rtm_addrs),
		   rtp->rtm_addrs,
		   rtp->rtm_pid,
		   rtp->rtm_seq);
	    if (rtp->rtm_errno) {
		errno = rtp->rtm_errno;
		trace_log_tp(krt_task,
			     0,
			     pri,
			     (" error %d: %m",
			      rtp->rtm_errno));
	    } else {
		trace_log_tp(krt_task,
			     0,
			     pri,
			     (NULL));
	    }

	    tracef("KRT %s flags %s(%x)",
		   direction,
		   trace_bits(krt_flag_bits, (flag_t) rtp->rtm_flags),
		   rtp->rtm_flags & 0xffff);
	    if (rtp->rtm_rmx.rmx_locks) {
		tracef("  locks %s(%x)",
		       trace_bits(rtm_lock_bits, rtp->rtm_rmx.rmx_locks),
		       rtp->rtm_rmx.rmx_locks);
	    }
	    if (rtp->rtm_inits) {
		tracef(" inits %s(%x)",
		       trace_bits(rtm_lock_bits, rtp->rtm_inits),
		       rtp->rtm_inits);
	    }
	    trace_log_tp(krt_task,
			 0,
			 pri,
			 (NULL));

	    /* Display metrics */
	    switch (rtp->rtm_type) {
	    case RTM_ADD:
	    case RTM_CHANGE:
	    case RTM_GET:
	    case RTM_LOCK:
		if (BIT_TEST(rtp->rtm_rmx.rmx_locks|rtp->rtm_inits, RTV_MTU|RTV_HOPCOUNT|RTV_EXPIRE|RTV_SSTHRESH)) {
		    trace_log_tp(krt_task,
				 0,
				 pri,
				 ("KRT %s mtu %d hopcount %d expire %#T ssthresh %d",
				  direction,
				  rtp->rtm_rmx.rmx_mtu,
				  rtp->rtm_rmx.rmx_hopcount,
				  rtp->rtm_rmx.rmx_expire,
				  rtp->rtm_rmx.rmx_ssthresh));
		}
		if (BIT_TEST(rtp->rtm_rmx.rmx_locks|rtp->rtm_inits, RTV_RPIPE|RTV_SPIPE|RTV_RTT|RTV_RTTVAR)) {
		    trace_log_tp(krt_task,
				 0,
				 pri,
				 ("KRT %s recvpipe %d sendpipe %d rtt %d rttvar %d",
				  direction,
				  rtp->rtm_rmx.rmx_recvpipe,
				  rtp->rtm_rmx.rmx_sendpipe,
				  rtp->rtm_rmx.rmx_rtt,
				  rtp->rtm_rmx.rmx_rttvar));
		}
		break;
	    }
	} else {
	    /* Short form */

	    tracef("flags %s(%x) error %d",
		   trace_bits(krt_flag_bits, (flag_t) rtp->rtm_flags),
		   rtp->rtm_flags & 0xffff,
		   rtp->rtm_errno);
	    if (rtp->rtm_errno) {
		errno = rtp->rtm_errno;
		trace_log_tp(krt_task,
			     0,
			     pri,
			     (": %m"));
	    } else {
		trace_log_tp(krt_task,
			     0,
			     pri,
			     (NULL));
	    }
	}
	break;

#ifdef	KRT_IFREAD_KINFO
    case RTM_IFINFO:
        {
	    struct if_msghdr *ifap = (struct if_msghdr *) rtp;
	    
	    trace_log_tp(krt_task,
			 0,
			 pri,
			 (" index %d flags <%s>%x mtu %d metric %d",
			  ifap->ifm_index,
			  trace_bits(krt_if_flag_bits, (flag_t) ifap->ifm_flags),
			  ifap->ifm_flags,
#ifdef	SCO_GEMINI
			  0, ((struct ifa_msghdr *) ifap)->ifam_metric));
#else	/*SCO_GEMINI*/
			  ifap->ifm_data.ifi_mtu,
			  ifap->ifm_data.ifi_metric));
#endif	/*SCO_GEMINI*/
	}
	break;

    case RTM_NEWADDR:
    case RTM_DELADDR:
        {
	    struct ifa_msghdr *ifap = (struct ifa_msghdr *) rtp;
	    
	    trace_log_tp(krt_task,
			 0,
			 pri,
			 (" index %d flags <%s>%x metric %d",
			  ifap->ifam_index,
			  trace_bits(krt_flag_bits, (flag_t) ifap->ifam_flags),
			  ifap->ifam_flags,
			  ifap->ifam_metric));
	}
	break;
#endif	/* KRT_IFREAD_KINFO */

    default:
	return;
    }
    
    /* Display addresses */
    if (adip->rti_addrs) {
	register int i;

	tracef("KRT %s", direction);

	RTAX_LIST(i) {
	    register sockaddr_un *ap = adip->rti_info[i];

	    if (ap) {
		tracef(" %s %A",
		       gd_lower(trace_state(rtm_sock_bits, i)),
		       ap);
	    }
	} RTAX_LIST_END(i);

	trace_log_tp(krt_task,
		     0,
		     pri,
		     (NULL));
    }
}


/* Read interfaces from kernel */

#ifdef	KRT_IFREAD_KINFO
void
krt_ifaddr __PF4(tp, task *,
		 ifap, struct ifa_msghdr *,
		 adip, krt_addrinfo *,
		 ifl, if_link *)
{
    sockaddr_un *ap;
    if_info ifi;
    int addrs;
    int p2p = FALSE;

    if (!ifl) {
	/* No link level interface association */
    }

    bzero((caddr_t) &ifi, sizeof (ifi));

    ifi.ifi_link = ifl;
    ifi.ifi_rtflags = ifap->ifam_flags;
    ifi.ifi_state = ifl->ifl_state;
    /* If we have a metric, use it, otherwise get it from the cached link level info */
    ifi.ifi_metric = ifap->ifam_metric ? ifap->ifam_metric : ifl->ifl_metric;
    /* Pickup the cached MTU info */
    ifi.ifi_mtu = ifl->ifl_mtu;

    switch (BIT_TEST(ifl->ifl_state, IFS_POINTOPOINT|IFS_LOOPBACK|IFS_BROADCAST)) {
    case IFS_POINTOPOINT:
	addrs = 2;
	p2p = TRUE;
	break;

    case IFS_LOOPBACK:
	addrs = 1;
	break;

    case IFS_BROADCAST:
	addrs = 3;
	break;

    default:
	/* NBMA */
	addrs = 2;
	break;
    }
    
    if ((ap = adip->rti_info[RTAX_IFA])) {
	/* Interace address */

	ifi.ifi_addr_local = sockdup(ap);
	if (!p2p) {
	    ifi.ifi_addr = ifi.ifi_addr_local;
	}
	addrs--;
    }
    if ((ap = adip->rti_info[RTAX_BRD])) {
	/* Remote or broadcast address */

	if (p2p) {
	    ifi.ifi_addr = sockdup(ap);
	} else {
	    ifi.ifi_addr_broadcast = sockdup(ap);
	}
	addrs--;
    }
    if ((ap = adip->rti_info[RTAX_NETMASK])) {
	/* Subnet mask */

	ifi.ifi_netmask = mask_locate(ap);
	addrs--;
    }

    if (addrs <= 0) {
	/* We have enough addresses */

	if (ifap->ifam_type == RTM_NEWADDR) {
	    if_conf_addaddr(tp, &ifi);
	} else {
	    if_conf_deladdr(tp, &ifi);
	}
    } else {
	/* Clean up addresses */

	ifi_addr_free(&ifi);
    }
}
#endif	/* KRT_IFREAD_KINFO */


/**/
/* Support for changing the routing table */

static u_int rtm_seq;					/* Sequence number */

/* Issue a request */
static int
krt_action __PF2(tp, task *,
		 rtp, struct rt_msghdr *)
{
    int pri = 0;
    const char *sent = "SENT";

    if (!rtp->rtm_seq) {
	rtp->rtm_seq = ++rtm_seq;
    }
    rtp->rtm_pid = task_pid;
    rtp->rtm_errno = 0;

    if (!BIT_TEST(task_state, TASKS_TEST)
	&& !BIT_TEST(krt_options, KRT_OPT_NOINSTALL)) {
	errno = 0;
#ifdef	SCO_OSR5
	{
	/* write does not work since it is not exactly a routing socket */
	    struct strioctl sioc;
	    sioc.ic_cmd = RTSTR_SEND;
	    sioc.ic_timout = 0;
	    sioc.ic_len = rtp->rtm_msglen;
	    sioc.ic_dp = (char *) rtp;

	    if (ioctl(tp->task_socket, I_STR, &sioc) < 0) {
		rtp->rtm_errno = errno;
		sent = "NOT SENT";
		pri = LOG_ERR;
	    }
	}
#else	/* SCO_OSR5 */
	if (write(tp->task_socket,
		  (caddr_t) rtp,
		  (size_t) rtp->rtm_msglen) < 0) {
	    rtp->rtm_errno = errno;
	    pri = LOG_ERR;
	}
#endif	/* SCO_OSR5 */
    }

    if (pri
	|| TRACE_PACKET_SEND_TP(tp,
				rtp->rtm_type,
				RTM_MAX,
				krt_trace_masks)) {
	krt_trace_msg(tp,
		      sent,
		      rtp,
		      (size_t) rtp->rtm_msglen,
		      (krt_addrinfo *) 0,
		      pri,
		      TRACE_DETAIL_SEND_TP(tp,
					   rtp->rtm_type,
					   RTM_MAX,
					   krt_trace_masks));
	trace_only_tp(tp,
		      0,
		      (NULL));
    }
    
    return rtp->rtm_errno;
}


/* Fill in a request and enqueue it */
static int
krt_request __PF6(tp, task *,
		  type, u_int,
		  dest, sockaddr_un *,
		  mask, sockaddr_un *,
		  krtp, krt_parms *,
		  router, sockaddr_un *)
{
    int rc = 0;
    struct sockaddr *ap;
    struct sockaddr *s_dest = sock2unix(dest, (int *) 0);
    struct sockaddr *s_mask = (struct sockaddr *) 0;
    struct sockaddr *s_router = sock2unix(router, (int *) 0);
    static union {
	struct rt_msghdr msg;
	u_char buf[sizeof (struct rt_msghdr) + 8 * 32];
    } u;
    struct rt_msghdr *rtp = &u.msg;
    flag_t flags = 0;
    size_t size = sizeof *rtp + unix_socksize(s_dest);

    if (TRACE_TP(tp, TR_KRT_REQUEST)) {
	tracef("KERNEL %-6s %-15A",
	       trace_state(rtm_type_bits, type - 1),
	       dest);

	if (mask) {
	    tracef(" mask %-15A",
		   mask);
	}

	tracef(" gateway %-15A %s <%s>",
	       router,
	       trace_state(rt_proto_bits, krtp->krtp_protocol),
	       trace_bits(rt_state_bits, krtp->krtp_state));
    }

    if (BIT_TEST(krt_options, KRT_OPT_NOINSTALL)) {
	trace_tp(tp,
		 TR_KRT_REQUEST,
		 0,
		 (NULL));

	return rc;
    }
    
    if (sockishost(dest, mask)) {
	BIT_SET(flags, RTF_HOST);
    } else if (mask) {
	s_mask = sock2unix(mask, (int *) 0);
	size += unix_socksize(s_mask);
    }
	
    switch (type) {
    case RTM_ADD:
    case RTM_CHANGE:
#ifdef	KRT_RT_SOCK_SUNOS5
    case RTM_DELETE:
#endif	/* KRT_RT_SOCK_SUNOS5 */
	size += unix_socksize(s_router);
	break;

    default:
	s_router = (struct sockaddr *) 0;
    }

    /* Allocate a block and clear it */
    assert(size < sizeof u);

    rtp->rtm_addrs = 0;
    rtp->rtm_type = type;
    rtp->rtm_version = RTM_VERSION;
    rtp->rtm_flags = flags | krt_state_to_flags(krtp->krtp_state);
    if (krtp->krtp_n_gw
	&& krtp->krtp_ifaps
	&& krtp->krtp_ifap
	&& BIT_TEST(krtp->krtp_ifap->ifa_state, IFS_UP)) {
	BIT_SET(rtp->rtm_flags, RTF_UP);
    }
#ifdef	KRT_IFREAD_KINFO
    if (krtp->krtp_protocol == RTPROTO_DIRECT
	&& krtp->krtp_n_gw
	&& krtp->krtp_ifaps
	&& krtp->krtp_ifap
	&& !BIT_TEST(krtp->krtp_state, RTS_REJECT)) {
	/* Set interface specific flags */
	rtp->rtm_flags |= krtp->krtp_ifap->ifa_info.ifi_rtflags;
    }
#endif	/* KRT_IFREAD_KINFO */
#ifdef	RTF_DYNAMIC
    if (krtp->krtp_protocol == RTPROTO_REDIRECT) {
	BIT_SET(rtp->rtm_flags, RTF_DYNAMIC);
    }
#endif	/* RTF_DYNAMIC */

    /* XXX - set metrics */

    ap = (struct sockaddr *) (rtp + 1);

    bcopy((caddr_t) s_dest, (caddr_t) ap, (size_t) unix_socksize(s_dest));
    RTM_ADDR(ap);
    BIT_SET(rtp->rtm_addrs, RTA_DST);

    if (s_router) {
	bcopy((caddr_t) s_router, (caddr_t) ap, (size_t) unix_socksize(s_router));
	RTM_ADDR(ap);
	BIT_SET(rtp->rtm_addrs, RTA_GATEWAY);
    }

    /* Provide a mask if this is not a host route */
    if (s_mask) {
	/* Convert our netmask format into the kernel's netmask format. */
	/* The kernel does not want the address family nor trailing zeros. */
	register byte *sp = (byte *) s_mask;
	register byte *lp = (byte *) s_mask + unix_socksize(s_mask);
	register byte *dp = (byte *) ap;
	register byte *cp = (byte *) 0;

#ifdef SOCKET_LENGTHS
	s_mask->sa_len = s_mask->sa_family = 0;	/* OK to write here, it's a scratch buffer */
#else
	s_mask->sa_family = 0;	/* OK to write here, it's a scratch buffer */
#endif

	/* Copy mask and keep track of last non-zero byte */
	while (sp < lp) {
	    if ((*dp++ = *sp++)) {
		/* We actually point to the first byte after last zero byte */
		cp = dp;
	    }
	}

	if (cp <= (byte *) &ap->sa_family) {
	    /* If the netmask is zero length, make sure there is at least a */
	    /* long word of zeros present */
	    *((u_long *) ap) = 0;
	} else {
#ifdef SOCKET_LENGTHS
	    ap->sa_len = cp - (byte *) ap;
#endif
	}
		
	RTM_ADDR(ap);
	BIT_SET(rtp->rtm_addrs, RTA_NETMASK);
    }

    rtp->rtm_msglen = (caddr_t) ap - (caddr_t) rtp;

    /* If this is the first entry on the queue, run the queue */
    trace_tp(tp,
	     TR_KRT_REQUEST,
	     0,
	     (NULL));
    return krt_action(krt_task, rtp);
}


int
krt_change_start __PF1(tp, task *)
{
    return KRT_OP_SUCCESS;
}


int
krt_change_end __PF1(tp, task *)
{
    return KRT_OP_SUCCESS;
}


int
krt_change __PF5(tp, task *,
		 dest, sockaddr_un *,
		 mask, sockaddr_un *,
		 old_krtp, krt_parms *,
		 new_krtp, krt_parms *)
{
    int rc = KRT_OP_SUCCESS;
    sockaddr_un *new_router = (sockaddr_un *) 0;
    sockaddr_un *old_router = (sockaddr_un *) 0;

    if (new_krtp) {
	if (!new_krtp->krtp_n_gw) {
	    new_router = krt_make_router(socktype(dest), new_krtp->krtp_state);
	    assert(new_router);
	} else {
	    new_router = new_krtp->krtp_router;
	}
    }

    if (old_krtp) {
	if (!old_krtp->krtp_n_gw) {
	    old_router = krt_make_router(socktype(dest), old_krtp->krtp_state);
	    assert(old_router);
	} else {
	    old_router = old_krtp->krtp_router;
	}
    }
    
    if (old_krtp && new_krtp
      && old_krtp->krtp_state == new_krtp->krtp_state
      && (!old_krtp->krtp_n_gw || old_krtp->krtp_ifap)) {
	/* Routes are potentially the same, or we are allowed to do a change */

	if (BIT_TEST(new_krtp->krtp_state, RTS_REJECT|RTS_BLACKHOLE)
		|| (sockaddrcmp(old_router, new_router)
		    && new_krtp->krtp_ifap == old_krtp->krtp_ifap)) {
	    /* If nothing has changed, there isn't anything to do */

	    return rc;
	}
#ifndef	KRT_RT_NOCHANGE
	if (socksize(new_router) <= socksize(old_router)) {
	    /* Try a change */

	    switch (krt_request(tp, RTM_CHANGE, dest, mask, new_krtp, new_router)) {
	    case ENOBUFS:
		/* Out of resources, wait until later */
		return KRT_OP_FULL;

	    case ESRCH:
		/* Try the Delete/Add method */
		break;

	    case ENETUNREACH:
		/* Interface down?  Wait a while and it might get fixed */
		return KRT_OP_DEFER;

	    case EEXIST:
	    case EDQUOT:
	    case ETOOMANYREFS:
	    default:
		krt_trace(tp,
			  "SEND",
			  "CHANGE",
			  dest,
			  mask,
			  new_router,
			  (flag_t) krt_state_to_flags(new_krtp->krtp_state),
			  strerror(errno),
			  LOG_CRIT);
		/* Try to do a delete/add */
		break;

	    case 0:
		return rc;
	    }
	}
#endif	/* KRT_RT_NOCHANGE */
    }

    if (old_krtp) {
	
	switch (krt_request(tp, RTM_DELETE, dest, mask, old_krtp, old_router)) {
	case ENOBUFS:
	    /* Not enough resources to perform the action */
	    return KRT_OP_FULL;

	case ESRCH:
	    /* Route not found.  Not a problem since we were trying to delete it anyway */
	case ENETUNREACH:
	case EEXIST:
	case ETOOMANYREFS:
	case EDQUOT:
	default:
	    /* Should not happen */
	    krt_trace(tp,
		      "SEND",
		      "DELETE",
		      dest,
		      mask,
		      old_router,
		      (flag_t) krt_state_to_flags(old_krtp->krtp_state),
		      strerror(errno),
		      LOG_CRIT);
	    break;

	case 0:
	    krt_n_routes--;
	    break;
	}
    }
    
    if (new_krtp) {
	int retry = 5;

	if (krt_n_routes > krt_limit_routes) {
	    /* Too many routes */

	    return KRT_OP_FULL;
	}
	
    retry_add:
	if (!--retry) {
	    /* Give up */
	    return KRT_OP_NOCANDO;
	}

	switch (krt_request(tp, RTM_ADD, dest, mask, new_krtp, new_router)) {
	case EEXIST:
	    /* Route already exists - delete and re-install */
	    
	    switch (krt_request(tp, RTM_DELETE, dest, mask, new_krtp, new_router)) {
	    case ENOBUFS:
		/* Not enough resources to perform the action */
		rc = KRT_OP_FULL;
		break;

	    default:
	    case EEXIST:
	    case ENETUNREACH:
	    case ESRCH:
	    case ETOOMANYREFS:
	    case EDQUOT:
		krt_trace(tp,
			  "SEND",
			  "DELETE",
			  dest,
			  mask,
			  new_router,
			  (flag_t) krt_state_to_flags(new_krtp->krtp_state),
			  strerror(errno),
			  LOG_CRIT);
		/* Fall through */

	    case 0:
		goto retry_add;
	    }
	    break;

	case ENOBUFS:
	    /* No resources */
	    rc = KRT_OP_FULL;
	    break;

	case ENETUNREACH:
	    /* Probably an interface down. */
	    /* If we defer this the higher levels will remove this from the queue */
	    rc = KRT_OP_DEFER;
	    break;

	case ESRCH:
	case ETOOMANYREFS:
	case EDQUOT:
	default:
	    krt_trace(tp,
		      "SEND",
		      "ADD",
		      dest,
		      mask,
		      new_router,
		      (flag_t) krt_state_to_flags(new_krtp->krtp_state),
		      strerror(errno),
		      LOG_CRIT);

	case 0:
	    krt_n_routes++;
	    break;
	}

	if (rc != KRT_OP_SUCCESS && old_krtp) {
	    rc |= KRT_OP_PARTIAL;
	}
    }

    return rc;
}



void
krt_delete_dst __PF8(tp, task *,
		     dest, sockaddr_un *,
		     mask, sockaddr_un *,
		     proto, proto_t,
		     state, flag_t,
		     n_gw, int,
		     routers, sockaddr_un **,
		     ifaps, if_addr **)
{
    krt_parms krtp;

    krtp.krtp_protocol = proto;
    krtp.krtp_state = state | RTS_GATEWAY;
    krtp.krtp_n_gw = n_gw;
    krtp.krtp_routers = routers;
    krtp.krtp_ifaps = ifaps;

    (void) krt_change(tp,
		      dest,
		      mask,
		      &krtp,
		      (krt_parms *) 0);
}


/**/

/* Process an information routing socket request */
static void
krt_recv_rtinfo __PF3(tp, task *,
		      rtp, struct rt_msghdr *,
		      adip, krt_addrinfo *)
{
    switch (rtp->rtm_type) {
    case RTM_LOSING:
	trace_tp(tp,
		 TR_KRT_INFO,
		 0,
		 ("krt_recv_rtinfo: kernel reports TCP lossage on route to %A/%A via %A",
		  adip->rti_info[RTAX_DST],
		  adip->rti_info[RTAX_NETMASK],
		  adip->rti_info[RTAX_GATEWAY]));
	break;

    case RTM_MISS:
	trace_tp(tp,
		 TR_KRT_INFO,
		 0,
		 ("krt_recv_rtinfo: kernel can not find route to %A/%A",
		  adip->rti_info[RTAX_DST],
		  adip->rti_info[RTAX_NETMASK]));
	break;
	
    case RTM_RESOLVE:
	trace_tp(tp,
		 TR_KRT_INFO,
		 0,
		 ("krt_recv_rtinfo: kernel resolution request for %A/%A",
		  adip->rti_info[RTAX_DST],
		  adip->rti_info[RTAX_NETMASK]));
	break;

    default:
	assert(FALSE);
    }
}


/* Process a route related routing socket request */
static void
krt_recv_route __PF3(tp, task *,
		     rtp, struct rt_msghdr *,
		     adip, krt_addrinfo *)
{
#ifdef	IP_MULTICAST
    int mc = FALSE;
#endif	/* IP_MULTICAST */
    int need_sync = FALSE;
    sockaddr_un *author = (sockaddr_un *) 0;
    rt_entry *rt;
    rt_parms rtparms;

    bzero((caddr_t) &rtparms, sizeof (rtparms));
    rtparms.rtp_n_gw = 1;

    switch (krt_rtaddrs(adip, &rtparms, &author, (flag_t) rtp->rtm_flags)) {
	const char *errmsg;
	int pri;

    case KRT_ADDR_OK:
	break;

    case KRT_ADDR_IGNORE:
	errmsg = "ignoring";
	pri = LOG_INFO;

    Trace:
	krt_trace(tp,
		  "RECV",
		  "MSG",
		   adip->rti_info[RTAX_DST],
		   adip->rti_info[RTAX_NETMASK],
		   adip->rti_info[RTAX_GATEWAY],
		  (flag_t) rtp->rtm_flags,
		  errmsg,
		  pri);
	return;

    case KRT_ADDR_BOGUS:
	errmsg = "ignoring bogus";
	pri = LOG_WARNING;
	goto Trace;	/* XXX - Delete? */

#ifdef	IP_MULTICAST
    case KRT_ADDR_MC:
	mc = TRUE;
	break;
#endif	/* IP_MULTICAST */
    }

    if (rtp->rtm_pid == task_pid) {
	trace_log_tp(tp,
		     0,
		     LOG_ERR,
		     ("krt_recv_route: received a response to my own request"));
	return;
    }
    
    switch (socktype(rtparms.rtp_dest)) {
#ifdef	PROTO_INET
    case AF_INET:
	    
	if (BIT_TEST(rtparms.rtp_state, RTS_GATEWAY)) {
	    rtparms.rtp_gwp = krt_gwp;
	    rtparms.rtp_preference = RTPREF_KERNEL;
	    BIT_SET(rtparms.rtp_state, RTS_RETAIN);
	} else {
	    rtparms.rtp_gwp = krt_gwp_remnant;
	    rtparms.rtp_preference = RTPREF_KERNEL_REMNANT;
	    BIT_SET(rtparms.rtp_state, RTS_NOADVISE);
	}
	break;
#endif	/* PROTO_INET */

#ifdef	PROTO_ISO
    case AF_ISO:
	BIT_SET(rtparms.rtp_state, RTS_RETAIN);
	rtparms.rtp_preference = RTPREF_KERNEL;
	rtparms.rtp_gwp = krt_gwp;
	break;
#endif	/* PROTO_ISO */
    }

#ifdef	IP_MULTICAST
    if (mc) {
	if (!rtp->rtm_errno) {
	    krt_multicast_change(rtp->rtm_type, &rtparms);
	}
	return;
    }
#endif	/* IP_MULTICAST */

    /* Rescan the interfaces to make sure we have the latest information */
    /* Even if we are getting interface status change messages, there are */
    /* cases where the kernel does not send them, or we could have missed one */

    /* krt_ifread(task_state); */
    /* Don't wan't to call krt_ifread until after we are done processing here. */
    /* The sockaddr buffer is common between them. Schedule it for after */
    /* this routine.   */
    krt_ifcheck();
    
    if (rtp->rtm_type ==  RTM_REDIRECT) {
	if (!rtp->rtm_errno) {
	    if (!BIT_TEST(rtp->rtm_flags, RTF_MODIFIED)) {
		/* Indicate that a route was added */
		
		krt_n_routes++;
	    }
	    redirect(rtparms.rtp_dest,
		     rtparms.rtp_dest_mask,
		     rtparms.rtp_router,
		     author);
	}

	return;
    }

    /* Lets assume we have something to change */
    rt_open(tp);
    
    switch (rtp->rtm_type) {
    case RTM_ADD:
    case RTM_OLDADD:
	switch (rtp->rtm_errno) {
	default:
	    /* We don't want anything to do with this */
	    goto Return;

	case ENOBUFS:
	    /* We can queue this */
	    break;

	case EEXIST:
	    /* See if we can add a kernel static */
	    break;
	    
	case 0:
	    /* Indicate a route was added */

	    krt_n_routes++;
	}

	/* Attempt to find an existing route */
	rt = rt_locate(RTS_NETROUTE,
		       rtparms.rtp_dest,
		       rtparms.rtp_dest_mask,
		       RTPROTO_BIT_ANY);
	if (rt) {
	    /* We have a route to this destination */

	    if (!rtp->rtm_errno) {

		/* Sync to the kernel */
		krt_rth_reset(rt->rt_head,
			      rtparms.rtp_state,
			      rtparms.rtp_n_gw,
			      rtparms.rtp_routers,
			      (if_addr **) 0);

		switch (rt->rt_gwp->gw_proto) {
		case RTPROTO_KERNEL:
		    goto Change;

		case RTPROTO_REDIRECT:
		    rt_delete(rt);
		    break;

		case RTPROTO_DIRECT:
#ifdef	PROTO_INET
		    if (!rtp->rtm_errno
			&& socktype(rt->rt_dest) == AF_INET
			&& !BIT_TEST(rtp->rtm_flags, RTF_GATEWAY)) {
			/* Just an interface route being added */
			
			goto Return;
		    }
#endif	/* PROTO_INET */
		    break;

		default:
		    break;
		}
	    }
	} else if (!rtp->rtm_errno) {
	    /* Indicate the kernel is changed, but we are not in sync */

	    need_sync = TRUE;
	}

    Add:
	BIT_SET(rtparms.rtp_state, RTS_STATIC);

	rt = rt_add(&rtparms);
	if (!rt) {
	    /* The add failed */

	    if (need_sync) {
		/* Delete the bogus entry */

		krt_delete_dst(tp,
			       rtparms.rtp_dest,
			       rtparms.rtp_dest_mask,
			       RTPROTO_KERNEL,
			       rtparms.rtp_state,
			       rtparms.rtp_n_gw,
			       rtparms.rtp_routers,
			       (if_addr **) 0);
	    }

	    trace_log_tp(tp,
			 TRC_NL_BEFORE|TRC_NL_AFTER,
			 LOG_ERR,
			 ("krt_recv_route: unable to add requested route to %A/%A via %A",
			  rtparms.rtp_dest,
			  rtparms.rtp_dest_mask,
			  rtparms.rtp_router));
	} else {
	    /* The add succeeded */

	    if (rtparms.rtp_gwp == krt_gwp_remnant) {
		/* Make sure the remnant timer is running */

		krt_age_create();
	    }

	    if (need_sync) {
		/* Indicate current state of kernel */
		
		krt_rth_reset(rt->rt_head,
			      rtparms.rtp_state,
			      rtparms.rtp_n_gw,
			      rtparms.rtp_routers,
			      (if_addr **) 0);
	    }
	    if (rt != rt->rt_active) {
		/* This is not the active route - delete it */

		rt_delete(rt);

		trace_log_tp(tp,
			     TRC_NL_BEFORE|TRC_NL_AFTER,
			     LOG_ERR,
			     ("krt_recv_route: requested route to %A/%A via %A not active - deleting",
			      rtparms.rtp_dest,
			      rtparms.rtp_dest_mask,
			      rtparms.rtp_router));
	    }
	}
	break;

    case RTM_DELETE:
    case RTM_OLDDEL:
	switch (rtp->rtm_errno) {
	case 0:
	    /* Something was actually changed */

	    /* Indicate a route was deleted */
	    krt_n_routes--;
	    /* Fall through */

	case ENOBUFS:
	    rt = rt_locate(RTS_NETROUTE,
			   rtparms.rtp_dest,
			   rtparms.rtp_dest_mask,
			   RTPROTO_BIT_ANY);
	    if (rt) {
		if (!rtp->rtm_errno) {
		    /* Indicate current kernel status */
		    krt_rth_reset(rt->rt_head,
				  (flag_t) 0,
				  0,
				  (sockaddr_un **) 0,
				  (if_addr **) 0);
		}

		switch (rt->rt_gwp->gw_proto) {
		case RTPROTO_REDIRECT:
		case RTPROTO_KERNEL:
		    /* Delete the route */
		    rt_delete(rt);
		    break;

		case RTPROTO_DIRECT:
#ifdef	PROTO_INET
		    if (socktype(rt->rt_dest) == AF_INET
			&& !BIT_TEST(rtp->rtm_flags, RTF_GATEWAY)) {
			/* Just and interface route being deleted */
			
			break;
		    }
#endif	/* PROTO_INET */
		    /* Fall through */
		    
		default:
		    trace_log_tp(tp,
				 TRC_NL_BEFORE|TRC_NL_AFTER,
				 LOG_ERR,
				 ("krt_recv_route: overriding attempt to delete route to %A/%A",
				  rtparms.rtp_dest,
				  rtparms.rtp_dest_mask,
				  rtparms.rtp_router));
		    break;
		}
	    }
	    break;

	default:
	    break;
	}
	break;

    case RTM_CHANGE:
	switch (rtp->rtm_errno) {
	default:
	    /* Can't handle it */
	    goto Return;

	case 0:
	    need_sync = TRUE;
	    break;
	    
	case ESRCH:
	case ENOBUFS:
	case EDQUOT:
	    /* We can process these */
	    break;
	}
	
	rt = rt_locate(RTS_NETROUTE,
		       rtparms.rtp_dest,
		       rtparms.rtp_dest_mask,
		       RTPROTO_BIT_ANY);
	if (!rt) {
	    /* Could not find this route, treat it as an add */

	    goto Add;
	}

	if (!rtp->rtm_errno) {
	    /* Make note of the current kernel state */

	    need_sync = FALSE;
	    krt_rth_reset(rt->rt_head,
			  rtparms.rtp_state,
			  rtparms.rtp_n_gw,
			  rtparms.rtp_routers,
			  (if_addr **) 0);
	}

	switch (rt->rt_gwp->gw_proto) {
	case RTPROTO_REDIRECT:
	    /* Delete the redirect and add a kernel static route */
	    rt_delete(rt);
	    goto Add;

	case RTPROTO_KERNEL:
	Change:
	    BIT_RESET(rt->rt_state, RTS_NOADVISE|RTS_REJECT);
	    BIT_SET(rt->rt_state, RTS_STATIC);
	    rt->rt_state |= BIT_TEST(rtparms.rtp_state, RTS_REJECT);
	    (void) rt_change(rt,
			     (metric_t) 0,
			     (metric_t) 0,
			     (metric_t) 0,
			     RTPREF_KERNEL,
			     (pref_t) 0,
			     1, &rtparms.rtp_router);
	    break;
	    
	default:
	    goto Add;
	}
	break;
    }

 Return:
    rt_close(tp, (gw_entry *) 0, 1, NULL);
}


/* Process a route socket response from the kernel */
void
krt_recv __PF1(tp, task *)
{
    int n_packets = TASK_PACKET_LIMIT;
    size_t size;

#ifdef	SCO_OSR5
    while (n_packets-- && !task_receive_route_packet(tp, &size)) {
#else	/* SCO_OSR5 */
    while (n_packets-- && !task_receive_packet(tp, &size)) {
#endif	/* SCO_OSR5 */
	struct rt_msghdr *rtp = task_get_recv_buffer(struct rt_msghdr *);
	krt_addrinfo *adip;

	adip = krt_xaddrs(rtp, size);
	if (!adip) {
	    continue;
	}

	if (TRACE_PACKET_RECV_TP(tp,
				 rtp->rtm_type,
				 RTM_MAX,
				 krt_trace_masks)) {
	    krt_trace_msg(tp,
			  "RECV",
			  rtp,
			  size,
			  adip,
			  LOG_DEBUG,
			  TRACE_DETAIL_RECV_TP(tp,
					       rtp->rtm_type,
					       RTM_MAX,
					       krt_trace_masks));
	}

	switch (rtp->rtm_type) {
	case RTM_ADD:
	case RTM_DELETE:
	case RTM_CHANGE:
	case RTM_OLDADD:
	case RTM_OLDDEL:
	case RTM_REDIRECT:
	    krt_recv_route(tp, rtp, adip);
	    break;

	case RTM_LOSING:
	case RTM_MISS:
	case RTM_RESOLVE:
	    krt_recv_rtinfo(tp, rtp, adip);
	    break;

	case RTM_GET:
	case RTM_LOCK:
	    break;
	    
#ifdef	KRT_IFREAD_KINFO
	case RTM_IFINFO:
            {
		struct if_msghdr *ifp = (struct if_msghdr *) rtp;
		sockaddr_un *ap = adip->rti_info[RTAX_IFP];

		if_conf_open(tp, FALSE);
		(void) ifl_addup(tp,
				 ifl_locate_index(ifp->ifm_index),
				 ifp->ifm_index,
				 krt_if_flags(ifp->ifm_flags),
#ifdef	SCO_GEMINI
			  	 ((struct ifa_msghdr *) ifp)->ifam_metric,
				 (mtu_t) 0,
#else	/*SCO_GEMINI*/
				 ifp->ifm_data.ifi_metric,
				 (mtu_t) ifp->ifm_data.ifi_mtu,
#endif	/*SCO_GEMINI*/
				 ap ? ap->dl.gdl_data : (char *) 0,
				 ap ? ap->dl.gdl_nlen : (size_t) 0,
				 ap ? sockbuild_ll(krt_type_to_ll(ap->dl.gdl_type),
						      (byte *) (ap->dl.gdl_data + ap->dl.gdl_nlen),
						      ap->dl.gdl_alen) : (sockaddr_un *) 0);
		if_conf_close(tp, TRUE);
	    }
	    break;

	case RTM_NEWADDR:
	case RTM_DELADDR:
	    {
		struct ifa_msghdr *ifap = (struct ifa_msghdr *) rtp;
		if_link *ifl = ifl_locate_index(ifap->ifam_index);

		if_conf_open(tp, FALSE);
		krt_ifaddr(tp,
			   (struct ifa_msghdr *) rtp,
			   adip,
			   ifl);
		if_conf_close(tp, FALSE);
	    }
	    break;
#endif	/* KRT_IFREAD_KINFO */

	default:
	    break;
	}
    }
}
