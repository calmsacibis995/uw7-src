#ident	"@(#)icmp.c	1.4"
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
 *  Routines for handling ICMP messages
 */

#include "include.h"
#include "inet.h"
#include "ip_icmp.h"
#include "icmp.h"

#if	defined(PROTO_ICMP)

#ifdef	KRT_RT_IOCTL
#define	ICMP_PROCESS_REDIRECTS
#endif

task *icmp_task = (task *) 0;
trace *icmp_trace_options = (trace *) 0;

static int icmp_proto = 0;

#ifdef	ICMP_SEND
static int icmp_unicast_ttl = -1;
#ifdef	IP_MULTICAST
static if_addr *icmp_multicast_ifap = (if_addr *) 0;           /*the current multicast interface to send */
#endif	/* IP_MULTICAST */
#endif	/* ICMP_SEND */

_PROTOTYPE(icmp_methods[ICMP_MAXTYPE],
	   void,
	   (task *,
	    sockaddr_un *,
	    sockaddr_un *,
	    struct icmp *,
	    size_t));

struct icmptype {
    const char *typename;
    u_int codes;
    const char *codenames[6];
};

static const struct icmptype icmp_types[ICMP_MAXTYPE] =
{
    {"EchoReply", 0,
     {""}},
    {"", 0,
     {""}},
    {"", 0,
     {""}},
    {"UnReachable", 5,
   {"Network", "Host", "Protocol", "Port", "NeedFrag", "SourceFailure"}},
    {"SourceQuench", 0,
     {""}},
    {"ReDirect", 4,
     {"Network", "Host", "TOSNetwork", "TOSHost"}},
    {"", 0,
     {""}},
    {"", 0,
     {""}},
    {"Echo", 0,
     {""}},
    {"RouterAdvertisement", 0,
     {""}},
    {"", 0,
     {"RouterSolicitation"}},
    {"TimeExceeded", 1,
     {"InTransit", "Reassembly"}},
    {"ParamProblem", 0,
     {""}},
    {"TimeStamp", 0,
     {""}},
    {"TimeStampReply", 0,
     {""}},
    {"InfoRequest", 0,
     {""}},
    {"InfoRequestReply", 0,
     {""}},
    {"MaskRequest", 0,
     {""}},
    {"MaskReply", 0,
     {""}}
};

static const flag_t icmp_trace_masks[ICMP_MAXTYPE + 1] = {
    TR_ICMP_DETAIL_INFO,	/* 0 - Echo Reply */
    TR_ALL,			/* 1 - Invalid */
    TR_ALL,			/* 2 - Invalid */
    TR_ICMP_DETAIL_ERROR,	/* 3 - Unreachable */
    TR_ICMP_DETAIL_ERROR,	/* 4 - Source Quench */
    TR_ICMP_DETAIL_REDIRECT,	/* 5 - Redirect */
    TR_ALL,			/* 6 - Invalid */
    TR_ALL,			/* 7 - Invalid */
    TR_ICMP_DETAIL_INFO,	/* 8 - Echo Request */
    TR_ICMP_DETAIL_ROUTER,	/* 9 - Router Advertisement */
    TR_ICMP_DETAIL_ROUTER,	/* 10 - Router Solicitation */
    TR_ICMP_DETAIL_ERROR,	/* 11 - Time exceeded */
    TR_ICMP_DETAIL_ERROR,	/* 12 - Parameter Problem */
    TR_ICMP_DETAIL_INFO,	/* 13 - Timestamp Request */
    TR_ICMP_DETAIL_INFO,	/* 14 - Timestamp Reply */
    TR_ICMP_DETAIL_INFO,	/* 15 - Info Request */
    TR_ICMP_DETAIL_INFO,	/* 16 - Info Reply */
    TR_ICMP_DETAIL_INFO,	/* 17 - Mask Request */
    TR_ICMP_DETAIL_INFO		/* 18 - Mask Reply */
};

const bits icmp_trace_types[] = {
    { TR_DETAIL,		"detail packets" },
    { TR_DETAIL_SEND,	"detail send packets" },
    { TR_DETAIL_RECV,	"detail recv packets" },
    { TR_PACKET,		"packets" },
    { TR_PACKET_SEND,	"send packets" },
    { TR_PACKET_RECV,	"recv packets" },
    { TR_DETAIL_1,	"detail redirect" },
    { TR_DETAIL_SEND_1,	"detail send redirect" },
    { TR_DETAIL_RECV_1,	"detail recv redirect" },
    { TR_PACKET_1,	"redirect" },
    { TR_PACKET_SEND_1,	"send redirect" },
    { TR_PACKET_RECV_1,	"recv redirect" },
    { TR_DETAIL_2,	"detail router" },
    { TR_DETAIL_SEND_2,	"detail send router" },
    { TR_DETAIL_RECV_2,	"detail recv router" },
    { TR_PACKET_2,	"router" },
    { TR_PACKET_SEND_2,	"send router" },
    { TR_PACKET_RECV_2,	"recv router" },
    { TR_DETAIL_3,	"detail info" },
    { TR_DETAIL_SEND_3,	"detail send info" },
    { TR_DETAIL_RECV_3,	"detail recv info" },
    { TR_PACKET_3,	"info" },
    { TR_PACKET_SEND_3,	"send info" },
    { TR_PACKET_RECV_3,	"recv info" },
    { TR_DETAIL_4,	"detail error" },
    { TR_DETAIL_SEND_4,	"detail send error" },
    { TR_DETAIL_RECV_4,	"detail recv error" },
    { TR_PACKET_4,	"error" },
    { TR_PACKET_SEND_4,	"send error" },
    { TR_PACKET_RECV_4,	"recv error" },
    { 0, NULL }
};


/*
 * Trace received ICMP messages
 */
static void
icmp_trace __PF5(from, sockaddr_un *,
		 to, sockaddr_un *,
		 dir, const char *,
		 icmp, struct icmp *,
		 detail, int)
{
    const char *type_name, *code_name;
    const struct icmptype *itp;

    if (icmp->icmp_type < ICMP_MAXTYPE) {
	itp = &icmp_types[icmp->icmp_type];
	type_name = itp->typename;
	if (icmp->icmp_code <= itp->codes) {
	    code_name = itp->codenames[icmp->icmp_code];
	} else {
	    code_name = "Invalid";
	}
    } else {
	type_name = "Invalid";
	code_name = "";
    }

    tracef("ICMP %s %A ",
	   dir,
	   from);

    if (to) {
	tracef("-> %A ",
	       to);
    }

    tracef("type %s(%u) code %s(%u)",
	   type_name,
	   icmp->icmp_type,
	   code_name,
	   icmp->icmp_code);

    if (detail) {
	int do_mask = FALSE;
	int do_dest = FALSE;
	int do_idseq = FALSE;
    
	switch (icmp->icmp_type) {
	case ICMP_ECHOREPLY:
	case ICMP_ECHO:
	    do_idseq = TRUE;
	    break;
    
	case ICMP_UNREACH:
	    /* XXX - Port? */
	    do_dest = TRUE;
	    break;
	    
	case ICMP_SOURCEQUENCH:
	    break;
    
	case ICMP_REDIRECT:
	    tracef(" dest %A via %A",
		   sockbuild_in(0, icmp->icmp_ip.ip_dst.s_addr),
		   sockbuild_in(0, icmp->icmp_gwaddr.s_addr));
	    break;
    
	case ICMP_TIMXCEED:
	case ICMP_PARAMPROB:
	    do_dest = TRUE;
	    break;

	case ICMP_TSTAMP:
	case ICMP_TSTAMPREPLY:
	    tracef(" originate %T.%u receive %T.%u transmit %T.%u",
		   icmp->icmp_otime / 1000,
		   icmp->icmp_otime % 1000,
		   icmp->icmp_rtime / 1000,
		   icmp->icmp_rtime % 1000,
		   icmp->icmp_ttime / 1000,
		   icmp->icmp_ttime % 1000);
	    break;
    
	case ICMP_IREQ:
	case ICMP_IREQREPLY:
	    do_idseq = do_dest = TRUE;
	    break;
    
	case ICMP_MASKREQ:
	case ICMP_MASKREPLY:
	    do_idseq = do_mask = TRUE;
	    break;

	case ICMP_ROUTERADVERT:
	    {
		register struct id_rdisc *rdp = &icmp->icmp_rdisc;
		register struct id_rdisc *lp = (struct id_rdisc *) ((void_t) ((byte *) rdp + (icmp->icmp_addrnum * icmp->icmp_addrsiz * sizeof (u_int32))));
		trace_only_tf(icmp_trace_options,
			      0,
			      (NULL));
		tracef("ICMP %s	addresses %u size %u lifetime %#T",
		       dir,
		       icmp->icmp_addrnum,
		       icmp->icmp_addrsiz * sizeof (struct in_addr),
		       ntohs(icmp->icmp_lifetime));

		for (; rdp < lp;
		     rdp = (struct id_rdisc *) (void_t) (((byte *) rdp + (icmp->icmp_addrsiz * sizeof (u_int32))))) {
		    trace_only_tf(icmp_trace_options,
				  0,
				  (NULL));
		    tracef("ICMP %s	router %-15A preference %d",
			   dir,
			   sockbuild_in(0, rdp->ird_addr.s_addr),
			   ntohl(rdp->ird_pref));
		}			
	    }
	    break;

	case ICMP_ROUTERSOLICIT:
	    break;
	}

	if (do_idseq) {
	    tracef(" id %u sequence %u",
		   icmp->icmp_id,
		   icmp->icmp_seq);
	}
	if (do_dest) {
	    tracef(" dest %A protocol %s(%u)",
		   sockbuild_in(0, icmp->icmp_ip.ip_dst.s_addr),
		   trace_value(inet_proto_bits, icmp->icmp_ip.ip_p),
		   icmp->icmp_ip.ip_p);
	}
	if (do_mask) {
	    tracef(" mask %A",
		   sockbuild_in(0, icmp->icmp_mask));
	}
    }
    trace_only_tf(icmp_trace_options,
		  TRC_NL_AFTER,
		  (NULL));
}


#ifdef	ICMP_PROCESS_REDIRECTS
/**/

static task_job *icmp_redirect_job = (task_job *) 0;
static block_t icmp_block_index;

struct icmp_redirect_entry {
    struct icmp_redirect_entry *ire_forw, *ire_back;
    u_int32 ire_author;		/* Source of the packet */
    u_int32 ire_router;		/* New gateway */
    u_int32 ire_dest;		/* Destination to redirect */
    int ire_mask;		/* Prefix */
} ;

static struct icmp_redirect_entry icmp_redirect_list = { &icmp_redirect_list, &icmp_redirect_list };


static void
icmp_redirect_process __PF1(jp, task_job *)
{
    register struct icmp_redirect_entry *rep;

    while ((rep = icmp_redirect_list.ire_forw) != &icmp_redirect_list) {
	sockaddr_un *author, *router, *dest, *mask;

	author = sockdup(sockbuild_in(0, rep->ire_author));
	router = sockdup(sockbuild_in(0, rep->ire_router));
	dest = sockdup(sockbuild_in(0, rep->ire_dest));
	mask = inet_mask_prefix(rep->ire_mask);

	redirect(dest, mask, router, author);

	sockfree(dest);
	sockfree(author);
	sockfree(router);

	REMQUE(rep);
	task_block_free(icmp_block_index, (void_t) rep);
    }

    icmp_redirect_job = (task_job *) 0;
}


static void
icmp_recv_redirect __PF5(tp, task *,
			 src, sockaddr_un *,
			 dst, sockaddr_un *,
			 icmp, struct icmp *,
			 len, size_t)
{
    register struct icmp_redirect_entry *rep;
    struct icmp_redirect_entry re;
    struct icmp_redirect_entry *il;

    re.ire_author = sock2ip(src);
    re.ire_router = icmp->icmp_gwaddr.s_addr;
    re.ire_dest = icmp->icmp_ip.ip_dst.s_addr;

    switch (icmp->icmp_code) {
    case ICMP_REDIRECT_TOSNET:
	if (icmp->icmp_ip.ip_tos) {
	    /* We only support TOS 0 */
	    
	    return;
	}
	/* Fall through */

    case ICMP_REDIRECT_NET:
	re.ire_mask = inet_prefix_mask(inet_mask_natural_byte(&re.ire_dest));
	break;

    case ICMP_REDIRECT_TOSHOST:
	if (icmp->icmp_ip.ip_tos) {
	    /* We only support TOS 0 */
	    
	    return;
	}
	/* Fall through */

    case ICMP_REDIRECT_HOST:
	re.ire_mask = inet_prefix_mask(inet_mask_host);
	break;

    default:
	/* Unknown type */
	return;
    }
    
    /* Check for duplicate */
    for (rep = icmp_redirect_list.ire_forw; rep != &icmp_redirect_list; rep = rep->ire_forw) {
	if (rep->ire_author == re.ire_author
	    && rep->ire_router == re.ire_router
	    && rep->ire_dest == re.ire_dest
	    && rep->ire_mask == re.ire_mask) {
	    /* Found a duplicate */

	    return;
	}
    }

    /* Add to list */
    il = (struct icmp_redirect_entry *) task_block_alloc(icmp_block_index);
    *il = re;	/* struct copy */
    INSQUE(il, icmp_redirect_list.ire_back);

    if (!icmp_redirect_job) {
	/* Create the job */
	
	icmp_redirect_job = task_job_create(tp,
					    TASK_JOB_FG,
					    "icmp_redirect_job",
					    icmp_redirect_process,
					    (void_t) 0);
    }
}
#endif	/* ICMP_PROCESS_REDIRECTS */


/*
 * icmp_recv() handles ICMP redirect messages.
 */
static void
icmp_recv __PF1(tp, task *)
{
    int n_packets = TASK_PACKET_LIMIT * 10;
    size_t count;

    while (n_packets-- && !task_receive_packet(tp, &count)) {
	struct icmp *icmp;
	sockaddr_un *dst;

	struct ip *ip;

	task_parse_ip(ip, icmp, struct icmp *);

	count -= sizeof (struct ip);

	dst = sockbuild_in(0, ip->ip_dst.s_addr);

	if (TRACE_PACKET_RECV_TP(tp,
				 icmp->icmp_type,
				 ICMP_MAXTYPE,
				 icmp_trace_masks)) {
	    icmp_trace(task_recv_srcaddr,
		       dst,
		       "RECV",
		       icmp,
		       TRACE_DETAIL_RECV_TP(tp,
					    icmp->icmp_type,
					    ICMP_MAXTYPE,
					    icmp_trace_masks));
	}

	if (icmp->icmp_type < ICMP_MAXTYPE
	    && icmp_methods[icmp->icmp_type]) {
	    /* Call the routine to process this packet */

	    icmp_methods[icmp->icmp_type](tp,
					  task_recv_srcaddr,
					  dst,
					  icmp,
					  count);
	}
    }
}


#ifdef	ICMP_SEND
/* 
 *	Send an ICMP packet
 */
int
icmp_send __PF5(icmp, struct icmp *,
		len, size_t,
		dest, sockaddr_un *,
		ifap, if_addr *,
		flags, flag_t)
{
    int rc;

    /* calc checksum */
    icmp->icmp_cksum = 0;
    icmp->icmp_cksum = inet_cksum(icmp, len);  

#ifdef	IP_MULTICAST
    if (inet_class_of(dest) == INET_CLASSC_MULTICAST) {
	/* Multicast processing */
	
	/* Multicast sends fail if MSG_DONTROUTE is set */
	BIT_RESET(flags, MSG_DONTROUTE);
	
	if (icmp_multicast_ifap != ifap) {
	    /* reset the interface on which to multicast */
	    IFA_FREE(icmp_multicast_ifap);
	    IFA_ALLOC(icmp_multicast_ifap = ifap);
	    if (task_set_option(icmp_task,
				TASKOPTION_MULTI_IF,
				icmp_multicast_ifap) < 0) {
		task_quit(errno);
	    }
	}
    } else
#endif	/* IP_MULTICAST */
    {
	int ttl = BIT_TEST(flags, MSG_DONTROUTE) ? 1 : MAXTTL;
	
	/* Unicast processing */

	if (ttl != icmp_unicast_ttl) {
	    /* Set the TTL */

	    (void) task_set_option(icmp_task,
				   TASKOPTION_TTL,
				   icmp_unicast_ttl = ttl);
	}
    }
    
    /* send the packet */
    rc = task_send_packet(icmp_task, 
			  (void_t) icmp,
			  len,
			  flags,
			  dest);

    if (TRACE_PACKET_SEND_TP(icmp_task,
			     icmp->icmp_type,
			     ICMP_MAXTYPE,
			     icmp_trace_masks)) {
	icmp_trace(ifap->ifa_addr_local,
		   dest,
		   "SEND",
		   icmp,
		   TRACE_DETAIL_SEND_TP(icmp_task,
					icmp->icmp_type,
					ICMP_MAXTYPE,
					icmp_trace_masks));
	}


    return rc;
}


/*
 *  Initialize ICMP socket and ICMP task
 */

static void
icmp_ifachange __PF2(tp, task *,
		     ifap, if_addr *)
{
    switch (ifap->ifa_change) {
    case IFC_NOCHANGE:
    case IFC_ADD:
	break;
    
    case IFC_DELETE:
	break;
    
    case IFC_DELETE|IFC_UPDOWN:
    Down:
	/* mark addr as down */
#ifdef	IP_MULTICAST
	if (ifap == icmp_multicast_ifap) {
	    IFA_FREE(icmp_multicast_ifap);
	    icmp_multicast_ifap = (if_addr *) 0;
	}	
#endif	/* IP_MULTICAST */
	break;
    
    default:
	/* Something has changed */
  
	if (BIT_TEST(ifap->ifa_change, IFC_UPDOWN)) {
	    if (!BIT_TEST(ifap->ifa_state, IFS_UP)) {
		/* Transition to DOWN */
	
		goto Down;
	    }
	}
    }
}
#endif	/* ICMP_SEND */


/*
 *  Status dump
 */
static void
icmp_dump __PF2(tp, task *,
		fp, FILE *)
{
}


void
icmp_var_init __PF0(void)
{
    int i;

    for (i = 0; i < ICMP_MAXTYPE; i++) {
	icmp_methods[i] = NULL;
    }
}


static void
icmp_cleanup __PF1(tp, task *)
{
#if	defined(ICMP_SEND) && defined(IP_MULTICAST)
    if (icmp_multicast_ifap) {
	IFA_FREE(icmp_multicast_ifap);
	icmp_multicast_ifap = (if_addr *) 0;
    }
#endif	/* defined(ICMP_SEND) && defined(IP_MULTICAST) */

    trace_freeup(icmp_trace_options);
    trace_freeup(tp->task_trace);
}


static void
icmp_reinit __PF1(tp, task *)
{
    trace_inherit_global(icmp_trace_options, icmp_trace_types, (flag_t) 0);
    trace_set(tp->task_trace, icmp_trace_options);
#ifdef	ICMP_PROCESS_REDIRECTS
    icmp_methods[ICMP_REDIRECT] = icmp_recv_redirect;
#endif	/* ICMP_PROCESS_REDIRECTS */
}


static void
icmp_terminate __PF1(tp, task *)
{
    icmp_cleanup(tp);

    task_delete(tp);
}


void
icmp_init __PF0(void)
{

    if (!icmp_task) {
	trace_inherit_global(icmp_trace_options, icmp_trace_types, (flag_t) 0);
	if (!icmp_proto) {
	    icmp_proto = task_get_proto(icmp_trace_options,
					"icmp",
					IPPROTO_ICMP);
	}
	icmp_task = task_alloc("ICMP",
			       TASKPRI_ICMP,
			       icmp_trace_options);
	icmp_task->task_proto = icmp_proto;
	task_set_recv(icmp_task, icmp_recv);
	task_set_cleanup(icmp_task, icmp_cleanup);
	task_set_reinit(icmp_task, icmp_reinit);
	task_set_terminate(icmp_task, icmp_terminate);
	task_set_dump(icmp_task, icmp_dump);
#ifdef	ICMP_SEND
	task_set_ifachange(icmp_task, icmp_ifachange);
#endif	/* ICMP_SEND */
	if ((icmp_task->task_socket = task_get_socket(icmp_task, AF_INET, SOCK_RAW, icmp_proto)) < 0) {
	    task_quit(errno);
	}
	if (!task_create(icmp_task)) {
	    task_quit(EINVAL);
	}

        if (task_set_option(icmp_task,
			    TASKOPTION_NONBLOCKING,
			    TRUE) < 0) {
	    task_quit(errno);
        }
#ifdef	IP_MULTICAST
	/* Disable reception of our own packets */
	if (task_set_option(icmp_task,
			    TASKOPTION_MULTI_LOOP,
			    FALSE) < 0) {
	    task_quit(errno);
	}
	/* Assume for now that only local wire multicasts will be sent */
	if (task_set_option(icmp_task,
			    TASKOPTION_MULTI_TTL,
			    1) < 0) {
	    task_quit(errno);
	}
#endif	/* IP_MULTICAST */
	task_alloc_recv(icmp_task, ICMP_PACKET_MAX);

	/* Do all we can to avoid losing packets */
	if (task_set_option(icmp_task,
			    TASKOPTION_RECVBUF,
			    task_maxpacket) < 0) {
	    task_quit(errno);
	}

#ifdef	ICMP_PROCESS_REDIRECTS
	icmp_block_index = task_block_init(sizeof (struct icmp_redirect_entry), "icmp_redirect_entry");
#endif	/* ICMP_PROCESS_REDIRECTS */
    }
}

#endif	/* defined(PROTO_ICMP) && !defined(KRT_RT_SOCK) */
