#ident	"@(#)ospf_init.c	1.3"
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


#define	INCLUDE_CTYPE
#define	INCLUDE_TIME
#include "include.h"
#include "inet.h"
#include "ospf.h"
#include "krt.h"

struct OSPF ospf = { 0 };

static int ospf_proto = 0;

PROTOTYPE(ospf_startup,
	  static void,
	  (void));

const flag_t ospf_trace_masks[OSPF_PKT_MAX] = {
    TR_ALL,	/* XXX */		/* 0 - Monitor */
    TR_OSPF_DETAIL_HELLO,	/* 1 - Hello */
    TR_OSPF_DETAIL_DD,		/* 2 - Database Description */
    TR_OSPF_DETAIL_LSR,		/* 3 - Link State Request */
    TR_OSPF_DETAIL_LSU,		/* 4 - Link State Update */
    TR_OSPF_DETAIL_ACK,		/* 5 - Link State Ack */
} ;

static const bits ospf_area_bits[] = {
    { OSPF_AREAF_TRANSIT, 	"Transit" },
    { OSPF_AREAF_VIRTUAL_UP, 	"VirtualUp" },
    { OSPF_AREAF_STUB, 		"Stub" },
    { OSPF_AREAF_NSSA, 		"NSSA" },
    { OSPF_AREAF_STUB_DEFAULT,	"StubDefault" },
    { 0, NULL }
} ;

/* Memory allocation */
block_t	ospf_router_index = (block_t) 0;	/* For allocating router config block */
block_t	ospf_intf_index = (block_t) 0; 		/* For allocating interface structures */
block_t	ospf_area_index = (block_t) 0;		/* For allocating area structures */
block_t ospf_nbr_index = (block_t) 0;		/* For allocating neighbor structures */
block_t ospf_nbr_node_index = (block_t) 0;	/* For allocating neighbor tree structures */
block_t ospf_nh_block_index = (block_t) 0;	/* For allocating next hop blocks */
block_t ospf_lsdb_index = (block_t) 0;
block_t ospf_route_index = (block_t) 0;
block_t ospf_dbsum_index = (block_t) 0;
block_t ospf_netrange_index = (block_t) 0;
block_t ospf_hosts_index = (block_t) 0;
block_t ospf_hdrq_index = (block_t) 0;
block_t ospf_lsdblist_index = (block_t) 0;
block_t ospf_nbrlist_index = (block_t) 0;
block_t ospf_lsreq_index = (block_t) 0;
block_t ospf_auth_index = (block_t) 0;

block_t *ospf_lsa_index_4;
block_t *ospf_lsa_index_16;

/**/
#ifdef	IP_MULTICAST
static if_addr *ospf_multicast_ifap;

sockaddr_un *ospf_addr_allspf = 0;
sockaddr_un *ospf_addr_alldr = 0;

static const bits ospf_if_bits[] = {
    { OSPF_IFPS_ALLSPF,	"AllSPF" },
    { OSPF_IFPS_ALLDR,	"AllDR" },
    { 0 }
} ;

static void
ospf_multicast_init __PF1(tp, task *)
{
    /* Disable reception of our own packets */
    if (task_set_option(tp,
			TASKOPTION_MULTI_LOOP,
			FALSE) < 0) {
	task_quit(errno);
    }

    if (!ospf_addr_allspf) {
	/* Initialize address constants */
	ospf_addr_allspf = sockdup(sockbuild_in(0, htonl(OSPF_ADDR_ALLSPF)));
	ospf_addr_alldr = sockdup(sockbuild_in(0, htonl(OSPF_ADDR_ALLDR)));
    }

    krt_multicast_add(ospf_addr_allspf);
    krt_multicast_add(ospf_addr_alldr);
}


static void
ospf_multicast_cleanup __PF1(tp, task *)
{
    krt_multicast_delete(ospf_addr_allspf);
    krt_multicast_delete(ospf_addr_alldr);
}


void
ospf_multicast_alldr __PF2(intf, struct INTF *,
			   add, int)
{
    if_addr *ifap = intf->ifap;
    
    if (add) {

	assert(intf->pri
	       && !BIT_TEST(ifap->ifa_ps[ospf.task->task_rtproto].ips_state, OSPF_IFPS_ALLDR));
	
	if ((task_set_option(ospf.task,
			     TASKOPTION_GROUP_ADD,
			     ifap,
			     ospf_addr_alldr) < 0)
	    && (errno != EADDRINUSE)) {
	    /* Failure other than the address is already there */

	    task_quit(errno);
	}

	/* Indicate we joined the group */
	BIT_SET(ifap->ifa_ps[RTPROTO_OSPF].ips_state, OSPF_IFPS_ALLDR);
    } else {

	assert(BIT_TEST(ifap->ifa_ps[RTPROTO_OSPF].ips_state, OSPF_IFPS_ALLDR));
	
	(void) task_set_option(ospf.task,
			       TASKOPTION_GROUP_DROP,
			       ifap,
			       ospf_addr_alldr);

	/* Indicate we are not longer a member of the group */
	BIT_RESET(ifap->ifa_ps[RTPROTO_OSPF].ips_state, OSPF_IFPS_ALLDR);
    }
}
#endif	/* IP_MULTICAST */

/* Packet reception */

/*
 *	Receive a packet and figure out who it is from and to
 */
static void
ospf_recv __PF1(tp, task *)
{
    int n_packets = TASK_PACKET_LIMIT;
    size_t len;
    sockaddr_un *dst = (sockaddr_un *) 0;

    while (n_packets-- && !task_receive_packet(tp, &len)) {
	register struct ip *ip;
	register struct OSPF_HDR *o_hdr;

	task_parse_ip(ip, o_hdr, struct OSPF_HDR *);
	
	/* Build destination address */
	if (!dst || sock2ip(dst) != ip->ip_dst.s_addr) {
	    if (dst) {
		sockfree(dst);
	    }
	    dst = sockdup(sockbuild_in(0, ip->ip_dst.s_addr));
	}
		
	/* Process the packet */
	ospf_rxpkt(ip,
		   o_hdr,
		   task_recv_srcaddr,
		   dst);
    }

    /* Free destination address */
    if (dst) {
	sockfree(dst);
    }
}


/**/


void
ospf_txpkt __PF6(pkt, struct OSPF_HDR *,
		 intf, struct INTF *,
		 type, u_int,
		 length, size_t,
		 to, u_int32,
		 retrans, int)
{
    struct NBR *nbr;
    flag_t flag;
    sockaddr_un *src = intf->ifap->ifa_addr_local;
    sockaddr_un *dst = sockdup(sockbuild_in(0, to));

    pkt->ospfh_version = OSPF_VERSION;
    pkt->ospfh_type = type;
    pkt->ospfh_length = htons((u_int16) length);
    pkt->ospfh_rtr_id = MY_ID;
    pkt->ospfh_checksum = 0;
    pkt->ospfh_auth_type = htons(intf->auth.auth_type);

    if (intf->type == VIRTUAL_LINK) {
	pkt->ospfh_area_id = OSPF_BACKBONE;
	flag = 0;
    } else {
	pkt->ospfh_area_id = intf->area->area_id;
	flag = MSG_DONTROUTE;
    }

    switch (intf->auth.auth_type) {
	u_int32 *dp;
	
    case OSPF_AUTH_NONE:
	pkt->ospfh_auth_key[0] = pkt->ospfh_auth_key[1] = 0;
	pkt->ospfh_checksum = inet_cksum((void_t) pkt, length);
	break;

    case OSPF_AUTH_SIMPLE:
	pkt->ospfh_auth_key[0] = pkt->ospfh_auth_key[1] = 0;
	pkt->ospfh_checksum = inet_cksum((void_t) pkt, length);
	pkt->ospfh_auth_key[0] = intf->auth.auth_key[0];
	pkt->ospfh_auth_key[1] = intf->auth.auth_key[1];
	break;

    case OSPF_AUTH_MD5:
	/* Init MD5 fields */
	pkt->ospfh_md5_offset = pkt->ospfh_length;		/* Offset is length */
	pkt->ospfh_md5_instance = htons(0);			/* XXX - Don't support instances */
	pkt->ospfh_md5_sequence = htonl((u_long) (time_boot + time_sec));	/* Sequence is current time */

	/* Append secret to the message */
	dp = (u_int32 *) ((void_t) ((byte *) pkt + length));
	dp[0] = intf->auth.auth_key[0]; dp[1] = intf->auth.auth_key[1];
	dp[2] = intf->auth.auth_key[2]; dp[3] = intf->auth.auth_key[3];

	/* Add trailer to length */
	length += OSPF_AUTH_MD5_SIZE;

	/* Calculate the MD5 checksum */
	md5_cksum(pkt, 
		  length,
		  length,
		  dp,
		  (u_int32 *) 0);

	break;
    }
    
    /* Log the packet */
    if (TRACE_PACKET_SEND(ospf.trace_options,		/* XXX - Per area and interface */
			  type,
			  OSPF_PKT_MAX,
			  ospf_trace_masks)) {
	ospf_trace(pkt,
		   length,
		   type,
		   FALSE,
		   intf,
		   src,
		   dst,
		   TRACE_DETAIL_SEND(ospf.trace_options,
				     type,
				     OSPF_PKT_MAX,
				     ospf_trace_masks));
    }
    
#define	ospf_send_packet(pkt, type, len, flag, addr) \
    if (task_send_packet(ospf.task, (void_t) pkt, len, flag, addr) < 0) { \
	trace_log_tf(ospf.trace_options, \
		     0, \
		     LOG_ERR, \
		     ("OSPF SENT %-15A -> %-15A: %m", \
		      src, \
		      addr)); \
        OSPF_LOG_RECORD(OSPF_ERR_OSPF_SEND); \
    } else { \
        OSPF_LOG_RECORD_TX(type); \
    } \

															  
    /* Handle NBMA send cases */
    switch (to) {
    case ALL_UP_NBRS:
	for (nbr = intf->nbr.next; nbr != NBRNULL; nbr = nbr->next) {
	    if (nbr->state != NDOWN) {
		ospf_send_packet(pkt, type, length, flag, nbr->nbr_addr);
	    }
	}
	break;

    case ALL_ELIG_NBRS:
	for (nbr = intf->nbr.next; nbr != NBRNULL; nbr = nbr->next) {
	    if (nbr->pri) {
		ospf_send_packet(pkt, type, length, flag, nbr->nbr_addr);
	    }
	}
	break;

    case ALL_EXCH_NBRS:
	for (nbr = intf->nbr.next; nbr != NBRNULL; nbr = nbr->next) {
	    if (nbr->state >= NEXCHANGE) {
		ospf_send_packet(pkt, type, length, flag, nbr->nbr_addr);
	    }
	}
	break;

    case DR_and_BDR:
	if (intf->dr) {
	    ospf_send_packet(pkt, type, length, flag, intf->dr->nbr_addr);
	}
	if (intf->bdr) {
	    ospf_send_packet(pkt, type, length, flag, intf->bdr->nbr_addr);
	}
	break;

    default:
#ifdef	IP_MULTICAST
	if (inet_class_of(dst) == INET_CLASSC_MULTICAST) {
	    /* Multicast sends fail if MSG_DONTROUTE is set */
	    flag = 0;

	    /* Set the source address of multicast packets to be this interface */
	    if (ospf_multicast_ifap != intf->ifap) {
		IFA_FREE(ospf_multicast_ifap);
		IFA_ALLOC(ospf_multicast_ifap = intf->ifap);
		if (task_set_option(ospf.task,
				    TASKOPTION_MULTI_IF,
				    ospf_multicast_ifap) < 0) {
		    /* Interface may have gone away, we should detect it soon */
		    
		    goto Return;
		}
	    }
	}
#endif	/* IP_MULTICAST */
	    
	ospf_send_packet(pkt, type, length, flag, dst);
	break;
    }

#ifdef	IP_MULTICAST
 Return:
#endif	/* IP_MULTICAST */
    sockfree(dst);
#undef	ospf_send_packet
}


/**/
/* Initialization */

const bits ospf_router_type_bits[] = {
    { RTR_IF_TYPE_RTR, "Router" },
    { RTR_IF_TYPE_TRANS_NET, "Transit net" },
    { RTR_IF_TYPE_STUB_NET, "Stub net" },
    { RTR_IF_TYPE_VIRTUAL, "Virtual" },
    {0}
};


const bits ospf_if_type_bits[] = {
    { BROADCAST, "Broadcast" },
    { NONBROADCAST, "NBMA" },
    { POINT_TO_POINT, "PointToPoint" },
    { VIRTUAL_LINK, "Virtual" },
    {0}
};

const bits ospf_if_state_bits[] = {
    { IDOWN, "Down" },
    { ILOOPBACK, "Loopback" },
    { IWAITING, "Waiting" },
    { IPOINT_TO_POINT, "P To P" },
    { IDr, "DR" },
    { IBACKUP, "BackupDR" },
    { IDrOTHER, "DR Other" },
    {0}
};


const bits ospf_neighbor_mode_bits[] = {
    { 0, "None" },
    { SLAVE, "Slave" },
    { MASTER, "Master" },
    { 0, "Null" },
    { SLAVE_HOLD, "Hold" },
    {0}
};

const bits ospf_neighbor_state_bits[] = {
    { NDOWN, "Down" },
    { NATTEMPT, "Attempt" },
    { NINIT, "Init" },
    { N2WAY, "2 Way" },
    { NEXSTART, "Exch Start" },
    { NEXCHANGE, "Exchange" },
    { NLOADING, "Loading" },
    { NFULL, "Full" },
    { 8, "SCVirtual" },
    {0}
};

const bits ospf_change_bits[] = {
    { E_UNCHANGE,		"UnChanged" },
    { E_NEW,			"New" },
    { E_NEXTHOP,		"NextHop" },
    { E_METRIC,			"Metric" },
    { E_WAS_INTER_NOW_INTRA,	"WasInterNowIntra" },
    { E_WAS_INTRA_NOW_INTER,	"WasIntraNowInter" },
    { E_ASE_METRIC,		"AseMetric" },
    { E_WAS_ASE,		"WasAse" },
    { E_WAS_INTRA_NOW_ASE,	"WasIntraNowAse" },
    { E_WAS_INTER_NOW_ASE,	"WasInterNowAse" },
    { E_ASE_TYPE ,		"AseType" },
    { E_ASE_TAG ,		"AseTag" },
    { E_DELETE,			"Delete" },
    { 0 }
};

const bits ospf_nh_bits[] = {
    { 0 },
    { NH_DIRECT,		"Direct" },
    { NH_NBR,			"Neighbor" },
    { NH_LOCAL,			"Local" },
    { NH_DIRECT_FORWARD,	"Forward" },
    { 0 }
} ;


/*
 * Interface fini routine
 */
static void
ospf_interface_delete __PF1(intf, struct INTF *)
{
    if_addr *ifap = intf->ifap;
    struct ifa_ps *ips = &ifap->ifa_ps[RTPROTO_OSPF];

    /* Delete timers */
    if (intf->timer_hello) {
	task_timer_delete(intf->timer_hello);
	intf->timer_hello = (task_timer *) 0;
    }
    
    if (intf->timer_adjacency) {
	task_timer_delete(intf->timer_adjacency);
	intf->timer_adjacency = (task_timer *) 0;
    }
    
    if (intf->timer_retrans) {
	task_timer_delete(intf->timer_retrans);
	intf->timer_retrans = (task_timer *) 0;
    }
    
    /* Send hello with rhf reset to notify all neighbors were going away */
    if (intf->state > IDOWN) {
	send_hello(intf, 0, TRUE);
    }

    if (intf->type != VIRTUAL_LINK) { 
#ifdef	IP_MULTICAST
	if (BIT_TEST(intf->flags, OSPF_INTFF_MULTICAST)) {
	    /* Multicast may be enabled on this interface */

	    if (BIT_TEST(ifap->ifa_ps[RTPROTO_OSPF].ips_state, OSPF_IFPS_ALLSPF)) {
		/* Remove ourselves from the All SPF Routers group */

		(void) task_set_option(ospf.task,
				       TASKOPTION_GROUP_DROP,
				       ifap,
				       ospf_addr_allspf);
		BIT_RESET(ifap->ifa_ps[RTPROTO_OSPF].ips_state, OSPF_IFPS_ALLSPF);
	    } 
	    
	    if (BIT_TEST(ifap->ifa_ps[RTPROTO_OSPF].ips_state, OSPF_IFPS_ALLDR)) {
		/* Remove ourselves from the All Designated Routers group */

		ospf_multicast_alldr(intf, FALSE);
	    }
	}
	
	/* Reset cached interface if necesary */
	if (ospf_multicast_ifap == ifap) {
	    IFA_FREE(ospf_multicast_ifap);
	    ospf_multicast_ifap = (if_addr *) 0;
	}
#endif	/* IP_MULTICAST */

	/* Delete all neighbors for an NBMA interface */
	if (intf->type < POINT_TO_POINT) {
	    struct NBR *n;
	
	    while ((n = intf->nbr.next)) {
		intf->nbr.next = n->next;
		ospf_nbr_delete(intf, n);
	    }
	}

	/* Indicate that we are no longer active on this interface */
	BIT_RESET(ifap->ifa_rtactive, RTPROTO_BIT(RTPROTO_OSPF));

	ips->ips_state = (flag_t) 0;
	ips->ips_datas[0] = ips->ips_datas[1] = ips->ips_datas[2] = (void_t) 0;

	/* Unlink the interface from if_addr */
	ifap->ifa_ospf_intf = (void_t) 0;
	IFA_FREE(ifap);
	intf->ifap = (if_addr *) 0;

	intf->area->ifcnt--;
    } else {
	/* Virtual Link */

	if (ifap) {
	    IFA_FREE(ifap);
	    intf->ifap = (if_addr *) 0;
	}
	
	ospf.vcnt--;
    }

    /* Remove from list */
    REMQUE(intf);

    /* And free */
    task_block_free(ospf_intf_index, (void_t) intf);
}


/*
 * Interface init routine
 */
static void
ospf_interface_init __PF2(intf, struct INTF *,
			  offset, time_t)
{
    if_addr *ifap = intf->ifap;

    if (intf->type == VIRTUAL_LINK) {
    	/* Virtual Link - no interface */
    	
	trace_tf(ospf.trace_options,
		 TR_NORMAL,
		 0,
		 ("ospf_interface_init: initializing virtual interface to neighbor ID %A area %A",
		  intf->nbr.nbr_id,
		  sockbuild_in(0, intf->area->area_id)));
    } else {
    	/* Non-virtual interface */
    	
	trace_tf(ospf.trace_options,
		 TR_NORMAL,
		 0,
		 ("ospf_interface_init: initializing interface %A  area %A",
		  ifap->ifa_addr,
		  sockbuild_in(0, intf->area->area_id)));

	/* Set interface address pointer back to our interface structure */
	ifap->ifa_ospf_intf = (void_t) intf;

	/* Indicate there is a routing protocol active on this interface */
	BIT_SET(ifap->ifa_rtactive, RTPROTO_BIT(RTPROTO_OSPF));
    }

    if (intf->type != VIRTUAL_LINK) {

	/* Make sure our buffer sizes are large enough */
	task_alloc_recv(ospf.task, (size_t) ifap->ifa_mtu + IP_MAXHDRLEN);
	task_alloc_send(ospf.task, (size_t) ifap->ifa_mtu);

	switch (intf->type) {
	case POINT_TO_POINT:
	    /* Fall through */

#ifdef	IP_MULTICAST
	case BROADCAST:
	    if (BIT_TEST(intf->flags, OSPF_INTFF_MULTICAST)) {

		/* Configure the multicast groups for this interface */
		if ((task_set_option(ospf.task,
				     TASKOPTION_GROUP_ADD,
				     ifap,
				     ospf_addr_allspf) < 0)
		    && (errno != EADDRINUSE)) {
		    /* Failure other than the address is already there */

		    if (intf->type == POINT_TO_POINT) {
			/* This may be due to a bug in the Deering multicast code */
			/* that does not let you set the multicast address with the */
			/* remote end of a P2P link, just disable multicast for this */
			/* interface and continue */

			BIT_RESET(intf->flags, OSPF_INTFF_MULTICAST);
		    } else {
			/* Multicast is required on BROADCAST interfaces */

			task_quit(errno);
		    }
		} else {
		    /* Indicate we joined the group */
		    BIT_SET(ifap->ifa_ps[RTPROTO_OSPF].ips_state, OSPF_IFPS_ALLSPF);
		}
	    }
	    /* Fall through */
#endif	/* IP_MULTICAST */

	case NONBROADCAST:
	    break;

	default:
	    assert(FALSE);
	}

    }
	

    /* create timers */
    /* hello timer */
    intf->timer_hello =
	task_timer_create(ospf.task,	
			  "Hello",
			  TIMERF_HIPRIO,
			  intf->hello_timer,
			  offset % intf->hello_timer,
			  tq_hellotmr,
			  (void_t) intf);
    /* adjacency timer */
    intf->timer_adjacency =
	task_timer_create(ospf.task,	
			  "Adjacency",
			  0,
			  (time_t) 0,
			  intf->dead_timer - offset % intf->dead_timer,
			  tq_adjtmr,
			  (void_t) intf);
    /* retransmit timer */
    intf->timer_retrans =
	task_timer_create(ospf.task,
			  "Retransmit",
			  0,
			  intf->retrans_timer,
			  offset % intf->retrans_timer,
			  tq_retrans,
			  (void_t) intf);

    if (intf->type != VIRTUAL_LINK
	&& BIT_TEST(ifap->ifa_state, IFS_UP)
	&& !BIT_TEST(task_state, TASKS_TEST)) {
	/* Init this interface */
	
	(*(if_trans[INTF_UP][IDOWN]))(intf);
    }
}

static void
ospf_auth_dump __PF4(fp, FILE *,
		     oap, ospf_auth *,
		     p1, const char *,
		     p2, const char *)
{
    switch (oap->auth_type) {
    case OSPF_AUTH_NONE:
	(void) fprintf(fp, "%s%snone\n",
		       p1,
		       p2);
	break;

    case OSPF_AUTH_SIMPLE:
	(void) fprintf(fp, "%s%ssimple %08x.%08x\n",
		       p1,
		       p2,
		       oap->auth_key[0],
		       oap->auth_key[1]);
	break;

    case OSPF_AUTH_MD5:
	(void) fprintf(fp, "%s%smd5 %08x.%08x.%08x.%08x\n",
		       p1,
		       p2,
		       oap->auth_key[0],
		       oap->auth_key[1],
		       oap->auth_key[2],
		       oap->auth_key[3]);
	break;
    }
}


/*
 *	Display OSPF information
 */
static void
ospf_interface_dump __PF3(fp, FILE *,
			  intf, struct INTF *,
			  prefix, const char *)
{
    struct NBR *n;

    /* Interface Info */
    if (intf->ifap) {
	(void) fprintf(fp, "%sInterface: %A (%s)\n",
		       prefix,
		       intf->ifap->ifa_addr,
		       intf->ifap->ifa_link->ifl_name);
    }
    (void) fprintf(fp, "%s\tCost: %d\tState: %s\t\tType: %s\n",
		   prefix,
		   intf->cost,
		   trace_state(ospf_if_state_bits, intf->state),
		   trace_state(ospf_if_type_bits, intf->type-1));

    if (intf->type == VIRTUAL_LINK) {
	(void) fprintf(fp, "%s\tTransit Area: %A\n",
		       prefix,
		       sockbuild_in(0, intf->trans_area->area_id));
    } else {
	(void) fprintf(fp, "%s\tPriority: %d\n",
		       prefix,
		       intf->pri);
	if (intf->dr) {
	    (void) fprintf(fp, "%s\tDesignated Router: %A\n",
			   prefix,
			   intf->dr->nbr_addr);
	}
	if (intf->bdr) {
	    (void) fprintf(fp, "%s\tBackup Designated Router: %A\n",
			   prefix,
			   intf->bdr->nbr_addr);
	}
    }

    ospf_auth_dump(fp, &intf->auth, prefix, "\tAuthentication: ");
    if (BIT_TEST(intf->flags, OSPF_INTFF_SECAUTH)) {
	ospf_auth_dump(fp, &intf->auth2, prefix, "\t\tSecondary: ");
    }
    
    (void) fprintf(fp, "%s\tTimers:\n%s\t\tHello: %#T  Poll: %#T  Dead: %#T  Retrans: %#T\n",
		   prefix,
		   prefix,
		   intf->hello_timer,
		   intf->poll_timer,
		   intf->dead_timer,
		   intf->retrans_timer);

    /* Neighbor info */
    if (FirstNbr(intf)) {
	(void) fprintf(fp, "%s\tNeighbors:\n",
		       prefix);

	NBRS_LIST(n, intf) {
	    (void) fprintf(fp, "%s\t\tRouterID: ",
			   prefix);
	    if (n->nbr_id) {
		(void) fprintf(fp, "%-15A",
			       n->nbr_id);
	    } else {
		(void) fprintf(fp, "%-15s", "Unknown");
	    }
	    (void) fprintf(fp, "\tAddress: %-15A\n",
			   n->nbr_addr);

	    (void) fprintf(fp, "%s\t\tState: %s\tMode: %-6s\tPriority: %d\n",
			   prefix,
			   trace_state(ospf_neighbor_state_bits, n->state),
			   trace_state(ospf_neighbor_mode_bits, n->mode),
			   n->pri);

	    (void) fprintf(fp, "%s\t\tDR: %A\tBDR: %A\n",
			   prefix,
			   n->dr ? sockbuild_in(0, n->dr) : sockbuild_str("None"),
			   n->bdr ? sockbuild_in(0, n->bdr) : sockbuild_str("None"));

	    (void) fprintf(fp, "%s\t\tLast Hello: %T\tLast Exchange: %T\n",
			   prefix,
			   n->last_hello,
			   n->last_exch);

	    if (n->nbr_sequence) {
		(void) fprintf(fp, "%s\t\tAuthentication Sequence: %x\n",
			       prefix,
			       n->nbr_sequence);
	    }
	    
	    /* Retrans list */
	    if (n->rtcnt) {
		int hash = OSPF_HASH_QUEUE;

		(void) fprintf(fp, "%s\t\tRetrans list:\n",
			       prefix);

		while (hash--) {
			struct ospf_lsdb_list *ll;

		    for (ll = (struct ospf_lsdb_list *) n->retrans[hash].ptr[NEXT];
			 ll;
			 ll = ll->ptr[NEXT]) {
			(void) fprintf(fp, "%s\t\t\tType: %s\t%A %A\n",
				       prefix,
				       trace_state(ospf_ls_type_bits, LS_TYPE(ll->lsdb)),
				       sockbuild_in(0, LS_ID(ll->lsdb)),
				       sockbuild_in(0, ADV_RTR(ll->lsdb)));
		    }
		}
	    }
	    (void) fprintf(fp, "\n");
	} NBRS_LIST_END(n, intf) ;
    }

    (void) fprintf(fp, "\n");
}


static void
ospf_route_dump __PF4(fp, FILE *,
		      rr, struct OSPF_ROUTE *,
		      pfx, const char *,
		      ttl, const char *)

{
    fprintf(fp, "\n%s%s Border Routes:\n",
	    pfx,
	    ttl);
    
    while ((rr = RRT_NEXT(rr))) {
	u_int i;
	
	fprintf(fp, "%s\t%-15A\tArea %-15A  Cost %4d  AdvRouter %-15A\n",
		pfx,
		sockbuild_in(0, RRT_DEST(rr)),
		sockbuild_in(0, RRT_AREA(rr) ? RRT_AREA(rr)->area_id : OSPF_BACKBONE),
		RRT_COST(rr),
		sockbuild_in(0, RRT_ADVRTR(rr)));

	for (i = 0; i < RRT_NH_CNT(rr); i++) {
	    fprintf(fp, "%s\t\tNexthop: %-15A  Interface: %A(%s)\n",
		    pfx,
		    sockbuild_in(0, RRT_NH(rr)[i]->nh_addr),
		    RRT_NH(rr)[i]->nh_ifap->ifa_addr,
		    RRT_NH(rr)[i]->nh_ifap->ifa_link->ifl_name);
		    
	}
    }
}


static void
ospf_dump __PF2(tp, task *,
		fp, FILE *)
{
    struct AREA *area;
    struct LSDB *e;
    struct LSDB_HEAD *hp;
    struct NBR_LIST *nl;

    /* OSPF information */
    (void) fprintf(fp, "\tRouterID: %A",
		   ospf.router_id);
    if (IAmBorderRtr || ospf.asbr) {
	(void) fprintf(fp, "\tBorder Router:%s%s\n",
		       IAmBorderRtr ? " Area" : "",
		       ospf.asbr ? " AS" : "");
    } else {
	(void) fprintf(fp, "\n");
    }
    
    (void) fprintf(fp, "\tPreference:\tInter/Intra: %d\tExternal: %d\n",
		   ospf.preference,
		   ospf.preference_ase);
    (void) fprintf(fp, "\tDefault:\tMetric: %d\tTag: %A\tType: %d\n",
		   ospf.export_metric,
		   ospf_path_tag_dump(inet_autonomous_system, ospf.export_tag),
		   ospf.export_type);
    (void) fprintf(fp, "\tSPF count: %d\n",
		   SPFCNT);
    {
	u_int type = 0;

	(void) fprintf(fp, "\tLSAs originated: %u\treceived: %u\n\t\t",
		   ospf.orig_new_lsa,
		   ospf.rx_new_lsa);
	while (++type < LS_MAX) {
	    if (ospf.orig_lsa_cnt[type]) {
		(void) fprintf(fp, "%s%s: %u",
			       type == 1 ? "" : "  ",
			       trace_state(ospf_ls_type_bits, type),
			       ospf.orig_lsa_cnt[type]);
	    }
	}
	(void) fprintf(fp, "\n");
    }
    (void) fprintf(fp, "\tSyslog first %u, then every %u\n",
		   ospf.log_first,
		   ospf.log_every);

    ospf_auth_dump(fp, &ospf.mon_auth, "\t", "Monitor authentication: ");

    if (ospf.import_list) {
	adv_entry *list;

	(void) fprintf(fp, "\tImport controls:\n");
	
	ADV_LIST(ospf.import_list, list) {
	    int first = TRUE;
	    
	    if (BIT_TEST(list->adv_flag, ADVFOT_PREFERENCE)) {
		if (first) {
		    (void) fprintf(fp, "\t");
		    first = FALSE;
		}
		(void) fprintf(fp, "\tPreference %d",
			       list->adv_result.res_preference);
	    }
	    if (PS_FUNC_VALID(list, ps_print)) {
		if (first) {
		    (void) fprintf(fp, "\t");
		    first = FALSE;
		}
		(void) fprintf(fp, "\t%s",
			       PS_FUNC(list, ps_print)(list->adv_ps, TRUE));
	    }
	    if (!first) {
		(void) fprintf(fp, ":\n");
	    }
			   
	    control_dmlist_dump(fp, 3, list->adv_list, (adv_entry *) 0, (adv_entry *) 0);
	    (void) fprintf(fp, "\n");
	} ADV_LIST_END(ospf.import_list, adv) ;
    }

    if (ospf.export_list) {
	adv_entry *list;

	(void) fprintf(fp, "\tExport controls:\n");
	
	ADV_LIST(ospf.export_list, list) {
	    adv_entry *adv;
	    int first = TRUE;

	    if (BIT_TEST(list->adv_flag, ADVF_NO)) {
		if (first) {
		    (void) fprintf(fp, "\t");
		    first = FALSE;
		}
		(void) fprintf(fp, "\tRestrict\n");
		continue;
	    }

	    if (BIT_TEST(list->adv_flag, ADVFOT_METRIC)) {
		if (first) {
		    (void) fprintf(fp, "\t");
		    first = FALSE;
		}
		(void) fprintf(fp, "\tMetric %d",
			       list->adv_result.res_metric);
	    }
	    if (PS_FUNC_VALID(list, ps_print)) {
		if (first) {
		    (void) fprintf(fp, "\t");
		    first = FALSE;
		}
		(void) fprintf(fp, "\t%s",
			       PS_FUNC(list, ps_print)(list->adv_ps, TRUE));
	    }
	    if (!first) {
		(void) fprintf(fp, ":\n");
	    }
			   
	    if ((adv = list->adv_list)) {
		do {
		    control_entry_dump(fp, 3, adv);
		    if (adv->adv_list) {
			control_dmlist_dump(fp, 3, adv->adv_list, (adv_entry *) 0, (adv_entry *) 0);
		    }
		    do {
			adv = adv->adv_next;
		    } while (adv && !BIT_TEST(adv->adv_flag, ADVF_FIRST));
		} while (adv);
	    }
	    (void) fprintf(fp, "\n");
	} ADV_LIST_END(ospf.export_list, adv) ;
    }
    (void) fprintf(fp, "\n");

    /* Print error info */
    {
	u_int type = 0;

	fprintf(fp,"\tPackets Received:\n");
	do {
	    (void) fprintf(fp, "\t%8d: %-32s%s",
			   ospf_cumlog[type],
			   trace_state(ospf_logtype, type),
			   (type & 1) ? "\n" : "  ");
	} while (++type < 6) ;

	fprintf(fp,"\n\tPackets Sent:\n");
	do {
	    (void) fprintf(fp, "\t%8d: %-32s%s",
			   ospf_cumlog[type],
			   trace_state(ospf_logtype, type),
			   (type & 1) ? "\n" : "  ");
	} while (++type < 12) ;

	fprintf(fp,"\n\tErrors:\n");
	do {
	    (void) fprintf(fp, "\t%8d: %-32s%s",
			   ospf_cumlog[type],
			   trace_state(ospf_logtype, type),
			   (type & 1) ? "\n" : "  ");
	} while (++type < OSPF_ERR_LAST) ;
#if	OSPF_ERR_LAST & 1
	(void) fprintf(fp, "\n\n");
#else
	(void) fprintf(fp, "\n");
#endif
    }

    /* Virtual interfaces */
    if (ospf.vcnt) {
	struct INTF *vintf;

	(void) fprintf(fp, "\tVirtual Interfaces:\n");
	
	VINTF_LIST(vintf) {
	    ospf_interface_dump(fp, vintf, "\t");
	} VINTF_LIST_END(vintf) ;
    }
    
    /* Area information */
    AREA_LIST(area) {
	u_int type;
	struct INTF *intf;
	
	(void) fprintf(fp, "\tArea %A:\n",
		       sockbuild_in(0, area->area_id));

	(void) fprintf(fp, "\t\tAuthtype: ");
	switch (area->authtype) {
	case OSPF_AUTH_NONE:
	    (void) fprintf(fp, "none");
	    break;

	case OSPF_AUTH_SIMPLE:
	    (void) fprintf(fp, "simple");
	    break;

	case OSPF_AUTH_MD5:
	    (void) fprintf(fp, "md5");
	    break;
	    
	default:
	    assert(FALSE);
	}

	(void) fprintf(fp, "\tflags: <%s>",
		       trace_bits(ospf_area_bits, area->area_flags));

	if (BIT_TEST(area->area_flags, OSPF_AREAF_STUB_DEFAULT)) {
	    (void) fprintf(fp, "\tDefault cost: %d",
			   area->dflt_metric);
	}

	(void) fprintf(fp, "\n");

	(void) fprintf(fp, "\t\tSPF scheduled: <%s>\n",
		       trace_bits(ospf_sched_bits, (flag_t) area->spfsched));
	
	(void) fprintf(fp, "\n");

	/* Print ranges */
	if (area->nr.ptr[NEXT]) {
	    register struct NET_RANGE *nr;

	    (void) fprintf(fp, "\t\tRanges:\n");

	    RANGE_LIST(nr, area) {
		(void) fprintf(fp, "\t\t\t%-15A %-15A\tcost %u\t%sAdvertise\n",
			       sockbuild_in(0, nr->net),
			       sockbuild_in(0, nr->mask),
			       nr->cost,
			       nr->status == DoNotAdvertise ? "DoNot" : "");
	    } RANGE_LIST_END(nr, area) ;

	    (void) fprintf(fp, "\n");
	}

	/* Print stub hosts */
	if (area->hosts.ptr[NEXT]) {
	    struct OSPF_HOSTS *host;

	    (void) fprintf(fp, "\t\tStub hosts:\n");
	    
	    for (host = area->hosts.ptr[NEXT]; host != HOSTSNULL; host = host->ptr[NEXT]) {
		(void) fprintf(fp, "\t\t\t%-15A\tcost %u\n",
			       sockbuild_in(0, host->host_if_addr),
			       host->host_cost);
	    }
	}

	/* Print interfaces */
	INTF_LIST(intf, area) {
	    ospf_interface_dump(fp, intf, "\t\t");
	} INTF_LIST_END(intf, area) ;

	/* AS border routers */
	ospf_route_dump(fp, &area->asbrtab, "\t\t", "AS");

	/* Area border rtrs */
	ospf_route_dump(fp, &area->abrtab, "\t\t", "Area");

	(void) fprintf(fp, "\n\t\tLink State Database:\n");
	for (type = LS_STUB; type < LS_ASE; type++) {
	    LSDB_HEAD_LIST(area->htbl[type], hp, 0, HTBLSIZE) {
		LSDB_LIST(hp, e) {
		    int i, cnt;
		    struct NET_LA_PIECES *att_rtr;
		    struct RTR_LA_PIECES *linkp;

		    (void) fprintf(fp, "\t\t%s\tAdvRtr: %A\tLen: %d\tAge: %#T\tSeq: %08x\n",
				   trace_state(ospf_ls_type_bits, type),
				   sockbuild_in(0, ADV_RTR(e)),
				   ntohs(LS_LEN(e)),
				   MIN(ADV_AGE(e), MaxAge),
				   ntohl(LS_SEQ(e)));

		    switch (type) {
		    case LS_STUB:
		    case LS_NET:
			(void) fprintf(fp, "\t\t\tRouter: %A Netmask: %A Network: %A\n",
				       sockbuild_in(0, LS_ID(e)),
				       sockbuild_in(0, DB_MASK(e)),
				       sockbuild_in(0, DB_NETNUM(e)));

			cnt = ntohs(DB_NET(e)->ls_hdr.length) - NET_LA_HDR_SIZE;

			for (att_rtr = &DB_NET(e)->att_rtr, i = 0;
			     i < cnt;
			     att_rtr++, i += 4) {
			    (void) fprintf(fp, "\t\t\tAttached Router: %A\n",
					   sockbuild_in(0, att_rtr->lnk_id));
			}
			break;
			
		    case LS_RTR:
			(void) fprintf(fp, "\t\t\tRouterID: %A\tArea Border: %s\tAS Border: %s\n",
				       sockbuild_in(0, LS_ID(e)),
				       (ntohs(DB_RTR(e)->E_B) & bit_B) ? "On" : "Off",
				       (ntohs(DB_RTR(e)->E_B) & bit_E) ? "On" : "Off");

			for (cnt = ntohs(DB_RTR(e)->lnk_cnt),
			     i = 0,
			     linkp = (struct RTR_LA_PIECES *) &DB_RTR(e)->link;
			     i < cnt;
			     linkp = (struct RTR_LA_PIECES *) ((long) linkp +
							      RTR_LA_PIECES_SIZE +
							      ((linkp->metric_cnt) * RTR_LA_METRIC_SIZE)),
			     i++) {
			    (void) fprintf(fp, "\t\t\t\tType: %s\tCost: %u\n",
					   trace_state(ospf_router_type_bits, linkp->con_type - 1),
					   ntohs(linkp->tos0_metric));
			    switch (linkp->con_type) {
			    case RTR_IF_TYPE_RTR:
			    case RTR_IF_TYPE_VIRTUAL:
				(void) fprintf(fp, "\t\t\t\tRouterID: %A\tAddress: %A\n",
					       sockbuild_in(0, linkp->lnk_id),
					       sockbuild_in(0, linkp->lnk_data));
				break;
				
			    case RTR_IF_TYPE_TRANS_NET:
				(void) fprintf(fp, "\t\t\t\tDR: %A\tAddress: %A\n",
					       sockbuild_in(0, linkp->lnk_id),
					       sockbuild_in(0, linkp->lnk_data));
				break;
				
			    case RTR_IF_TYPE_STUB_NET:
				(void) fprintf(fp, "\t\t\t\tNetwork: %A\tNetMask: %A\n",
					       sockbuild_in(0, linkp->lnk_id),
					       sockbuild_in(0, linkp->lnk_data));
				break;
			    }
			}
			break;

		    case LS_SUM_NET:
			(void) fprintf(fp, "\t\t\tNetwork: %A\tNetmask: %A\tCost: %u\n",
				       sockbuild_in(0, LS_ID(e)),
				       sockbuild_in(0, DB_SUM(e)->net_mask),
				       BIG_METRIC(e));
			break;

		    case LS_SUM_ASB:
			(void) fprintf(fp, "\t\t\tRouterID: %A\tCost: %u\n",
				       sockbuild_in(0, LS_ID(e)),
				       BIG_METRIC(e));
			break;
		    }

		    /* Next hops */
		    if (e->lsdb_nhcnt) {
			u_int nh;

			(void) fprintf(fp, "\t\t\tNexthops(%u):\n",
				       e->lsdb_nhcnt);
			for (nh = 0; nh < e->lsdb_nhcnt; nh++) {
			    fprintf(fp, "\t\t\t\t%-15A  Interface: %A(%s)\n",
				    sockbuild_in(0, e->lsdb_nh[nh]->nh_addr),
				    e->lsdb_nh[nh]->nh_ifap->ifa_addr,
				    e->lsdb_nh[nh]->nh_ifap->ifa_link->ifl_name);
			}
		    }

		    /* Retransmission list */
		    if ((nl = e->lsdb_retrans)) {
			(void ) fprintf(fp, "\t\tRetransmission List:\n");

			do {
			    (void) fprintf(fp, "\t\t\tNeighbor: %-15A\n",
					   nl->nbr->nbr_addr);
			} while ((nl = nl->ptr[NEXT])) ;
		    }
		    (void) fprintf(fp, "\n");
		} LSDB_LIST_END(DBH_LIST(hp), db) ;
	    } LSDB_HEAD_LIST_END(area->htbl[type], hp, 0, HTBLSIZE) ;
	}
    } AREA_LIST_END(area) ;

    /* Next hops */
    if (ospf.nh_list.nh_forw != &ospf.nh_list) {
	register struct NH_BLOCK *np;

	(void) fprintf(fp, "\tNext hops:\n");

	NH_LIST(np) {
	    (void) fprintf(fp, "\t\t%A\t%-8s\tintf %-15A\trefcount %d\n",
			   sockbuild_in(0, np->nh_addr),
			   trace_state(ospf_nh_bits, np->nh_type),
			   np->nh_ifap->ifa_addr,
			   np->nh_refcount);
	} NH_LIST_END(np) ;
    }

    /* Summary ASB routes */
    ospf_route_dump(fp, &ospf.sum_asb_rtab, "\t", "Summary AS");

    /* Print AS External LSDB */
    (void) fprintf(fp, "\n\tAS Externals (%u):\n\n",
		   ospf.db_ase_cnt);
    area = ospf.area.area_forw;
    LSDB_HEAD_LIST(area->htbl[LS_ASE], hp, 0, HTBLSIZE) {
	LSDB_LIST(hp, e) {
	    (void) fprintf(fp, "\t\tAdvRtr: %A\tLen: %d\tAge: %#T\tSeq: %08x\n",
			   sockbuild_in(0, ADV_RTR(e)),
			   ntohs(LS_LEN(e)),
			   MIN(ADV_AGE(e), MaxAge),
			   ntohl(LS_SEQ(e)));
	    (void) fprintf(fp, "\t\tNetwork: %A\tNetmask: %A\tCost: %u\n",
			   sockbuild_in(0, LS_ID(e)),
			   sockbuild_in(0, DB_ASE(e)->net_mask),
			   BIG_METRIC(e));
	    (void) fprintf(fp, "\t\tType: %d\tForward: %A\tTag: %A\n",
			   ASE_TYPE1(e) ? 1 : 2,
			   sockbuild_in(0, DB_ASE_FORWARD(e)),
			   ospf_path_tag_dump(inet_autonomous_system, DB_ASE(e)->tos0.ExtRtTag));

	    /* Next hops */
	    if (e->lsdb_nhcnt) {
		u_int nh;
		
		(void) fprintf(fp, "\t\tNexthops(%u):\n",
			       e->lsdb_nhcnt);
		for (nh = 0; nh < e->lsdb_nhcnt; nh++) {
		    fprintf(fp, "\t\t\t%-15A  Interface: %A(%s)\n",
			    sockbuild_in(0, e->lsdb_nh[nh]->nh_addr),
			    e->lsdb_nh[nh]->nh_ifap->ifa_addr,
			    e->lsdb_nh[nh]->nh_ifap->ifa_link->ifl_name);
		}
	    }

	    /* Retransmission list */
	    if ((nl = e->lsdb_retrans)) {
		(void) fprintf(fp, "\t\tRetransmission List:\n");
		
		do {
		    (void) fprintf(fp, "\t\t\tNeighbor: %-15A\n",
				   nl->nbr->nbr_addr);
		} while ((nl = nl->ptr[NEXT])) ;
	    }
	    (void) fprintf(fp, "\n");
	} LSDB_LIST_END(hp, e) ;
    } LSDB_HEAD_LIST_END(area->htbl[LS_ASE], hp, 0, HTBLSIZE) ;

    /* Routes waiting to be exported into OSPF */
    if (ospf.export_queue.forw != &ospf.export_queue) {
	ospf_export_entry *ce = ospf.export_queue.forw;

	(void) fprintf(fp, "\tExport queue:  %d entries\n",
		       ospf.export_queue_size);

	do {
	    const char *actptr;
	    const char *action;
	    rt_entry *rt;

	    if (ospf.export_queue_delete == ce
	      || ospf.export_queue_change == ce) {
		actptr = "     -> ";
	    } else {
		actptr = "\t";
	    }
	    if (ce->new_rt != NULL) {
		rt = ce->new_rt;
		if (ce->old_rt != NULL) {
		    action = "Change";
		} else {
		    action = "Add";
		}
	    } else {
		rt = ce->old_rt;
		action = "Delete";
	    }
	    (void) fprintf(fp, "\t%s%-8s %-15A",
			   actptr,
			   action,
			   rt->rt_dest);
	    if (rt->rt_dest_mask) {
		(void) fprintf(fp, " mask %-15A",
			       rt->rt_dest_mask);
	    }
	    (void) fprintf(fp, "metric %8x tag %A\n",
			   ntohl(ce->metric),
			   ospf_path_tag_dump(inet_autonomous_system, ce->tag));
	} while ((ce = ce->forw) != &ospf.export_queue);

	(void) fprintf(fp, "\n");
    }
}


/**/

/*
 *	Manage next hop entries for our interfaces
 */
static void
ospf_nh_init __PF1(ifap, if_addr *)
{
    /* Make sure we have a next hop for this guy */

    int nh_type = NH_DIRECT;

    if (BIT_TEST(ifap->ifa_state, IFS_POINTOPOINT)) {
	OSPF_NH_ALLOC(ifap->ifa_ospf_nh = (void_t) ospf_nh_add(ifap,
							       sock2ip(ifap->ifa_addr),
							       nh_type));
	nh_type = NH_LOCAL;
    } else if (BIT_TEST(ifap->ifa_state, IFS_LOOPBACK)
	       && BIT_TEST(inet_class_flags(ifap->ifa_addr_local), INET_CLASSF_LOOPBACK)) {
	return;
    }
	
    OSPF_NH_ALLOC(ifap->ifa_ospf_nh_lcl = (void_t) ospf_nh_add(ifap,
							       sock2ip(ifap->ifa_addr_local),
							       nh_type));
}


/*
 *	Manage next hop entries for our interfaces
 */
static void
ospf_nh_fini __PF1(ifap, if_addr *)
{
    ospf_nh_free((struct NH_BLOCK **) &ifap->ifa_ospf_nh);
    ospf_nh_free((struct NH_BLOCK **) &ifap->ifa_ospf_nh_lcl);
}

/**/

/*
 *	OSPF termination
 */
static void
ospf_terminate __PF1(tp, task *)
{
    if_addr *ifap;
    struct AREA *area;
    struct INTF *intf;
    struct OSPF_ROUTE *rr, *rrt_next;
    struct LSDB_HEAD *hp;
    register struct LSDB *db;	    

#ifdef	PROTO_SNMP
    ospf_init_mib(FALSE);
#endif	/* PROTO_SNMP */
#ifdef  OSPF_HPMIB
    ospf_init_hpmib(FALSE);
#endif  /* OSPF_HPMIB */

    /* Free any NH blocks */
    IF_ADDR(ifap) {
	if (BIT_TEST(ifap->ifa_state, IFS_UP)) {
	    ospf_nh_fini(ifap);
	}
    } IF_ADDR_END(ifap) ;

    /* Free up any virtual links that did not already have tasks */
    VINTF_LIST(intf) {
	ospf_interface_delete(intf);
    } VINTF_LIST_END(intf) ;

    /* Terminate the areas */
    AREA_LIST(area) {
	struct ospf_lsdb_list *txq = LLNULL;
	int i;

	REMQUE(area);

	/* Delete timer */
	if (area->timer_lock) {
	    task_timer_delete(area->timer_lock);
	    area->timer_lock = (task_timer *) 0;
	}
    
	/* Free any disabled interfaces */
	INTF_LIST(intf, area) {
	    ospf_interface_delete(intf);
	} INTF_LIST_END(intf, area) ;

	if (ospf.router_id) {
	    /* build_rtr and run spf */
	    reset_rtr_lock(area);
	    area->build_rtr = FALSE;
	    area->spfsched |= build_rtr_lsa(area, &txq, 0);
	    ospf_freeq((struct Q **)&txq, ospf_lsdblist_index);
	    
	    /* Now that interfaces are deleted, run spf to cleanup */
	    /* XXX this could be optimized to only run ASE's once */
	    area->spfsched = ALLSCHED;
	    ospf_spf_run(area, 0);
	}
    
	/* Free NetRanges and Hosts */
	ospf_freeRangeList(area);
	ospf_freeHostsList(area);
    
	/* Free hashtable */
	for (i = LS_STUB; i < LS_ASE; i++) {
	    LSDB_HEAD_LIST(area->htbl[i], hp, 0, HTBLSIZE) {
		LSDB_LIST(hp, db) {
		    db_free(db, LS_TYPE(db));
		} LSDB_LIST_END(hp, db) ;
	    } LSDB_HEAD_LIST_END(area->htbl[i], hp, 0, HTBLSIZE) ;
	}

	/* Free router routes */
	for (rr = area->asbrtab.ptr[NEXT];
	     rr;
	     rr = rrt_next) {
	    rrt_next = RRT_NEXT(rr);
	    ospf_nh_free_list(RRT_NH_CNT(rr), RRT_NH(rr));
	    task_block_free(ospf_route_index, (void_t) rr);
	}

	for (rr = area->abrtab.ptr[NEXT];
	     rr;
	     rr = rrt_next) {
	    rrt_next = RRT_NEXT(rr);
	    ospf_nh_free_list(RRT_NH_CNT(rr), RRT_NH(rr));
	    task_block_free(ospf_route_index, (void_t) rr);
	}

	if (area != &ospf.backbone) {
	    task_block_free(ospf_area_index, (void_t) area);
	}
	ospf.acnt--;
    } AREA_LIST_END(area) ;

    /* Re-enable redirects on said interface */
    redirect_enable(RTPROTO_OSPF);

#ifdef	IP_MULTICAST
    if (tp) {
	ospf_multicast_cleanup(tp);
    }
#endif	/* IP_MULTICAST */

    /* Free other memory */
    if (ospf.router_id) {
	sockfree(ospf.router_id);
	ospf.router_id =(sockaddr_un *) 0;
    }

    /* Clean up routing table interface */
    ospf_policy_cleanup(tp);

    /* Remove ASEs */
    LSDB_HEAD_LIST(ospf.ase, hp, 0, HTBLSIZE) {
	LSDB_LIST(hp, db) {
	    db_free(db, LS_TYPE(db));
	} LSDB_LIST_END(hp, db) ;
    } LSDB_HEAD_LIST_END(ospf.ase, hp, 0, HTBLSIZE) ;

    assert(DB_EMPTYQ(ospf.db_free_list));
#ifdef	notdef
    while (!DB_EMPTYQ(ospf.db_free_list)
	   && (db = DB_FIRSTQ(ospf.db_free_list))) {
	db_free(db, LS_TYPE(db));
    }
#endif	/* notdef */

    /* free asb routes */
    for (rr = ospf.sum_asb_rtab.ptr[NEXT];
	 rr;
	 rr = rrt_next) { 
	rrt_next = RRT_NEXT(rr);
	ospf_nh_free_list(RRT_NH_CNT(rr), RRT_NH(rr));
	task_block_free(ospf_route_index, (void_t) rr);
    }

    /* Garbage collect the NH entries */
    (void) ospf_nh_collect();
    
    /* Finally free up the task */
    if (tp) {
	task_delete(tp);
	ospf.timer_ase = ospf.timer_ack = (task_timer *) 0;
	ospf.gwp->gw_task = ospf.gwp_ase->gw_task = ospf.task = (task *) 0;
    }
}


/**/

/*
 *	Cleanup before reconfiguration
 */
static void
ospf_cleanup __PF1(tp, task *)
{
    struct AREA *area;

    AREA_LIST(area) {
	struct INTF *intf;
	
	adv_free_list(area->intf_policy);
	area->intf_policy = (adv_entry *) 0;

	INTF_LIST(intf, area) {
	    ospf_interface_delete(intf);
	} INTF_LIST_END(intf, area) ;
    } AREA_LIST_END(area) ;
    
    adv_free_list(ospf.import_list);
    ospf.import_list = (adv_entry *) 0;
    adv_free_list(ospf.export_list);
    ospf.export_list = (adv_entry *) 0;

    if (ospf.backbone.intf_policy) {
 	adv_free_list(ospf.backbone.intf_policy);
 	ospf.backbone.intf_policy = (adv_entry *) 0;
    }
    
    if (ospf.backbone.nrcnt) {
 	ospf_freeRangeList(&ospf.backbone);
    }
   
    /* Reconfig currently requires complete shutdown */
    ospf_terminate(tp);

    trace_freeup(ospf.trace_options);
}


/*
 *	Initialize static variables
 */
void
ospf_var_init()
{
    bzero((caddr_t) &ospf, sizeof ospf);
    
    ospf.ospf_admin_stat = OSPF_DISABLED;
    ospf.preference = RTPREF_OSPF;
    ospf.preference_ase = RTPREF_OSPF_ASE;
    ospf.export_metric = OSPF_DEFAULT_METRIC;
    ospf.export_tag = OSPF_DEFAULT_TAG;
    ospf.export_type = OSPF_DEFAULT_TYPE;
    ospf.export_interval = MinASEInterval;
    ospf.export_limit = ASEGenLimit;
    ospf.log_first = OSPF_LOG_FIRST;
    ospf.log_every = OSPF_LOG_EVERY;
    
#ifdef	IP_MULTICAST
    int_ps_bits[RTPROTO_OSPF] = ospf_if_bits;
#endif	/* IP_MULTICAST */

    if (!ospf.export_queue.forw) {
	ospf.export_queue.forw =
	    ospf.export_queue.back =
		ospf.export_queue_delete =
		    ospf.export_queue_change =
			&ospf.export_queue;
	ospf.export_queue_size = 0;
    }

    DB_INITQ(ospf.my_ase_list);
    DB_INITQ(ospf.db_free_list);

    if (!ospf.nh_list.nh_forw) {
	ospf.nh_list.nh_forw = ospf.nh_list.nh_back = &ospf.nh_list;
    }

    if (!ospf_router_index) {
	ospf_router_index = task_block_init(sizeof (ospf_config_router), "ospf_config_router");
    }

    /* Interfaces */
    if (!ospf_intf_index) {
	ospf_intf_index = task_block_init(sizeof (struct INTF), "ospf_INTF");
    }
    ospf.vl.intf_forw = ospf.vl.intf_back = &ospf.vl;
    ospf.backbone.intf.intf_forw = ospf.backbone.intf.intf_back = &ospf.backbone.intf;

    /* Areas */
    ospf.area.area_forw = ospf.area.area_back = &ospf.area;
    if (!ospf_area_index) {
	ospf_area_index = task_block_init(sizeof (struct AREA), "ospf_AREA");
    }

    /* Neighbors */
    if (!ospf_nbr_index) {
	ospf_nbr_index = task_block_init(sizeof (struct NBR),
					      "ospf_NBR");
    }
    if (!ospf_nbr_node_index) {
	ospf_nbr_node_index = task_block_init(sizeof (ospf_nbr_node),
					      "ospf_nbr_node");
    }

    /* Next hop block */
    if (!ospf_nh_block_index) {
	ospf_nh_block_index = task_block_init(sizeof (struct NH_BLOCK),
					      "ospf_NH_BLOCK");
    }

    if (!ospf_lsdb_index) {
	ospf_lsdb_index = task_block_init(sizeof (struct LSDB),
					  "ospf_LSDB");
    }

    if (!ospf_route_index) {
	ospf_route_index = task_block_init(sizeof (struct OSPF_ROUTE),
					   "ospf_ROUTE");
    }

    if (!ospf_dbsum_index) {
	ospf_dbsum_index = task_block_init(sizeof (struct LSDB_SUM),
					   "ospf_LSDB_SUM");
    }
    
    if (!ospf_netrange_index) {
	ospf_netrange_index = task_block_init(sizeof (struct NET_RANGE),
					      "ospf_NET_RANGE");
    }
    
    if (!ospf_hosts_index) {
	ospf_hosts_index = task_block_init(sizeof (struct OSPF_HOSTS),
					   "ospf_HOSTS");
    }

    if (!ospf_hdrq_index) {
	ospf_hdrq_index = task_block_init(sizeof (struct LS_HDRQ),
					  "ospf_LS_HDRQ");
    }

    if (!ospf_lsdblist_index) {
	ospf_lsdblist_index = task_block_init(sizeof (struct ospf_lsdb_list),
					      "ospf_LSDB_LIST");
    }

    if (!ospf_nbrlist_index) {
	ospf_nbrlist_index = task_block_init(sizeof (struct NBR_LIST),
					     "ospf_NBR_LIST");
    }

    if (!ospf_lsreq_index) {
	ospf_lsreq_index = task_block_init(sizeof (struct LS_REQ),
					   "ospf_LS_REQ");
    }

    if (!ospf_lsa_index_4) {
	ospf_lsa_index_4 = task_block_malloc(MAX(4096, task_pagesize));
	ospf_lsa_index_16 = task_block_malloc(task_pagesize);
    }

    if (!ospf_auth_index) {
	ospf_auth_index = task_block_init(sizeof (ospf_auth),
					  "ospf_auth");
    }
}


/*
 *	After reconfiguration
 */
static void
ospf_reinit __PF1(tp, task *)
{
    int entries = 0;
    rt_entry *rt;

    /* Set autonomous system from gated's default AS */
    /* XXX - What needs to be done if this changes during reinit? */
    ospf.gwp->gw_local_as = ospf.gwp_ase->gw_local_as = inet_autonomous_system;
    
    if (ospf.asbr) {
	
	/* Open the routing table */
	rt_open(tp);

	RTQ_LIST(&ospf.gwp->gw_rtq, rt) {
	    pref_t preference = ospf.preference_ase;

	    /* Calculate preference of this route */
	    (void) import(rt->rt_dest,
			  rt->rt_dest_mask,
			  ospf.import_list,
			  (adv_entry *) 0,
			  (adv_entry *) 0,
			  &preference,
			  (if_addr *) 0,
			  (void_t) ORT_V(rt));

	    if (rt->rt_preference != preference) {
		/* The preference has changed, change the route */
		(void) rt_change(rt,
				 rt->rt_metric,
				 rt->rt_metric2,
				 rt->rt_tag,
				 preference,
				 rt->rt_preference2,
				 rt->rt_n_gw, rt->rt_routers);
	    }
	    entries++;
	} RTQ_LIST_END(&ospf.gwp->gw_rtq, rt) ;

	/* Close the routing table */
	rt_close(tp, (gw_entry *) 0, entries, NULL);
    } else if (ospf.import_list) {
	/* Ignored if we are not an AS Border router */

	trace_log_tf(ospf.trace_options,
		     0,
		     LOG_WARNING,
		     ("ospf_reinit: import list ignored if not an AS border router"));
    }

#if defined(PROTO_SNMP) || defined(OSPF_HPMIB)
    /* Update the MIB info */
    o_vintf_get();
#endif	/* PROTO_SNMP || OSPF_HPMIB */
}


static void
ospf_ifachange __PF2(tp, task *,
		     ifap, if_addr *)
{
    int action = IFC_NOCHANGE;
    struct INTF *intf = IF_INTF(ifap);

    if (socktype(ifap->ifa_addr) != AF_INET) {
	return;
    }
    
    switch (ifap->ifa_change) {
    case IFC_NOCHANGE:
	if (intf) {
	    /* We already know about this interface */
	    action = IFC_REFRESH;
	    break;
	}
	/* Fall through */

    case IFC_ADD:
	if (BIT_TEST(ifap->ifa_state, IFS_UP)) {
	    goto Up;
	}
	break;

    case IFC_DELETE:
	break;

    case IFC_DELETE|IFC_UPDOWN:
	goto Down;

    default:
	/* Something has changed */
	if (BIT_TEST(ifap->ifa_change, IFC_UPDOWN)) {
	    /* UP <-> DOWN transition */

	    if (BIT_TEST(ifap->ifa_state, IFS_UP)) {
		/* Interface is now UP */

	    Up:
		ospf_nh_init(ifap);

		assert(!intf);

		if (!BIT_TEST(ifap->ifa_state, IFS_LOOPBACK)) {
		    action = IFC_ADD;
		}
	    } else {
		/* Interface is now down */

	    Down:
		ospf_nh_fini(ifap);

		/* Schedule an OSPF run to clear the cache */
		ospf_spf_sched();

		if (intf) {
		    if (BIT_TEST(intf->flags, OSPF_INTFF_ENABLE)) {
			ospf_ifdown(intf);
		    }

		    ospf_interface_delete(intf);
		}
	    }
	} else if (intf) {
	    /* Something about an interface we know about has changed */

	    if (BIT_TEST(ifap->ifa_change, IFC_METRIC)) {
		/* The metric has changed */

		if (!BIT_TEST(intf->flags, OSPF_INTFF_COSTSET)) {
		    intf->cost = ifap->ifa_metric + OSPF_HOP;
		}
		set_rtr_sched(intf->area);
	    }
	    if (BIT_TEST(ifap->ifa_change, IFC_NETMASK)) {
		/* Toggle the interface */
		(*if_trans[INTF_DOWN][intf->state]) (intf);
		(*if_trans[INTF_UP][IDOWN]) (intf);
	    }
	    if (BIT_TEST(ifap->ifa_change, IFC_ADDR)) {
		set_rtr_sched(intf->area);
	    }
	}
	/* BROADCAST change - we don't use the broadcast address */
	/* MTU change - should take effect on output */
	/* SEL change - homey don't do ISO */
	break;
    }

    if (action) {
	/* Check configuration */
	struct AREA *area = (struct AREA *) 0, *ap;
	config_entry **list = (config_entry **) 0;

	AREA_LIST(ap) {
	    config_entry **new_list = config_resolv_ifa(ap->intf_policy,
							ifap,
							OSPF_CONFIG_MAX);
	    if (new_list) {
		if (list) {
		    trace_log_tf(ospf.trace_options,
				 0,
				 LOG_WARNING,
				 ("ospf_ifachange: interface %A (%s) configured in two areas",
				  ifap->ifa_addr,
				  ifap->ifa_link->ifl_name));
		    config_resolv_free(new_list, OSPF_CONFIG_MAX);
		    goto Ignore;
		}
		list = new_list;
		area = ap;
	    }
	} AREA_LIST_END(ap) ;

	if (list) {
	    int type = OSPF_CONFIG_MAX;
	    config_entry *cp = list[OSPF_CONFIG_TYPE];
	    int intf_type = cp ? (int) GA2S(cp->config_data) : 0;

	    /* Interface was configured - create or update configuration */

	    /* Figure out type */
	    switch (BIT_TEST(ifap->ifa_state, IFS_BROADCAST|IFS_POINTOPOINT)) {
	    case IFS_BROADCAST:
		switch (intf_type) {
		case NONBROADCAST:
		    break;

		default:
#ifdef	IP_MULTICAST
		    intf_type = BROADCAST;
		    break;
#else	/* IP_MULTICAST */
		    trace_log_tf(ospf.trace_options,
				 0,
				 LOG_WARNING,
				 ("ospf_ifachange: system does not support multicast; ignoring interface %A (%s)",
				  ifap->ifa_addr,
				  ifap->ifa_link->ifl_name));
		    goto Ignore;
#endif	/* IP_MULTICAST */
		}
		break;

	    case 0:
		/* NBMA */
		intf_type = NONBROADCAST;
		break;

	    case IFS_POINTOPOINT:
		switch (intf_type) {
		case 0:
		case POINT_TO_POINT:
		    break;

		default:
		    trace_log_tf(ospf.trace_options,
				 0,
				 LOG_WARNING,
				 ("ospf_ifachange: ignoring %s specification for point-to-point interface %A (%s)",
				  trace_value(ospf_if_type_bits, intf_type),
				  ifap->ifa_addr,
				  ifap->ifa_link->ifl_name));
		    break;
		}
		intf_type = POINT_TO_POINT;
		break;
	    }

	    switch (GA2S(intf)) {
	    default:
		/* See if type has changed */

		if (intf_type == intf->type) {
		    /* Nothing to do */
		    break;
		}
		/* Type has changed, delete interface and create a new one */

		ospf_ifdown(intf);
		ospf_interface_delete(intf);
		/* Fall through */

	    case 0:
		/* New interface, allocate structure */

		intf = ospf_parse_intf_alloc(area, intf_type, ifap);
		break;
	    }

	    /* Default the cost */
	    intf->cost = ifap->ifa_metric + OSPF_HOP;
	    BIT_RESET(intf->flags, OSPF_INTFF_SECAUTH);

	    /* Fill in the parameters */
	    while (--type) {
		if ((cp = list[type])) {
		    switch (type) {
		    case OSPF_CONFIG_TYPE:
			/* Dealt with above */
			break;

		    case OSPF_CONFIG_COST:
			intf->cost = (u_int) GA2S(cp->config_data);
			/* Indicate cost was manually configured */
			BIT_SET(intf->flags, OSPF_INTFF_COSTSET);
			break;
		    
		    case OSPF_CONFIG_ENABLE:
			if ((int) GA2S(cp->config_data)) {
			    BIT_SET(intf->flags, OSPF_INTFF_ENABLE);
			} else {
			    BIT_RESET(intf->flags, OSPF_INTFF_ENABLE);
			}
			break;
		    
		    case OSPF_CONFIG_RETRANSMIT:
			intf->retrans_timer = (time_t) GA2S(cp->config_data);
			break;

		    case OSPF_CONFIG_TRANSIT:
			intf->transdly = (time_t) GA2S(cp->config_data);
			break;

		    case OSPF_CONFIG_PRIORITY:
			switch (intf->type) {
			case BROADCAST:
			case NONBROADCAST:
			    intf->nbr.pri = intf->pri = (u_int) GA2S(cp->config_data);
			    break;

			case POINT_TO_POINT:
			    trace_log_tf(ospf.trace_options,
					 0,
					 LOG_INFO,
					 ("ospf_ifachange: priority option ignored for point-to-point interface %A (%s)",
					  ifap->ifa_addr,
					  ifap->ifa_link->ifl_name));
			    break;
			}
			break;

		    case OSPF_CONFIG_HELLO:
			intf->hello_timer = (time_t) GA2S(cp->config_data);
			break;
		    
		    case OSPF_CONFIG_ROUTERDEAD:
			intf->dead_timer = (time_t) GA2S(cp->config_data);
			break;

		    case OSPF_CONFIG_AUTH:
			bcopy((caddr_t) cp->config_data, (caddr_t) &intf->auth, sizeof (ospf_auth));
			break;
			
		    case OSPF_CONFIG_AUTH2:
			bcopy((caddr_t) cp->config_data, (caddr_t) &intf->auth2, sizeof (ospf_auth));
			BIT_SET(intf->flags, OSPF_INTFF_SECAUTH);
			break;
		    
		    case OSPF_CONFIG_POLL:
			intf->poll_timer = (time_t) GA2S(cp->config_data);
			break;
		    
		    case OSPF_CONFIG_ROUTERS:
		        {
			    ospf_config_router *ocrp = (ospf_config_router *) cp->config_data;
			    struct NBR *nbr_list = intf->nbr.next;
			    struct NBR *nbr;

			    /* We have old list */
			    intf->nbr.next = (struct NBR *) 0;

			    do {
				register u_int32 new_addr = ntohl(ocrp->ocr_router.s_addr);
				
				struct NBR *nbr_last = (struct NBR *) 0;

				if (ifap != if_withdst(sockbuild_in(0, ocrp->ocr_router.s_addr))) {
				    /* Neighbor is not on the same net */

				    continue;
				}

				/* See if we have this neighbor already */
				for (nbr = nbr_list; nbr; nbr_last = nbr, nbr = nbr->next ) {
				    register u_int32 nbr_addr = ntohl(NBR_ADDR(nbr));

				    if (new_addr == nbr_addr) {
					/* Found it */

					break;
				    } else if (new_addr < nbr_addr) {
					/* Won't find it here */

					goto New;
				    }
				}

				if (nbr) {
				    /* Exists, remove from old list */

				    if (nbr_last) {
					nbr_last->next = nbr->next;
				    } else {
					nbr_list = nbr->next;
				    }
				    nbr->next = (struct NBR *) 0;

				    /* Drop the count to compensate for future increase */
				    ospf.nbrcnt--;
				} else {
				    /* New neighbor */

				New:
				    nbr = (struct NBR *) task_block_alloc(ospf_nbr_index);

				    nbr->nbr_addr = sockdup(sockbuild_in(0, ocrp->ocr_router.s_addr));
				    nbr->intf = intf;
				}
				/* Add to list for this interface */
				ospf_nbr_add(intf, nbr);

				/* Set the priority */
				nbr->pri = ocrp->ocr_priority;

				/* XXX - What to do if priority has changed? */
			    } while ((ocrp = ocrp->ocr_next)) ;

			    /* Free the remaining neighbors */
			    while ((nbr = nbr_list)) {
				/* Bring the neighbor down */
				(*(nbr_trans[KILL_NBR][nbr->state])) (intf, nbr);

				nbr_list = nbr->next;
				ospf_nbr_delete(intf, nbr);
			    }
			}
			break;
			
		    case OSPF_CONFIG_NOMULTI:
			if (intf->type != POINT_TO_POINT) {
			    trace_log_tf(ospf.trace_options,
					 0,
					 LOG_WARNING,
					 ("ospf_ifachange: ignoring nomulticast specification for point-to-point interface %A (%s)",
					  ifap->ifa_addr,
					  ifap->ifa_link->ifl_name));
			    break;
			}
			/* Don't attempt to use multicast on this */
			/* interface, probably because the other end */
			/* does not support it */
			BIT_RESET(intf->flags, OSPF_INTFF_MULTICAST);
			break;
		    }
		}
	    }
	} else if (intf) {
	    /* Interface is no longer in configuration, delete it */

	    ospf_ifdown(intf);

	    ospf_interface_delete(intf);
	    intf = (struct INTF *) 0;
	}

	if (intf) {
	    /* Verify the configuration */
	    ospf_parse_intf_check(intf);

	    /* Create the control blocks and fire it up */
	    if (BIT_TEST(intf->flags, OSPF_INTFF_ENABLE)) {
                if(ospf.intf_offset >= intf->hello_timer)
                   ospf.intf_offset = 0;
		ospf_interface_init(intf, ospf.intf_offset++);
	    }

	    /* Schedule rebuild of router links */
	    set_rtr_sched(intf->area);
	}

    Ignore:
	if (list) {
	    config_resolv_free(list, OSPF_CONFIG_MAX);
	}
    }

    /* Check router id */
    if (!sockaddrcmp(ospf.router_id, inet_routerid)) {
	/* Router ID has changed */

	trace_log_tf(ospf.trace_options,
		     TRC_NL_BEFORE,
		     LOG_ERR,
		     ("ospf_ifachange: router ID changed from %A to %A; terminating OSPF",
		      ospf.router_id,
		      inet_routerid));
	trace_log_tf(ospf.trace_options,
		     TRC_NL_AFTER,
		     LOG_ERR,
		     ("ospf_ifachange: reconfigure to restart OSPF"));

	/* Locate and terminate all other interfaces */
	IF_ADDR(ifap) {
	    if (BIT_TEST(ifap->ifa_state, IFS_UP)) {
		ospf_nh_fini(ifap);

		intf = IF_INTF(ifap);
		if (intf) {
		    if (BIT_TEST(intf->flags, OSPF_INTFF_ENABLE)) {
			ospf_ifdown(intf);
		    }
 
		    ospf_interface_delete(intf);
		}
	    }
        } IF_ADDR_END(ifap);
	ospf_cleanup(tp);
	/* XXX - We can not stop and start because the act of stopping causes us to */
	/* XXX - lose all our configuration information */
#ifdef	notdef
	ospf_startup();
#endif	/* notdef */
	return;
    }

#if defined(PROTO_SNMP) || defined(OSPF_HPMIB)
    /* Update the MIB info */
    o_intf_get();
#endif	/* PROTO_SNMP || OSPF_HPMIB */
}


static void
ospf_startup __PF0(void)
{
    struct INTF *intf;
    u_int area_offset = 0;
    struct AREA *a;

    if (!inet_routerid_entry) {
	trace_log_tf(ospf.trace_options,
		     0,
		     LOG_WARNING,
		     ("ospf_startup: Router ID not defined"));
	return;
    }
    if (!ospf.router_id) {
	ospf.router_id = sockdup(inet_routerid);
    }

    trace_inherit_global(ospf.trace_options, ospf_trace_types, (flag_t) 0);
    if (!ospf_proto) {
	ospf_proto = task_get_proto(ospf.trace_options,
				    "ospf",
				    IPPROTO_OSPF);
    }
    ospf.task = task_alloc("OSPF",
			   TASKPRI_PROTO + 1,
			   ospf.trace_options);
    ospf.task->task_proto = ospf_proto;
    ospf.task->task_rtproto = RTPROTO_OSPF;
    ospf.task->task_data = (void_t) &ospf;
    task_set_terminate(ospf.task, ospf_terminate);
    task_set_ifachange(ospf.task, ospf_ifachange);
    task_set_dump(ospf.task, ospf_dump);
    task_set_cleanup(ospf.task, ospf_cleanup);
    task_set_reinit(ospf.task, ospf_reinit);
    task_set_recv(ospf.task, ospf_recv);
    if ((ospf.task->task_socket = task_get_socket(ospf.task, 
						  AF_INET,
						  SOCK_RAW,
						  ospf.task->task_proto)) < 0) {
	task_quit(errno);
    }

    if (!task_create(ospf.task)) {
	task_quit(EINVAL);
    }

#ifdef	IP_MULTICAST
    ospf_multicast_init(ospf.task);
#endif	/* IP_MULTICAST */
	
    /* Create the gateway structures */
    if (ospf.gwp) {
	ospf.gwp->gw_task = ospf.task;
    } else {
	ospf.gwp = gw_init((gw_entry *) 0,
			   RTPROTO_OSPF,
			   ospf.task,
			   (as_t) 0,
			   (as_t) 0,
			   (sockaddr_un *) 0,
			   (flag_t) 0);
    }

    if (ospf.gwp_ase) {
	ospf.gwp_ase->gw_task = ospf.task;
    } else {
	ospf.gwp_ase = gw_init((gw_entry *) 0,
			       RTPROTO_OSPF_ASE,
			       ospf.task,
			       (as_t) 0,
			       (as_t) 0,
			       (sockaddr_un *) 0,
			       (flag_t) 0);
    }

    ospf_policy_init(ospf.task);

    /* Set the receive buffer size */
    if (task_set_option(ospf.task,
			TASKOPTION_RECVBUF,
			task_maxpacket) < 0) {
	task_quit(errno);
    }

    /* Set the send buffer size */
    if (task_set_option(ospf.task,
			TASKOPTION_SENDBUF,
			task_maxpacket) < 0) {
	task_quit(errno);
    }
    if (task_set_option(ospf.task,
			TASKOPTION_NONBLOCKING,
			TRUE) < 0) {
	task_quit(errno);
    }

#ifdef	IPTOS_PREC_INTERNETCONTROL
    /* Set Precendence for OSPF packets */
    (void) task_set_option(ospf.task,
			   TASKOPTION_TOS,
			   IPTOS_PREC_INTERNETCONTROL);
#endif	/* IPTOS_PREC_INTERNETCONTROL */

    /* Create global timers */

    /* create lsa generate timers */
    (void) task_timer_create(ospf.task,	
			     "LSAGenInt",
			     0,
			     LSRefreshTime,
			     OFF1,
			     tq_IntLsa,
			     (void_t) 0);
    if (IAmBorderRtr) {
	(void) task_timer_create(ospf.task,	
				 "LSAGenSum",
				 0,
				 LSRefreshTime,
				 OFF2,
				 tq_SumLsa,
				 (void_t) 0);
    }
	
    /* create age timers */
    (void) task_timer_create(ospf.task,	
			     "LSDBIntAge",
			     0,
			     DbAgeTime,
			     OFF4,
			     tq_int_age,
			     (void_t) 0);
    (void) task_timer_create(ospf.task,	
			     "LSDBSumAge",
			     0,
			     DbAgeTime,
			     OFF5,
			     tq_sum_age,
			     (void_t) 0);
    (void) task_timer_create(ospf.task,	
			     "LSDBAseAge",
			     0,
			     AseAgeTime,
			     OFF6,
			     tq_ase_age,
			     (void_t) 0);

    /* ack timer */
    ospf.timer_ack = task_timer_create(ospf.task,
				       "Ack",
				       TIMERF_HIPRIO,
				       0,		/* OSPF_T_ACK */
				       (time_t) 0,
				       tq_ack,
				       (void_t) 0);

    /* Setup the areas */
    AREA_LIST(a) {
	struct ospf_lsdb_list *txq = LLNULL;
    
	a->timer_lock = task_timer_create(ospf.task,
					  "Lock",
					  (flag_t) 0,
					  (time_t) MinLSInterval,
					  (time_t) area_offset++,
					  tq_lsa_lock,
					  (void_t) a);

	/* build_rtr and run spf */
	reset_rtr_lock(a);
	a->build_rtr = FALSE;
	a->spfsched |= build_rtr_lsa(a, &txq, 0);

	/* generate default summary for area */
	if (IAmBorderRtr &&
	    BIT_TEST(a->area_flags, OSPF_AREAF_STUB_DEFAULT)) {
	    build_sum_dflt(a);
	}

	ospf_freeq((struct Q **)&txq, ospf_lsdblist_index);
    } AREA_LIST_END(a) ;
	
    /* Set timers for virtual links */
    if (IAmBorderRtr && ospf.vcnt) {
	VINTF_LIST(intf) {
	    ospf_interface_init(intf, 0);
	} VINTF_LIST_END(intf) ;
    }

    /* Schedule an SPF run */
    ospf_spf_sched();

    /* Indicate we are active */
    redirect_disable(RTPROTO_OSPF);

#ifdef	PROTO_SNMP
    ospf_init_mib(TRUE);
#endif	/* PROTO_SNMP */
#ifdef  OSPF_HPMIB
    ospf_init_hpmib(TRUE);
#endif  /* OSPF_HPMIB */
}


/*
 *	OSPF protocol initialization
 */
void
ospf_init __PF0(void)
{

    switch (ospf.ospf_admin_stat) {
    case OSPF_ENABLED:
	/* XXX - If no RouterId, defer startup */
	ospf_startup();
	break;

    case OSPF_DISABLED:
	ospf_cleanup(ospf.task);
	assert(!ospf.task);
	break;

    default:
	assert(FALSE);
    }
}


static int
ospf_adv_dstmatch __PF3(p, void_t,
			dst, sockaddr_un *,
			ps_data, void_t)
{
    adv_entry *adv = (adv_entry *) p;
    register u_int32 ls_tag = ntohl(LS_ASE_TAG((struct LSDB *) ps_data));

    if (BIT_TEST(ls_tag, PATH_OSPF_TAG_TRUSTED)) {
	/* Extract arbitrary data from tag */

	ls_tag = (ls_tag & PATH_OSPF_TAG_USR_MASK) >> PATH_OSPF_TAG_USR_SHIFT;
    }

    if (BIT_TEST(adv->adv_result.res_flag, OSPF_EXPORT_TAG) &&
	OSPF_ADV_TAG(adv) != ls_tag) {
	return FALSE;
    }

    return TRUE;
}


static int
ospf_adv_compare __PF2(p1, void_t,
		       p2, void_t)
{
    register adv_entry *adv1 = (adv_entry *) p1;
    register adv_entry *adv2 = (adv_entry *) p2;

    if (adv1->adv_result.res_flag != adv2->adv_result.res_flag) {
	return FALSE;
    }

    if (BIT_TEST(adv1->adv_result.res_flag, OSPF_EXPORT_TAG)) {
	if (OSPF_ADV_TAG(adv1) != OSPF_ADV_TAG(adv2)) {
	    return FALSE;
	}
    }

    return TRUE;
}


static char *
ospf_adv_print __PF2(p, void_t,
		     first, int)
{
    adv_entry *adv = (adv_entry *) p;
    static char line[LINE_MAX];

    *line = (char) 0;

    if (BIT_TEST(adv->adv_result.res_flag, OSPF_EXPORT_TYPE1|OSPF_EXPORT_TYPE2)) {
	if (BIT_TEST(adv->adv_result.res_flag, OSPF_EXPORT_TYPE1)) {
	    (void) strcat(line, "type 1");
	}

	if (BIT_TEST(adv->adv_result.res_flag, OSPF_EXPORT_TYPE2)) {
	    (void) strcat(line, "type 2");
	}
    } else {
	(void) strcat(line, "type 1,2");
    }

    if (BIT_TEST(adv->adv_result.res_flag, OSPF_EXPORT_TAG)) {
	register u_int32 tag = OSPF_ADV_TAG(adv);

	if (BIT_TEST(tag, PATH_OSPF_TAG_TRUSTED)) {
	    (void) sprintf(line, " tag as %u",
			   (tag & PATH_OSPF_TAG_USR_MASK) >> PATH_OSPF_TAG_USR_SHIFT);
	} else {
	    (void) sprintf(line, "tag %u",
			   tag);
	}
    }

    return line;
}

adv_psfunc ospf_adv_psfunc = {
    0,
    ospf_adv_dstmatch,
    ospf_adv_compare,
    ospf_adv_print,
    0,
} ;

