#ident	"@(#)bgp_init.c	1.4"
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
#include "bgp_proto.h"
#include "bgp.h"
#include "bgp_var.h"
#include "krt.h"

/*
 * Miscellany
 */
int doing_bgp = FALSE;			/* Is BGP active? */
trace *bgp_default_trace_options;	/* Trace flags from parser */
pref_t bgp_default_preference;		/* Preference for BGP routes */
pref_t bgp_default_preference2;		/* Alternate preference for BGP peer */
metric_t bgp_default_metric;

bgpPeerGroup *bgp_groups;		/* Linked list of BGP groups */
int bgp_n_groups;			/* Number of BGP groups */

int bgp_n_peers;			/* Number of BGP peers */
int bgp_n_unconfigured;			/* Number of unconfigured peers */
int bgp_n_established;			/* Number of established peers */

bgpProtoPeer *bgp_protopeers;		/* Link list of BGP proto-peers */
int bgp_n_protopeers;			/* Number of active protopeers */

adv_entry *bgp_import_list = NULL;	/* List of BGP advise entries */
adv_entry *bgp_import_aspath_list = NULL;	/* List of BGP import aspath policy */
adv_entry *bgp_export_list = NULL;	/* List of BGP export entries */

static u_short bgp_port;		/* Well known BGP port */
task *bgp_listen_task = (task *) 0;	/* Task we use for listening */
task_timer *bgp_listen_timer = (task_timer *) 0; /* Initial delay timer */

static bgpPeer *bgp_free_list = NULL;	/* List of free'd peer structures */
static int bgp_n_free = 0;		/* Number of free peer structures */

static bgpPeerGroup *bgp_group_free_list = NULL; /* Free list for groups */
static int bgp_n_groups_free = 0;	/* Number of free group structures */

static int bgp_n_ifa_policy_groups = 0;	/* Number of groups with ifa policy */

const flag_t bgp_trace_masks[BGP_PACKET_MAX] = {
    TR_ALL,			/* 0 - Invalid */
    TR_BGP_DETAIL_OPEN,		/* 1 - Open */
    TR_BGP_DETAIL_UPDATE,	/* 2 - Update */
    TR_ALL,			/* 3 - Notify */
    TR_BGP_DETAIL_KEEPALIVE,	/* 4 - Keepalive */
} ;

const bits bgp_trace_types[] = {
    { TR_DETAIL,	"detail packets" },
    { TR_DETAIL_SEND,	"detail send packets" },
    { TR_DETAIL_RECV,	"detail recv packets" },
    { TR_PACKET,	"packets" },
    { TR_PACKET_SEND,	"send packets" },
    { TR_PACKET_RECV,	"recv packets" },
    { TR_DETAIL_1,	"detail open" },
    { TR_DETAIL_SEND_1,	"detail send open" },
    { TR_DETAIL_RECV_1,	"detail recv open" },
    { TR_PACKET_1,	"open" },
    { TR_PACKET_SEND_1,	"send open" },
    { TR_PACKET_RECV_1,	"recv open" },
    { TR_DETAIL_2,	"detail update" },
    { TR_DETAIL_SEND_2,	"detail send update" },
    { TR_DETAIL_RECV_2,	"detail recv update" },
    { TR_PACKET_2,	"update" },
    { TR_PACKET_SEND_2,	"send update" },
    { TR_PACKET_RECV_2,	"recv update" },
    { TR_DETAIL_3,	"detail keepalive" },
    { TR_DETAIL_SEND_3,	"detail send keepalive" },
    { TR_DETAIL_RECV_3,	"detail recv keepalive" },
    { TR_PACKET_3,	"keepalive" },
    { TR_PACKET_SEND_3,	"send keepalive" },
    { TR_PACKET_RECV_3,	"recv keepalive" },
    { 0, NULL }
};

/*
 * Forward declarations
 */
PROTOTYPE(bgp_terminate, static void, (task *));
PROTOTYPE(bgp_fake_terminate, static void, (task *));
PROTOTYPE(bgp_dump, static void, (task *, FILE *));
PROTOTYPE(bgp_group_dump, static void, (task *, FILE *));
PROTOTYPE(bgp_peer_dump, static void, (task *, FILE *));
PROTOTYPE(bgp_pp_dump, static void, (task *, FILE *));
PROTOTYPE(bgp_pp_timeout, static void, (task_timer *, time_t));
PROTOTYPE(bgp_connect_start, static void, (bgpPeer *));
PROTOTYPE(bgp_traffic_timeout, static void, (task_timer *, time_t));
PROTOTYPE(bgp_connect_timeout, static void, (task_timer *, time_t));
PROTOTYPE(bgp_init_traffic_timer, static void, (bgpPeer *));
PROTOTYPE(bgp_cleanup, static void, (task *));
PROTOTYPE(bgp_peer_start, static void, (bgpPeer *, time_t));
PROTOTYPE(bgp_set_peer_if, static if_addr *, (bgpPeer *, int));
PROTOTYPE(bgp_ifachange, static void, (task *, if_addr *));

/*
 * Pretty-printing info
 */
const bits bgp_flag_bits[] =
{
    {BGPF_DELETE, "Delete"},
    {BGPF_UNCONFIGURED, "Unconfigured"},
    {BGPF_TRY_CONNECT, "TryConnect"},
    {BGPF_WRITEFAILED, "WriteFailed"},
    {BGPF_IDLED, "Idled"},
    {BGPF_GENDEFAULT, "GenDefault"},
    {BGPF_RT_TIMER, "RtTimerActive"},
    {BGPF_SEND_RTN, "WriteActive"},
    {BGPF_CLEANUP, "CleanupScheduled"},
    {BGPF_INITIALIZING, "Initializing"},
    {0}
};

const bits bgp_group_flag_bits[] =
{
    {BGPGF_DELETE, "Delete"},
    {BGPGF_IDLED, "Idled"},
    {BGPGF_RT_TIMER, "RtTimerActive"},
    {0}
};

const bits bgp_option_bits[] =
{
    {BGPO_METRIC_OUT, "MetricOut"},
    {BGPO_LOCAL_AS, "LocalAs"},
    {BGPO_NOGENDEFAULT, "NoGenDefault"},
    {BGPO_GATEWAY, "Gateway"},
    {BGPO_PREFERENCE, "Preference"},
    {BGPO_LCLADDR, "LocalAddress"},
    {BGPO_HOLDTIME, "HoldTime"},
    {BGPO_PASSIVE, "Passive"},
    {BGPO_VERSION, "VersionSpecified"},
    {BGPO_SETPREF, "UseLocalPref"},
    {BGPO_KEEPALL, "KeepAll"},
    {BGPO_KEEPNONE, "KeepNone"},
    {BGPO_ANALRETENTIVE, "Anal"},
    {BGPO_NOAGGRID, "NoAggrID"},
    {BGPO_NOAUTHCHECK, "NoAuthCheck"},
    {BGPO_KEEPALIVESALWAYS, "KeepAlivesAlways"},
    {BGPO_PREFERENCE2, "Preference2"},
    {BGPO_V3ASLOOPOKAY, "V3ASLoopOk"},
    {BGPO_NOV4ASLOOP, "NoV4ASLoop"},
    {BGPO_TTL, "Ttl"},
    {BGPO_LOGUPDOWN, "LogUpDown"},
    {BGPO_MED, "MED"},
    {BGPO_IGNOREFIRSTASHOP, "IgnoreFirstASHop"},
    {0}
};

const bits bgp_state_bits[] =
{
    { 0, "NoState" },
    {BGPSTATE_IDLE, "Idle"},
    {BGPSTATE_CONNECT, "Connect"},
    {BGPSTATE_ACTIVE, "Active"},
    {BGPSTATE_OPENSENT, "OpenSent"},
    {BGPSTATE_OPENCONFIRM, "OpenConfirm"},
    {BGPSTATE_ESTABLISHED, "Established"},
    {0}
};
BGP_MAKE_CODES(bgp_state_codes, bgp_state_bits);

const bits bgp_event_bits[] =
{
    { 0, "NoEvent" },
    {BGPEVENT_START, "Start"},
    {BGPEVENT_STOP, "Stop"},
    {BGPEVENT_OPEN, "Open"},
    {BGPEVENT_CLOSED, "Closed"},
    {BGPEVENT_OPENFAIL, "OpenFail"},
    {BGPEVENT_ERROR, "TransportError"},
    {BGPEVENT_CONNRETRY, "ConnectRetry"},
    {BGPEVENT_HOLDTIME, "HoldTime"},
    {BGPEVENT_KEEPALIVE, "KeepAlive"},
    {BGPEVENT_RECVOPEN, "RecvOpen"},
    {BGPEVENT_RECVKEEPALIVE, "RecvKeepAlive"},
    {BGPEVENT_RECVUPDATE, "RecvUpdate"},
    {BGPEVENT_RECVNOTIFY, "RecvNotify"},
    {0}
};
BGP_MAKE_CODES(bgp_event_codes, bgp_event_bits);

const bits bgp_message_type_bits[] =
{
    { 0, "invalid" },
    {BGP_OPEN, "Open"},
    {BGP_UPDATE, "Update"},
    {BGP_NOTIFY, "Notification"},
    {BGP_KEEPALIVE, "KeepAlive"},
    {0}
};
BGP_MAKE_CODES(bgp_message_type_codes, bgp_message_type_bits);

const bits bgp_error_bits[] =
{
    { 0, "None" },
    {BGP_ERR_HEADER, "Message Header Error"},
    {BGP_ERR_OPEN, "Open Message Error"},
    {BGP_ERR_UPDATE, "Update Message Error"},
    {BGP_ERR_HOLDTIME, "Hold Timer Expired Error"},
    {BGP_ERR_FSM, "Finite State Machine Error"},
    {BGP_CEASE, "Cease"},
    {0}
};
BGP_MAKE_CODES(bgp_error_codes, bgp_error_bits);

const bits bgp_header_error_bits[] =
{
    {BGP_ERR_UNSPEC, "unspecified error"},
    {BGP_ERRHDR_UNSYNC, "lost connection synchronization"},
    {BGP_ERRHDR_LENGTH, "bad length"},
    {BGP_ERRHDR_TYPE, "bad message type"},
    {0}
};
BGP_MAKE_CODES(bgp_header_error_codes, bgp_header_error_bits);

const bits bgp_open_error_bits[] =
{
    {BGP_ERR_UNSPEC, "unspecified error"},
    {BGP_ERROPN_VERSION, "unsupported version"},
    {BGP_ERROPN_AS, "bad AS number"},
    {BGP_ERROPN_BGPID, "bad BGP ID"},
    {BGP_ERROPN_AUTHCODE, "unsupported authentication code"},
    {BGP_ERROPN_AUTH, "authentication failure"},
    {0}
};
BGP_MAKE_CODES(bgp_open_error_codes, bgp_open_error_bits);

const bits bgp_update_error_bits[] =
{
    {BGP_ERR_UNSPEC, "unspecified error"},
    {BGP_ERRUPD_ATTRLIST, "invalid attribute list"},
    {BGP_ERRUPD_UNKNOWN, "unknown well known attribute"},
    {BGP_ERRUPD_MISSING, "missing well known attribute"},
    {BGP_ERRUPD_FLAGS, "attribute flags error"},
    {BGP_ERRUPD_LENGTH, "bad attribute length"},
    {BGP_ERRUPD_ORIGIN, "bad ORIGIN attribute"},
    {BGP_ERRUPD_ASLOOP, "AS loop detected"},
    {BGP_ERRUPD_NEXTHOP, "invalid NEXT_HOP"},
    {BGP_ERRUPD_OPTATTR, "error with optional attribute"},
    {BGP_ERRUPD_BADNET, "bad address/prefix field"},
    {BGP_ERRUPD_ASPATH, "AS path attribute problem"},
    {0}
};
BGP_MAKE_CODES(bgp_update_error_codes, bgp_update_error_bits);

const bits bgp_group_bits[] =
{
    {BGPG_EXTERNAL, "External" },
    {BGPG_INTERNAL, "Internal" },
    {BGPG_INTERNAL_IGP, "IGP" },
    {BGPG_INTERNAL_RT, "Routing"},
    {BGPG_TEST, "Test" },
    {0}
};
BGP_MAKE_CODES(bgp_group_codes, bgp_group_bits);

#ifdef notdef
const static bits bgp_group_initial_bits[] =
{
    { 0, "X" },
    {BGPG_EXTERNAL, "E" },
    {BGPG_INTERNAL, "IP" },
    {BGPG_INTERNAL_IGP, "I" },
    {BGPG_TEST, "T" },
    {0}
};
#endif

/*
 * BGP memory management.
 *
 * There are several types of objects which BGP manages the allocation
 * of.  Peer structures are one of them, but are malloc'd and free'd to
 * agree with the parser.
 *
 * Other objects are the 4k input packet buffers, output packet buffers
 * and structures used for sorting updates.  These we allocate and free
 * from a pool of 4k blocks maintained by the task_block_*() routines.
 *
 * Proto-peer structures are allocated from a separate (appropriately
 * sized) pool.
 */
block_t bgp_buf_blk_index = (block_t) 0;	/* returned by call to task_block_init() */
static block_t bgp_pp_blk_index = (block_t) 0;	/* ibid */

/*
 * Outgoing data is buffered in an output packet buffer with a structure
 * to keep track of where the data is in the packet.  We keep a buffer
 * back to avoid some of the cost of clearing the buffers before freeing
 * them.
 */
static bgpOutBuffer *bgp_out_buffer = (bgpOutBuffer *) 0;
static block_t bgp_obuf_blk_index = (block_t) 0;

/*
 * Interface lists are similarly allocated via task_block*.  The
 * pointer for this one is here.
 */
static block_t bgp_iflist_blk = (block_t) 0;

/*
 * Connect timer slot maintenance.  We attempt to spread connection
 * attempt times into slots to reduce the impact of starting a bunch
 * of connections all at once.  This is an utter waste of time if
 * you've only got a few peers, but may help if you've got a lot.
 *
 * This is the slot array.
 */
static byte bgp_connect_slots[BGPCONN_N_SLOTS] = {0};

/*
 * Macro to determine the buffer sizes to which a socket should be set.
 */
#define	BGP_RX_BUFSIZE(size) \
    (((size) == 0) ? BGP_RECV_BUFSIZE : \
    (((size) >= task_maxpacket) ? task_maxpacket : (size)))

#define	BGP_TX_BUFSIZE(size) \
    (((size) == 0) ? BGP_SEND_BUFSIZE : \
    (((size) >= task_maxpacket) ? task_maxpacket : (size)))

/*
 *	Pretty name formatting
 */

/*
 * bgp_make_names - routine to create a pretty names for a BGP peer
 */
static void
bgp_make_names __PF1(bnp, bgpPeer *)
{
    (void) sprintf(bnp->bgp_name, "%A (%s AS %u)",
		   bnp->bgp_addr,
		   trace_state(bgp_group_bits, bnp->bgp_type),
		   (u_int) bnp->bgp_peer_as);
    if (bnp->bgp_local_as != inet_autonomous_system) {
	(void) sprintf(bnp->bgp_task_name, "BGP_%u_%u",
		       (u_int) bnp->bgp_peer_as,
		       (u_int) bnp->bgp_local_as);
    } else {
	(void) sprintf(bnp->bgp_task_name, "BGP_%u",
		       (u_int) bnp->bgp_peer_as);
    }
}


/*
 * bgp_make_pp_name - make a name for a protopeer
 */
static void
bgp_make_pp_name __PF1(bpp, bgpProtoPeer *)
{
    (void) sprintf(bpp->bgpp_name, "%#A (proto)",
		   bpp->bgpp_task->task_addr);
}

/*
 * bgp_make_group_names - make some names for a group
 */
static void
bgp_make_group_names __PF1(bgp, bgpPeerGroup *)
{
    if (bgp->bgpg_type == BGPG_INTERNAL ||
	bgp->bgpg_type == BGPG_INTERNAL_IGP ||
	bgp->bgpg_type == BGPG_INTERNAL_RT ||
	bgp->bgpg_local_as != inet_autonomous_system) {
        (void) sprintf(bgp->bgpg_name, "group type %s AS %u",
		       trace_state(bgp_group_bits, bgp->bgpg_type),
		       (u_int) bgp->bgpg_peer_as);
	(void) sprintf(bgp->bgpg_task_name, "BGP_Group_%u",
		       (u_int) bgp->bgpg_peer_as);
    } else {
        (void) sprintf(bgp->bgpg_name, "group type %s AS %u local %u",
		       trace_state(bgp_group_bits, bgp->bgpg_type),
		       (u_int) bgp->bgpg_peer_as,
		       (u_int) bgp->bgpg_local_as);
	(void) sprintf(bgp->bgpg_task_name, "BGP_Group_%u_%u",
		       (u_int) bgp->bgpg_peer_as,
		       (u_int) bgp->bgpg_local_as);
    }
}


/*
 *	Peer structure allocation and basic unconfigured peer support
 */

/*
 * bgp_peer_alloc - return a new peer structure
 */
static bgpPeer *
bgp_peer_alloc __PF1(tp, task *)
{
    bgpPeer *bnp;

    /*
     * If we've got one in the free list, return it, else malloc() one.
     */
    if (bgp_free_list != NULL) {
	bnp = bgp_free_list;
	bgp_free_list = bnp->bgp_next;
	bgp_n_free--;
    } else {
	bnp = (bgpPeer *) task_mem_malloc(tp, sizeof(bgpPeer));
    }

    return (bnp);
}

/*
 * bgp_peer_free - move peer structure onto free list
 */
static void
bgp_peer_free __PF1(bnp, bgpPeer *)
{
    /*
     * XXX Someday might free things if we get more than a certain number
     * on the free list.
     */
    bnp->bgp_next = bgp_free_list;
    bgp_free_list = bnp;
    bgp_n_free++;
}


/*
 * bgp_peer_free_all - really free all structures on the free list
 */
static void
bgp_peer_free_all __PF0(void)
{
    bgpPeer *bnp;
    bgpPeer *bnpnext;

    for (bnp = bgp_free_list; bnp != NULL; bnp = bnpnext) {
	bnpnext = bnp->bgp_next;
	task_mem_free(bgp_listen_task, (caddr_t) bnp);
    }
    bgp_free_list = NULL;
    bgp_n_free = 0;
}


/*
 * bgp_peer_add - add a peer to a peer group
 */
static void
bgp_peer_add __PF2(bgp, bgpPeerGroup *,
		   bnp, bgpPeer *)
{
    flag_t flags;

    /*
     * Put deleted before unconfigured, and normal before
     * deleted.
     */
    flags = 0;
    if (!BIT_TEST(bnp->bgp_flags, BGPF_UNCONFIGURED)) {
	flags = BGPF_UNCONFIGURED;
	if (!BIT_TEST(bnp->bgp_flags, BGPF_DELETE)) {
	    BIT_SET(flags, BGPF_DELETE);
	}
    }

    if (bgp->bgpg_peers == NULL
	|| BIT_TEST(bgp->bgpg_peers->bgp_flags, flags)) {
	bnp->bgp_next = bgp->bgpg_peers;
	bgp->bgpg_peers = bnp;
    } else {
	register bgpPeer *bnp2;

	for (bnp2 = bgp->bgpg_peers;
	     bnp2->bgp_next != NULL;
	     bnp2 = bnp2->bgp_next) {
	    if (BIT_TEST(bnp2->bgp_next->bgp_flags, flags)) {
		break;
	    }
	}

	bnp->bgp_next = bnp2->bgp_next;
	bnp2->bgp_next = bnp;
    }
    bnp->bgp_group = bgp;
    bgp->bgpg_n_peers++;
    bgp_n_peers++;
    if (BIT_TEST(bnp->bgp_flags, BGPF_UNCONFIGURED)) {
        bgp_n_unconfigured++;
    }
#ifdef	PROTO_SNMP
    bgp_sort_add(bnp);
#endif	/* PROTO_SNMP */
}


/*
 * bgp_peer_remove - remove a peer from its peer group
 */
static void
bgp_peer_remove __PF1(bnp, bgpPeer *)
{
    bgpPeerGroup *bgp;

    bgp = bnp->bgp_group;
    if (bgp->bgpg_peers == NULL) {
	goto fuckup;
    }

#ifdef	PROTO_SNMP
    bgp_sort_remove(bnp);
#endif	/* PROTO_SNMP */
    if (bgp->bgpg_peers == bnp) {
	bgp->bgpg_peers = bnp->bgp_next;
    } else {
	bgpPeer *bnp2;

	for (bnp2 = bgp->bgpg_peers;
	     bnp2->bgp_next != bnp && bnp2->bgp_next != NULL;
	     bnp2 = bnp2->bgp_next) {
	    /* nothing */
	}

	if (bnp2->bgp_next == NULL) {
	    goto fuckup;
	}
	bnp2->bgp_next = bnp->bgp_next;
    }

    bgp->bgpg_n_peers--;
    bgp_n_peers--;
    if (BIT_TEST(bnp->bgp_flags, BGPF_UNCONFIGURED)) {
        bgp_n_unconfigured--;
    }
    return;

fuckup:
    /* Not found in list */
    assert(FALSE);
}


/*
 *	Group structure allocation.  Only used during (re)configuration
 */

/*
 * bgp_group_alloc - return a new group structure
 */
static bgpPeerGroup *
bgp_group_alloc()
{
    bgpPeerGroup *bgp;

    /*
     * If we've got one in the free list, return it, else malloc() one.
     */
    if (bgp_group_free_list != NULL) {
	bgp = bgp_group_free_list;
	bgp_group_free_list = bgp->bgpg_next;
	bgp_n_groups_free--;
    } else {
	bgp = (bgpPeerGroup *) task_mem_malloc(bgp_listen_task,
					       sizeof(bgpPeerGroup));
    }

    return (bgp);
}

/*
 * bgp_group_free - move group structure onto free list
 */
static void
bgp_group_free __PF1(bgp, bgpPeerGroup *)
{
    /*
     * XXX Someday might free things if we get more than a certain number
     * on the free list.
     */
    bgp->bgpg_next = bgp_group_free_list;
    bgp_group_free_list = bgp;
    bgp_n_groups_free++;
}


/*
 * bgp_group_free_all - really free all structures on the free list
 */
static void
bgp_group_free_all __PF0(void)
{
    bgpPeerGroup *bgp;
    bgpPeerGroup *bgpnext;

    for (bgp = bgp_group_free_list; bgp != NULL; ) {
	bgpnext = bgp->bgpg_next;
	task_mem_free(bgp_listen_task, (caddr_t) bgp);
	bgp = bgpnext;
    }
    bgp_group_free_list = NULL;
    bgp_n_groups_free = 0;
}


/*
 * bgp_group_add - add a group to the group list
 */
static void
bgp_group_add __PF1(bgp, bgpPeerGroup *)
{
    /*
     * Try to keep them in the order they appear in the configuration file
     */
    bgp->bgpg_next = NULL;
    if (bgp_groups == NULL) {
	bgp_groups = bgp;
    } else {
	register bgpPeerGroup *bgp2;

	for (bgp2 = bgp_groups;
	  bgp2->bgpg_next != NULL;
	  bgp2 = bgp2->bgpg_next) {
	    /* nothing */
	}

	bgp2->bgpg_next = bgp;
    }
    bgp_n_groups++;
}


/*
 * bgp_group_remove - remove a group from its peer group
 */
static void
bgp_group_remove __PF1(bgp, bgpPeerGroup *)
{
    if (bgp_groups == NULL) {
	goto fuckup;
    }
    if (bgp == bgp_groups) {
	bgp_groups = bgp->bgpg_next;
    } else {
	bgpPeerGroup *bgp2;

	for (bgp2 = bgp_groups;
	  bgp2->bgpg_next != bgp && bgp2->bgpg_next != NULL;
	  bgp2 = bgp2->bgpg_next) {
	    /* nothing */
	}

	if (bgp2->bgpg_next == NULL) {
	    goto fuckup;
	}
	bgp2->bgpg_next = bgp->bgpg_next;
    }

    bgp_n_groups--;
    return;

fuckup:
    /* Not found in group list */
    assert(FALSE);
}



/*
 *
 *	Memory block allocation routines
 *
 */

/*
 * bgp_buffer_alloc - allocate and initialize an input buffer
 */
static void
bgp_buffer_alloc __PF1(bp, bgpBuffer *)
{
    /*
     * Allocate memory.  Init bucket if we need it.  Fix up pointers
     * in buffer allocation
     */
    if (!bgp_buf_blk_index) {
	bgp_buf_blk_index = task_block_init(BGPRECVBUFSIZE, "bgp_buffer");
    }
    if (bp->bgpb_buffer != NULL) {
	trace_log_tf(bgp_default_trace_options,
		     0,
		     LOG_ERR,
		     ("bgp_buffer_alloc: buffer already allocated, something screwed up!!"));
    } else {
	bp->bgpb_buffer = (byte *)task_block_alloc(bgp_buf_blk_index);
    }
    bp->bgpb_bufpos = bp->bgpb_readptr = bp->bgpb_buffer;
    bp->bgpb_endbuf = bp->bgpb_buffer + BGPRECVBUFSIZE;
}


/*
 * bgp_buffer_free - free an input buffer
 */
static void
bgp_buffer_free __PF1(bp, bgpBuffer *)
{
    if (bp->bgpb_buffer != NULL) {
	task_block_free(bgp_buf_blk_index, (void_t) bp->bgpb_buffer);
    } else {
	trace_log_tf(bgp_default_trace_options,
		     0,
		     LOG_ERR,
		     ("bgp_buffer_free: buffer not present, something screwed up!!"));
    }
    bp->bgpb_buffer = bp->bgpb_bufpos = NULL;
    bp->bgpb_readptr = bp->bgpb_endbuf = NULL;
}


/*
 * bgp_buffer_copy - copy memory buffer from one structure to another
 */
static void
bgp_buffer_copy __PF2(bpfrom, bgpBuffer *,
		      bpto, bgpBuffer *)
{
    if (bpto->bgpb_buffer != NULL) {
	trace_log_tf(bgp_default_trace_options,
		     0,
		     LOG_ERR,
		     ("bgp_buffer_copy: destination already has buffer allocated!!"));
	bgp_buffer_free(bpto);
    }
    *bpto = *bpfrom;	/* struct copy */
    bpfrom->bgpb_buffer = bpfrom->bgpb_bufpos = NULL;
    bpfrom->bgpb_readptr = bpfrom->bgpb_endbuf = NULL;
}


/*
 *	Output buffer management
 */

/*
 * bgp_outbuf_alloc - allocate an output buffer for a peer
 *		      and copy data into it.
 */
static void
bgp_outbuf_alloc __PF3(bnp, bgpPeer *,
		       data, byte *,
		       len, size_t)
{
    bgpOutBuffer *obp;
    byte *startp, *endp;

    assert(!bnp->bgp_outbuf && len && len <= BGPMAXV4SENDPKTSIZE);

    if (!bgp_obuf_blk_index) {
	bgp_obuf_blk_index = task_block_init(BGPHMAXPACKETSIZE, "bgp_outbuf");
    }

    /*
     * If we have a buffer queued, use it.  Otherwise fetch one.
     */
    if (bgp_out_buffer) {
	obp = bgp_out_buffer;
	bgp_out_buffer = (bgpOutBuffer *) 0;
	obp->bgpob_flags = (flag_t) 0;
    } else {
	obp = (bgpOutBuffer *) task_block_alloc(bgp_obuf_blk_index);
	obp->bgpob_clearto = ((byte *) obp) + sizeof(bgpOutBuffer);
    }
    startp = ((byte *) obp) + sizeof(bgpOutBuffer);
    obp->bgpob_start = startp;
    endp = startp + len;
    obp->bgpob_end = endp;
    if (endp > obp->bgpob_clearto) {
	obp->bgpob_clearto = endp;
    }
    bcopy((void_t) data, (void_t) startp, len);

    bnp->bgp_outbuf = obp;
}


/*
 * bgp_outbuf_free - free an output buffer attached to a peer
 */
static void
bgp_outbuf_free __PF1(bnp, bgpPeer *)
{
    bgpOutBuffer *obp;

    obp = bnp->bgp_outbuf;
    if (!obp) {
	return;
    }
    bnp->bgp_outbuf = (bgpOutBuffer *) 0;

    if (!bgp_out_buffer) {
	bgp_out_buffer = obp;
	return;
    }

    if ((bgp_out_buffer->bgpob_clearto - (byte *) bgp_out_buffer)
		< (obp->bgpob_clearto - (byte *) obp)) {
	bgpOutBuffer *tmp = bgp_out_buffer;

	bgp_out_buffer = obp;
	obp = tmp;
    }

    bzero((void_t) obp, (size_t) (obp->bgpob_clearto - (byte *) obp));
    task_block_free_clear(bgp_obuf_blk_index, (void_t) obp);
}


/*
 * bgp_outbuf_free_all - free the queued buffer, if any
 */
static void
bgp_outbuf_free_all __PF0(void)
{
    if (bgp_out_buffer) {
	bzero((void_t) bgp_out_buffer,
	      (size_t) (bgp_out_buffer->bgpob_clearto-(byte *)bgp_out_buffer));
	task_block_free_clear(bgp_obuf_blk_index, (void_t) bgp_out_buffer);
	bgp_out_buffer = (bgpOutBuffer *) 0;
    }
}



/*
 *	Protopeer creation/deletion
 */

/*
 * bgp_pp_create - allocate and init a protopeer structure
 */
static bgpProtoPeer *
bgp_pp_create __PF2(tp, task *,
		    lcladdr, sockaddr_un *)
{
    bgpProtoPeer *ppp;

    /*
     * If no bucket allocated, do it now.
     */
    if (!bgp_pp_blk_index) {
	bgp_pp_blk_index = task_block_init(sizeof(bgpProtoPeer), "bgpProtoPeer");
    }
    ppp = (bgpProtoPeer *)task_block_alloc(bgp_pp_blk_index);

    /*
     * Initialize structure and hook it into chain.
     */
    ppp->bgpp_task = tp;
    tp->task_data = (caddr_t)ppp;
    ppp->bgpp_myaddr = lcladdr;
    ppp->bgpp_connect_time = bgp_time_sec;
    bgp_make_pp_name(ppp);
    bgp_buffer_alloc(&(ppp->bgpp_inbuf));

    ppp->bgpp_next = bgp_protopeers;
    bgp_protopeers = ppp;
    bgp_n_protopeers++;
    return (ppp);
}


/*
 * bgp_pp_delete - free a protopeer entirely
 */
void
bgp_pp_delete(ppp)
bgpProtoPeer *ppp;
{
    if (ppp->bgpp_timeout_timer) {
	task_timer_delete(ppp->bgpp_timeout_timer);
    }
    if (ppp->bgpp_task) {
	task_delete(ppp->bgpp_task);
    }
    if (ppp->bgpp_myaddr) {
	sockfree(ppp->bgpp_myaddr);
	ppp->bgpp_myaddr = (sockaddr_un *) 0;
    }
    if (ppp->bgpp_buffer) {
	bgp_buffer_free(&(ppp->bgpp_inbuf));
    }

    if (ppp == bgp_protopeers) {
	bgp_protopeers = ppp->bgpp_next;
    } else {
        bgpProtoPeer *pp2;

	for (pp2 = bgp_protopeers; pp2 != NULL; pp2 = pp2->bgpp_next) {
	    if (pp2->bgpp_next == ppp) {
		pp2->bgpp_next = ppp->bgpp_next;
		break;
	    }
	}

	/* Not found in list */
	assert(pp2);
    }
    bgp_n_protopeers--;
    task_block_free(bgp_pp_blk_index, (void_t) ppp);
}


/*
 * bgp_pp_delete_all - delete all protopeers
 */
static void
bgp_pp_delete_all __PF0(void)
{
    while (bgp_protopeers != NULL) {
	bgp_pp_delete(bgp_protopeers);
    }
}


/*
 *	Group/peer task creation
 */


/*
 * bgp_task_create - create a task for a BGP peer
 */
static void
bgp_task_create __PF1(bnp, bgpPeer *)
{
    task *tp;

    tp = bnp->bgp_task = task_alloc(bnp->bgp_task_name,
				    TASKPRI_EXTPROTO,
				    bnp->bgp_trace_options);
    tp->task_addr = sockdup(bnp->bgp_addr);
    tp->task_rtproto = RTPROTO_BGP;
    task_set_terminate(tp, bgp_fake_terminate);
    task_set_dump(tp, bgp_peer_dump);
    tp->task_data = (caddr_t) bnp;
    BIT_SET(tp->task_flags, TASKF_LOWPRIO);
    if (!task_create(tp)) {
	task_quit(EINVAL);
    }
}


static void
bgp_group_task_create __PF1(bgp, bgpPeerGroup *)
{
    task *tp;

    tp = task_alloc(bgp->bgpg_task_name,
		    TASKPRI_EXTPROTO,
		    bgp->bgpg_trace_options);
    tp->task_rtproto = RTPROTO_BGP;
    tp->task_data = (caddr_t) bgp;
    task_set_terminate(tp, bgp_fake_terminate);
    task_set_dump(tp, bgp_group_dump);
    BIT_SET(tp->task_flags, TASKF_LOWPRIO);
    if (!task_create(tp)) {
	task_quit(EINVAL);
    }
    bgp->bgpg_task = tp;
}


/*
 *	Socket manipulation
 */

/*
 * bgp_recv_change - change the receiver for read ready requests
 */
void
bgp_recv_change(bnp, recv_rtn, recv_rtn_name)
bgpPeer *bnp;
void (*recv_rtn)();
const char *recv_rtn_name;
{
    task *tp;

    /*
     * Make sure there's already a socket associated with this
     */
    tp = bnp->bgp_task;
    /* Socket not initialized */
    assert(tp->task_socket >= 0);

    trace_tp(tp,
	     TR_NORMAL,
	     0,
	     ("bgp_recv_change: peer %s receiver changed to %s",
	      bnp->bgp_name,
	      recv_rtn_name));
    
    task_set_recv(tp, recv_rtn);
    task_set_socket(tp, tp->task_socket);
}

/*
 * bgp_send_set - set up a write processing routine for this peer's task
 */
PROTOTYPE(bgp_send_set,
	  static void,
	 (bgpPeer *,
	  _PROTOTYPE(send_rtn,
		     void,
		     (task *))));
static void
bgp_send_set(bnp, send_rtn)
bgpPeer *bnp;
_PROTOTYPE(send_rtn,
	   void,
	   (task *));
{
    task *tp;

    trace_tp(bnp->bgp_task,
	     TR_NORMAL,
	     0,
	     ("bgp_set_send: setting task write routine for peer %s",
	      bnp->bgp_name));

    /*
     * Set the task_write routine.  If the socket is open call task_set_socket.
     */
    tp = bnp->bgp_task;

    BIT_SET(bnp->bgp_flags, BGPF_SEND_RTN);
    task_set_write(tp, send_rtn);
    if (tp->task_socket >= 0) {
	task_set_socket(tp, tp->task_socket);
    }
}


/*
 * bgp_send_reset - unset the write processing routine for this peer's task
 */
static void
bgp_send_reset __PF1(bnp, bgpPeer *)
{
    task *tp = bnp->bgp_task;

    trace_tp(tp,
	     TR_NORMAL,
	     0,
	     ("bgp_set_send: removing task write routine from peer %s",
	      bnp->bgp_name));

    /*
     * NULL the write routine in the task.  If the socket is open call
     * task_set_socket() to propagate this.
     */
    BIT_RESET(bnp->bgp_flags, BGPF_SEND_RTN);
    task_set_write(tp, NULL);
    if (tp->task_socket >= 0) {
	task_set_socket(tp, tp->task_socket);
    }
}


/*
 * bgp_set_flash - set the flash and newpolicy routines for a task
 */
void
bgp_set_flash(tp, flash_rtn, newpolicy_rtn)
task *tp;
_PROTOTYPE(flash_rtn,
	   void,
	   (task *,
	    rt_list *));
_PROTOTYPE(newpolicy_rtn,
	   void,
	   (task *,
	    rt_list *));
{
    trace_tp(tp,
	     TR_NORMAL,
	     0,
	     ("bgp_set_flash: setting flash/new policy routines for %s",
	      tp->task_name));

    task_set_flash(tp, flash_rtn);
    task_set_newpolicy(tp, newpolicy_rtn);
}


/*
 * bgp_reset_flash - remove the flash and newpolicy routes from a task
 */
void
bgp_reset_flash __PF1(tp, task *)
{
    trace_tp(tp,
	     TR_NORMAL,
	     0,
	     ("bgp_reset_flash: resetting flash/new policy routes for %s",
	      tp->task_name));

    task_set_flash(tp, NULL);
    task_set_newpolicy(tp, NULL);
}


/*
 * bgp_set_reinit - set a reinit routine for a task
 */
void
bgp_set_reinit(tp, reinit_rtn)
task *tp;
_PROTOTYPE(reinit_rtn,
	   void,
	   (task *));
{
    trace_tp(tp,
	     TR_NORMAL,
	     0,
	     ("bgp_set_reinit: setting reinit routine for %s",
	      tp->task_name));

    task_set_reinit(tp, reinit_rtn);
}


/*
 * bgp_reset_reinit - reset the reinit routine on a task
 */
void
bgp_reset_reinit __PF1(tp, task *)
{
    trace_tp(tp,
	     TR_NORMAL,
	     0,
	     ("bgp_reset_reinit: resetting reinit routine for %s",
	      tp->task_name));

    task_set_reinit(tp, NULL);
}



/*
 * Set send and receive buffers for a task
 */
static void
bgp_recv_setbuf __PF3(tp, task *,
		      recvbuf, size_t,
		      sendbuf, size_t)
{
    int size;

    size = (int) BGP_RX_BUFSIZE(recvbuf);
    if (task_set_option(tp, TASKOPTION_RECVBUF, size) < 0) {
	task_quit(errno);
    }
    size = (int) BGP_TX_BUFSIZE(sendbuf);
    if (task_set_option(tp, TASKOPTION_SENDBUF, size) < 0) {
	task_quit(errno);
    }
}


/*
 * bgp_recv_setopts - set options on a task to be used for reading
 */
static void
bgp_recv_setopts __PF3(tp, task *,
		       recvbuf, size_t,
		       sendbuf, size_t)
{
    if (task_set_option(tp, TASKOPTION_NONBLOCKING, TRUE) < 0) {
	task_quit(errno);
    }
#ifdef  IPTOS_PREC_INTERNETCONTROL
#ifndef	SCO_UW21
    /* do not set TOS, otherwise bgp will dropwithreset
    */
    (void) task_set_option(tp, TASKOPTION_TOS, IPTOS_PREC_INTERNETCONTROL);
#endif	/* SCO_UW21 */
#endif  /* IPTOS_PREC_INTERNETCONTROL */

    bgp_recv_setbuf(tp, recvbuf, sendbuf);

    /* Linger will block the close for the linger timeout, but it doesn't
       work on a non-blocking socket */
    if (task_set_option(tp, TASKOPTION_LINGER, 0) < 0) {
	task_quit(errno);
    }
}

/*
 * bgp_recv_setdirect - set socket for host on direct network
 */
static void
bgp_recv_setdirect __PF1(bnp, bgpPeer *)
{
    task *tp = bnp->bgp_task;

    /* Don't use routing table */
    if (task_set_option(tp, TASKOPTION_DONTROUTE, TRUE) < 0) {
	task_quit(errno);
    }

    /* Set TTL to 1 if supported */
    if (BIT_TEST(bnp->bgp_options, BGPO_TTL)) {
 	(void) task_set_option(tp, TASKOPTION_TTL, bnp->bgp_ttl);
 	bnp->bgp_ttl_current = bnp->bgp_ttl;
    } else {
 	(void) task_set_option(tp, TASKOPTION_TTL, 1);
 	bnp->bgp_ttl_current = 1;
    }
}


/*
 * bgp_recv_setup - set up an open socket to receive data via task
 */
PROTOTYPE(bgp_recv_setup,
	  static void,
	 (bgpPeer *,
	  int,
	  _PROTOTYPE(recv_rtn,
		     void,
		     (task *)),
	  const char *));
static void
bgp_recv_setup(bnp, recv_socket, recv_rtn, recv_rtn_name)
bgpPeer *bnp;
int recv_socket;
_PROTOTYPE(recv_rtn,
	   void,
	   (task *));
const char *recv_rtn_name;
{
    _PROTOTYPE(savesendrtn, void, (task *));
    task *tp = bnp->bgp_task;

    trace_tp(tp,
	     TR_NORMAL,
	     0,
	     ("bgp_recv_setup: peer %s socket %d set for reading",
	      bnp->bgp_name,
	      recv_socket));

    /*
     * If this task has been using a socket, reset it first
     */
    savesendrtn = tp->task_write_method;
    if (tp->task_socket >= 0) {
	task_reset_socket(tp);
    }

    task_set_write(tp, savesendrtn);
    task_set_recv(tp, recv_rtn);
    task_set_socket(tp, recv_socket);

    /*
     * Set up appropriate options
     */
    bgp_recv_setopts(tp, bnp->bgp_recv_bufsize, bnp->bgp_send_bufsize);
    if (bnp->bgp_ifap != NULL && !BIT_TEST(bnp->bgp_options, BGPO_GATEWAY)) {
	bgp_recv_setdirect(bnp);
    }
}


/*
 * bgp_close_socket - close a peer's socket
 */
static void
bgp_close_socket __PF1(bnp, bgpPeer *)
{
    trace_tp(bnp->bgp_task,
	     TR_NORMAL,
	     0,
	     ("bgp_close_socket: peer %s",
	      bnp->bgp_name));

    task_close(bnp->bgp_task);
    bnp->bgp_ttl_current = 0;
    if (bnp->bgp_myaddr != NULL) {
	sockfree(bnp->bgp_myaddr);
	bnp->bgp_myaddr = NULL;
    }
    if (BGP_OPTIONAL_SHAREDIF(bnp->bgp_group) && bnp->bgp_ifap != NULL) {
	IFA_FREE(bnp->bgp_ifap);
	bnp->bgp_ifap = NULL;
    }
}


/*
 *	Interface list maintenance
 */

/*
 * bgp_iflist_add - add a new INTERNAL or TEST peer's interface to
 *		    the interface list, either by referencing an
 *		    existing match or by allocating a new one.
 */
static void
bgp_iflist_add __PF1(bnp, bgpPeer *)
{
    register bgp_ifap_list *ifp;
    register bgpPeerGroup *bgp = bnp->bgp_group;

    switch(bgp->bgpg_type) {
    default:
	return;		/* no need to do this in here */
    case BGPG_INTERNAL:
    case BGPG_TEST:
	break;
    }

    if (!bnp->bgp_ifap) {
	/*
	 * Get a current view of the kernel interfaces. This is necessary to
           allow the  bgp_ifachange processing to set bgp_ifap appropriately.
	 */
	krt_ifcheck();
	if(!bnp->bgp_ifap)
	/* Distant test peer, just return */
	assert(bgp->bgpg_type == BGPG_TEST);
	return;
    }

    for (ifp = bgp->bgpg_ifap_list; ifp; ifp = ifp->bgp_if_next) {
	if (ifp->bgp_if_ifap == bnp->bgp_ifap) {
	    ifp->bgp_if_refcount++;
	    return;
	}
    }

    if (!bgp_iflist_blk) {
	bgp_iflist_blk = task_block_init(sizeof(bgp_ifap_list), "bgp_iflist");
    }
    ifp = (bgp_ifap_list *) task_block_alloc(bgp_iflist_blk);

    ifp->bgp_if_next = bgp->bgpg_ifap_list;
    bgp->bgpg_ifap_list = ifp;
    ifp->bgp_if_ifap = bnp->bgp_ifap;
    IFA_ALLOC(ifp->bgp_if_ifap);
    ifp->bgp_if_refcount = 1;
}


/*
 * bgp_iflist_free - free this peer's interface from the iflist.  Delete
 *		     the iflist if the refcount drops to zero.
 */
static void
bgp_iflist_free __PF1(bnp, bgpPeer *)
{
    register bgp_ifap_list *ifp, *ifp_prev;
    register bgpPeerGroup *bgp = bnp->bgp_group;

    switch(bgp->bgpg_type) {
    default:
	return;		/* no need to do this in here */
    case BGPG_INTERNAL:
    case BGPG_TEST:
	break;
    }

    if (!bnp->bgp_ifap) {
	/*
	 * Distant test peer, just return
	 */
	assert(bgp->bgpg_type == BGPG_TEST);
	return;
    }

    ifp_prev = (bgp_ifap_list *) 0;
    for (ifp = bgp->bgpg_ifap_list; ifp; ifp = ifp->bgp_if_next) {
	if (bnp->bgp_ifap == ifp->bgp_if_ifap) {
	    break;
	}
	ifp_prev = ifp;
    }

    assert(ifp);
    if ((ifp->bgp_if_refcount--) == 0) {
	if (ifp_prev) {
	    ifp_prev->bgp_if_next = ifp->bgp_if_next;
	} else {
	    bgp->bgpg_ifap_list = ifp->bgp_if_next;
	}
	if (bgp->bgpg_n_established) {
	    bgp_rt_if_terminate(bgp, ifp->bgp_if_ifap);
	}
	IFA_FREE(ifp->bgp_if_ifap);
	task_block_free(bgp_iflist_blk, (void_t) ifp);
    }
}


/*
 * bgp_check_ifa_policy - check to see if we should add or delete an
 *			  interface to/from a routing group's IFA list.
 */
static void
bgp_check_ifa_policy __PF2(bgp, bgpPeerGroup *,
			   ifap, if_addr *)
{
    register bgp_ifap_list *ifp, *ifp_prev;
    int try_up = 0;

    /*
     * Check to see what we need to do
     */
    switch (ifap->ifa_change) {
    case IFC_NOCHANGE:
    case IFC_ADD:
	if (!BIT_TEST(ifap->ifa_state, IFS_UP)) {
	    return;
	}
	try_up = 1;
	break;

    case IFC_DELETE:
    case IFC_DELETE|IFC_UPDOWN:
	break;

    default:
	if (!BIT_TEST(ifap->ifa_change, IFC_UPDOWN)) {
	    return;
	}

	if (BIT_TEST(ifap->ifa_state, IFS_UP)) {
	    try_up = 1;
	}
	break;
    }

    /*
     * See if we can find the interface in the list.
     */
    ifp_prev = (bgp_ifap_list *) 0;
    ifp = bgp->bgpg_ifap_list;
    while (ifp) {
	if (ifp->bgp_if_ifap == ifap) {
	    break;
	}
	ifp_prev = ifp;
	ifp = ifp->bgp_if_next;
    }

    /*
     * If interface is up, add it to the list if it isn't there already.
     * If interface is down, delete it from the list if it is there.
     */
    if (try_up) {
	if (!ifp && config_resolv_ifa(bgp->bgpg_ifap_policy, ifap, 0)) {
	    trace_tf(bgp->bgpg_trace_options,
		     TR_NORMAL,
		     0,
		     ("bgp_check_ifa_policy: adding %A(%s) to %s",
		      ifap->ifa_addr,
		      ifap->ifa_link->ifl_name,
		      bgp->bgpg_name));
	    if (!bgp_iflist_blk) {
		bgp_iflist_blk = task_block_init(sizeof(bgp_ifap_list),
							"bgp_iflist");
	    }
	    ifp = (bgp_ifap_list *) task_block_alloc(bgp_iflist_blk);
	    ifp->bgp_if_ifap = ifap;
	    IFA_ALLOC(ifap);
	    /* XXX metric!? */
	    if (ifp_prev) {
		ifp_prev->bgp_if_next = ifp;
	    } else {
		bgp->bgpg_ifap_list = ifp;
	    }
	}
    } else {
	if (ifp) {
	    trace_tf(bgp->bgpg_trace_options,
		     TR_NORMAL,
		     0,
		     ("bgp_check_ifa_policy: deleting %A(%s) from %s",
		      ifap->ifa_addr,
		      ifap->ifa_link->ifl_name,
		      bgp->bgpg_name));
	    IFA_FREE(ifap);
	    if (ifp_prev) {
		ifp_prev->bgp_if_next = ifp->bgp_if_next;
	    } else {
		bgp->bgpg_ifap_list = ifp->bgp_if_next;
	    }
	    task_block_free(bgp_iflist_blk, (void_t) ifp);
	}
    }
}


/*
 *	Write spool support
 */

/*
 * bgp_write_flush - flush as much data as possible from this peer's spool.
 *		     Return TRUE if this was error free (whether we actually
 *		     wrote anything or not), FALSE on hard errors.
 */
static int
bgp_write_flush __PF1(bnp, bgpPeer *)
{
    bgpOutBuffer *obp;
    int sent_len, rc = 0;
    size_t len;
    int times_around;
    int total_sent;

    /*
     * Fetch the spool header.  If there isn't one we're in trouble
     */
    obp = bnp->bgp_outbuf;
    assert(obp);

    /*
     * If there is something to do, compute the length of the first buffer.
     */
    len = obp->bgpob_end - obp->bgpob_start;
    assert(len != 0);		/* XXX */

    /*
     * Go around while we've still got stuff to write
     */
    times_around = 3;
    total_sent = 0;
    do {
	sent_len = write(bnp->bgp_task->task_socket,
			 (void_t) obp->bgpob_start,
			 len);
	if (sent_len == len) {
	    /*
	     * Complete buffer gone, dump it and return success
	     */
	    bgp_outbuf_free(bnp);
	    bnp->bgp_out_octets += (u_long) sent_len;
	    bnp->bgp_last_sent = bgp_time_sec;
	    return (TRUE);
	} else if (sent_len < 0) {
	    rc = errno;
	    switch(rc) {
	    case EHOSTUNREACH:
	    case ENETUNREACH:
		trace_tp(bnp->bgp_task,
			 TR_NORMAL|TR_STATE,
			 0,
			 ("bgp_write_flush: retrying write to %s: %m",
			  bnp->bgp_name));
		times_around--;
		break;

	    case EINTR:
		times_around--;
		break;

	    case EWOULDBLOCK:
#if	defined(EAGAIN) && EAGAIN != EWOULDBLOCK
	    case EAGAIN:		/* System V style */
#endif	/* EAGAIN */
		/*
		 * Would have blocked, we expect this in here.  Terminate
		 * the loop so we return.
		 */
		len = 0;
		break;

	    default:
		trace_log_tf(bnp->bgp_trace_options,
			     0,
			     LOG_ERR,
			     ("bgp_write_flush: sending %d (sent %d) bytes to %s failed: %m",
			      len,
			      total_sent,
			      bnp->bgp_name));
		errno = rc;
		return (FALSE);
	    }
	} else if (sent_len == 0) {
	    trace_log_tf(bnp->bgp_trace_options,
			 0,
			 LOG_ERR,
			 ("bgp_write_flush: sending %d (sent %d) bytes to %s: connection closed",
			  len,
			  total_sent,
			  bnp->bgp_name));
	    return (FALSE);
	} else {
	    /*
	     * Partial write, i.e. 0 < sent_len < len.  Advance the pointer
	     * in the buffer and try again.
	     */
	    len -= sent_len;
	    total_sent += sent_len;
	    obp->bgpob_start += sent_len;
	    bnp->bgp_out_octets += (u_long) sent_len;
	    bnp->bgp_last_sent = bgp_time_sec;
	    BIT_RESET(obp->bgpob_flags, BGPOBF_FULL_MESSAGE);
	    times_around = 3;
	}
    } while (len > 0 && times_around > 0);

    /*
     * If we are looping emit an error and return, otherwise give
     * them an informational message.
     */
    if (times_around == 0) {
	errno = rc;
	trace_log_tf(bnp->bgp_trace_options,
		     0,
		     LOG_ERR,
		     ("bgp_write_flush: sending to %s (sent %d, %d remain%s) looping: %m",
		      total_sent,
		      (obp->bgpob_end - obp->bgpob_start),
		      (((obp->bgpob_end - obp->bgpob_start) == 1) ? "s" : ""),
		      bnp->bgp_name));
	errno = rc;
	return (FALSE);
    }

    trace_tp(bnp->bgp_task,
	     TR_ALL,
	     0,
	     ("bgp_write_flush: sent %d byte%s to %s, %d byte%s still spooled",
	      total_sent,
	      ((total_sent == 1) ? "" : "s"),
	      bnp->bgp_name,
	      (obp->bgpob_end - obp->bgpob_start),
	      (((obp->bgpob_end - obp->bgpob_start) == 1) ? "s" : "")));
    return (TRUE);
}


/*
 * bgp_force_write - force a write on a peer's spooled data, return true
 *		     if it succeeds.  This is done when we're trying to
 *		     send a notification to a peer before closing it.
 */
int
bgp_force_write __PF1(bnp, bgpPeer *)
{
    bgpOutBuffer *obp;

    obp = bnp->bgp_outbuf;
    if (obp == (bgpOutBuffer *) 0) {
	return (TRUE);
    }

    if (BIT_TEST(obp->bgpob_flags, BGPOBF_FULL_MESSAGE)) {
	/*
	 * Full message buffered, just dump it.
	 */
	bgp_outbuf_free(bnp);
	return (TRUE);
    }

    /*
     * Force a write.  Return FALSE if we don't flush it all.
     */
    if (!bgp_write_flush(bnp) || bnp->bgp_outbuf) {
	return (FALSE);
    }
    return (TRUE);
}


/*
 * bgp_write_ready - ready to write for a peer.  Flush out spooled data
 *		     then inform the routing code that we're ready to send.
 */
static void
bgp_write_ready __PF1(tp, task *)
{
    bgpPeer *bnp = (bgpPeer *)tp->task_data;

    trace_tp(tp,
	     TR_NORMAL,
	     0,
	     ("bgp_write_ready: write ready for peer %s",
	      bnp->bgp_name));

    if (bnp->bgp_outbuf) {
	if (!bgp_write_flush(bnp)) {
	    /*
	     * Failed.  Dump him.
	     */
	    bgp_peer_close(bnp, BGPEVENT_ERROR);
	    return;
	}
	if (bnp->bgp_outbuf) {
	    /*
	     * Didn't get it all out.  Try again later.
	     */
	    return;
	}
    }

    /*
     * So far so good.  Inform the route code that this peer is
     * ready to send.
     */
    bgp_rt_send_ready(bnp);

    /*
     * If we have nothing spooled, cancel the write routine.  Otherwise
     * we'll do this again (unless we've been closed already due to errors).
     */
    if (bnp->bgp_state == BGPSTATE_ESTABLISHED
      && bnp->bgp_outbuf == (bgpOutBuffer *) 0) {
	if (BIT_TEST(bnp->bgp_flags, BGPF_INITIALIZING)) {
	    int res = bgp_send_keepalive(bnp, 1);

	    if (res < 0) {
		bgp_peer_close(bnp, BGPEVENT_ERROR);
		return;
	    }
	    BIT_RESET(bnp->bgp_flags, BGPF_INITIALIZING);
	    bnp->bgp_last_keepalive = bgp_time_sec;
	    if (res == 0) {
		return;
	    }
	}
	bgp_send_reset(bnp);
	bgp_rt_sync(bnp);
    }
}

/*
 * bgp_write_message - buffer outgoing data on a peer and set the send
 *		       routine (if not set already).
 */
void
bgp_write_message __PF4(bnp, bgpPeer *,
			message, byte *,
			mlen, size_t,
			full, int)
{
    assert(bnp->bgp_outbuf == (bgpOutBuffer *) 0 && mlen > 0);

    trace_tp(bnp->bgp_task,
	     TR_NORMAL,
	     0,
	     ("bgp_write_message: buffering a %s message (%d byte%s) for peer %s",
	      (full ? "full" : "partial"),
	      mlen,
	      ((mlen == 1) ? "" : "s"),
	      bnp->bgp_name));

    /*
     * Allocate the buffer.  If this is a full message, note it.
     */
    bgp_outbuf_alloc(bnp, message, mlen);
    if (full) {
	BIT_SET(bnp->bgp_outbuf->bgpob_flags, BGPOBF_FULL_MESSAGE);
    }

    /*
     * If the write routine isn't running already, start it.
     */
    if (!BIT_TEST(bnp->bgp_flags, BGPF_SEND_RTN)) {
	bgp_send_set(bnp, bgp_write_ready);
	bgp_rt_unsync(bnp);
    }
}


/*
 * bgp_set_write - set up the write routine for a peer, whether data
 *		   is buffered or not.
 */
void
bgp_set_write __PF1(bnp, bgpPeer *)
{
    trace_tp(bnp->bgp_task,
	     TR_NORMAL,
	     0,
	     ("bgp_set_write: initiating write routine for peer %s",
	      bnp->bgp_name));
    
    if (!BIT_TEST(bnp->bgp_flags, BGPF_SEND_RTN)) {
	bgp_send_set(bnp, bgp_write_ready);
	bgp_rt_unsync(bnp);
    }
}




/*
 * bgp_set_connect_timer - create, if necessary, and start connect timer
 */
static void
bgp_set_connect_timer __PF2(bnp, bgpPeer *,
			    timeo, time_t)
{
    register int slot;
    register time_t add_interval;

    if (bnp->bgp_connect_timer) {
	BGPCONN_DONE(bnp);
	BIT_RESET(bnp->bgp_flags, BGPF_TRY_CONNECT);
    }

    slot = BGPCONN_SLOT(timeo);
    if (bgp_connect_slots[slot] == 0) {
	/*
	 * Common case, I think.  His interval is okay, just bump
	 * the count and save the slot number.
	 */
	add_interval = 0;
    } else {
	register int i, s;
	register time_t intr;

	/*
	 * Slot not zero.  Go searching through looking for a better one.
	 */
	s = slot;
	add_interval = intr = 0;
	for (i = BGPCONN_SLOTDELAY; i > 0; i--) {
	    s = BGPCONN_NEXTSLOT(s);
	    intr += BGPCONN_SLOTSIZE;
	    if (bgp_connect_slots[s] < bgp_connect_slots[slot]) {
		slot = s;
		add_interval = intr;
		if (bgp_connect_slots[s] == 0) {
		    break;
		}
	    }
	}
    }

    /*
     * slot is now the best of them.  Save this and use it
     */
    bgp_connect_slots[slot]++;
    bnp->bgp_connect_slot = slot + 1;
    if (!(bnp->bgp_connect_timer)) {
        bnp->bgp_connect_timer =  task_timer_create(bnp->bgp_task,
						    "Connect",
						    (flag_t) 0,
						    (time_t) 0,
						    (timeo + add_interval),
						    bgp_connect_timeout,
						    (void_t) bnp);
    } else {
	task_timer_set(bnp->bgp_connect_timer,
		       (time_t) 0,
		       (timeo + add_interval));
    }
}


/*
 * bgp_reset_connect_timer - reset the connect timer but don't delete it
 */
static void
bgp_reset_connect_timer __PF1(bnp, bgpPeer *)
{
    if (bnp->bgp_connect_timer) {
	BGPCONN_DONE(bnp);
	BIT_RESET(bnp->bgp_flags, BGPF_TRY_CONNECT);
	task_timer_reset(bnp->bgp_connect_timer);
    }
}


/*
 * bgp_delete_connect_timer - delete the connect timer
 */
static void
bgp_delete_connect_timer __PF1(bnp, bgpPeer *)
{
    if (bnp->bgp_connect_timer) {
	BIT_RESET(bnp->bgp_flags, BGPF_TRY_CONNECT);
	BGPCONN_DONE(bnp);
	task_timer_delete(bnp->bgp_connect_timer);
	bnp->bgp_connect_timer = (task_timer *) 0;
    }
}


/*
 * bgp_peer_connected - a connection to a peer has opened, send an
 *   open message and prepare to receive the response.
 */
static void
bgp_peer_connected __PF1(bnp, bgpPeer *)
{
    sockaddr_un *addr;
    task *tp = bnp->bgp_task;

    trace_tp(bnp->bgp_task,
	     TR_NORMAL,
	     0,
	     ("bgp_peer_connected: connection established with %s",
	      bnp->bgp_name));

    addr = task_get_addr_local(tp);
    if (addr == NULL) {
	int rc = errno;
	trace_log_tf(bnp->bgp_trace_options,
		     0,
		     LOG_ERR,
		     ("bgp_peer_connected: task_get_addr_local(%s): %m",
		      bnp->bgp_name));
	task_quit(rc);
    }

    if (bnp->bgp_myaddr == NULL) {
	bnp->bgp_myaddr = sockdup(addr);
    } else {
	sock2port(bnp->bgp_myaddr) = sock2port(addr);
    }

    bgp_delete_connect_timer(bnp);
    bgp_recv_setup(bnp, tp->task_socket, bgp_recv_open, "bgp_recv_open");
    bgp_buffer_alloc(&(bnp->bgp_inbuf));
    bgp_event(bnp, BGPEVENT_OPEN, BGPSTATE_OPENSENT);
    if (!bgp_send_open(bnp, BGP_VERSION_UNSPEC)) {
	bgp_peer_close(bnp, BGPEVENT_ERROR);
    } else {
        bgp_init_traffic_timer(bnp);
    }
}

/*
 * bgp_connect_complete - connect has completed, ready socket to recv
 */
static void
bgp_connect_complete __PF1(tp, task *)
{
    sockaddr_un *addr;
    bgpPeer *bnp = (bgpPeer *) tp->task_data;

    /*
     * Bump the connect failed counter here (it is somewhat misnamed
     * since it is bumped even if the connection fails).  It will be
     * cleared if the peer manages to become established.
     */
    bnp->bgp_connect_failed++;

    addr = task_get_addr_remote(tp);
    if (addr == NULL) {
	/*
	 * Nothing to do here except close socket and wait for
	 * new restart of the connection
	 */
	trace_tp(tp,
		 TR_ALL,
		 0,
		 ("bgp_connect_complete: connection error: %m"));
	bgp_close_socket(bnp);
	bgp_event(bnp, BGPEVENT_OPENFAIL, BGPSTATE_IDLE);
	/*
	 * See if we should do an immediate reconnect.  Otherwise,
	 * timer will start us.
	 */
	if (BIT_TEST(bnp->bgp_flags, BGPF_TRY_CONNECT)) {
	    BIT_RESET(bnp->bgp_flags, BGPF_TRY_CONNECT);
            bgp_event(bnp, BGPEVENT_START, BGPSTATE_CONNECT);
	    bgp_connect_start(bnp);
	} else {
            bgp_event(bnp, BGPEVENT_START, BGPSTATE_ACTIVE);
	}
    } else {
	bgp_peer_connected(bnp);
    }
}


#ifdef	SCO_UW21
/* re-call connect() to establish the connection on a non-blocking socket
 */
static void
bgp_do_connect __PF1(tp, task *)
{
    if (task_connect(tp)) {

	bgpPeer *bnp = (bgpPeer *) tp->task_data;

	switch (errno) {
	case EINPROGRESS:	/* FALLTHRU */
	case EALREADY:		/* better luck next time */
		break ;
	case EISCONN:
		bgp_connect_complete(tp);
		break ;
	default:
		trace_log_tf(bnp->bgp_trace_options,
			0,
			LOG_WARNING,
			("bgp_do_connect: connect %s: %m",
			bnp->bgp_name));
		bnp->bgp_connect_failed++;
		bgp_peer_close(bnp, BGPEVENT_OPENFAIL);
		break ;
	}
    }
}
#endif	/* SCO_UW21 */

/*
 * bgp_connect_start - initiate a connection to the neighbour
 */
static void
bgp_connect_start(bnp)
bgpPeer *bnp;
{
    task *tp = bnp->bgp_task;
    int bgp_socket;

    trace_tp(bnp->bgp_task,
	     TR_NORMAL,
	     0,
	     ("bgp_connect_start: peer %s",
	      bnp->bgp_name));

    bgp_event(bnp, BGPEVENT_CONNRETRY, BGPSTATE_CONNECT);

    if ((bgp_socket = task_get_socket(tp, AF_INET, SOCK_STREAM, 0)) < 0) {
	task_quit(errno);
    }
    BIT_SET(tp->task_flags, TASKF_CONNECT);
#ifdef	SCO_UW21
    task_set_connect(tp, bgp_do_connect);
#else	/* SCO_UW21 */
    task_set_connect(tp, bgp_connect_complete);
#endif	/* SCO_UW21 */
    task_set_socket(tp, bgp_socket);

    if (task_set_option(tp, TASKOPTION_NONBLOCKING, TRUE) < 0) {
	task_quit(errno);
    }
    if (task_set_option(tp, TASKOPTION_REUSEADDR, TRUE) < 0) {
	task_quit(errno);
    }

    /*
     * Here's how we do it.  If there is a local address specified,
     * copy this to bnp->bgp_myaddr (assumes the port is zero'd) and bind the
     * local end to this.  On the peer address set the port to bgp_port.
     */
    assert(bnp->bgp_myaddr == NULL);
    if (BGP_OPTIONAL_SHAREDIF(bnp->bgp_group)) {
	(void) bgp_set_peer_if(bnp, 0);
    }
    if (bnp->bgp_ifap != NULL || bnp->bgp_lcladdr != NULL) {
    	if (bnp->bgp_lcladdr != NULL) {
	    bnp->bgp_myaddr = sockdup(bnp->bgp_lcladdr->ifae_addr);
    	} else {
	    bnp->bgp_myaddr = sockdup(bnp->bgp_ifap->ifa_addr_local);
	}
	if (task_addr_local(tp, bnp->bgp_myaddr)) {
	    if (errno == EADDRNOTAVAIL) {
		trace_log_tf(bnp->bgp_trace_options,
			     0,
			     LOG_WARNING,
			     ("bgp_connect_start: peer %s local address %A unavailable, connection failed",
			      bnp->bgp_name,
			      bnp->bgp_myaddr));
		bnp->bgp_connect_failed++;
		bgp_peer_close(bnp, BGPEVENT_OPENFAIL);
		return;
	    }
	    task_quit(errno);
	}
    }

    /* XXX Grot, fix */
    sock2port(bnp->bgp_addr) = bgp_port;
    sock2port(tp->task_addr) = bgp_port;
    if (task_connect(tp)) {
	if (errno == EINPROGRESS) {
	    if (bnp->bgp_myaddr == NULL) {
		sockaddr_un *addr;
		addr = task_get_addr_local(tp);
		if (addr == NULL) {
		    trace_log_tf(bnp->bgp_trace_options,
				 0,
				 LOG_ERR,
				 ("bgp_peer_connected: task_get_addr_local(%s): %m",
				  bnp->bgp_name));
		    bnp->bgp_connect_failed++;
		    bgp_peer_close(bnp, BGPEVENT_OPENFAIL);
		    return;
		}
		bnp->bgp_myaddr = sockdup(addr);
	    }
            bgp_set_connect_timer(bnp, BGPCONN_INTERVAL(bnp));
	} else {
	    trace_log_tf(bnp->bgp_trace_options,
			 0,
			 LOG_WARNING,
			 ("bgp_connect_start: connect %s: %m",
			  bnp->bgp_name));
	      bnp->bgp_connect_failed++;
	      bgp_peer_close(bnp, BGPEVENT_OPENFAIL);
	}
    } else {
        bnp->bgp_connect_failed++;
	bgp_peer_connected(bnp);
    }
}


/*
 * bgp_connect_timeout - called when connect timer expires, attempt a connect
 */
static void
bgp_connect_timeout(tip, interval)
task_timer *tip;
time_t interval;
{
    task *tp = tip->task_timer_task;
    bgpPeer *bnp = (bgpPeer *) tp->task_data;

    trace_tp(tp,
	     TR_NORMAL,
	     0,
	     ("bgp_connect_timeout: %s",
	      task_timer_name(tip)));

    if (bnp->bgp_connect_slot != 0) {
	bgp_connect_slots[bnp->bgp_connect_slot - 1]--;
        bnp->bgp_connect_slot = 0;
    }

    /*
     * If we're still waiting for connection completion, flag for
     * restart when the connection times out.  Otherwise, do it now.
     */
    if (bnp->bgp_state == BGPSTATE_CONNECT) {
	bgp_reset_connect_timer(bnp);
	BIT_SET(bnp->bgp_flags, BGPF_TRY_CONNECT);
    } else {
        bgp_connect_start(bnp);
    }
}


/*
 * bgp_set_traffic_timer - set the traffic timer
 */
static void
bgp_set_traffic_timer __PF2(bnp, bgpPeer *,
			    timeo, time_t)
{
    if (!(bnp->bgp_traffic_timer)) {
        bnp->bgp_traffic_timer = task_timer_create(bnp->bgp_task,
						   "Traffic",
						   (flag_t) 0,
						   (time_t) 0,
						   timeo,
						   bgp_traffic_timeout,
						   (void_t) bnp);
	bnp->bgp_last_rcvd = bnp->bgp_last_sent
	  = bnp->bgp_last_checked = bgp_time_sec;
    } else {
	task_timer_set(bnp->bgp_traffic_timer,
		       (time_t) 0,
		       timeo);
    }
    bnp->bgp_traffic_interval = timeo;
}


/*
 * bgp_delete_traffic_timer - delete the traffic timer
 */
static void
bgp_delete_traffic_timer __PF1(bnp, bgpPeer *)
{
    if (bnp->bgp_traffic_timer) {
	task_timer_delete(bnp->bgp_traffic_timer);
	bnp->bgp_traffic_timer = (task_timer *) 0;
	bnp->bgp_last_rcvd = bnp->bgp_last_sent = bnp->bgp_last_checked = 0;
    }
}


/*
 * bgp_init_traffic_timer - (re)start the traffic timer for a peer
 */
static void
bgp_init_traffic_timer(bnp)
bgpPeer *bnp;
{
    /*
     * If we are not established, just set the timer for our
     * current holdtime.  This will provide a time out for open
     * negotiation.  If we are established, however, set the timer
     * for the negotiated holdtime/3. In either case set the times
     * to the current time.
     */
    bnp->bgp_last_rcvd = bnp->bgp_last_sent
      = bnp->bgp_last_checked = bgp_time_sec;
    if (bnp->bgp_state != BGPSTATE_ESTABLISHED) {
	if (bnp->bgp_holdtime_out) {
	bgp_set_traffic_timer(bnp, bnp->bgp_holdtime_out);
	}
    } else {
	if (bnp->bgp_holdtime) {
	    bgp_set_traffic_timer(bnp, (bnp->bgp_holdtime / 3));
	}
    }
}



/*
 * bgp_traffic_timeout - set the traffic timeout
 */
static void
bgp_traffic_timeout(tip, interval)
task_timer *tip;
time_t interval;
{
    bgpPeer *bnp = (bgpPeer *) tip->task_timer_data;
    time_t next_rcvd_time, next_sent_time;
    time_t next_interval;

    trace_tf(bnp->bgp_trace_options,
	     TR_NORMAL,
	     0,
	     ("bgp_traffic_timeout: peer %s last checked %d last recv'd %d last sent %d last keepalive %d",
	      bnp->bgp_name,
	      bgp_time_sec - bnp->bgp_last_checked,
	      bgp_time_sec - bnp->bgp_last_rcvd,
	      bgp_time_sec - bnp->bgp_last_sent,
	      bgp_time_sec - bnp->bgp_last_keepalive));

    /*
     * If we're past the hold time, inform someone and return.
     * Otherwise, if we're past the keepalive time, send a keepalive.
     */
    bnp->bgp_last_checked = bgp_time_sec;

    /*
     * If we aren't in establshed, or if he exceeded the holdtime,
     * terminate him.
     */
    next_rcvd_time = bnp->bgp_last_rcvd + bnp->bgp_holdtime;
    if (bnp->bgp_state != BGPSTATE_ESTABLISHED
      || next_rcvd_time <= bgp_time_sec) {
	trace_log_tf(bnp->bgp_trace_options,
		     0,
		     LOG_ERR,
		     ("bgp_traffic_timeout: holdtime expired for %s",
		      bnp->bgp_name));
	bgp_send_notify_none(bnp, BGP_ERR_HOLDTIME, 0);
	bgp_peer_close(bnp, BGPEVENT_HOLDTIME);
	return;
    }

    if (BIT_TEST(bnp->bgp_options, BGPO_KEEPALIVESALWAYS)) {
	next_sent_time = bnp->bgp_last_keepalive + bnp->bgp_holdtime/3;
    } else {
	next_sent_time = bnp->bgp_last_sent + bnp->bgp_holdtime/3;
    }
    if (next_sent_time <= bgp_time_sec) {
	/* Only send one when established and unblocked */
	if (bnp->bgp_state == BGPSTATE_ESTABLISHED
	  && !BIT_TEST(bnp->bgp_flags, BGPF_INITIALIZING|BGPF_SEND_RTN)
	  && bnp->bgp_outbuf == (bgpOutBuffer *) 0) {
	    int res = bgp_send_keepalive(bnp, 0);

	    if (res < 0) {
		trace_log_tf(bnp->bgp_trace_options,
			     0,
			     LOG_ERR,
			     ("bgp_traffic_timeout: error sending KEEPALIVE to %s: %m",
			      bnp->bgp_name));
		bgp_peer_close(bnp, BGPEVENT_ERROR);
		return;
	    }
 	    bnp->bgp_last_keepalive = bgp_time_sec;
	}
	bnp->bgp_last_sent = bgp_time_sec;
 	if (BIT_TEST(bnp->bgp_options, BGPO_KEEPALIVESALWAYS)
	    && bnp->bgp_last_keepalive != bgp_time_sec) {
 	    next_sent_time = bgp_time_sec + (time_t) 2;	/* XXX */
 	} else {
	    next_sent_time = bgp_time_sec + bnp->bgp_holdtime/3;
	}
    }

    /*
     * Compute the next interval to use.
     */
    if (next_rcvd_time < next_sent_time) {
	next_interval = next_rcvd_time - bgp_time_sec;
    } else {
	next_interval = next_sent_time - bgp_time_sec;
    }
    task_timer_set(tip,
		   (time_t) 0,
		   next_interval);
}


/*
 * bgp_route_timeout - process a route timer timeout.  Call the
 *		       routing code to flush out anything ready
 *		       to go.
 */
static void
bgp_route_timeout __PF2(tip, task_timer *,
		 	interval, time_t)
{
    bgpPeer *bnp = (bgpPeer *)tip->task_timer_data;

    assert(bnp && bnp->bgp_route_timer == tip);

    /*
     * First reset the timer flag.  Since we're running as a one-shot
     * timer we'll go idle if a new interval isn't set.
     */
    BIT_RESET(bnp->bgp_flags, BGPF_RT_TIMER);

    /*
     * If there is something delayed, inform the routing code so we
     * can flush it out.  This is all we need to do, if we'll need
     * to run again bgp_route_timer_set() will be called to reset
     * the timeout, otherwise we'll go idle.
     */
    if (bnp->bgp_rto_next_time) {
	bgp_rt_peer_timer(bnp);
    }
}


/*
 * bgp_route_timer_set - set the peer route timer
 */
void
bgp_route_timer_set __PF1(bnp, bgpPeer *)
{
    time_t timeo;

    assert(bnp->bgp_rto_next_time);
    assert(bnp->bgp_rto_next_time <= bgp_time_sec);

    timeo = bgp_time_sec - bnp->bgp_rto_next_time;
    if (timeo < bnp->bgp_rto_time) {
	timeo = bnp->bgp_rto_time - timeo;
    } else {
	timeo = 1;		/* XXX what else to do? */
    }

    if (!(bnp->bgp_route_timer)) {
	bnp->bgp_route_timer = task_timer_create(bnp->bgp_task,
						 "Route",
						 (flag_t) 0,
						 (time_t) 0,
						 timeo,
						 bgp_route_timeout,
						 (void_t) bnp);
    } else {
	task_timer_set(bnp->bgp_route_timer,
		       (time_t) 0,
		       timeo);
    }
    BIT_SET(bnp->bgp_flags, BGPF_RT_TIMER);
}


/*
 * bgp_route_timer_delete - delete the route timer, we
 */
static void
bgp_route_timer_delete __PF1(bnp, bgpPeer *)
{
    if (bnp->bgp_route_timer) {
	BIT_RESET(bnp->bgp_flags, BGPF_RT_TIMER);
	task_timer_delete(bnp->bgp_route_timer);
	bnp->bgp_route_timer = (task_timer *) 0;
    }
}


/*
 * bgp_group_route_timeout - process a route timer timeout for a group.
 *			     Call the routing code to flush out anything ready
 *			     to go.
 */
static void
bgp_group_route_timeout __PF2(tip, task_timer *,
			      interval, time_t)
{
    bgpPeerGroup *bgp = (bgpPeerGroup *)tip->task_timer_data;

    assert(bgp && bgp->bgpg_route_timer == tip);

    /*
     * First reset the timer flag.  Since we're running as a one-shot
     * timer we'll go idle if a new interval isn't set.
     */
    BIT_RESET(bgp->bgpg_flags, BGPGF_RT_TIMER);

    /*
     * If there is something delayed, inform the routing code so we
     * can flush it out.  This is all we need to do, if we'll need
     * to run again bgp_group_route_timer_set() will be called to reset
     * the timeout, otherwise we'll go idle.
     */
    if (bgp->bgpg_rto_next_time) {
	bgp_rt_group_timer(bgp);
    }
}


/*
 * bgp_group_route_timer_set - set the group route timer
 */
void
bgp_group_route_timer_set __PF1(bgp, bgpPeerGroup *)
{
    time_t timeo;

    assert(bgp->bgpg_rto_next_time);
    assert(bgp->bgpg_rto_next_time <= bgp_time_sec);

    timeo = bgp_time_sec - bgp->bgpg_rto_next_time;
    if (timeo < bgp->bgpg_rto_time) {
	timeo = bgp->bgpg_rto_time - timeo;
    } else {
	timeo = 1;		/* XXX what else to do? */
    }

    if (!(bgp->bgpg_route_timer)) {
	bgp->bgpg_route_timer = task_timer_create(bgp->bgpg_task,
						  "Route",
						  (flag_t) 0,
						  (time_t) 0,
						  timeo,
						  bgp_group_route_timeout,
						  (void_t) bgp);
    } else {
	task_timer_set(bgp->bgpg_route_timer,
		       (time_t) 0,
		       timeo);
    }
    BIT_SET(bgp->bgpg_flags, BGPGF_RT_TIMER);
}


/*
 * bgp_group_route_timer_delete - delete the route timer, we
 */
static void
bgp_group_route_timer_delete __PF1(bgp, bgpPeerGroup *)
{
    if (bgp->bgpg_route_timer) {
	BIT_RESET(bgp->bgpg_flags, BGPGF_RT_TIMER);
	task_timer_delete(bgp->bgpg_route_timer);
	bgp->bgpg_route_timer = (task_timer *) 0;
    }
}


/*
 * bgp_listen_accept - process an incoming connection
 */
static void
bgp_listen_accept __PF1(listen_tp, task *)
{
    task *tp;
    bgpProtoPeer *ppp;
    struct sockaddr_in inaddr;
    size_t inaddrlen;
    int s;
    sockaddr_un *myaddr;

    /*
     * Accept the connection
     */
    inaddrlen = sizeof(inaddr);
    if ((s = accept(listen_tp->task_socket,
		    (struct sockaddr *) &inaddr,
		    (size_t *) &inaddrlen)) < 0) {
	trace_log_tp(listen_tp,
		     0,
		     LOG_ERR,
		     ("bgp_listen_accept: accept(%d): %m",
		      listen_tp->task_socket));
	return;
    }

    /*
     * Now allocate a task to go with this.  Stick the address/port
     * in there, along with the socket number.
     */
    tp = task_alloc("BGP_Proto",
		    TASKPRI_EXTPROTO,
		    bgp_default_trace_options);
    BIT_SET(tp->task_flags, TASKF_LOWPRIO);
    tp->task_socket = s;
    tp->task_addr = sockdup(sock2gated((struct sockaddr *) &inaddr, inaddrlen));

    /*
     * Determine the local address if we don't know it already
     */
    myaddr = task_get_addr_local(tp);
    if (myaddr == NULL) {
	trace_log_tp(listen_tp,
		     0,
		     LOG_ERR,
		     ("bgp_listen_accept: task_get_addr_local() failed, terminating!!"));
	task_quit(EINVAL);
    }
    myaddr = sockdup(myaddr);


    trace_tp(tp,
	     TR_NORMAL,
	     0,
	     ("bgp_listen_accept: accepting connection from %#A (local %#A)",
	      tp->task_addr,
	      myaddr));

    /*
     * Initialize the task structure
     */
    tp->task_rtproto = RTPROTO_BGP;
    task_set_recv(tp, bgp_pp_recv);
    task_set_dump(tp, bgp_pp_dump);
    task_set_terminate(tp, bgp_fake_terminate);

    if (!task_create(tp)) {
	task_quit(EINVAL);
    }

    /*
     * Set options for receiving
     */
    bgp_recv_setopts(tp, BGP_RECV_BUFSIZE, BGP_SEND_BUFSIZE);

    /*
     * Build a proto-peer structure to go along with this
     */
    ppp = bgp_pp_create(tp, myaddr);

    /*
     * Set a timeout for the receive using the traffic timer
     */
    ppp->bgpp_timeout_timer = task_timer_create(tp,
						"OpenTimeOut",
						(flag_t) 0,
						(time_t) BGP_OPEN_TIMEOUT,
						(time_t) 0,
						bgp_pp_timeout,
						(void_t) ppp);
}


/*
 * bgp_listen_start - delete the startup timer and start listening on
 *   the BGP socket
 */
static void
bgp_listen_start __PF2(tip, task_timer *,
		       interval, time_t)
{
    task *tp = tip->task_timer_task;
    int s;

    /* Must be listen task */
    assert(tp == bgp_listen_task);
    bgp_listen_timer = (task_timer *) 0;

    if ((s = task_get_socket(tp, AF_INET, SOCK_STREAM, 0)) < 0) {
	s = errno;
	trace_log_tp(tp,
		     0,
		     LOG_ERR,
		     ("bgp_listen_start: couldn't get BGP listen socket!!"));
	task_quit(s);
    }

    tp->task_flags |= TASKF_ACCEPT;
    tp->task_addr = sockdup(inet_addr_any);
    sock2port(tp->task_addr) = bgp_port;
    task_set_accept(tp, bgp_listen_accept);
    task_set_socket(tp, s);

    if (task_set_option(tp, TASKOPTION_REUSEADDR, TRUE) < 0) {
	task_quit(errno);
    }
    if (task_addr_local(tp, tp->task_addr)) {
	task_quit(errno);
    }

    if (listen(s, 5) < 0) {	/* XXX - need a task routine to listen */
	trace_log_tp(tp,
		     0,
		     LOG_ERR,
		     ("bgp_listen_start: listen: %m"));
	task_quit(errno);
    }
}


/*
 * bgp_listen_init - create a task for listening, and set a timer so we'll
 *   actually start doing this a little while into the future.
 */
static void
bgp_listen_init __PF0(void)
{
    if (!bgp_listen_task) {
	bgp_listen_task = task_alloc("BGP",
				     TASKPRI_PROTO,
				     bgp_default_trace_options);
	BIT_SET(bgp_listen_task->task_flags, TASKF_LOWPRIO);
	bgp_listen_task->task_rtproto = RTPROTO_BGP;
	task_set_cleanup(bgp_listen_task, bgp_cleanup);
	task_set_dump(bgp_listen_task, bgp_dump);
	task_set_terminate(bgp_listen_task, bgp_terminate);
	task_set_ifachange(bgp_listen_task, bgp_ifachange);

	if (!task_create(bgp_listen_task)) {
	    task_quit(EINVAL);
	}
    }

    if (!bgp_listen_timer) {
	bgp_listen_timer = task_timer_create(bgp_listen_task,
					     "Listen_Init",
					     TIMERF_DELETE,
					     (time_t) 0,
					     (time_t) BGP_LISTEN_TIMEOUT,
					     bgp_listen_start,
					     (void_t) 0);
    }
}


/*
 * bgp_listen_stop - stop listening for connections by deleting the listen task
 */
static void
bgp_listen_stop __PF0(void)
{
    task_delete(bgp_listen_task);
    bgp_listen_task = (task *) 0;
    bgp_listen_timer = (task_timer *) 0;
}


/*
 * bgp_set_peer_if - for a peer which can use an interface on a shared
 *   subnet, find the shared interface.  Return the interface pointer
 *   if one is found, NULL otherwise.  Complain if the interface isn't
 *   there and he told you to be verbose.
 */
static if_addr *
bgp_set_peer_if(bnp, logit)
bgpPeer *bnp;
int logit;
{
    if_addr *ifap;

    if (BIT_TEST(bnp->bgp_options, BGPO_GATEWAY)) {
	ifap = if_withdst(bnp->bgp_gateway);
    } else {
	ifap = if_withdst(bnp->bgp_addr);
    }

    if (ifap == NULL) {
	if (logit) {
	    if (BIT_TEST(bnp->bgp_options, BGPO_GATEWAY)) {
		trace_log_tf(bnp->bgp_trace_options,
			     0,
			     LOG_WARNING,
			     ("bgp_set_peer_if: BGP peer %s interface for gateway %A not found.  Leaving peer idled",
			      bnp->bgp_name,
			      bnp->bgp_gateway));
	    } else {
		trace_log_tf(bnp->bgp_trace_options,
			     0,
			     LOG_WARNING,
			     ("bgp_set_peer_if: BGP peer %s interface not found.  Leaving peer idled",
			      bnp->bgp_name));
	    }
	}
	return ((if_addr *)0);
    }

    if (bnp->bgp_lcladdr!=NULL 
	&& !BIT_TEST(bnp->bgp_options, BGPO_GATEWAY)
	&& ifap->ifa_addrent_local!=bnp->bgp_lcladdr) {
	if (logit) {
	    trace_log_tf(bnp->bgp_trace_options,
			 0,
			 LOG_WARNING,
			 ("bgp_set_peer_if: BGP peer %s local address %A not on shared net.  Leaving peer idled",
			  bnp->bgp_name,
			  bnp->bgp_lcladdr->ifae_addr));
	}
	return ((if_addr *)0);
    }

    IFA_ALLOC(bnp->bgp_ifap = ifap);
    return (ifap);
}


/*
 *	Group and peer location routines
 */

/*
 * bgp_find_group - find a bgp group that matches the peer's address,
 *		    as and authentication.
 */
bgpPeerGroup *
bgp_find_group __PF7(addr, sockaddr_un *,
		     lcladdr, sockaddr_un *,
		     as, as_t,
		     myas, as_t,
		     authcode, int,
		     authdata, byte *,
		     authdatalen, int)
{
    register bgpPeerGroup *bgp;
    register bgpPeer *bnp;
    bgpPeerGroup *okgroup = (bgpPeerGroup *)NULL;
    if_addr *ifap;

    /*
     * Determine the shared interface if there is one
     */
    ifap = if_withdst(addr);
    if (ifap != NULL && !sockaddrcmp_in(ifap->ifa_addr_local, lcladdr)) {
	ifap = NULL;
    }

    /*
     * We look through the list of bgpPeerGroups looking for something
     * which matches the address, incoming AS and authentication.  We
     * save the first one we match.  As there could be more than one
     * matching group, however, we search the entire table for a matching
     * peer as well.  If the matching peer is in another group we return
     * that group instead.
     *
     * This is a crock of shit, actually.  I should think harder about
     * what it is exactly we are trying to match.
     */
    BGP_GROUP_LIST(bgp) {
	if (as != bgp->bgpg_peer_as) {
	    continue;
	}
	if (myas != 0 && myas != bgp->bgpg_local_as) {
	    continue;
	}
	if (BIT_TEST(bgp->bgpg_flags, BGPGF_IDLED)) {
	    continue;
	}
	if (okgroup == NULL
	  && bgp->bgpg_allow != NULL
	  && adv_destmask_match(bgp->bgpg_allow,
				addr,
				inet_mask_host)) {
	    int groupisok = 1;

	    if (BGP_NEEDS_SHAREDIF(bgp)) {
		if (bgp->bgpg_gateway != NULL) {
		    if_addr *ifap2;

		    ifap2 = if_withdst(bgp->bgpg_gateway);
		    if (!sockaddrcmp_in(ifap2->ifa_addr_local, lcladdr)) {
			groupisok = 0;
		    }
		} else if (ifap == NULL) {
		    groupisok = 0;
		}
	    }
	    if (groupisok
		&& bgp->bgpg_lcladdr != NULL
		&& !sockaddrcmp_in(bgp->bgpg_lcladdr->ifae_addr, lcladdr)) {
		groupisok = 0;
	    }
	    if (groupisok && !bgp_open_auth(NULL, &bgp->bgpg_authinfo,
	      authcode, authdata, authdatalen)) {
		groupisok = 0;
	    }
	    if (groupisok) {
		okgroup = bgp;
	    }
	}

	BGP_PEER_LIST(bgp, bnp) {
	    if (sockaddrcmp_in(addr, bnp->bgp_addr)) {
		break;
	    }
	} BGP_PEER_LIST_END(bgp, bnp);

	if (bnp != NULL) {
	    int peerstinks = 0;

	    if (BIT_TEST(bnp->bgp_flags, BGPF_IDLED|BGPF_UNCONFIGURED)) {
		peerstinks = 1;
	    } else if (BGP_NEEDS_SHAREDIF(bgp)) {
		if (bnp->bgp_gateway != NULL) {
		    if (!sockaddrcmp_in(bnp->bgp_ifap->ifa_addr_local,
					lcladdr)) {
			peerstinks = 1;
		    }
		} else if (bnp->bgp_ifap != ifap) {
		    peerstinks = 1;
		}
	    } else if (bnp->bgp_lcladdr != NULL) {
		if (!sockaddrcmp_in(bnp->bgp_lcladdr->ifae_addr, lcladdr)) {
		    peerstinks = 1;
		}
	    }

	    if (!peerstinks && !bgp_open_auth(NULL, &bnp->bgp_authinfo,
	      authcode, authdata, authdatalen)) {
		peerstinks = 1;
	    }

	    if (!peerstinks) {
		/* good one, return group */
		return (bgp);
	    }

	    /*
	     * If this peer is rotten but we liked his group, forget
	     * about the group.  The peer entry overrides.
	     */
	    if (bnp->bgp_group == okgroup) {
		okgroup = NULL;
	    }
	}
    } BGP_GROUP_LIST_END(bgp);

    return (okgroup);
}


/*
 * bgp_find_peer - find a peer in a particular group, return NULL if
 *   not there.  This assumes the bgp_find_group() was previously called,
 *   so that it is sufficient to simply match the peer address.
 */
bgpPeer *
bgp_find_peer(bgp, addr, lcladdr)
bgpPeerGroup *bgp;
sockaddr_un *addr;
sockaddr_un *lcladdr;
{
    register bgpPeer *bnp;

    BGP_PEER_LIST(bgp, bnp) {
	if (sockaddrcmp_in(addr, bnp->bgp_addr)) {
	    return (bnp);
	}
    } BGP_PEER_LIST_END(bgp, bnp);

    return (bgpPeer *)NULL;
}


/*
 * bgp_find_group_by_addr - find a group when the only thing we know
 *   about a peer is its addresses.  This is a gross and rotten, and
 *   unlikely to return particularly correct answers, but is necessary
 *   to return error messages to protopeers.
 *
 *   I would combine this with bgp_find_group(), but I really think
 *   the spec should change to allow NOTIFY messages before OPEN
 *   messages so this could just go away.  So it is separate to make
 *   it easy to delete.
 */
bgpPeerGroup *
bgp_find_group_by_addr(addr, lcladdr)
sockaddr_un *addr;
sockaddr_un *lcladdr;
{
    register bgpPeerGroup *bgp;
    register bgpPeer *bnp;
    bgpPeerGroup *okgroup = (bgpPeerGroup *)NULL;
    if_addr *ifap;

    /*
     * Determine the shared interface if there is one
     */
    ifap = if_withdst(addr);
    if (ifap != NULL && !sockaddrcmp_in(ifap->ifa_addr_local, lcladdr)) {
	ifap = NULL;
    }

    /*
     * Look for group and peer address matches.  Note that the requirement
     * for a shared interface will help eliminate some groups.
     */
    BGP_GROUP_LIST(bgp) {
	if (okgroup == NULL
	  && bgp->bgpg_allow != NULL
	  && adv_destmask_match(bgp->bgpg_allow,
				addr,
				inet_mask_host)) {
	    int groupisok = 1;

	    if (BGP_NEEDS_SHAREDIF(bgp)) {
		if (bgp->bgpg_gateway != NULL) {
		    if_addr *ifap2;

		    ifap2 = if_withdst(bgp->bgpg_gateway);
		    if (!sockaddrcmp_in(ifap2->ifa_addr_local, lcladdr)) {
			groupisok = 0;
		    }
		} else if (ifap == NULL) {
		    groupisok = 0;
		}
	    }
	    if (groupisok
		&& BIT_TEST(bgp->bgpg_options, BGPO_LCLADDR)
		&& !sockaddrcmp_in(bgp->bgpg_lcladdr->ifae_addr, lcladdr)) {
		groupisok = 0;
	    }
	    if (groupisok) {
		okgroup = bgp;
	    }
	}

	BGP_PEER_LIST(bgp, bnp) {
	    if (sockaddrcmp_in(addr, bnp->bgp_addr)) {
		break;
	    }
	} BGP_PEER_LIST_END(bgp, bnp);

	if (bnp != NULL) {
	    int peerstinks = 0;

	    if (BIT_TEST(bnp->bgp_flags, BGPF_UNCONFIGURED)) {
		peerstinks = 1;
	    } else if (BGP_NEEDS_SHAREDIF(bgp)) {
		if (bnp->bgp_gateway != NULL) {
		    if (!sockaddrcmp_in(bnp->bgp_ifap->ifa_addr_local,
					lcladdr)) {
			peerstinks = 1;
		    }
		} else if (bnp->bgp_ifap != ifap) {
		    peerstinks = 1;
		}
	    } else if (bnp->bgp_lcladdr != NULL) {
		if (!sockaddrcmp_in(bnp->bgp_lcladdr->ifae_addr, lcladdr)) {
		    peerstinks = 1;
		}
	    }

	    if (!peerstinks) {
		/* good one, return group */
		return (bgp);
	    }

	    /*
	     * If this peer is rotten but we liked his group, forget
	     * about the group.  The peer entry overrides.
	     */
	    if (bnp->bgp_group == okgroup) {
		okgroup = NULL;
	    }
	}
    } BGP_GROUP_LIST_END(bgp);

    return (okgroup);
}


/*
 *	Auxilliary protocol initiation and termination
 */

/*
 * bgp_aux_initiate - initiate an internal BGP session when the IGP
 *   is up and ready to go.
 */
static void
bgp_aux_initiate __PF3(tp, task *,
		       proto, proto_t,
		       rtbit, u_int)
{
    bgpPeerGroup *bgp;
    bgpPeer *bnp;

    bgp = (bgpPeerGroup *)tp->task_data;

    trace_tp(tp,
	     TR_NORMAL,
	     0,
	     ("bgp_aux_initiate: starting %s for %s",
	      bgp->bgpg_name,
	      trace_state(rt_proto_bits, proto)));

    /* Non IGP group */
    assert(bgp->bgpg_type == BGPG_INTERNAL_IGP);

    /* Non idled group */
    assert(BIT_TEST(bgp->bgpg_flags, BGPGF_IDLED));

    /*
     * All okay.  Save the rtbit.  Go through the list of peers,
     * initializing any which are ready to go.
     */
    bgp->bgpg_igp_rtbit = rtbit;
    bgp->bgpg_igp_proto = proto;
    BIT_RESET(bgp->bgpg_flags, BGPGF_IDLED);

    BGP_PEER_LIST(bgp, bnp) {
	/* Peer not idle */
	assert(bnp->bgp_state == BGPSTATE_IDLE);

	if (!BIT_TEST(bnp->bgp_flags, BGPF_IDLED)) {
	    bgp_peer_start(bnp, BGPCONN_INIT);
	}
    } BGP_PEER_LIST_END(bgp, bnp);
}


/*
 * bgp_aux_terminate - the IGP is giving up, idle the associated BGP
 */
static void
bgp_aux_terminate __PF1(tp, task *)
{
    bgpPeerGroup *bgp;
    bgpPeer *bnp;

    bgp = (bgpPeerGroup *)tp->task_data;

    trace_tp(tp,
	     TR_NORMAL,
	     0,
	     ("bgp_aux_terminate: terminating %s for %s",
	      bgp->bgpg_name,
	      trace_state(rt_proto_bits, bgp->bgpg_igp_proto)));

    /* Non-IGP group */
    assert(bgp->bgpg_type == BGPG_INTERNAL_IGP);

    /* Already idled */
    assert(!BIT_TEST(bgp->bgpg_flags, BGPGF_IDLED));

    /*
     * Okay.  Idle the group and terminate all running peers.
     */
    BIT_SET(bgp->bgpg_flags, BGPGF_IDLED);
    BGP_PEER_LIST(bgp, bnp) {
	if (bnp->bgp_state != BGPSTATE_IDLE) {
	    if (bnp->bgp_state == BGPSTATE_OPENSENT
	      || bnp->bgp_state == BGPSTATE_OPENCONFIRM
	      || bnp->bgp_state == BGPSTATE_ESTABLISHED) {
		bgp_send_notify_none(bnp, BGP_CEASE, 0);
	    }
	    bgp_peer_close(bnp, BGPEVENT_STOP);
	}
    } BGP_PEER_LIST_END(bgp, bnp) ;
    bgp->bgpg_igp_rtbit = 0;
    bgp->bgpg_igp_proto = 0;
}



/*
 *	Protopeer support routines.
 */

/*
 * bgp_pp_dump - dump BGP protopeer status to dump file
 */
static void
bgp_pp_dump(tp, fd)
task *tp;
FILE *fd;
{
    bgpProtoPeer *bpp;

    bpp = (bgpProtoPeer *) tp->task_data;

    /*
     * Not much to dump here
     */
    (void) fprintf(fd,
       "\n\tPeer Address: %#A\tLocal Address: %#A\tConnected seconds: %d\n",
	bpp->bgpp_addr,
	bpp->bgpp_myaddr,
	bgp_time_sec - bpp->bgpp_connect_time);

    if (bpp->bgpp_bufpos != bpp->bgpp_readptr) {
	(void) fprintf(fd, "\t\tReceived and buffered octets: %u\n",
	  bpp->bgpp_readptr - bpp->bgpp_bufpos);
    }
}


/*
 * bgp_pp_timeout - receive a timeout notification for a protopeer
 */
static void
bgp_pp_timeout(tip, interval)
task_timer *tip;
time_t interval;
{
    bgpProtoPeer *bpp = (bgpProtoPeer *) tip->task_timer_data;

    trace_log_tp(tip->task_timer_task,
		 0,
		 LOG_WARNING,
		 ("bgp_pp_timeout: peer %s timed out waiting for OPEN",
		  bpp->bgpp_name));

    /*
     * Same as above.  Send the default open message, then send
     * a holdtime notification.  Then dump him.
     */
    bgp_pp_notify_none(bpp, (bgpPeerGroup *)0, (bgpPeer *)0,
      BGP_ERR_HOLDTIME, 0);
    bgp_pp_delete(bpp);
}



/*
 *	Peer creation/deletion/state transition handling
 */


/*
 * bgp_peer_create - create a peer in the specified group
 */
static bgpPeer *
bgp_peer_create __PF1(bgp, bgpPeerGroup *)
{
    bgpPeer *bnp;

    /*
     * Fetch a peer structure and zero it
     */
    bnp = bgp_peer_alloc(bgp->bgpg_task);
    bzero((caddr_t)bnp, sizeof(bgpPeer));
    bnp->bgp_group = bgp;

    /*
     * Copy configuration information out of the group structure
     */
    bnp->bgp_conf = bgp->bgpg_conf;	/* struct copy */
    if (bnp->bgp_trace_options) {
	(void) trace_alloc(bnp->bgp_trace_options);
    }
    if (BIT_TEST(bnp->bgp_options, BGPO_GATEWAY)) {
	bnp->bgp_gateway = sockdup(bnp->bgp_gateway);
    }
    if (BIT_TEST(bnp->bgp_options, BGPO_LCLADDR)) {
	bnp->bgp_lcladdr = ifae_alloc(bnp->bgp_lcladdr);
    }

    /*
     * Now set up the gw entry.  Mostly zeros at this point
     */
    bnp->bgp_proto = RTPROTO_BGP;
    bnp->bgp_peer_as = bgp->bgpg_peer_as;
    bnp->bgp_local_as = bgp->bgpg_local_as;
    bnp->bgp_gw.gw_rtq.rtq_forw = bnp->bgp_gw.gw_rtq.rtq_back
      = &(bnp->bgp_gw.gw_rtq);
    if (bgp->bgpg_type == BGPG_INTERNAL_IGP) {
	BIT_SET(bnp->bgp_gw.gw_flags, GWF_AUXPROTO);
    }
 
    /*
     * That's all we can do for now
     */
    return (bnp);
}


/*
 * bgp_new_peer - create a new peer from a protopeer in the specified
 *   peer group
 */
bgpPeer *
bgp_new_peer(bgp, bpp, length)
bgpPeerGroup *bgp;
bgpProtoPeer *bpp;
size_t length;
{
    bgpPeer *bnp;
    int save_socket;

    /*
     * Fetch a new peer
     */
    bnp = bgp_peer_create(bgp);

    /*
     * Set the flags.  Mark this guy unconfigured
     */
    bnp->bgp_flags = BGPF_UNCONFIGURED;

    /*
     * Now set up the gw entry.  This mostly requires assembling the
     * data from other places.
     */
    bnp->bgp_addr = sockdup(bpp->bgpp_task->task_addr);
    bgp_make_names(bnp);
    /* XXX this policy stuff isn't quite right.  Think about it */
    switch (bgp->bgpg_type) {
    case BGPG_EXTERNAL:
    case BGPG_INTERNAL:
    case BGPG_INTERNAL_RT:
	bnp->bgp_import = control_exterior_locate(bgp_import_list, bnp->bgp_peer_as);
	bnp->bgp_export = control_exterior_locate(bgp_export_list, bnp->bgp_peer_as);
	break;
    default:
	break;
    }

    /*
     * Copy the local address from the protopeer.  If this requires
     * an interface pointer, determine this now.
     */
    bnp->bgp_myaddr = bpp->bgpp_myaddr;
    bpp->bgpp_myaddr = NULL;
    if (BGP_USES_SHAREDIF(bgp)) {
	if (bgp_set_peer_if(bnp, 0) == NULL) {
	    assert(!BGP_NEEDS_SHAREDIF(bgp));
	}
    }

    /*
     * Link it to the group structure.  This must not be moved north
     * of here since bgp_peer_add() also does a bgp_sort_add(), and
     * bgp_sort_add depends on the addresses.
     */
    bgp_peer_add(bgp, bnp);

    /*
     * Initialize counters
     */
    bnp->bgp_in_notupdates = 1;
    bnp->bgp_in_octets = (u_long)length;

    /*
     * Copy the buffer in from the protopeer
     */
    bgp_buffer_copy(&(bpp->bgpp_inbuf), &(bnp->bgp_inbuf));

    /*
     * Okay so far.  Now save the socket and reset it in the protopeer's
     * task.  Then delete the protopeer and create a new task for the peer.
     */
    save_socket = bpp->bgpp_task->task_socket;
    task_reset_socket(bpp->bgpp_task);
    bgp_pp_delete(bpp);
    bgp_task_create(bnp);
    task_set_socket(bnp->bgp_task, save_socket);
    bgp_recv_setbuf(bnp->bgp_task, bnp->bgp_recv_bufsize,bnp->bgp_send_bufsize);

    /*
     * Churn this peer into active state from idle, the caller will
     * do the rest.
     */
    bnp->bgp_state = BGPSTATE_IDLE;
    /* XXX This transition to active violates the FSM.  The FSM is shitty */
    bgp_event(bnp, BGPEVENT_START, BGPSTATE_ACTIVE);

    /*
     * Start the connection timer.
     */
    bgp_init_traffic_timer(bnp);

    return (bnp);
}


/*
 * bgp_use_protopeer - a protopeer has been found to match a configured
 *   peer.  We shut down the peer's socket, delete the connect timer
 *   if there is one, reset the traffic timer if this is running, move
 *   the protopeer's socket to the peer and do the state transitions to
 *   get this to active.
 */
void
bgp_use_protopeer(bnp, bpp, length)
bgpPeer *bnp;
bgpProtoPeer *bpp;
size_t length;
{
    task *tp;
    int save_socket;

    /*
     * Check to make sure we aren't in idle state.  If we are
     * someone really screwed up.
     */
    /* State is IDLE!! */
    assert(bnp->bgp_state != BGPSTATE_IDLE);

    /*
     * Connect timer first
     */
    tp = bnp->bgp_task;
    if (bnp->bgp_connect_timer) {
	bgp_delete_connect_timer(bnp);
    }

    /*
     * Got that all shut down.  If the peer has a socket (i.e.
     * is not in IDLE or ACTIVE) close the socket and delete
     * it.  If the peer has buffers (i.e. not IDLE or ACTIVE or
     * CONNECT) delete those too. Then move the protopeer's socket
     * and dump the protopeer.
     */
    if (bnp->bgp_state != BGPSTATE_IDLE && bnp->bgp_state != BGPSTATE_ACTIVE) {
	task_close(tp);
	if (bnp->bgp_state != BGPSTATE_CONNECT) {
	    bgp_buffer_free(&(bnp->bgp_inbuf));
	}
    }
    save_socket = bpp->bgpp_task->task_socket;
    task_reset_socket(bpp->bgpp_task);
    task_set_socket(tp, save_socket);
    bgp_recv_setbuf(tp, bnp->bgp_recv_bufsize, bnp->bgp_send_bufsize);
    bgp_buffer_copy(&(bpp->bgpp_inbuf), &(bnp->bgp_inbuf));

    sock2port(bnp->bgp_addr) =
	sock2port(tp->task_addr) =
	    sock2port(bpp->bgpp_task->task_addr);

    /*
     * Sanity check this to make sure the local address is correct.  The
     * code which found this peer should have done this already but an
     * extra check is cheap.
     */
    if (BGP_NEEDS_SHAREDIF(bnp->bgp_group)) {
	assert(bnp->bgp_ifap != NULL);
	assert(sockaddrcmp_in(bnp->bgp_ifap->ifa_addr_local, bpp->bgpp_myaddr));
    } else if (BIT_TEST(bnp->bgp_options, BGPO_LCLADDR)) {
	assert(sockaddrcmp_in(bnp->bgp_lcladdr->ifae_addr, bpp->bgpp_myaddr));
    }

    if (bnp->bgp_myaddr != NULL) {
	sockfree(bnp->bgp_myaddr);
    }
    bnp->bgp_myaddr = bpp->bgpp_myaddr;
    bpp->bgpp_myaddr = NULL;

    /*
     * If there is no shared interface currently, but the flavour of BGP
     * wants to know should a shared interface exist, determine this now.
     */
    if (bnp->bgp_ifap == NULL && BGP_USES_SHAREDIF(bnp->bgp_group)) {
	if_addr *ifap;

	if (bnp->bgp_gateway != NULL) {
	    ifap = if_withdst(bnp->bgp_gateway);
	} else {
	    ifap = if_withdst(bnp->bgp_addr);
	}

	if (ifap != NULL && sockaddrcmp_in(ifap->ifa_addrent_local->ifae_addr,
					   bnp->bgp_myaddr)) {
	    IFA_ALLOC(bnp->bgp_ifap = ifap);
	}
    }

    /*
     * Dump the protopeer.
     */
    bgp_pp_delete(bpp);

    /*
     * Straighten up a few structure variables
     */
    bnp->bgp_hisversion = BGP_VERSION_UNSPEC;
    bnp->bgp_in_updates = bnp->bgp_out_updates = 0;
    bnp->bgp_out_notupdates = 0;
    bnp->bgp_out_octets = 0;
    bnp->bgp_in_notupdates = 1;
    bnp->bgp_in_octets = (u_long)length;

    /*
     * Done that.  Now do the right thing with regard to the state.
     */
    if (bnp->bgp_state != BGPSTATE_ACTIVE) {
	if (bnp->bgp_state != BGPSTATE_IDLE) {
	    bgp_event(bnp, BGPEVENT_STOP, BGPSTATE_IDLE);
	}
	/* XXX Violate the FSM.  Make my day. */
	bgp_event(bnp, BGPEVENT_START, BGPSTATE_ACTIVE);
    }

    /*
     * Finally, set the traffic timer to time out OPENCONFIRM state
     */
    bgp_init_traffic_timer(bnp);

    /*
     * Ha! Ah gone!
     */
}


/*
 * bgp_peer_start - start up a peer from idle state
 */
static void
bgp_peer_start(bnp, start_interval)
bgpPeer *bnp;
time_t start_interval;
{
    int connect_now;

    /*
     * This doesn't actually do much.  If the peer is passive just
     * transition to ACTIVE state.  Otherwise we need to set the
     * connect timer to start us up.  If the start interval is
     * zero do an immediate connect
     */
    if (BIT_TEST(bnp->bgp_options, BGPO_PASSIVE) || start_interval != 0) {
	/* XXX Violate stinking FSM */
	bgp_event(bnp, BGPEVENT_START, BGPSTATE_ACTIVE);
	if (BIT_TEST(bnp->bgp_options, BGPO_PASSIVE)) {
	    return;
	}
	connect_now = 0;
    } else {
	bgp_event(bnp, BGPEVENT_START, BGPSTATE_CONNECT);
	connect_now = 1;
	start_interval = BGPCONN_INTERVAL(bnp);
    }

    bgp_set_connect_timer(bnp, start_interval);
    if (connect_now) {
	bgp_connect_start(bnp);
    }
}



/*
 * bgp_peer_close - shut down a connection to IDLE from whatever state it
 *   is in, then start it up again as appropriate.
 *
 * This used to be bgp_neighbour_close(), but was renamed in recognition
 * of the odd spelling some people use.  Should purge the odd spelling
 * from the configuration file, actually.
 */
void
bgp_peer_close(bnp, event)
bgpPeer *bnp;
int event;
{
    task *tp = bnp->bgp_task;
    bgpPeerGroup *bgp = bnp->bgp_group;
    int needs_restart;
    time_t interval;

    trace_tp(tp,
	     TR_NORMAL|TR_STATE,
	     0,
	     ("bgp_peer_close: closing peer %s, state is %d (%s)",
	      bnp->bgp_name,
	      bnp->bgp_state,
	      trace_state(bgp_state_bits, bnp->bgp_state)));

    /*
     * If the peer has spooled write data, flush what we can and dump the
     * rest.
     */
    if (bnp->bgp_outbuf != NULL) {
	if (!BIT_TEST(bnp->bgp_flags, BGPF_WRITEFAILED)) {
	    (void) bgp_force_write(bnp);
	}
	if (bnp->bgp_outbuf != NULL) {
	    bgp_outbuf_free(bnp);
	}
    }

    /*
     * If he has a write routine running, cancel the flag
     * and zero the routine so we don't have unwanted state
     * when the socket is reopenned.
     */
    if (BIT_TEST(bnp->bgp_flags, BGPF_SEND_RTN)) {
	task_set_write(tp, NULL);
	BIT_RESET(bnp->bgp_flags, BGPF_SEND_RTN);
    }

    /*
     * If this guy failed while writing, clean up his state so we
     * don't get confused next time.  Also clean up other assorted
     * flags.
     * XXX does this leave the cleanup job running?
     */
    BIT_RESET(bnp->bgp_flags, BGPF_CLEANUP|BGPF_INITIALIZING|BGPF_WRITEFAILED);

    /*
     * If it is in ESTABLISHED there may be routes in the table from it.
     * Delete them now, then free the routing bit.  Drop us out of
     * established *before* doing this so we don't try to flash this
     * peer.  Otherwise close the socket if it is open and drop us to IDLE.
     */
    if (bnp->bgp_state == BGPSTATE_ESTABLISHED) {
        bgp_event(bnp, event, BGPSTATE_IDLE);
	bgp_n_established--;
	bgp->bgpg_n_established--;
	if (bnp->bgp_version == BGP_VERSION_4) {
	    bgp->bgpg_n_v4_established--;
	}
	bgp_rt_terminate(bnp);
	bgp_iflist_free(bnp);
	task_close(tp);
    } else if (bnp->bgp_state != BGPSTATE_IDLE) {
	if (bnp->bgp_state != BGPSTATE_ACTIVE) {
	    task_close(tp);
	}
        bgp_event(bnp, event, BGPSTATE_IDLE);
    }

    /*
     * Decide if this is one we will immediately restart.  Do this here
     * so we know whether to delete the connect timer or not.
     */
    if (BIT_TEST(bnp->bgp_flags, BGPF_IDLED|BGPF_UNCONFIGURED|BGPF_DELETE)
      || BIT_TEST(bgp->bgpg_flags, BGPGF_IDLED|BGPGF_DELETE)) {
	needs_restart = 0;
    } else {
	needs_restart = 1;
    }

    /*
     * Delete the traffic timer if there.  Delete the connect timer unless
     * it may be needed.  Delete route timer from peer or group if necessary.
     */
    bgp_delete_traffic_timer(bnp);
    bnp->bgp_rto_next_time = (time_t) 0;
    if (bnp->bgp_route_timer) {
	bgp_route_timer_delete(bnp);
    } else if (bgp->bgpg_route_timer && bgp->bgpg_n_established == 0) {
	bgp->bgpg_rto_next_time = (time_t) 0;
	bgp_group_route_timer_delete(bgp);
    }
    if (bnp->bgp_connect_timer) {
	if (!needs_restart || BIT_TEST(bnp->bgp_options, BGPO_PASSIVE)) {
	    bgp_delete_connect_timer(bnp);
	} else {
	    bgp_reset_connect_timer(bnp);
	}
    }

    /*
     * If we have a buffer, drop it now.
     */
    if (bnp->bgp_buffer != NULL) {
	bgp_buffer_free(&(bnp->bgp_inbuf));
    }

    /*
     * Delete local address and zero the port numbers in the peer's address.
     * Forget any interface pointer we don't need.
     */
    if (bnp->bgp_myaddr != NULL) {
	sockfree(bnp->bgp_myaddr);
	bnp->bgp_myaddr = NULL;
    }
    sock2port(bnp->bgp_addr) = 0;
    if (tp->task_addr != NULL) {
        sock2port(tp->task_addr) = 0;
    }
    if (bnp->bgp_ifap != NULL && !BGP_NEEDS_SHAREDIF(bnp->bgp_group)) {
	IFA_FREE(bnp->bgp_ifap);
	bnp->bgp_ifap = NULL;
    }

    /*
     * If the peer is unconfigured or deleted, dump the bastard.
     */
    if (!needs_restart) {
        if (BIT_TEST(bnp->bgp_flags, BGPF_UNCONFIGURED|BGPF_DELETE)) {
            task_delete(tp);
            bnp->bgp_task = NULL;
	    bgp_peer_remove(bnp);
	    if (bnp->bgp_gateway != NULL) {
		sockfree(bnp->bgp_gateway);
		bnp->bgp_gateway = NULL;
	    }
	    if (bnp->bgp_myaddr != NULL) {
		sockfree(bnp->bgp_myaddr);
		bnp->bgp_myaddr = NULL;
	    }
	    if (bnp->bgp_lcladdr) {
		ifae_free(bnp->bgp_lcladdr);
		bnp->bgp_lcladdr = NULL;
	    }
	    if (bnp->bgp_addr != NULL) {
		sockfree(bnp->bgp_addr);
		bnp->bgp_addr = NULL;
	    }
	    if (bnp->bgp_ifap != NULL) {
		IFA_FREE(bnp->bgp_ifap);
		bnp->bgp_ifap = NULL;
	    }
	    if (bnp->bgp_trace_options) {
		trace_freeup(bnp->bgp_trace_options);
	    }
	    bgp_peer_free(bnp);
	}
	return;
    }

    /*
     * If it is passive, or if the peer is busy with version
     * negotiation, restart immediately.  Otherwise, restart
     * as appropriate for the number of connection failures
     * it has had.
     */
    if (BIT_TEST(bnp->bgp_options, BGPO_PASSIVE)
      || bnp->bgp_hisversion != BGP_VERSION_UNSPEC) {
	interval = 0;
	bnp->bgp_connect_failed = 0;
    } else {
	interval = BGPCONN_INTERVAL(bnp);
    }
    bgp_peer_start(bnp, interval);
}


/*
 * bgp_peer_established - transition a peer to established state
 */
void
bgp_peer_established(bnp)
bgpPeer *bnp;
{
    bgpPeerGroup *bgp = bnp->bgp_group;

    /* State must be OpenConfirm */
    assert(bnp->bgp_state == BGPSTATE_OPENCONFIRM);

    /*
     * The *only* way we get to ESTABLISHED from OPENCONFIRM is
     * if we receive a keepalive.  Tell the world.
     */
    bgp_event(bnp, BGPEVENT_RECVKEEPALIVE, BGPSTATE_ESTABLISHED);

    /*
     * Mark this guy established
     */
    bgp->bgpg_n_established++;
    bgp_n_established++;
    if (bnp->bgp_version == BGP_VERSION_4) {
	bgp->bgpg_n_v4_established++;
    }

    /*
     * Clear the version negotiation info and the connect errors counter.
     */
    bnp->bgp_hisversion = BGP_VERSION_UNSPEC;
    bnp->bgp_connect_failed = 0;

    /*
     * Reset his traffic timer now.
     */
    bgp_init_traffic_timer(bnp);

    /*
     * If it is a peer group which depends on it, adjust the interface
     * list.
     */
    bgp_iflist_add(bnp);

    /*
     * Send the initial blast of routes.  I really wish we could try
     * reading the socket first, since this might prevent a lot of
     * unnecessary sending, but everything assumes that all ESTABLISHED
     * peers are synchronized which they aren't until after the initial
     * send.  Oh, well.
     *
     * But for BGP2/3/4 we no longer need to keep established peers
     * synchronized, and we do our first write to the peer in a
     * task_write() routine.  Since reads get preference to writes we
     * will read anything the peer has sent in already before sending
     * to him.  This works out okay, if we like his routes better we
     * may not send any of our own.
     */
    bgp_rt_send_init(bnp);

    /*
     * For good measure, do a read.  This whole process kind of leaves
     * the rest of gated locked out for a substantial amount of time, but
     * I dare not go ahead without doing the read now if there are
     * buffered messages.  Note that an error in bgp_rt_send_init() would
     * have taken this out of ESTABLISHED
     *
     * XXX bgp_rt_send_init does this now, since we don't know what to call
     */
#ifdef	notdef
    if (bnp->bgp_state == BGPSTATE_ESTABLISHED
      && BGPBUF_SPACE(bnp) >= BGP_HEADER_LEN) {
        bgp_recv_update(tp);
    }
#endif	/* notdef */
}


/*
 * bgp_ifachange - deal with changes in the interface configuration
 */
static void
bgp_ifachange __PF2(tp, task *,
		    ifap, if_addr *)
{
    bgpPeerGroup *bgp;
    bgpPeer *bnp, *bnpnext;
    int check_up = 0;		/* interface may have come up */
    int check_down = 0;		/* interface may have gone down */
    int affected = 0;		/* count of affected peers */

    /*
     * This is what BGP peers care about:
     *
     * Peers in EXTERNAL and INTERNAL groups want a shared interface
     * with their neighbours, and care about this.  If the shared
     * interface goes down the peers which use it should be idled.
     * If the local address on the shared interface changes, peers which
     * use it may need to be taken down and restarted.  If we get a
     * new interface which is "better" than the current one (i.e. a
     * P2P interface with an address we were formerly assuming to be
     * on a multiaccess network) we need to restart the peer since
     * next hops might have changed.
     *
     * Peers in TEST groups care about shared interfaces as the above
     * when one exists, but will run without a shared interface when
     * none exists.  The effects the next hops which are advertised.
     * We only need idle a TEST peer if the local address is configured
     * and that address doesn't exist, but may want to take it down
     * and bring it back up if a shared interface comes up or goes
     * down.
     *
     * Peers in IGP groups couldn't care less about the shared interface,
     * and just want to make sure they are using a valid local address.
     * If all interfaces with that local address go away we may need to
     * idle the peer (if the local address was configured) or restart
     * it (if it acquired its local address by defaulting the connect()).
     *
     * Peers in ROUTING groups couldn't care less about shared interfaces
     * either, but the groups need to maintain their interface list based
     * on this.
     *
     * Jeff told me that I should only count local addresses as "up" if
     * the local address is on an interface which is "up".  This is more
     * severe than necessary, but blame him.
     */

    /*
     * We only care if the interface went up or down, or if it changed
     * while it was up.  Ignore everything else.
     */
    if (BIT_TEST(ifap->ifa_state, IFS_UP)) {
	if (BIT_TEST(ifap->ifa_change,
	  IFC_ADD|IFC_ADDR|IFC_NETMASK|IFC_UPDOWN)) {
	    check_up = 1;
	}
	if (BIT_TEST(ifap->ifa_change, IFC_DELETE|IFC_ADDR|IFC_NETMASK)) {
	    check_down = 1;
	}
    } else if (BIT_TEST(ifap->ifa_change, IFC_UPDOWN)) {
#ifdef	notdef
    } else if (BIT_TEST(ifap->ifa_change, IFC_DELETE|IFC_UPDOWN)) {
#endif	/* notdef */
	check_down = 1;
    }
    
    if (!check_up && !check_down) {
	if (bgp_n_ifa_policy_groups) {
	    BGP_GROUP_LIST(bgp) {
		if (bgp->bgpg_ifap_policy) {
		    trace_tp(tp,
			     TR_NORMAL,
			     0,
			     ("bgp_ifachange: checking %s interface policy for %A(%s)",
			      bgp->bgpg_name,
			      ifap->ifa_addr,
			      ifap->ifa_link->ifl_name));
		    bgp_check_ifa_policy(bgp, ifap);
		}
	    } BGP_GROUP_LIST_END(bgp);
	    return;
	}
	trace_tp(tp,
		 TR_NORMAL,
		 0,
		 ("bgp_ifachange: interface %A(%s) changes not interesting to us",
		  ifap->ifa_addr,
		  ifap->ifa_link->ifl_name));
	return;
    }

    trace_tp(tp,
	     TR_NORMAL,
	     0,
	     ("bgp_ifachange: interface %A(%s) %s, checking who this bothers",
	      ifap->ifa_addr,
	      ifap->ifa_link->ifl_name,
	      (check_up ? (check_down ? "changed" : "came up") : "went down")));

    /*
     * Scan through the groups.  For those groups which need to be
     * checked, scan through the peers.
     */
    BGP_GROUP_LIST(bgp) {
	if (bgp->bgpg_ifap_policy) {
	    trace_tp(tp,
		     TR_NORMAL,
		     0,
		     ("bgp_ifachange: checking %s interface policy for %A(%s)",
		      bgp->bgpg_name,
		      ifap->ifa_addr,
		      ifap->ifa_link->ifl_name));
	    bgp_check_ifa_policy(bgp, ifap);
	}
	for (bnp = bgp->bgpg_peers; bnp != NULL; bnp = bnpnext) {
	    int stopit = 0;
	    bnpnext = bnp->bgp_next;

	    if (!BGP_NEEDS_SHAREDIF(bgp)) {
		if (check_up && BIT_TEST(bnp->bgp_flags, BGPF_IDLED)) {
		    /*
		     * Check to see if an appropriate interface for him
		     * has arrived.  The only way he could get in this
		     * state is if a local address was specified.
		     */
		    assert(bnp->bgp_lcladdr != NULL);
		    if (IFAE_ADDR_EXISTS(bnp->bgp_lcladdr)) {
			BIT_RESET(bnp->bgp_flags, BGPF_IDLED);
			stopit = 1;
			trace_tp(tp,
				 TR_NORMAL|TR_STATE,
				 0,
				 ("bgp_ifachange: peer %s started, local address %A now exists",
				  bnp->bgp_name,
				  bnp->bgp_lcladdr->ifae_addr));
		    }
		} else if (check_down
		  && !BIT_TEST(bnp->bgp_flags, BGPF_IDLED)) {

		    if (bnp->bgp_lcladdr != NULL) {
			if (!IFAE_ADDR_EXISTS(bnp->bgp_lcladdr)) {
			    if (!BIT_TEST(bnp->bgp_flags, BGPF_UNCONFIGURED)) {
				BIT_SET(bnp->bgp_flags, BGPF_IDLED);
			    }
			    trace_tp(tp,
				     TR_NORMAL|TR_STATE,
				     0,
				     ("bgp_ifachange: peer %s %s, local address %A no longer exists",
				      bnp->bgp_name,
				      (BIT_TEST(bnp->bgp_flags, BGPF_UNCONFIGURED)
				       ? "terminated" : "idled"),
				      bnp->bgp_lcladdr->ifae_addr));
			    stopit = 1;
			}
		    } else if (bnp->bgp_myaddr != NULL) {
			if_addr_entry *ifae;

			if ((ifae = ifae_locate(bnp->bgp_myaddr,
			    &if_local_list)) == NULL
			  || !IFAE_ADDR_EXISTS(ifae)) {
			    trace_tp(tp,
				     TR_NORMAL|TR_STATE,
				     0,
				     ("bgp_ifachange: peer %s %s, local address %A no longer exists",
				      bnp->bgp_name,
				      (BIT_TEST(bnp->bgp_flags, BGPF_UNCONFIGURED)
				       ? "terminated" : "restarted"),
				      bnp->bgp_myaddr));
			    stopit = 1;
			}
		    }
		}
	    }

	    if (!stopit && BGP_USES_SHAREDIF(bgp)) {
		if_addr *new_ifap;

		/*
		 * Find the current view of the shared interface
		 */
		if (BIT_TEST(bnp->bgp_options, BGPO_GATEWAY)) {
		    new_ifap = if_withdst(bnp->bgp_gateway);
		} else {
		    new_ifap = if_withdst(bnp->bgp_addr);
		}
		if (new_ifap != NULL 
	            && bnp->bgp_lcladdr != NULL
		    && !BIT_TEST(bnp->bgp_options, BGPO_GATEWAY)
		    && new_ifap->ifa_addrent_local != bnp->bgp_lcladdr) {
		    new_ifap = NULL;
		}
		if (new_ifap != bnp->bgp_ifap
		 || (new_ifap != NULL && bnp->bgp_myaddr != NULL
		   && !sockaddrcmp_in(new_ifap->ifa_addrent_local->ifae_addr,
				     bnp->bgp_myaddr))) {
		    if_addr *old_ifap = bnp->bgp_ifap;

		    /*
		     * Something is different.  If we have a new interface
		     * we'll need to shut down this peer (if running) and
		     * (re)start him.
		     */
		    if (BGP_NEEDS_SHAREDIF(bgp)) {
			if (new_ifap != NULL) {
			    IFA_ALLOC(new_ifap);
			}
			if (bnp->bgp_ifap != NULL) {
			    IFA_FREE(bnp->bgp_ifap);
			}
			bnp->bgp_ifap = new_ifap;
			if (new_ifap == NULL) {
			    if (!BIT_TEST(bnp->bgp_flags, BGPF_UNCONFIGURED)) {
				BIT_SET(bnp->bgp_flags, BGPF_IDLED);
			    }
			} else {
			    BIT_RESET(bnp->bgp_flags, BGPF_IDLED);
			}
		    }
		    if (TRACE_TP(tp, TR_NORMAL|TR_STATE)) {
			const char *action = "restarted";
			const char *change;

			if (new_ifap == NULL) {
			    change = "went down";
			    if (BIT_TEST(bnp->bgp_flags, BGPF_IDLED)) {
				action = "idled";
			    } else {
				action = "terminated";
			    }
			} else if (old_ifap != NULL) {
			    change = "changed";
			} else {
			    change = "came up";
			    if (!BGP_OPTIONAL_SHAREDIF(bgp)) {
				action = "started";
			    }
			}
			trace_only_tp(tp,
				      0,
				      ("bgp_ifachange: peer %s %s, shared interface %s",
				       bnp->bgp_name,
				       action,
				       change));
		    }
		    stopit = 1;
		}
	    }

	    /*
	     * Stop and/or start peer if his status changed and if
	     * his group is not idled.
	     */
	    if (stopit) {
		affected++;
		if (BIT_TEST(bgp->bgpg_flags, BGPGF_IDLED)) {
		    trace_tp(tp,
			     TR_STATE,
			     0,
			     ("bgp_ifachange: peer %s not started, group is idle",
			      bnp->bgp_name));
		} else {
		    if (bnp->bgp_state == BGPSTATE_OPENSENT
		      || bnp->bgp_state == BGPSTATE_OPENCONFIRM
		      || bnp->bgp_state == BGPSTATE_ESTABLISHED) {
			bgp_send_notify_none(bnp, BGP_CEASE, 0);
		    }
		    bnp->bgp_connect_failed = 0;
		    bnp->bgp_hisversion = BGP_VERSION_UNSPEC;
		    if (bnp->bgp_state != BGPSTATE_IDLE) {
			/*
			 * bgp_peer_close will do the right thing.
			 *
			 * XXX should we ask for an immediate restart?
			 */
			bgp_peer_close(bnp, BGPEVENT_STOP);
		    } else {
			bgp_peer_start(bnp, (time_t) BGPCONN_IFUP);
		    }
	        }
	    }
	}
    } BGP_GROUP_LIST_END(bgp);

    if (!affected) {
	trace_tp(tp,
		 TR_STATE,
		 0,
		 ("bgp_ifachange: no BGP peers found affected by interface change"));
    }
}


/*
 * bgp_fake_terminate - terminate routine which doesn't do anything.
 */
static void
bgp_fake_terminate(tp)
task *tp;
{
    /* nothing */
}


/*
 * bgp_terminate - terminate BGP cleanly and in sequence.
 */
static void
bgp_terminate(tp)
task *tp;
{
    bgpPeerGroup *bgp;
    bgpPeer *bnp;
    int delete_task;

    /*
     * Kill all the protopeers.  Don't send them anything, it would be
     * an FSM violation anyway.
     */
    bgp_pp_delete_all();

    /*
     * Go through each group IDLEing all peers and deleting their
     * tasks.  Then delete the group's task.  Finally, delete the
     * listen task.  Since terminating a peer may cause it to
     * be reordered in the list, we search the list from the beginning
     * multiple times.
     */
    BGP_GROUP_LIST(bgp) {
	do {
	    BGP_PEER_LIST(bgp, bnp) {
		if (bnp->bgp_task == NULL) {
		    continue;
		}
		delete_task = 1;
		if (bnp->bgp_state != BGPSTATE_IDLE) {
		    /*
		     * If it is far enough along to have sent an OPEN message,
		     * send a CEASE.
		     */
		    if (bnp->bgp_state == BGPSTATE_OPENSENT
		      || bnp->bgp_state == BGPSTATE_OPENCONFIRM
		      || bnp->bgp_state == BGPSTATE_ESTABLISHED) {
			bgp_send_notify_none(bnp, BGP_CEASE, 0);
		    }

		    /*
		     * Determine whether we need to delete the
		     * task after the peer is closed.  If not,
		     * idle the peer so it won't be auto-started.
		     */
		    if (BIT_TEST(bnp->bgp_flags,
		      BGPF_UNCONFIGURED|BGPF_DELETE)) {
			delete_task = 0;
		    } else {
			BIT_SET(bnp->bgp_flags, BGPF_IDLED);
		    }
		    bgp_peer_close(bnp, BGPEVENT_STOP);
		}

		/*
		 * Now delete its task if someone else hasn't.
		 */
		if (delete_task) {
		    task_delete(bnp->bgp_task);
		    bnp->bgp_task = NULL;
		}

		/*
		 * Break out of loop so we start again from the beginning.
		 */
		break;
	    } BGP_PEER_LIST_END(bgp, bnp);
	} while (bnp != NULL);

	/*
	 * Now delete the group's task
	 */
	task_delete(bgp->bgpg_task);
	bgp->bgpg_task = NULL;
    } BGP_GROUP_LIST_END(bgp);

    /*
     * Finally, stop listening.
     */
    bgp_listen_stop();
}


/*
 * bgp_group_init - initialize a BGP group into shape
 */
static void
bgp_group_init __PF1(bgp, bgpPeerGroup *)
{
    /*
     * All that is required of us here is to make the group's
     * name and associated task.  Except for an IGP group, where
     * we need to register it, and don't bother with the task.
     */
    bgp_make_group_names(bgp);
    bgp_group_task_create(bgp);

    trace_tp(bgp->bgpg_task,
	     TR_NORMAL,
	     0,
	     ("bgp_group_init: initializing %s", bgp->bgpg_name));

    if (bgp->bgpg_type == BGPG_INTERNAL_IGP) {
	BIT_SET(bgp->bgpg_flags, BGPGF_IDLED);
	task_aux_register(bgp->bgpg_task,
			  RTPROTO_OSPF_ASE,
			  bgp_aux_initiate,
			  bgp_aux_terminate,
			  bgp_aux_flash,
			  bgp_aux_newpolicy);
    }
}


/*
 * bgp_peer_init - initialize a BGP peer into running state
 */
static void
bgp_peer_init __PF1(bnp, bgpPeer *)
{
    /*
     * The peer given to us here is configured, and in its group's
     * peer list, but is barely out of configuration and needs a
     * task and other junk to go along with this.  Do this now.
     */
    
    /*
     * Copy some stuff into the gw entry from other places.
     */
    bnp->bgp_local_as = bnp->bgp_conf_local_as;
    bnp->bgp_peer_as = bnp->bgp_group->bgpg_peer_as;

    /*
     * Make a name for this peer, and create a task.
     */
    bgp_make_names(bnp);
    bgp_task_create(bnp);

    /*
     * Determine if this peer requires a shared interface and, if
     * so, whether we have one.  If not, emit a warning and idle
     * the peer.  If it doesn't use a shared interface make sure the
     * local address is valid.  If not, idle the peer.
     */
    if (BGP_NEEDS_SHAREDIF(bnp->bgp_group)) {
	if (bgp_set_peer_if(bnp, 1) == NULL) {
	    BIT_SET(bnp->bgp_flags, BGPF_IDLED);
	}
    } else if (bnp->bgp_lcladdr != NULL) {
	if (!IFAE_ADDR_EXISTS(bnp->bgp_lcladdr)) {
	    trace_log_tf(bnp->bgp_trace_options,
			 0,
			 LOG_WARNING,
			 ("bgp_peer_init: BGP peer %s local address %A not found.  Leaving peer idled",
			  bnp->bgp_name,
			  bnp->bgp_lcladdr->ifae_addr));
	    BIT_SET(bnp->bgp_flags, BGPF_IDLED);
	}
    }

    /*
     * Put the peer in idle state.  If the peer isn't idled then start it up.
     */
    bnp->bgp_state = BGPSTATE_IDLE;
    if (!BIT_TEST(bnp->bgp_group->bgpg_flags, BGPGF_IDLED)
      && !BIT_TEST(bnp->bgp_flags, BGPF_IDLED)) {
	bgp_peer_start(bnp, BGPCONN_INIT);
    }
}


/*
 * bgp_peer_delete - delete a peer entirely.  The peer may be running.
 */
static void
bgp_peer_delete __PF1(bnp, bgpPeer *)
{
    if (bnp->bgp_task != NULL) {
	/*
	 * If the peer has a task, bgp_peer_close() can do the work.
	 * Mark him deleted.
	 */
	if (bnp->bgp_state == BGPSTATE_OPENSENT
	  || bnp->bgp_state == BGPSTATE_OPENCONFIRM
	  || bnp->bgp_state == BGPSTATE_ESTABLISHED) {
	    bgp_send_notify_none(bnp, BGP_CEASE, 0);
	}
        BIT_SET(bnp->bgp_flags, BGPF_DELETE);
        bgp_peer_close(bnp, BGPEVENT_STOP);
    } else {
	/*
	 * The peer has no task.  This can occur when some dork uses
	 * a bgp no in the configuration file, followed by some
	 * BGP configuration.  Dump him.
	 */
	bgp_peer_remove(bnp);
	if (bnp->bgp_gateway != NULL) {
	    sockfree(bnp->bgp_gateway);
	    bnp->bgp_gateway = NULL;
	}
	if (bnp->bgp_myaddr != NULL) {
	    sockfree(bnp->bgp_myaddr);
	    bnp->bgp_myaddr = NULL;
	}
	if (bnp->bgp_lcladdr) {
	    ifae_free(bnp->bgp_lcladdr);
	    bnp->bgp_lcladdr = NULL;
	}
	if (bnp->bgp_addr != NULL) {
	    sockfree(bnp->bgp_addr);
	    bnp->bgp_addr = NULL;
	}
	if (bnp->bgp_trace_options) {
	    trace_freeup(bnp->bgp_trace_options);
	}
	bgp_peer_free(bnp);
    }
}


/*
 * bgp_group_delete - delete a whole group.  We call bgp_peer_delete()
 *   repeatedly to get rid of the peers.
 */
static void
bgp_group_delete __PF1(bgp, bgpPeerGroup *)
{
    while (bgp->bgpg_peers != NULL) {
	bgp_peer_delete(bgp->bgpg_peers);
    }

    if (bgp->bgpg_task != NULL) {
        task_delete(bgp->bgpg_task);
	bgp->bgpg_task = NULL;
    }

    /*
     * XXX Free adv_entry
     */
    if (bgp->bgpg_allow != NULL) {
	adv_free_list(bgp->bgpg_allow);
	bgp->bgpg_allow = NULL;
    }

    /*
     * Free up the gateway address if there is one
     */
    if (bgp->bgpg_gateway) {
	sockfree(bgp->bgpg_gateway);
	bgp->bgpg_gateway = (sockaddr_un *) 0;
    }

    /*
     * Free up the local address if there is one
     */
    if (bgp->bgpg_lcladdr) {
	ifae_free(bgp->bgpg_lcladdr);
	bgp->bgpg_lcladdr = (if_addr_entry *) 0;
    }

    /*
     * Free up the trace options, if any (there shouldn't be)
     */
    if (bgp->bgpg_trace_options) {
	trace_freeup(bgp->bgpg_trace_options);
    }

    /*
     * Remove the structure from the linked list and free it.
     */
    bgp_group_remove(bgp);
    bgp_group_free(bgp);
}



/*
 *	(Re)Configuration related routines
 */

/*
 * bgp_var_init - initialize default globals
 */
void
bgp_var_init()
{
    doing_bgp = FALSE;
    bgp_default_metric = BGP_METRIC_NONE;
    bgp_default_preference = RTPREF_BGP_EXT;
    bgp_default_preference2 = 0;
    bgp_n_ifa_policy_groups = 0;
}

/*
 * bgp_cleanup - this is called before the configuration file is to
 *   be reread.  Loop through marking all our groups and peers deleted,
 *   free all policy, then reset our globals.
 */
/*ARGSUSED*/
static void
bgp_cleanup(tp)
task *tp;
{
    bgpPeerGroup *bgp;

    BGP_GROUP_LIST(bgp) {
	bgpPeer *bnp;

	BIT_SET(bgp->bgpg_flags, BGPGF_DELETE);
	if (bgp->bgpg_task) {
	    trace_freeup(bgp->bgpg_task->task_trace);
	}
	trace_freeup(bgp->bgpg_trace_options);
	if (bgp->bgpg_allow) {
	    adv_free_list(bgp->bgpg_allow);
	    bgp->bgpg_allow = (adv_entry *) 0;
	}
	if (bgp->bgpg_ifap_policy) {
	    bgp_ifap_list *bgpl = bgp->bgpg_ifap_list;

	    assert(bgp->bgpg_type == BGPG_INTERNAL_RT);
	    adv_free_list(bgp->bgpg_ifap_policy);
	    bgp->bgpg_ifap_policy = (adv_entry *) 0;
	    while (bgpl) {
		bgp_ifap_list *bgpl_next = bgpl->bgp_if_next;
		IFA_FREE(bgpl->bgp_if_ifap);
		task_block_free(bgp_iflist_blk, (void_t) bgpl);
		bgpl = bgpl_next;
	    }
	    bgp->bgpg_ifap_list = (bgp_ifap_list *) 0;
	}

	BGP_PEER_LIST(bgp, bnp) {
	    BIT_SET(bnp->bgp_flags, BGPF_DELETE);
	    bnp->bgp_import = bnp->bgp_export = (adv_entry *) 0;
	    if (bnp->bgp_task) {
		trace_freeup(bnp->bgp_task->task_trace);
	    }
	    trace_freeup(bnp->bgp_trace_options);
	} BGP_PEER_LIST_END(bgp, bnp);

    } BGP_GROUP_LIST_END(bgp);

    adv_free_list(bgp_import_list);
    bgp_import_list = (adv_entry *) 0;
    adv_free_list(bgp_import_aspath_list);
    bgp_import_aspath_list = (adv_entry *) 0;
    adv_free_list(bgp_export_list);
    bgp_export_list = (adv_entry *) 0;
    bgp_var_init();
    
    trace_freeup(tp->task_trace);
    trace_freeup(bgp_default_trace_options);
}


/*
 * bgp_conf_check - final check of configuration
 */
int
bgp_conf_check(parse_error)
char *parse_error;
{
    if (!inet_autonomous_system) {
	(void) sprintf(parse_error, "autonomous-system not specified");
	return FALSE;
    }
    if (!bgp_n_groups) {
	(void) sprintf(parse_error, "no BGP groups specified");
	return FALSE;
    }

    return TRUE;
}


/*
 * bgp_conf_group_alloc - fetch a group structure for the parser
 */
bgpPeerGroup *
bgp_conf_group_alloc()
{
    bgpPeerGroup *bgp;

    /*
     * Get a structure and zero it
     */
    bgp = bgp_group_alloc();
    bzero((caddr_t)bgp, sizeof(bgpPeerGroup));

    /*
     * Set up a few of the options to reasonable defaults
     */
    bgp->bgpg_holdtime_out = BGP_HOLDTIME;
    bgp->bgpg_metric_out = bgp_default_metric;
    bgp->bgpg_local_as = inet_autonomous_system;
    bgp->bgpg_preference = bgp_default_preference;
    bgp->bgpg_preference2 = bgp_default_preference2;

    return (bgp);
}

/*
 * bgp_conf_peer_alloc - fetch a peer structure for the parser
 */
bgpPeer *
bgp_conf_peer_alloc(bgp)
bgpPeerGroup *bgp;
{
    return bgp_peer_create(bgp);
}


/*
 * bgp_conf_group_add - add a group to the list if there isn't something
 *   wrong with it.
 */
bgpPeerGroup *
bgp_conf_group_add(bgp, errstr)
bgpPeerGroup *bgp;
char *errstr;
{
    bgpPeerGroup *obgp = NULL;

    /*
     * See if the group is sensible at all
     */
    if (bgp->bgpg_type == BGPG_INTERNAL
      || bgp->bgpg_type == BGPG_INTERNAL_IGP
      || bgp->bgpg_type == BGPG_INTERNAL_RT) {
	if (!BIT_TEST(bgp->bgpg_options, BGPO_LOCAL_AS)) {
	    bgp->bgpg_local_as = bgp->bgpg_peer_as;
	} else if (bgp->bgpg_local_as != bgp->bgpg_peer_as) {
	    (void) sprintf(errstr,
  "the BGP local AS (%u) and peer AS (%u) must be the same for internal peers",
	      (u_int)bgp->bgpg_local_as,
	      (u_int)bgp->bgpg_peer_as);
	    return (bgpPeerGroup *)NULL;
	}

	if (bgp->bgpg_type == BGPG_INTERNAL_IGP
	  && BIT_TEST(bgp->bgpg_options, BGPGO_VERSION)
	  && bgp->bgpg_conf_version <= BGP_VERSION_2) {
	    (void) sprintf(errstr,
  "internal BGP peers associated with an IGP must run at version 3 or better");
	    return (bgpPeerGroup *)NULL;
	}
    } else if (bgp->bgpg_type == BGPG_EXTERNAL) {
	if (bgp->bgpg_local_as == bgp->bgpg_peer_as) {
	    (void) sprintf(errstr,
	      "external peer must not have the same AS as we do locally (%u)",
	      (u_int)bgp->bgpg_local_as);
	    return (bgpPeerGroup *)NULL;
	}
    }

    if (bgp->bgpg_holdtime_out
      && bgp->bgpg_holdtime_out < BGP_REAL_MIN_HOLDTIME) {
	(void) sprintf(errstr,
"the holdtime for BGP group peers (%d) is less than the minimum permitted (%d)",
	  bgp->bgpg_holdtime_out,
	  BGP_REAL_MIN_HOLDTIME);
	return (bgpPeerGroup *)NULL;
    }

    if (bgp->bgpg_type == BGPG_INTERNAL_RT) {
	if (!bgp->bgpg_proto) {
	    (void) sprintf(errstr,
		"the IGP protocol must be specified for routing peers using the proto option");
	    return (bgpPeerGroup *)NULL;
	}
	if (bgp->bgpg_ifap_policy) {
	    bgp_n_ifa_policy_groups++;
	}
    } else {
	if (bgp->bgpg_proto) {
	    (void) sprintf(errstr,
	      "the proto option may only be used for internal routing groups");
	    return (bgpPeerGroup *)NULL;
	}
	if (bgp->bgpg_ifap_policy) {
	    (void) sprintf(errstr,
		"interfaces may only be specified for internal routing groups");
	    return (bgpPeerGroup *)NULL;
	}
    }

    /*
     * Find out if the group is there already
     * XXX We may want to distinguish by more of this stuff.
     */
    if (bgp_groups != NULL) {
	BGP_GROUP_LIST(obgp) {
	    if (obgp->bgpg_local_as == bgp->bgpg_local_as
	      && obgp->bgpg_peer_as == bgp->bgpg_peer_as
	      && obgp->bgpg_type == bgp->bgpg_type) {
		break;
	    }
	} BGP_GROUP_LIST_END(obgp);
    }

    if (obgp == NULL) {
	/*
	 * This guy is unique, return him.
	 */
	bgp_group_add(bgp);
	return (bgp);
    } else if (!BIT_TEST(obgp->bgpg_flags, BGPGF_DELETE)) {
	(void) sprintf(errstr,
  "duplicate BGP group found, groups must differ by type and/or AS");
	return (bgpPeerGroup *)NULL;
    }

    /*
     * Tricky here.  What we do is transfer the options from
     * the new guy to the old guy and delete the new guy.  We'll
     * deal with option changes when we deal with peers.
     */
    if (obgp->bgpg_gateway) {
	sockfree(obgp->bgpg_gateway);
    }
    if (obgp->bgpg_lcladdr) {
	ifae_free(obgp->bgpg_lcladdr);
    }
    obgp->bgpg_conf = bgp->bgpg_conf;		/* Struct copy */
    bgp->bgpg_gateway = (sockaddr_un *) 0;
    bgp->bgpg_lcladdr = (if_addr_entry *) 0;
    if (obgp->bgpg_allow) {
	adv_free_list(obgp->bgpg_allow);
    }
    obgp->bgpg_allow = bgp->bgpg_allow;
    bgp->bgpg_allow = (adv_entry *) 0;
    obgp->bgpg_ifap_policy = bgp->bgpg_ifap_policy;
    bgp->bgpg_ifap_policy = (adv_entry *) 0;

    /*
     * That should be everything.  Unlink the old group and link it
     * to the end.  Then free the new group.
     */
    BIT_RESET(obgp->bgpg_flags, BGPGF_DELETE);
    bgp_group_remove(obgp);
    bgp_group_add(obgp);
    bgp_group_free(bgp);
    bgp_make_group_names(obgp);
    return (obgp);
}


/*
 * bgp_conf_group_check - see if this group is okay
 */
int
bgp_conf_group_check(bgp, errstr)
bgpPeerGroup *bgp;
char *errstr;
{
    if (bgp->bgpg_n_peers == 0 && bgp->bgpg_allow == NULL) {
	(void) sprintf(errstr,
    "BGP group has neither configured peers or an allow list: invalid configuration");
	return FALSE;
    }
    return TRUE;
}


/*
 * bgp_conf_peer_add - add a peer to a group
 */
bgpPeer *
bgp_conf_peer_add(bgp, bnp, errstr)
bgpPeerGroup *bgp;
bgpPeer *bnp;
char *errstr;
{
    bgpPeer *obnp;

    /*
     * Make sure this peer is vaguely sensible
     */
    if (bnp->bgp_conf_local_as != bgp->bgpg_local_as) {
	(void) sprintf(errstr,
		       "the local AS number (%u) used for BGP peer %A must be that of the group (%u)",
		       (u_int) bnp->bgp_conf_local_as,
		       bnp->bgp_addr,
		       (u_int) bgp->bgpg_local_as);
	return (bgpPeer *)NULL;
    }

    if (bgp->bgpg_type == BGPG_INTERNAL_IGP
	&& BIT_TEST(bnp->bgp_options, BGPO_VERSION)
	&& bnp->bgp_conf_version <= BGP_VERSION_2) {
	(void) sprintf(errstr,
		       "internal BGP peer %A associated with an IGP must run at version 3 or better",
		       bnp->bgp_addr);
	return (bgpPeer *)NULL;
    }

    if (bnp->bgp_holdtime_out
      && bnp->bgp_holdtime_out < BGP_REAL_MIN_HOLDTIME) {
	(void) sprintf(errstr,
		       "the holdtime for BGP peer %A (%d) is less than the minimum permitted (%d)",
		       bnp->bgp_addr,
		       bnp->bgp_holdtime_out,
		       BGP_REAL_MIN_HOLDTIME);
	return (bgpPeer *) 0;
    }

    if (bgp->bgpg_type == BGPG_INTERNAL
      || bgp->bgpg_type == BGPG_INTERNAL_RT
      || bgp->bgpg_type == BGPG_INTERNAL_IGP) {
	if (bnp->bgp_metric_out != bgp->bgpg_metric_out) {
	    (void) sprintf(errstr,
			   "the metricout option should be used in the group declaration only");
	    return (bgpPeer *) 0;
	}
	if (bnp->bgp_lcladdr != bgp->bgpg_lcladdr) {
	    (void) sprintf(errstr,
			   "the lcladdr option should be used in the group declaration only");
	    return (bgpPeer *) 0;
	}
	if (bnp->bgp_rto_time != bgp->bgpg_rto_time) {
	    (void) sprintf(errstr,
			   "the outdelay option should be used in the group declaration only");
	    return (bgpPeer *) 0;
	}
    }

    /*
     * Search through looking for a peer with the same address.  If
     * we find one, check him out.
     */
    if (bgp->bgpg_peers == NULL) {
	obnp = NULL;
    } else {
	BGP_PEER_LIST(bgp, obnp) {
	    if (sockaddrcmp_in(bnp->bgp_addr, obnp->bgp_addr)) {
		break;
	    }
	} BGP_PEER_LIST_END(bgp, obnp);
    }

    if (obnp == NULL) {
	/*
	 * Unique peer.  Add him to the group and forget about it.
	 */
	bgp_peer_add(bgp, bnp);
	return (bnp);
    } else if (!BIT_TEST(obnp->bgp_flags, BGPF_DELETE)) {
	(void) sprintf(errstr,
		       "duplicate entries for peer %A found in group type %s AS %u",
		       bnp->bgp_addr,
		       trace_state(bgp_group_bits, bgp->bgpg_type),
		       (u_int) bgp->bgpg_peer_as);
	return (bgpPeer *)NULL;
    }

    /*
     * Here we've got a deleted duplicate.  If there have been any
     * significant changes to it leave it deleted, otherwise copy
     * the insignificant changes over.
     */
    if (obnp->bgp_lcladdr != bnp->bgp_lcladdr
	|| obnp->bgp_conf_version != bnp->bgp_conf_version
	|| obnp->bgp_holdtime_out != bnp->bgp_holdtime_out
	|| obnp->bgp_conf_local_as != bnp->bgp_conf_local_as
	|| obnp->bgp_peer_as != bgp->bgpg_peer_as
	|| BIT_TEST(obnp->bgp_options, BGPO_KEEPNONE)
        || (BIT_TEST(bnp->bgp_options, BGPO_KEEPALL)
            && !BIT_TEST(obnp->bgp_options, BGPO_KEEPALL))
        || (BIT_TEST(obnp->bgp_options, BGPO_IGNOREFIRSTASHOP) !=
            BIT_TEST(bnp->bgp_options, BGPO_IGNOREFIRSTASHOP))
        || (BIT_TEST(obnp->bgp_options, BGPO_MED) !=
            BIT_TEST(bnp->bgp_options, BGPO_MED))
	|| (obnp->bgp_gateway == NULL && bnp->bgp_gateway != NULL)
	|| (obnp->bgp_gateway != NULL && bnp->bgp_gateway == NULL)
	|| (obnp->bgp_gateway != NULL && !sockaddrcmp_in(bnp->bgp_gateway, obnp->bgp_gateway))) {
	bgp_peer_add(bgp, bnp);
	return (bnp);
    }

    /*
     * If the peer has a socket open, check his receive and send buffer
     * settings.  If they've changed we'll need to replace him.  If
     * he has more data spooled on his socket than permitted by the
     * new peer's configuration, replace him too.
     */
    if (obnp->bgp_state != BGPSTATE_IDLE && obnp->bgp_state !=BGPSTATE_ACTIVE) {
	if (BGP_RX_BUFSIZE(obnp->bgp_recv_bufsize)
	    != BGP_RX_BUFSIZE(bnp->bgp_recv_bufsize)
	  || BGP_RX_BUFSIZE(obnp->bgp_send_bufsize)
	    != BGP_RX_BUFSIZE(bnp->bgp_send_bufsize)) {
	    bgp_peer_add(bgp, bnp);
	    return (bnp);
	}
    }

    /*
     * Check out the interface.  This only matters if the guy is
     * up and running, unless the fellow is in connect.  This code
     * is funky
     */
    if (bnp->bgp_ifap != NULL) {
	if (obnp->bgp_lcladdr != NULL) {
	    if (!sockaddrcmp_in(bnp->bgp_ifap->ifa_addr_local,
				obnp->bgp_lcladdr->ifae_addr)) {
		bgp_peer_add(bgp, bnp);
		return (bnp);
	    }
	} else if (obnp->bgp_state == BGPSTATE_CONNECT
		   && (obnp->bgp_ifap == NULL
		       || !sockaddrcmp_in(obnp->bgp_ifap->ifa_addr_local,
					  bnp->bgp_ifap->ifa_addr_local))) {
	    bgp_peer_add(bgp, bnp);
	    return (bnp);
	}
    }

    /*
     * Here we've got changes we can live with.  Copy them over
     * into the old peer and return him.
     */
    if (obnp->bgp_gateway != NULL) {
	sockfree(obnp->bgp_gateway);
    }
    if (obnp->bgp_lcladdr != NULL) {
	ifae_free(obnp->bgp_lcladdr);
    }
    obnp->bgp_conf = bnp->bgp_conf;		/* Struct copy */
    bnp->bgp_gateway = NULL;
    bnp->bgp_lcladdr = NULL;
    if (bnp->bgp_addr) {
	sockfree(bnp->bgp_addr);
	bnp->bgp_addr = (sockaddr_un *) 0;
    }
    if (bnp->bgp_ifap) {
	IFA_FREE(bnp->bgp_ifap);
	bnp->bgp_ifap = (if_addr *) 0;
    }
    bgp_peer_free(bnp);
    BIT_RESET(obnp->bgp_flags, BGPF_UNCONFIGURED|BGPF_DELETE);
    bgp_peer_remove(obnp);
    bgp_peer_add(bgp, obnp);
    bgp_make_names(obnp);

    return (obnp);
}


/*
 * bgp_init - start up bgp
 */
void
bgp_init()
{
    bgpPeerGroup *bgp;
    bgpPeer *bnp;

    if (!doing_bgp) {
	/*
	 * Get rid of any groups which might be present
	 */
	while (bgp_groups != NULL) {
	    bgp_group_delete(bgp_groups);
	}

	/*
	 * If we have policy, free it.
	 */
 	if (bgp_import_list) {
 	    adv_free_list(bgp_import_list);
 	    bgp_import_list = (adv_entry *) 0;
 	}
 	if (bgp_import_aspath_list) {
 	    adv_free_list(bgp_import_aspath_list);
 	    bgp_import_aspath_list = (adv_entry *) 0;
 	}
 	if (bgp_export_list) {
 	    adv_free_list(bgp_export_list);
 	    bgp_export_list = (adv_entry *) 0;
 	}
 
	/*
	 * If BGP has been running before there will be a load of crap
	 * which needs to be taken down.  Do so now.
	 */
	if (bgp_listen_task) {
#ifdef	PROTO_SNMP
	    bgp_init_mib(FALSE);
#endif	/* PROTO_SNMP */
	    bgp_pp_delete_all();
	    bgp_listen_stop();

	    bgp_peer_free_all();	/* free malloc()'d peer structures */
	    bgp_group_free_all();	/* free malloc()'d group structures */
	    bgp_outbuf_free_all();	/* free remaining output buffer */
	}
    } else {
	/*
	 * Determine the well-known port to use if we don't know it
	 */
	if (bgp_port == 0) {
	    bgp_port = task_get_port(bgp_default_trace_options,
				     "bgp", "tcp",
				     htons(BGP_PORT));
	}

	trace_inherit_global(bgp_default_trace_options, bgp_trace_types, (flag_t) 0);
	/*
	 * Initialize the listen task if we haven't got that, and make
	 * sure the send buffer is big enough.
	 */
	if (bgp_listen_task == NULL) {
	    bgp_listen_init();
#ifdef	PROTO_SNMP
	    bgp_init_mib(TRUE);
#endif	/* PROTO_SNMP */
	    task_alloc_send(bgp_listen_task, BGPMAXV4SENDPKTSIZE);
	} else {
	    bgpPeerGroup *bgpnext;

	    /*
	     * BGP was running before and this is just a change.  Go
	     * around deleting groups which are gone.  Careful about
	     * groups disappearing from under us.
	     */
	    for (bgp = bgp_groups; bgp != NULL; bgp = bgpnext) {
		bgpnext = bgp->bgpg_next;
		if (BIT_TEST(bgp->bgpg_flags, BGPGF_DELETE)) {
		    bgp_group_delete(bgp);
		}
	    }

	    /*
	     * Go through all the peers now, looking for deleted guys.
	     * Any one who is deleted is checked to see if they would
	     * qualify as an unconfigured peer.  If so they are marked
	     * that way, if not they are dumped.
	     */
	    BGP_GROUP_LIST(bgp) {
		bgpPeer *bnpnext;

		for (bnp = bgp->bgpg_peers; bnp != NULL; bnp = bnpnext) {
		    bnpnext = bnp->bgp_next;
		    if (BIT_TEST(bnp->bgp_flags, BGPF_DELETE)) {
	      		if ((bnp->bgp_state == BGPSTATE_OPENSENT
			    || bnp->bgp_state == BGPSTATE_OPENCONFIRM
			    || bnp->bgp_state == BGPSTATE_ESTABLISHED)
			  && bgp->bgpg_allow != NULL
			  && adv_destmask_match(bgp->bgpg_allow,
						bnp->bgp_addr,
						inet_mask_host)) {
			    bgpPeer *bnpchk;
			    int terminate_him = 0;
			    /*
			     * We're still not in the clear here.  Check
			     * to see if there is already a configured
			     * peer in the list with the same address.
			     * If so, terminate this guy anyway.
			     */
			    if (BIT_TEST(bnp->bgp_options, BGPO_KEEPNONE)) {
				terminate_him = 1;
			    } else {
				for (bnpchk = bgp->bgpg_peers;
				     bnpchk != bnp;
				     bnpchk = bnpchk->bgp_next) {
				    if (sockaddrcmp_in(bnpchk->bgp_addr,
						       bnp->bgp_addr)) {
					terminate_him = 1;
					break;
				    }
				}
			    }

			    /*
			     * And we're *still* not in the clear here.  As
			     * an unconfigured peer he would inherit options
			     * from the group.  See if his current options
			     * match.
			     */
			    if (!terminate_him &&
				(bnp->bgp_metric_out != bgp->bgpg_metric_out
			      || bnp->bgp_conf_version != bgp->bgpg_conf_version
			      || bnp->bgp_lcladdr != bgp->bgpg_lcladdr
			      || (bnp->bgp_conf_local_as != bgp->bgpg_local_as
				&& bnp->bgp_local_as != bgp->bgpg_local_as)
			      || bnp->bgp_holdtime_out
				!= bgp->bgpg_holdtime_out
			      || bnp->bgp_peer_as != bgp->bgpg_peer_as
			      || (!BIT_TEST(bnp->bgp_options, BGPO_KEEPALL)
				&& BIT_TEST(bgp->bgpg_options, BGPO_KEEPALL))
			      || (bnp->bgp_gateway == NULL
				&& bgp->bgpg_gateway != NULL)
			      || (bnp->bgp_gateway != NULL
				&& bgp->bgpg_gateway == NULL)
			      || (bnp->bgp_gateway != NULL
				&& !sockaddrcmp_in(bnp->bgp_gateway,
				  bgp->bgpg_gateway))
			      || BGP_RX_BUFSIZE(bnp->bgp_recv_bufsize)
				!= BGP_RX_BUFSIZE(bgp->bgpg_recv_bufsize)
			      || BGP_RX_BUFSIZE(bnp->bgp_send_bufsize)
				!= BGP_RX_BUFSIZE(bgp->bgpg_send_bufsize))) {
				terminate_him = 1;
			    }

			    if (terminate_him) {
				bgp_peer_delete(bnp);
			    } else {
				BIT_RESET(bnp->bgp_flags, BGPF_DELETE);
				BIT_SET(bnp->bgp_flags, BGPF_UNCONFIGURED);
				bgp_peer_remove(bnp);
				bgp_peer_add(bgp, bnp);
    				bgp_make_names(bnp);
			    }
			} else {
			    bgp_peer_delete(bnp);
			}
		    }
		}
	    } BGP_GROUP_LIST_END(bgp);

	    /*
	     * Free all malloc'd peer and group structures so we start
	     * clean.
	     */
	    bgp_peer_free_all();
	    bgp_group_free_all();
	}

	/*
	 * The listen task inherits its options from the default.
	 */
	trace_set(bgp_listen_task->task_trace, bgp_default_trace_options);

	/*
	 * Scan the group list looking for guys who have no tasks.
	 * Create them for anyone who needs them.
	 */
	BGP_GROUP_LIST(bgp) {
	    trace_inherit(bgp->bgpg_trace_options,
			  bgp_default_trace_options);
	    if (bgp->bgpg_task == NULL) {
		bgp_group_init(bgp);
	    } else {
		trace_set(bgp->bgpg_task->task_trace,
			  bgp->bgpg_trace_options);
	    }
	    bgp->bgpg_asbit = (u_short) aslocal_bit(bgp->bgpg_local_as);
	} BGP_GROUP_LIST_END(bgp);

	/*
	 * Now go around looking for peers with no tasks.  Initialize these
	 * guys also.  In addition, find the policy for everyone who needs
	 * it.
	 */
	BGP_GROUP_LIST(bgp) {
	    BGP_PEER_LIST(bgp, bnp) {
		/* Inherit tracing while we're at this */
		trace_inherit(bnp->bgp_trace_options,
			      bgp->bgpg_trace_options);
		if (bnp->bgp_task == NULL) {
		    bgp_peer_init(bnp);
		} else {
		    trace_set(bnp->bgp_task->task_trace,
			      bgp->bgpg_trace_options);
		}

		if (BIT_TEST(bnp->bgp_options, BGPO_TTL)
		  && bnp->bgp_ttl_current
		  && bnp->bgp_ttl_current != bnp->bgp_ttl) {
		    (void) task_set_option(bnp->bgp_task,
					   TASKOPTION_TTL,
					   bnp->bgp_ttl);
		    bnp->bgp_ttl_current = bnp->bgp_ttl;
		}

		/* XXX this policy stuff isn't quite right.  Think about it */
		switch (bgp->bgpg_type) {
		case BGPG_EXTERNAL:
		case BGPG_INTERNAL:
		case BGPG_INTERNAL_RT:
		    bnp->bgp_import = control_exterior_locate(bgp_import_list, bnp->bgp_peer_as);
		    bnp->bgp_export = control_exterior_locate(bgp_export_list, bnp->bgp_peer_as);
		    break;
		default:
		    break;
		}
	    } BGP_PEER_LIST_END(bgp, bnp);
	} BGP_GROUP_LIST_END(bgp);
    }
}


/*
 *	Dump routines of a variety of flavours
 */

/*
 * bgp_dump - dump BGP's global state
 */
static void
bgp_dump(tp, fd)
task *tp;
FILE *fd;
{
    /*
     * Not much to dump here.  Print out the counts of things we're
     * keeping track of.
     */
    (void) fprintf(fd,
 "\tGroups: %d\tPeers: %d (%d configured)\tActive Incoming: %d\n",
      bgp_n_groups,
      bgp_n_peers,
      bgp_n_peers - bgp_n_unconfigured,
      bgp_n_protopeers);
    (void) fprintf(fd,
      "\tFree Peers: %d\tFree Groups: %d\t\tState: %s\n",
      bgp_n_free,
      bgp_n_groups_free,
      ((tp->task_socket == (-1)) ? "Initializing" : "Listening"));

    if (bgp_import_list) {
	control_exterior_dump(fd, 1, control_import_dump, bgp_import_list);
    }
    if (bgp_import_aspath_list) {
	/* XXX */
	control_exterior_dump(fd, 1, control_import_dump, bgp_import_aspath_list);
    }
    if (bgp_export_list) {
	control_exterior_dump(fd, 1, control_export_dump, bgp_export_list);
    }

}


/*
 * bgp_group_dump - dump the state of a peer group
 */
static void
bgp_group_dump(tp, fd)
task *tp;
FILE *fd;
{
    bgpPeerGroup *bgp = (bgpPeerGroup *)tp->task_data;

    (void) fprintf(fd,
      "\tGroup Type: %s\tAS: %u\tLocal AS: %u\tFlags: <%s>\n",
      trace_state(bgp_group_bits, bgp->bgpg_type),
      (u_int)bgp->bgpg_peer_as,
      (u_int)bgp->bgpg_local_as,
      trace_bits(bgp_group_flag_bits, bgp->bgpg_flags));

    (void) fprintf(fd,
      "\tTotal Peers: %d\t\tEstablished Peers: %d\n",
      bgp->bgpg_n_peers,
      bgp->bgpg_n_established);

    if (bgp->bgpg_type != BGPG_EXTERNAL) {
        int wroteit = 0;

	if (bgp->bgpg_n_established > 0) {
	    (void) fprintf(fd, "\tRouting Bit: %u", tp->task_rtbit);
	    wroteit = 1;
	}
	if (bgp->bgpg_igp_proto != 0) {
	    (void) fprintf(fd, "\tIGP Proto: %s\tIGP Routing Bit: %u\n",
	      trace_state(rt_proto_bits, bgp->bgpg_igp_proto),
	      bgp->bgpg_igp_rtbit);
	} else if (wroteit) {
	    (void) fprintf(fd, "\n");
	}
    }
    if (bgp->bgpg_allow != NULL) {
	(void) fprintf(fd, "\tAllowed Unconfigured Peer Addresses:\n");
	control_dmlist_dump(fd, 2, bgp->bgpg_allow,
	  (adv_entry *)0, (adv_entry *)0);
    }

    if (bgp->bgpg_type != BGPG_EXTERNAL && bgp->bgpg_n_established > 0) {
	(void) fprintf(fd, "\tRoute Queue Timer: ");
	if (bgp->bgpg_rto_next_time == (time_t) 0) {
	    if (BIT_TEST(bgp->bgpg_flags, BGPGF_RT_TIMER)) {
		(void) fprintf(fd, "running but ");
	    }
	    (void) fprintf(fd, "unset\t");
	} else if ((bgp->bgpg_rto_next_time + bgp->bgpg_rto_time) >= bgp_time_sec) {
	    register time_t tmp;

	    tmp = bgp->bgpg_rto_next_time + bgp->bgpg_rto_time - bgp_time_sec;
	    (void) fprintf(fd, "%u second%s%s\t",
			   tmp,
			   ((tmp == 1) ? "" : "s"),
			   (BIT_TEST(bgp->bgpg_flags, BGPGF_RT_TIMER)
			     ? "" : " not running!"));
	} else {
	    register time_t tmp;

	    tmp = bgp_time_sec - bgp->bgpg_rto_next_time - bgp->bgpg_rto_time;
	    (void) fprintf(fd, "%u second%s late%s\t",
			   tmp,
			   ((tmp == 1) ? "" : "s"),
			   (BIT_TEST(bgp->bgpg_flags, BGPGF_RT_TIMER)
			     ? "!" : " not running!"));
	}
	if (BGP_RTQ_EMPTY(&(bgp->bgpg_asp_queue))) {
	    (void) fprintf(fd, "Route Queue: empty\n");
	} else {
	    register bgp_asp_list *aspl;
	    
	    (void) fprintf(fd, "Route Queue: active\n");

	    aspl = BGP_ASPL_FIRST(&(bgp->bgpg_asp_queue));
	    while (aspl) {
		register bgpg_rto_entry *bgrto;

		if (aspl->bgpl_asp) {
		    aspath_dump(fd, aspl->bgpl_asp, "\tQueued ", "\n");
		} else {
		    (void) fprintf(fd, "\tDeleted routes:\n");
		}

		for (bgrto = aspl->bgpl_grto_next;
		     bgrto != (bgpg_rto_entry *) aspl;
		     bgrto = bgrto->bgpgo_next) {
		    register time_t tmp = bgrto->bgpgo_time + bgp->bgpg_rto_time;

		    (void) fprintf(fd, "\t    %A/%d ",
				 bgrto->bgpgo_rt->rt_dest,
				 inet_prefix_mask(bgrto->bgpgo_rt->rt_dest_mask));
		    if (bgrto->bgpgo_time == (time_t) 0) {
			(void) fprintf(fd, "immediately\n");
		    } else if (tmp == bgp_time_sec) {
			(void) fprintf(fd, "now\n");
		    } else if (tmp > bgp_time_sec) {
			(void) fprintf(fd, "in %u second%s\n",
				       (tmp - bgp_time_sec),
				       (((tmp-bgp_time_sec) == 1) ? "" : "s"));
		    } else {
			(void) fprintf(fd, "%u second%s late!!\n",
				       (bgp_time_sec - tmp),
				       (((bgp_time_sec-tmp) == 1) ? "" : "s"));
		    }
		}
		aspl = BGP_ASPL_NEXT(&(bgp->bgpg_asp_queue), aspl);
	    }
	}
    }
}


/*
 * bgp_peer_dump - dump info about a BGP peer
 */
static void
bgp_peer_dump(tp, fd)
task *tp;
FILE *fd;
{
    bgpPeer *bnp = (bgpPeer *)tp->task_data;
    int printed;

    (void) fprintf(fd, "\tPeer: %#A\tLocal: ", bnp->bgp_addr);
    if (bnp->bgp_myaddr != NULL) {
	(void) fprintf(fd, "%#A", bnp->bgp_myaddr);
    } else if (bnp->bgp_ifap != NULL) {
	(void) fprintf(fd, "%A", bnp->bgp_ifap->ifa_addr_local);
    } else if (bnp->bgp_lcladdr != NULL) {
	(void) fprintf(fd, "%A", bnp->bgp_lcladdr->ifae_addr);
    } else {
	(void) fprintf(fd, "unspecified");
    }
    (void) fprintf(fd, "\tType: %s\n",
		   trace_state(bgp_group_bits, bnp->bgp_type));
    
    (void) fprintf(fd, "\tState: %s\tFlags: <%s>\n",
		   trace_state(bgp_state_bits, bnp->bgp_state),
		   trace_bits(bgp_flag_bits, bnp->bgp_flags));

    (void) fprintf(fd,
		   "\tLast State: %s\tLast Event: %s\tLast Error: %s\n",
		   trace_state(bgp_state_bits, bnp->bgp_laststate),
		   trace_state(bgp_event_bits, bnp->bgp_lastevent),
		   trace_state(bgp_error_bits, bnp->bgp_last_code));

    (void) fprintf(fd, "\tOptions: <%s>\n",
		   trace_bits(bgp_option_bits, bnp->bgp_options));
    printed = 0;
    if (BIT_TEST(bnp->bgp_options, BGPO_VERSION)) {
	(void) fprintf(fd, "\t\tVersion: %d", bnp->bgp_conf_version);
	printed++;
    }
    if (BIT_TEST(bnp->bgp_options, BGPO_GATEWAY)) {
	if (printed == 0) {
	    (void) fprintf(fd, "\t");
	}
	(void) fprintf(fd, "\tGateway: %A", bnp->bgp_gateway);
	printed++;
    }
    if (BIT_TEST(bnp->bgp_options, BGPO_LCLADDR)) {
	if (printed == 0) {
	    (void) fprintf(fd, "\t");
	}
	(void) fprintf(fd, "\tLocal Address: %A",
		       bnp->bgp_lcladdr->ifae_addr);
	printed++;
    }
    if (BIT_TEST(bnp->bgp_options, BGPO_HOLDTIME)) {
	if (printed == 3) {
	    (void) fprintf(fd, "\n\t");
	    printed = 0;
	} else if (printed == 0) {
	    (void) fprintf(fd, "\t");
	}
	(void) fprintf(fd, "\tHoldtime: %d", bnp->bgp_holdtime_out);
	printed++;
    }
    if (BIT_TEST(bnp->bgp_options, BGPO_METRIC_OUT)) {
	if (printed == 3) {
	    (void) fprintf(fd, "\n\t");
	    printed = 0;
	} else if (printed == 0) {
	    (void) fprintf(fd, "\t");
	}
	(void) fprintf(fd, "\tMetric Out: %u", (u_int)bnp->bgp_metric_out);
	printed++;
    }
    if (BIT_TEST(bnp->bgp_options, BGPO_PREFERENCE)) {
	if (printed == 3) {
	    (void) fprintf(fd, "\n\t");
	    printed = 0;
	} else if (printed == 0) {
	    (void) fprintf(fd, "\t");
	}
	(void) fprintf(fd, "\tPreference: %u", (u_int)bnp->bgp_preference);
	printed++;
    }
    if (BIT_TEST(bnp->bgp_options, BGPO_PREFERENCE2)) {
	if (printed == 3) {
	    (void) fprintf(fd, "\n\t");
	    printed = 0;
	} else if (printed == 0) {
	    (void) fprintf(fd, "\t");
	}
	(void) fprintf(fd, "\tPreference2: %u", (u_int)bnp->bgp_preference2);
	printed++;
    }
    if (bnp->bgp_recv_bufsize != 0) {
	if (printed == 3) {
	    (void) fprintf(fd, "\n\t");
	    printed = 0;
	} else if (printed == 0) {
	    (void) fprintf(fd, "\t");
	}
	(void) fprintf(fd, "\tReceive Bufsize: %u", bnp->bgp_recv_bufsize);
	printed++;
    }
    if (bnp->bgp_send_bufsize != 0) {
	if (printed == 3) {
	    (void) fprintf(fd, "\n\t");
	    printed = 0;
	} else if (printed == 0) {
	    (void) fprintf(fd, "\t");
	}
	(void) fprintf(fd, "\tSend Bufsize: %u", bnp->bgp_send_bufsize);
	printed++;
    }
    if (bnp->bgp_rti_time != 0) {
	if (printed == 3) {
	    (void) fprintf(fd, "\n\t");
	    printed = 0;
	} else if (printed == 0) {
	    (void) fprintf(fd, "\t");
	}
	(void) fprintf(fd, "\tInbound Timer: %u", bnp->bgp_rti_time);
	printed++;
    }
    if (bnp->bgp_group->bgpg_type == BGPG_EXTERNAL && bnp->bgp_rto_time != 0) {
	if (printed == 3) {
	    (void) fprintf(fd, "\n\t");
	    printed = 0;
	} else if (printed == 0) {
	    (void) fprintf(fd, "\t");
	}
	(void) fprintf(fd, "\tOutbound Timer: %u", bnp->bgp_rto_time);
	printed++;
    }

    if (printed) {
	(void) fprintf(fd, "\n");
    }

    if (bnp->bgp_state != BGPSTATE_IDLE
	&& bnp->bgp_state != BGPSTATE_CONNECT
	&& bnp->bgp_state != BGPSTATE_ACTIVE) {
	if (bnp->bgp_state != BGPSTATE_OPENSENT) {
	    (void) fprintf(fd, "\tPeer Version: %d\tPeer ID: %A\t",
			   bnp->bgp_version,
			   sockbuild_in(0, bnp->bgp_id));
	    (void) fprintf(fd, "Local ID: %A\tActive Holdtime: %d\n",
			   sockbuild_in(0, bnp->bgp_out_id),
			   bnp->bgp_holdtime);
	}
	if (bnp->bgp_state == BGPSTATE_ESTABLISHED) {
	    if (bnp->bgp_group->bgpg_type != BGPG_EXTERNAL) {
		register bgp_bits *sbits;

		if (bnp->bgp_version == BGP_VERSION_4) {
		    sbits = BGPG_GETBITS(bnp->bgp_group->bgpg_v4_sync,
					 bnp->bgp_group->bgpg_idx_size);
		} else {
		    sbits = BGPG_GETBITS(bnp->bgp_group->bgpg_v3_sync,
					 bnp->bgp_group->bgpg_idx_size);
		}
		(void) fprintf(fd, "\tGroup Bit: %u\tSend state: %sin sync\n",
			       bnp->bgp_group_bit,
			       (BGPB_BTEST(sbits, bnp->bgp_group_bit) ? "" : "not "));
	    }
	}
	(void) fprintf(fd,
		       "\tLast traffic (seconds):\tReceived %u\tSent %u\tChecked %u\n",
		       bgp_time_sec - bnp->bgp_last_rcvd,
		       bgp_time_sec - bnp->bgp_last_sent,
		       bgp_time_sec - bnp->bgp_last_checked);
	(void) fprintf(fd,
		       "\tInput messages:\tTotal %lu\tUpdates %lu\tOctets %lu\n",
		       bnp->bgp_in_updates + bnp->bgp_in_notupdates,
		       bnp->bgp_in_updates,
		       bnp->bgp_in_octets);
	(void) fprintf(fd,
		       "\tOutput messages:\tTotal %lu\tUpdates %lu\tOctets %lu\n",
		       bnp->bgp_out_updates + bnp->bgp_out_notupdates,
		       bnp->bgp_out_updates,
		       bnp->bgp_out_octets);
    }
    printed = 0;
    if (bnp->bgp_bufpos != bnp->bgp_readptr) {
	(void) fprintf(fd, "\tReceived and buffered octets: %u",
		       bnp->bgp_readptr - bnp->bgp_bufpos);
	printed++;
    }
    if (bnp->bgp_outbuf != NULL) {
	(void) fprintf(fd, "\tWrite buffered octets: %u\n",
		       (bnp->bgp_outbuf->bgpob_end
		        - bnp->bgp_outbuf->bgpob_start));
	printed = 0;
    }
    if (printed) {
	(void) fprintf(fd, "\n");
    }

    if (bnp->bgp_group->bgpg_type == BGPG_EXTERNAL
      && bnp->bgp_state == BGPSTATE_ESTABLISHED) {
	(void) fprintf(fd, "\tRoute Queue Timer: ");
	if (bnp->bgp_rto_next_time == (time_t) 0) {
	    if (BIT_TEST(bnp->bgp_flags, BGPF_RT_TIMER)) {
		(void) fprintf(fd, "running, ");
	    }
	    (void) fprintf(fd, "unset\t");
	} else if ((bnp->bgp_rto_next_time + bnp->bgp_rto_time) >= bgp_time_sec) {
	    register time_t tmp;

	    tmp = bnp->bgp_rto_next_time + bnp->bgp_rto_time - bgp_time_sec;
	    (void) fprintf(fd, "%u second%s%s\t",
			   tmp,
			   ((tmp == 1) ? "" : "s"),
			   (BIT_TEST(bnp->bgp_flags, BGPF_RT_TIMER)
			     ? "" : " not running!"));
	} else {
	    register time_t tmp;

	    tmp = bgp_time_sec - bnp->bgp_rto_next_time - bnp->bgp_rto_time;
	    (void) fprintf(fd, "%u second%s late%s\t",
			   tmp,
			   ((tmp == 1) ? "" : "s"),
			   (BIT_TEST(bnp->bgp_flags, BGPF_RT_TIMER)
			     ? "!" : " not running!"));
	}
	if (BGP_RTQ_EMPTY(&(bnp->bgp_asp_queue))) {
	    (void) fprintf(fd, "Route Queue: empty\n");
	} else {
	    register bgp_asp_list *aspl;
	    
	    (void) fprintf(fd, "Route Queue: active\n");

	    aspl = BGP_ASPL_FIRST(&(bnp->bgp_asp_queue));
	    while (aspl) {
		register bgp_rto_entry *brto;

		if (aspl->bgpl_asp) {
		    aspath_dump(fd, aspl->bgpl_asp, "\tQueued ", "\n");
		} else {
		    (void) fprintf(fd, "\tDeleted routes:\n");
		}

		for (brto = aspl->bgpl_rto_next;
		     brto != (bgp_rto_entry *) aspl;
		     brto = brto->bgpo_next) {
		    register time_t tmp = brto->bgpo_time + bnp->bgp_rto_time;

		    (void) fprintf(fd, "\t    %A/%d ",
				 brto->bgpo_rt->rt_dest,
				 inet_prefix_mask(brto->bgpo_rt->rt_dest_mask));
		    if (brto->bgpo_time == (time_t) 0) {
			(void) fprintf(fd, "immediately\n");
		    } else if (tmp == bgp_time_sec) {
			(void) fprintf(fd, "now\n");
		    } else if (tmp > bgp_time_sec) {
			(void) fprintf(fd, "in %u second%s\n",
				       (tmp - bgp_time_sec),
				       (((tmp-bgp_time_sec) == 1) ? "" : "s"));
		    } else {
			(void) fprintf(fd, "%u second%s late!!\n",
				       (bgp_time_sec - tmp),
				       (((bgp_time_sec-tmp) == 1) ? "" : "s"));
		    }
		}
		aspl = BGP_ASPL_NEXT(&(bnp->bgp_asp_queue), aspl);
	    }
	}
    }
}
