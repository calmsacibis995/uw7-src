#ident	"@(#)krt_ifread_kinfo.c	1.2"
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


#ifdef	SCO_GEMINI
#define	INCLUDE_IOCTL
#endif	/*SCO_GEMINI*/
#define	INCLUDE_KINFO
#define	INCLUDE_ROUTE
#define	INCLUDE_IF
#include "include.h"
#ifdef	PROTO_INET
#include "inet.h"
#endif	/* PROTO_INET */
#ifdef	PROTO_ISO
#include "iso.h"
#endif	/* PROTO_ISO */
#include "krt.h"
#include "krt_var.h"

#if	IFNAMSIZ > IFL_NAMELEN
error - IFL_NAMELEN not compatible with IFNAMESIZ
#endif


int
krt_ifread __PF1(save_task_state, flag_t)
{
    size_t size;
    caddr_t kbuf, cp, limit;
    struct if_msghdr *ifap;
    if_link *ifl = (if_link *) 0;
    register task *tp = krt_task;
#ifdef	USE_SYSCTL
    static int mib[] = { CTL_NET, PF_ROUTE, 0, 0, NET_RT_IFLIST, 0 };
#endif	/* USE_SYSCTL */
#ifdef SCO_GEMINI
    metric_t	metric = 0;
    mtu_t	mtu = 0;
    static int	sock = -1;
    struct rt_giarg *gp;

    if (task_send_buffer_len < sizeof(struct rt_giarg)) {
	task_alloc_send(tp, sizeof(struct rt_giarg));
    }

    for(;;) {
	gp = (struct rt_giarg *)task_send_buffer;
	gp->gi_op = KINFO_RT_DUMP;
	gp->gi_where = (caddr_t)task_send_buffer;
	gp->gi_size = task_send_buffer_len;
	gp->gi_arg = 0;

	if (ioctl(tp->task_socket, RTSTR_GETIFINFO, task_send_buffer) <0) {
	    trace_log_tp(tp, 0, LOG_ERR,
			 ("krt_ifread: GETIFINFO: %m"));
	    return errno;
	}

	size = gp->gi_size;
	if (task_send_buffer_len >= size)
		break;

	trace_tp(tp, TR_NORMAL, 0,
		     ("krt_ifread: %s estimates %d bytes needed",
		      "GETIFINFO", size));
	task_alloc_send(tp, size);
    }
    kbuf = task_send_buffer;
    kbuf += sizeof(struct rt_giarg);
    size -=sizeof(struct rt_giarg);

#else	/*SCO_GEMINI*/

    while (TRUE) {

	if (
#ifdef	USE_SYSCTL
	    sysctl(mib, sizeof mib / sizeof *mib, (caddr_t) 0, &size, NULL, 0)
#else	/* USE_SYSCTL */
	    (int) (size = getkerninfo(KINFO_RT_IFLIST, (caddr_t) 0, (size_t *) 0, 0))
#endif	/* USE_SYSCTL */
	    < 0) {
	    trace_log_tp(tp,
			 0,
			 LOG_ERR,
			 ("krt_ifread: getkerninfo/sysctl: %m"));
	    return errno;
	}
	if (size > task_send_buffer_len) {
	    /* Need more memory */

	    trace_tp(tp,
		     TR_NORMAL,
		     0,
		     ("krt_ifread: %s estimates %d bytes needed",
		      "getkerninfo/sysctl",
		      size));
	    task_alloc_send(tp, size);
	}
	kbuf = task_send_buffer;
	if (
#ifdef	USE_SYSCTL
	    sysctl(mib, sizeof mib / sizeof *mib, task_send_buffer, &size, NULL, 0)
#else	/* USE_SYSCTL */
	    getkerninfo(KINFO_RT_IFLIST, task_send_buffer, &size, 0)
#endif	/* USE_SYSCTL */
	    < 0) {
	    trace_log_tp(tp,
			 0,
			 LOG_ERR,
			 ("krt_ifread: interface table retrieval %m"));
	} else {
	    /* Should have the data */
	    
	    break;
	}
    }
#endif	/*SCO_GEMINI*/

    limit = kbuf + size;

    /* Tell the IF code that we are passing complete knowledge */
    if_conf_open(tp, TRUE);
    
    for (cp = kbuf; cp < limit; cp += ifap->ifm_msglen) {
	krt_addrinfo *adip;
	
	ifap = (struct if_msghdr *) cp;

	/* Pick out the addresses */
	adip = krt_xaddrs((struct rt_msghdr *) ifap, ifap->ifm_msglen);
	if (!adip) {
	    /* Try the next message */
	    continue;
	}

	/* Trace the message */
	if (TRACE_TP(tp, TR_KRT_IFLIST)) {
	    /* Always trace in detail */
	    
	    krt_trace_msg(tp,
			  "IFINFO",
			  (struct rt_msghdr *) ifap,
			  ifap->ifm_msglen,
			  adip,
			  0,
			  TRUE);
	}

	switch (ifap->ifm_type) {
	    sockaddr_un *ap;

	case RTM_IFINFO:
	    {
		/* New interface */

		if ((ap = adip->rti_info[RTAX_IFP])) {
		    /* Link level info */
		    sockaddr_un *lladdr;

		    if (ap->dl.gdl_alen) {
			lladdr = sockbuild_ll(krt_type_to_ll(ap->dl.gdl_type),
					      (byte *) ap->dl.gdl_data + ap->dl.gdl_nlen,
					      (size_t) ap->dl.gdl_alen);
		    } else {
			lladdr = (sockaddr_un *) 0;
		    }
#ifdef	SCO_GEMINI
	{
		struct ifreq ifreq;
		struct ifreq *ifr = &ifreq;

		if (sock < 0) {
		    if (BIT_TEST(task_state, TASKS_TEST))
			sock = socket(AF_INET, SOCK_DGRAM, 0);
		    else
			sock = task_get_socket(tp, AF_INET, SOCK_DGRAM, 0);
		}
		if (sock < 0) {
			return EBADF;
		}
		bzero((caddr_t) &ifr->ifr_ifru, sizeof(ifr->ifr_ifru));
		strncpy(ifr->ifr_name, ap->dl.gdl_data, sizeof(ifr->ifr_name));
		if (ioctl(sock, SIOCGIFMETRIC, ifr, sizeof(*ifr)) < 0) {
		    trace_log_tp(tp, 0, LOG_ERR,
			("krt_ifread: %s: ioctl SIOCGIFMETRIC: %m",
			ifr->ifr_name));
		    metric = 0;
		} else {
		    metric = ifr->ifr_metric;
		}
		bzero((caddr_t) &ifr->ifr_ifru, sizeof(ifr->ifr_ifru));
		strncpy(ifr->ifr_name, ap->dl.gdl_data, sizeof(ifr->ifr_name));
		if (ioctl(sock, SIOCGIFMTU, ifr, sizeof(*ifr)) < 0) {
		    trace_log_tp(tp, 0, LOG_ERR,
			("krt_ifread: %s: ioctl SIOCGIFMTU: %m",
			ifr->ifr_name));
		    mtu = 0;
		} else {
		    mtu = ifr->ifr_mtu;
		}
	}
#endif	/*SCO_GEMINI*/

		    ifl = ifl_addup(tp,
				    ifl_locate_index(ap->dl.gdl_index),
				    ap->dl.gdl_index,
				    krt_if_flags(ifap->ifm_flags),
#ifdef	SCO_GEMINI
				    (metric_t) metric, (mtu_t) mtu,
#else	/*SCO_GEMINI*/
				    ifap->ifm_data.ifi_metric,
				    (mtu_t) ifap->ifm_data.ifi_mtu,
#endif	/*SCO_GEMINI*/
				    ap->dl.gdl_data,
				    ap->dl.gdl_nlen,
				    lladdr);
		} else {
		    /* No link level info? */

		    ifl = (if_link *) 0;
		}
	    }
	    break;

	case RTM_NEWADDR:
	    krt_ifaddr(tp,
		       (struct ifa_msghdr *) ifap,
		       adip,
		       ifl);
	    break;

	default:
	    trace_log_tp(tp,
			 0,
			 LOG_ERR,
			 ("krt_ifread: ignoring unknown message type: %s (%d)",
			  trace_state(rtm_type_bits, ifap->ifm_type),
			  ifap->ifm_type));
	    continue;
	}

	trace_tp(tp,
		 TR_NORMAL,
		 0,
		 (NULL));
    }

    if_conf_close(tp, FALSE);

    return 0;
}
