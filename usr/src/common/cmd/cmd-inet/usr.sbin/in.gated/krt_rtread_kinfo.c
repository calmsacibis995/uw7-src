#ident	"@(#)krt_rtread_kinfo.c	1.3"
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


#define	INCLUDE_FILE
#define	INCLUDE_IOCTL
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


/* Use the ioctl call to read the routing table(s) */
/*ARGSUSED*/
int
krt_rtread __PF1(tp, task *)
{
    size_t size, alloc_size;
    caddr_t kbuf, korig, cp, limit;
    rt_parms rtparms;
    struct rt_msghdr *rtp;
    struct rt_giarg gi_arg, *gp;
    int  r, s;


    trace_only_tp(tp,
	TRC_NL_BEFORE,
	("krt_rtread: Initial routes read from kernel (via RTSTR_GETROUTE):"));

    bzero((caddr_t) &rtparms, sizeof (rtparms));
    rtparms.rtp_n_gw = 1;

    s = open(_PATH_ROUTE, O_RDWR);
    if (s < 0) {
	trace_log_tp(tp,
		     0,
		     LOG_ERR,
		     ("krt_rtread: %s: %m", _PATH_ROUTE));
	return errno;
    }

    gi_arg.gi_op = KINFO_RT_DUMP;
    gi_arg.gi_where = (caddr_t)0;
    gi_arg.gi_size = 0;
    gi_arg.gi_arg = 0;
    r = ioctl(s, RTSTR_GETROUTE, &gi_arg);

    if (r < 0) {
	trace_log_tp(tp,
	     0,
	     LOG_ERR,
	     ("krt_rtread: RTSTR_GETROUTE estimate: %m"));
	(void) close (s);
	return errno;
    }
    size = alloc_size = gi_arg.gi_size;
    trace_tp(tp,
	     TR_STATE,
	     0,
	     ("krt_rtread: ioctl estimates %d bytes needed",
	      alloc_size));

    size = alloc_size = ROUNDUP(alloc_size, task_pagesize);
    korig = kbuf = (caddr_t) task_block_malloc(alloc_size);
    if (kbuf == 0) {
	trace_log_tp(tp, 0, LOG_ERR,
	     ("krt_rtread: malloc(%d) failed", size));
	(void) close (s);
	return ENOMEM;
    }

    gp = (struct rt_giarg *)kbuf;
    gp->gi_op = KINFO_RT_DUMP;
    gp->gi_where = (caddr_t)kbuf;
    gp->gi_size = gi_arg.gi_size;
    gp->gi_arg = 0;
    r = ioctl(s, RTSTR_GETROUTE, kbuf);
    if (r < 0) {
	trace_log_tp(tp, 0, LOG_ERR,
	     ("krt_rtread: RTSTR_GETROUTE: %m"));
	(void) close (s);
	return errno;
    }
    limit = kbuf + gp->gi_size;
    kbuf += sizeof(gi_arg);

    for (cp = kbuf; cp < limit; cp += rtp->rtm_msglen) {
	sockaddr_un *author;
	krt_addrinfo *adip;
	const char *errmsg = (char *) 0;
	int pri = 0;

	rtp = (struct rt_msghdr *) ((void_t) cp);

	adip = krt_xaddrs(rtp,
			  (size_t) rtp->rtm_msglen);
	if (!adip) {
	    continue;
	}

	if (TRACE_TP(tp, TR_KRT_REMNANTS)) {
	    /* always trace in detail */
	    krt_trace_msg(tp,
		      "RTINFO",
		      rtp,
		      (size_t) rtp->rtm_msglen,
		      adip,
		      0,
		      TRUE);
	}

	switch (krt_rtaddrs(adip, &rtparms, &author, (flag_t) rtp->rtm_flags)) {
	case KRT_ADDR_OK:
	    break;

	case KRT_ADDR_IGNORE:
	    errmsg = "ignoring";
	    pri = LOG_INFO;
	    goto Trace;

	case KRT_ADDR_BOGUS:
	    errmsg = "deleting bogus";
	    pri = LOG_WARNING;
	    krt_delq_add(&rtparms);
	    goto Trace;

#ifdef	IP_MULTICAST
	case KRT_ADDR_MC:
	    if (krt_multicast_install(rtparms.rtp_dest, rtparms.rtp_router)) {
		errmsg = "deleting multicast";
		pri = LOG_WARNING;
		krt_delq_add(&rtparms);
		goto Trace;
	    }
	    errmsg = "ignoring multicast";
	    pri = LOG_INFO;
	    goto Trace;
#endif	/* IP_MULTICAST */
	}

	errmsg = krt_rtadd(&rtparms, (flag_t) rtp->rtm_flags);
	if (errmsg) {
	    /* It has been deleted */

	    pri = LOG_WARNING;
	}

    Trace:
	if (errmsg) {
	    krt_trace(tp,
		      "READ",
		      "REMNANT",
		      adip->rti_info[RTAX_DST],
		      adip->rti_info[RTAX_NETMASK],
		      adip->rti_info[RTAX_GATEWAY],
		      (flag_t) rtp->rtm_flags,
		      errmsg,
		      pri);
	}
    }

    task_block_reclaim(alloc_size, korig);
    (void) close(s);
    return 0;
}
