#ident	"@(#)egp.c	1.4"
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

PROTOTYPE(egp_event_up,
	  static void,
	  (egp_neighbor *));
PROTOTYPE(egp_event_down,
	  static void,
	  (egp_neighbor *));

/*
 *	Format an EGP network update packet
 */
static void
egp_trace_NR __PF3(ngp, egp_neighbor *,
		   nr, egp_packet_nr *,
		   nr_length, size_t)
{
    u_int gateways;
    byte *nr_ptr, *nr_end;
    sockaddr_un *gateway;

    gateway = sockdup(sockbuild_in(0, nr->en_net));

    if (!inet_class_valid_byte((byte *) &nr->en_net)) {
	trace_only_tf(ngp->ng_trace_options,
		      TRC_NOSTAMP,
		      ("\tInvalid shared network class\n"));
	return;
    }
    
    nr_ptr = (byte *) nr + sizeof (egp_packet_nr);
    nr_end = (byte *) nr + nr_length;

    trace_only_tf(ngp->ng_trace_options,
		  TRC_NOSTAMP,
		  ("\tnet %A - %d interior gateways, %d exterior gateways",
		   gateway,
		   nr->en_igw,
		   nr->en_egw));

    gateways = nr->en_igw + nr->en_egw;

    while (gateways--) {
	int distances;
	byte *cp;

	cp = (byte *) &sock2ip(gateway);
	switch (inet_class_of_byte((byte *) &nr->en_net)) {
	case INET_CLASSC_A:
	    cp += 1;
	    *cp++ = *nr_ptr++;
	    *cp++ = *nr_ptr++;
	    *cp++ = *nr_ptr++;
	    break;
	    
	case INET_CLASSC_B:
	    cp += 2;
	    *cp++ = *nr_ptr++;
	    *cp++ = *nr_ptr++;
	    break;
	    
	case INET_CLASSC_C:
	    cp += 3;
	    *cp++ = *nr_ptr++;
	    break;
	    
	case INET_CLASSC_E:
	case INET_CLASSC_C_SHARP:
	    *(cp += 2) &= 0xf0;
	    if (*nr_ptr & 0xf0) {
		/* XXX - How to recover sync? */
	    }
	    *cp++ = *nr_ptr++;
	    *cp++ = *nr_ptr++;
	}

	distances = *nr_ptr++;

	trace_only_tf(ngp->ng_trace_options,
		      TRC_NOSTAMP,
		      ("\t    %s gateway %A, %d distances",
		       gateways < nr->en_egw ? "exterior" : "interior",
		       gateway,
		       distances));

	while (distances--) {
	    int distance, networks;
	    int n_routes = 0;

	    distance = *nr_ptr++;
	    networks = *nr_ptr++;

	    trace_only_tf(ngp->ng_trace_options,
			  TRC_NOSTAMP,
			  ("\t\tdistance %d, %d networks",
			   distance,
			   networks));

	    while (networks--) {
		struct in_addr network;

		network.s_addr = 0;
		cp = (byte *) &network.s_addr;
		switch (inet_class_of_byte(nr_ptr)) {
		default:
		    /* Let default fall through to Class C */

		case INET_CLASSC_E:
		case INET_CLASSC_C_SHARP:
		    /* Treat class E the same as a Class C for parsing */
		    /* XXX - Should provide some indication of a problem */
		    /* Fall through */

		case INET_CLASSC_C:
		    *cp++ = *nr_ptr++;
		    /* Fall through */
		    
		case INET_CLASSC_B:
		    *cp++ = *nr_ptr++;
		    /* Fall through */
		    
		case INET_CLASSC_A:
		    *cp++ = *nr_ptr++;
		    break;
		}

		tracef("%s%-13A",
		       n_routes ? " " : "\t\t  ",
		       sockbuild_in(0, network.s_addr));
		if (++n_routes == 6) {
		    n_routes = 0;
		    trace_only_tf(ngp->ng_trace_options,
				  TRC_NOSTAMP,
				  (NULL));
		}
		if (nr_ptr > nr_end) {
		    trace_only_tf(ngp->ng_trace_options,
				  TRC_NOSTAMP,
				  ("\tpremature end of packet\n"));
		    sockfree(gateway);
		    return;
		}
	    }

	    if (n_routes) {
		trace_only_tf(ngp->ng_trace_options,
			      TRC_NOSTAMP,
			      (NULL));
	    }
	}
    }
    trace_only_tf(ngp->ng_trace_options,
		  0,
		  ("end of packet"));
    sockfree(gateway);
    return;
}


static const flag_t egp_trace_masks[EGP_PKT_MAX] = {
    TR_ALL,			/* 0 - Invalid */
    TR_EGP_DETAIL_UPDATE,	/* 1 - NR */
    TR_EGP_DETAIL_UPDATE,	/* 2 - POLL */
    TR_EGP_DETAIL_ACQUIRE,	/* 3 - ACQUIRE */
    TR_ALL,			/* 4 - Invalid */
    TR_EGP_DETAIL_HELLO,	/* 5 - HELLO */
    TR_ALL,			/* 6 - Invalid */
    TR_ALL,			/* 7 - Invalid */
    TR_ALL,			/* 8 - ERROR */
} ;

const bits egp_trace_types[] = {
    { TR_DETAIL,	"detail packets" },
    { TR_DETAIL_SEND,	"detail send packets" },
    { TR_DETAIL_RECV,	"detail recv packets" },
    { TR_PACKET,	"packets" },
    { TR_PACKET_SEND,	"send packets" },
    { TR_PACKET_RECV,	"recv packets" },
    { TR_DETAIL_1,	"detail hello" },
    { TR_DETAIL_SEND_1,	"detail send hello" },
    { TR_DETAIL_RECV_1,	"detail recv hello" },
    { TR_PACKET_1,	"hello" },
    { TR_PACKET_SEND_1,	"send hello" },
    { TR_PACKET_RECV_1,	"recv hello" },
    { TR_DETAIL_2,	"detail acquire" },
    { TR_DETAIL_SEND_2,	"detail send acquire" },
    { TR_DETAIL_RECV_2,	"detail recv acquire" },
    { TR_PACKET_2,	"acquire" },
    { TR_PACKET_SEND_2,	"send acquire" },
    { TR_PACKET_RECV_2,	"recv acquire" },
    { TR_DETAIL_3,	"detail update" },
    { TR_DETAIL_SEND_3,	"detail send update" },
    { TR_DETAIL_RECV_3,	"detail recv update" },
    { TR_PACKET_3,	"update" },
    { TR_PACKET_SEND_3,	"send update" },
    { TR_PACKET_RECV_3,	"recv update" },
    { 0, NULL }
};

/*
 * Trace EGP packet
 */
static void
egp_trace __PF6(ngp, egp_neighbor *,
		comment, const char *,
		send_flag, int,
		egp, egp_packet *,
		length, size_t,
		detail, int)
{
    egp_packet *ep;
    int reason;
    char packet_status;
    const char *type = (char *) 0;
    const char *status = (char *) 0;
    const char *code = (char *) 0;
    static const char *no_codes[1] =
    {"0"};
    static struct {
	const char *et_type;
	const int et_ncodes;
	const char **et_codes;
	const int et_nstatus;
	const char **et_status;
    } const egp_types[9] = {
	{ "Invalid", -1, (const char **) 0, -1, (const char **) 0 },	/* 0 - Error */
	{ "Update", 0, no_codes, 3, egp_nr_status },			/* 1 - Nets Reachable */
	{ "Poll", 0, no_codes, 3, egp_nr_status },				/* 2 - Poll */
	{ "Acquire", 5, egp_acq_codes, 7, egp_acq_status },			/* 3 - Neighbor Aquisition */
	{ "Invalid", -1, (const char **) 0, -1, (const char **) 0 },	/* 4 - Error */
	{ "Neighbor", 2, egp_reach_codes, 3, egp_nr_status },		/* 5 - Neighbor Reachability */
	{ "Invalid", -1, (const char **) 0, -1, (const char **) 0 },	/* 6 - Error */
	{ "Invalid", -1, (const char **) 0, -1, (const char **) 0 },	/* 7 - Error */
	{ "ERROR", -1, (const char **) 0, 3, egp_nr_status }		/* 8 - Error packet */
    };

    trace_only_tf(ngp->ng_trace_options,
		  0,
		  ("%s %A -> %A length %d",
		   comment,
		   send_flag ? ngp->ng_ifap->ifa_addr_local : ngp->ng_addr,
		   send_flag ? ngp->ng_addr : ngp->ng_ifap->ifa_addr_local,
		   length));

    if (egp->header.egp_type <= EGP_PKT_ERROR) {
	type = egp_types[egp->header.egp_type].et_type;
	if ((short) egp->header.egp_code <= egp_types[egp->header.egp_type].et_ncodes) {
	    code = egp_types[egp->header.egp_type].et_codes[egp->header.egp_code];
	} else {
	    if (egp->header.egp_code == 0) {
		code = "";
	    } else {
		code = "Invalid";
	    }
	}
	packet_status = egp->header.egp_status % EGP_STATUS_UNSOLICITED;
	if (packet_status <= egp_types[egp->header.egp_type].et_nstatus) {
	    status = egp_types[egp->header.egp_type].et_status[packet_status];
	} else {
	    status = "Invalid";
	}
    } else {
	type = "Invalid";
    }
    tracef("%s vers %d, type %s(%d), code %s(%d), status %s(%d)%s, AS %d, id %d",
	   comment,
	   egp->header.egp_ver,
	   type,
	   egp->header.egp_type,
	   code,
	   egp->header.egp_code,
	   status,
	   egp->header.egp_status,
	   BIT_TEST(egp->header.egp_status, EGP_STATUS_UNSOLICITED) ? " Unsolicited" : "",
	   ntohs(egp->header.egp_system),
	   ntohs(egp->header.egp_id));
    if (length >= sizeof (egp_packet_header)) {
	switch (egp->header.egp_type) {
	case EGP_PKT_ACQUIRE:
	    if (length == sizeof (egp_packet_acquire)) {
		trace_only_tf(ngp->ng_trace_options,
			      0,
			      (", hello %d, poll %d",
			       ntohs(egp->acquire.ea_hint),
			       ntohs(egp->acquire.ea_pint)));
	    }
	    break;

	case EGP_PKT_POLL:
	    if (length >= sizeof (egp_packet_poll)) {
		trace_only_tf(ngp->ng_trace_options,
			      0,
			      (", src net %A",
			       sockbuild_in(0, egp->poll.ep_net)));
	    }
	    break;

	case EGP_PKT_NR:
	    if (length >= sizeof (egp_packet_nr)) {
		trace_only_tf(ngp->ng_trace_options,
			      0,
			      (", #int %d, #ext %d, src net %A",
			       egp->nr.en_igw,
			       egp->nr.en_egw,
			       sockbuild_in(0, egp->nr.en_net)));
	    } else if (length >= (sizeof (egp_packet_nr) - sizeof (struct in_addr))) {
		trace_only_tf(ngp->ng_trace_options,
			      0,
			      (", #int %d, #ext %d",
			       egp->nr.en_igw,
			       egp->nr.en_egw));
	    }
	    if (length > sizeof (egp_packet_nr) && detail) {
		egp_trace_NR(ngp, &egp->nr, length);
	    }
	    break;

	case EGP_PKT_ERROR:
	    reason = ntohs(egp->error.ee_rsn);
	    if (reason > EGP_REASON_MAX) {
		trace_only_tf(ngp->ng_trace_options,
			      0,
			      (", error %d (invalid)",
			       reason));
	    } else {
		trace_only_tf(ngp->ng_trace_options,
			      0,
			      (", error: %s(%d)",
			       egp_reasons[reason],
			       reason));
	    }
	    ep = (egp_packet *) ((void_t) egp->error.ee_egphd);
	    if (length >= sizeof (egp_packet_error)) {
		char e_comment[MAXHOSTNAMELENGTH];

		(void) strcpy(e_comment, comment);
		(void) strcat(e_comment, " ERROR");
		egp_trace(ngp,
			  e_comment,
			  send_flag,
			  ep,
			  sizeof (egp_packet_header),
			  FALSE);
	    }
	    break;

	case EGP_PKT_HELLO:
	default:
	    trace_only_tf(ngp->ng_trace_options,
			  0,
			  (NULL));
	}
    }
    trace_only_tf(ngp->ng_trace_options,
		  0,
		  (NULL));
    return;
}


#define	egp_msg_event(func, ngp, ev) trace_tp(ngp->ng_task, \
					      TR_STATE, \
					      0, \
					      ("%s: neighbor %s version %d state %s EVENT %s", \
					       (func), \
					       (ngp)->ng_name, \
					       (ngp)->ng_V, \
					       trace_state(egp_states, (ngp)->ng_state), \
					       (ev)))

#define	egp_msg_state(func, ngp, st) trace_tp(ngp->ng_task, \
					      TR_STATE, \
					      0, \
					      ("%s: neighbor %s version %d state %s TRANSITION to %s", \
 					       (func), \
					       (ngp)->ng_name, \
					       (ngp)->ng_V, \
					       trace_state(egp_states, (ngp)->ng_state), \
					       trace_state(egp_states, (st))))

#define	egp_msg_confused(func, ngp, ev) trace_tp(ngp->ng_task, \
						 TR_ALL, \
						 0, \
						 ("%s: neighbor %s version %d event %s SHOULD NOT OCCUR in state %s", \
						  (func), \
						  (ngp)->ng_name, \
						  (ngp)->ng_V, \
						  (ev), \
						  trace_state(egp_states, (ngp)->ng_state)))


/*
 *	Set new version and print a message
 */
static void
egp_set_version __PF2(ngp, egp_neighbor *,
		      egp_version, byte)
{
    trace_log_tp(ngp->ng_task,
		 0,
		 LOG_INFO,
		 ("egp_set_version: neighbor %s version %d state %s set version %d",
		  ngp->ng_name,
		  ngp->ng_V,
		  trace_state(egp_states, ngp->ng_state),
		  egp_version));
    ngp->ng_V = egp_version;
}


/*
 *	Routines to deal with maxacquire limits
 */
static int
egp_group_acquired __PF1(ngp, egp_neighbor *)
{
    int acquired = 0;
    egp_neighbor *tngp;

    for (tngp = ngp->ng_gr_head; tngp != &egp_neighbor_head; tngp = tngp->ng_forw) {
	if (tngp->ng_gr_index != ngp->ng_gr_index) {
	    break;
	}
	if (tngp->ng_state == NGS_UP) {
	    acquired++;
	}
    }

    return acquired;
}


static void
egp_group_checkmax __PF1(ngp, egp_neighbor *)
{
    u_int acquired = 0;
    u_int acquire;
    egp_neighbor *tngp;

    acquire = ngp->ng_gr_head->ng_gr_acquire;

    for (tngp = ngp->ng_gr_head; tngp != &egp_neighbor_head; tngp = tngp->ng_forw) {
	if (tngp->ng_gr_index != ngp->ng_gr_index) {
	    break;
	}
#ifdef	notdef
	/* DEBUG */
	trace_log_tf(ngp->ng_trace_options,
		     0,
		     LOG_WARNING,
		     ("egp_group_checkmax: neighbor %s state %s acquired %d acquire %d",
		      tngp->ng_name,
		      trace_state(egp_states, tngp->ng_state),
		      acquired,
		      acquire));
#endif	/* notdef */
	switch (tngp->ng_state) {
	case NGS_IDLE:
	case NGS_CEASE:
	    break;

	case NGS_UP:
	case NGS_ACQUISITION:
	case NGS_DOWN:
	    if (acquired >= acquire) {
		egp_event_stop(tngp, EGP_STATUS_GOINGDOWN);
	    }
	    if (tngp->ng_state == NGS_UP) {
		acquired++;
	    }
	    break;
	}
    }
}


/*
 * egp_check_neighborLoss() handles the loss of a neighbor.  It deletes any
 * routes in the routing table pointing at this gateway, calls egp_check_as()
 * to determine if we lost all neighbors for this AS.
 */
static void
egp_check_neighborLoss __PF1(ngp, egp_neighbor *)
{
    int changes = 0;

    switch (ngp->ng_state) {
    case NGS_IDLE:
    case NGS_ACQUISITION:
    case NGS_DOWN:
    case NGS_CEASE:
	break;
	
    case NGS_UP:
#ifdef	PROTO_ISODE_SNMP
	egp_trap_neighbor_loss(ngp);
#endif	/* PROTO_ISODE_SNMP */

    	{
	    register egp_neighbor *ngp2;

	    EGP_LIST(ngp2) {
		if (ngp2 != ngp &&
		    ngp2->ng_peer_as == ngp->ng_peer_as &&
		    (ngp2->ng_state == NGS_UP || ngp2->ng_state == NGS_DOWN)) {
		    break;
		}
	    } EGP_LIST_END(ngp2);

	    if (!ngp2) {
		trace_log_tf(ngp->ng_trace_options,
			     0,
			     LOG_WARNING,
			     ("egp_check_neighborloss: lost all neighbors to AS %d",
			      ngp->ng_peer_as));
	    }
	    changes++;
	}

	/* Delete routes for down gateway */
        {
	    register rt_entry *rt;

	    rt_open(ngp->ng_task);

	    RTQ_LIST(&ngp->ng_gw.gw_rtq, rt) {
		rt_delete(rt);
		changes++;
	    } RTQ_LIST_END(&ngp->ng_gw.gw_rtq, rt) ;

	    rt_close(ngp->ng_task, &ngp->ng_gw, changes, NULL);
	}


	/* Remove our request for default */
	if (BIT_TEST(ngp->ng_flags, NGF_GENDEFAULT)) {
	    rt_default_delete();
	    BIT_RESET(ngp->ng_flags, NGF_GENDEFAULT);
	}

	if (changes) {
	    trace_tp(ngp->ng_task,
		     TR_ROUTE,
		     0,
		     ("egp_check_neighborLoss: above changes due to loss of neighbor %s",
		      ngp->ng_name));
	}
    }
}


/*
 *	Routines to change state
 */
static void
egp_state_idle __PF1(ngp, egp_neighbor *)
{

    egp_msg_state("egp_state_idle", ngp, NGS_IDLE);

    egp_check_neighborLoss(ngp);

    if (ngp->ng_state == NGS_UP) {
	ngp->ng_stats.statedowns++;
    }
    ngp->ng_state = NGS_IDLE;

    egp_ngp_idlecheck(ngp);
}


static void
egp_state_acquisition __PF1(ngp, egp_neighbor *)
{

    egp_msg_state("egp_state_acquisition", ngp, NGS_ACQUISITION);

    egp_check_neighborLoss(ngp);

    if (ngp->ng_state == NGS_UP) {
	ngp->ng_stats.statedowns++;
    }
    ngp->ng_state = NGS_ACQUISITION;
    ngp->ng_status = 0;
}


/*
 * egp_state_down() Set down state.
 */
static void
egp_state_down __PF1(ngp, egp_neighbor *)
{

    egp_msg_state("egp_state_down", ngp, NGS_DOWN);

    egp_check_neighborLoss(ngp);

    if (ngp->ng_state != NGS_UP) {
	trace_log_tf(ngp->ng_trace_options,
		     0,
		     LOG_WARNING,
		     ("egp_state_down: acquired neighbor %s AS %d in %s",
		      ngp->ng_name,
		      ngp->ng_peer_as,
		      egp_acq_status[ngp->ng_M]));
    }
    if (ngp->ng_state == NGS_UP) {
	ngp->ng_stats.statedowns++;
    }
    ngp->ng_state = NGS_DOWN;

}


static void
egp_state_up __PF1(ngp, egp_neighbor *)
{

    egp_msg_state("egp_state_up", ngp, NGS_UP);

    ngp->ng_state = NGS_UP;

    egp_group_checkmax(ngp);

    ngp->ng_stats.stateups++;
}


/*
 * egp_state_cease() initiates the sending of an egp neighbor cease.
 */
static void
egp_state_cease __PF1(ngp, egp_neighbor *)
{

    egp_check_neighborLoss(ngp);

    egp_msg_state("egp_state_cease", ngp, NGS_CEASE);

    if (ngp->ng_state == NGS_UP) {
	ngp->ng_stats.statedowns++;
    }

    ngp->ng_state = NGS_CEASE;

    return;
}


/*
 *	Routines to send packets
 */

/*
 * egp_send() sends an egp packet.
 */
static void
egp_send __PF3(ngp, egp_neighbor *,
	       egp, egp_packet *,
	       length, size_t)
{
    int error = FALSE;

    /* Set AS number in outgoing packet */
    egp->header.egp_system = htons((unsigned short) (ngp ? ngp->ng_local_as : inet_autonomous_system));

    /* Set version in outgoing packet */
    egp->header.egp_ver = ngp ? ngp->ng_V : EGPVER;

    /* Calculate packet checksum */
    egp->header.egp_chksum = 0;
    egp->header.egp_chksum = inet_cksum((void_t) egp, length);

    if (task_send_packet(ngp->ng_task,
			 (void_t) egp,
			 length,
			 0,
			 (sockaddr_un *) 0) < 0) {
	error = TRUE;
    }
    /* Should we trace this packet? */
    if (TRACE_PACKET_SEND_TP(ngp->ng_task,
			     egp->header.egp_type,
			     EGP_PKT_MAX,
			     egp_trace_masks)) {

	egp_trace(ngp,
		  error ? "EGP *NOT* SENT" : "EGP SENT",
		  TRUE,
		  egp,
		  length,
		  TRACE_DETAIL_SEND_TP(ngp->ng_task,
				       egp->header.egp_type,
				       EGP_PKT_MAX,
				       egp_trace_masks));
    }

    egp_stats.outmsgs++;
    ngp->ng_stats.outmsgs++;
    if (error) {
	egp_stats.outerrors++;
	ngp->ng_stats.outerrors++;
    }
}


/*
 * egp_send_acquire() sends an acquisition or cease packet.
 */
static void
egp_send_acquire __PF4(ngp, egp_neighbor *,
		       code, u_int,
		       status, u_int,
		       id, u_int)
{
    egp_packet *acqpkt = task_get_send_buffer(egp_packet *);
    size_t length;

    acqpkt->acquire.ea_pkt.egp_type = EGP_PKT_ACQUIRE;
    acqpkt->acquire.ea_pkt.egp_code = code;
    acqpkt->acquire.ea_pkt.egp_status = status;
    acqpkt->acquire.ea_pkt.egp_id = htons((u_int16) id);

    switch (code) {
    case EGP_CODE_ACQ_REQUEST:
    case EGP_CODE_ACQ_CONFIRM:
	length = sizeof (egp_packet_acquire);
	acqpkt->acquire.ea_hint = htons(ngp->ng_P1);
	acqpkt->acquire.ea_pint = htons(ngp->ng_P2);
	break;

    default:
	/* omit hello & poll int */
	length = sizeof (egp_packet_header);
    }

    egp_send(ngp, acqpkt, length);
}


/*
 * egp_send_hello() sends a hello or I-H-U packet.
 */
static void
egp_send_hello __PF3 (ngp, egp_neighbor *,
		      code, byte,
		      id, u_int)
{
    egp_packet *hellopkt =  task_get_send_buffer(egp_packet *);

    hellopkt->header.egp_type = EGP_PKT_HELLO;
    hellopkt->header.egp_code = code;
    hellopkt->header.egp_status = (ngp->ng_state == NGS_UP) ? EGP_STATUS_UP : EGP_STATUS_DOWN;
    hellopkt->header.egp_id = htons((u_int16) id);

    if (code == EGP_CODE_HELLO) {
	/* Remember the ID of this Hello */
	ngp->ng_S_lasthello = id;
    }
    
    egp_send(ngp, hellopkt, sizeof (egp_packet_header));
}


/*
 * egp_send_poll() sends an NR poll packet.
 */
static void
egp_send_poll __PF1(ngp, egp_neighbor *)
{
    egp_packet_poll *pollpkt = task_get_send_buffer(egp_packet_poll *);

    pollpkt->ep_pkt.egp_type = EGP_PKT_POLL;
    pollpkt->ep_pkt.egp_code = 0;
    pollpkt->ep_pkt.egp_status = (ngp->ng_state == NGS_UP) ? EGP_STATUS_UP : EGP_STATUS_DOWN;
    pollpkt->ep_pkt.egp_id = htons((u_int16) ngp->ng_S);
    pollpkt->ep_unused = 0;
    pollpkt->ep_net = sock2ip(ngp->ng_saddr);	/* struct copy */

    egp_send(ngp, (egp_packet *) pollpkt, sizeof (egp_packet_poll));
}


/*
 * egp_send_error() sends an error packet.
 */
static void
egp_send_error __PF5(ngp, egp_neighbor *,
		     egp, egp_packet *,
		     length, size_t,
		     error, int,
		     msg, const char *)
{
    egp_packet *errpkt = task_get_send_buffer(egp_packet *);

    errpkt->header.egp_type = EGP_PKT_ERROR;
    errpkt->header.egp_code = (error == EGP_REASON_UVERSION) ? EGPVMASK : 0;
    if (ngp) {
	switch (ngp->ng_state) {
	case NGS_UP:
	    errpkt->header.egp_status = EGP_STATUS_UP;
	    break;
	    
	case NGS_DOWN:
	    errpkt->header.egp_status = EGP_STATUS_DOWN;
	    break;

	default:
	    errpkt->header.egp_status = EGP_STATUS_INDETERMINATE;
	}
    }
    errpkt->header.egp_id = htons((u_int16) egprid_h);	/* recvd seq.# */
    errpkt->error.ee_rsn = htons(error);
    /*
     * copy header of erroneous egp packet
     */
    bzero((char *) errpkt->error.ee_egphd, sizeof (egp_packet_header));
    if (length > sizeof (egp_packet_header)) {
	length = sizeof (egp_packet_header);
    }
    if (length) {
	bcopy((char *) egp, (char *) errpkt->error.ee_egphd, length);
    } else {
	BIT_SET(errpkt->header.egp_status, EGP_STATUS_UNSOLICITED);
    }

    trace_log_tf(ngp->ng_trace_options,
		 0,
		 LOG_WARNING,
		 ("egp_send_error: error packet to neighbor %s: %s",
		  ngp->ng_name,
		  msg));

#ifdef	notdef
    if (XXX /* - If egp_trace did not trace the contents */) {
	egp_trace(ngp,
		  "egp_send_error: send error pkt ",
		  TRUE,
		  errpkt,
		  sizeof (egp_packet_error),
		  FALSE);
    }
#endif	/* notdef */
    ngp->ng_stats.outerrmsgs++;
    egp_send(ngp, errpkt, sizeof (egp_packet_error));
}


/*
 * egp_send_update() sends an NR message packet.
 *
 * It fills in the header information, calls if_rtcheck() to update the
 * interface status information and egp_rt_send() to fill in the reachable
 * networks.
 */

static void
egp_send_update __PF2(ngp, egp_neighbor *,
		      unsol, int)
{
    size_t length;
    egp_packet_nr *nrp;

    /* Make sure the transmit buffer is large enough */
    if (ngp->ng_length > egp_maxpacket) {
	trace_log_tf(ngp->ng_trace_options,
		     0,
		     LOG_WARNING,
		     ("egp_send_update: neighbor %s AS %d NR message size (%d) larger than maximum packet size (%d)",
		      ngp->ng_name,
		      ngp->ng_peer_as,
		      ngp->ng_length,
		      egp_maxpacket));
	return;
    } else if (ngp->ng_length > ngp->ng_send_size) {
	ngp->ng_send_size = ROUNDUP(ngp->ng_length, task_pagesize);
	if (ngp->ng_send_size > egp_maxpacket) {
	    ngp->ng_send_size = egp_maxpacket;
	}

	/* Set the output buffer size */
	task_alloc_send(ngp->ng_task, ngp->ng_send_size);

	/* And the kernel's buffer size */
	if (task_set_option(ngp->ng_task,
			    TASKOPTION_SENDBUF,
			    ngp->ng_send_size) < 0) {
	    task_quit(errno);
	}
    }
    
    /* prepare static part of NR message header */
    nrp = task_get_send_buffer(egp_packet_nr *);
    nrp->en_pkt.egp_type = EGP_PKT_NR;
    nrp->en_pkt.egp_code = 0;
    nrp->en_pkt.egp_status = (ngp->ng_state == NGS_UP) ? EGP_STATUS_UP : EGP_STATUS_DOWN;
    if (unsol) {
	BIT_SET(nrp->en_pkt.egp_status, EGP_STATUS_UNSOLICITED);
    }
    nrp->en_pkt.egp_id = htons((u_int16) ngp->ng_R);
    /*
     * copy shared net address
     */
    nrp->en_net = sock2ip(ngp->ng_paddr);
    nrp->en_igw = 0;
    nrp->en_egw = 0;

    /* Build the update portion */
    length = egp_rt_send(ngp, nrp);
    if (length != (size_t) EGP_ERROR) {
	egp_send(ngp, (egp_packet *) nrp, length);
    }
    
    return;
}


/*
 *	Front end for task timer routines
 */
static void
egp_set_timer __PF3(ngp, egp_neighbor *,
		    tip, task_timer *,
		    value, time_t)
{
    task_timer_set_interval(tip, value);

    trace_tp(ngp->ng_task,
	     TR_STATE,
	     0,
	     ("egp_set_timer: neighbor %s version %d state %s timer %s SET to %#T at %T",
	      ngp->ng_name,
	      ngp->ng_V,
	      trace_state(egp_states, ngp->ng_state),
	      tip->task_timer_name,
	      tip->task_timer_interval,
	      tip->task_timer_next_time));
}


static void
egp_reset_timer __PF3(ngp, egp_neighbor *,
		      tip, task_timer *,
		      value, time_t)
{

    task_timer_set(tip,
		   value,
		   (time_t) 0);

    trace_tp(ngp->ng_task,
	     TR_STATE,
	     0,
	     ("egp_reset_timer: neighbor %s version %d state %s timer %s RESET to %#T at %T",
	      ngp->ng_name,
	      ngp->ng_V,
	      trace_state(egp_states, ngp->ng_state),
	      tip->task_timer_name,
	      tip->task_timer_interval,
	      tip->task_timer_next_time));
}


static void
egp_clear_timer __PF2(ngp, egp_neighbor *,
		      tip, task_timer *)
{
    task_timer_reset(tip);

    trace_tp(ngp->ng_task,
	     TR_STATE,
	     0,
	     ("egp_clear_timer: neighbor %s version %d state %s timer %s STOPPED",
	      ngp->ng_name,
	      ngp->ng_V,
	      trace_state(egp_states, ngp->ng_state),
	      tip->task_timer_name));
}


/**/

/* Routines to process reachability */

/*
 *  egp_check_reachability() checks the reachability status of our neighbors
 */
static void
egp_check_reachability __PF1(ngp, egp_neighbor *)
{
    int change = 0;

    trace_tp(ngp->ng_task,
	     TR_STATE,
	     0,
	     ("egp_check_reachability: neighbor %s version %d state %s [%04B] %d / %d / %d",
	      ngp->ng_name,
	      ngp->ng_V,
	      trace_state(egp_states, ngp->ng_state),
	      ngp->ng_responses,
	      ngp->ng_j,
	      egp_reachability[ngp->ng_responses],
	      ngp->ng_k));

    switch (ngp->ng_state) {
    case NGS_IDLE:
    case NGS_ACQUISITION:
    case NGS_CEASE:
	egp_msg_confused("egp_check_reachability", ngp, "ReachabilityCheck");
	break;

    case NGS_DOWN:
	if (egp_reachability[ngp->ng_responses] >= ngp->ng_j) {
	    egp_event_up(ngp);
	    change++;
	}
	break;

    case NGS_UP:
	if (egp_reachability[ngp->ng_responses] <= ngp->ng_k) {
	    egp_event_down(ngp);
	    change++;
	}
	break;
    }
    if (change) {
	trace_log_tf(ngp->ng_trace_options,
		     0,
		     LOG_WARNING,
		     ("egp_check_reachability: neighbor %s AS %d state %s received %d of %d %s",
		      ngp->ng_name,
		      ngp->ng_peer_as,
		      trace_state(egp_states, ngp->ng_state),
		      egp_reachability[ngp->ng_responses],
		      REACH_RATIO,
		      (ngp->ng_M == EGP_STATUS_ACTIVE) ? "responses" : "requests"));
	trace_tp(ngp->ng_task,
		 TR_STATE,
		 0,
		 ("egp_check_reachability: neighbor %s version %d state %s [%04B] %d / %d / %d",
		  ngp->ng_name,
		  ngp->ng_V,
		  trace_state(egp_states, ngp->ng_state),
		  ngp->ng_responses,
		  ngp->ng_j,
		  egp_reachability[ngp->ng_responses],
		  ngp->ng_k));
    }
}


static void
egp_event_reachability __PF1(ngp, egp_neighbor *)
{

    egp_msg_event("egp_event_reachability", ngp, "reachability");

    BIT_SET(ngp->ng_responses, 1);
    egp_reset_timer(ngp,
		    ngp->ng_timer_t3,
		    (time_t) EGP_P4);
    egp_check_reachability(ngp);
}


static void
egp_shift_reachability __PF1(ngp, egp_neighbor *)
{
    ngp->ng_responses = (ngp->ng_responses << 1) & ((1 << REACH_RATIO) - 1);
    egp_check_reachability(ngp);
}


/*
 *	Check for a status change and issue a reachability event
 */
static int
egp_check_status __PF3(ngp, egp_neighbor *,
		       egp, egp_packet *,
		       status, byte)
{
    int do_check = ngp->ng_M == status;

    switch (egp->header.egp_status & ~EGP_STATUS_UNSOLICITED) {
    case EGP_STATUS_UP:
	if (do_check) {
	    /* Check status in up state if we are passive or active */

	    egp_event_reachability(ngp);
	}
	break;

    case EGP_STATUS_DOWN:
	if (do_check &&
	    status == EGP_STATUS_ACTIVE) {
	    /* Check status in down state only if we are active */

	    egp_event_reachability(ngp);
	}
	break;
	
    default:
	return EGP_REASON_BADHEAD;
    }

    return EGP_NOERROR;
}


/*
 *	Routines for checking polling rate
 */
/*ARGSUSED*/
static void
egp_rate_init __PF3(ngp, egp_neighbor *,
		    rp, struct egp_rate *,
		    last, time_t)
{
    int i;

    for (i = 0; i < RATE_WINDOW; i++) {
	rp->rate_window[i] = rp->rate_min;
    }
    rp->rate_last = last;
}


static int
egp_rate_check __PF2(ngp, egp_neighbor *,
		     rp, struct egp_rate *)
{
    int i;
    int excessive;
    time_t interval;

    interval = rp->rate_last ? time_sec - rp->rate_last : rp->rate_min;

    excessive = interval < rp->rate_min ? 1 : 0;

    for (i = RATE_WINDOW - 1; i; i--) {
	if ((rp->rate_window[i] = rp->rate_window[i - 1]) < rp->rate_min) {
	    excessive++;
	}
    }

    trace_tp(ngp->ng_task,
	     TR_STATE,
	     0,
	     ("egp_rate_check: neighbor %s min %#T last %#T excessive %d",
	      ngp->ng_name,
	      rp->rate_min,
	      rp->rate_last,
	      excessive));
    if (TRACE_TP(ngp->ng_task, TR_STATE)) {
	tracef("egp_rate_check: neighbor %s window ",
	       ngp->ng_name);
    }

    rp->rate_window[0] = interval;

    rp->rate_last = time_sec;

    if (TRACE_TP(ngp->ng_task, TR_STATE)) {
	for (i = 0; i < RATE_WINDOW; i++) {
	    tracef("%#T ",
		   rp->rate_window[i]);
	}

	trace_only_tf(ngp->ng_trace_options,
		      0,
		      (NULL));
    }

    if (excessive < RATE_MAX) {
	return 0;
    } else {
	egp_rate_init(ngp, rp, rp->rate_last);
	return 1;
    }
}


/*
 * egp_set_intervals() sets EGP hello and poll intervals and times.
 * Returns 1 if either poll or hello intervals too big, 0 otherwise.
 */
static int
egp_set_intervals __PF2(ngp, egp_neighbor *,
			egppkt, egp_packet *)
{
    egp_packet_acquire *egpa = (egp_packet_acquire *) egppkt;
    u_int helloint, pollint, ratio;

    /*
     * check parameters within bounds
     */
    helloint = ntohs(egpa->ea_hint);
    pollint = ntohs(egpa->ea_pint);
    if (helloint > MAXHELLOINT || pollint > MAXPOLLINT) {
	trace_log_tf(ngp->ng_trace_options,
		     0,
		     LOG_WARNING,
		     ("egp_set_intervals: Hello interval = %d or poll interval = %d too big from %s, code %d",
		      helloint,
		      pollint,
		      ngp->ng_name,
		      egpa->ea_pkt.egp_code));
	return 1;
    }
    if ((helloint != ngp->ng_P1) || (pollint != ngp->ng_P2)) {
	trace_tp(ngp->ng_task,
		 TR_NORMAL,
		 0,
		 ("egp_set_intervals: neighbor %s specified hello/poll intervals %d/%d, we specified %d/%d",
		  ngp->ng_name,
		  helloint,
		  pollint,
		  ngp->ng_P1,
		  ngp->ng_P2));
    }
    if (helloint < ngp->ng_P1) {
	helloint = ngp->ng_P1;
    }
    if (pollint < ngp->ng_P2) {
	pollint = ngp->ng_P2;
    }
    ratio = (pollint - 1) / helloint + 1;	/* keep ratio pollint:helloint */
    helloint += HELLOMARGIN;
    pollint = ratio * helloint;
    trace_tp(ngp->ng_task,
	     TR_NORMAL,
	     0,
	     ("egp_set_intervals: neighbor %s version %d state %s using intervals %d/%d",
	      ngp->ng_name,
	      ngp->ng_V,
	      trace_state(egp_states, ngp->ng_state),
	      helloint,
	      pollint));
    ngp->ng_T1 = helloint;
    ngp->ng_T3 = ngp->ng_T1 * REACH_RATIO;
    ngp->ng_T2 = helloint * ratio;
    return 0;
}


/*
 * egp_init_variables() go into neighbor state, initialize most variables.
 */

/* ARGSUSED */
static int
egp_init_variables __PF2(ngp, egp_neighbor *,
			 egp, egp_packet *)
{
    as_t peer_as = htons(egp->header.egp_system);

    if (egp_set_intervals(ngp, egp)) {
	return TRUE;
    }

    ngp->ng_responses = 0;
    ngp->ng_status = 0;
    ngp->ng_noupdate = 0;
    ngp->ng_R_lastpoll = (u_int) -1;	/* Invalid ID for last poll */
    BIT_RESET(ngp->ng_flags, NGF_SENT_POLL | NGF_SENT_REPOLL | NGF_SENT_UNSOL | NGF_RECV_UNSOL | NGF_RECV_REPOLL | NGF_PROC_POLL);

    egp_rate_init(ngp, &ngp->ng_hello_rate, (time_t) 0);
    egp_rate_init(ngp, &ngp->ng_poll_rate, (time_t) 0);

    /* Check the AS for validity */
    switch (peer_as) {
    case 0:
    case (as_t) -1:
	trace_log_tf(ngp->ng_trace_options,
		     0,
		     LOG_WARNING,
		     ("egp_init_variables: neighbor %s version %d state %s invalid AS %u",
		      ngp->ng_name,
		      ngp->ng_V,
		      trace_state(egp_states, ngp->ng_state),
		      peer_as));
	return TRUE;

    default:
	break;
    }
    
    if (!BIT_TEST(ngp->ng_flags, NGF_POLICY) ||
	ngp->ng_peer_as != peer_as) {
	/* Need new policy */

	ngp->ng_peer_as = peer_as;

	/* Re-evaluate policy for this peer */
	egp_rt_newpolicy(ngp);
    }


    /* Determine whether we are active or passive ... */
    switch (egp->header.egp_status) {
    case EGP_STATUS_ACTIVE:
	ngp->ng_M = EGP_STATUS_PASSIVE;
	break;
	
    case EGP_STATUS_PASSIVE:
	ngp->ng_M = EGP_STATUS_ACTIVE;
	break;
	
    case EGP_STATUS_UNSPEC:
	ngp->ng_M = ngp->ng_peer_as < ngp->ng_local_as ? EGP_STATUS_PASSIVE : EGP_STATUS_ACTIVE;
	break;

    default:
	/* Bogus reachability */
	return TRUE;
    }

    /* ... and set our reachability values */
    switch (ngp->ng_M) {
    case EGP_STATUS_ACTIVE:
	ngp->ng_j = 3;
	ngp->ng_k = 1;
	break;

    case EGP_STATUS_PASSIVE:
	ngp->ng_j = 1;
	ngp->ng_k = 0;	/* note that the value of 4 in RFC904 is a typo */
	break;
    }

    return FALSE;
}


/*
 *	Routine to call when a poll is due
 */
static void
egp_do_poll __PF2(ngp, egp_neighbor *,
		  sync_t1, int)
{
    task_timer *tip = ngp->ng_timer_t2;

    assert(ngp->ng_state == NGS_UP);
    
    if (BIT_TEST(ngp->ng_flags, NGF_SENT_REPOLL)) {
	BIT_RESET(ngp->ng_flags, NGF_SENT_POLL | NGF_SENT_REPOLL);
    }
    if (BIT_TEST(ngp->ng_flags, NGF_SENT_POLL)) {
	BIT_SET(ngp->ng_flags, NGF_SENT_REPOLL);
    } else {
	ngp->ng_S++;
	BIT_SET(ngp->ng_flags, NGF_SENT_POLL);
	BIT_RESET(ngp->ng_flags, NGF_RECV_UNSOL);
	if (++ngp->ng_noupdate > MAXNOUPDATE) {
	    char buf[LINE_MAX];

	    sprintf(buf, "no Update received for %d successive new poll id's",
		    ngp->ng_noupdate);
	    egp_send_error(ngp, (egp_packet *) 0, 0, EGP_REASON_NORESPONSE, buf);
	    ngp->ng_noupdate = 0;
	    return;
	}
    }
    egp_send_poll(ngp);

    if (sync_t1) {
	/* Sync t2 to t1 */
	egp_reset_timer(ngp,
			tip,
			ngp->ng_T2 - (time_sec - ngp->ng_timer_t1->task_timer_last_time));
    } else if (tip->task_timer_interval != ngp->ng_T2) {
	/* Set our proper interval */
	egp_set_timer(ngp, tip, ngp->ng_T2);
    }
}


/*
 *	Routines to process events
 */
static void
egp_event_up __PF1(ngp, egp_neighbor *)
{

    egp_msg_event("egp_event_up", ngp, "Up");

    switch (ngp->ng_state) {
    case NGS_IDLE:
    case NGS_ACQUISITION:
    case NGS_CEASE:
    case NGS_UP:
	egp_msg_confused("egp_event_up", ngp, "Up");
	break;

    case NGS_DOWN:
	/* Transition to UP state */
	egp_state_up(ngp);

	if (ngp->ng_state == NGS_UP) {
	    /* Maxup code may have forced us out of the UP state */

	    /* Send a POLL */
	    egp_do_poll(ngp, TRUE);

	    /* Just for good luck, send and unsolicited update if we can make an educated guess about which net he is */
	    /* interested in */
	    if (!(ngp->ng_flags & (NGF_SENT_UNSOL | NGF_PROC_POLL))) {
		struct in_addr source_net;

		/* Assume he wants the same net we do */
		source_net = sock2in(ngp->ng_saddr);	/* struct copy */

		if (!source_net.s_addr &&
		    inet_net_natural(ngp->ng_addr) == inet_net_natural(ngp->ng_ifap->ifa_addr_local)) {
		    /* Polled net not set, use shared net if we are both on it */
		    source_net.s_addr = inet_net_natural(ngp->ng_addr);
		}

		/* If we figured out a net, and we have a route to it, send the unsolicited update */
		if (source_net.s_addr) {
		    /* We figured out a network */
			
		    if (!egp_rt_newaddr(ngp, sockbuild_in(0, source_net.s_addr))) {
			/* Netlist is up to date, send the update */
			
			egp_send_update(ngp, 1);
			BIT_SET(ngp->ng_flags, NGF_SENT_UNSOL);
		    }
		}
	    }
	}
	break;
    }
}


static void
egp_event_down __PF1(ngp, egp_neighbor *)
{

    egp_msg_event("egp_event_down", ngp, "Down");

    switch (ngp->ng_state) {
    case NGS_IDLE:
    case NGS_ACQUISITION:
    case NGS_CEASE:
    case NGS_DOWN:
	egp_msg_confused("egp_event_down", ngp, "Down");
	break;

    case NGS_UP:
	egp_clear_timer(ngp, ngp->ng_timer_t2);
	egp_state_down(ngp);
	break;
    }
}


/*ARGSUSED*/
static void
egp_event_request __PF3(ngp, egp_neighbor *,
			egp, egp_packet *,
			egplen, size_t)
{
    u_int helloint;

    egp_msg_event("egp_event_request", ngp, "Request");

    switch (ngp->ng_state) {
    case NGS_IDLE:
    case NGS_ACQUISITION:
    case NGS_DOWN:
    case NGS_UP:
	if (BIT_TEST(ngp->ng_options, NGO_PEERAS) &&
	    (ngp->ng_peer_as != htons(egp->header.egp_system))) {
	    trace_log_tf(ngp->ng_trace_options,
			 0,
			 LOG_ERR,
			 ("egp_event_request: neighbor %s version %d state %s specified AS %d, we expected %d",
			  ngp->ng_name,
			  ngp->ng_V,
			  trace_state(egp_states, ngp->ng_state),
			  htons(egp->header.egp_system),
			  ngp->ng_peer_as));
	    egp_send_acquire(ngp, EGP_CODE_ACQ_REFUSE, EGP_STATUS_ADMINPROHIB, egprid_h);
	    egp_clear_timer(ngp, ngp->ng_timer_t1);
	    egp_clear_timer(ngp, ngp->ng_timer_t2);
	    egp_reset_timer(ngp,
			    ngp->ng_timer_t3,
			    (time_t) EGP_START_LONG);
	    egp_state_idle(ngp);
	    break;
	}
	if (egp_init_variables(ngp, egp)) {
	    /* XXX - May want to declare a stop? */
	    egp_send_acquire(ngp, EGP_CODE_ACQ_REFUSE, EGP_STATUS_PARAMPROB, egprid_h);
	    break;
	}
	ngp->ng_R = egprid_h;
	egp_send_acquire(ngp, EGP_CODE_ACQ_CONFIRM, (u_int) ngp->ng_M, egprid_h);
	if (ngp->ng_M == EGP_STATUS_ACTIVE) {
	    egp_send_hello(ngp, EGP_CODE_HELLO, ngp->ng_S);
	}
	egp_reset_timer(ngp,
			ngp->ng_timer_t1,
			ngp->ng_T1);
	helloint = ntohs(((egp_packet_acquire *) egp)->ea_hint);
	if (helloint < ngp->ng_P1) {
	    helloint = ngp->ng_P1;
	}
	egp_reset_timer(ngp,
			ngp->ng_timer_t3,
			(time_t) (2 * REACH_RATIO * helloint));
	egp_state_down(ngp);
	break;

    case NGS_CEASE:
	ngp->ng_status = EGP_STATUS_GOINGDOWN;
	egp_send_acquire(ngp, EGP_CODE_CEASE, ngp->ng_status, ngp->ng_S);
	egp_state_cease(ngp);
    }
}


static void
egp_event_confirm __PF3(ngp, egp_neighbor *,
			egp, egp_packet *,
			egplen, size_t)
{
    u_int helloint;

    egp_msg_event("egp_event_confirm", ngp, "Confirm");

    switch (ngp->ng_state) {
    case NGS_IDLE:
	egp_send_acquire(ngp, EGP_CODE_CEASE, EGP_STATUS_PROTOVIOL, ngp->ng_S);
	break;

    case NGS_ACQUISITION:
	if (BIT_TEST(ngp->ng_options, NGO_PEERAS) &&
	    (ngp->ng_peer_as != htons(egp->header.egp_system))) {
	    trace_log_tf(ngp->ng_trace_options,
			 0,
			 LOG_ERR,
			 ("egp_event_confirm: neighbor %s version %d state %s specified AS %d, we expected %d",
			  ngp->ng_name,
			  ngp->ng_V,
			  trace_state(egp_states, ngp->ng_state),
			  htons(egp->header.egp_system),
			  ngp->ng_peer_as));
	    egp_send_acquire(ngp, EGP_CODE_ACQ_REFUSE, EGP_STATUS_ADMINPROHIB, egprid_h);
	    egp_clear_timer(ngp, ngp->ng_timer_t1);
	    egp_clear_timer(ngp, ngp->ng_timer_t2);
	    egp_reset_timer(ngp,
			    ngp->ng_timer_t3,
			    (time_t) EGP_START_LONG);
	    egp_state_idle(ngp);
	    break;
	}
	if (egp_init_variables(ngp, egp)) {
	    /* XXX - May need more thought */
	    egp_event_stop(ngp, EGP_STATUS_PARAMPROB);
	    break;
	}
	ngp->ng_R = egprid_h;
	if (ngp->ng_M == EGP_STATUS_ACTIVE) {
	    egp_send_hello(ngp, EGP_CODE_HELLO, ngp->ng_S);
	}
	egp_reset_timer(ngp,
			ngp->ng_timer_t1,
			ngp->ng_T1);
	helloint = ntohs(((egp_packet_acquire *) egp)->ea_hint);
	if (helloint < ngp->ng_P1) {
	    helloint = ngp->ng_P1;
	}
	egp_reset_timer(ngp,
			ngp->ng_timer_t3,
			(time_t) (2 * REACH_RATIO * helloint));
	egp_state_down(ngp);
	break;

    case NGS_DOWN:
    case NGS_UP:
    case NGS_CEASE:
	/* XXX - Should we ignore this? */
	egp_msg_confused("egp_event_confirm", ngp, "Confirm");
	break;
    }
}


static void
egp_event_refuse __PF3(ngp, egp_neighbor *,
		       egp, egp_packet *,
		       egplen, size_t)
{
    time_t restart_delay = 0;

    egp_msg_event("egp_event_refuse", ngp, "Refuse");

    switch (ngp->ng_state) {
    case NGS_IDLE:
	egp_send_acquire(ngp, EGP_CODE_CEASE, EGP_STATUS_PROTOVIOL, ngp->ng_S);
	break;

    case NGS_ACQUISITION:
	trace_log_tf(ngp->ng_trace_options,
		     0,
		     LOG_WARNING,
		     ("egp_event_refuse: neighbor %s AS %d state %s Cease Refuse %s",
		      ngp->ng_name,
		      ngp->ng_peer_as,
		      trace_state(egp_states, ngp->ng_state),
		      egp_acq_status[egp->header.egp_status]));
	egp_clear_timer(ngp, ngp->ng_timer_t1);
	egp_clear_timer(ngp, ngp->ng_timer_t2);
	switch (egp->header.egp_status) {
	case EGP_STATUS_UNSPEC:
	case EGP_STATUS_ACTIVE:
	case EGP_STATUS_PASSIVE:
	case EGP_STATUS_NORESOURCES:
	case EGP_STATUS_GOINGDOWN:
	    restart_delay = EGP_START_SHORT;
	    break;

	case EGP_STATUS_ADMINPROHIB:
	case EGP_STATUS_PARAMPROB:
	case EGP_STATUS_PROTOVIOL:
	    restart_delay = EGP_START_LONG;
	    break;

	default:
	    egp_send_error(ngp, egp, egplen, EGP_REASON_BADHEAD, "invalid Status field in Refuse");
	    egp_stats.inerrors++;
	    egp_stats.inmsgs--;
	    ngp->ng_stats.inerrors++;
	    ngp->ng_stats.inmsgs--;
	    break;
	}
	egp_reset_timer(ngp,
			ngp->ng_timer_t3,
			restart_delay);
	egp_state_idle(ngp);
	break;

    case NGS_DOWN:
    case NGS_UP:
    case NGS_CEASE:
	egp_msg_confused("egp_event_refuse", ngp, "Refuse");
	break;
    }
}


static void
egp_event_cease __PF3(ngp, egp_neighbor *,
		      egp, egp_packet *,
		      egplen, size_t)
{
    time_t restart_delay = 0;

    egp_msg_event("egp_event_cease", ngp, "Cease");

    switch (ngp->ng_state) {
    case NGS_IDLE:
	egp_send_acquire(ngp, EGP_CODE_CEASE_ACK, EGP_STATUS_UNSPEC, egprid_h);
	break;

    case NGS_ACQUISITION:
    case NGS_DOWN:
    case NGS_UP:
	trace_log_tf(ngp->ng_trace_options,
		     0,
		     LOG_WARNING,
		     ("egp_event_cease: neighbor %s AS %d state %s Cease reason %s",
		      ngp->ng_name,
		      ngp->ng_peer_as,
		      trace_state(egp_states, ngp->ng_state),
		      egp_acq_status[egp->header.egp_status]));
	/* Fall through */

    case NGS_CEASE:
	egp_send_acquire(ngp, EGP_CODE_CEASE_ACK, EGP_STATUS_UNSPEC, egprid_h);
	egp_clear_timer(ngp, ngp->ng_timer_t1);
	egp_clear_timer(ngp, ngp->ng_timer_t2);
	switch (egp->header.egp_status) {
	case EGP_STATUS_UNSPEC:
	case EGP_STATUS_ACTIVE:
	case EGP_STATUS_PASSIVE:
	case EGP_STATUS_NORESOURCES:
	case EGP_STATUS_GOINGDOWN:
	    restart_delay = EGP_START_SHORT;
	    break;

	case EGP_STATUS_ADMINPROHIB:
	case EGP_STATUS_PARAMPROB:
	case EGP_STATUS_PROTOVIOL:
	    restart_delay = EGP_START_LONG;
	    break;

	default:
	    egp_send_error(ngp, egp, egplen, EGP_REASON_BADHEAD, "invalid Status field in Cease");
	    egp_stats.inerrors++;
	    egp_stats.inmsgs--;
	    ngp->ng_stats.inerrors++;
	    ngp->ng_stats.inmsgs--;
	    break;
	}
	egp_reset_timer(ngp,
			ngp->ng_timer_t3,
			restart_delay);
	egp_state_idle(ngp);
	break;
    }
}


static void
egp_event_ceaseack __PF3(ngp, egp_neighbor *,
			 egp, egp_packet *,
			 egplen, size_t)
{
    time_t restart_delay = 0;

    egp_msg_event("egp_event_ceaseack", ngp, "Cease-ack");

    switch (ngp->ng_state) {
    case NGS_IDLE:
	break;

    case NGS_ACQUISITION:
    case NGS_DOWN:
    case NGS_UP:
	break;

    case NGS_CEASE:
	egp_clear_timer(ngp, ngp->ng_timer_t1);
	egp_clear_timer(ngp, ngp->ng_timer_t2);
	switch (ngp->ng_status) {
	case EGP_STATUS_UNSPEC:
	case EGP_STATUS_ACTIVE:
	case EGP_STATUS_PASSIVE:
	case EGP_STATUS_NORESOURCES:
	case EGP_STATUS_GOINGDOWN:
	    restart_delay = EGP_START_SHORT;
	    break;

	case EGP_STATUS_ADMINPROHIB:
	case EGP_STATUS_PARAMPROB:
	case EGP_STATUS_PROTOVIOL:
	    restart_delay = EGP_START_LONG;
	    break;

	default:
	    egp_send_error(ngp, egp, egplen, EGP_REASON_BADHEAD, "invalid Status field in Cease-ack");
	    egp_stats.inerrors++;
	    egp_stats.inmsgs--;
	    ngp->ng_stats.inerrors++;
	    ngp->ng_stats.inmsgs--;
	    break;
	}
	egp_reset_timer(ngp,
			ngp->ng_timer_t3,
			restart_delay);
	egp_state_idle(ngp);
	break;
    }
}


static void
egp_event_hello __PF3(ngp, egp_neighbor *,
		      egp, egp_packet *,
		      egplen, size_t)
{
    int error = EGP_NOERROR;
    const char *msg = NULL;

    egp_msg_event("egp_event_hello", ngp, "Hello");

    switch (ngp->ng_state) {
    case NGS_IDLE:
	egp_send_acquire(ngp, EGP_CODE_CEASE, EGP_STATUS_PROTOVIOL, ngp->ng_S);
	break;

    case NGS_ACQUISITION:
    case NGS_CEASE:
	break;

    case NGS_DOWN:
    case NGS_UP:
	if ((error = egp_check_status(ngp, egp, EGP_STATUS_PASSIVE)) != EGP_NOERROR) {
	    msg = "invalid Status field in Hello";
	    egp_stats.inerrors++;
	    egp_stats.inmsgs--;
	    ngp->ng_stats.inerrors++;
	    ngp->ng_stats.inmsgs--;
	    break;
	} else {
	    if (egp_rate_check(ngp, &ngp->ng_hello_rate)) {
		error = EGP_REASON_XSPOLL;
		msg = "excessive HELLO rate";
		break;
	    }
	    egp_send_hello(ngp, EGP_CODE_HEARDU, egprid_h);
	}
	break;
    }
    if (error != EGP_NOERROR) {
	egp_send_error(ngp, egp, egplen, error, msg);
    }
}


static void
egp_event_heardu __PF3(ngp, egp_neighbor *,
		       egp, egp_packet *,
		       egplen, size_t)
{

    egp_msg_event("egp_event_heardu", ngp, "I-H-U");

    switch (ngp->ng_state) {
    case NGS_IDLE:
	egp_send_acquire(ngp, EGP_CODE_CEASE, EGP_STATUS_PROTOVIOL, ngp->ng_S);
	break;

    case NGS_ACQUISITION:
    case NGS_CEASE:
	break;

    case NGS_DOWN:
    case NGS_UP:
	if (egp_check_status(ngp, egp, EGP_STATUS_ACTIVE) != EGP_NOERROR) {
	    egp_send_error(ngp, egp, egplen, EGP_REASON_BADHEAD, "invalid Status field in I-H-U");
	    egp_stats.inerrors++;
	    egp_stats.inmsgs--;
	    ngp->ng_stats.inerrors++;
	    ngp->ng_stats.inmsgs--;
	}
	break;
    }
}


static void
egp_event_poll __PF3(ngp, egp_neighbor *,
		     egp, egp_packet *,
		     egplen, size_t)
{
    int error = EGP_NOERROR;
    const char *msg = NULL;

    egp_msg_event("egp_event_poll", ngp, "Poll");

    switch (ngp->ng_state) {
    case NGS_IDLE:
	egp_send_acquire(ngp, EGP_CODE_CEASE, EGP_STATUS_PROTOVIOL, ngp->ng_S);
	break;

    case NGS_ACQUISITION:
    case NGS_CEASE:
	break;

    case NGS_DOWN:
    case NGS_UP:
	BIT_SET(ngp->ng_flags, NGF_PROC_POLL);
	if ((error = egp_check_status(ngp, egp, EGP_STATUS_PASSIVE)) != EGP_NOERROR) {
	    BIT_RESET(ngp->ng_flags, NGF_PROC_POLL);
	    msg = "invalid Status field in Poll";
	    break;
	}
	BIT_RESET(ngp->ng_flags, NGF_PROC_POLL);
	if (egp->header.egp_code != 0) {
	    error = EGP_REASON_BADHEAD;
	    msg = "invalid Code field in Poll";
	    break;
	}
	ngp->ng_R = egprid_h;
	if (egprid_h == ngp->ng_R_lastpoll) {
	    if (ngp->ng_flags & (NGF_RECV_REPOLL | NGF_SENT_UNSOL)) {
		error = EGP_REASON_XSPOLL;
		msg = "too many Polls received";
		break;
	    }
	    BIT_SET(ngp->ng_flags, NGF_RECV_REPOLL | NGF_SENT_UNSOL);
	} else {
	    ngp->ng_R_lastpoll = egprid_h;
	    BIT_RESET(ngp->ng_flags, NGF_RECV_REPOLL | NGF_SENT_UNSOL);
	}
	if (egp_rate_check(ngp, &ngp->ng_poll_rate)) {
	    error = EGP_REASON_XSPOLL;
	    msg = "excessive Polling rate";
	    break;
	}
	if (ngp->ng_state == NGS_DOWN) {
	    /* Ignore Polls in Down state */
	    break;
	}

	/* Rebuild the network list based on this polled address if necessary */
	if (!egp_rt_newaddr(ngp, sockbuild_in(0, egp->poll.ep_net))) {
	    egp_send_update(ngp, 0);
	} else {
	    error = EGP_REASON_NOREACH;
	    msg = "no interface on net of Poll";
	}
	break;
    }
    if (error != EGP_NOERROR) {
	egp_send_error(ngp, egp, egplen, error, msg);
	egp_stats.inerrors++;
	egp_stats.inmsgs--;
	ngp->ng_stats.inerrors++;
	ngp->ng_stats.inmsgs--;
    }
}


static void
egp_event_update __PF3(ngp, egp_neighbor *,
		       egp, egp_packet *,
		       egplen, size_t)
{
    int error = EGP_NOERROR;
    const char *msg = NULL;


    egp_msg_event("egp_event_update", ngp, "Update");

    switch (ngp->ng_state) {
    case NGS_IDLE:
	egp_send_acquire(ngp, EGP_CODE_CEASE, EGP_STATUS_PROTOVIOL, ngp->ng_S);
	break;

    case NGS_ACQUISITION:
    case NGS_DOWN:
    case NGS_CEASE:
	break;

    case NGS_UP:
	if (egp->header.egp_code != 0) {
	    error = EGP_REASON_BADHEAD;
	    msg = "invalid Code field in Update";
	    break;
	}
	error = egp_check_status(ngp, egp, EGP_STATUS_ACTIVE);
	if (error != EGP_NOERROR) {
	    msg = "invalid Status field in Update";
	    break;
	}
        {
	    sockaddr_un *net = sockbuild_in(0, egp->nr.en_net);
	    
	    if (inet_net_natural(net) != inet_net_natural(ngp->ng_saddr)) {
		error = EGP_REASON_BADHEAD;
		msg = "Update Response/Indication IP Net Address field does not match command";
		break;
	    }
	}
	if (egprid_h != ngp->ng_S) {
	    /* Ignore packets with bad sequence number */
	    trace_tp(ngp->ng_task,
		     TR_STATE,
		     0,
		     ("egp_event_update: neighbor %s AS %u: Sequence %d expected %d",
		      ngp->ng_name,
		      ngp->ng_peer_as,
		      egprid_h,
		      ngp->ng_S));
	    break;
	}
	if (BIT_TEST(egp->header.egp_status, EGP_STATUS_UNSOLICITED)) {
	    if (BIT_TEST(ngp->ng_flags, NGF_RECV_UNSOL)) {
		error = EGP_REASON_UNSPEC;
		msg = "too many unsolicited Update Indications";
		break;
	    }
	    BIT_SET(ngp->ng_flags, NGF_RECV_UNSOL);
	} else {
	    if (!BIT_TEST(ngp->ng_flags, NGF_SENT_POLL)) {
		error = EGP_REASON_UNSPEC;
		msg = "too many Update Indications";
		break;
	    }
	}
	BIT_RESET(ngp->ng_flags, NGF_SENT_POLL | NGF_SENT_REPOLL);
	if ((error = egp_rt_recv(ngp, egp, egplen)) != EGP_NOERROR) {
	    switch (error) {
	    case EGP_REASON_BADDATA:
		msg = "invalid Update message format";
		break;

	    case EGP_REASON_UNSPEC:
		msg = "unable to find interface for this neighbor";
		break;

	    default:
		msg = "internal error parsing Update ";
	    }
	    break;
	} else {
	    ngp->ng_noupdate = 0;
	}
	if (BIT_TEST(egp->header.egp_status, EGP_STATUS_UNSOLICITED)) {
	    /* XXX - What should we do here? */
	}
	egp_set_timer(ngp,
		      ngp->ng_timer_t2,
		      ngp->ng_T2);	/* t2 is reset relative to last start */
	break;
    }

    if (error != EGP_NOERROR) {
	egp_send_error(ngp, egp, egplen, error, msg);
	egp_stats.inerrors++;
	egp_stats.inmsgs--;
	ngp->ng_stats.inerrors++;
	ngp->ng_stats.inmsgs--;
    }
}


void
egp_event_start __PF1(tp, task *)
{
    egp_neighbor *ngp;

    ngp = (egp_neighbor *) tp->task_data;

    egp_msg_event("egp_event_start", ngp, "Start");

    switch (ngp->ng_state) {
    case NGS_ACQUISITION:
    case NGS_DOWN:
    case NGS_UP:
	trace_log_tf(ngp->ng_trace_options,
		     0,
		     LOG_WARNING,
		     ("egp_event_start: neighbor %s AS %d state %s Start",
		      ngp->ng_name,
		      ngp->ng_peer_as,
		      trace_state(egp_states, ngp->ng_state)));
	goto Start;

    case NGS_IDLE:
        if (!ngp->ng_ifap) {
	    if_addr *ifap = egp_ngp_ifa_select(ngp);
	    
	    if (!ifap) {
		trace_log_tf(ngp->ng_trace_options,
			     0,
			     LOG_INFO,
			     ("egp_event_start: neighbor %s AS %d: No acceptable interface",
			      ngp->ng_name,
			      ngp->ng_peer_as));

		/* Sleep until we have a valid interface */
		egp_clear_timer(ngp, ngp->ng_timer_t3);
		break;
	    }
	    if (!egp_ngp_ifa_bind(ngp, ifap)) {
		/* Problem binding to the new interface */
		
		trace_log_tf(ngp->ng_trace_options,
			     0,
			     LOG_INFO,
			     ("egp_event_start: neighbor %s AS %d: Cannot allocate resources",
			      ngp->ng_name,
			      ngp->ng_peer_as));

		/* Retry again in a little bit */
		egp_reset_timer(ngp,
				ngp->ng_timer_t3,
				EGP_START_SHORT);
		break;
	    }
	}
	/* Fall through */

    Start:
	egp_send_acquire(ngp, EGP_CODE_ACQ_REQUEST, EGP_STATUS_UNSPEC, ngp->ng_S);
	egp_reset_timer(ngp,
			ngp->ng_timer_t1,
			(time_t) EGP_P3);
	egp_reset_timer(ngp,
			ngp->ng_timer_t3,
			(time_t) EGP_P5);
	egp_state_acquisition(ngp);
	break;

    case NGS_CEASE:
	break;
    }
}


/*
 * egp_event_stop() sends Ceases to all neighbors when going down (when SIGTERM
 * received).
 *
 */

void
egp_event_stop __PF2(ngp, egp_neighbor *,
		     status, int)
{

    egp_msg_event("egp_event_stop", ngp, "Stop");

    switch (ngp->ng_state) {
    case NGS_IDLE:
	egp_ngp_idlecheck(ngp);
	break;

    case NGS_ACQUISITION:
    case NGS_CEASE:
	egp_clear_timer(ngp, ngp->ng_timer_t1);
	egp_clear_timer(ngp, ngp->ng_timer_t2);
	egp_reset_timer(ngp,
			ngp->ng_timer_t3,
			(time_t) EGP_START_SHORT);
	egp_state_idle(ngp);
	break;

    case NGS_DOWN:
    case NGS_UP:
	trace_log_tf(ngp->ng_trace_options,
		     0,
		     LOG_WARNING,
		     ("egp_event_stop: neighbor %s AS %d state %s Stop reason %s",
		      ngp->ng_name,
		      ngp->ng_peer_as,
		      trace_state(egp_states, ngp->ng_state),
		      egp_acq_status[EGP_STATUS_GOINGDOWN]));
	egp_reset_timer(ngp,
			ngp->ng_timer_t1,
			(time_t) EGP_P3);
	/* The spec does not allow for this, but if t2 fires in this */
	/* state we'll get an annoying message */
	egp_clear_timer(ngp, ngp->ng_timer_t2);
	egp_reset_timer(ngp,
			ngp->ng_timer_t3,
			(time_t) EGP_P5);
	ngp->ng_status = status;
	egp_send_acquire(ngp, EGP_CODE_CEASE, ngp->ng_status, ngp->ng_S);
	egp_state_cease(ngp);
	break;
    }
}


/*ARGSUSED*/
void
egp_event_t3 __PF2(tip, task_timer *,
		   interval, time_t)
{
    egp_neighbor *ngp;

    ngp = (egp_neighbor *) tip->task_timer_task->task_data;

    egp_msg_event("egp_event_t3", ngp, "t3");

    switch (ngp->ng_state) {
    case NGS_IDLE:
	if (egp_group_acquired(ngp) < ngp->ng_gr_head->ng_gr_acquire) {
	    egp_event_start(tip->task_timer_task);
	} else {
	    egp_reset_timer(ngp,
			    ngp->ng_timer_t3,
			    (time_t) EGP_START_RETRY);
	}
	break;

    case NGS_ACQUISITION:
#ifdef	EGPVERDEFAULT
	if (ngp->ng_V != EGPVERDEFAULT) {
	    egp_set_version(ngp, EGPVERDEFAULT);
	    egp_event_start(tip->task_timer_task);
	    break;
	}
#endif	/* EGPVERDEFAULT */

    case NGS_CEASE:
	egp_clear_timer(ngp, ngp->ng_timer_t1);
	egp_clear_timer(ngp, ngp->ng_timer_t2);
	egp_reset_timer(ngp,
			ngp->ng_timer_t3,
			(time_t) EGP_START_SHORT);
	egp_state_idle(ngp);
	break;

    case NGS_DOWN:
    case NGS_UP:
	trace_log_tf(ngp->ng_trace_options,
		     0,
		     LOG_WARNING,
		     ("egp_event_t3: neighbor %s AS %d state %s Abort",
		      ngp->ng_name,
		      ngp->ng_peer_as,
		      trace_state(egp_states, ngp->ng_state)));
	egp_reset_timer(ngp,
			ngp->ng_timer_t1,
			(time_t) EGP_P3);
	/* The spec does not allow for this, but if t2 fires in this */
	/* state we'll get an annoying message */
	egp_clear_timer(ngp, ngp->ng_timer_t2);
	egp_reset_timer(ngp,
			ngp->ng_timer_t3,
			(time_t) EGP_P5);
	ngp->ng_status = EGP_STATUS_GOINGDOWN;
	egp_send_acquire(ngp, EGP_CODE_CEASE, ngp->ng_status, ngp->ng_S);
	egp_state_cease(ngp);
	break;
    }
}


/*ARGSUSED*/
void
egp_event_t1 __PF2(tip, task_timer *,
		   interval, time_t)
{
    egp_neighbor *ngp;

    ngp = (egp_neighbor *) tip->task_timer_task->task_data;

    egp_msg_event("egp_event_t1", ngp, "t1");

    switch (ngp->ng_state) {
    case NGS_IDLE:
	egp_msg_confused("egp_event_t1", ngp, "t1");
	break;

    case NGS_ACQUISITION:
	egp_send_acquire(ngp, EGP_CODE_ACQ_REQUEST, EGP_STATUS_UNSPEC, ngp->ng_S);
	egp_set_timer(ngp,
		      ngp->ng_timer_t1,
		      (time_t) EGP_P3);
	break;

    case NGS_DOWN:
    case NGS_UP:
	egp_shift_reachability(ngp);
	if (ngp->ng_M == EGP_STATUS_ACTIVE) {
	    egp_send_hello(ngp, EGP_CODE_HELLO, ngp->ng_S);
	}

	/* Check to see if a repoll is due */
	if (ngp->ng_state == NGS_UP &&
	    BIT_TEST(ngp->ng_flags, NGF_SENT_POLL) &&
	    !BIT_TEST(ngp->ng_flags, NGF_SENT_REPOLL) &&
	    tip->task_timer_next_time - ngp->ng_timer_t2->task_timer_last_time >= ngp->ng_T1) {
	    /* Time for a repoll */

	    egp_do_poll(ngp, FALSE);
	}

	/* Make sure our interval is correct (should not be necessary) */
	egp_set_timer(ngp,
		      ngp->ng_timer_t1,
		      ngp->ng_T1);
	break;

    case NGS_CEASE:
	egp_send_acquire(ngp, EGP_CODE_CEASE, ngp->ng_status, ngp->ng_S);
	egp_set_timer(ngp,
		      ngp->ng_timer_t1,
		      (time_t) EGP_P3);
	break;
    }
}


/*ARGSUSED*/
void
egp_event_t2 __PF2(tip, task_timer *,
		   interval, time_t)
{
    egp_neighbor *ngp;

    ngp = (egp_neighbor *) tip->task_timer_task->task_data;

    egp_msg_event("egp_event_t2", ngp, "t2");

    switch (ngp->ng_state) {
    case NGS_IDLE:
    case NGS_ACQUISITION:
    case NGS_DOWN:
    case NGS_CEASE:
	egp_msg_confused("egp_event_t2", ngp, "t2");
	break;

    case NGS_UP:
	egp_do_poll(ngp, FALSE);
	break;
    }
}


/*
 * egp_recv_acquire() handles received Neighbor Acquisition messages: Request, Confirm,
 * Refuse, Cease and Cease-ack.
 *
 */
static void
egp_recv_acquire __PF3(ngp, egp_neighbor *,
		       egp, egp_packet *,
		       egplen, size_t)
{
    int error = EGP_NOERROR;
    const char *msg = NULL;


    switch (egp->header.egp_code) {
    case EGP_CODE_ACQ_REQUEST:			/* Neighbor acquisition request */
	if (egplen != sizeof (egp_packet_acquire)) {
	    error = EGP_REASON_BADHEAD;
	    msg = "bad message length";
	    break;
	}
	egp_event_request(ngp, egp, egplen);
	break;

    case EGP_CODE_ACQ_CONFIRM:			/* Neighbor acq. confirm */
	if (egplen != sizeof (egp_packet_acquire)) {
	    error = EGP_REASON_BADHEAD;
	    msg = "bad message length";
	    break;
	}
	if (egprid_h != ngp->ng_S) {
	    /* Ignore packets with invalid sequence number */
	    break;
	}
	egp_event_confirm(ngp, egp, egplen);
	break;

    case EGP_CODE_ACQ_REFUSE:			/* Neighbor acq. refuse */
	if (egplen != sizeof (egp_packet_header) && egplen != sizeof (egp_packet_acquire)) {
	    error = EGP_REASON_BADHEAD;
	    msg = "bad message length";
	    break;
	}
	if (egprid_h != ngp->ng_S) {
	    /* Ignore packets with invalid sequence number */
	    break;
	}
	egp_event_refuse(ngp, egp, egplen);
	break;

    case EGP_CODE_CEASE:			/* Neighbor acq. cease */
	if (egplen != sizeof (egp_packet_header) && egplen != sizeof (egp_packet_acquire)) {
	    error = EGP_REASON_BADHEAD;
	    msg = "bad message length";
	    break;
	}
	egp_event_cease(ngp, egp, egplen);
	break;

    case EGP_CODE_CEASE_ACK:			/* Neighbor acq. cease ack */
	if (egplen != sizeof (egp_packet_header) && egplen != sizeof (egp_packet_acquire)) {
	    error = EGP_REASON_BADHEAD;
	    msg = "bad message length";
	    break;
	}
	if (egprid_h != ngp->ng_S) {
	    /* Ignore packets with invalid sequence number */
	    break;
	}
	egp_event_ceaseack(ngp, egp, egplen);
	break;

    default:
	error = EGP_REASON_BADHEAD;
	msg = "invalid Code field";
	break;
    }
    if (error != EGP_NOERROR) {
	egp_send_error(ngp, egp, egplen, error, msg);
	egp_stats.inerrors++;
	egp_stats.inmsgs--;
	ngp->ng_stats.inerrors++;
	ngp->ng_stats.inmsgs--;
    }
    return;
}


/*
 * egp_recv_neighbor() processes received hello packet
 */
static void
egp_recv_neighbor __PF3(ngp, egp_neighbor *,
			egp, egp_packet *,
			egplen, size_t)
{

    switch (egp->header.egp_code) {
    case EGP_CODE_HELLO:
	ngp->ng_R = egprid_h;
	egp_event_hello(ngp, egp, egplen);
	break;

    case EGP_CODE_HEARDU:
	if (egprid_h != ngp->ng_S_lasthello) {
	    /* Ignore packets with bad sequence numbers */
	    break;
	}
	egp_event_heardu(ngp, egp, egplen);
	break;

    default:
	egp_send_error(ngp, egp, egplen, EGP_REASON_BADHEAD, "invalid Code field");
	egp_stats.inerrors++;
	egp_stats.inmsgs--;
	ngp->ng_stats.inerrors++;
	ngp->ng_stats.inmsgs--;
	break;
    }
    return;
}


/*ARGSUSED*/
static void
egp_recv_error __PF3(ngp, egp_neighbor *,
		     egp, egp_packet *,
		     egplen, size_t)
{
    const char *err_msg;
    int reason;
    egp_packet_error *ee = (egp_packet_error *) egp;

    reason = htons(ee->ee_rsn);

    if (reason > EGP_REASON_UVERSION) {
	err_msg = "(invalid reason)";
    } else {
	err_msg = egp_reasons[reason];
    }

    ngp->ng_stats.inerrmsgs++;

    trace_log_tf(ngp->ng_trace_options,
		 0,
		 LOG_WARNING,
		 ("egp_recv_error: neighbor %s AS %d state %s error %s",
		  ngp->ng_name,
		  ngp->ng_peer_as,
		  trace_state(egp_states, ngp->ng_state),
		  err_msg));

    switch (reason) {
    case EGP_REASON_UNSPEC:
    case EGP_REASON_BADHEAD:
    case EGP_REASON_BADDATA:
    case EGP_REASON_XSPOLL:
    case EGP_REASON_NORESPONSE:
	break;

    case EGP_REASON_UVERSION:
	switch (ngp->ng_state) {
	case NGS_IDLE:
	case NGS_DOWN:
	case NGS_UP:
	case NGS_CEASE:
	    break;

	case NGS_ACQUISITION:
	    if (egp->header.egp_ver != ngp->ng_V) {
		egp_set_version(ngp, egp->header.egp_ver);
	    }
	    break;
	}
	break;

    case EGP_REASON_NOREACH:
	switch (ngp->ng_state) {
	case NGS_IDLE:
	case NGS_ACQUISITION:
	case NGS_CEASE:
	case NGS_DOWN:
	    break;

	case NGS_UP:
	    egp_event_stop(ngp, EGP_STATUS_GOINGDOWN);
	    break;
	}
	break;
    }
}


static int
egp_check_packet __PF4(ngp, egp_neighbor *,
		       ip, struct ip *,
		       egp, egp_packet *,
		       egplen, size_t)
{
    /* Remove IP header from length */
    egplen -= task_parse_ip_hl(ip);

    if (egplen != ip->ip_len) {
	trace_log_tf(ngp->ng_trace_options,
		     0,
		     LOG_ERR,
		     ("egp_check_packet: length mismatch: read: %u, ip_len: %u",
		      egplen,
		      ip->ip_len));
	return 0;
    }
    
    if (ip->ip_off & ~IP_DF) {
	trace_log_tf(ngp->ng_trace_options,
		     0,
		     LOG_ERR,
		     ("egp_check_packet: recv fragmanted pkt from %A",
		      task_recv_srcaddr));
	return 0;
    }

    if (TRACE_PACKET_RECV_TP(ngp->ng_task,
			     egp->header.egp_type,
			     EGP_PKT_MAX,
			     egp_trace_masks)) {

	egp_trace(ngp,
		  "EGP RECV",
		  FALSE,
		  egp,
		  MIN(egplen, task_recv_buffer_len),
		  TRACE_DETAIL_RECV_TP(ngp->ng_task,
				       egp->header.egp_type,
				       EGP_PKT_MAX,
				       egp_trace_masks));
    }

    /* Increase receive buffer size by 25% if it is too small */
    if (egplen > task_recv_buffer_len) {
	if ((egplen + IP_MAXHDRLEN) <= egp_maxpacket) {
	    task_alloc_recv(ngp->ng_task, egplen);
	    trace_log_tf(ngp->ng_trace_options,
			 0,
			 LOG_WARNING,
			 ("egp_check_packet: packet too big from %A, increased packet buffer size to %d",
			  task_recv_srcaddr,
			  task_recv_buffer_len));
	} else {
	    trace_log_tf(ngp->ng_trace_options,
			 0,
			 LOG_WARNING,
			 ("egp_check_packet: packet too big from %A",
			  task_recv_srcaddr));
	}
	return 0;
    }

    /* Update the timer on the interface we use for this peer */
    if_rtupdate(ngp->ng_ifap);

    if (inet_cksum((void_t) egp, egplen)) {
	trace_log_tf(ngp->ng_trace_options,
		     0,
		     LOG_WARNING,
		     ("egp_check_packet: bad EGP checksum from %A",
		      task_recv_srcaddr));
	BIT_SET(ngp->ng_gw.gw_flags, GWF_CHECKSUM);
	return 0;
    }
    if (egplen < sizeof (egp_packet_header)) {
	trace_log_tf(ngp->ng_trace_options,
		     0,
		     LOG_WARNING,
		     ("egp_check_packet: bad pkt length %d from %A",
		      egplen,
		      task_recv_srcaddr));
	BIT_SET(ngp->ng_gw.gw_flags, GWF_FORMAT);
	return 0;
    }
    return egplen;
}


static int
egp_check_version __PF3(ngp, egp_neighbor *,
			egp, egp_packet *,
			egplen, size_t)
{
    int error = EGP_NOERROR;
    const char *msg = NULL;

    if (!(EGPVMASK & (1 << (egp->header.egp_ver - 2)))) {
	if (egp->header.egp_type != EGP_PKT_ERROR) {
	    error = EGP_REASON_UVERSION;
	    msg = "unsupported version";
	}
    } else if (ngp && (egp->header.egp_ver != ngp->ng_V)) {
	switch (egp->header.egp_type) {
	case EGP_PKT_ACQUIRE:
	    egp_set_version(ngp, egp->header.egp_ver);
	    break;

	case EGP_PKT_HELLO:
	case EGP_PKT_NR:
	case EGP_PKT_POLL:
	    error = EGP_REASON_BADHEAD;
	    msg = "invalid Version field";
	    break;

	case EGP_PKT_ERROR:
	    /* Fall through and let egp_recv's switch switch versions */
	    break;
	}
    }
    if (error != EGP_NOERROR) {
	egp_send_error(ngp, egp, egplen, error, msg);
	return 1;
    } else {
	return 0;
    }
}


/*
 *	Process and incoming EGP packet from a known neighbor
 */
/*ARGSUSED*/
void
egp_recv __PF1(tp, task *)
{
    int n_packets = TASK_PACKET_LIMIT;
    egp_neighbor *ngp = (egp_neighbor *) tp->task_data;

    while (n_packets--
	   && ngp->ng_task
	   && ngp->ng_task->task_socket != -1) {
	register egp_packet *egp;
	register struct ip *ip;
	size_t egplen;
	size_t count;

	switch (task_receive_packet(tp, &count)) {
	case TASKRC_OK:
	    break;

	case TASKRC_TRUNC:
	    /* We'll notice that we have not received everything */
	    break;

	case TASKRC_EOF:
	default:
	    /* System error - end loop */
	    return;
	}

	task_parse_ip(ip, egp, egp_packet *);

	egplen = egp_check_packet(ngp, ip, egp, count);
	if (!egplen) {
	    egp_stats.inerrors++;
	    ngp->ng_stats.inerrors++;
	    continue;
	}
	egprid_h = ntohs(egp->header.egp_id);	/* save sequence number in host byte order */

	if (egp_check_version(ngp, egp, egplen)) {
	    egp_stats.inerrors++;
	    ngp->ng_stats.inerrors++;
	    continue;
	}
	egp_stats.inmsgs++;
	ngp->ng_stats.inmsgs++;

	switch (egp->header.egp_type) {
	case EGP_PKT_ACQUIRE:
	    egp_recv_acquire(ngp, egp, egplen);
	    break;

	case EGP_PKT_HELLO:
	    egp_recv_neighbor(ngp, egp, egplen);
	    break;

	case EGP_PKT_NR:
	    egp_event_update(ngp, egp, egplen);
	    break;

	case EGP_PKT_POLL:
	    egp_event_poll(ngp, egp, egplen);
	    break;

	case EGP_PKT_ERROR:
	    egp_recv_error(ngp, egp, egplen);
	    break;

	default:
	    BIT_SET(ngp->ng_gw.gw_flags, GWF_FORMAT);
	    egp_send_error(ngp, egp, egplen, EGP_REASON_BADHEAD, "invalid Type field");
	    egp_stats.inerrors++;
	    egp_stats.inmsgs--;
	    ngp->ng_stats.inerrors++;
	    ngp->ng_stats.inmsgs--;
	}
    }
}


/*
 *	Compare the old neighbor's configuration with the new configuration.
 *	Some options may be changed on the fly, some require the neighbor to be restarted.
 */
int
egp_neighbor_changed __PF2(ngpo, egp_neighbor *,
			   ngpn, egp_neighbor *)
{
    int changed = FALSE;
    flag_t changed_options = ngpo->ng_options ^ ngpn->ng_options;
    flag_t new_options = ngpn->ng_options;

    /* XXX - What about maxacquire? */
    /* ifa_change should take care of lcladdr */

    if (changed_options & (NGO_PEERAS | NGO_LOCALAS | NGO_GATEWAY)) {
	changed = TRUE;
    } else if (BIT_TEST(new_options, NGO_PEERAS)
	&& ngpn->ng_peer_as != ngpo->ng_peer_as) {
	changed = TRUE;
    } else if (BIT_TEST(new_options, NGO_LOCALAS)
	&& ngpn->ng_local_as != ngpo->ng_local_as) {
	changed = TRUE;
    } else if (BIT_TEST(new_options, NGO_GATEWAY)
	&& !sockaddrcmp_in(ngpn->ng_gateway, ngpo->ng_gateway)) {
	changed = TRUE;
    } else if (BIT_TEST(new_options, NGO_P1)
	&& ngpn->ng_P1 != ngpo->ng_P1) {
	changed = TRUE;
    } else if (BIT_TEST(new_options, NGO_P2)
	&& ngpn->ng_P2 != ngpo->ng_P2) {
	changed = TRUE;
    }
    if (!changed) {
	/* Nothing has changed that has required a restart.  Let's deal with things */
	/* that can be changed on the fly */

	/* Default propagation and generation options can be changed by just changing the flags */
#define	FLAGS	(NGO_NOGENDEFAULT|NGO_DEFAULTIN|NGO_DEFAULTOUT|NGO_METRICOUT|NGO_SADDR|NGO_VERSION|NGO_PREFERENCE|NGO_PREFERENCE2)
	BIT_RESET(ngpo->ng_options, FLAGS);
	BIT_SET(ngpo->ng_options, BIT_TEST(ngpn->ng_options, FLAGS));
	BIT_RESET(changed_options, FLAGS);
#undef	FLAGS

	ngpo->ng_metricout = ngpn->ng_metricout;
	ngpo->ng_preference = ngpn->ng_preference;
	ngpo->ng_preference2 = ngpn->ng_preference2;

	trace_set(ngpo->ng_trace_options, egp_trace_options);
	
	if (BIT_TEST(ngpn->ng_options, NGO_SADDR)) {
	    if (ngpo->ng_saddr) {
		sockfree(ngpo->ng_saddr);
	    }
	    ngpo->ng_saddr = ngpn->ng_saddr;
	}
	if (BIT_TEST(ngpn->ng_options, NGO_VERSION)) {
	    if (ngpo->ng_paddr) {
		sockfree(ngpo->ng_paddr);
		ngpo->ng_paddr = (sockaddr_un *) 0;
	    }
	    ngpo->ng_version = ngpn->ng_version;
	}
 	if (BIT_TEST(changed_options, NGO_TTL)
	    || BIT_TEST(ngpo->ng_options, NGO_TTL)) {
 	    if (ngpo->ng_task->task_socket >= 0) {
 		int local = if_withdst(ngpo->ng_addr) ? TRUE : FALSE;
 		int old = (ngpo->ng_ttl ? ngpo->ng_ttl : (local ? 1 : 255));
 		int new = (ngpn->ng_ttl ? ngpn->ng_ttl : (local ? 1 : 255));
		
 		if (old != new) {
 		    (void) task_set_option(ngpo->ng_task,
 					   TASKOPTION_TTL,
 					   new);
 		}
 	    }
 	    if (BIT_TEST(ngpn->ng_options, NGO_TTL)) {
 		BIT_SET(ngpo->ng_options, NGO_TTL);
 		ngpo->ng_ttl = ngpn->ng_ttl;
 	    } else {
 		BIT_RESET(ngpo->ng_options, NGO_TTL);
 		ngpo->ng_ttl = 0;
 	    }
	}
    }
    return changed;
}


static void
egp_dump_rate __PF3(fd, FILE *,
		    rp, struct egp_rate *,
		    name, const char *)
{
    int i;

    (void) fprintf(fd, "\t\t%s: rate_min: %#T rate_last: %#T\n",
		   name,
		   rp->rate_min,
		   rp->rate_last);
    (void) fprintf(fd, "\t\t\twindow:");

    for (i = 0; i < RATE_WINDOW; i++) {
	(void) fprintf(fd, " %#T", rp->rate_window[i]);
    }
    (void) fprintf(fd, "\n");
}


void
egp_ngp_dump __PF2(tp, task *,
		   fd, FILE *)
{
    egp_neighbor *ngp = (egp_neighbor *) tp->task_data;

    (void) fprintf(fd, "\n\t%s\tGroup: %d\tV: %d",
		   ngp->ng_name,
		   ngp->ng_gr_head->ng_gr_index,
		   ngp->ng_V);
    if (ngp->ng_lcladdr) {
	(void) fprintf(fd, "\tLocal Address: %A\t",
		       ngp->ng_lcladdr->ifae_addr);
    }
    if (ngp->ng_ifap) {
	(void) fprintf(fd, "\tInterface: %A\n",
		       ngp->ng_ifap->ifa_addr_local);
    } else if (ngp->ng_lcladdr) {
	(void) fprintf(fd, "\t");
    }
    (void) fprintf(fd, "\t\tReachability: [%04B] %d\tj: %d\tk: %d\n",
		   ngp->ng_responses,
		   egp_reachability[ngp->ng_responses],
		   ngp->ng_j,
		   ngp->ng_k);
    (void) fprintf(fd, "\t\tT1: %#T\tT2: %#T\n",
		   ngp->ng_T1,
		   ngp->ng_T2);
    (void) fprintf(fd, "\t\tt1: %T\tt2: %T\tt3: %T\n",
		   ngp->ng_timer_t1->task_timer_next_time,
		   ngp->ng_timer_t2->task_timer_next_time,
		   ngp->ng_timer_t3->task_timer_next_time);
    (void) fprintf(fd, "\t\tP1: %#T\tP2: %#T\tP3: %#T\tP4: %#T\tP5: %#T\n",
		   ngp->ng_P1,
		   ngp->ng_P2,
		   EGP_P3,
		   EGP_P4,
		   EGP_P5);
    (void) fprintf(fd, "\t\tState: <%s>\tMode: %s\n", trace_state(egp_states, ngp->ng_state), egp_acq_status[ngp->ng_M]);
    (void) fprintf(fd, "\t\tFlags: %#x <%s>\n", ngp->ng_flags, trace_bits(egp_flags, ngp->ng_flags));
    (void) fprintf(fd, "\t\tOptions: %#x <%s>\n", ngp->ng_options, trace_bits(egp_options, ngp->ng_options));
    (void) fprintf(fd, "\t\tLast poll received: %A",
		   ngp->ng_paddr);
    (void) fprintf(fd, "\tNet to poll: %A\n",
		   ngp->ng_saddr);
    (void) fprintf(fd, "\t\tMetricOut: ");
    if (BIT_TEST(ngp->ng_options, NGO_METRICOUT)) {
	(void) fprintf(fd, "%d", ngp->ng_metricout);
    } else {
	(void) fprintf(fd, "N/A");
    }
    (void) fprintf(fd, "\tGateway: ");
    if (BIT_TEST(ngp->ng_options, NGO_GATEWAY)) {
	(void) fprintf(fd, "%A",
		       ngp->ng_gateway);
    } else {
	(void) fprintf(fd, "N/A");
    }
    (void) fprintf(fd, "\n");
    egp_dump_rate(fd, &ngp->ng_poll_rate, "Poll");
    egp_dump_rate(fd, &ngp->ng_hello_rate, "Hello");
    (void) fprintf(fd, "\t\tUpdate packet size needed: %u\tset to: %u\n",
		   ngp->ng_length,
		   ngp->ng_send_size);
    (void) fprintf(fd, "\t\tPackets In: %u\t\t\tErrors In: %u\n",
		   ngp->ng_stats.inmsgs,
		   ngp->ng_stats.inerrors);
    (void) fprintf(fd, "\t\tPackets Out: %d\t\t\tErrors Out: %d\n",
		   ngp->ng_stats.outmsgs,
		   ngp->ng_stats.outerrors);
    (void) fprintf(fd, "\n");

    /* Dump the annoucement list */
    egp_rt_dump(fd, ngp);
}


/*
 *	Dump EGP status to dump file
 */
void
egp_dump __PF2(tp, task *,
	       fd, FILE *)
{
    egp_neighbor *ngp;
    egp_neighbor *gr_ngp = (egp_neighbor *) 0;
    
    /*
     *	EGP neighbor status
     */
    if (doing_egp) {
	(void) fprintf(fd, "\tdefaultegpmetric: %d\tpreference: %d\tpreference2: %d\n",
		       egp_default_metric,
		       egp_preference,
		       egp_preference2);
	(void) fprintf(fd, "\tPacket size: %u\tMaximum system packet size: %u\n",
		       egp_pktsize,
		       egp_maxpacket);
	(void) fprintf(fd, "\tPackets In: %u\t\t\tErrors In: %u\n",
		       egp_stats.inmsgs,
		       egp_stats.inerrors);
	(void) fprintf(fd, "\tPackets Out: %u\t\t\tErrors Out: %u\n",
		       egp_stats.outmsgs,
		       egp_stats.outerrors);
	(void) fprintf(fd, "\t\t\t\t\tTotal Errors: %u\n\n",
		       egp_stats.outerrors + egp_stats.inerrors);
	if (egp_import_list) {
	    control_exterior_dump(fd, 1, control_import_dump, egp_import_list);
	}
	if (egp_export_list) {
	    control_exterior_dump(fd, 1, control_export_dump, egp_export_list);
	}

	EGP_LIST(ngp) {
	    if (gr_ngp != ngp->ng_gr_head) {
		gr_ngp = ngp->ng_gr_head;
		(void) fprintf(fd, "\n\tGroup: %d\tMembers: %d\tAcquire: %d\tAcquired: %d",
			       gr_ngp->ng_gr_index,
			       gr_ngp->ng_gr_number,
			       gr_ngp->ng_gr_acquire,
			       egp_group_acquired(ngp));
		if (BIT_TEST(gr_ngp->ng_options, NGO_PREFERENCE)) {
		    (void) fprintf(fd, "\n\t\tPreference: %u",
				   gr_ngp->ng_preference);
		}
		if (BIT_TEST(gr_ngp->ng_options, NGO_PREFERENCE2)) {
		    (void) fprintf(fd, "\tPreference2: %u",
				   gr_ngp->ng_preference2);
		}
		(void) fprintf(fd, "\n\t\t\tLocal AS: ");
		if (BIT_TEST(gr_ngp->ng_options, NGO_LOCALAS)) {
		    (void) fprintf(fd, "%u",
				   gr_ngp->ng_local_as);
		} else {
		    (void) fprintf(fd, "N/A");
		}
		(void) fprintf(fd, "\tPeer AS: ");
		if (BIT_TEST(gr_ngp->ng_options, NGO_PEERAS) || gr_ngp->ng_peer_as) {
		    (void) fprintf(fd, "%u",
				   gr_ngp->ng_peer_as);
		} else {
		    (void) fprintf(fd, "N/A");
		}
		(void) fprintf(fd, "\n");
	    }
	    (void) fprintf(fd, "\t\t%s\n",
			   ngp->ng_name);
	} EGP_LIST_END(ngp) ;
    }
}
