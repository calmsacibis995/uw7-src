#ident	"@(#)egp_init.c	1.4"
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
#include "inet.h"
#include "egp.h"

int egp_neighbors;
egp_neighbor egp_neighbor_head = {
    &egp_neighbor_head,
    &egp_neighbor_head
};

u_int egprid_h;
pref_t	egp_preference;			/* Preference for EGP routes */
pref_t	egp_preference2;		/* Alternate preference */
metric_t egp_default_metric;
size_t	egp_pktsize;			/* Maximum packet size to use */
size_t	egp_maxpacket;			/* Maximum packet size system supports */

int	doing_egp = FALSE;
trace	*egp_trace_options;		/* Trace flags from parser */
adv_entry *egp_import_list = NULL;	/* List of EGP advise entries */
adv_entry *egp_export_list = NULL;	/* List of EGP export entries */

u_int egp_reachability[1 << REACH_RATIO];

static int egp_proto = 0;

struct egpstats_t egp_stats =
{0, 0, 0, 0};

const bits egp_states[] =
{
    {NGS_IDLE, "Idle"},
    {NGS_ACQUISITION, "Acquisition"},
    {NGS_DOWN, "Down"},
    {NGS_UP, "Up"},
    {NGS_CEASE, "Cease"},
};

const bits egp_flags[] =
{
    {NGF_SENT_UNSOL, "SentUnsol"},
    {NGF_SENT_POLL, "SentPoll"},
    {NGF_SENT_REPOLL, "SentRepoll"},
    {NGF_RECV_REPOLL, "RecvRepoll"},
    {NGF_RECV_UNSOL, "RecvUnsol"},
    {NGF_PROC_POLL, "ProcPoll"},
    {NGF_DELETE, "Delete"},
    {NGF_WAIT, "Wait"},
    {NGF_GENDEFAULT, "GenDefault"},
    {NGF_POLICY, "Policy"},
    {0, 0}
};

const bits egp_options[] =
{
    {NGO_METRICOUT,	"MetricOut"},
    {NGO_PEERAS,	"PeerAs"},
    {NGO_LOCALAS,	"LocalAs"},
    {NGO_NOGENDEFAULT,	"NoGenDefault"},
    {NGO_LCLADDR,	"LocalAddress"},
    {NGO_PREFERENCE,	"Preference"},
    {NGO_PREFERENCE2,	"Preference2"},
    {NGO_SADDR,		"Saddr"},
    {NGO_GATEWAY,	"Gateway"},
    {NGO_MAXACQUIRE,	"MaxAcquire"},
    {NGO_VERSION,	"Version"},
    {NGO_P1,		"P1"},
    {NGO_P2,		"P2"},
    {NGO_TTL,		"ttl"},
    {NGO_DEFAULTIN,	"ImportDefault"},
    {NGO_DEFAULTOUT,	"ExportDefault"},
    {0, 0}
};

const char *egp_acq_codes[5] =
{
    "Request",
    "Confirm",
    "Refuse",
    "Cease",
    "CeaseAck"};

const char *egp_reach_codes[2] =
{
    "Hello",
    "I-H-U"};

const char *egp_nr_status[3] =
{
    "Indeterminate",
    "Up",
    "Down"};

const char *egp_acq_status[8] =
{
    "Unspecified",
    "ActiveMode",
    "PassiveMode",
    "Insufficient Resources",
    "Prohibited",
    "Going Down",
    "Parameter Problem",
    "Protocol Violation"};

const char *egp_reasons[7] =
{
    "Unspecified",
    "Bad EGP header format",
    "Bad EGP data field format",
    "Reachability info unavailable",
    "Excessive polling rate",
    "No response",
    "Unsupported version"};

/*
 *	Allocate and free neighbor blocks
 */
static block_t egp_block_index;

egp_neighbor *
egp_ngp_alloc(proto)
egp_neighbor *proto;
{
    egp_neighbor *ngp;
    
    if (!egp_block_index) {
	egp_block_index = task_block_init(sizeof (egp_neighbor), "egp_neighbor");
    }

    ngp = (egp_neighbor *) task_block_alloc(egp_block_index);

    if (proto) {
	*ngp = *proto;	/* struct copy */

	/* Resolve duplicate references to sockaddrs */
	if (BIT_TEST(ngp->ng_options, NGO_GATEWAY)) {
	    ngp->ng_gateway = sockdup(ngp->ng_gateway);
	}
	if (BIT_TEST(ngp->ng_options, NGO_SADDR)) {
	    ngp->ng_saddr = sockdup(ngp->ng_saddr);
	}
    }

    /* Init the update packet length */
    ngp->ng_length = sizeof (egp_packet_nr) + IP_MAXHDRLEN;

    return ngp;
}


void
egp_ngp_free __PF1(ngp, egp_neighbor *)
{

    trace_freeup(ngp->ng_trace_options);
    if (ngp->ng_addr) {
	sockfree(ngp->ng_addr);
    }
    if (ngp->ng_paddr) {
	sockfree(ngp->ng_paddr);
    }
    if (ngp->ng_saddr) {
	sockfree(ngp->ng_saddr);
    }
    if (ngp->ng_ifap) {
	IFA_FREE(ngp->ng_ifap);
    }
    if (ngp->ng_lcladdr) {
	(void) ifae_free(ngp->ng_lcladdr);
    }

    task_block_free(egp_block_index, (void_t) ngp);
}


/*
 *	Bind and unbind neighbor and interface
 */
static void
egp_ngp_ifa_unbind __PF1(ngp, egp_neighbor *)
{
    IFA_FREE(ngp->ng_ifap);
    ngp->ng_ifap = (if_addr *) 0;

    if (ngp->ng_task) {
	/* Reset the socket */
	task_close(ngp->ng_task);
    }
    
}


int
egp_ngp_ifa_bind __PF2(ngp, egp_neighbor *,
		       ifap, if_addr *)
{
    int s;
    int local = if_withdst(ngp->ng_addr) ? TRUE : FALSE;
    task *tp = ngp->ng_task;

    /* Allocate a socket for this peer */
    if ((s = task_get_socket(tp, AF_INET, SOCK_RAW, tp->task_proto)) < 0) {
	return FALSE;
    }

    /* Free the old one if present */
    if (ngp->ng_ifap) {
	egp_ngp_ifa_unbind(ngp);
    }
    
    /* Allocate the address pointer */
    IFA_ALLOC(ngp->ng_ifap = ifap);

    /* Bind the socket to the task */
    task_set_recv(ngp->ng_task, egp_recv);
    task_set_socket(tp, s);

    /* Set the kernel receive buffer size to be able to receive as big a packet as possible */
    if (task_maxpacket < egp_pktsize) {
	trace_log_tf(ngp->ng_trace_options,
		     0,
		     LOG_ERR,
		     ("egp_ngp_ifa_bind: Can not allocate kernel buffer space for packet size of %d",
		      egp_pktsize));
	task_quit(EINVAL);
    }
    egp_maxpacket = MIN(task_maxpacket, IP_MAXPACKET);
    if (task_set_option(tp,
			TASKOPTION_RECVBUF,
			egp_maxpacket) < 0) {
	task_quit(errno);
    }

    /* Set the kernel send buffer size for our estimated max packet size */
    ngp->ng_send_size = egp_pktsize;
    if (task_set_option(tp,
			TASKOPTION_SENDBUF,
			egp_pktsize) < 0) {
	task_quit(errno);
    }
		
    if (task_set_option(tp,
			TASKOPTION_NONBLOCKING,
			TRUE) < 0) {
	task_quit(errno);
    }

    /* Set the local address of the socket */
    if (task_addr_local(tp, ngp->ng_ifap->ifa_addr_local)) {
	task_quit(errno);
    }

    /* Set the remote address of the socket */
    if (task_connect(tp)) {
	trace_log_tf(ngp->ng_trace_options,
		     0,
		     LOG_ERR,
		     ("egp_ngp_ifa_bind: task_connect %A: %m",
		      ngp->ng_addr));
	task_quit(errno);
    }

    /* Set the source address if necessary */
    if (!BIT_TEST(ngp->ng_options, NGO_SADDR)) {
	if (ngp->ng_saddr) {
	    sockfree(ngp->ng_saddr);
	}
	ngp->ng_saddr = sockdup(sockbuild_in(0,
					     inet_net_natural(ngp->ng_addr)));
    }

    /* Set the don't route flag */
    (void) task_set_option(tp,
			   TASKOPTION_DONTROUTE,
			   local);

    /* Set the TTL (if supported) */
    (void) task_set_option(tp,
			   TASKOPTION_TTL,
			   (ngp->ng_ttl ? ngp->ng_ttl : (local ? 1 : 255)));

    trace_log_tf(ngp->ng_trace_options,
		 0,
		 LOG_INFO,
		 ("egp_event_start: neighbor %s AS %d: %s interface %A (%s)",
		  ngp->ng_name,
		  ngp->ng_peer_as,
		  local ? "local" : "remote",
		  ngp->ng_ifap->ifa_addr,
		  ngp->ng_ifap->ifa_link->ifl_name));

    return TRUE;
}


/*
 *	Delete a neighbor
 */
static void
egp_ngp_delete __PF1(ngp, egp_neighbor *)
{
    egp_neighbor *ngp2;
	
    if (ngp->ng_task) {
	/* Free the routing table resources */
	egp_rt_terminate(ngp);

	trace_freeup(ngp->ng_task->task_trace);

	/* Delete the task */
	task_delete(ngp->ng_task);
	ngp->ng_task = (task *) 0;
	ngp->ng_timer_t1 = ngp->ng_timer_t2 = ngp->ng_timer_t3 = (task_timer *) 0;
    }

    /* Delete this neighbor from the list of neighbors */
    egp_neighbors--;
    REMQUE(ngp);
#if	defined(PROTO_SNMP)
    egp_sort_neighbors(ngp);
#endif	/* defined(PROTO_SNMP) */

    /* Now see if we were superceeded and cause him to wake up */
    EGP_LIST(ngp2) {
	if (BIT_TEST(ngp2->ng_flags, NGF_WAIT)
	    && sockaddrcmp_in(ngp->ng_addr, ngp2->ng_addr)) {
	    BIT_RESET(ngp2->ng_flags, NGF_WAIT);
	    egp_event_t3(ngp2->ng_timer_t3, (time_t) 0);
	    break;
	}
    } EGP_LIST_END(ngp2);

    /* Release this interface */
    if (ngp->ng_ifap) {
	egp_ngp_ifa_unbind(ngp);
    }

    /* Release the memory */
    egp_ngp_free(ngp);
}


/*
 *	Check if interface has gone away or if neighbor is scheduled
 *	for deletion.
 */
void
egp_ngp_idlecheck __PF1(ngp, egp_neighbor *)
{
    if (ngp->ng_ifap
	&& !BIT_TEST(ngp->ng_ifap->ifa_state, IFS_UP)) {
	/* Our interface is gone */

	egp_ngp_ifa_unbind(ngp);
    }

    if (BIT_TEST(ngp->ng_flags, NGF_DELETE)) {
	/* Scheduled to go away */

	egp_ngp_delete(ngp);
    }
}


/*
 *	Initialize the reachability structure
 */
static void
egp_init_reachability __PF0(void)
{
    int reach, mask, bit, n_bits;

    for (reach = 0; reach < 1 << REACH_RATIO; reach++) {
	n_bits = 0;
	for (bit = 0; bit < REACH_RATIO; bit++) {
	    mask = 1 << bit;
	    if (reach & mask) {
		n_bits++;
	    }
	}
	egp_reachability[reach] = n_bits;
    }
}


static void
egp_tsi_dump __PF4(fp, FILE *,
		   rth, rt_head *,
		   data, void_t,
		   pfx, const char *)
{
    egp_neighbor *ngp = (egp_neighbor *) data;
    egp_route *rp;

    rttsi_get(rth, ngp->ng_task->task_rtbit, (byte *) &rp);
    
    if (rp) {
	egp_dist *dp = rp->route_dist;

	(void) fprintf(fp, "%sEGP %s distance %u gateway %A\n",
		       pfx,
		       ngp->ng_name,
		       dp->dist_distance,
		       sockbuild_in(0, dp->dist_gwentry->gwe_addr.s_addr));
    }
    
    return;
}


/**/


/*
 *	Cleanup before re-parse
 */
static void
egp_ngp_cleanup __PF1(tp, task *)
{
    egp_neighbor *ngp = (egp_neighbor *) tp->task_data;

    BIT_SET(ngp->ng_flags, NGF_DELETE);

    ngp->ng_import = (adv_entry *) 0;
    ngp->ng_export = (adv_entry *) 0;

    /* Indicate we need to get policy */
    BIT_RESET(ngp->ng_flags, NGF_POLICY);

    trace_freeup(ngp->ng_trace_options);
    trace_freeup(tp->task_trace);
}


static void
egp_ngp_terminate __PF1(tp, task *)
{
    egp_neighbor *ngp = (egp_neighbor *) tp->task_data;

    BIT_SET(ngp->ng_flags, NGF_DELETE);

    egp_event_stop(ngp, EGP_STATUS_GOINGDOWN);
}


/*
 *	Re-init after re-parse
 */
static void
egp_ngp_reinit __PF1(tp, task *)
{
    egp_neighbor *ngp = (egp_neighbor *) tp->task_data;

    trace_inherit(ngp->ng_trace_options, egp_trace_options);
    trace_set(tp->task_trace, ngp->ng_trace_options);

    if (!doing_egp || BIT_TEST(ngp->ng_flags, NGF_DELETE)) {
	BIT_SET(ngp->ng_flags, NGF_DELETE);
	egp_event_stop(ngp, EGP_STATUS_GOINGDOWN);
    } else {
	/* Setup policy */
	egp_rt_reinit(ngp);
#ifdef	notdef
	switch (ngp->ng_state) {
	case NGS_IDLE:
	    if (!BIT_TEST(ngp->ng_flags, NGF_WAIT)) {
		egp_event_start(tp);
	    }
	    break;

	case NGS_ACQUISITION:
	case NGS_DOWN:
	case NGS_UP:
	case NGS_CEASE:
	    break;
	}
#endif	/* notdef */
    }
}


/*
 *	Deal with a routing table change
 */
static void
egp_ngp_flash __PF2(tp, task *,
		    change_list, rt_list *)
{
    egp_neighbor *ngp = (egp_neighbor *) tp->task_data;

    if (ngp->ng_peer_as) {
	/* If we have an AS number, re-evaluate policy */
	egp_rt_policy(ngp, change_list);
    }
}


/*
 *	Deal with new policy, same as flash except for message
 */
static void
egp_ngp_newpolicy __PF2(tp, task *,
			change_list, rt_list *)
{
    egp_neighbor *ngp = (egp_neighbor *) tp->task_data;

    if (!BIT_TEST(ngp->ng_flags, NGF_WAIT)
	&& !ngp->ng_ifap) {
	/* Indicate we are having problems finding an interface */

	trace_log_tf(ngp->ng_trace_options,
		     0,
		     LOG_INFO,
		     ("egp_ngp_newpolicy: neighbor %s AS %d: No acceptable interface",
		      ngp->ng_name,
		      ngp->ng_peer_as));

    }
    
    if (ngp->ng_peer_as) {
	/* If we have an AS number, re-evaluate policy */
	egp_rt_policy(ngp, change_list);
    }
}


/*
 *	Select the correct interface for this peer
 */
if_addr *
egp_ngp_ifa_select __PF1(ngp, egp_neighbor *)
{
    register if_addr *ifap;

    if (BIT_TEST(ngp->ng_options, NGO_GATEWAY)) {
	/* No shared net */

	ifap = if_withdst(ngp->ng_gateway);
	if (ifap
	    && BIT_TEST(ngp->ng_options, NGO_LCLADDR)
	    && !(ifap = if_withlcladdr(ngp->ng_lcladdr->ifae_addr, FALSE))) {
	    /* Do not have specified local address */

	    ifap = (if_addr *) 0;
	}
    } else {
	/* Must have a shared net */
	
	ifap = if_withdst(ngp->ng_addr);
	if (ifap
	    && BIT_TEST(ngp->ng_options, NGO_LCLADDR)
	    && ifap->ifa_addrent_local != ngp->ng_lcladdr) {
	    /* This does not match specified local address */

	    ifap = (if_addr *) 0;
	}
    }

    return ifap;
}


/*
 *	Deal with an interface status change
 */
static void
egp_ngp_ifachange __PF2(tp, task *,
		       ifap, if_addr *)
{
    egp_neighbor *ngp = (egp_neighbor *)tp->task_data;
    
    if (socktype(ifap->ifa_addr) != AF_INET) {
	return;
    }
    
    switch (ifap->ifa_change) {
    case IFC_NOCHANGE:
    case IFC_ADD:
	if (BIT_TEST(ifap->ifa_state, IFS_UP)) {
	    /* Interface is now up */
	    
	Up:
	    if (!BIT_TEST(ngp->ng_flags, NGF_WAIT)
		&& ifap == egp_ngp_ifa_select(ngp)) {
		/* A better interface */

	    Bind:
		/* Bind to the new interface */
		if (egp_ngp_ifa_bind(ngp, ifap)) {

		    /* Restart the protocol with the new interface */
		    egp_event_start(ngp->ng_task);
		} else {
		    /* Problem with new socket */
	    
		    goto Stop;
		}
	    }
	}
	break;

    case IFC_DELETE:
	break;
	
    case IFC_DELETE|IFC_UPDOWN:
    Down:
	/* Our current interface is down */
	    
	if (ngp->ng_ifap == ifap) {
	    if_addr *new_ifap = egp_ngp_ifa_select(ngp);
		
	    if (new_ifap) {
		/* Have another interface */

		/* Bind to the new interface */
		if (egp_ngp_ifa_bind(ngp, new_ifap)) {

		    /* Restart the protocol with the new interface */
		    egp_event_start(ngp->ng_task);
		} else {
		    /* There was a problem with the new socket */

		    goto Stop;
		}
	    } else {
		/* No new interface */

		/* Indicate there is a problem */
		trace_log_tf(ngp->ng_trace_options,
			     0,
			     LOG_INFO,
			     ("egp_event_start: neighbor %s AS %d: No acceptable interface",
			      ngp->ng_name,
			      ngp->ng_peer_as));

	    Stop:
		/* Make sure packets will reach him */
		(void) task_set_option(ngp->ng_task,
				       TASKOPTION_DONTROUTE,
				       FALSE);
		(void) task_set_option(ngp->ng_task,
				       TASKOPTION_TTL,
				       255);

		/* Bring it down */
		egp_event_stop(ngp, EGP_STATUS_GOINGDOWN);
	    }
	}
	break;

    default:
	/* Something has changed */
	if (BIT_TEST(ifap->ifa_change, IFC_UPDOWN)) {
	    /* State transition */

	    if (BIT_TEST(ifap->ifa_state, IFS_UP)) {
		/* Now up */

		goto Up;
	    } else {
		/* Now down */

		goto Down;
	    }
	}

	if (BIT_TEST(ifap->ifa_change, IFC_NETMASK|IFC_ADDR)) {
	    if (!BIT_TEST(ngp->ng_flags, NGF_WAIT)
		&& ifap == egp_ngp_ifa_select(ngp)
		&& (!ngp->ng_ifap
		    || ngp->ng_ifap != ifap)) {
		/* A better interface */

		goto Bind;
	    }
	}
	/* METRIC change - nothing to do */
	/* BROADCAST change - we don't use broadcast */
	/* MTU change - we don't care, we fragment */
	/* SEL change - homey don't do ISO */
	break;
    }
}


/*
 *	Terminate signal received for a task
 */
static void
egp_ngp_init __PF1(ngp, egp_neighbor *)
{

    /* If LocalAS is not specified, default to global value */
    if (!BIT_TEST(ngp->ng_options, NGO_LOCALAS)) {
	ngp->ng_local_as = inet_autonomous_system;
    }
    if (!BIT_TEST(ngp->ng_options, NGO_PREFERENCE)) {
	ngp->ng_preference = egp_preference;
    }
    if (!BIT_TEST(ngp->ng_options, NGO_PREFERENCE2)) {
	ngp->ng_preference2 = egp_preference2;
    }
    if (!BIT_TEST(ngp->ng_options, NGO_P1)) {
	ngp->ng_P1 = EGP_P1;
    }
    if (!BIT_TEST(ngp->ng_options, NGO_P2)) {
	ngp->ng_P2 = EGP_P2;
    }
    ngp->ng_state = NGS_IDLE;
    ngp->ng_V = BIT_TEST(ngp->ng_options, NGO_VERSION) ? ngp->ng_version : EGPVER;
    ngp->ng_S = 1;
    ngp->ng_T1 = ngp->ng_P1 + HELLOMARGIN;
    ngp->ng_T2 = ((ngp->ng_P2 - 1) / ngp->ng_P1 + 1) * ngp->ng_T1;
    ngp->ng_hello_rate.rate_min = ngp->ng_P1;
    ngp->ng_poll_rate.rate_min = ngp->ng_P2;
    ngp->ng_M = EGP_STATUS_ACTIVE;
#ifdef	PROTO_SNMP
    ngp->ng_stats.trigger = EGP_TRIGGER_STOP;
#endif	/* PROTO_SNMP */

    trace_inherit(ngp->ng_trace_options, egp_trace_options);

    ngp->ng_task = task_alloc("EGP",
			      TASKPRI_EXTPROTO,
			      ngp->ng_trace_options);
    ngp->ng_task->task_proto = egp_proto;
    ngp->ng_task->task_rtproto = RTPROTO_EGP;
    ngp->ng_task->task_addr = sockdup(ngp->ng_addr);
    ngp->ng_task->task_data = (void_t) ngp;
    task_set_terminate(ngp->ng_task, egp_ngp_terminate);
    task_set_cleanup(ngp->ng_task, egp_ngp_cleanup);
    task_set_dump(ngp->ng_task, egp_ngp_dump);
    task_set_reinit(ngp->ng_task, egp_ngp_reinit);
    task_set_flash(ngp->ng_task, egp_ngp_flash);
    task_set_newpolicy(ngp->ng_task, egp_ngp_newpolicy);
    task_set_ifachange(ngp->ng_task, egp_ngp_ifachange);
    ngp->ng_task->task_rtbit = rtbit_alloc(ngp->ng_task,
					   FALSE,
					   sizeof (egp_route *),
					   (void_t) ngp,
					   egp_tsi_dump);
    BIT_SET(ngp->ng_task->task_flags, TASKF_LOWPRIO);
    if (!task_create(ngp->ng_task)) {
	task_quit(EINVAL);
    }
		
    ngp->ng_timer_t1 = task_timer_create(ngp->ng_task,
					 "t1",
					 0,
					 (time_t) 0,
					 (time_t) 0,
					 egp_event_t1,
					 (void_t) 0);

    ngp->ng_timer_t2 = task_timer_create(ngp->ng_task,
					 "t2",
					 0,
					 (time_t) 0,
					 (time_t) 0,
					 egp_event_t2,
					 (void_t) 0);

    ngp->ng_timer_t3 = task_timer_create(ngp->ng_task,
					 "t3",
					 (flag_t) 0,
					 (time_t) 0,
					 (time_t) 0,
					 egp_event_t3,
					 (void_t) 0);
}

/**/

/*
 * Init static variables
 */
void
egp_var_init()
{
    egp_default_metric = EGP_DISTANCE_MAX;
    egp_preference = RTPREF_EGP;
    egp_preference2 = 0;
    egp_pktsize = EGP_PKTSIZE;
}


/*
 *	Cleanup main EGP task before reparse
 */
/*ARGSUSED*/
static void
egp_cleanup __PF1(tp, task *)
{
    adv_free_list(egp_import_list);
    egp_import_list = (adv_entry *) 0;
    adv_free_list(egp_export_list);
    egp_export_list = (adv_entry *) 0;

    if (tp) {
	trace_freeup(tp->task_trace);
    }
    trace_freeup(egp_trace_options);
}


static void
egp_terminate __PF1(tp, task *)
{
    /* Clean up */
    egp_cleanup(tp);

    /* And exit */
    task_delete(tp);
}


/*
 *	Initialize EGP socket and task
 */
void
egp_init __PF0(void)
{
    egp_neighbor *ngp;
    static task *egp_task = (task *) 0;

    if (doing_egp) {
	trace_inherit_global(egp_trace_options, egp_trace_types, (flag_t) 0);
	if (egp_task) {
	    trace_set(egp_task->task_trace, egp_trace_options);
	} else {
	    if (!egp_proto) {
		egp_proto = task_get_proto(egp_trace_options,
					   "egp",
					   IPPROTO_EGP);
	    }

	    egp_task = task_alloc("EGP",
				  TASKPRI_EXTPROTO,
				  egp_trace_options);
	    task_set_cleanup(egp_task, egp_cleanup);
	    task_set_terminate(egp_task, egp_terminate);
	    task_set_dump(egp_task, egp_dump);
	    BIT_SET(egp_task->task_flags, TASKF_LOWPRIO);
	    if (!task_create(egp_task)) {
		task_quit(EINVAL);
	    }
#ifdef	PROTO_SNMP
	    egp_mib_init(TRUE);
#endif	/* PROTO_SNMP */
	}

	egp_init_reachability();

	/* Specify the send and receive buffer sizes we need */
	task_alloc_recv(egp_task, egp_pktsize);
	task_alloc_send(egp_task, egp_pktsize);

	EGP_LIST(ngp) {
	    if (!ngp->ng_task) {
		egp_ngp_init(ngp);
	    }
	} EGP_LIST_END(ngp);
    } else {
	/* Delete any neighbors without tasks */
	EGP_LIST(ngp) {
	    if (!ngp->ng_task) {
		egp_ngp_delete((ngp = ngp->ng_back)->ng_forw);
	    }
	} EGP_LIST_END(ngp);

	egp_cleanup((task *) 0);
#ifdef	PROTO_SNMP
	egp_mib_init(FALSE);
#endif	/* PROTO_SNMP */
	if (egp_task) {
	    task_delete(egp_task);
	    egp_task = (task *) 0;
	}
    }
}
