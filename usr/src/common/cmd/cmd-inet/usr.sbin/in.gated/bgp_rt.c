#ident	"@(#)bgp_rt.c	1.4"
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

/*
 * There are basically six entry points in here, bgp_recv_update(),
 * bgp_rt_terminate(), bgp_rt_send_init(), bgp_flash(), bgp_aux_flash()
 * and bgp_rt_reinit().
 *
 * Oh, I forgot bgp_newpolicy(), and bgp_aux_newpolicy().  That
 * makes eight.
 *
 * And new for version 4 are bgp_rt_peer_timer(), bgp_rt_group_timer(),
 * bgp_rt_send_ready() and bgp_rt_init().  That makes twelve.
 *
 * And now bgp_flash() has been split into bgp_peer_flash() and
 * bgp_group_flash().  And bgp_newpolicy() has been split into
 * bgp_peer_newpolicy() and bgp_group_newpolicy().  That makes
 * 14.
 *
 * bgp_rt_init() is called from bgp_init() when BGP is first started.
 * It initializes common data required for BGP routing.
 *
 * bgp_recv_v2or3_update() is called when there is data to be read from a
 * version 2/3 neighbour's socket (it is the neighbour-task's "read"
 * routine when the neighbour is in established state).  It reads data
 * from the socket into an input buffer in fairly large gulps, processes
 * all complete packets contained in the input buffer, and then goes
 * around for more data.  bgp_v4_recv_update() does the same for version
 * 4 peers.
 *
 * bgp_rt_terminate() is called when a connection to a peer has been
 * terminated or otherwise detected down.  It deletes all routing information
 * received from the terminated peer, resets the peer/group routing bits
 * as appropriate and frees up incoming and outgoing queue information
 * associated with the peer.
 *
 * bgp_rt_send_init() is called when a new neighbour session is
 * established.  It queues a full set of routing announcements to synchronize
 * the state of the neighbour router with our current condition and sets
 * up a send routine to send this later.  This is used when the KEEPALIVE
 * message which ack's our open to the neighbour has been successfully
 * received.
 *
 * bgp_peer_flash() is called during flash updates to process
 * updated routing information for external peers.  bgp_group_flash()
 * is called during flash updates to process updated routing
 * information for groups whose type is other than external.  These
 * routines are added to a peer/group when the peer becomes established/
 * group gets an established peer.
 *
 * bgp_aux_flash() is like bgp_group_flash(), except it is called to flash
 * a single internal group which is operating as an auxiliary to an IGP.
 * This allows the group to use the IGP's policy to control its actions.
 *
 * bgp_rt_reinit() is called after policy may have changed, to allow
 * the reevalutation of import policy of routes already in the table.
 *
 * bgp_peer_newpolicy() and bgp_group_newpolicy() are called at the
 * end of a reconfiguration, to allow reevaluation of exported routes
 * based on new policy from the reconfiguration, and to resync with
 * other routing table changes the reconfig may have caused.  This is
 * like bgp_peer_flash()/bgp_group_flash(), but with an awareness that
 * the policy may have changed.  bgp_aux_newpolicy() does the same thing
 * for INTERNAL_IGP groups.
 *
 * bgp_rt_peer_timer() is called when a peer's route timer expires.
 * The code examines a peer's incoming route queue for routes which have
 * been stable long enough to believe, and for external peers who are
 * not write-blocked also examines the peer's outgoing route queue for
 * routes which need to be sent.
 *
 * bgp_rt_group_timer() is called when a (non-external) group's route
 * timer expires.  The outgoing route queue is checked for routes
 * which need to be sent.
 *
 * bgp_rt_send_ready() is called when (essentially) select() indicates
 * a peer's socket is ready for writing.  The peer's (or group's) outgoing
 * route queue is checked for routes which should be sent to the peer.  This
 * routine is used when a previous write to the peer has been EWOULDBLOCK'd,
 * and immediately after a peer becomes established to blast out the
 * initial set of routes.
 */

/*
 * task_block pointers for the allocation of bgp_rti_entry's, bgp_rto_entry's,
 * bgpg_rto_entry's and bgp_asp_list entries.  These are initialized by
 * bgp_rt_init().
 */
static block_t	bgp_rti_block = (block_t) 0;
static block_t	bgp_rto_block = (block_t) 0;
static block_t	bgpg_rto_block = (block_t) 0;
static block_t	bgp_asp_block = (block_t) 0;
static block_t	bgp_hash_block = (block_t) 0;
static block_t	bgp_mets_block = (block_t) 0;
static block_t	bgp_ent_block = (block_t) 0;

/*
 * Macroes for allocating and freeing route queue entries.
 */
#define	BRT_RTI_ALLOC()	((bgp_rti_entry *)task_block_alloc(bgp_rti_block))
#define	BRT_RTO_ALLOC()	((bgp_rto_entry *)task_block_alloc(bgp_rto_block))
#define	BRT_GRTO_ALLOC() ((bgpg_rto_entry *)task_block_alloc(bgpg_rto_block))
#define	BRT_ASPL_ALLOC() ((bgp_asp_list *)task_block_alloc(bgp_asp_block))
#define	BRT_HASH_ALLOC() ((bgp_asp_list **)task_block_alloc(bgp_hash_block))
#define	BRT_METS_ALLOC() ((bgp_metrics_node *)task_block_alloc(bgp_mets_block))
#define	BRT_ENT_ALLOC()	((bgp_adv_entry *)task_block_alloc(bgp_ent_block))

#define	BRT_RTI_FREE(rti)	task_block_free(bgp_rti_block, (void_t)(rti))
#define	BRT_RTO_FREE(rto) \
    do { \
	register bgp_rto_entry *Xrto = (rto); \
	if (Xrto->bgpo_metrics) { \
	    BGPM_FREE(Xrto->bgpo_metrics); \
	} \
	if (Xrto->bgpo_asp) { \
	    ASPATH_FREE(Xrto->bgpo_asp); \
	} \
	task_block_free(bgp_rto_block, (void_t)Xrto); \
    } while (0)

#define	BRT_GRTO_FREE(rtog)	task_block_free(bgpg_rto_block, (void_t)(rtog))
#define	BRT_ASPL_FREE(aspl) \
	task_block_free(bgp_asp_block, (void_t)(aspl))
#define	BRT_ASPL_FREE_FAST(aspl) \
	task_block_free(bgp_asp_block, (void_t)(aspl))
#define	BRT_HASH_FREE_CLEAR(hashp) \
	task_block_free(bgp_hash_block, (void_t)(hashp))
#define	BRT_ENT_FREE(entp) \
	task_block_free(bgp_ent_block, (void_t)(entp))
#define	BRT_METS_FREE(mets)	task_block_free(bgp_mets_block, (void_t)(mets))

#define	BRT_INFO_ALLOC(bgp) \
    ((bgpg_rtinfo_entry *)task_block_alloc((bgp)->bgpg_oinfo_blk))
#define	BRT_INFO_FREE(bgp, infop) \
    do { \
	register bgpg_rtinfo_entry *Xinfop = (infop); \
	if (Xinfop->bgp_info_asp) { \
	    ASPATH_FREE(Xinfop->bgp_info_asp); \
	} \
	if (Xinfop->bgp_info_metrics) { \
	    BGPM_FREE(Xinfop->bgp_info_metrics); \
	} \
	task_block_free((bgp)->bgpg_oinfo_blk, (void_t)Xinfop); \
    } while (0)
#define	BRT_INFO_FREE_FAST(bgp, infop) \
    task_block_free((bgp)->bgpg_oinfo_blk, (void_t)(infop))

/*
 * Outgoing message data.  We build message packets in the following
 * non-local memory.  As we currently build version 2/3 and version 4
 * packets separately we use the same storage, though the names
 * referencing it are separate in case I decide to do this differently
 * later.
 *
 * The macroes add a new route to the appropriate message, while
 * subroutines build the headers and actually transmit the packet.
 */
#define	send_v3_message		((byte *)task_send_buffer)
static byte *send_v3_hdr_end;		/* end of the path attributes */
static byte *send_v3_next_route;	/* where the next route goes */
static byte *send_v3_next_hop;		/* where the next hop goes */

#define	send_v4_message		((byte *)task_send_buffer)
static byte *send_v4_hdr_start;		/* points at start of attributes */
static byte *send_v4_hdr_end;		/* points past end of attributes */
static byte *send_v4_next_route;	/* points where next route should go */
static byte *send_v4_next_hop;		/* where the next hop goes */

/*
 * The start of an update packet
 */
#define	SEND_START_V3	(send_v3_message + BGP_HEADER_LEN)
#define	SEND_START_V4	(send_v4_message + BGP_HEADER_LEN)

/*
 * Where attributes (normally) go in the packet
 */
#define	SEND_ATTR_V3	SEND_START_V3
#define	SEND_ATTR_V4	(SEND_START_V4+BGP_UNREACH_SIZE_LEN)

/*
 * See if the send packet has no room for another route
 */
#define	SEND_V3_PKT_FULL() \
    ((send_v3_next_route-send_v3_message) \
      > (BGPMAXSENDPKTSIZE-BGP_ROUTE_LENGTH))

#define	SEND_V4_PKT_FULL(bitcount) \
    ((send_v4_next_route-send_v4_message) \
      > (BGPMAXV4SENDPKTSIZE-BGP_BITCOUNT_LEN-BGP_PREFIX_LEN((bitcount))))

#define	SEND_V4_UNREACH_PKT_FULL(bitcount) \
    ((send_v4_next_route-send_v4_message) \
      > (BGPMAXV4SENDPKTSIZE-BGP_ATTR_SIZE_LEN \
       -BGP_BITCOUNT_LEN-BGP_PREFIX_LEN((bitcount))))

/* Don't bother trying to stuff the last 200 bytes */
#define	SEND_V4_MESSAGE_WASTE	200
#define	SEND_V4_MESSAGE_SLOP	50

/*
 * Add a route to the send packet.
 */
#define	SEND_ADDR(sun)		BGP_PUT_ADDR((sun), send_v3_next_route)
#define	SEND_PREFIX(sun, len)	BGP_PUT_PREFIX((len), (sun), send_v4_next_route)

/*
 * Change the next hop in the send packet.
 */
#define	SEND_PUT_NEXTHOP(nh, nhp) \
    do { \
	register byte *Xnh = (byte *)&(nh); \
	register byte *Xnhp = (byte *)nhp; \
	*Xnhp++ = *Xnh++; \
	*Xnhp++ = *Xnh++; \
	*Xnhp++ = *Xnh++; \
	*Xnhp++ = *Xnh++; \
    } while (0)

#define	SEND_PUT_NH_ADDR(addr, nhp) \
    do { \
	register byte *Xnh = (byte *)&(sock2ip(addr)); \
	register byte *Xnhp = (byte *)nhp; \
	*Xnhp++ = *Xnh++; \
	*Xnhp++ = *Xnh++; \
	*Xnhp++ = *Xnh++; \
	*Xnhp++ = *Xnh++; \
    } while (0)

/*
 * Return codes for send_v?_message_flush.  These are used to inform
 * the caller when we've had difficulty writing to the peer.
 */
#define	SEND_OK		0
#define	SEND_BLOCKED	0x1
#define	SEND_FAILED	0x2
#define	SEND_ALLFAILED	0x4

/*
 * Version bits.  Used by sending routines to keep track of what they
 * should be sending.
 */
#define	SEND_VERSION_2OR3	0x1
#define	SEND_VERSION_4		0x2

/*
 * Next hop types, for recording group next hop affinity.
 */
#define	SEND_NEXTHOP_NONE	0	/* always use local addr as next hop */
#define	SEND_NEXTHOP_PEERIF	1	/* send 3rd party if shared net */
#define	SEND_NEXTHOP_LIST	2	/* send 3rd party if if_addr in list */


/*
 * Definitions for sent update types.  It is either an initial update,
 * a flash update or a newpolicy update.
 */
#define	BRTUPD_NEWPOLICY	(0)
#define	BRTUPD_FLASH		(1)
#define	BRTUPD_INITIAL		(2)

static bits bgp_rt_update_bits[] = {
    {BRTUPD_NEWPOLICY, "new policy"},
    {BRTUPD_FLASH, "flash"},
    {BRTUPD_INITIAL, "initial"},
    {0}
};


/*
 * Definitions for group peer initialization.  It is either the
 * first peer to come up, the first version 4 peer to come up,
 * or an additional peer.
 */
#define	BRTINI_FIRST		(0)
#define	BRTINI_FIRST_V4		(1)
#define	BRTINI_ADD		(2)

static bits bgp_rt_init_group_bits[] = {
    {BRTINI_FIRST, "first group peer"},
    {BRTINI_FIRST_V4, "first group v4 peer"},
    {BRTINI_ADD, "add group peer"},
    {0}
};

/*
 * Metrics patricia support.  We keep all distinct metric/nexthop
 * combinations sorted in a patricia tree.  This allows us to
 * run economically in the normal case, where there aren't a whole
 * lot of metric/nexthop combinations in use, and makes searches
 * for the common structure fairly cheap.
 */

static bgp_metrics_node bgp_metrics_node_none = { { 0 } };	/* common case? */
#define	bgp_metrics_none	(&(bgp_metrics_node_none.bgpmn_metrics))

static bgp_metrics_node *bgp_metrics_node_tree = (bgp_metrics_node *) 0;

static u_long bgp_n_metrics = 0;
static u_long bgp_n_metrics_nonh = 0;
static u_long bgp_n_metrics_nomets = 0;

#define	BGPM_NO_NEXTHOP		((bgp_nexthop) 0)

/*
 * Bit encoding.  The low order byte of the node bit number is the
 * bit to test in a byte.  Bits are tested low-order end first so
 * the value of the node bit increases as we move down the tree.
 * the high-order part of the bit number is the byte offset into
 * the metrics structure.
 */
#define	BGP_NBBY		8	/* better be 8-bit bytes */

#define	BGPM_BIT_NONE		0	/* node with external info only */

#define	BGPM_BIT(bit)		((byte)(bit))
#define	BGPM_OFFSET(bit)	((bit) >> BGP_NBBY)
#define	BGPM_TEST(cp, bit) \
    (BIT_TEST(((byte *)(cp))[BGPM_OFFSET((bit))], BGPM_BIT((bit))) != (byte) 0)

/*
 * Check to see if the metrics are the same.  I can't decide whether
 * it is better to do it this way or with bcmp(), but it may not
 * matter since comparisons are done infrequently.
 */
#define	BGP_METRICS_SAME(m1, m2) \
    (((bgp_metrics *)(m1))->bgpm_nexthop \
	== ((bgp_metrics *)(m2))->bgpm_nexthop \
      && ((bgp_metrics *)(m1))->bgpm_types \
	== ((bgp_metrics *)(m2))->bgpm_types \
      && (((bgp_metrics *)(m1))->bgpm_types == 0 \
      || (((bgp_metrics *)(m1))->bgpm_metric \
	== ((bgp_metrics *)(m2))->bgpm_metric \
      && ((bgp_metrics *)(m1))->bgpm_localpref \
	== ((bgp_metrics *)(m2))->bgpm_localpref \
      && ((bgp_metrics *)(m1))->bgpm_tag \
	== ((bgp_metrics *)(m2))->bgpm_tag)))

#define	BGP_NO_METRICS(m) \
    ((m)->bgpm_nexthop == BGPM_NO_NEXTHOP && (m)->bgpm_types == 0)

/*
 * Allocate and free metrics structures.
 */
#define	BGPM_ALLOC(m)	(((bgp_metrics_node *)(m))->bgpmn_refcount++)

#define	BGPM_FREE(m) \
    do { \
	if (((bgp_metrics_node *)(m))->bgpmn_refcount <= 1) { \
	    bgp_rt_metrics_free((m)); \
	} else { \
	    ((bgp_metrics_node *)(m))->bgpmn_refcount--; \
	} \
    } while (0)

#define	BGPM_FIRST(m)	(((bgp_metrics_node *)(m))->bgpmn_refcount == 1)

#define	BGPM_FIND(m, mfound) \
    do { \
	if (BGP_NO_METRICS((m))) { \
	    BGPM_ALLOC(bgp_metrics_none); \
	    (mfound) = bgp_metrics_none; \
	} else { \
	    register bgp_metrics_node *Xmn = bgp_metrics_node_tree; \
	    if (Xmn) { \
		register byte *Xcp = (byte *)(m); \
		register u_int Xbit = BGPM_BIT_NONE; \
		while (Xbit < Xmn->bgpmn_bit) { \
		    Xbit = Xmn->bgpmn_bit; \
		    if (BGPM_TEST(Xcp, Xbit)) { \
			Xmn = Xmn->bgpmn_right; \
		    } else { \
			Xmn = Xmn->bgpmn_left; \
		    } \
		} \
		if (BGP_METRICS_SAME((m), Xmn)) { \
		    BGPM_ALLOC(Xmn); \
		    (mfound) = &(Xmn->bgpmn_metrics); \
		} else { \
		    (mfound) = bgp_rt_metrics_add((m), Xmn); \
		} \
	    } else { \
		(mfound) = bgp_rt_metrics_add((m), (bgp_metrics_node *)0); \
	    } \
	} \
    } while (0)

#ifdef	notdef
#define	BGPM_FIND(m, mfound) \
    do { \
	if (BGP_NO_METRICS((m))) { \
	    BGPM_ALLOC(bgp_metrics_none); \
	    (mfound) = bgp_metrics_none; \
	} else { \
	    (mfound) = bgp_rt_metrics_find((m)); \
	} \
    } while (0)
#endif	/* notdef */

/*
 * Array to find the lowest order bit set in a byte
 */
static const byte bgp_metrics_bit[256] = {
    0x00, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x10, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x20, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x10, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x40, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x10, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x20, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x10, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x80, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x10, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x20, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x10, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x40, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x10, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x20, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x10, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01,
    0x08, 0x01, 0x02, 0x01, 0x04, 0x01, 0x02, 0x01
};


/*
 * The tsi field for a peer/group either contains a pointer to a
 * bgp*_rto_entry structure, or a pointer to a metrics structure.
 * We store an extra byte so we can tell whether we've got a pointer
 * or the metric (pair) and process the data accordingly.
 *
 * The length of data should be the larger of the largest pointer
 * and twice the length of a metric.
 */
#define	BRT_RTO_LENGTH	sizeof(bgp_rto_entry *)
#define	BRT_GRTO_LENGTH	sizeof(bgpg_rto_entry *)
#define	BRT_METRIC_LEN	sizeof(bgp_metrics *)
#define	BRT_TSI_DATA_SIZE \
    ((BRT_METRIC_LEN > BRT_RTO_LENGTH) \
      ? ((BRT_METRIC_LEN > BRT_GRTO_LENGTH) \
	? BRT_METRIC_LEN \
	: BRT_GRTO_LENGTH) \
      : ((BRT_RTO_LENGTH > BRT_GRTO_LENGTH) \
	? BRT_RTO_LENGTH \
	: BRT_GRTO_LENGTH))
#define	BRT_TSI_SIZE	(1 + BRT_TSI_DATA_SIZE)

union brt_tsi_union {
    bgp_adv_entry *brt_tsi_entry;
    bgp_rto_entry *brt_tsi_rto;
    bgpg_rto_entry *brt_tsi_grto;
    byte brt_tsi_byte[BRT_TSI_SIZE];	/* 1 byte longer than longest above */
};

#define	BRT_TSI_OPTBYTE(btu)	((btu).brt_tsi_byte[BRT_TSI_DATA_SIZE])

#define	BRT_TSI_NONE		0	/* not set */
#define	BRT_TSI_ENT		1	/* has adv entry */
#define	BRT_TSI_RTO		2	/* has peer RTO */
#define	BRT_TSI_GRTO		3	/* has group RTO */

/*
 * Clear the tsi field.  Set the first byte to zero
 */
#define	BRT_TSI_CLEAR(rth, bit)	rttsi_reset((rth), (bit))

/*
 * Store a bgp_rto_entry pointer into the tsi
 */
#ifdef	BGPDEBUG
#define	BRT_TSI_PUT_RTO(rth, bit, brto) \
    do { \
	union brt_tsi_union Xtsi; \
	Xtsi.brt_tsi_rto = (brto); \
	assert(Xtsi.brt_tsi_rto->bgpo_advrt \
	 && BIT_TEST(Xtsi.brt_tsi_rto->bgpo_advrt->bgpe_flags, BGPEF_QUEUED)); \
	BRT_TSI_OPTBYTE(Xtsi) = BRT_TSI_RTO; \
	rttsi_set((rth), (bit), Xtsi.brt_tsi_byte); \
    } while (0)
#else	/* BGPDEBUG */
#define	BRT_TSI_PUT_RTO(rth, bit, brto) \
    do { \
	union brt_tsi_union Xtsi; \
	Xtsi.brt_tsi_rto = (brto); \
	BRT_TSI_OPTBYTE(Xtsi) = BRT_TSI_RTO; \
	rttsi_set((rth), (bit), Xtsi.brt_tsi_byte); \
    } while (0)
#endif	/* BGPDEBUG */

/*
 * Store a bgpg_rto_entry pointer into the tsi
 */
#ifdef	BGPDEBUG
#define	BRT_TSI_PUT_GRTO(rth, bit, bgrto) \
    do { \
	union brt_tsi_union Xtsi; \
	Xtsi.brt_tsi_grto = (bgrto); \
	assert(Xtsi.brt_tsi_grto->bgpgo_advrt \
	 && BIT_TEST(Xtsi.brt_tsi_grto->bgpgo_advrt->bgpe_flags, BGPEF_QUEUED));\
	BRT_TSI_OPTBYTE(Xtsi) = BRT_TSI_GRTO; \
	rttsi_set((rth), (bit), Xtsi.brt_tsi_byte); \
    } while (0)
#else	/* BGPDEBUG */
#define	BRT_TSI_PUT_GRTO(rth, bit, bgrto) \
    do { \
	union brt_tsi_union Xtsi; \
	Xtsi.brt_tsi_grto = (bgrto); \
	BRT_TSI_OPTBYTE(Xtsi) = BRT_TSI_GRTO; \
	rttsi_set((rth), (bit), Xtsi.brt_tsi_byte); \
    } while (0)
#endif	/* BGPDEBUG */

/*
 * Store an adv entry into the tsi
 */
#ifdef	BGPDEBUG
#define	BRT_TSI_PUT_ENT(rth, bit, entp) \
    do { \
	union brt_tsi_union Xtsi; \
	Xtsi.brt_tsi_entry = (entp); \
	assert(!BIT_TEST(Xtsi.brt_tsi_entry->bgpe_flags, BGPEF_QUEUED)); \
	BRT_TSI_OPTBYTE(Xtsi) = BRT_TSI_ENT; \
	rttsi_set((rth), (bit), Xtsi.brt_tsi_byte); \
    } while (0)
#else	/* BGPDEBUG */
#define	BRT_TSI_PUT_ENT(rth, bit, entp) \
    do { \
	union brt_tsi_union Xtsi; \
	Xtsi.brt_tsi_entry = (entp); \
	BRT_TSI_OPTBYTE(Xtsi) = BRT_TSI_ENT; \
	rttsi_set((rth), (bit), Xtsi.brt_tsi_byte); \
    } while (0)
#endif	/* BGPDEBUG */

/*
 * Fetch the tsi contents for a peer.  Assumes the local preference
 * won't ever be set when this is used.
 */
#ifdef	BGPDEBUG	
#define	BRT_TSI_GET_PEER(rth, bit, brto, entp) \
    do { \
	union brt_tsi_union Xtsi; \
	rttsi_get((rth), (bit), Xtsi.brt_tsi_byte); \
	switch (BRT_TSI_OPTBYTE(Xtsi)) { \
	default: \
	    assert(FALSE); \
	case BRT_TSI_NONE: \
	    (brto) = (bgp_rto_entry *) 0; \
	    (entp) = (bgp_adv_entry *) 0; \
	    break; \
	case BRT_TSI_ENT: \
	    (brto) = (bgp_rto_entry *) 0; \
	    (entp) = Xtsi.brt_tsi_entry; \
	    assert(!BIT_TEST(Xtsi.brt_tsi_entry->bgpe_flags, BGPEF_QUEUED)); \
	    break; \
	case BRT_TSI_RTO: \
	    (brto) = Xtsi.brt_tsi_rto; \
	    (entp) = Xtsi.brt_tsi_rto->bgpo_advrt; \
	    assert(Xtsi.brt_tsi_rto->bgpo_advrt \
	      && BIT_TEST(Xtsi.brt_tsi_rto->bgpo_advrt->bgpe_flags, \
		BGPEF_QUEUED)); \
	    break; \
	} \
    } while (0)
#else	/* BGPDEBUG */
#define	BRT_TSI_GET_PEER(rth, bit, brto, entp) \
    do { \
	union brt_tsi_union Xtsi; \
	rttsi_get((rth), (bit), Xtsi.brt_tsi_byte); \
	switch (BRT_TSI_OPTBYTE(Xtsi)) { \
	default: \
	    assert(FALSE); \
	case BRT_TSI_NONE: \
	    (brto) = (bgp_rto_entry *) 0; \
	    (entp) = (bgp_adv_entry *) 0; \
	    break; \
	case BRT_TSI_ENT: \
	    (brto) = (bgp_rto_entry *) 0; \
	    (entp) = Xtsi.brt_tsi_entry; \
	    break; \
	case BRT_TSI_RTO: \
	    (brto) = Xtsi.brt_tsi_rto; \
	    (entp) = Xtsi.brt_tsi_rto->bgpo_advrt; \
	    break; \
	} \
    } while (0)
#endif	/* BGPDEBUG */

/*
 * Fetch the tsi contents for a group.
 */
#ifdef	BGPDEBUG
#define	BRT_TSI_GET_GROUP(rth, bit, bgrto, entp) \
    do { \
	union brt_tsi_union Xtsi; \
	rttsi_get((rth), (bit), Xtsi.brt_tsi_byte); \
	switch (BRT_TSI_OPTBYTE(Xtsi)) { \
	default: \
	    assert(FALSE); \
	case BRT_TSI_NONE: \
	    (bgrto) = (bgpg_rto_entry *) 0; \
	    (entp) = (bgp_adv_entry *) 0; \
	    break; \
	case BRT_TSI_ENT: \
	    (bgrto) = (bgpg_rto_entry *) 0; \
	    (entp) = Xtsi.brt_tsi_entry; \
	    assert(!BIT_TEST(Xtsi.brt_tsi_entry->bgpe_flags, BGPEF_QUEUED)); \
	    break; \
	case BRT_TSI_GRTO: \
	    (bgrto) = Xtsi.brt_tsi_grto; \
	    (entp) = Xtsi.brt_tsi_grto->bgpgo_advrt; \
	    assert(Xtsi.brt_tsi_grto->bgpgo_advrt \
	      && BIT_TEST(Xtsi.brt_tsi_grto->bgpgo_advrt->bgpe_flags, \
		BGPEF_QUEUED)); \
	    break; \
	} \
    } while (0)
#else	/* BGPDEBUG */
#define	BRT_TSI_GET_GROUP(rth, bit, bgrto, entp) \
    do { \
	union brt_tsi_union Xtsi; \
	rttsi_get((rth), (bit), Xtsi.brt_tsi_byte); \
	switch (BRT_TSI_OPTBYTE(Xtsi)) { \
	default: \
	    assert(FALSE); \
	case BRT_TSI_NONE: \
	    (bgrto) = (bgpg_rto_entry *) 0; \
	    (entp) = (bgp_adv_entry *) 0; \
	    break; \
	case BRT_TSI_ENT: \
	    (bgrto) = (bgpg_rto_entry *) 0; \
	    (entp) = Xtsi.brt_tsi_entry; \
	    break; \
	case BRT_TSI_GRTO: \
	    (bgrto) = Xtsi.brt_tsi_grto; \
	    (entp) = Xtsi.brt_tsi_grto->bgpgo_advrt; \
	    break; \
	} \
    } while (0)
#endif	/* BGPDEBUG */

/*
 * Queuing routes on an AS path list.
 */
#define	BRT_ASPL_QUEUE_START(aspl, brto) \
    do { \
	register bgp_asp_list *Xaspl = (aspl); \
	register bgp_rto_entry *Xbrto = (brto); \
	Xbrto->bgpo_prev = (bgp_rto_entry *)Xaspl; \
	(Xbrto->bgpo_next = Xaspl->bgpl_rto_next)->bgpo_prev = Xbrto; \
	Xaspl->bgpl_rto_next = Xbrto; \
    } while (0)

#define	BRT_ASPL_QUEUE_END(aspl, brto) \
    do { \
	register bgp_asp_list *Xaspl = (aspl); \
	register bgp_rto_entry *Xbrto = (brto); \
	Xbrto->bgpo_next = (bgp_rto_entry *)Xaspl; \
	(Xbrto->bgpo_prev = Xaspl->bgpl_rto_prev)->bgpo_next = Xbrto; \
	Xaspl->bgpl_rto_prev = Xbrto; \
    } while (0)

#define	BRT_GASPL_QUEUE_START(aspl, bgrto) \
    do { \
	register bgp_asp_list *Xaspl = (aspl); \
	register bgpg_rto_entry *Xbgrto = (bgrto); \
	Xbgrto->bgpgo_prev = (bgpg_rto_entry *)Xaspl; \
	(Xbgrto->bgpgo_next = Xaspl->bgpl_grto_next)->bgpgo_prev = Xbgrto; \
	Xaspl->bgpl_grto_next = Xbgrto; \
    } while (0)

#define	BRT_GASPL_QUEUE_END(aspl, bgrto) \
    do { \
	register bgp_asp_list *Xaspl = (aspl); \
	register bgpg_rto_entry *Xbgrto = (bgrto); \
	Xbgrto->bgpgo_next = (bgpg_rto_entry *)Xaspl; \
	(Xbgrto->bgpgo_prev = Xaspl->bgpl_grto_prev)->bgpgo_next = Xbgrto; \
	Xaspl->bgpl_grto_prev = Xbgrto; \
    } while (0)


/*
 * See if this route came from a BGP peer
 */
#define	BRT_ISBGP(rt)	((rt)->rt_gwp->gw_proto == RTPROTO_BGP)

/*
 * See if this route came from BGP or EGP.  Since EGP may not be compiled
 * in we've got a couple of alternatives.
 */
#ifdef	PROTO_EGP
#define	BRT_BGP_OR_EGP(rt) \
    (BRT_ISBGP(rt) || (rt)->rt_gwp->gw_proto == RTPROTO_EGP)
#else	/* PROTO_EGP */
#define	BRT_BGP_OR_EGP(rt)	BRT_ISBGP(rt)
#endif	/* PROTO_EGP */

/*
 * Given a route from a BGP peer, return his group
 */
#define	BRT_GROUP(rt) \
    (((bgpPeer *)((rt)->rt_gwp->gw_task->task_data))->bgp_group)

/*
 * XXX THIS SHOULD NOT BE HERE
 */
#define	bgp_is_on_if(ifap, host) \
    ((ifap)->ifa_net \
    ? inet_addrcmp_mask(sock2ip(host), \
			sock2ip((ifap)->ifa_net), \
			sock2ip((ifap)->ifa_netmask)) \
    : (sockaddrcmp_in((host), (ifap)->ifa_addr) \
       || sockaddrcmp_in((host), (ifap)->ifa_addr_local)))


/*
 * bgp_peer_tsi_dump - pretty print a peer's tsi info
 */
static void
bgp_peer_tsi_dump __PF4(fp, FILE *,
			rth, rt_head *,
			data, void_t,
			pfx, const char *)
{
    bgpPeer *bnp = (bgpPeer *)data;
    bgp_metrics *mets;
    bgp_adv_entry *entp;
    bgp_rto_entry *brto;

    BRT_TSI_GET_PEER(rth, bnp->bgp_task->task_rtbit, brto, entp);
    if (entp) {
	(void) fprintf(fp,
		       "%sBGP %s",
		       pfx,
		       bnp->bgp_name);
	mets = entp->bgpe_metrics;
	if (mets) {
	    if (!mets->bgpm_tmetric
		&& !mets->bgpm_ttag
		&& !mets->bgpm_tlocalpref) {
		(void) fprintf(fp,
			       " no metrics");
	    } else {
		/* pretty stinky */
		if (mets->bgpm_tmetric) {
		    (void) fprintf(fp,
				   " metric %lu",
				   mets->bgpm_metric);
		}
		if (mets->bgpm_tlocalpref) {
		    (void) fprintf(fp,
				   " pref %lu",
				   mets->bgpm_localpref);
		}
		if (mets->bgpm_ttag) {
		    (void) fprintf(fp,
				   " tag %lu",
				   mets->bgpm_tag);
		}
	    }
	    if (brto) {
		(void) fprintf(fp, " (queued)\n");
	    } else {
		(void) fprintf(fp, "\n");
	    }
	} else {
	    (void) fprintf(fp, " (deleting)\n");
	}
    }
}


/*
 * bgp_group_tsi_dump - pretty print a group's tsi info
 */
static void
bgp_group_tsi_dump __PF4(fp, FILE *,
			 rth, rt_head *,
			 data, void_t,
			 pfx, const char *)
{
    bgpPeerGroup *bgp = (bgpPeerGroup *)data;
    bgpg_rto_entry *bgrto;
    bgp_adv_entry *entp;
    bgp_metrics *mets;

    BRT_TSI_GET_GROUP(rth, bgp->bgpg_task->task_rtbit, bgrto, entp);
    if (entp) {
	(void) fprintf(fp,
		       "%sBGP %s",
		       pfx,
		       bgp->bgpg_name);
	mets = entp->bgpe_metrics;
	if (mets) {
	    if (!mets->bgpm_tmetric
		&& !mets->bgpm_ttag
		&& !mets->bgpm_tlocalpref) {
		(void) fprintf(fp, " no metrics");
	    } else {
		/* pretty stinky */
		if (mets->bgpm_tmetric) {
		    (void) fprintf(fp,
				   " metric %lu",
				   mets->bgpm_metric);
		}
		if (mets->bgpm_tlocalpref) {
		    (void) fprintf(fp,
				   " pref %lu",
				   mets->bgpm_localpref);
		}
		if (mets->bgpm_ttag) {
		    (void) fprintf(fp,
				   " tag %lu",
				   mets->bgpm_tag);
		}
	    }
	    if (bgrto) {
		(void) fprintf(fp, " (queued)\n");
	    } else {
		(void) fprintf(fp, "\n");
	    }
	} else {
	    (void) fprintf(fp, " (deleting)\n");
	}
    }
}


#ifdef	BGPDEBUG
/*
 * bgp_blewit - a hack replacement for places assert() is inconvenient
 */
void_t
bgp_blewit __PF2(file, const char *,
		 line, int)
{
    task_assert(file, line, (char *) 0);
    return (void_t) 0;
}
#endif	/* BGPDEBUG */


/*
 * bgp_rt_metrics_add - add a new set of metrics to the tree.  The
 *			argument passed in is the node found during
 *			a previous unsucessful search.
 */
static bgp_metrics *
bgp_rt_metrics_add __PF2(mets, bgp_metrics *,
			 mfound, bgp_metrics_node *)
{
    register bgp_metrics_node *mnp;
    register bgp_metrics_node *mnp_new;
    register bgp_metrics_node *mnp_prev;
    register byte *cp, *ncp;
    register u_int nbit = 0;	/* to shut gcc up */
    register u_int i;

    /*
     * Fetch a node structure and initialize it.
     */
    mnp_new = BRT_METS_ALLOC();
    mnp_new->bgpmn_metrics = *mets;
    mnp_new->bgpmn_refcount = 1;
    bgp_n_metrics++;
    if (mnp_new->bgpmn_metrics.bgpm_nexthop == BGPM_NO_NEXTHOP) {
	bgp_n_metrics_nonh++;
    } else if (mnp_new->bgpmn_metrics.bgpm_types == 0) {
	bgp_n_metrics_nomets++;
    }

    /*
     * Catch the easy case (first node in the tree) now.
     */
    cp = (byte *) mfound;
    if (!cp) {
	bgp_metrics_node_tree = mnp_new;
	mnp_new->bgpmn_left = mnp_new->bgpmn_right = mnp_new;
	mnp_new->bgpmn_bit = BGPM_BIT_NONE;
	return (bgp_metrics *) mnp_new;
    }

    /*
     * Find the bit number where the new guy and the found guy
     * differ.  This is where we'll need to insert a new node.
     */
    ncp = (byte *) mnp_new;

    for (i = 0; i < sizeof(bgp_metrics); i++) {
	if (*cp != *ncp) {
	    nbit = bgp_metrics_bit[*cp ^ *ncp] | (i << BGP_NBBY);
	    break;
	}
	cp++, ncp++;
    }
    assert(i < sizeof(bgp_metrics));

    /*
     * Got nbit.  Search our way down through the tree until
     * we find the spot to insert the new node.
     */
    cp = (byte *) mnp_new;
    mnp_prev = (bgp_metrics_node *) 0;
    mnp = bgp_metrics_node_tree;
    i = BGPM_BIT_NONE;
    while (i < mnp->bgpmn_bit) {
	i = mnp->bgpmn_bit;
	if (nbit <= i) {
	    break;
	}

	mnp_prev = mnp;
	if (BGPM_TEST(cp, i)) {
	    mnp = mnp->bgpmn_right;
	} else {
	    mnp = mnp->bgpmn_left;
	}
    }
    assert(nbit != i);

    /*
     * mnp points to the node which will be after us, mnp_prev
     * points to the node before us.  Fill them in.
     */
    if (BGPM_TEST(cp, nbit)) {
	mnp_new->bgpmn_right = mnp_new;
	mnp_new->bgpmn_left = mnp;
    } else {
	mnp_new->bgpmn_left = mnp_new;
	mnp_new->bgpmn_right = mnp;
    }
    mnp_new->bgpmn_bit = nbit;

    if (!mnp_prev) {
	bgp_metrics_node_tree = mnp_new;
    } else if (mnp_prev->bgpmn_right == mnp) {
	mnp_prev->bgpmn_right = mnp_new;
    } else {
	mnp_prev->bgpmn_left = mnp_new;
    }

    return (bgp_metrics *) mnp_new;
}


/*
 * bgp_rt_metrics_free - dereference a set of metrics, free it if
 *			 the reference count drops to zero.
 */
static void
bgp_rt_metrics_free __PF1(mets, bgp_metrics *)
{
    register bgp_metrics_node *mnp_free = (bgp_metrics_node *) mets;
    register u_int i;
    register bgp_metrics_node *mnp, *tmp;
    register bgp_metrics_node *mnp_prev, *mnp_int;

    /*
     * Decrement the reference count.  If it is nonzero afterwards,
     * or if this is the metrics_none node, just return.
     */
    assert(mnp_free->bgpmn_refcount);
    if ((--mnp_free->bgpmn_refcount) || mnp_free == &bgp_metrics_node_none) {
	return;
    }

    /*
     * The node needs to come out of the tree.  If this is the trivial
     * case (one node in tree) catch that first.  Otherwise we search
     * the tree to find references to the node.
     */
    mnp = bgp_metrics_node_tree;
    i = mnp->bgpmn_bit;
    if (i == BGPM_BIT_NONE) {
	assert(mnp == mnp_free);
	bgp_metrics_node_tree = (bgp_metrics_node *) 0;
    } else {
	mnp_prev = mnp_int = (bgp_metrics_node *) 0;
	for (;;) {
	    if (BGPM_TEST(mnp_free, i)) {
		tmp = mnp->bgpmn_right;
	    } else {
		tmp = mnp->bgpmn_left;
	    }

	    if (tmp == mnp_free) {
		if (tmp->bgpmn_bit <= i) {
		    break;
		}
		mnp_int = mnp;
	    } else {
		assert(tmp->bgpmn_bit > i);
	    }
	    mnp_prev = mnp;
	    i = tmp->bgpmn_bit;
	    mnp = tmp;
	}

	/*
	 * Found all we need to know.  mnp_int points to the node
	 * which precedes mnp_free as an internal node.  mnp points
	 * to the node which precedes mnp_free as an external, and
	 * mnp_prev points to mnp's predecessor.
	 */
	if (mnp->bgpmn_right == mnp_free) {
	    tmp = mnp->bgpmn_left;
	} else {
	    tmp = mnp->bgpmn_right;
	}

	if (!mnp_prev) {
	    bgp_metrics_node_tree = tmp;
	} else if (mnp_prev->bgpmn_right == mnp) {
	    mnp_prev->bgpmn_right = tmp;
	} else {
	    mnp_prev->bgpmn_left = tmp;
	}

	if (mnp != mnp_free) {
	    if (mnp_free->bgpmn_bit == BGPM_BIT_NONE) {
		mnp->bgpmn_bit = BGPM_BIT_NONE;
		mnp->bgpmn_left = mnp->bgpmn_right = mnp;
	    } else {
		mnp->bgpmn_bit = mnp_free->bgpmn_bit;
		mnp->bgpmn_left = mnp_free->bgpmn_left;
		mnp->bgpmn_right = mnp_free->bgpmn_right;
		if (!mnp_int) {
		    bgp_metrics_node_tree = mnp;
		} else if (mnp_int->bgpmn_right == mnp_free) {
		    mnp_int->bgpmn_right = mnp;
		} else {
		    mnp_int->bgpmn_left = mnp;
		}
	    }
	}
    }

    if (mnp_free->bgpmn_metrics.bgpm_nexthop == BGPM_NO_NEXTHOP) {
	bgp_n_metrics_nonh--;
    } else if (mnp_free->bgpmn_metrics.bgpm_types == 0) {
	bgp_n_metrics_nomets--;
    }
    bgp_n_metrics--;
    BRT_METS_FREE(mnp_free);
}


/*
 * bgp_rt_unsync - set a group peer unsynchronized.  Used when we get
 *		   a write block or failure somewhere.
 */
void
bgp_rt_unsync __PF1(bnp, bgpPeer *)
{
    register bgp_bits *sbits;
    register bgpPeerGroup *bgp;
    register int word, wbit;

    bgp = bnp->bgp_group;
    if (bgp->bgpg_type == BGPG_EXTERNAL) {
	return;			/* XXX should we complain? */
    }

    word = BGPB_WORD(bnp->bgp_group_bit);
    wbit = BGPB_WBIT(bnp->bgp_group_bit);

    if (bnp->bgp_version == BGP_VERSION_4) {
	if (bgp->bgpg_n_v4_sync > 0) {
	    sbits = BGPG_GETBITS(bgp->bgpg_v4_sync, bgp->bgpg_idx_size);
	    if (BGPB_WB_TEST(sbits, word, wbit)) {
		BGPB_WB_RESET(sbits, word, wbit);
		bgp->bgpg_n_v4_sync--;
	    }
	}
    } else {
	if (bgp->bgpg_n_v3_sync > 0) {
	    sbits = BGPG_GETBITS(bgp->bgpg_v3_sync, bgp->bgpg_idx_size);
	    if (BGPB_WB_TEST(sbits, word, wbit)) {
		BGPB_WB_RESET(sbits, word, wbit);
		bgp->bgpg_n_v3_sync--;
	    }
	}
    }
}


/*
 * bgp_rt_sync - set a group peer synchronized, if he is established
 *		 but currently un-sync'd.
 */
void
bgp_rt_sync __PF1(bnp, bgpPeer *)
{
    register bgp_bits *sbits;
    register bgpPeerGroup *bgp;
    register int word, wbit;
    register size_t idxsize;

    bgp = bnp->bgp_group;
    if (bgp->bgpg_type == BGPG_EXTERNAL) {
	return;			/* XXX should we complain? */
    }

    word = BGPB_WORD(bnp->bgp_group_bit);
    wbit = BGPB_WBIT(bnp->bgp_group_bit);
    idxsize = bgp->bgpg_idx_size;

    if (bnp->bgp_version == BGP_VERSION_4) {
	if (bgp->bgpg_n_v4_bits > 0) {
	    sbits = BGPG_GETBITS(bgp->bgpg_v4_bits, idxsize);
	    if (BGPB_WB_TEST(sbits, word, wbit)) {
		sbits = BGPG_GETBITS(bgp->bgpg_v4_sync, idxsize);
		if (!BGPB_WB_TEST(sbits, word, wbit)) {
		    BGPB_WB_SET(sbits, word, wbit);
		    bgp->bgpg_n_v4_sync++;
		}
	    }
	}
    } else {
	if (bgp->bgpg_n_v3_bits > 0) {
	    sbits = BGPG_GETBITS(bgp->bgpg_v3_bits, idxsize);
	    if (BGPB_WB_TEST(sbits, word, wbit)) {
		sbits = BGPG_GETBITS(bgp->bgpg_v3_sync, idxsize);
		if (!BGPB_WB_TEST(sbits, word, wbit)) {
		    BGPB_WB_SET(sbits, word, wbit);
		    bgp->bgpg_n_v3_sync++;
		}
	    }
	}
    }
}



/*
 * bgp_rt_send_v3_message_attr - initialize the outgoing version 2/3 message,
 *			       setting up the path attributes.
 */
static void
bgp_rt_send_v3_message_attr __PF3(my_as, as_t,
				  asp, as_path *,
				  asip, as_path_info *)
{
    register byte *cp;
    register byte *endp;

    /*
     * Point at the start of the send packet, then skip past the
     * place where the header will go.
     */
    cp = send_v3_message;
    BGP_SKIP_HEADER(cp);

    /*
     * Insert the path attributes into the packet.  Leave room for the
     * length, then fill it in afterward.
     */
    endp = aspath_format(my_as,
		         asp,
		         asip,
		         &send_v3_next_hop,
		         cp + BGP_ATTR_SIZE_LEN);
    /* XXX relies on goodness of macro */
    BGP_PUT_SHORT((endp - cp) - BGP_ATTR_SIZE_LEN, cp);

    /*
     * Initialize the packet pointers.
     */
    send_v3_hdr_end = send_v3_next_route = endp;
}


/*
 * bgp_rt_send_v4_message_attr - add path attributes to the outgoing version 4
 *			       message.  If the append flag is set we attempt
 *			       to place the attributes just after the
 *			       unreachable route data, otherwise they go
 *			       at the start of the packet.  Return an error
 *			       if this is an append and there is no room
 *			       for the header and a few routes.
 */
static int
bgp_rt_send_v4_message_attr __PF4(my_as, as_t,
				  asp, as_path *,
				  asip, as_path_info *,
				  append, int)
{
    register byte *cp;
    register byte *endp;
    byte *attrp;
    int estlen = 0;

    /*
     * If an append, check for sufficient room in message for the attributes.
     * Otherwise start at the beginning.
     */
    cp = SEND_ATTR_V4;
    if (append && send_v4_next_route != NULL && send_v4_next_route != cp) {
	register int len;
	
	len = BGPMAXV4SENDPKTSIZE - (send_v4_next_route - send_v4_message);
	if (len < SEND_V4_MESSAGE_WASTE) {
	    return (SEND_ALLFAILED);
	}
	len -= SEND_V4_MESSAGE_SLOP;
	if (len < (estlen = aspath_v4_estimate_len(my_as, asp, asip))) {
	    return (SEND_ALLFAILED);
	}

	/*
	 * Fix up unreachable length
	 */
	len = send_v4_next_route - cp;
	cp -= BGP_UNREACH_SIZE_LEN;
	BGP_PUT_SHORT(len, cp);

	cp = send_v4_next_route;
    } else {
	*(cp - 2) = *(cp - 1) = 0;
    }

    /*
     * Insert the path attributes in the location just found.
     */
    send_v4_hdr_start = cp;
    attrp = cp + BGP_ATTR_SIZE_LEN;
    endp = aspath_format_v4(my_as,
			    asp,
			    asip,
			    &send_v4_next_hop,
			    attrp);
    if (estlen && (endp - attrp) != estlen) {
	trace_log_tf(trace_global,
		     0,
		     LOG_ERR,
		     ("bgp_rt_send_v4_message_attr: estimated length %d actual %u",
		      estlen,
		      (endp - attrp)));
    }
    BGP_PUT_SHORT((endp - attrp), cp);

    /*
     * Set up end and route pointers here
     */
    send_v4_hdr_end = send_v4_next_route = endp;
    return (SEND_OK);
}


/*
 * bgp_rt_send_v3_unreachable_init - initialize the outgoing message with
 *			          unreachable attributes.
 */
static void
bgp_rt_send_v3_unreachable_init __PF3(my_as, as_t,
				      nexthop, sockaddr_un *,
				      internal, int)
{
    register byte *cp;
    register byte *endp;
    as_path_info api;

    /*
     * Skip past the header for now
     */
    cp = send_v3_message;
    BGP_SKIP_HEADER(cp);

    /*
     * Initialize the AS path info appropriately.
     */
    bzero((caddr_t)&api, sizeof(api));
    if (internal) {
        api.api_flags = APIF_INTERNAL|APIF_UNREACH|APIF_NEXTHOP;
    } else {
	api.api_flags = APIF_UNREACH|APIF_NEXTHOP;
    }

    /*
     * Insert the path attributes into the packet.  Leave room for the
     * length, then fill it in afterward.
     */
    endp = aspath_format(my_as,
		       (as_path *)0,
		       &api,
		       &send_v3_next_hop,
		       cp + BGP_ATTR_SIZE_LEN);
    /* XXX relies on goodness of macro */
    BGP_PUT_SHORT((endp - cp) - BGP_ATTR_SIZE_LEN, cp);
    if (nexthop) {
	SEND_PUT_NH_ADDR(nexthop, send_v3_next_hop);
    }

    /*
     * Initialize the packet pointers.
     */
    send_v3_hdr_end = send_v3_next_route = endp;
}


/*
 * bgp_rt_send_v4_unreachable_init - initialize the outgoing message
 *				  to receive unreachable routing information
 */
static void
bgp_rt_send_v4_unreachable_init()
{
    /*
     * Nothing really to do here except initialize the pointers.
     */
    send_v4_hdr_start = send_v4_hdr_end = send_v4_next_hop = (byte *) 0;
    send_v4_next_route = SEND_ATTR_V4;
}

/*
 * bgp_rt_send_message - send the specified message to the given peer.
 *			 If it blocks, spool the remainder into a send
 *			 buffer attached to the peer and return an error.
 */
static int
bgp_rt_send_message __PF3(bnp, bgpPeer *,
			  message, byte *,
			  mlen, size_t)
{
    int res;

    if (mlen == 0) {
	return SEND_OK;
    }

    if (BIT_TEST(bnp->bgp_flags, BGPF_WRITEFAILED)) {
	return SEND_FAILED;
    }

    res = bgp_send(bnp, message, mlen, TRUE);

    if (res < 0) {
	BIT_SET(bnp->bgp_flags, BGPF_WRITEFAILED);
	return SEND_FAILED;
    }

    bnp->bgp_out_updates++;
    if (res == 0) {
	return SEND_BLOCKED;
    }
    return SEND_OK;
}


/*
 * bgp_rt_send_v4_message_adjust - move the version 4 header to
 *				   the start of the message.
 */
static void
bgp_rt_send_v4_message_adjust __PF0(void)
{
    register byte *cp;
    register int moved;
    register size_t hdrlen;

    /*
     * Don't move it if we don't need to.
     */
    cp = SEND_ATTR_V4;
    if (send_v4_hdr_start == NULL || send_v4_hdr_start == cp) {
	return;
    }

    /*
     * Move it up in the packet, adjust the start and end pointers.
     * Adjust the next hop pointer if we have one.
     */
    moved = send_v4_hdr_start - cp;
    hdrlen = send_v4_hdr_end - send_v4_hdr_start;
    bcopy((void_t) send_v4_hdr_start, (void_t) cp, hdrlen);
    send_v4_hdr_start = cp;
    send_v4_hdr_end = cp + hdrlen;
    send_v4_next_route = send_v4_hdr_end;
    send_v4_next_hop -= moved;

    /*
     * Zero out unreachable length
     */
    *(--cp) = 0;
    *(--cp) = 0;
}


/*
 * bgp_rt_send_v4_prepare - prepare the v4 send packet to be sent
 */
static size_t
bgp_rt_send_v4_prepare __PF1(hdrlen, int *)
{
    register byte *cp;
    register size_t len;

    *hdrlen = 0;

    if (send_v4_hdr_start == NULL) {
	/*
	 * Message is entirely unreachables.  Mark that there
	 * are no attributes in the packet and put the length of the
	 * unreachables at the start of the packet.  Then compute
	 * the total length of the message;
	 */
	len = send_v4_next_route - SEND_ATTR_V4;
	if (len == 0) {
	    /* XXX shouldn't happen */
	    return (0);
	}
	cp = SEND_START_V4;
	BGP_PUT_SHORT(len, cp);

	cp = send_v4_next_route;	/* Zero attributes length */
	*cp++ = 0; *cp++ = 0;
	len = cp - send_v4_message;
    } else {
	if (send_v4_hdr_end == send_v4_next_route) {
	    /*
	     * No routes in reachable section, respond by
	     * temporarily zeroing out the attributes length
	     * and sending the unreachables (if we have any).
	     */
	    cp = send_v4_hdr_start;
	    if (cp == SEND_ATTR_V4) {
		/* XXX shouldn't happen */
		return (0);
	    } else {
		BGP_GET_SHORT(*hdrlen, cp);
		len = cp - send_v4_message;
		*(--cp) = 0; *(--cp) = 0;
	    }
	} else {
	    len = send_v4_next_route - send_v4_message;
	}
    }

    BGP_PUT_HDRLEN(len, send_v4_message);
    BGP_PUT_HDRTYPE(BGP_UPDATE, send_v4_message);

    return len;
}



/*
 * bgp_rt_send_v4_flush - flush the version 4 packet out to a peer.
 *			  If he's going to be sending more, adjust
 *			  the position of the header upward in the
 *			  packet.
 */
static int
bgp_rt_send_v4_flush __PF2(bnp, bgpPeer *,
			   more, int)
{
    size_t len;
    int res;
    int hdrlen;

    /*
     * Prepare the message for sending
     */
    len = bgp_rt_send_v4_prepare(&hdrlen);
    BGP_ADD_AUTH(&(bnp->bgp_authinfo), send_v4_message, len);

    /*
     * Send it if we need to.
     */
    if (len) {
	res = bgp_rt_send_message(bnp, send_v4_message, len);
    } else {
	res = SEND_OK;
    }

    /*
     * Adjust the packet if there is more to do, initialize it if not
     */
    if (more) {
	if (hdrlen) {
	    *send_v4_hdr_start = (hdrlen >> 8) & 0xff;
	    *(send_v4_hdr_start + 1) = hdrlen & 0xff;
	}
	if (send_v4_hdr_start == NULL) {
	    send_v4_next_route = SEND_ATTR_V4;
	} else if (send_v4_hdr_start == SEND_ATTR_V4) {
	    send_v4_next_route = send_v4_hdr_end;
	} else {
	    bgp_rt_send_v4_message_adjust();
	}
    } else {
	send_v4_hdr_start = send_v4_hdr_end = send_v4_next_hop = (byte *) 0;
	send_v4_next_route = SEND_ATTR_V4;
    }

    return res;
}


static int
bgp_rt_send_v4_group_flush __PF6(bgp, bgpPeerGroup *,
				 sendbits, bgp_bits *,
				 send_type, int,
				 ifap, if_addr *,
				 nexthop, bgp_nexthop,
				 more, int)
{
    register bgpPeer *bnp;
    register int i;
    size_t len;
    int hdrlen;
    int ok = 0;
    int res = SEND_OK;

    /*
     * Prepare the message for sending
     */
    len = bgp_rt_send_v4_prepare(&hdrlen);

    /*
     * If we need to send it, do it now.
     */
    if (len) {
	BGP_PUT_HDRLEN(len, send_v4_message);
	BGP_PUT_HDRTYPE(BGP_UPDATE, send_v4_message);

	if (send_type == SEND_NEXTHOP_LIST && nexthop) {
	    SEND_PUT_NEXTHOP(nexthop, send_v4_next_hop);
	}

	BGP_PEER_LIST(bgp, bnp) {
	    /*
	     * If the peer isn't established, or the group bit isn't set
	     * don't bother.
	     */
	    if (bnp->bgp_state != BGPSTATE_ESTABLISHED
	      || !BGPB_BTEST(sendbits, bnp->bgp_group_bit)) {
		continue;
	    }

	    /*
	     * Set the next hop
	     */
	    if (send_v4_hdr_start && hdrlen == 0) {
		switch(send_type) {
		case SEND_NEXTHOP_NONE:
		    /*
		     * Put the local address in the packet.
		     */
		    SEND_PUT_NH_ADDR(bnp->bgp_myaddr, send_v4_next_hop);
		    break;
		case SEND_NEXTHOP_PEERIF:
		    if (nexthop && ifap == bnp->bgp_ifap) {
			SEND_PUT_NEXTHOP(nexthop, send_v4_next_hop);
		    } else {
			SEND_PUT_NH_ADDR(bnp->bgp_myaddr, send_v4_next_hop);
		    }
		    break;
		case SEND_NEXTHOP_LIST:
		    if (!nexthop) {
			SEND_PUT_NH_ADDR(bnp->bgp_myaddr, send_v4_next_hop);
		    }
		    break;
		default:
		    assert(FALSE);
		}
	    }
	    BGP_ADD_AUTH(&(bnp->bgp_authinfo), send_v4_message, len);

	    i = bgp_rt_send_message(bnp, send_v4_message, len);

	    if (i == SEND_OK) {
		ok++;
	    } else {
		BGPB_BRESET(sendbits, bnp->bgp_group_bit);
		BIT_SET(res, i);
	    }
	} BGP_PEER_LIST_END(bgp, bnp);

	if (ok == 0) {
	    BIT_SET(res, SEND_ALLFAILED);
	}
    }

    /*
     * Adjust the packet if there is more to do, initialize it if not
     */
    if (more) {
	if (hdrlen) {
	    register byte *cp = send_v4_hdr_start;
	    BGP_PUT_SHORT(hdrlen, cp);
	}
	if (send_v4_hdr_start == NULL) {
	    send_v4_next_route = SEND_ATTR_V4;
	} else if (send_v4_hdr_start == SEND_ATTR_V4) {
	    send_v4_next_route = send_v4_hdr_end;
	} else {
	    bgp_rt_send_v4_message_adjust();
	}
    } else {
	send_v4_hdr_start = send_v4_hdr_end = send_v4_next_hop = (byte *) 0;
	send_v4_next_route = SEND_ATTR_V4;
    }

    return res;
}


/*
 * bgp_rt_send_v3_flush - flush the version 3 packet out to a peer.
 *			  Reset the route pointer so the guy can put
 *			  more in the packet.
 */
static int
bgp_rt_send_v3_flush __PF1(bnp, bgpPeer *)
{
    size_t len;
    int res;

    /*
     * Prepare the message for sending
     */
    len = send_v3_next_route - send_v3_message;

    /*
     * Send it if we need to.
     */
    if (send_v3_next_route != send_v3_hdr_end) {
	BGP_PUT_HDRLEN(len, send_v3_message);
	BGP_PUT_HDRTYPE(BGP_UPDATE, send_v3_message);
	BGP_ADD_AUTH(&(bnp->bgp_authinfo), send_v3_message, len);

	res = bgp_rt_send_message(bnp, send_v3_message, len);
	send_v3_next_route = send_v3_hdr_end;
    } else {
	res = SEND_OK;
    }

    return res;
}


static int
bgp_rt_send_v3_group_flush __PF5(bgp, bgpPeerGroup *,
				 sendbits, bgp_bits *,
				 send_type, int,
				 ifap, if_addr *,
				 nexthop, bgp_nexthop)
{
    register bgpPeer *bnp;
    register int i;
    size_t len;
    int ok = 0;
    int res = SEND_OK;

    /*
     * Prepare the message for sending
     */
    len = send_v3_next_route - send_v3_message;

    /*
     * If we need to send it, do it now.
     */
    if (send_v3_next_route != send_v3_hdr_end) {
	BGP_PUT_HDRLEN(len, send_v3_message);
	BGP_PUT_HDRTYPE(BGP_UPDATE, send_v3_message);

	if (send_type == SEND_NEXTHOP_LIST && nexthop) {
	    SEND_PUT_NEXTHOP(nexthop, send_v3_next_hop);
	}
	send_v3_next_route = send_v3_hdr_end;

	BGP_PEER_LIST(bgp, bnp) {
	    /*
	     * If the group bit isn't set, don't bother.  If
	     * he's not established don't bother either.
	     */
	    if (bnp->bgp_state != BGPSTATE_ESTABLISHED
	      || !BGPB_BTEST(sendbits, bnp->bgp_group_bit)) {
		continue;
	    }

	    /*
	     * Set the next hop
	     */
	    switch(send_type) {
	    case SEND_NEXTHOP_NONE:
		/*
		 * Put the local address in the packet.
		 */
		SEND_PUT_NH_ADDR(bnp->bgp_myaddr, send_v3_next_hop);
		break;
	    case SEND_NEXTHOP_PEERIF:
		if (nexthop && ifap == bnp->bgp_ifap) {
		    SEND_PUT_NEXTHOP(nexthop, send_v3_next_hop);
		} else {
		    SEND_PUT_NH_ADDR(bnp->bgp_myaddr, send_v3_next_hop);
		}
		break;
	    case SEND_NEXTHOP_LIST:
		if (!nexthop) {
		    SEND_PUT_NH_ADDR(bnp->bgp_myaddr, send_v3_next_hop);
		}
		break;
	    default:
		assert(FALSE);
	    }
	    BGP_ADD_AUTH(&(bnp->bgp_authinfo), send_v3_message, len);

	    i = bgp_rt_send_message(bnp, send_v3_message, len);

	    if (i == SEND_OK) {
		ok++;
	    } else {
		BGPB_BRESET(sendbits, bnp->bgp_group_bit);
		BIT_SET(res, i);
	    }
	} BGP_PEER_LIST_END(bgp, bnp);

	if (ok == 0) {
	    BIT_SET(res, SEND_ALLFAILED);
	}
    }

    return res;
}


/*
 * bgp_rt_timer_str - pretty print the next expiry timer
 */
static char *
bgp_rt_timer_str __PF2(nexttime, time_t,
		       timeo, time_t)
{
    static char buf[30];

    if (nexttime == 0) {
	(void) strcpy(buf, "no routes deferred");
    } else if ((nexttime + timeo) > bgp_time_sec) {
	(void) sprintf(buf,
		       "next in %d second%s",
		       (nexttime + timeo - bgp_time_sec),
		       (((nexttime + timeo - bgp_time_sec) == 1) ? "" : "s"));
    } else {
	(void) strcpy(buf, "timer fucked!");
    }

    return buf;
}



/*
 * bgp_rt_send_peer - takes a send list for an external peer,
 *		      builds messages from this, and sends each
 *		      message to the peer.  If the transmission
 *		      blocks we queue the remainder of the packet.
 *		      Otherwise we update the time for delayed sends.
 *		      In either case we return an indication of
 *		      what happened so the caller can clean up
 *		      as appropriate.
 */
static int
bgp_rt_send_peer __PF2(bnp, bgpPeer *,
		       isflash, int)
{
    register bgp_rto_entry *rtop;
    register rt_entry *rt;
    register int isversion4 = (bnp->bgp_version == BGP_VERSION_4);
    bgp_rto_entry *rtop_next;
    bgp_rto_entry *rtop_check_next;
    bgp_asp_list *aspl;
    bgp_adv_entry *entp;
    time_t nexttime, send_to;
    as_path_info api;
    bgp_metrics *mets;
    sockaddr_un nhaddr;
    u_int rtbit = bnp->bgp_task->task_rtbit;
    flag_t aspath_flags;
    int send_buf_initialized = 0;
    int bclen;
    int error = SEND_OK;
    int sent = 0;
    int sendnextflash = 0;

    bnp->bgp_rto_next_time = 0;
    aspl = BGP_ASPL_FIRST(&(bnp->bgp_asp_queue));
    if (!aspl) {
	return (SEND_OK);
    }

    /*
     * Log that we're doing this
     */
    trace_tp(bnp->bgp_task,
	     TR_NORMAL,
	     0,
	     ("bgp_rt_send_peer: sending to %s",
	      bnp->bgp_name));

    /*
     * Figure out the time on routes we are allowed to send.
     */
    if (bgp_time_sec <= bnp->bgp_rto_time) {
	send_to = (time_t) 0;
    } else {
	send_to = bgp_time_sec - bnp->bgp_rto_time;
    }
    nexttime = 0;

    /*
     * Put the router ID in the path info unless we're configured not
     * to.
     */
    if (BIT_TEST(bnp->bgp_options, BGPO_NOAGGRID)) {
	aspath_flags = APIF_NEXTHOP;
    } else {
	aspath_flags = APIF_LOCALID|APIF_NEXTHOP;
	api.api_localid = bnp->bgp_out_id;
    }

    /*
     * Initialize nhaddr in case we need it.
     */
    sockclear_in(&nhaddr);

    /*
     * First process unreachables if there are any.
     */
    if (aspl->bgpl_asp == NULL) {
	for (rtop = aspl->bgpl_rto_next;
	  rtop != (bgp_rto_entry *)aspl;
	  rtop = rtop_next) {
	    rtop_next = rtop->bgpo_next;

	    /*
	     * Check to see if we're into future changes.
	     * If so, break out of this.
	     */
	    if (send_to < rtop->bgpo_time) {
		if (nexttime == 0 || nexttime > rtop->bgpo_time) {
		    nexttime = rtop->bgpo_time;
		}
		break;
	    }

	    /*
	     * Check to see if this route is on a flash list.
	     * If so ignore it, we'll pick up changes at
	     * the next flash.
	     */
	    entp = rtop->bgpo_advrt;
	    rt = entp->bgpe_rt;
	    if (!isflash && BIT_TEST(rt->rt_head->rth_state, RTS_ONLIST)) {
		sendnextflash++;
		continue;
	    }

	    /*
	     * We're going to send this.  If this is the first
	     * unreachable we've had to send, open the routing
	     * table so we can reset the rtbit and then initialize
	     * the send buffer.
	     */
	    if  (!send_buf_initialized) {
		send_buf_initialized = 1;
		rt_open(bnp->bgp_task);
		if (isversion4) {
		    bgp_rt_send_v4_unreachable_init();
		} else {
		    bgp_rt_send_v3_unreachable_init(
		      bnp->bgp_group->bgpg_local_as,
		      bnp->bgp_myaddr, 0);
		}
	    }

	    /*
	     * Write the route into the packet.  Flush the buffer first
	     * if there is insufficient room.
	     */
	    if (isversion4) {
		bclen = inet_prefix_mask(rt->rt_dest_mask);
		if (SEND_V4_UNREACH_PKT_FULL(bclen)) {
		    if ((error = bgp_rt_send_v4_flush(bnp, 1)) != SEND_OK) {
			rt_close(bnp->bgp_task, &bnp->bgp_gw, 0, NULL);
			goto oh_shit;
		    }
		}
		SEND_PREFIX(rt->rt_dest, bclen);
	    } else {
		if (SEND_V3_PKT_FULL()) {
		    if ((error = bgp_rt_send_v3_flush(bnp)) != SEND_OK) {
			rt_close(bnp->bgp_task, &bnp->bgp_gw, 0, NULL);
			goto oh_shit;
		    }
		}
		SEND_ADDR(rt->rt_dest);
	    }
	    sent++;

	    /*
	     * Route is safely in packet.  Switch off the rtbit and
	     * free the rto_entry.
	     */
	    BRT_TSI_CLEAR(rt->rt_head, rtbit);
	    (void) rtbit_reset(rt, rtbit);
	    BGP_RTO_UNLINK(rtop);
	    BGP_ADV_DEQUEUE(entp);
	    BRT_ENT_FREE(entp);
	    BRT_RTO_FREE(rtop);
	}

	/*
	 * See if anything was sent.  If so close the routing table and,
	 * if this is a version 3 peer, flush the packet.
	 */
	if (send_buf_initialized) {
	    rt_close(bnp->bgp_task, &bnp->bgp_gw, 0, NULL);
	    if (!isversion4) {
		if ((error = bgp_rt_send_v3_flush(bnp)) != SEND_OK) {
		    goto oh_shit;
		}
		send_buf_initialized = 0;
	    }
	}

	/*
	 * Check to see if we entirely exhausted the list.  If so,
	 * dump the list entry as well.  If not, point to the next
	 * list entry.
	 */
	if (BGP_ASPL_EMPTY(aspl)) {
	    aspl->bgpl_q_next->bgpq_prev = aspl->bgpl_q_prev;
	    bnp->bgp_asp_first = aspl->bgpl_q_next;
	    BRT_ASPL_FREE(aspl);
	    aspl = BGP_ASPL_FIRST(&(bnp->bgp_asp_queue));
	} else {
	    aspl = BGP_ASPL_NEXT(&(bnp->bgp_asp_queue), aspl);
	}
    }

    /*
     * So far so good.  Here we have reachable routing information
     * to send.  For each AS path find routes with the same outgoing
     * metric and send them.
     */
    while (aspl != NULL) {
	for (rtop = aspl->bgpl_rto_next;
	  rtop != (bgp_rto_entry *)aspl;
	  rtop = rtop_next) {
	    rtop_next = rtop->bgpo_next;

	    /*
	     * See if we've run out of on-time routes.  If so
	     * we're done with this path.
	     */
	    if (send_to < rtop->bgpo_time) {
		if (nexttime == 0 || nexttime > rtop->bgpo_time) {
		    nexttime = rtop->bgpo_time;
		}
		break;
	    }

	    /*
	     * See if this route is being flashed.  If so, leave it
	     * for now.
	     */
	    entp = rtop->bgpo_advrt;
	    rt = entp->bgpe_rt;
	    if (!isflash && BIT_TEST(rt->rt_head->rth_state, RTS_ONLIST)) {
		sendnextflash++;
		continue;
	    }
	    assert(rt->rt_aspath == aspl->bgpl_asp);

	    /*
	     * Here we've got a route we need to send.  Initialize
	     * his path attributes.
	     */
	    mets = rtop->bgpo_new_metrics;
	    api.api_flags = aspath_flags;
	    if (mets->bgpm_tmetric) {
		BIT_SET(api.api_flags, APIF_METRIC);
		api.api_metric = mets->bgpm_metric;
	    }
	    if (mets->bgpm_nexthop) {
		sock2ip(&nhaddr) = mets->bgpm_nexthop;
		api.api_nexthop = &nhaddr;
	    } else {
		api.api_nexthop = bnp->bgp_myaddr;
	    }

	    /*
	     * Done that, now set up the path attribute header.  If
	     * this is version 4 we may need to flush out the
	     * unreachables if the new header won't fit.  Write the
	     * route into the packet.
	     */
	    if (isversion4) {
		if (send_buf_initialized) {
		    if (bgp_rt_send_v4_message_attr(
			bnp->bgp_group->bgpg_local_as,
			rt->rt_aspath, &api, 1) != SEND_OK) {
			if ((error = bgp_rt_send_v4_flush(bnp, 1)) != SEND_OK) {
			    goto oh_shit;
			}
			(void) bgp_rt_send_v4_message_attr(
						bnp->bgp_group->bgpg_local_as,
						rt->rt_aspath,
						&api,
						0);
		    }
		    send_buf_initialized = 0;
		} else {
		    (void) bgp_rt_send_v4_message_attr(
		    				bnp->bgp_group->bgpg_local_as,
		    				rt->rt_aspath,
						&api,
						0);
		}
		bclen = inet_prefix_mask(rt->rt_dest_mask);
		SEND_PREFIX(rt->rt_dest, bclen);
	    } else {
		bgp_rt_send_v3_message_attr(bnp->bgp_group->bgpg_local_as,
					    rt->rt_aspath,
					    &api);
		SEND_ADDR(rt->rt_dest);
	    }
	    sent++;

	    /*
	     * Route safely into packet, remove it from the list.
	     */
	    BGP_RTO_UNLINK(rtop);
	    BIT_RESET(entp->bgpe_flags, BGPEF_QUEUED);
	    BRT_TSI_PUT_ENT(rt->rt_head, rtbit, entp);
	    BRT_RTO_FREE(rtop);
	    rtop = rtop_next;

	    /*
	     * Now go around looking for routes with a matching
	     * metric and next hop.  Add every one which is found
	     * to the list.
	     */
	    for ( ; rtop != (bgp_rto_entry *) aspl
	      && bgp_time_sec >= rtop->bgpo_time;
	      rtop = rtop_check_next) {
		rtop_check_next = rtop->bgpo_next;

		/*
		 * If the route is on the flash list, ignore it.
		 * If the metric isn't the same, skip it.
		 */
		entp = rtop->bgpo_advrt;
		rt = entp->bgpe_rt;
		if (!isflash && BIT_TEST(rt->rt_head->rth_state, RTS_ONLIST)) {
		    sendnextflash++;
		    continue;
		}
		if (rtop->bgpo_new_metrics != mets) {
		    continue;
		}
		assert(rt->rt_aspath == aspl->bgpl_asp);

		/*
		 * Same metrics and next hop.  Put the route in the
		 * packet.
		 */
		if (isversion4) {
		    bclen = inet_prefix_mask(rt->rt_dest_mask);
		    if (SEND_V4_UNREACH_PKT_FULL(bclen)) {
			if ((error = bgp_rt_send_v4_flush(bnp, TRUE)) != SEND_OK) {
			    goto oh_shit;
		        }
		    }
		    SEND_PREFIX(rt->rt_dest, bclen);
		} else {
		    if (SEND_V3_PKT_FULL()) {
			if ((error = bgp_rt_send_v3_flush(bnp)) != SEND_OK) {
			    goto oh_shit;
			}
		    }
		    SEND_ADDR(rt->rt_dest);
		}
		sent++;

		/*
		 * Route safely into packet, remove it from the list.
		 */
		if (rtop == rtop_next) {
		    rtop_next = rtop_check_next;
		}
		BGP_RTO_UNLINK(rtop);
		BIT_RESET(entp->bgpe_flags, BGPEF_QUEUED);
		BRT_TSI_PUT_ENT(rt->rt_head, rtbit, entp);
		BRT_RTO_FREE(rtop);
	    }

	    /*
	     * If we're here we got at least one sensible route into
	     * the packet.  Flush the stupid thing out.
	     */
	    if (isversion4) {
		if ((error = bgp_rt_send_v4_flush(bnp, FALSE)) != SEND_OK) {
		    break;
		}
	    } else {
		if ((error = bgp_rt_send_v3_flush(bnp)) != SEND_OK) {
		    break;
		}
	    }

	    /*
	     * Done with that metric/nexthop, go around again.
	     */
	}

	/*
	 * Here we've finished announcing all routes with this AS path
	 * which were ready to go.  If we've announced all routes with
	 * the AS path, remove the list entry.  Otherwise update the
	 * previous pointer.  In either case, aspl ends up pointing
	 * at the next entry.
	 */
	if (BGP_ASPL_EMPTY(aspl)) {
	    bgp_asp_list *aspl_next = BGP_ASPL_NEXT(&(bnp->bgp_asp_queue), aspl);

	    BGP_ASPL_REMOVE(&(bnp->bgp_asp_queue), aspl);
	    BRT_ASPL_FREE(aspl);
	    aspl = aspl_next;
	} else {
	    aspl = BGP_ASPL_NEXT(&(bnp->bgp_asp_queue), aspl);
	}

	if (error != SEND_OK) {
	    goto oh_shit;
	}
    }

    /*
     * Here we've done all we possibly can.  There may, however, be
     * some left over unreachables in the version 4 packet.  Send these
     * now.
     */
    if (isversion4 && send_buf_initialized) {
	if ((error = bgp_rt_send_v4_flush(bnp, FALSE)) != SEND_OK) {
	    goto oh_shit;
	}
    }

    /*
     * All written, no errors.  We need to update the timer to the
     * time of the next route which needs to be written.  Do that now.
     */
    if (sendnextflash) {
	trace_tp(bnp->bgp_task,
		 TR_NORMAL,
		 0,
		 ("bgp_rt_send_peer: peer %s sent %d route%s %s %d awaiting flash",
		  bnp->bgp_name,
		  sent,
		  ((sent == 1) ? "" : "s"),
		  bgp_rt_timer_str(nexttime, bnp->bgp_rto_time),
		  sendnextflash));
    } else {
	trace_tp(bnp->bgp_task,
		 TR_NORMAL,
		 0,
		 ("bgp_rt_send_peer: peer %s sent %d route%s %s",
		  bnp->bgp_name,
		  sent,
		  ((sent == 1) ? "" : "s"),
		  bgp_rt_timer_str(nexttime, bnp->bgp_rto_time)));
    }
    bnp->bgp_rto_next_time = nexttime;
    return SEND_OK;

oh_shit:
    /*
     * Here we got some kind of error.  If it was a block
     * we'll have fallen back to the send routine to move
     * more routing information, so zero the timer to make
     * sure any pending timer interrupt doesn't try to write
     * more.
     */
    trace_tp(bnp->bgp_task,
	     TR_NORMAL,
	     0,
	     ("bgp_rt_send_peer: peer %s %s after %d route%s sent",
	      bnp->bgp_name,
	      (BIT_TEST(error, SEND_FAILED) ? "write failed" : "write blocked"),
	      sent,
	      ((sent == 1) ? "" : "s")));
    bnp->bgp_rto_next_time = 0;
    return error;
}


/*
 * bgp_rt_send_group_peer - takes a send list for a peer group
 *		and flushes all routing information for a single
 *		peer.  This routine is normally used to flush
 *		the routing information for a peer which has blocked,
 *		or which has just been started, and is sending
 *		information from his task_write() routine, but may
 *		also be used for a group with only one established
 *		peer since it is cheaper to do it this way.
 *
 *		This is a whole lot like bgp_rt_send_peer() above.
 */
static int
bgp_rt_send_group_peer __PF2(bnp, bgpPeer *,
			     isflash, int)
{
    register bgpPeerGroup *bgp = bnp->bgp_group;
    register bgpg_rto_entry *rtop;
    register bgpg_rtinfo_entry *infop;
    register rt_entry *rt;
    register u_int word, wbit;
    int isversion4 = (bnp->bgp_version == BGP_VERSION_4);
    int isinternal = BGP_GROUP_INTERNAL(bgp);
    bgpg_rto_entry *rtop_next;
    bgpg_rto_entry *rtop_check_next;
    bgp_asp_list *aspl;
    bgpg_rtinfo_entry *info_prev;
    bgp_adv_entry *entp;
    time_t nexttime, send_to;
    as_path_info api;
    bgp_nexthop thisnexthop;
    int nexthoptype;
    bgp_metrics *his_metrics;
    u_int rtbit = bgp->bgpg_task->task_rtbit;
    int rttableopen = 0;
    flag_t aspath_flags;
    int send_buf_initialized = 0;
    sockaddr_un mynexthop;
    int bclen, res;
    int error = SEND_OK;
    int sendnextflash = 0;
    int sent = 0;

    /*
     * Log that we're doing this
     */
    trace_tp(bnp->bgp_task,
	     TR_NORMAL,
	     0,
	     ("bgp_rt_send_group_peer: sending to %s",
	      bnp->bgp_name));

    /*
     * Put the router ID in the path info unless we're configured not
     * to.
     */
    if (BIT_TEST(bnp->bgp_options, BGPO_NOAGGRID)) {
	aspath_flags = APIF_NEXTHOP;
    } else {
	aspath_flags = APIF_LOCALID|APIF_NEXTHOP;
	api.api_localid = bnp->bgp_out_id;
    }
    if (isinternal) {
	BIT_SET(aspath_flags, APIF_INTERNAL);
    }

    /*
     * Figure out the age of routes we're supposed to send.
     */
    if (bgp_time_sec <= bgp->bgpg_rto_time) {
	send_to = (time_t) 0;
    } else {
	send_to = bgp_time_sec - bgp->bgpg_rto_time;
    }
    nexttime = 0;

    /*
     * Make sure he has a bit.  Split the bit out into its components
     * to speed checks.
     */
    word = BGPB_WORD(bnp->bgp_group_bit);
    wbit = BGPB_WBIT(bnp->bgp_group_bit);

    /*
     * First process unreachables if there are any.
     */
    if ((aspl = BGP_ASPL_FIRST(&(bgp->bgpg_asp_queue))) == NULL) {
	bgp->bgpg_rto_next_time = 0;
	return SEND_OK;
    }

    if (aspl->bgpl_asp == NULL) {
	for (rtop = aspl->bgpl_grto_next;
	  rtop != (bgpg_rto_entry *)aspl;
	  rtop = rtop_next) {
	    rtop_next = rtop->bgpgo_next;

	    /*
	     * Check to see if we're into future changes.
	     * If so, break out of this.
	     */
	    if (send_to < rtop->bgpgo_time) {
		if (nexttime == 0 || nexttime > rtop->bgpgo_time) {
		    nexttime = rtop->bgpgo_time;
		}
		break;
	    }

	    /*
	     * Check to see if this route is on a flash list.
	     * If so ignore it, we'll pick up changes at
	     * the next flash.
	     */
	    entp = rtop->bgpgo_advrt;
	    rt = entp->bgpe_rt;
	    if (!isflash && BIT_TEST(rt->rt_head->rth_state, RTS_ONLIST)) {
		sendnextflash++;
		continue;
	    }

	    /*
	     * Check to see if this guy's bit is set in an info entry.
	     * If so we get to send it.  If not just ignore this, someone
	     * else will send it.
	     */
	    for (infop = rtop->bgpgo_info, info_prev = NULL; infop != NULL;
	      infop = infop->bgp_info_next) {
		if (BGPB_WB_TEST(infop->bgp_info_bits, word, wbit)) {
		    break;
		}
		info_prev = infop;
	    }
	    if (infop == NULL) {
		continue;
	    }

	    /*
	     * We're going to send this.  If this is the first
	     * unreachable we've had to send, initialize
	     * the send buffer.
	     */
	    if  (!send_buf_initialized) {
		send_buf_initialized = 1;
		if (isversion4) {
		    bgp_rt_send_v4_unreachable_init();
		} else {
		    bgp_rt_send_v3_unreachable_init(bgp->bgpg_local_as,
						    bnp->bgp_myaddr,
						    isinternal);
		}
	    }

	    /*
	     * Write the route into the packet.  Flush the buffer first
	     * if there is insufficient room.
	     */
	    if (isversion4) {
		bclen = inet_prefix_mask(rt->rt_dest_mask);
		if (SEND_V4_UNREACH_PKT_FULL(bclen)) {
		    if ((error = bgp_rt_send_v4_flush(bnp, TRUE)) != SEND_OK) {
			goto oh_shit;
		    }
		}
		SEND_PREFIX(rt->rt_dest, bclen);
	    } else {
		if (SEND_V3_PKT_FULL()) {
		    if ((error = bgp_rt_send_v3_flush(bnp)) != SEND_OK) {
			goto oh_shit;
		    }
		}
		SEND_ADDR(rt->rt_dest);
	    }
	    sent++;

	    /*
	     * Route is safely in packet.  Switch off the peer bit in
	     * the info structure.  If this was the only bit set, delete
	     * the info structure.  If this was the only info structure
	     * in the rto entry, reset the rtbit and delete the rto entry.
	     */
	    BGPB_WB_RESET(infop->bgp_info_bits, word, wbit);
	    BGPB_CHECK_CLEAR(infop->bgp_info_bits, bgp->bgpg_idx_size, res);
	    if (res) {
		if (info_prev == NULL) {
		    rtop->bgpgo_info = infop->bgp_info_next;
		    if (rtop->bgpgo_info == NULL) {
			if (rttableopen == 0) {
			    rttableopen = 1;
			    rt_open(bnp->bgp_task);
			}
			BRT_TSI_CLEAR(rt->rt_head, rtbit);
			(void) rtbit_reset(rt, rtbit);
			BGP_GRTO_UNLINK(rtop);
			BGP_ADV_DEQUEUE(entp);
			BRT_ENT_FREE(entp);
			BRT_GRTO_FREE(rtop);
			rtop = NULL;
		    }
		} else {
		    info_prev->bgp_info_next = infop->bgp_info_next;
		}
		BRT_INFO_FREE(bgp, infop);
	    }
	}

	/*
	 * See if anything was sent.  If so close the routing table
	 * if we need to and, * if this is a version 3 peer, flush
	 * the packet.
	 */
	if (send_buf_initialized) {
	    if (rttableopen) {
		rt_close(bnp->bgp_task, (gw_entry *)0, 0, NULL);
		rttableopen = 0;
	    }
	    if (!isversion4) {
		if ((error = bgp_rt_send_v3_flush(bnp)) != SEND_OK) {
		    goto oh_shit;
		}
		send_buf_initialized = 0;
	    }
	}

	/*
	 * Check to see if we entirely exhausted the list.  If so,
	 * dump the list entry as well.  If not, point to the next
	 * list entry.
	 */
	if (BGP_ASPL_EMPTY(aspl)) {
	    aspl->bgpl_q_next->bgpq_prev = aspl->bgpl_q_prev;
	    bgp->bgpg_asp_first = aspl->bgpl_q_next;
	    BRT_ASPL_FREE(aspl);
	    aspl = BGP_ASPL_FIRST(&(bgp->bgpg_asp_queue));
	} else {
	    aspl = BGP_ASPL_NEXT(&(bgp->bgpg_asp_queue), aspl);
	}
    }

    /*
     * So far so good.  Here we have reachable routing information
     * to send (if not we would have gone to alldone above).  Figure
     * out how we need deal with next hops.  The choices are (1) ignore
     * next hops, always use the local address, (2) send a third party
     * next hop if the next hop is on the peer's shared interface, and
     * (3) send a third party next hop if the next hop interface is in
     * the interface list.  Then for each AS path find routes with the same
     * outgoing metric and send them.
     */
    switch (bgp->bgpg_type) {
    default:
	assert(FALSE);
    case BGPG_TEST:
    case BGPG_INTERNAL:
	nexthoptype = SEND_NEXTHOP_PEERIF;
	sockclear_in(&mynexthop);
	break;
    case BGPG_INTERNAL_IGP:
	nexthoptype = SEND_NEXTHOP_NONE;
	break;
    case BGPG_INTERNAL_RT:
	nexthoptype = SEND_NEXTHOP_LIST;
	sockclear_in(&mynexthop);
	break;
    }

    while (aspl != NULL) {
	for (rtop = aspl->bgpl_grto_next;
	  rtop != (bgpg_rto_entry *)aspl;
	  rtop = rtop_next) {
	    rtop_next = rtop->bgpgo_next;

	    /*
	     * See if we've run out of on-time routes.  If so
	     * we're done with this path.
	     */
	    if (send_to < rtop->bgpgo_time) {
		if (nexttime == 0 || nexttime > rtop->bgpgo_time) {
		    nexttime = rtop->bgpgo_time;
		}
		break;
	    }

	    /*
	     * See if this route is being flashed.  If so, leave it
	     * for now.
	     */
	    entp = rtop->bgpgo_advrt;
	    rt = entp->bgpe_rt;
	    if (!isflash && BIT_TEST(rt->rt_head->rth_state, RTS_ONLIST)) {
		sendnextflash++;
		continue;
	    }

	    /*
	     * Check to see if this guy's bit is set in an info entry.
	     * If so we get to send it.  If not ignore the route.
	     */
	    for (infop = rtop->bgpgo_info, info_prev = NULL; infop != NULL;
	      infop = infop->bgp_info_next) {
		if (BGPB_WB_TEST(infop->bgp_info_bits, word, wbit)) {
		    break;
		}
		info_prev = infop;
	    }
	    if (infop == NULL) {
		continue;
	    }

	    /*
	     * Here we've got a route we need to send.  Initialize
	     * his path attributes.  Note that version 3 internal
	     * peers use localpref as their metric, while external
	     * peers use metric as their metric.  Version 4 internal
	     * peers must include a localpref, even if none came from
	     * policy.
	     */
	    his_metrics = entp->bgpe_metrics;

	    api.api_flags = aspath_flags;
	    if (isversion4) {
		if (his_metrics->bgpm_tmetric) {
		    api.api_metric = his_metrics->bgpm_metric;
		    BIT_SET(api.api_flags, APIF_METRIC);
		}
		if (isinternal) {
		    BIT_SET(api.api_flags, APIF_LOCALPREF);
		    if (his_metrics->bgpm_tlocalpref) {
			api.api_localpref = his_metrics->bgpm_localpref;
		    } else {
			api.api_localpref = BGP_DEF_LOCALPREF;
		    }
		}
	    } else {
		if (isinternal) {
		    if (his_metrics->bgpm_tlocalpref) {
			api.api_metric
			  = BGP_METRIC_4TO3(his_metrics->bgpm_localpref);
			BIT_SET(api.api_flags, APIF_METRIC);
		    } else if (bgp->bgpg_type == BGPG_INTERNAL_RT) {
			api.api_metric = BGP_DEF_V3METRIC;
			BIT_SET(api.api_flags, APIF_METRIC);
		    }
		} else if (his_metrics->bgpm_tmetric) {
		    api.api_metric = his_metrics->bgpm_metric;
		    BIT_SET(api.api_flags, APIF_METRIC);
		}
	    }

	    res = 0;
	    if (his_metrics->bgpm_nexthop) {
		if (nexthoptype == SEND_NEXTHOP_LIST) {
		    res = 1;
		} else {
		    assert(nexthoptype == SEND_NEXTHOP_PEERIF);
		    if (bnp->bgp_ifap == RT_IFAP(rt)) {
			res = 1;
		    }
		}
	    }

	    if (res) {
		sock2ip(&mynexthop) = thisnexthop = his_metrics->bgpm_nexthop;
		api.api_nexthop = &mynexthop;
	    } else {
		thisnexthop = (bgp_nexthop) 0;
		api.api_nexthop = bnp->bgp_myaddr;
	    }

	    /*
	     * Done that, now set up the path attribute header.  If
	     * this is version 4 we may need to flush out the
	     * unreachables if the new header won't fit.  Write the
	     * route into the packet.
	     */
	    if (isversion4) {
		if (send_buf_initialized) {
		    if (bgp_rt_send_v4_message_attr(bgp->bgpg_local_as,
		      rt->rt_aspath, &api, 1) != SEND_OK) {
			if ((error = bgp_rt_send_v4_flush(bnp, TRUE)) != SEND_OK) {
			    goto oh_shit;
			}
			(void) bgp_rt_send_v4_message_attr(bgp->bgpg_local_as,
			  rt->rt_aspath, &api, 0);
		    }
		    send_buf_initialized = 0;
		} else {
		    (void) bgp_rt_send_v4_message_attr(
		      bnp->bgp_group->bgpg_local_as, rt->rt_aspath,
		      &api, 0);
		}
		bclen = inet_prefix_mask(rt->rt_dest_mask);
		SEND_PREFIX(rt->rt_dest, bclen);
	    } else {
		bgp_rt_send_v3_message_attr(bnp->bgp_group->bgpg_local_as,
		  rt->rt_aspath, &api);
		SEND_ADDR(rt->rt_dest);
	    }
	    sent++;

	    /*
	     * Route safely into packet, reset our bit.  If doing
	     * so clears the bit mask, delete the info structure.
	     * If this is the last remaining info structure, delete
	     * the rtop.
	     */
	    BGPB_WB_RESET(infop->bgp_info_bits, word, wbit);
	    BGPB_CHECK_CLEAR(infop->bgp_info_bits, bgp->bgpg_idx_size, res);
	    if (res) {
		if (info_prev == NULL) {
		    rtop->bgpgo_info = infop->bgp_info_next;
		    if (rtop->bgpgo_info == NULL) {
			BGP_GRTO_UNLINK(rtop);
			BIT_RESET(entp->bgpe_flags, BGPEF_QUEUED);
			BRT_TSI_PUT_ENT(rt->rt_head, rtbit, entp);
			BRT_GRTO_FREE(rtop);
			rtop = NULL;
		    }
		} else {
		    info_prev->bgp_info_next = infop->bgp_info_next;
		}
		BRT_INFO_FREE(bgp, infop);
	    }

	    /*
	     * Now go around looking for routes with matching
	     * metrics and next hop.  Add every one which is found
	     * to the list.
	     */
	    for (rtop = rtop_next; rtop != (bgpg_rto_entry *) aspl
	      && bgp_time_sec >= rtop->bgpgo_time;
	      rtop = rtop_check_next) {
		rtop_check_next = rtop->bgpgo_next;

		/*
		 * If the route is on the flash list, ignore it.
		 */
		entp = rtop->bgpgo_advrt;
		rt = entp->bgpe_rt;
		if (!isflash && BIT_TEST(rt->rt_head->rth_state, RTS_ONLIST)) {
		    sendnextflash++;
		    if (rtop == rtop_next) {
			rtop_next = rtop_check_next;
		    }
		    continue;
		}

		/*
		 * Check to see if this guy's bit is set in an info entry.
		 * If so we get to send it.  If not see if we can ignore this
		 * guy in the outer loop too.
		 */
		for (infop = rtop->bgpgo_info, info_prev = NULL; infop != NULL;
		  infop = infop->bgp_info_next) {
		    if (BGPB_WB_TEST(infop->bgp_info_bits, word, wbit)) {
		        break;
		    }
		    info_prev = infop;
		}
		if (infop == NULL) {
		    if (rtop == rtop_next) {
			rtop_next = rtop_check_next;
		    }
		    continue;
	        }

		/*
		 * Check to see if the metrics are the same.  If not,
		 * forget it.
		 */
		if (entp->bgpe_metrics != his_metrics) {
		    register bgp_metrics *m = entp->bgpe_metrics;

		    if (isversion4 || !isinternal) {
			if (nexthoptype != SEND_NEXTHOP_PEERIF
			  || thisnexthop) {
			    continue;
			}
			if (bnp->bgp_ifap == RT_IFAP(rt)) {
			    continue;
			}
			if (m->bgpm_types != his_metrics->bgpm_types
			  || m->bgpm_metric != his_metrics->bgpm_metric
			  || m->bgpm_localpref != his_metrics->bgpm_localpref) {
			    continue;
			}
		    } else {
			if (m->bgpm_tlocalpref != his_metrics->bgpm_tlocalpref
			  || m->bgpm_localpref != his_metrics->bgpm_localpref) {
			    continue;
			}
			if (thisnexthop != his_metrics->bgpm_nexthop) {
			    if (nexthoptype != SEND_NEXTHOP_PEERIF
			      || thisnexthop) {
				continue;
			    }
			    if (bnp->bgp_ifap == RT_IFAP(rt)) {
				continue;
			    }
			}
		    }
		}

		/*
		 * Same metrics and next hop.  Put the route in the
		 * packet.
		 */
		if (isversion4) {
		    bclen = inet_prefix_mask(rt->rt_dest_mask);
		    if (SEND_V4_PKT_FULL(bclen)) {
			if ((error = bgp_rt_send_v4_flush(bnp, TRUE)) != SEND_OK) {
			    goto oh_shit;
		        }
		    }
		    SEND_PREFIX(rt->rt_dest, bclen);
		} else {
		    if (SEND_V3_PKT_FULL()) {
			if ((error = bgp_rt_send_v3_flush(bnp)) != SEND_OK) {
			    goto oh_shit;
			}
		    }
		    SEND_ADDR(rt->rt_dest);
		}
		sent++;

		/*
		 * Route safely into packet, remove it from the list.
		 */
		if (rtop == rtop_next) {
		    rtop_next = rtop_check_next;
		}

		/*
		 * Route safely into packet, reset our bit.  If doing
		 * so clears the bit mask, delete the info structure.
		 * If this is the last remaining info structure, delete
		 * the rtop.
		 */
		BGPB_WB_RESET(infop->bgp_info_bits, word, wbit);
		BGPB_CHECK_CLEAR(infop->bgp_info_bits, bgp->bgpg_idx_size, res);
		if (res) {
		    if (info_prev == NULL) {
			rtop->bgpgo_info = infop->bgp_info_next;
			if (rtop->bgpgo_info == NULL) {
			    BGP_GRTO_UNLINK(rtop);
			    BIT_RESET(entp->bgpe_flags, BGPEF_QUEUED);
			    BRT_TSI_PUT_ENT(rt->rt_head, rtbit, entp);
			    BRT_GRTO_FREE(rtop);
			    rtop = NULL;
			}
		    } else {
			info_prev->bgp_info_next = infop->bgp_info_next;
		    }
		    BRT_INFO_FREE(bgp, infop);
		}
	    }

	    /*
	     * If we're here we got at least one sensible route into
	     * the packet.  Flush the stupid thing out.
	     */
	    if (isversion4) {
		if ((error = bgp_rt_send_v4_flush(bnp, FALSE)) != SEND_OK) {
		    break;
		}
	    } else {
		if ((error = bgp_rt_send_v3_flush(bnp)) != SEND_OK) {
		    break;
		}
	    }

	    /*
	     * Done with that metric/localpref/nexthop, go around again.
	     */
	}

	/*
	 * Here we've finished announcing all routes with this AS path
	 * which were ready to go.  If we've announced all routes with
	 * the AS path, remove the list entry.  Otherwise update the
	 * previous pointer.  In either case, aspl ends up pointing
	 * at the next entry.
	 */
	if (BGP_ASPL_EMPTY(aspl)) {
	    bgp_asp_list *aspl_next = BGP_ASPL_NEXT(&(bgp->bgpg_asp_queue), aspl);

	    BGP_ASPL_REMOVE(&(bgp->bgpg_asp_queue), aspl);
	    BRT_ASPL_FREE(aspl);
	    aspl = aspl_next;
	} else {
	    aspl = BGP_ASPL_NEXT(&(bgp->bgpg_asp_queue), aspl);
	}

	/*
	 * See if we got an error from the last write.  If so, terminate now.
	 */
	if (error != SEND_OK) {
	    goto oh_shit;
	}
    }

    /*
     * Here we've done all we possibly can.  There may, however, be
     * some left over unreachables in the version 4 packet.  Send these
     * now.
     */
    if (isversion4 && send_buf_initialized) {
	if ((error = bgp_rt_send_v4_flush(bnp, FALSE)) != SEND_OK) {
	    goto oh_shit;
	}
    }

    /*
     * All written, no errors.  We need to update the timer to the
     * time of the next route which needs to be written.  Do that now.
     */
    if (sendnextflash) {
	trace_tp(bnp->bgp_task,
		 TR_NORMAL,
		 0,
		 ("bgp_rt_send_group_peer: peer %s sent %d route%s %s %d awaiting flash",
		  bnp->bgp_name,
		  sent,
		  ((sent == 1) ? "" : "s"),
		  bgp_rt_timer_str(nexttime, bgp->bgpg_rto_time),
		  sendnextflash));
    } else {
	trace_tp(bnp->bgp_task,
		 TR_NORMAL,
		 0,
		 ("bgp_rt_send_group_peer: peer %s sent %d route%s %s",
		  bnp->bgp_name,
		  sent,
		  ((sent == 1) ? "" : "s"),
		  bgp_rt_timer_str(nexttime, bgp->bgpg_rto_time)));
    }

    if (bgp->bgpg_rto_next_time == 0) {
	bgp->bgpg_rto_next_time = nexttime;
    }
    return SEND_OK;

oh_shit:
    if (rttableopen) {
	rt_close(bnp->bgp_task, (gw_entry *)0, 0, NULL);
    }

    /*
     * Here we got some kind of error.  If it was a block
     * we'll have fallen back to the send routine to move
     * more routing information, so zero the timer to make
     * sure any pending timer interrupt doesn't try to write
     * more.
     */
    trace_tp(bnp->bgp_task,
	     TR_NORMAL,
	     0,
	     ("bgp_rt_send_group_peer: peer %s %s after %d route%s sent",
	      bnp->bgp_name,
	      (BIT_TEST(error, SEND_FAILED) ? "write failed" : "write blocked"),
	      sent,
	      ((sent == 1) ? "" : "s")));
    return error;
}


/*
 * bgp_rt_send_v4_group - takes a send list for a peer group
 *	and flushes all routing information for version 4 peers
 *	which can be written to.  The routine attempts to build
 *	single packets which can be flooded to as many peers as
 *	possible.
 */
static int
bgp_rt_send_v4_group __PF2(bgp, bgpPeerGroup *,
			   isflash, int)
{
    register bgpg_rto_entry *rtop;
    register bgpg_rtinfo_entry *infop;
    register rt_entry *rt;
    int isinternal = BGP_GROUP_INTERNAL(bgp);
    bgpg_rto_entry *rtop_next;
    bgpg_rto_entry *rtop_check_next;
    bgpg_rto_entry *rtop_unreach;
    bgp_asp_list *aspl;
    bgp_asp_list *aspl_unreach;
    bgpg_rtinfo_entry *info_prev;
    bgp_bits *grbits;
    bgp_bits mybits[BGP_MAXBITS];
    bgp_bits hisbits[BGP_MAXBITS];
    bgp_adv_entry *entp;
    if_addr *ifap;
    time_t nexttime, send_to;
    int len_bits = bgp->bgpg_idx_size;
    as_path_info api;
    bgp_nexthop thisnexthop;
    int nexthoptype;
    bgp_metrics *his_metrics;
    u_int rtbit = bgp->bgpg_task->task_rtbit;
    int rttableopen = 0;
    flag_t aspath_flags;
    int send_buf_initialized = 0;
    int bclen, res;
    int error = SEND_OK;
    int sendnextflash = 0;
    int sent = 0;
    int were_in_sync;

    if (BGP_RTQ_EMPTY(&(bgp->bgpg_asp_queue))) {
	bgp->bgpg_rto_next_time = 0;	/* nothing to do */
	return SEND_OK;
    }

    /*
     * See if there is anything to do.  If not, just return.  If
     * so, fetch the group bits.
     */
    if (bgp->bgpg_n_v4_sync == 0) {
	return SEND_OK;
    }
    grbits = BGPG_GETBITS(bgp->bgpg_v4_sync, bgp->bgpg_idx_size);

    /*
     * Log what we're doing here
     */
    were_in_sync = bgp->bgpg_n_v4_sync;
    trace_tp(bgp->bgpg_task,
	     TR_NORMAL,
	     0,
	     ("bgp_rt_send_v4_group: sending to %d in-sync peers in %s",
	      were_in_sync,
	      bgp->bgpg_name));

    /*
     * Determine which times we can send now, and which need to be
     * deferred.
     */
    if (bgp_time_sec <= bgp->bgpg_rto_time) {
	send_to = (time_t) 0;
    } else {
	send_to = bgp_time_sec - bgp->bgpg_rto_time;
    }
    nexttime = (time_t) 0;

    /*
     * See if there is anything on the list.  If not, return.  See if
     * there are unreachables on the list.  If so we will need to pay
     * attention to them.
     */
    aspl = BGP_ASPL_FIRST(&(bgp->bgpg_asp_queue));
    if (!aspl) {
	return SEND_OK;
    }
    if (aspl->bgpl_asp == NULL) {
	rtop_unreach = aspl->bgpl_grto_next;
	aspl_unreach = aspl;
	aspl = BGP_ASPL_NEXT(&(bgp->bgpg_asp_queue), aspl);
	if (!aspl) {
	    aspl = aspl_unreach;
	}
    } else {
	aspl_unreach = (bgp_asp_list *) 0;
	rtop_unreach = (bgpg_rto_entry *) 0;
    }

    /*
     * Put the router ID in the path info unless we're configured not
     * to.
     */
    if (bgp->bgpg_type == BGPG_TEST
      || BIT_TEST(bgp->bgpg_options, BGPGO_NOAGGRID)) {
	aspath_flags = APIF_NEXTHOP;
    } else {
	aspath_flags = APIF_LOCALID|APIF_NEXTHOP;
	api.api_localid = bgp->bgpg_out_id;
    }
    if (isinternal) {
	BIT_SET(aspath_flags, APIF_INTERNAL);
    }

    /*
     * Figure out how we need deal with next hops.  The choices are
     * (1) ignore next hops, always use the local address, (2) send
     * a third party next hop if the next hop is on the peer's shared
     * interface, and (3) send a third party next hop if the next hop
     * interface is in the interface list.
     *
     * PEERIF groups are dealt with slightly inefficiently if there are
     * members in the group on different interfaces.  This is probably
     * not a big deal.
     */
    switch (bgp->bgpg_type) {
    default:
	assert(0);
    case BGPG_TEST:
    case BGPG_INTERNAL:
	nexthoptype = SEND_NEXTHOP_PEERIF;
	break;
    case BGPG_INTERNAL_IGP:
	nexthoptype = SEND_NEXTHOP_NONE;
	break;
    case BGPG_INTERNAL_RT:
	nexthoptype = SEND_NEXTHOP_LIST;
	break;
    }
    ifap = (if_addr *) 0;

    /*
     * Now go around looking at each path list with an AS path, or
     * at the unreachable list if that is all we have.
     */
    while (aspl) {
	for (rtop = aspl->bgpl_grto_next;
	  rtop != (bgpg_rto_entry *) aspl;
	  rtop = rtop_next) {
	    rtop_next = rtop->bgpgo_next;
	    /*
	     * Check to see if we're into future changes.
	     * If so, break out of this.
	     */
	    if (send_to < rtop->bgpgo_time) {
		if (nexttime == 0 || nexttime > rtop->bgpgo_time) {
		    nexttime = rtop->bgpgo_time;
		}
		break;
	    }

	    /*
	     * If the route is being flashed, leave it for the flash routine
	     */
	    if (!isflash
	      && BIT_TEST(rtop->bgpgo_rt->rt_head->rth_state, RTS_ONLIST)) {
		sendnextflash++;
		continue;
	    }

	    /*
	     * Find the set of bits for the guys we might like to
	     * send to.  We do this or'ing the bits in each info
	     * structure into a set, and then and'ing this with the
	     * group bits.
	     */
	    BGPB_INFOBITS(mybits, rtop, len_bits);
	    BGPB_AND_CHECK(mybits, grbits, len_bits, res);
	    if (!res) {
		continue;		/* nothing to do */
	    }

	    /*
	     * We now know who we wish to send to.  First scan the
	     * unreachables list looking for routes to send along with
	     * this.
	     */
	    if (aspl_unreach) {
		int alldone = 0;
#define	rtopu	rtop_check_next		/* use otherwise unused variable */
		if (aspl == aspl_unreach) {
		    rtop_unreach = rtop;
		}
		for (rtopu = rtop_unreach;
		  rtopu != (bgpg_rto_entry *)aspl_unreach;
		  rtopu = rtop_next) {
		    rtop_next = rtopu->bgpgo_next;

		    /*
		     * If we've reached the end of time, quit.
		     */
		    if (send_to < rtopu->bgpgo_time) {
			if (rtopu == rtop_unreach) {
			    if (nexttime == 0 || nexttime > rtopu->bgpgo_time) {
				nexttime = rtopu->bgpgo_time;
			    }
			    alldone = 1;
			}
			break;
		    }

		    /*
		     * If the route is being flashed, leave it for the flash
		     * routine.
		     */
		    entp = rtopu->bgpgo_advrt;
		    rt = entp->bgpe_rt;
		    if (!isflash
		      && BIT_TEST(rt->rt_head->rth_state, RTS_ONLIST)) {
			sendnextflash++;
			if (rtopu == rtop_unreach) {
			    rtop_unreach = rtop_next;
			}
			continue;
		    }

		    /*
		     * First thing to do is scan the info entries collecting
		     * bits.  See if all of mybits are included in these.
		     * If not, just ignore the entry, we'll get to it later.
		     */
		    BGPB_INFOBITS(hisbits, rtopu, len_bits);
		    BGPB_MBSET(hisbits, mybits, len_bits, res);
		    if (!res) {
			/*
			 * If this is the current unreachable pointer
			 * and there are no group bits set we may skip
			 * it next time round
			 */
			if (rtopu == rtop_unreach) {
			    BGPB_COMMON(hisbits, grbits, len_bits, res);
			    if (!res) {
				rtop_unreach = rtop_next;
			    }
			}
			continue;
		    }

		    /*
		     * Something to send here, put it in the packet
		     */
		    bclen = inet_prefix_mask(rt->rt_dest_mask);
		    if (!send_buf_initialized) {
			bgp_rt_send_v4_unreachable_init();
			send_buf_initialized = 1;
		    } else if (SEND_V4_UNREACH_PKT_FULL(bclen)) {
			error |= bgp_rt_send_v4_group_flush(bgp,
			  mybits, SEND_NEXTHOP_NONE, (if_addr *)0,
			  BGPM_NO_NEXTHOP, 1);
			/*
			 * If everything failed, don't bother doing any more
			 */
			if (BIT_TEST(error, SEND_ALLFAILED)) {
			    break;
			}
		    }
		    SEND_PREFIX(rt->rt_dest, bclen);
		    sent++;

		    /*
		     * Reset the bits for the peers we're sending this to.
		     * Dump any info structures where we've eliminated all
		     * bits.  If the rto structure can go away, do it.
		     */
		    info_prev = NULL;
		    infop = rtopu->bgpgo_info;
		    do {
			BGPB_CLEAR(infop->bgp_info_bits, mybits, len_bits, res);
			if (res) {
			    bgpg_rtinfo_entry *info_tmp = infop->bgp_info_next;

			    if (info_prev == NULL) {
				if (info_tmp == NULL) {
				    if (!rttableopen) {
					rttableopen = 1;
					rt_open(bgp->bgpg_task);
				    }
				    BRT_TSI_CLEAR(rt->rt_head, rtbit);
				    (void) rtbit_reset(rt, rtbit);
				    if (rtopu == rtop_unreach) {
					rtop_unreach = rtop_next;
				    }
				    BGP_GRTO_UNLINK(rtopu);
				    BGP_ADV_DEQUEUE(entp);
				    BRT_ENT_FREE(entp);
				    BRT_GRTO_FREE(rtopu);
				} else {
				    rtopu->bgpgo_info = info_tmp;
				}
			    } else {
				info_prev->bgp_info_next = info_tmp;
			    }
		    	    BRT_INFO_FREE(bgp, infop);
			    infop = info_tmp;
			} else {
			    info_prev = infop;
			    infop = infop->bgp_info_next;
			}
		    } while (infop != NULL);
		}
#undef	rtopu		/* don't need this any more */

		/*
		 * Okay.  That's all the unreachables we're going to
		 * get this time around.  If we've had massive errors,
		 * terminate this.  If we're just doing unreachables,
		 * flush out any pending data.  If we've exhausted the
		 * list, indicate this.
		 */
		if (aspl == aspl_unreach && send_buf_initialized
		  && !BIT_TEST(error, SEND_ALLFAILED)) {
		    send_buf_initialized = 0;
		    error |= bgp_rt_send_v4_group_flush(bgp, mybits,
		      SEND_NEXTHOP_NONE, (if_addr *) 0, BGPM_NO_NEXTHOP, 0);
		}

		if (BIT_TEST(error, SEND_ALLFAILED)) {
		    if (bgp->bgpg_n_v4_sync == 0) {
			goto dead_duck;
		    } else {
			BIT_RESET(error, SEND_ALLFAILED);
		    }
		}

		if (alldone ||
		  rtop_unreach == (bgpg_rto_entry *)aspl_unreach) {
		    /*
		     * Check to see if this aspl is emtpy.  If so, remove it.
		     */
		    if (BGP_ASPL_EMPTY(aspl_unreach)) {
			aspl_unreach->bgpl_q_next->bgpq_prev
			  = aspl_unreach->bgpl_q_prev;
			bgp->bgpg_asp_first = aspl_unreach->bgpl_q_next;
			BRT_ASPL_FREE(aspl_unreach);
		    }
		    if (aspl == aspl_unreach) {
			aspl = aspl_unreach = (bgp_asp_list *) 0;
			break;
		    }
		    aspl_unreach = (bgp_asp_list *) 0;
		    if (rttableopen) {
			rt_close(bgp->bgpg_task, (gw_entry *)0, 0, NULL);
			rttableopen = 0;
		    }
		}

		if (aspl == aspl_unreach) {
		    rtop_next = rtop_unreach;
		    continue;
		} else {
		    rtop_next = rtop->bgpgo_next;
		}
	    }

	    /*
	     * Here we've got a reachable route to send (the continue four
	     * lines up ensures this).  Initialize the packet with the
	     * route's attributes.
	     */
	    entp = rtop->bgpgo_advrt;
	    rt = entp->bgpe_rt;
	    his_metrics = entp->bgpe_metrics;

	    api.api_flags = aspath_flags;
	    if (his_metrics->bgpm_tmetric) {
		api.api_metric = his_metrics->bgpm_metric;
		BIT_SET(api.api_flags, APIF_METRIC);
	    }
	    if (his_metrics->bgpm_tlocalpref) {
		api.api_localpref = his_metrics->bgpm_localpref;
		BIT_SET(api.api_flags, APIF_LOCALPREF);
	    } else if (isinternal) {
		api.api_localpref = BGP_DEF_LOCALPREF;
		BIT_SET(api.api_flags, APIF_LOCALPREF);
	    }
	    api.api_nexthop = (sockaddr_un *) 0;    /* doesn't matter much */

	    /*
	     * Figure out what to do for a next hop.
	     */
	    thisnexthop = his_metrics->bgpm_nexthop;
	    if (nexthoptype == SEND_NEXTHOP_PEERIF) {
		/*
		 * If the next hop is non-zero, determine the interface
		 * this next hop is on.
		 */
		ifap = (if_addr *) 0;
		if (thisnexthop) {
		    ifap = RT_IFAP(rt);
		    assert(ifap);
		}
	    }

	    /*
	     * We know everything we need to now.  Shove it all in the
	     * packet.  Note that the message may already contain
	     * unreachables from above.
	     */
	    if (send_buf_initialized) {
		if (bgp_rt_send_v4_message_attr(bgp->bgpg_local_as,
		  rt->rt_aspath, &api, 1) != SEND_OK) {
		    send_buf_initialized = 0;
		    error |= bgp_rt_send_v4_group_flush(bgp,
		      mybits, SEND_NEXTHOP_NONE, (if_addr *)0,
		      BGPM_NO_NEXTHOP, 1);
		    if (BIT_TEST(error, SEND_ALLFAILED)) {
			if (bgp->bgpg_n_v4_sync == 0) {
			    goto dead_duck;
			} else {
			    BIT_RESET(error, SEND_ALLFAILED);
			}
			continue;
		    }
		}
	    }

	    bclen = inet_prefix_mask(rt->rt_dest_mask);
	    if (!send_buf_initialized) {
		(void) bgp_rt_send_v4_message_attr(bgp->bgpg_local_as,
		  rt->rt_aspath, &api, 0);
		send_buf_initialized = 1;
	    } else if (SEND_V4_PKT_FULL(bclen)) {
		error |= bgp_rt_send_v4_group_flush(bgp, mybits,
		  nexthoptype, ifap, thisnexthop, 1);
		if (BIT_TEST(error, SEND_ALLFAILED)) {
		    send_buf_initialized = 0;
		    /*
		     * If all v4 peers are stuck, we're done.  Otherwise
		     * continue on, other routes may be sendable.
		     */
		    if (bgp->bgpg_n_v4_sync == 0) {
			goto dead_duck;
		    } else {
			BIT_RESET(error, SEND_ALLFAILED);
		    }
		    continue;
		}
	    }
	    SEND_PREFIX(rt->rt_dest, bclen);
	    sent++;

	    /*
	     * Prefix safely in packet, clear the bits we'll send this to.
	     */
	    info_prev = NULL;
	    infop = rtop->bgpgo_info;
	    do {
		BGPB_CLEAR(infop->bgp_info_bits, mybits, len_bits, res);
		if (res) {
		    bgpg_rtinfo_entry *info_tmp = infop->bgp_info_next;

		    if (info_prev == NULL) {
			if (info_tmp == NULL) {
			    BGP_GRTO_UNLINK(rtop);
			    BIT_RESET(entp->bgpe_flags, BGPEF_QUEUED);
			    BRT_TSI_PUT_ENT(rt->rt_head, rtbit, entp);
			    BRT_GRTO_FREE(rtop);
			} else {
			    rtop->bgpgo_info = info_tmp;
			}
		    } else {
			info_prev->bgp_info_next = info_tmp;
		    }
	    	    BRT_INFO_FREE(bgp, infop);
		    infop = info_tmp;
		} else {
		    info_prev = infop;
		    infop = infop->bgp_info_next;
		}
	    } while (infop != NULL);

	    /*
	     * Now scan the remainder of the list looking for compatable
	     * guys to send along with this.  Stick anyone we find into
	     * the packet.
	     */
	    for (rtop = rtop_next; rtop != (bgpg_rto_entry *) aspl;
	      rtop = rtop_check_next) {
		rtop_check_next = rtop->bgpgo_next;

		/*
		 * Check to see if we're into future changes.
		 * If so, break out of this.
		 */
		if (bgp_time_sec < rtop->bgpgo_time) {
		    break;
	        }
	    
		/*
		 * If the route is being flashed, leave it for the flash routine
		 */
		entp = rtop->bgpgo_advrt;
		rt = entp->bgpe_rt;
		if (!isflash && BIT_TEST(rt->rt_head->rth_state, RTS_ONLIST)) {
		    if (rtop == rtop_next) {
			rtop_next = rtop_check_next;
		    }
		    sendnextflash++;
		    continue;
		}

		/*
		 * Check out the metrics.  If they aren't the same we'll send
		 * this later.
		 */
		if (entp->bgpe_metrics != his_metrics) {
		    continue;
		}

		/*
		 * See if this guy has a full set of the right bits.
		 */
		BGPB_INFOBITS(hisbits, rtop, len_bits);
		BGPB_MBSET(hisbits, mybits, len_bits, res);
		if (!res) {
		    /*
		     * If this is the next pointer there are no group
		     * bits set we may skip it next time round
		     */
		    if (rtop == rtop_next) {
			BGPB_COMMON(hisbits, grbits, len_bits, res);
			if (!res) {
			    rtop_next = rtop_check_next;
			}
		    }
		    continue;
		}

		/*
		 * This is a keeper.  Put the route in the message.
		 */
		bclen = inet_prefix_mask(rt->rt_dest_mask);
		if (SEND_V4_PKT_FULL(bclen)) {
		    error |= bgp_rt_send_v4_group_flush(bgp, mybits,
		      nexthoptype, ifap, thisnexthop, TRUE);
		    if (BIT_TEST(error, SEND_ALLFAILED)) {
			/*
			 * If all v4 peers are stuck, we're done.  Otherwise
			 * break out of this, other routes may be sendable.
			 */
			send_buf_initialized = 0;
			if (bgp->bgpg_n_v4_sync == 0) {
			    goto dead_duck;
			} else {
			    BIT_RESET(error, SEND_ALLFAILED);
			}
			if (rtop == rtop_next) {
			    rtop_next = rtop_check_next;
			}
			break;
		    }
		}
		SEND_PREFIX(rt->rt_dest, bclen);
		sent++;

		/*
		 * Route safely in the packet.  Clear the bits out from
		 * its mask.  Free any structures which are no longer
		 * needed.
		 */
		info_prev = NULL;
		infop = rtop->bgpgo_info;
		do {
		    BGPB_CLEAR(infop->bgp_info_bits, mybits, len_bits, res);
		    if (res) {
			bgpg_rtinfo_entry *info_tmp = infop->bgp_info_next;

			if (info_prev == NULL) {
			    if (info_tmp == NULL) {
				if (rtop == rtop_next) {
				    rtop_next = rtop_check_next;
				}
				BGP_GRTO_UNLINK(rtop);
				BIT_RESET(entp->bgpe_flags, BGPEF_QUEUED);
				BRT_TSI_PUT_ENT(rt->rt_head, rtbit, entp);
				BRT_GRTO_FREE(rtop);
			    } else {
				rtop->bgpgo_info = info_tmp;
			    }
			} else {
			    info_prev->bgp_info_next = info_tmp;
			}
	    		BRT_INFO_FREE(bgp, infop);
			infop = info_tmp;
		    } else {
			info_prev = infop;
			infop = infop->bgp_info_next;
		    }
		} while (infop != NULL);
		/*
		 * Done with that.  Go around for more.
		 */
	    }

	    /*
	     * Done with this set of attributes.  If there is something
	     * to write, do so now.
	     */
	    if (send_buf_initialized) {
		send_buf_initialized = 0;
		error |= bgp_rt_send_v4_group_flush(bgp, mybits,
		  nexthoptype, ifap, thisnexthop, 0);
		if (BIT_TEST(error, SEND_ALLFAILED)) {
		    if (bgp->bgpg_n_v4_sync == 0) {
			goto dead_duck;
		    } else {
			BIT_RESET(error, SEND_ALLFAILED);
		    }
		}
	    }
	}	/* Look for more routes with same AS path */

	/*
	 * Check to see if this AS path is empty.  If so, delete it.
	 */
	if (aspl) {
	    if (aspl == aspl_unreach) {
		if (BGP_ASPL_EMPTY(aspl)) {
		    BGP_ASPL_REMOVE(&(bgp->bgpg_asp_queue), aspl);
		    BRT_ASPL_FREE(aspl);
		}
		aspl = (bgp_asp_list *) 0;
	    } else {
		if (BGP_ASPL_EMPTY(aspl)) {
		    bgp_asp_list *aspl_next
			= BGP_ASPL_NEXT(&(bgp->bgpg_asp_queue), aspl);

		    BGP_ASPL_REMOVE(&(bgp->bgpg_asp_queue), aspl);
		    BRT_ASPL_FREE(aspl);
		    aspl = aspl_next;
		} else {
		    aspl = BGP_ASPL_NEXT(&(bgp->bgpg_asp_queue), aspl);
		}
		if (!aspl) {
		    aspl = aspl_unreach;
		}
	    }
	}
    }		/* Look at another AS path */

    /*
     * If the routing table is still open, close it.
     */
    if (rttableopen) {
	rt_close(bgp->bgpg_task, (gw_entry *)0, 0, NULL);
    }

    /*
     * If we get here we've successfully sent all the routes
     * we can from this.  Set the next time and return.
     */
    if (sendnextflash) {
	trace_tp(bgp->bgpg_task,
		 TR_NORMAL,
		 0,
		 ("bgp_rt_send_v4_group: group %s sent %d route%s %s %d awaiting flash %d peer%s dropped sync",
		  bgp->bgpg_name,
		  sent,
		  ((sent == 1) ? "" : "s"),
		  bgp_rt_timer_str(nexttime, bgp->bgpg_rto_time),
		  sendnextflash,
		  (were_in_sync - bgp->bgpg_n_v4_sync),
		  (((were_in_sync - bgp->bgpg_n_v4_sync) == 1) ? "" : "s")));
    } else {
	trace_tp(bgp->bgpg_task,
		 TR_NORMAL,
		 0,
		 ("bgp_rt_send_v4_group: group %s sent %d route%s %s %d peer%s dropped sync",
		  bgp->bgpg_name,
		  sent,
		  ((sent == 1) ? "" : "s"),
		  bgp_rt_timer_str(nexttime, bgp->bgpg_rto_time),
		  (were_in_sync - bgp->bgpg_n_v4_sync),
		  (((were_in_sync - bgp->bgpg_n_v4_sync) == 1) ? "" : "s")));
    }
    if (bgp->bgpg_rto_next_time == 0) {
	bgp->bgpg_rto_next_time = nexttime;
    }
    return (error);

dead_duck:
    /*
     * Here we're had a failure which took out all the v4 bits.
     * Clean up and return.
     */
    trace_tp(bgp->bgpg_task,
	     TR_NORMAL,
	     0,
	     ("bgp_rt_send_v4_group: group %s sent %d route%s all (%d) peers dropped sync",
	      bgp->bgpg_name,
	      sent,
	      ((sent == 1) ? "" : "s"),
	      were_in_sync));
    if (rttableopen) {
	rt_close(bgp->bgpg_task, (gw_entry *)0, 0, NULL);
    }
    return error;
}

/*
 * bgp_rt_send_v2or3_group - takes a send list for a peer group
 *	and flushes all routing information for version 3 peers
 *	which can be written to.  The routine attempts to build
 *	single packets which can be flooded to as many peers as
 *	possible.
 */
static int
bgp_rt_send_v2or3_group __PF2(bgp, bgpPeerGroup *,
			      isflash, int)
{
    register bgpg_rto_entry *rtop;
    register bgpg_rtinfo_entry *infop;
    register rt_entry *rt;
    int isinternal = BGP_GROUP_INTERNAL(bgp);
    bgpg_rto_entry *rtop_next;
    bgpg_rto_entry *rtop_check_next;
    bgp_asp_list *aspl;
    bgpg_rtinfo_entry *info_prev;
    bgp_bits *grbits;
    bgp_bits mybits[BGP_MAXBITS];
    bgp_bits hisbits[BGP_MAXBITS];
    bgp_adv_entry *entp;
    if_addr *ifap;
    time_t nexttime, send_to;
    int len_bits = bgp->bgpg_idx_size;
    as_path_info api;
    bgp_nexthop thisnexthop;
    int nexthoptype;
    bgp_metrics *his_metrics;
    u_int rtbit = bgp->bgpg_task->task_rtbit;
    int rttableopen = 0;
    int res, doingunreachables;
    flag_t aspath_flags;
    int error = SEND_OK;
    int sendnextflash = 0;
    int sent = 0;
    int were_in_sync;

    /*
     * See if there is anything to do.  If not, just return.  If
     * so, fetch the group bits.
     */
    if (bgp->bgpg_n_v3_sync == 0) {
	return SEND_OK;
    }
    grbits = BGPG_GETBITS(bgp->bgpg_v3_sync, bgp->bgpg_idx_size);

    /*
     * Log what we're doing here
     */
    were_in_sync = bgp->bgpg_n_v3_sync;
    trace_tp(bgp->bgpg_task,
	     TR_NORMAL,
	     0,
	     ("bgp_rt_send_v2or3_group: sending to %d in-sync peers in %s",
	      were_in_sync,
	      bgp->bgpg_name));


    /*
     * Figure out the times at which we're allowed to send stuff.
     */
    if (bgp_time_sec <= bgp->bgpg_rto_time) {
	send_to = (time_t) 0;
    } else {
	send_to = bgp_time_sec - bgp->bgpg_rto_time;
    }
    nexttime = (time_t) 0;

    /*
     * See if there is anything on the list.  If not, return.
     * If so we'll process these first.
     */
    aspl = BGP_ASPL_FIRST(&(bgp->bgpg_asp_queue));
    if (aspl == NULL) {
	bgp->bgpg_rto_next_time = (time_t) 0;	/* nothing to do */
	return SEND_OK;
    }
    if (aspl->bgpl_asp == NULL) {
	doingunreachables = 1;
    } else {
	doingunreachables = 0;
    }

    /*
     * Figure out how we need deal with next hops.  The choices are
     * (1) ignore next hops, always use the local address, (2) send
     * a third party next hop if the next hop is on the peer's shared
     * interface, and (3) send a third party next hop if the next hop
     * interface is in the interface list.
     *
     * PEERIF groups are dealt with slightly inefficiently if there are
     * members in the group on different interfaces.  This is probably
     * not a big deal.
     */
    switch (bgp->bgpg_type) {
    default:
	assert(FALSE);
    case BGPG_TEST:
    case BGPG_INTERNAL:
	nexthoptype = SEND_NEXTHOP_PEERIF;
	break;
    case BGPG_INTERNAL_IGP:
	nexthoptype = SEND_NEXTHOP_NONE;
	break;
    case BGPG_INTERNAL_RT:
	nexthoptype = SEND_NEXTHOP_LIST;
	break;
    }
    ifap = (if_addr *) 0;

    /*
     * Put the router ID in the path info unless we're configured not
     * to.
     */
    if (bgp->bgpg_type == BGPG_TEST
      || BIT_TEST(bgp->bgpg_options, BGPGO_NOAGGRID)) {
	aspath_flags = APIF_NEXTHOP;
    } else {
	aspath_flags = APIF_LOCALID|APIF_NEXTHOP;
	api.api_localid = bgp->bgpg_out_id;
    }
    if (isinternal) {
	BIT_SET(aspath_flags, APIF_INTERNAL);
    }


    /*
     * Now go around looking at each AS path list for things to send
     */
    do {
	for (rtop = aspl->bgpl_grto_next;
	  rtop != (bgpg_rto_entry *) aspl;
	  rtop = rtop_next) {
	    rtop_next = rtop->bgpgo_next;

	    /*
	     * Check to see if we're into future changes.
	     * If so, break out of this.
	     */
	    if (send_to < rtop->bgpgo_time) {
		if (nexttime == 0 || nexttime > rtop->bgpgo_time) {
		    nexttime = rtop->bgpgo_time;
		}
		break;
	    }
	    
	    /*
	     * If the route is being flashed, leave it for the flash routine
	     */
	    if (!isflash
	      && BIT_TEST(rtop->bgpgo_rt->rt_head->rth_state, RTS_ONLIST)) {
		sendnextflash++;
		continue;
	    }

	    /*
	     * Find the set of bits for the guys we might like to
	     * send to.  We do this or'ing the bits in each info
	     * structure into a set, and then and'ing this with the
	     * group bits.
	     */
	    BGPB_INFOBITS(mybits, rtop, len_bits);
	    BGPB_AND_CHECK(mybits, grbits, len_bits, res);
	    if (!res) {
		continue;		/* nothing to do */
	    }

	    /*
	     * Here we've got a route to send.  Initialize the packet
	     * with the route's attributes.
	     */
	    entp = rtop->bgpgo_advrt;
	    rt = entp->bgpe_rt;
	    his_metrics = entp->bgpe_metrics;

	    if (doingunreachables) {
		bgp_rt_send_v3_unreachable_init(bgp->bgpg_local_as,
		  (sockaddr_un *) 0, isinternal);
		thisnexthop = 0;
	    } else {
		api.api_flags = aspath_flags;
		if (isinternal) {
		    if (his_metrics->bgpm_tlocalpref) {
			api.api_metric
			  = BGP_METRIC_4TO3(his_metrics->bgpm_localpref);
			BIT_SET(api.api_flags, APIF_METRIC);
		    } else if (bgp->bgpg_type == BGPG_INTERNAL_RT) {
			api.api_metric = BGP_DEF_V3METRIC;
			BIT_SET(api.api_flags, APIF_METRIC);
		    }
		} else {
		    if (his_metrics->bgpm_tmetric) {
			api.api_metric = his_metrics->bgpm_metric;
			BIT_SET(api.api_flags, APIF_METRIC);
		    }
		}
		api.api_nexthop = (sockaddr_un *) 0;  /* doesn't matter much */

		thisnexthop = his_metrics->bgpm_nexthop;
		if (nexthoptype == SEND_NEXTHOP_PEERIF) {
		    /*
		     * If the next hop is non-zero, determine the interface
		     * this next hop is on.
		     */
		    ifap = (if_addr *) 0;
		    if (thisnexthop) {
			ifap = RT_IFAP(rt);
			assert(ifap);
		    }
		}

		/*
		 * We know everything we need to now.  Set up the packet.
		 */
		bgp_rt_send_v3_message_attr(bgp->bgpg_local_as,
					    rt->rt_aspath,
					    &api);
	    }
	    SEND_ADDR(rt->rt_dest);
	    sent++;

	    /*
	     * Prefix safely in packet, clear the bits we'll send this to.
	     */
	    info_prev = NULL;
	    infop = rtop->bgpgo_info;
	    do {
		BGPB_CLEAR(infop->bgp_info_bits, mybits, len_bits, res);
		if (res) {
		    bgpg_rtinfo_entry *info_tmp = infop->bgp_info_next;

		    if (info_prev == NULL) {
			if (info_tmp == NULL) {
			    if (doingunreachables) {
				if (!rttableopen) {
				    rttableopen = 1;
				    rt_open(bgp->bgpg_task);
				}
				BRT_TSI_CLEAR(rt->rt_head, rtbit);
				(void) rtbit_reset(rt, rtbit);
				BGP_ADV_DEQUEUE(entp);
				BRT_ENT_FREE(entp);
			    } else {
				BIT_RESET(entp->bgpe_flags, BGPEF_QUEUED);
				BRT_TSI_PUT_ENT(rt->rt_head, rtbit, entp);
			    }
			    BGP_GRTO_UNLINK(rtop);
			    BRT_GRTO_FREE(rtop);
			} else {
			    rtop->bgpgo_info = info_tmp;
			}
		    } else {
			info_prev->bgp_info_next = info_tmp;
		    }
	    	    BRT_INFO_FREE(bgp, infop);
		    infop = info_tmp;
		} else {
		    info_prev = infop;
		    infop = infop->bgp_info_next;
		}
	    } while (infop != NULL);

	    /*
	     * Now scan the remainder of the list looking for compatable
	     * guys to send along with this.  Stick anyone we find into
	     * the packet.
	     */
	    for (rtop = rtop_next;
	      rtop != (bgpg_rto_entry *) aspl;
	      rtop = rtop_check_next) {
		rtop_check_next = rtop->bgpgo_next;

		/*
		 * Check to see if we're into future changes.
		 * If so, break out of this.
		 */
		if (bgp_time_sec < rtop->bgpgo_time) {
		    break;
	        }
	    
		/*
		 * If the route is being flashed, leave it for the flash routine
		 */
		entp = rtop->bgpgo_advrt;
		rt = entp->bgpe_rt;
		if (!isflash && BIT_TEST(rt->rt_head->rth_state, RTS_ONLIST)) {
		    if (rtop == rtop_next) {
			rtop_next = rtop_check_next;
		    }
		    sendnextflash++;
		    continue;
		}

		/*
		 * If this is a reachable, check out the metrics and next hop.
		 */
		if (!doingunreachables && entp->bgpe_metrics != his_metrics) {
		    register bgp_metrics *m = entp->bgpe_metrics;

		    if (!isinternal) {
			continue;
		    }
		    if (m->bgpm_nexthop != thisnexthop
		      || m->bgpm_tlocalpref != his_metrics->bgpm_tlocalpref
		      || m->bgpm_localpref != his_metrics->bgpm_localpref) {
			continue;
		    }
		}

		/*
		 * See if this guy has a full set of the right bits.
		 */
		BGPB_INFOBITS(hisbits, rtop, len_bits);
		BGPB_MBSET(hisbits, mybits, len_bits, res);
		if (!res) {
		    /*
		     * If this is the next pointer there are no group
		     * bits set we may skip it next time round
		     */
		    if (rtop == rtop_next) {
			BGPB_COMMON(hisbits, grbits, len_bits, res);
			if (!res) {
			    rtop_next = rtop_check_next;
			}
		    }
		    continue;
		}

		/*
		 * This is a keeper.  Put the route in the message.
		 */
		if (SEND_V3_PKT_FULL()) {
		    error |= bgp_rt_send_v3_group_flush(bgp, mybits,
		      nexthoptype, ifap, thisnexthop);
		    if (BIT_TEST(error, SEND_ALLFAILED)) {
			/*
			 * If all v3 peers are stuck, we're done.  Otherwise
			 * break out of this, other routes may be sendable.
			 */
			if (bgp->bgpg_n_v3_sync == 0) {
			    goto dead_duck;
			} else {
			    BIT_RESET(error, SEND_ALLFAILED);
			}
			break;
		    }
		}
		SEND_ADDR(rt->rt_dest);
		sent++;

		/*
		 * Route safely in the packet.  Clear the bits out from
		 * its mask.  Free any structures which are no longer
		 * needed.
		 */
		info_prev = NULL;
		infop = rtop->bgpgo_info;
		do {
		    BGPB_CLEAR(infop->bgp_info_bits, mybits, len_bits, res);
		    if (res) {
			bgpg_rtinfo_entry *info_tmp = infop->bgp_info_next;

			if (info_prev == NULL) {
			    if (info_tmp == NULL) {
				if (rtop == rtop_next) {
				    rtop_next = rtop_check_next;
				}
				if (doingunreachables) {
				    if (!rttableopen) {
					rttableopen = 1;
					rt_open(bgp->bgpg_task);
				    }
				    BRT_TSI_CLEAR(rt->rt_head, rtbit);
				    (void) rtbit_reset(rt, rtbit);
				    BGP_ADV_DEQUEUE(entp);
				    BRT_ENT_FREE(entp);
				} else {
				    BIT_RESET(entp->bgpe_flags, BGPEF_QUEUED);
				    BRT_TSI_PUT_ENT(rt->rt_head, rtbit, entp);
				}
				BGP_GRTO_UNLINK(rtop);
				BRT_GRTO_FREE(rtop);
			    } else {
				rtop->bgpgo_info = info_tmp;
			    }
			} else {
			    info_prev->bgp_info_next = info_tmp;
			}
	    		BRT_INFO_FREE(bgp, infop);
			infop = info_tmp;
		    } else {
			info_prev = infop;
			infop = infop->bgp_info_next;
		    }
		} while (infop != NULL);
		/*
		 * Done with that.  Go around for more.
		 */
	    }

	    /*
	     * Done with this set of attributes/peers.  Flush it out,
	     * there is at least one guy to write it to.
	     */
	    error |= bgp_rt_send_v3_group_flush(bgp, mybits,
	      nexthoptype, ifap, thisnexthop);
	    if (BIT_TEST(error, SEND_ALLFAILED)) {
		if (bgp->bgpg_n_v3_sync == 0) {
		    goto dead_duck;
		} else {
		    BIT_RESET(error, SEND_ALLFAILED);
		}
	    }
	}	/* End of routes with same path */

	/*
	 * See if we sent all routes listed with this AS path.
	 * If so, drop the AS path structure.
	 */
	if (BGP_ASPL_EMPTY(aspl)) {
	    bgp_asp_list *aspl_next = BGP_ASPL_NEXT(&(bgp->bgpg_asp_queue), aspl);

	    BGP_ASPL_REMOVE(&(bgp->bgpg_asp_queue), aspl);
	    BRT_ASPL_FREE(aspl);
	    aspl = aspl_next;
	} else {
	    aspl = BGP_ASPL_NEXT(&(bgp->bgpg_asp_queue), aspl);
	}

	/*
	 * If we were doing unreachables, we have done them
	 */
	if (doingunreachables) {
	    if (rttableopen) {
		rt_close(bgp->bgpg_task, (gw_entry *)0, 0, NULL);
		rttableopen = 0;
	    }
	    doingunreachables = 0;
	}
    } while (aspl != NULL);

    /*
     * If we get here we've successfully sent all the routes
     * we can from this.  Set the next time and return.
     */
    if (sendnextflash) {
	trace_tp(bgp->bgpg_task,
		 TR_NORMAL,
		 0,
		 ("bgp_rt_send_v2or3_group: group %s sent %d route%s %s %d awaiting flash %d peer%s dropped sync",
		  bgp->bgpg_name,
		  sent,
		  ((sent == 1) ? "" : "s"),
		  bgp_rt_timer_str(nexttime, bgp->bgpg_rto_time),
		  sendnextflash,
		  (were_in_sync - bgp->bgpg_n_v3_sync),
		  (((were_in_sync - bgp->bgpg_n_v3_sync) == 1) ? "" : "s")));
    } else {
	trace_tp(bgp->bgpg_task,
		 TR_NORMAL,
		 0,
		 ("bgp_rt_send_v2or3_group: group %s sent %d route%s %s %d peer%s dropped sync",
		  bgp->bgpg_name,
		  sent,
		  ((sent == 1) ? "" : "s"),
		  bgp_rt_timer_str(nexttime, bgp->bgpg_rto_time),
		  (were_in_sync - bgp->bgpg_n_v3_sync),
		  (((were_in_sync - bgp->bgpg_n_v3_sync) == 1) ? "" : "s")));
    }

    if (bgp->bgpg_rto_next_time == 0) {
	bgp->bgpg_rto_next_time = nexttime;
    }
    return error;

dead_duck:
    /*
     * Here we're had a failure which took out all the v3 bits.
     * Clean up and return.
     */
    trace_tp(bgp->bgpg_task,
	     TR_NORMAL,
	     0,
	     ("bgp_rt_send_v2or3_group: group %s sent %d route%s all (%d) peers dropped sync",
	      bgp->bgpg_name,
	      sent,
	      ((sent == 1) ? "" : "s"),
	      were_in_sync));
    if (rttableopen) {
	rt_close(bgp->bgpg_task, (gw_entry *)0, 0, NULL);
    }
    return error;
}


/*
 * bgp_get_asp_list - given a queue and an AS path pointer, return
 *		      an (existing if possible, new if not) AS path
 *		      list head.
 */
static bgp_asp_list *
bgp_get_asp_list __PF2(his_qp, bgp_rt_queue *,
		       his_asp, as_path *)
{
    register bgp_rt_queue *qp = his_qp;
    register bgp_asp_list *aspl;
    register as_path *asp = his_asp;
    register int hash;

    if (asp == NULL) {
	if (!(aspl = BGP_ASPL_FIRST(qp)) || aspl->bgpl_asp) {
	    aspl = BRT_ASPL_ALLOC();
	    BGP_ASPL_INIT(aspl);
	    aspl->bgpl_asp_hash_check = (-1);
	    BGP_ASPL_ADD_AFTER(qp, aspl);
	}
	return aspl;
    }

    hash = asp->path_hash;
    aspl = (qp->bgpq_asp_hash)[hash];
    if (aspl)  {
	register bgp_asp_list *aspl_new;
	do {
	    if (aspl->bgpl_asp == asp) {
		return aspl;
	    }
	    aspl = BGP_ASPL_NEXT(qp, aspl);
	} while (aspl && aspl->bgpl_asp->path_hash == hash);

	aspl_new = BRT_ASPL_ALLOC();
	BGP_ASPL_INIT(aspl_new);
	aspl_new->bgpl_asp_hash_check = hash;
	aspl_new->bgpl_asp = asp;
	if (aspl) {
	    BGP_ASPL_ADD_BEFORE(&(aspl->bgpl_asp_queue), aspl_new);
	    return aspl_new;
	}
	aspl = aspl_new;
    } else {
	aspl = BRT_ASPL_ALLOC();
	BGP_ASPL_INIT(aspl);
	aspl->bgpl_asp_hash_check = hash;
	(qp->bgpq_asp_hash)[hash] = aspl;
	aspl->bgpl_asp = asp;
    }

    BGP_ASPL_ADD_BEFORE(qp, aspl);
    return aspl;
}


/*
 * bgp_rt_policy_add_group_peer - an additional peer in a group has
 *	come up, after one with the same or better version came up
 *	previously.  Scan the list of routes the other guys advertised
 *	and arrange that these routes be advertised to this guy too.
 */
static int
bgp_rt_policy_add_group_peer __PF1(bnp, bgpPeer *)
{
    bgpPeerGroup *bgp = bnp->bgp_group;
    u_int rtbit = bgp->bgpg_task->task_rtbit;
    u_int word, wbit;
    bgp_adv_entry *entp, *gentp;
    bgpg_rto_entry *bgrto;
    bgp_asp_list *aspl;
    bgpg_rtinfo_entry *infop;
    bgp_metrics mymetrics;
    rt_entry *rt;
    rt_head *rth;
    rt_changes *rtc;
    if_addr *ifap;
    time_t rto_time;
    int ready = 0;
    int total = 0;
    int isversion4 = (bnp->bgp_version == BGP_VERSION_4);

    /*
     * Log that we're here
     */
    trace_tp(bnp->bgp_task,
	     TR_NORMAL,
	     0,
	     ("bgp_rt_policy_add_group_peer: group %s adding neighbour %s",
	      bgp->bgpg_name,
	      bnp->bgp_name));

    /*
     * Split this peer's bit for faster action
     */
    word = BGPB_WORD(bnp->bgp_group_bit);
    wbit = BGPB_WBIT(bnp->bgp_group_bit);

    /*
     * Compute the time for which this peer's routes are ready
     */
    rto_time = (bgp_time_sec >= bgp->bgpg_rto_time)
	? (bgp_time_sec - bgp->bgpg_rto_time) : (time_t) 0;

    /*
     * If this is an INTERNAL or TEST peer, and if the first entry
     * on his ifap list is associated with his interface and the reference
     * count is one, this is a new interface for us.  If so, note the
     * new interface and provide correct third party next hops for
     * those routes on this interface.
     */
    if ((ifap = bnp->bgp_ifap)) {
	if (!(bgp->bgpg_type == BGPG_INTERNAL || bgp->bgpg_type == BGPG_TEST)
	  || bgp->bgpg_ifap_list->bgp_if_ifap != ifap
	  || bgp->bgpg_ifap_list->bgp_if_refcount != 1) {
	    ifap = (if_addr *) 0;
	}
    }

    /*
     * Go around looking at his adv entries
     */
#define	BRTVTOENT(v)	((bgp_adv_entry *)(v))
    for (entp = BRTVTOENT(bgp->bgpg_queue.bgpv_next);
    	 entp != BRTVTOENT(&(bgp->bgpg_queue));
	 entp = BRTVTOENT(entp->bgpe_next)) {

	/*
	 * If it isn't a version 4 peer, but is a version 4 route, forget it
	 */
	if (!isversion4 && BIT_TEST(entp->bgpe_flags, BGPEF_V4_ONLY)) {
	    continue;
	}

	/*
	 * If this is a delete, forget it too.  We're already deleted.
	 */
	if (!(entp->bgpe_metrics)) {
	    continue;
	}

	rt = entp->bgpe_rt;
	rth = rt->rt_head;
	rtc = rth->rth_changes;

	/*
	 * Check to see if we should change the next hop
	 * on this route to third party.  If so, do it.
	 */
	if (ifap && !(entp->bgpe_metrics->bgpm_nexthop)) {
	    mymetrics.bgpm_nexthop = BGPM_NO_NEXTHOP;

	    if (rtc && BIT_TEST(rtc->rtc_flags, RTCF_NEXTHOP)) {
		if (rtc->rtc_n_gw && RTC_IFAP(rtc) == ifap) {
		    mymetrics = *(entp->bgpe_metrics);
		    mymetrics.bgpm_nexthop = sock2ip(RTC_ROUTER(rtc));
		}
	    } else {
		if (rt->rt_n_gw && RT_IFAP(rt) == ifap) {
		    mymetrics = *(entp->bgpe_metrics);
		    mymetrics.bgpm_nexthop = sock2ip(RT_ROUTER(rt));
		}
	    }

	    if (mymetrics.bgpm_nexthop) {
		BGPM_FREE(entp->bgpe_metrics);
		BGPM_FIND(&mymetrics, entp->bgpe_metrics);
	    }
	}

	if (BIT_TEST(entp->bgpe_flags, BGPEF_QUEUED)) {
	    /*
	     * This one has something queued already.  Fetch it
	     * so we can hook our state onto it.
	     */
	    BRT_TSI_GET_GROUP(rt->rt_head, rtbit, bgrto, gentp);
	    assert(bgrto && gentp == entp);

	    /*
	     * If this route is ready to go, remember this
	     */
	    if (bgrto->bgpgo_time <= rto_time) {
		ready++;
	    }
	    total++;

	    /*
	     * Search the list for an entry with no previous route.
	     */
	    for (infop = bgrto->bgpgo_info;
		 infop;
		 infop = infop->bgp_info_next) {
		if (!(infop->bgp_info_asp)) {
		    break;
		}
	    }

	    if (infop) {
		/*
		 * Got one, add our bit.
		 */
		BGPB_WB_SET(infop->bgp_info_bits, word, wbit);
	    } else {
		/*
		 * Nothing useful there, allocate a new one, set
		 * our bit and add it on
		 */
		infop = BRT_INFO_ALLOC(bgp);
		BGPB_WB_SET(infop->bgp_info_bits, word, wbit);
		infop->bgp_info_next = bgrto->bgpgo_info;
		bgrto->bgpgo_info = infop;
	    }
	} else {
	    /*
	     * Here there is nothing queued.  Allocate an rto entry
	     * for this.  First find an as path list head to hook it
	     * on to.
	     */
	    assert(rt == rth->rth_last_active);
	    if ((rtc = rth->rth_changes) != (rt_changes *) 0
	      && BIT_TEST(rtc->rtc_flags, RTCF_ASPATH)) {
		aspl = bgp_get_asp_list(&(bgp->bgpg_asp_queue), rtc->rtc_aspath);
	    } else {
		aspl = bgp_get_asp_list(&(bgp->bgpg_asp_queue), rt->rt_aspath);
	    }

	    /*
	     * Got the list, now fetch an rto entry, initialize it
	     * and hook it on at the start of the list so it goes
	     * out right away.
	     */
	    bgrto = BRT_GRTO_ALLOC();
	    bgrto->bgpgo_advrt = entp;
	    BIT_SET(entp->bgpe_flags, BGPEF_QUEUED);
	    if (BGP_ASPL_EMPTY(aspl)
		|| aspl->bgpl_grto_prev->bgpgo_time == (time_t) 0) {
		BGP_GRTO_ADD_END(aspl, bgrto);
	    } else {
		BGP_GRTO_ADD_HEAD(aspl, bgrto);
	    }

	    /*
	     * Finally, fetch an info entry and add it to the rto entry.
	     */
	    infop = BRT_INFO_ALLOC(bgp);
	    BGPB_WB_SET(infop->bgp_info_bits, word, wbit);
	    bgrto->bgpgo_info = infop;
	    ready++;
	    total++;

	    /*
	     * Now write it into the TSI field and we're done.
	     */
	    BRT_TSI_PUT_GRTO(rth, rtbit, bgrto);
	}
    }
#undef BRTVTOENT

    /*
     * Tell the world what we've done
     */
    if (ready == total) {
	trace_tp(bnp->bgp_task,
		 TR_NORMAL,
		 0,
		 ("bgp_rt_policy_add_group_peer: group %s neighbour %s %d route%s ready to go",
		  bgp->bgpg_name,
		  bnp->bgp_name,
		  ready,
		  ((ready == 1) ? "" : "s")));
    } else {
	trace_tp(bnp->bgp_task,
		 TR_NORMAL,
		 0,
		 ("bgp_rt_policy_add_group_peer: group %s neighbour %s %d routes to be advertised %d ready to go",
		  bgp->bgpg_name,
		  bnp->bgp_name,
		  total,
		  ready));
    }
    return ready;
}


/*
 * bgp_rt_policy_init_group_peer - add an initial set of routes to a peer
 *   in a TEST, INTERNAL, INTERNAL_IGP or INTERNAL_ROUTING group to the
 *   send list.  This deals with three cases, when this is the first
 *   peer to come up in the group, when it is the first version 4
 *   peer but a version 3 peer is already up, and when another peer
 *   of the same or better version is up already.
 */
static int
bgp_rt_policy_init_group_peer __PF3(bnp, bgpPeer *,
				  rtl, rt_list *,
				  what, int)
{
    register rt_head *rth;
    register rt_entry *rt;
    register bgp_adv_entry *nentp = (bgp_adv_entry *) 0;
    register bgpg_rto_entry *bgrto;
    register bgpg_rtinfo_entry *infop;
    bgp_adv_entry *entp;
    bgp_metrics *nmetrics;
    bgp_sync *bsp = (bgp_sync *) 0;
    rt_changes *rtc;
    if_addr *ifap = (if_addr *) 0;
    time_t rto_time;
    static bgp_metrics mymetrics = { 0 };
    u_int rtbit;
    int ready = 0;
    int total = 0;
    int getnexthop = 0;
    int isversion4 = (bnp->bgp_version == BGP_VERSION_4);
    bgpPeerGroup *bgp = bnp->bgp_group;

    trace_tp(bnp->bgp_task,
	     TR_NORMAL,
	     0,
	     ("bgp_rt_policy_init_group_peer: %s update %s neighbour %s",
	      trace_state(bgp_rt_init_group_bits, what),
	      bgp->bgpg_name,
	      bnp->bgp_name));

    if (bgp->bgpg_type == BGPG_INTERNAL_IGP && bgp->bgpg_igp_rtbit == 0) {
	/* Nothing to do */
	return 0;
    }

    /*
     * If this is the first guy in a synchronized group, fetch the pointer
     * to the synchronization structure.  Otherwise see if we'll need to
     * adjust next hop's to account for a new interface being added to
     * the list.
     */
    if (what == BRTINI_FIRST) {
	if (bgp->bgpg_type == BGPG_INTERNAL_RT) {
	    bsp = bgp->bgpg_sync;
	    assert(bsp);
	}
    } else {
	if ((ifap = bnp->bgp_ifap)) {
	    if ((bgp->bgpg_type != BGPG_INTERNAL && bgp->bgpg_type != BGPG_TEST)
	      || bgp->bgpg_ifap_list->bgp_if_ifap != ifap
	      || bgp->bgpg_ifap_list->bgp_if_refcount != 1) {
		ifap = (if_addr *) 0;
	    }
	}
    }

    /*
     * Compute the time for which this peer's routes are ready
     */
    rto_time = (bgp_time_sec >= bgp->bgpg_rto_time)
	? (bgp_time_sec - bgp->bgpg_rto_time) : (time_t) 0;


    rt_open(bnp->bgp_task);
    rtbit = bgp->bgpg_task->task_rtbit;

    RT_LIST(rth, rtl, rt_head) {

	/*
	 * If this isn't an inet route, continue
	 */
	if (socktype(rth->rth_dest) != AF_INET) {
	    continue;
	}

	/*
	 * If we're synchronizing and either the active route is
	 * an IGP route of the right type, or an interface route,
	 * inform the synchronization code.
	 */
	rt = rth->rth_active;
	rtc = rth->rth_changes;
	if (bsp && rt && !BIT_TEST(rth->rth_state, RTS_ONLIST)
	  && (!BIT_TEST(rt->rt_state, RTS_GATEWAY)
	    || rt->rt_gwp->gw_proto == bsp->bgp_sync_proto)) {
	    bgp_sync_igp_rt(bsp, rth);
	}

	/*
	 * If this isn't the first peer, see if someone's announced
	 * the route already.  Otherwise we know it won't be announced
	 */
	if (what != BRTINI_FIRST && rth->rth_n_announce > 0) {
	    BRT_TSI_GET_GROUP(rth, rtbit, bgrto, entp);
	    if (entp) {
		/*
		 * If this is a delete we aren't interested
		 */
		if (!(entp->bgpe_metrics)) {
		    continue;
		}
		rt = entp->bgpe_rt;
		assert(rt == rth->rth_last_active);
	    }
	} else {
	    bgrto = (bgpg_rto_entry *) 0;
	    entp = (bgp_adv_entry *) 0;
	}

	/*
	 * If there is no route by now, we're not impressed.
	 */
	if (!rt) {
	    continue;
	}

	/*
	 * If this was announced by someone in the group, make sure
	 * we're allowed to announce it given our version.  Otherwise
	 * see what policy has to say about it.
	 */
	if (entp) {
	    assert(isversion4);
	    if (ifap && !(entp->bgpe_metrics->bgpm_nexthop)) {
		if (rtc && BIT_TEST(rtc->rtc_flags, RTCF_NEXTHOP)) {
		    if (rtc->rtc_n_gw && RTC_IFAP(rtc) == ifap) {
			mymetrics = *(entp->bgpe_metrics);
			mymetrics.bgpm_nexthop = sock2ip(RTC_ROUTER(rtc));
		    }
		} else {
		    if (rt->rt_n_gw && RT_IFAP(rt) == ifap) {
			mymetrics = *(entp->bgpe_metrics);
			mymetrics.bgpm_nexthop = sock2ip(RT_ROUTER(rt));
		    }
		}
		if (mymetrics.bgpm_nexthop) {
		    BGPM_FREE(entp->bgpe_metrics);
		    BGPM_FIND(&mymetrics, entp->bgpe_metrics);
		    mymetrics.bgpm_nexthop = BGPM_NO_NEXTHOP;
		    mymetrics.bgpm_metric = mymetrics.bgpm_localpref
		      = (bvalue_t) 0;
		    mymetrics.bgpm_tmetric = mymetrics.bgpm_tlocalpref
		      = (btype_t) 0;
		}
	    }
	} else {
	    byte class;
	    adv_results results;
	    int export_it;

	    /* use direct bzero of results */
            bzero(&results, sizeof(results));
	    
            /*
	     * If it is to be flashed, forget it, we'll pick it
	     * up in the flash routine.
	     */
	    if (BIT_TEST(rth->rth_state, RTS_ONLIST)) {
		continue;
	    }

	    /*
	     * If we aren't allowed to advertise this route, or
	     * if it has no AS path, no need to continue.
	     */
	    if (!rt->rt_aspath || BIT_TEST(rt->rt_state, RTS_NOADVISE|RTS_PENDING)) {
		continue;
	    }

	    /*
	     * Determine version 3 announceability
	     */
	    if (!nentp) {
		nentp = BRT_ENT_ALLOC();
	    } else {
		nentp->bgpe_flags = (flag_t) 0;
	    }
	    class = inet_class_flags(rth->rth_dest);
	    if (BIT_TEST(class, INET_CLASSF_LOOPBACK)) {
		continue;		/* can't do it */
	    } else if (BIT_TEST(class, INET_CLASSF_DEFAULT)) {
		if (rth->rth_dest_mask != inet_mask_default) {
		    if (!isversion4) {
			continue;
		    }
		    nentp->bgpe_flags = BGPEF_V4_ONLY;
		} else if (what == BRTINI_FIRST_V4) {
		    /*
		     * The version 3 peer would have sent this
		     */
		    continue;
		}
	    } else {
		if (!BIT_TEST(class, INET_CLASSF_NETWORK)) {
		    continue;	/* No one can announce this */
		}
		if (rth->rth_dest_mask != inet_mask_natural(rth->rth_dest)) {
		    if (!isversion4) {
			continue;	/* Can't do it */
		    }
		    nentp->bgpe_flags = BGPEF_V4_ONLY;
		} else if (what == BRTINI_FIRST_V4) {
		    /*
		     * The version 3 peer would have announced this.
		     */
		    continue;
		}
	    }

	    /*
	     * So far so good.  Now we check hardcore policy
	     */
	    switch (bgp->bgpg_type) {
	    default:
		assert(FALSE);

	    case BGPG_TEST:
		/* Anything goes! */
		export_it = TRUE;
		if (BRT_BGP_OR_EGP(rt)) {
		    if (rt->rt_metric != BGP_METRIC_NONE) {
			mymetrics.bgpm_tmetric = BGPM_HAS_VALUE;
			mymetrics.bgpm_metric = rt->rt_metric;
		    }
		}
		getnexthop = 1;
		break;

	    case BGPG_INTERNAL_RT:
	    case BGPG_INTERNAL:
		results.res_metric = bgp->bgpg_metric_out;
		export_it = export(rt,
				   (proto_t) 0,
				   bgp->bgpg_peers->bgp_export,
				   (adv_entry *) 0,
				   (adv_entry *) 0,
				    &results);
		if (export_it) {
		    if (results.res_metric != BGP_METRIC_NONE) {
			mymetrics.bgpm_tmetric = BGPM_HAS_VALUE;
			mymetrics.bgpm_metric = results.res_metric;
		    }
		    if (BIT_TEST(bgp->bgpg_options, BGPGO_SETPREF)) {
			mymetrics.bgpm_tlocalpref = BGPM_HAS_VALUE;
			mymetrics.bgpm_localpref
			  = BGP_PREF_TO_LOCALPREF(rt->rt_preference,
						  bgp->bgpg_setpref);
		    }
		    getnexthop = 1;
		}
		break;

	    case BGPG_INTERNAL_IGP:
		export_it = rtbit_isset(rt, bgp->bgpg_igp_rtbit);
		break;
	    }

	    /*
	     * If we aren't allowed to export it, forget it.
	     */
	    if (!export_it) {
		continue;
	    }

	    /*
	     * Find the next hop we should send it with.
	     */
	    if (getnexthop) {
		getnexthop = 0;
		if (bgp->bgpg_ifap_list
		  && BIT_TEST(rt->rt_state, RTS_GATEWAY)
		  && rt->rt_n_gw > 0) {
		    register bgp_ifap_list *ifapl = bgp->bgpg_ifap_list;
		    register if_addr *nifap = RT_IFAP(rt);

		    do {
			if (ifapl->bgp_if_ifap == nifap) {
			    mymetrics.bgpm_nexthop = sock2ip(RT_ROUTER(rt));
			    break;
			}
		    } while ((ifapl = ifapl->bgp_if_next));
		}
	    }

	    /*
	     * Here we're got a keeper, set our bit on the route and
	     * find a metrics pointer for this.
	     */
	    rtbit_set(rt, rtbit);
	    entp = nentp;
	    nentp = (bgp_adv_entry *) 0;
	    entp->bgpe_rt = rt;
	    BGPM_FIND(&mymetrics, nmetrics);
	    entp->bgpe_metrics = nmetrics;
	    mymetrics.bgpm_types = mymetrics.bgpm_localpref
		= mymetrics.bgpm_metric = (bvalue_t) 0;
	    mymetrics.bgpm_nexthop = BGPM_NO_NEXTHOP;
	    BGP_ADV_ADD_BEFORE(&bgp->bgpg_queue, entp);
	}

	/*
	 * Here the routing bit is set on one of the routes.  There are
	 * two cases of interest, when the route is already on the send
	 * list (bgrto != NULL) and when it isn't (bgrto == NULL).  Handle
	 * these cases separately.
	 */
	total++;
	if (bgrto == NULL) {
	    as_path *asp;
	    bgp_asp_list *aspl;

	    /*
	     * The route is not on the list, but someone else has announced
	     * it.  Here we need to find the route and create a list entry
	     * so that our guy can "sync up" with the other(s).
	     *
	     * This is complexified when the route is on the flash list.
	     * Here we build a list entry based on the state of the route
	     * when it was last flashed, in the expectation that this will
	     * work itself out when the flash routine is executed.
	     */
	    if (rtc && BIT_TEST(rtc->rtc_flags, RTCF_ASPATH)) {
		asp = rtc->rtc_aspath;
	    } else {
		asp = rt->rt_aspath;
	    }

	    /*
	     * We have the AS path for the previous route, obtain/create
	     * the AS path list.
	     */
	    aspl = bgp_get_asp_list(&(bgp->bgpg_asp_queue), asp);

	    /*
	     * Now create a list entry for the route, scheduling it to go
	     * immediately.  Add it to the end of the list if possible (i.e.
	     * if the end of the list is not scheduled for some future time)
	     * to try to preserve a modicum of FIFOness, the start otherwise.
	     * Then shove the rto entry pointer into the tsi area.
	     */
	    bgrto = BRT_GRTO_ALLOC();
	    bgrto->bgpgo_advrt = entp;
	    BIT_SET(entp->bgpe_flags, BGPEF_QUEUED);

	    if (BGP_ASPL_EMPTY(aspl)
	      || aspl->bgpl_grto_prev->bgpgo_time == (time_t) 0) {
		BGP_GRTO_ADD_END(aspl, bgrto);
	    } else {
		BGP_GRTO_ADD_HEAD(aspl, bgrto);
	    }
	    BRT_TSI_PUT_GRTO(rth, rtbit, bgrto);

	    /*
	     * End result is a group rto entry added for this route,
	     * ready to go now.
	     */
	    ready++;
	} else {
	    /*
	     * Here we already have a group rto entry on the list.  Look
	     * on the info list for someone else who has not announced
	     * this route yet and, if found, add our bit there.
	     */
	    if (bgrto->bgpgo_time <= rto_time) {
		ready++;
	    }

	    for (infop = bgrto->bgpgo_info; infop != NULL;
	      infop = infop->bgp_info_next) {
		if (infop->bgp_info_asp == NULL) {
		    break;
		}
	    }
	    if (infop != NULL) {
		/* Set our bit here and continue */
		BGPB_BSET(infop->bgp_info_bits, bnp->bgp_group_bit);
		continue;
	    }
	}

	/*
	 * Here we've got an rto entry, but no associated info entry
	 * which reflects our current state.  Allocate and initialize
	 * an appropriate info entry and add it to the list.
	 */
	infop = BRT_INFO_ALLOC(bgp);
	BGPB_BSET(infop->bgp_info_bits, bnp->bgp_group_bit);
	infop->bgp_info_next = bgrto->bgpgo_info;
	bgrto->bgpgo_info = infop;

	/*
	 * Done.  Go around for next route.
	 */
    } RT_LIST_END(rth, rtl, rt_head);

    /*
     * All queued, close the route table
     */
    rt_close(bnp->bgp_task, &bnp->bgp_gw, 0, NULL);

    /*
     * If we allocated an adv entry unnecessarily, blow it away
     */
    if (nentp) {
	BRT_ENT_FREE(nentp);
    }

    /*
     * Report what we did
     */
    trace_tp(bnp->bgp_task,
	     TR_NORMAL,
	     0,
	     ("bgp_rt_policy_init_group_peer: %s %d route%s ready %d deferred",
	      bnp->bgp_name,
	      ready,
	      ((ready == 1) ? "" : "s"),
	      (total - ready)));

    /*
     * Return the number of routes ready
     */
    return ready;
}


/*
 * bgp_rt_policy_group - run group policy in a flash/newpolicy
 *			 update.  This queues stuff on the group,
 *			 then tries to flush it out.
 */
static int
bgp_rt_policy_group __PF3(bgp, bgpPeerGroup *,
			  rtl, rt_list *,
			  what, int)
{
    rt_head *rth;
    rt_entry *new_rt, *old_rt;
    rt_changes *rtc;
    bgp_bits allbits[BGP_MAXBITS];
    static bgp_metrics mymetrics = { 0 };
    bgp_adv_entry *entp;
    bgp_adv_entry *nentp = (bgp_adv_entry *) 0;
    bgpg_rto_entry *bgrto;
    bgp_metrics *nmetrics = (bgp_metrics *) 0;
    u_int bitlen = bgp->bgpg_idx_size;
    u_int rtbit = bgp->bgpg_task->task_rtbit;
    bgpg_rtinfo_entry *ninfop = (bgpg_rtinfo_entry *) 0;
    bgp_asp_list *aspl = (bgp_asp_list *) 0;
    bgp_sync *bsp = (bgp_sync *) 0;
    as_path *old_asp;
    rt_entry *reset_rt = (rt_entry *) 0;
    time_t rto_time;
    time_t nexttime = (time_t) 0;
    int ready = 0;
    int total = 0;
    int no_old_state = 0;
    int still_on_list = 0;
    int must_remove_from_list = 0;
    int checkit = 0;
    int new_list = 0;
    int version_3_only;

    trace_tp(bgp->bgpg_task,
	     TR_NORMAL,
	     0,
	     ("bgp_rt_policy_group: %s update %s",
	      trace_state(bgp_rt_update_bits, what),
	      bgp->bgpg_name));

    /*
     * If we're running with an IGP and don't have his rtbit, nothing to do
     */
    if (bgp->bgpg_type == BGPG_INTERNAL_IGP && bgp->bgpg_igp_rtbit == 0) {
	return 0;
    }

    /*
     * If this is a synchronized group, fetch the pointer to the
     * synchronization structure.
     */
    if (bgp->bgpg_type == BGPG_INTERNAL_RT) {
	bsp = bgp->bgpg_sync;
	assert(bsp);
    }

    /*
     * Compute the time for which this peer's routes are ready
     */
    rto_time = (bgp_time_sec >= bgp->bgpg_rto_time)
	? (bgp_time_sec - bgp->bgpg_rto_time) : (time_t) 0;

    /*
     * Determine if this is version 3 only.  If so some things
     * are easier.  Also determine our full bit set.
     */
    if (bgp->bgpg_n_v4_bits == 0) {
	version_3_only = 1;
	BGPB_COPY(BGPG_GETBITS(bgp->bgpg_v3_bits, bitlen), allbits, bitlen);
    } else {
	version_3_only = 0;
	BGPB_COPY(BGPG_GETBITS(bgp->bgpg_v4_bits, bitlen), allbits, bitlen);
	if (bgp->bgpg_n_v3_bits) {
	    BGPB_SET(allbits, BGPG_GETBITS(bgp->bgpg_v3_bits, bitlen), bitlen);
	}
    }

    /*
     * Open the routing table for bit diddling.
     */
    rt_open(bgp->bgpg_task);

    /*
     * Now go around looking for things do to.
     */
    RT_LIST(rth, rtl, rt_head) {

	/*
	 * If this isn't an inet route, continue
	 */
	if (socktype(rth->rth_dest) != AF_INET) {
	    continue;
	}

	/*
	 * If we're synchronizing look at the active and last_active
	 * routes to determine whether either might be of interest
	 * to the synchronization code.  If so, call it.
	 */
	new_rt = rth->rth_active;
	if (bsp) {
	    register rt_entry *tmp_rt;

	    if ((new_rt && (!BIT_TEST(new_rt->rt_state, RTS_GATEWAY)
		|| new_rt->rt_gwp->gw_proto == bsp->bgp_sync_proto))
	      || ((tmp_rt = rth->rth_last_active)
		&& (!BIT_TEST(tmp_rt->rt_state, RTS_GATEWAY)
		|| tmp_rt->rt_gwp->gw_proto == bsp->bgp_sync_proto))) {
		bgp_sync_igp_rt(bsp, rth);
	    }
	}

	/*
	 * If this isn't the first peer, see if we've announced the
	 * route already.  Otherwise we know it won't be announced
	 */
	old_rt = (rt_entry *) 0;
	if (rth->rth_n_announce > 0) {
	    BRT_TSI_GET_GROUP(rth, rtbit, bgrto, entp);
	    if (bgrto) {
		/*
		 * See if this is a delete.  If not this has our old
		 * route.
		 */
		if (entp->bgpe_metrics) {
		    old_rt = entp->bgpe_rt;
		}
		assert(!version_3_only
		    	|| !BIT_TEST(entp->bgpe_flags, BGPEF_V4_ONLY));
	    } else if (entp) {
		old_rt = entp->bgpe_rt;
		assert(old_rt == rth->rth_last_active);
		assert(!version_3_only
		    	|| !BIT_TEST(entp->bgpe_flags, BGPEF_V4_ONLY));
	    }
	} else {
	    bgrto = (bgpg_rto_entry *) 0;
	    entp = (bgp_adv_entry *) 0;
	}

	/*
	 * If there is no old route, or this is a new policy run,
	 * determine whether this is a version 4 only route or not.
	 * If we're running version 3 we won't be able to announce
	 * this.
	 */
	new_rt = rth->rth_active;
	if (new_rt && !old_rt) {
	    byte class = inet_class_flags(rth->rth_dest);

	    if (!entp) {
		if (!nentp) {
		    nentp = BRT_ENT_ALLOC();
		} else {
		    nentp->bgpe_flags = (flag_t) 0;
		}
	    }

	    if (BIT_TEST(class, INET_CLASSF_LOOPBACK)) {
		continue;		/* can't do it */
	    } else if (BIT_TEST(class, INET_CLASSF_DEFAULT)) {
		if (rth->rth_dest_mask != inet_mask_default) {
		    if (version_3_only) {
			continue;
		    }
		    if (!entp) {
			BIT_SET(nentp->bgpe_flags, BGPEF_V4_ONLY);
		    }
		}
	    } else {
		if (!BIT_TEST(class, INET_CLASSF_NETWORK)) {
		    continue;	/* No one can announce this */
		}
		if (rth->rth_dest_mask != inet_mask_natural(rth->rth_dest)) {
		    if (version_3_only) {
			continue;	/* Can't do it */
		    }
		    if (!entp) {
			BIT_SET(nentp->bgpe_flags, BGPEF_V4_ONLY);
		    }
		}
	    }
	}

	/*
	 * If there is an old route, determine the AS path we wanted
	 * to send.  We may need this later.
	 */
	rtc = rth->rth_changes;
	old_asp = (as_path *) 0;
	if (old_rt) {
	    if (rtc && BIT_TEST(rtc->rtc_flags, RTCF_ASPATH)) {
		old_asp = rtc->rtc_aspath;
	    } else {
		old_asp = old_rt->rt_aspath;
	    }
	}

	/*
	 * Now determine whether there is a new route to send.
	 */
	if (new_rt) {
	    adv_results results;
	    int export_it;

            bzero(&results, sizeof(results));

	    /*
	     * If we aren't allowed to advertise this route, or
	     * if it has no AS path, no need to continue.
	     */
	    if (!new_rt->rt_aspath
	      || BIT_TEST(new_rt->rt_state, RTS_NOADVISE|RTS_PENDING)) {
		new_rt = (rt_entry *) 0;
		goto dooldroute;
	    }

	    /*
	     * Okay.  If this route arrived from another peer in the
	     * same group, forget it.
	     * XXX this may change some day.
	     */
	    if (bgp->bgpg_type != BGPG_TEST &&
		BRT_ISBGP(new_rt) &&
		((bgpPeer *)(new_rt->rt_gwp->gw_task->task_data))->bgp_group ==
		bgp) {
		new_rt = (rt_entry *) 0;
		goto dooldroute;
	    }

	    /*
	     * So far so good.  Now we check hardcore policy
	     */
	    switch (bgp->bgpg_type) {
	    default:
		assert(FALSE);

	    case BGPG_TEST:
		/* Anything goes! */
		export_it = TRUE;
		if (BRT_BGP_OR_EGP(new_rt)) {
		    if (new_rt->rt_metric != BGP_METRIC_NONE) {
			mymetrics.bgpm_tmetric = BGPM_HAS_VALUE;
			mymetrics.bgpm_metric = new_rt->rt_metric;
		    }
		}
		if (bgp->bgpg_ifap_list) {
		    checkit = 1;
		}
		break;

	    case BGPG_INTERNAL_RT:
	    case BGPG_INTERNAL:
		results.res_metric = bgp->bgpg_metric_out;
		export_it = export(new_rt,
				   (proto_t) 0,
				   bgp->bgpg_peers->bgp_export,
				   (adv_entry *) 0,
				   (adv_entry *) 0,
				    &results);
		if (export_it) {
		    if (results.res_metric != BGP_METRIC_NONE) {
			mymetrics.bgpm_tmetric = BGPM_HAS_VALUE;
			mymetrics.bgpm_metric = results.res_metric;
		    }
		    if (BIT_TEST(bgp->bgpg_options, BGPGO_SETPREF)) {
			mymetrics.bgpm_tlocalpref = BGPM_HAS_VALUE;
			mymetrics.bgpm_localpref
			  = BGP_PREF_TO_LOCALPREF(new_rt->rt_preference,
						  bgp->bgpg_setpref);
		    }
		    if (bgp->bgpg_ifap_list) {
			checkit = 1;
		    }
		}
		break;

	    case BGPG_INTERNAL_IGP:
		/* XXX add policy to determine metrics/tags */
		export_it = rtbit_isset(new_rt, bgp->bgpg_igp_rtbit);
		break;
	    }

	    /*
	     * If we aren't allowed to export it, forget it.
	     */
	    if (!export_it) {
		new_rt = (rt_entry *) 0;
		goto dooldroute;
	    }

	    /*
	     * If we need to check for a third-party next hop, do
	     * it now.
	     */
	    if (checkit) {
		checkit = 0;
		if (BIT_TEST(new_rt->rt_state, RTS_GATEWAY)
		  && new_rt->rt_n_gw > 0) {
		    register bgp_ifap_list *ifapl = bgp->bgpg_ifap_list;
		    register if_addr *nifap = RT_IFAP(new_rt);

		    do {
			if (ifapl->bgp_if_ifap == nifap) {
			    mymetrics.bgpm_nexthop = sock2ip(RT_ROUTER(new_rt));
			    break;
			}
		    } while ((ifapl = ifapl->bgp_if_next));
		}
	    }

	    /*
	     * Find the metrics.  Check to see if we've had changes
	     * from the last route we wanted to announce.  If not,
	     * don't go further.
	     */
	    BGPM_FIND(&mymetrics, nmetrics);
	    mymetrics.bgpm_types = mymetrics.bgpm_localpref
		= mymetrics.bgpm_metric = (bvalue_t) 0;
	    mymetrics.bgpm_nexthop = BGPM_NO_NEXTHOP;

	    if (old_rt
	      && entp->bgpe_metrics == nmetrics
	      && new_rt->rt_aspath == old_asp) {
		if (old_rt != new_rt) {
		    rtbit_set(new_rt, rtbit);
		    rtbit_reset(old_rt, rtbit);
		    entp->bgpe_rt = new_rt;
		}
		BGPM_FREE(nmetrics);
		if (bgrto) {
		    total++;
		    if (bgrto->bgpgo_time <= rto_time) {
			ready++;
		    } else if (nexttime == (time_t) 0
		      || nexttime > bgrto->bgpgo_time) {
			nexttime = bgrto->bgpgo_time;
		    }
		}
		continue;
	    }

	    if (!BGPM_FIRST(nmetrics)) {
		checkit = 1;
	    }

	    /*
	     * Here we're got a keeper, acquire an adv entry pointer
	     * for this and find a metrics pointer.
	     */
	    if (!entp) {
		entp = nentp;
		nentp = (bgp_adv_entry *) 0;
		BGP_ADV_ADD_BEFORE(&(bgp->bgpg_queue), entp);
	    }
	}

dooldroute:
	if (!new_rt && !old_rt) {
	    if (entp) {
		/*
		 * Here we have a deletion queued, and are still
		 * interested in deleting it.  If there is a new
		 * active route move the bit there so we don't hold
		 * down a deleted route.  Then count this in with
		 * the ready routes.
		 */
		if (rth->rth_active
		  && rth->rth_active != entp->bgpe_rt) {
		    rtbit_set(rth->rth_active, rtbit);
		    rtbit_reset(entp->bgpe_rt, rtbit);
		    entp->bgpe_rt = rth->rth_active;
		}
		if (bgrto->bgpgo_time <= rto_time) {
		    ready++;
		} else if (nexttime == (time_t) 0
		  || nexttime > bgrto->bgpgo_time) {
		    nexttime = bgrto->bgpgo_time;
		}
		total++;
	    }
	    continue;
	}

	/*
	 * Fetch an info pointer to record the old state, and collect
	 * the bits we'll need to set in it.  If this is a version 4
	 * only route only set the version 4 bits, otherwise set all
	 * of them.  If there is an existing queue, strip off the
	 * bits already registered in other infop's.
	 */
	if (!ninfop) {
	    ninfop = BRT_INFO_ALLOC(bgp);
	}
	if (BIT_TEST(entp->bgpe_flags, BGPEF_V4_ONLY)) {
	    BGPB_COPY(BGPG_GETBITS(bgp->bgpg_v4_bits, bitlen),
		      ninfop->bgp_info_bits,
		      bitlen);
	} else {
	    BGPB_COPY(allbits, ninfop->bgp_info_bits, bitlen);
	}

	if (bgrto) {
	    register bgpg_rtinfo_entry *infop = bgrto->bgpgo_info;

	    while (infop) {
		BGPB_RESET(ninfop->bgp_info_bits,
			   infop->bgp_info_bits,
			   bitlen);
		infop = infop->bgp_info_next;
	    }
	    /*
	     * If all bits are already set elsewhere, set no_old_state.
	     */
	    BGPB_CHECK_CLEAR(ninfop->bgp_info_bits,
			     bitlen,
			     no_old_state);
	    /*
	     * Determine whether we'll need to remove this from
	     * the AS path list or not.  We won't if the AS path
	     * hasn't changed, and if this is either a new policy
	     * update or the time on the entry is non-zero.
	     */
	    still_on_list = 1;
	    must_remove_from_list = 1;
	    if (old_rt && new_rt
	      && (what == BRTUPD_NEWPOLICY || bgrto->bgpgo_time != (time_t) 0)
	      && new_rt->rt_aspath == old_asp) {
		must_remove_from_list = 0;
	    }
	} else {
	    bgrto = BRT_GRTO_ALLOC();
	    new_list = 1;
	}

	if (old_rt && !no_old_state) {
	    ninfop->bgp_info_metrics = entp->bgpe_metrics;
	    entp->bgpe_metrics = (bgp_metrics *) 0;
	    ninfop->bgp_info_asp = old_asp;
	    ASPATH_ALLOC(old_asp);
	    ninfop->bgp_info_next = bgrto->bgpgo_info;
	    bgrto->bgpgo_info = ninfop;
	    ninfop = (bgpg_rtinfo_entry *) 0;
	} else if (!no_old_state) {
	    /*
	     * No old route here, just queue an add.
	     */
	    ninfop->bgp_info_next = bgrto->bgpgo_info;
	    bgrto->bgpgo_info = ninfop;
	    ninfop = (bgpg_rtinfo_entry *) 0;
	} else {
	    no_old_state = 0;
	}

	/*
	 * If our adv entry still has metrics at this point, free
	 * them.  We won't be needing them.
	 */
	if (entp->bgpe_metrics) {
	    BGPM_FREE(entp->bgpe_metrics);
	    entp->bgpe_metrics = (bgp_metrics *) 0;
	}

	/*
	 * So far so good.  We have a group rto with our info entry
	 * attached to it.  If we have a new route, see if it's state
	 * matches anything queued on the route.  These won't need
	 * to be updated.  If we're deleting a route catch anyone
	 * who hasn't sent it yet.
	 */
	if (new_rt) {
	    if (checkit) {
		register bgpg_rtinfo_entry *infop;

		checkit = 0;
		infop = bgrto->bgpgo_info;
		while (infop) {
		    if (new_rt->rt_aspath == infop->bgp_info_asp
		      && nmetrics == infop->bgp_info_metrics) {
			break;
		    }
		    infop = infop->bgp_info_next;
		}

		if (infop) {
		    /*
		     * We can blow this away.  Unlink it from
		     * the list.  If we can blow the entire queue
		     * entry away we'll do that later.
		     */
		    BGP_RTINFO_UNLINK(bgrto, infop);
		    BRT_INFO_FREE(bgp, infop);
		    if (!bgrto->bgpgo_info) {
			no_old_state = 1;
		    }
		}
	    }

	    /*
	     * Now record the current metrics in the adv entry.
	     */
	    entp->bgpe_metrics = nmetrics;
	} else {
	    register bgpg_rtinfo_entry *infop;

	    /*
	     * This is a delete.  See if there is an infop
	     * for guys who haven't advertised this yet.
	     */
	    infop = bgrto->bgpgo_info;
	    while (infop) {
		if (!infop->bgp_info_asp) {
		    break;
		}
		infop = infop->bgp_info_next;
	    }

	    if (infop) {
		/*
		 * Blow this off.
		 */
		BGP_RTINFO_UNLINK(bgrto, infop);
		BRT_INFO_FREE_FAST(bgp, infop);
		if (!bgrto->bgpgo_info) {
		    no_old_state = 1;
		}
	    }
	}

	/*
	 * Remove the old rto entry from the list if there is one and
	 * we need to.
	 */
	if (still_on_list && (must_remove_from_list || no_old_state)) {
	    aspl = (bgp_asp_list *) bgrto->bgpgo_prev;
	    BGP_GRTO_UNLINK(bgrto);
	    if (!BGP_ASPL_EMPTY(aspl)) {
		aspl = (bgp_asp_list *) 0;
	    }
	    still_on_list = 0;
	    must_remove_from_list = 0;
	} else {
	    aspl = (bgp_asp_list *) 0;
	}

	if (no_old_state) {
	    /*
	     * Here we just free the rto entry.  If the
	     * route is deleted we reset the rtbit, otherwise
	     * we make sure the rtbit is set on the new route.
	     */
	    if (new_rt) {
		BIT_RESET(entp->bgpe_flags, BGPEF_QUEUED);
		BRT_TSI_PUT_ENT(rth, rtbit, entp);
	    } else {
		reset_rt = entp->bgpe_rt;
		BGP_ADV_DEQUEUE(entp);
		BRT_ENT_FREE(entp);
		BRT_TSI_CLEAR(rth, rtbit);
	    }
	    BRT_GRTO_FREE(bgrto);
	    no_old_state = 0;
	} else if (still_on_list) {
	    still_on_list = 0;
	    if (bgrto->bgpgo_time <= rto_time) {
		ready++;
	    } else if (nexttime == (time_t) 0 || nexttime > bgrto->bgpgo_time) {
		nexttime = bgrto->bgpgo_time;
	    }
	    total++;
	} else {
	    register bgp_asp_list *naspl;

	    /*
	     * Fetch an AS path list to put this on
	     */
	    naspl = bgp_get_asp_list(&(bgp->bgpg_asp_queue),
	      (new_rt ? new_rt->rt_aspath : (as_path *) 0));
	    if (naspl == aspl) {
		aspl = (bgp_asp_list *) 0;
	    }

	    /*
	     * Figure out where to put it.  If the time in the
	     * entry is zero, set the time to bgp_time_sec and
	     * queue it at the end if this is a normal update, or
	     * leave the time as zero and queue it at the start
	     * if this is a new policy update.
	     */
	    if (bgrto->bgpgo_time == (time_t) 0) {
		if (what == BRTUPD_NEWPOLICY) {
		    if (BGP_ASPL_EMPTY(naspl)
		      || naspl->bgpl_grto_prev->bgpgo_time == (time_t) 0) {
			BGP_GRTO_ADD_END(naspl, bgrto);
		    } else {
			BGP_GRTO_ADD_HEAD(naspl, bgrto);
		    }
		} else {
		    bgrto->bgpgo_time = bgp_time_sec;
		    BGP_GRTO_ADD_END(naspl, bgrto);
		}
	    } else if (BGP_ASPL_EMPTY(naspl)
		|| bgrto->bgpgo_time == bgp_time_sec) {
		/*
		 * Queue at the end, this is okay.
		 */
		BGP_GRTO_ADD_END(naspl, bgrto);
	    } else if (naspl->bgpl_grto_next->bgpgo_time > bgrto->bgpgo_time) {
		/*
		 * Goes at front.
		 */
		BGP_GRTO_ADD_HEAD(naspl, bgrto);
	    } else {
		bgpg_rto_entry *tmprto = naspl->bgpl_grto_prev;

		/*
		 * Hard case, sort it.
		 */
		for (;;) {
		    if (tmprto->bgpgo_time <= bgrto->bgpgo_time) {
			BGP_GRTO_ADD_AFTER(tmprto, bgrto);
			break;
		    }
		    tmprto = tmprto->bgpgo_prev;
		    assert(tmprto != (bgpg_rto_entry *)naspl);
		}
	    }

	    /*
	     * Figure out if this goes now or not
	     */
	    if (bgrto->bgpgo_time <= rto_time) {
		ready++;
	    } else if (nexttime == (time_t) 0 || nexttime > bgrto->bgpgo_time) {
		nexttime = bgrto->bgpgo_time;
	    }
	    total++;

	    if (new_list) {
		BIT_SET(entp->bgpe_flags, BGPEF_QUEUED);
		bgrto->bgpgo_advrt = entp;
		BRT_TSI_PUT_GRTO(rth, rtbit, bgrto);
		new_list = 0;
	    }
	}

	/*
	 * If we have an AS path list left, it will
	 * be empty.  Blow it away.
	 */
	if (aspl) {
	    BGP_ASPL_REMOVE(&(bgp->bgpg_asp_queue), aspl);
	    BRT_ASPL_FREE(aspl);
	    aspl = (bgp_asp_list *) 0;
	}

	/*
	 * Now determine what to do about our rtbit.
	 */
	if (reset_rt) {
	    /*
	     * A deleted route which needs to be reset.  Do it.
	     */
	    rtbit_reset(reset_rt, rtbit);
	    reset_rt = (rt_entry *) 0;
	} else if (!(entp->bgpe_rt)) {
	    /*
	     * No bit set anywhere, this can only go on the new
	     * route.
	     */
	    assert(new_rt);
	    rtbit_set(new_rt, rtbit);
	    entp->bgpe_rt = new_rt;
	} else if (new_rt) {
	    /*
	     * We need the bit set on the new route, make sure it
	     * is.  Turn off the bit on the old route.
	     */
	    if (entp->bgpe_rt != new_rt) {
		rtbit_set(new_rt, rtbit);
		rtbit_reset(entp->bgpe_rt, rtbit);
		entp->bgpe_rt = new_rt;
	    }
	} else {
	    /*
	     * Here we are working on deleting a route.  If there
	     * is a new active route set the bit on that, otherwise
	     * leave it on the route being deleted.
	     */
	    if (rth->rth_active && rth->rth_active != entp->bgpe_rt) {
		rtbit_set(rth->rth_active, rtbit);
		rtbit_reset(entp->bgpe_rt, rtbit);
		entp->bgpe_rt = rth->rth_active;
	    }
	}
    } RT_LIST_END(rth, rtl, rt_head);

    /*
     * All queued, close the route table
     */
    rt_close(bgp->bgpg_task, (gw_entry *) 0, 0, NULL);

    /*
     * If we have a new info entry remaining, free it.
     */
    if (ninfop) {
	BRT_INFO_FREE_FAST(bgp, ninfop);
    }

    /*
     * If we have a left over adv entry, dump that too
     */
    if (nentp) {
	BRT_ENT_FREE(nentp);
    }

    /*
     * Log what we did.
     */
    trace_tp(bgp->bgpg_task,
	     TR_NORMAL,
	     0,
	     ("bgp_rt_policy_group: %s %d route%s ready %d deferred",
	      bgp->bgpg_name,
	      ready,
	      ((ready == 1) ? "" : "s"),
	      (total - ready)));

    /*
     * If none were ready, make sure the timer was set right.
     */
    if (ready == 0 && bgp->bgpg_rto_next_time == (time_t) 0) {
	bgp->bgpg_rto_next_time = nexttime;
    }


    /*
     * Now if there is work start the write routine to bring this peer
     * up to date.  When he finishes writing he'll mark himself synchronized.
     */
    return ready;
}


/*
 * bgp_rt_policy_peer - run external peer policy in a flash/newpolicy/init
 *			update.  This queues stuff on the peer, returning
 *			an indication of what has been queued.
 */
static int
bgp_rt_policy_peer __PF3(bnp, bgpPeer *,
			 rtl, rt_list *,
			 what, int)
{
    rt_head *rth;
    rt_entry *new_rt, *old_rt;
    rt_changes *rtc;
    static bgp_metrics mymetrics = { 0 };
    bgp_metrics *nmetrics = (bgp_metrics *) 0;
    bgp_adv_entry *entp;
    bgp_adv_entry *nentp = (bgp_adv_entry *) 0;
    u_int rtbit = bnp->bgp_task->task_rtbit;
    bgp_rto_entry *brto;
    bgp_asp_list *aspl = (bgp_asp_list *) 0;
    as_path *old_asp;
    time_t nexttime = (time_t) 0;
    time_t rto_time;
    int new_list = 0;
    int ready = 0;
    int total = 0;
    int still_on_list = 0;
    rt_entry *reset_rt = (rt_entry *) 0;
    int must_remove_from_list = 0;
    int blow_old_state = 0;
    int isversion4 = (bnp->bgp_version == BGP_VERSION_4);

    trace_tp(bnp->bgp_task,
	     TR_NORMAL,
	     0,
	     ("bgp_rt_policy_peer: %s update %s",
	      trace_state(bgp_rt_update_bits, what),
	      bnp->bgp_name));

    /*
     * Compute the time for which this peer's routes are ready
     */
    rto_time = (bgp_time_sec >= bnp->bgp_rto_time)
	? (bgp_time_sec - bnp->bgp_rto_time) : (time_t) 0;

    /*
     * Open the routing table for bit diddling.
     */
    rt_open(bnp->bgp_task);

    /*
     * Now go around looking for things do to.
     */
    RT_LIST(rth, rtl, rt_head) {

	/*
	 * If this isn't an inet route, continue
	 */
	if (socktype(rth->rth_dest) != AF_INET) {
	    continue;
	}

	/*
	 * If this is an initial update and the route is on
	 * the flash list, forget it.  We'll do this when
	 * it flashes.
	 */
	if (what == BRTUPD_INITIAL) {
	    if (BIT_TEST(rth->rth_state, RTS_ONLIST)) {
		continue;
	    }
	    assert(rth->rth_active == rth->rth_last_active
		&& rth->rth_changes == (rt_changes *) 0);
	}

	/*
	 * If this isn't the first peer, see if we've announced the
	 * route already.  Otherwise we know it won't be announced
	 */
	old_rt = (rt_entry *) 0;
	if (what != BRTUPD_INITIAL && rth->rth_n_announce > 0) {
	    BRT_TSI_GET_PEER(rth, rtbit, brto, entp);
	    if (brto) {
		/*
		 * See if this is a delete.  If not this has our old
		 * route.
		 */
		if (entp->bgpe_metrics) {
		    old_rt = entp->bgpe_rt;
		} else {
		    assert(isversion4
		    	|| !BIT_TEST(entp->bgpe_flags, BGPEF_V4_ONLY));
		}
	    } else if (entp) {
		old_rt = entp->bgpe_rt;
		assert(old_rt == rth->rth_last_active);
	    }
	} else {
	    brto = (bgp_rto_entry *) 0;
	    entp = (bgp_adv_entry *) 0;
	}

	/*
	 * If there is no old route, or this is a new policy run,
	 * determine whether this is a version 4 only route or not.
	 * If we're running version 3 we won't be able to announce
	 * this.
	 */
	new_rt = rth->rth_active;
	if (new_rt && !old_rt) {
	    byte class = inet_class_flags(rth->rth_dest);
	    if (!entp) {
		if (nentp) {
		    nentp->bgpe_flags = (flag_t) 0;
		} else {
		    nentp = BRT_ENT_ALLOC();
		}
	    }

	    if (BIT_TEST(class, INET_CLASSF_LOOPBACK)) {
		continue;		/* can't do it */
	    } else if (BIT_TEST(class, INET_CLASSF_DEFAULT)) {
		if (rth->rth_dest_mask != inet_mask_default) {
		    if (!isversion4) {
			continue;
		    }
		    if (!entp) {
			BIT_SET(nentp->bgpe_flags, BGPEF_V4_ONLY);
		    }
		}
	    } else {
		if (!BIT_TEST(class, INET_CLASSF_NETWORK)) {
		    continue;	/* No one can announce this */
		}
		if (rth->rth_dest_mask != inet_mask_natural(rth->rth_dest)) {
		    if (!isversion4) {
			continue;	/* Can't do it */
		    }
		    if (!entp) {
			BIT_SET(nentp->bgpe_flags, BGPEF_V4_ONLY);
		    }
		}
	    }
	}


	/*
	 * If there is an old route, compute the AS path it
	 * was advertised with.  We made need this in any
	 * of several places below.
	 */
	rtc = rth->rth_changes;
	old_asp = (as_path *) 0;
	if (old_rt) {
	    if (rtc && BIT_TEST(rtc->rtc_flags, RTCF_ASPATH)) {
		old_asp = rtc->rtc_aspath;
	    } else {
		old_asp = old_rt->rt_aspath;
	    }
	}

	/*
	 * Now determine whether there is a new route to send.
	 */
	if (new_rt) {
	    adv_results results;

            bzero(&results, sizeof(results));
	    /*
	     * If we aren't allowed to advertise this route, or
	     * if it has no AS path, no need to continue.
	     */
	    if (!new_rt->rt_aspath
	      || BIT_TEST(new_rt->rt_state, RTS_NOADVISE|RTS_PENDING)) {
		new_rt = (rt_entry *) 0;
		goto dooldroute;
	    }

	    /*
	     * If the route has an AS loop it may deserve special
	     * consideration.
	     */
	    if (BIT_TEST(new_rt->rt_aspath->path_flags, PATH_FLAG_ASLOOP)) {
		if (isversion4) {
		    if (BIT_TEST(bnp->bgp_options, BGPO_NOV4ASLOOP)) {
			new_rt = (rt_entry *) 0;
			goto dooldroute;
		    }
		} else {
		    if (!BIT_TEST(bnp->bgp_options, BGPO_V3ASLOOPOKAY)) {
			new_rt = (rt_entry *) 0;
			goto dooldroute;
		    }
		}
	    }

	    /*
	     * So far so good.  Now we check hardcore policy
	     */
	    results.res_metric = bnp->bgp_metric_out;
	    if (!export(new_rt,
			(proto_t) 0,
			bnp->bgp_export,
			(adv_entry *) 0,
			(adv_entry *) 0,
			&results)) {
		new_rt = (rt_entry *) 0;
		goto dooldroute;
	    }

	    if (results.res_metric != BGP_METRIC_NONE) {
		mymetrics.bgpm_tmetric = BGPM_HAS_VALUE;
		mymetrics.bgpm_metric = results.res_metric;
	    }

	    /*
	     * Determine the next hop to send the route with.
	     */
	    if (new_rt->rt_n_gw
	      && BIT_TEST(new_rt->rt_state, RTS_GATEWAY)
	      && RT_IFAP(new_rt) == bnp->bgp_ifap) {
		mymetrics.bgpm_nexthop = sock2ip(RT_ROUTER(new_rt));
	    }

	    /*
	     * Find the metrics
	     */
	    BGPM_FIND(&mymetrics, nmetrics);
	    mymetrics.bgpm_types = mymetrics.bgpm_metric = (bvalue_t) 0;
	    mymetrics.bgpm_nexthop = BGPM_NO_NEXTHOP;

	    /*
	     * If we have an old route, see if this is unchanged from
	     * what we (wished to) announce for the old route.  If so
	     * don't bother worrying about it.
	     */
	    if (old_rt
	      && entp->bgpe_metrics == nmetrics
	      && new_rt->rt_aspath == old_asp) {
		if (old_rt != new_rt) {
		    rtbit_set(new_rt, rtbit);
		    rtbit_reset(old_rt, rtbit);
		    entp->bgpe_rt = new_rt;
		}
		BGPM_FREE(nmetrics);
		if (brto) {
		    total++;
		    if (brto->bgpo_time <= rto_time) {
			ready++;
		    } else if (nexttime == (time_t) 0
		      || nexttime > brto->bgpo_time) {
			nexttime = brto->bgpo_time;
		    }
		}
		continue;
	    }

	    /*
	     * Here we're got a keeper, acquire an adv entry for the
	     * route if there isn't one already and find a metrics
	     * pointer for this.
	     */
	    if (!entp) {
		entp = nentp;
		nentp = (bgp_adv_entry *) 0;
		BGP_ADV_ADD_BEFORE(&(bnp->bgp_queue), entp);
	    }
	}

dooldroute:
	if (!new_rt && !old_rt) {
	    if (entp) {
		/*
		 * Here we have a deletion queued, and are still
		 * interested in deleting it.  If there is a new
		 * active route move the bit there so we don't hold
		 * down a deleted route.  Then count this in with
		 * the ready routes
		 */
		if (rth->rth_active
		  && rth->rth_active != entp->bgpe_rt) {
		    rtbit_reset(entp->bgpe_rt, rtbit);
		    rtbit_set(rth->rth_active, rtbit);
		    entp->bgpe_rt = rth->rth_active;
		}
		if (brto->bgpo_time <= rto_time) {
		    ready++;
		} else if (nexttime == (time_t) 0
		  || nexttime > brto->bgpo_time) {
		    nexttime = brto->bgpo_time;
		}
		total++;
	    }
	    continue;
	}

	if (brto) {
	    /*
	     * Determine whether we'll need to remove this from
	     * the AS path list or not.  We won't if this is a new
	     * policy update and the AS path hasn't changed.
	     */
	    still_on_list = 1;
	    must_remove_from_list = 1;
	    if (old_rt && new_rt
	      && (what != BRTUPD_FLASH || brto->bgpo_time != (time_t) 0)
	      && new_rt->rt_aspath == old_asp) {
		must_remove_from_list = 0;
	    }
	} else {
	    brto = BRT_RTO_ALLOC();
	    new_list = 1;
	}

	if (old_rt) {
	    if (!still_on_list) {
		/*
		 * Collect the state of the route when we last announced it.
		 */
		brto->bgpo_asp = old_asp;
		ASPATH_ALLOC(brto->bgpo_asp);
		brto->bgpo_metrics = entp->bgpe_metrics;
	    } else {
		/*
		 * Blow away the old new_metrics, we won't need to know
		 * this.
		 */
		BGPM_FREE(entp->bgpe_metrics);
	    }
	}

	/*
	 * So far so good.  We have a peer rto with our info
	 * attached to it.  If we have a new route, see if it's state
	 * matches what is queued on the route.  These won't need
	 * to be updated.  If we're deleting a route see if it
	 * hasn't been sent yet.
	 */
	brto->bgpo_advrt = entp;
	if (new_rt) {
	    entp->bgpe_metrics = nmetrics;
	    if (brto->bgpo_asp == new_rt->rt_aspath
	      && brto->bgpo_metrics == nmetrics) {
		blow_old_state = 1;
	    }
	} else {
	    /*
	     * See if route was not announced, if so we're done here
	     * too since this is a delete.
	     */
	    entp->bgpe_metrics = (bgp_metrics *) 0;
	    if (!brto->bgpo_asp) {
		blow_old_state = 1;
	    }
	}

	if (still_on_list && (blow_old_state || must_remove_from_list)) {
	    aspl = (bgp_asp_list *) brto->bgpo_prev;
	    BGP_RTO_UNLINK(brto);
	    if (!BGP_ASPL_EMPTY(aspl)) {
		aspl = (bgp_asp_list *) 0;
	    }
	    still_on_list = 0;
	    must_remove_from_list = 0;
	}

	if (blow_old_state) {
	    if (new_rt) {
		BIT_RESET(entp->bgpe_flags, BGPEF_QUEUED);
		BRT_TSI_PUT_ENT(rth, rtbit, entp);
	    } else {
		reset_rt = entp->bgpe_rt;
		BGP_ADV_DEQUEUE(entp);
		BRT_ENT_FREE(entp);
		BRT_TSI_CLEAR(rth, rtbit);
	    }
	    BRT_RTO_FREE(brto);
	    blow_old_state = 0;
	    new_list = 0;
	} else if (still_on_list) {
	    /*
	     * Leave the rto entry alone, we'll blow it out
	     * when we get to it.  Figure out when it needs to
	     * go though.
	     */
	    still_on_list = 0;
	    if (brto->bgpo_time <= rto_time) {
		ready++;
	    } else if (nexttime == (time_t) 0 || nexttime > brto->bgpo_time) {
		nexttime = brto->bgpo_time;
	    }
	    total++;
	} else {
	    register bgp_asp_list *naspl;

	    naspl = bgp_get_asp_list(&(bnp->bgp_asp_queue),
	      (new_rt ? new_rt->rt_aspath : (as_path *) 0));
	    if (naspl == aspl) {
		aspl = (bgp_asp_list *) 0;
	    }

	    /*
	     * Figure out where to put it.  If the time in the
	     * entry is zero, set the time to bgp_time_sec and
	     * queue it at the end if this is a normal update, or
	     * leave the time as zero and queue it at the start
	     * if this is a new policy/initial update.
	     */
	    if (brto->bgpo_time == (time_t) 0) {
		if (what != BRTUPD_FLASH) {
		    if (BGP_ASPL_EMPTY(naspl)
		      || naspl->bgpl_rto_prev->bgpo_time == (time_t) 0) {
			BGP_RTO_ADD_END(naspl, brto);
		    } else {
			BGP_RTO_ADD_HEAD(naspl, brto);
		    }
		} else {
		    brto->bgpo_time = bgp_time_sec;
		    BGP_RTO_ADD_END(naspl, brto);
		}
	    } else if (BGP_ASPL_EMPTY(naspl)
		|| brto->bgpo_time == bgp_time_sec) {
		/*
		 * Queue at the end, this is okay.
		 */
		BGP_RTO_ADD_END(naspl, brto);
	    } else if (naspl->bgpl_rto_next->bgpo_time > brto->bgpo_time) {
		/*
		 * Goes at front.
		 */
		BGP_RTO_ADD_HEAD(naspl, brto);
	    } else {
		bgp_rto_entry *tmprto = naspl->bgpl_rto_prev;

		/*
		 * Hard case, sort it.
		 */
		for (;;) {
		    if (tmprto->bgpo_time <= brto->bgpo_time) {
			BGP_RTO_ADD_AFTER(tmprto, brto);
			break;
		    }
		    tmprto = tmprto->bgpo_prev;
		    assert(tmprto != (bgp_rto_entry *)naspl);
		}
	    }

	    /*
	     * Figure out if this goes now or not
	     */
	    if (brto->bgpo_time <= rto_time) {
		ready++;
	    } else if (nexttime == (time_t) 0 || nexttime > brto->bgpo_time) {
		nexttime = brto->bgpo_time;
	    }
	    total++;

	    if (new_list) {
		BIT_SET(entp->bgpe_flags, BGPEF_QUEUED);
		BRT_TSI_PUT_RTO(rth, rtbit, brto);
		new_list = 0;
	    }
	}

	/*
	 * Remove the AS path list entry, if we still have one.  This
	 * *must* be done now or resetting the bit on the route we
	 * previously thought we'd announce may release the path.
	 */
	if (aspl) {
	    BGP_ASPL_REMOVE(&(bnp->bgp_asp_queue), aspl);
	    BRT_ASPL_FREE(aspl);
	    aspl = (bgp_asp_list *) 0;
	}

	/*
	 * Now determine what to do with the rtbit.  The adv entry
	 * will point at the route with our bit currently set.
	 * See if it needs to be moved.
	 */
	if (reset_rt) {
	    /*
	     * A deleted route which needs to be reset.  Do it.
	     */
	    rtbit_reset(reset_rt, rtbit);
	    reset_rt = (rt_entry *) 0;
	} else if (!(entp->bgpe_rt)) {
	    /*
	     * No bit set anywhere, this can only go on the new
	     * route.
	     */
	    assert(new_rt);
	    rtbit_set(new_rt, rtbit);
	    entp->bgpe_rt = new_rt;
	} else if (new_rt) {
	    /*
	     * We need the bit set on the new route, make sure it
	     * is.  Turn off the bit on the old route.
	     */
	    if (entp->bgpe_rt != new_rt) {
		rtbit_set(new_rt, rtbit);
		rtbit_reset(entp->bgpe_rt, rtbit);
		entp->bgpe_rt = new_rt;
	    }
	} else {
	    /*
	     * Here we are working on deleting a route.  If there
	     * is a new active route set the bit on that, otherwise
	     * leave it on the route being deleted.
	     */
	    if (rth->rth_active && rth->rth_active != entp->bgpe_rt) {
		rtbit_set(rth->rth_active, rtbit);
		rtbit_reset(entp->bgpe_rt, rtbit);
		entp->bgpe_rt = rth->rth_active;
	    }
	}
    } RT_LIST_END(rth, rtl, rt_head);

    /*
     * All queued, close the route table
     */
    rt_close(bnp->bgp_task, &bnp->bgp_gw, 0, NULL);

    /*
     * If we have an adv entry left over, free it now
     */
    if (nentp) {
	BRT_ENT_FREE(nentp);
    }

    /*
     * Log what we did.
     */
    trace_tp(bnp->bgp_task,
	     TR_NORMAL,
	     0,
	     ("bgp_rt_policy_peer: %s %d route%s ready %d deferred",
	      bnp->bgp_name,
	      ready,
	      ((ready == 1) ? "" : "s"),
	      (total - ready)));

    /*
     * If none were ready, make sure the timer was set right.
     */
    if (ready == 0 && bnp->bgp_rto_next_time == (time_t) 0) {
	bnp->bgp_rto_next_time = nexttime;
    }

    /*
     * Now if there is work start the write routine to bring this peer
     * up to date.  When he finishes writing he'll mark himself synchronized.
     */
    return ready;
}


/*
 * bgp_rt_if_terminate - all peers in a TEST or INTERNAL group running
 *			 on a particular interface have terminated.
 *			 Adjust adv entries which have third-party
 *			 next hops through this interface.
 */
void
bgp_rt_if_terminate __PF2(bgp, bgpPeerGroup *,
			  ifap, if_addr *)
{
    register bgp_adv_entry *entp;
    register bgp_metrics *mp;
    register rt_entry *rt;
    register rt_changes *rtc;
    bgp_metrics mymetrics;

    /*
     * Must be TEST or INTERNAL group
     */
    assert(bgp->bgpg_type == BGPG_TEST || bgp->bgpg_type == BGPG_INTERNAL);

#define	BRTVTOENT(v)	((bgp_adv_entry *)(v))
    for (entp = BRTVTOENT(bgp->bgpg_queue.bgpv_next);
    	 entp != BRTVTOENT(&(bgp->bgpg_queue));
	 entp = BRTVTOENT(entp->bgpe_next)) {

	mp = entp->bgpe_metrics;
	if (mp && mp->bgpm_nexthop) {
	    rt = entp->bgpe_rt;
	    rtc = rt->rt_head->rth_changes;
	    if (rtc && BIT_TEST(rtc->rtc_flags, RTCF_NEXTHOP)) {
		if (!rtc->rtc_n_gw || RTC_IFAP(rtc) != ifap) {
		    continue;
		}
	    } else {
		if (!rt->rt_n_gw || RT_IFAP(rt) != ifap) {
		    continue;
		}
	    }
	    mymetrics = *mp;	/* struct copy */
	    mymetrics.bgpm_nexthop = BGPM_NO_NEXTHOP;
	    BGPM_FREE(mp);
	    BGPM_FIND(&mymetrics, entp->bgpe_metrics);
	}
    }
#undef	BRTVTOENT
}


/*
 * bgp_rt_peer_update_cleanup - terminate a peer which failed during a
 *			    flash or new policy update.
 */
static void
bgp_rt_peer_update_cleanup __PF1(tjp, task_job *)
{
    bgpPeer *bnp = (bgpPeer *)(tjp->task_job_data);

    BIT_RESET(bnp->bgp_flags, BGPF_CLEANUP);
    if (BIT_TEST(bnp->bgp_flags, BGPF_WRITEFAILED)) {
	bgp_peer_close(bnp, BGPEVENT_ERROR);
    }
}


/*
 * bgp_rt_peer_update - flash/new policy update for a peer
 */
static void
bgp_rt_peer_update __PF3(tp, task *,
			 rtl, rt_list *,
			 what, int)
{
    bgpPeer *bnp = (bgpPeer *)(tp->task_data);
    int res;

    assert(bnp->bgp_group->bgpg_type == BGPG_EXTERNAL);
    assert(bnp->bgp_state == BGPSTATE_ESTABLISHED);

    /*
     * First update this guy's routes
     */
    res = bgp_rt_policy_peer(bnp, rtl, what);

    /*
     * Now decide whether we need to send the routes now or
     * not.  Don't bother if we don't have any routes or if
     * we're write failed/write blocked.
     */
    if (BGP_RTQ_EMPTY(&(bnp->bgp_asp_queue))
      || BIT_TEST(bnp->bgp_flags, BGPF_WRITEFAILED|BGPF_SEND_RTN)) {
	bnp->bgp_rto_next_time = (time_t) 0;
	return;
    }

    /*
     * If he returned none ready, but there are some future changes
     * to go, make sure his timer is set.  Then return.
     */
    if (!res) {
	assert(bnp->bgp_rto_next_time);
	if (!BIT_TEST(bnp->bgp_flags, BGPF_RT_TIMER)
	  || what == BRTUPD_NEWPOLICY) {
	    bgp_route_timer_set(bnp);
	}
	return;
    }

    /*
     * Try to flush some of this out.
     */
    bnp->bgp_rto_next_time = (time_t) 0;
    res = bgp_rt_send_peer(bnp, TRUE);

    if (res != SEND_OK) {
	if (BIT_TEST(bnp->bgp_flags, BGPF_WRITEFAILED)) {
	    /*
	     * If we've had a write failure, queue a task job to
	     * clean up afterwards.
	     */
	    BIT_SET(bnp->bgp_flags, BGPF_CLEANUP);
	    (void) task_job_create(bnp->bgp_task,
				   TASK_JOB_FG,
				   "Peer update failure cleanup",
				   bgp_rt_peer_update_cleanup,
				   (void_t) bnp);
	}
    } else if (bnp->bgp_rto_next_time
      && (!BIT_TEST(bnp->bgp_flags, BGPF_RT_TIMER)
	|| what == BRTUPD_NEWPOLICY)) {
	bgp_route_timer_set(bnp);
    }
}


/*
 * bgp_peer_flash - flash update for a peer
 */
static void
bgp_peer_flash __PF2(tp, task *,
		     rtl, rt_list *)
{
    bgp_rt_peer_update(tp, rtl, BRTUPD_FLASH);
}


/*
 * bgp_peer_newpolicy - new policy update for a peer
 */
static void
bgp_peer_newpolicy __PF2(tp, task *,
			 rtl, rt_list *)
{
    bgp_rt_peer_update(tp, rtl, BRTUPD_NEWPOLICY);
}


/*
 * bgp_rt_peer_flush - flush some data out to a peer after a timer
 *		       expiry or write unblock event.
 */
static void
bgp_rt_peer_flush __PF1(bnp, bgpPeer *)
{
    int res;

    bnp->bgp_rto_next_time = (time_t) 0;
    if (BIT_TEST(bnp->bgp_flags, BGPF_WRITEFAILED)
      || BGP_RTQ_EMPTY(&(bnp->bgp_asp_queue))) {
	/*
	 * Don't bother, let's hope it'll clean itself up.
	 */
	return;
    }

    /*
     * Now do the send.  We're outside the flash routine here
     */
    res = bgp_rt_send_peer(bnp, FALSE);

    /*
     * If we had a write failure we should close this peer.  Otherwise
     * the cleanup should take care of itself.
     */
    if (res != SEND_OK) {
	if (BIT_TEST(bnp->bgp_flags, BGPF_WRITEFAILED)) {
	    bgp_peer_close(bnp, BGPEVENT_ERROR);
	}
    } else if (bnp->bgp_rto_next_time
      && !BIT_TEST(bnp->bgp_flags, BGPF_RT_TIMER)) {
	bgp_route_timer_set(bnp);
    }
}




/*
 * bgp_rt_group_update - flash/new policy update a peer group
 */
static void
bgp_rt_group_update __PF3(tp, task *,
			  rtl, rt_list *,
			  what, int)
{
    bgpPeerGroup *bgp = (bgpPeerGroup *)tp->task_data;
    bgpPeer *bnp;
    int res;
    time_t v4time = (time_t) 0;

    /*
     * First flash the group
     */
    res = bgp_rt_policy_group(bgp, rtl, what);

    /*
     * Now decide whether we need to send routes now
     */
    if (BGP_RTQ_EMPTY(&(bgp->bgpg_asp_queue))
      || (bgp->bgpg_n_v3_sync == 0 && bgp->bgpg_n_v4_sync == 0)) {
	bgp->bgpg_rto_next_time = (time_t) 0;
	return;
    }

    /*
     * If everything is deferred, make sure the timer is set
     */
    if (!res) {
	if (bgp->bgpg_rto_next_time) {
	    if (!BIT_TEST(bgp->bgpg_flags, BGPGF_RT_TIMER)
	      || what == BRTUPD_NEWPOLICY) {
		bgp_group_route_timer_set(bgp);
	    }
	} else {
	    assert((bgp->bgpg_n_v3_sync + bgp->bgpg_n_v4_sync)
	      != bgp->bgpg_n_established);
	}
	return;
    }

    /*
     * We need to send some stuff.  Decide how to do this.
     */
    res = SEND_OK;
    if (bgp->bgpg_n_v4_sync > 0) {
	bgp->bgpg_rto_next_time = (time_t) 0;
	if (bgp->bgpg_n_v4_sync == 1) {
	    register bgp_bits *sbits;

	    /*
	     * Find the peer in sync and flash him by himself.  This
	     * is easier.
	     */
	    sbits = BGPG_GETBITS(bgp->bgpg_v4_sync, bgp->bgpg_idx_size);
	    BGP_PEER_LIST(bgp, bnp) {
		if (bnp->bgp_state == BGPSTATE_ESTABLISHED
		  && BGPB_BTEST(sbits, bnp->bgp_group_bit)) {
		    break;
		}
	    } BGP_PEER_LIST_END(bgp, bnp);
	    assert(bnp);
	    res |= bgp_rt_send_group_peer(bnp, TRUE);
	} else {
	    res |= bgp_rt_send_v4_group(bgp, TRUE);
	}
	v4time = bgp->bgpg_rto_next_time;
    }

    /*
     * Same thing for the version 3 peers
     */
    if (bgp->bgpg_n_v3_sync > 0) {
	bgp->bgpg_rto_next_time = (time_t) 0;
	if (bgp->bgpg_n_v3_sync == 1) {
	    register bgp_bits *sbits;

	    /*
	     * Find the peer in sync and flash him by himself.  This
	     * is easier.
	     */
	    sbits = BGPG_GETBITS(bgp->bgpg_v3_sync, bgp->bgpg_idx_size);
	    BGP_PEER_LIST(bgp, bnp) {
		if (bnp->bgp_state == BGPSTATE_ESTABLISHED
		  && BGPB_BTEST(sbits, bnp->bgp_group_bit)) {
		    break;
		}
	    } BGP_PEER_LIST_END(bgp, bnp);
	    assert(bnp);
	    res = bgp_rt_send_group_peer(bnp, TRUE);
	} else {
	    res = bgp_rt_send_v2or3_group(bgp, TRUE);
	}
    }

    if (BIT_TEST(res, SEND_FAILED)) {
	/*
	 * Find all the peers which failed and schedule a cleanup
	 * job for each.
	 */
	BGP_PEER_LIST(bgp, bnp) {
	    if (BIT_COMPARE(bnp->bgp_flags, BGPF_WRITEFAILED|BGPF_CLEANUP, BGPF_WRITEFAILED)) {
		(void) task_job_create(bnp->bgp_task,
				       TASK_JOB_FG,
				       "Group update failure cleanup",
				       bgp_rt_peer_update_cleanup,
				       (void_t) bnp);
		BIT_SET(bnp->bgp_flags, BGPF_CLEANUP);
	    }
	} BGP_PEER_LIST_END(bgp, bnp);
    }

    /*
     * Figure out if we need to schedule a transmission sometime in
     * the future.
     */
    if (bgp->bgpg_n_v3_sync != 0 || bgp->bgpg_n_v4_sync != 0) {
	if (v4time && !(bgp->bgpg_rto_next_time)) {
	    bgp->bgpg_rto_next_time = v4time;
	}
	if (bgp->bgpg_rto_next_time
	  && (!BIT_TEST(bgp->bgpg_flags, BGPGF_RT_TIMER)
	    || what == BRTUPD_NEWPOLICY)) {
	    bgp_group_route_timer_set(bgp);
	}
    }
}


/*
 * bgp_rt_group_flush - flush some data out to a group, or a peer
 *		     in a group, after a timer expirty or a write
 *		     unblock event.
 */
static void
bgp_rt_group_flush __PF2(bgp, bgpPeerGroup *,
			 bnp, bgpPeer *)
{
    int res = SEND_OK;
    time_t v4time = (time_t) 0;

    /*
     * Check to see whether there is anything to send
     */
    if (BGP_RTQ_EMPTY(&(bgp->bgpg_asp_queue))) {
	return;
    }

    /*
     * See if we have a specific peer.  If so he may be out
     * of sync, so check to see if this puts him back.
     */
    if (bnp) {
	/*
	 * Flushing a specific peer here.  Just do it quick.
	 */
	if (BIT_TEST(bnp->bgp_flags, BGPF_WRITEFAILED)) {
	    /*
	     * Don't bother, probably queued in a task job.
	     */
	    return;
	}
	res = bgp_rt_send_group_peer(bnp, FALSE);

	if (res != SEND_OK) {
	    if (BIT_TEST(bnp->bgp_flags, BGPF_WRITEFAILED)) {
		bgp_peer_close(bnp, BGPEVENT_ERROR);
	    }
	} else if (bgp->bgpg_rto_next_time
	  && !BIT_TEST(bgp->bgpg_flags, BGPGF_RT_TIMER)) {
	    bgp_group_route_timer_set(bgp);
	}
	return;
    }

    /*
     * Here we're flushing a group, or at least the in-sync peers
     * in a group.  See if we can flush individual peers, since this
     * is easier.
     */
    if (bgp->bgpg_n_v3_sync == 0 && bgp->bgpg_n_v4_sync == 0) {
	bgp->bgpg_rto_next_time = (time_t) 0;
	return;
    }

    /*
     * Do the version 4 peers first
     */
    if (bgp->bgpg_n_v4_sync > 0) {
	bgp->bgpg_rto_next_time = (time_t) 0;
	if (bgp->bgpg_n_v4_sync == 1) {
	    register bgp_bits *sbits;

	    /*
	     * Find the peer in sync and flash him by himself.  This
	     * is easier.
	     */
	    sbits = BGPG_GETBITS(bgp->bgpg_v4_sync, bgp->bgpg_idx_size);
	    BGP_PEER_LIST(bgp, bnp) {
		if (bnp->bgp_state == BGPSTATE_ESTABLISHED
		  && BGPB_BTEST(sbits, bnp->bgp_group_bit)) {
		    break;
		}
	    } BGP_PEER_LIST_END(bgp, bnp);
	    assert(bnp);
	    res = bgp_rt_send_group_peer(bnp, FALSE);
	    if (BIT_TEST(res, SEND_FAILED)) {
		/*
		 * Dump the peer right now since we know it.
		 */
		bgp_peer_close(bnp, BGPEVENT_ERROR);
		res = SEND_OK;
	    }
	} else {
	    res = bgp_rt_send_v4_group(bgp, FALSE);
	}
	v4time = bgp->bgpg_rto_next_time;
    }

    /*
     * Now try the version 3 peers
     */
    if (bgp->bgpg_n_v3_sync > 0 && !BGP_RTQ_EMPTY(&(bgp->bgpg_asp_queue))) {
	bgp->bgpg_rto_next_time = (time_t) 0;
	if (bgp->bgpg_n_v3_sync == 1) {
	    register bgp_bits *sbits;
	    int myres;

	    /*
	     * Find the peer in sync and flash him by himself.  This
	     * is easier.
	     */
	    sbits = BGPG_GETBITS(bgp->bgpg_v3_sync, bgp->bgpg_idx_size);
	    BGP_PEER_LIST(bgp, bnp) {
		if (bnp->bgp_state == BGPSTATE_ESTABLISHED
		  && BGPB_BTEST(sbits, bnp->bgp_group_bit)) {
		    break;
		}
	    } BGP_PEER_LIST_END(bgp, bnp);
	    assert(bnp);
	    myres = bgp_rt_send_group_peer(bnp, FALSE);
	    if (BIT_TEST(myres, SEND_FAILED)) {
		/*
		 * Dump the bugger, he died.
		 */
		bgp_peer_close(bnp, BGPEVENT_ERROR);
	    }
	} else {
	    res |= bgp_rt_send_v2or3_group(bgp, FALSE);
	}
    }

    /*
     * Failure cleanup
     */
    if (BIT_TEST(res, SEND_FAILED)) {
	/*
	 * Find all the peers which failed and close them
	 */
	BGP_PEER_LIST(bgp, bnp) {
	    if (BIT_COMPARE(bnp->bgp_flags,
			    BGPF_WRITEFAILED|BGPF_CLEANUP,
			    BGPF_WRITEFAILED)) {
		bgp_peer_close(bnp, BGPEVENT_ERROR);
	    }
	} BGP_PEER_LIST_END(bgp, bnp);
    }

    /*
     * See if we've got some future work to do
     */
    if (bgp->bgpg_n_v3_sync > 0 || bgp->bgpg_n_v4_sync > 0) {
	if (v4time && !(bgp->bgpg_rto_next_time)) {
	    bgp->bgpg_rto_next_time = v4time;
	}
	if (bgp->bgpg_rto_next_time
	  && !BIT_TEST(bgp->bgpg_flags, BGPGF_RT_TIMER)) {
	    bgp_group_route_timer_set(bgp);
	}
    }
}


/*
 * bgp_group_flash - flash update for a group
 */
static void
bgp_group_flash __PF2(tp, task *,
		      rtl, rt_list *)
{
    bgp_rt_group_update(tp, rtl, BRTUPD_FLASH);
}


/*
 * bgp_group_newpolicy - new policy update for a group
 */
static void
bgp_group_newpolicy __PF2(tp, task *,
			  rtl, rt_list *)
{
    bgp_rt_group_update(tp, rtl, BRTUPD_NEWPOLICY);
}

/*
 * bgp_aux_flash - flash update for an INTERNAL_IGP group
 */
void
bgp_aux_flash __PF2(tp, task *,
		    rtl, rt_list *)
{
    bgpPeerGroup *bgp = (bgpPeerGroup *)(tp->task_data);

    assert(bgp->bgpg_type == BGPG_INTERNAL_IGP);

    /*
     * If we have nothing established yet, return.  Otherwise do
     * the flash.
     */

    if (bgp->bgpg_n_established == 0) {
	return;
    }

    bgp_rt_group_update(tp, rtl, BRTUPD_FLASH);
}


/*
 * bgp_aux_newpolicy - flash update for an INTERNAL_IGP group
 */
void
bgp_aux_newpolicy __PF2(tp, task *,
			rtl, rt_list *)
{
    bgpPeerGroup *bgp = (bgpPeerGroup *)(tp->task_data);

    assert(bgp->bgpg_type == BGPG_INTERNAL_IGP);

    /*
     * If we have nothing established yet, return.  Otherwise do
     * the new policy update.
     */

    if (bgp->bgpg_n_established == 0) {
	return;
    }

    bgp_rt_group_update(tp, rtl, BRTUPD_NEWPOLICY);
}


/*
 * bgp_recv_v2or3_update - receive an update from a peer.  This is actually
 *		        the task_read routine when we are in established state
 */
static void
bgp_recv_v2or3_update(tp)
task *tp;
{
    register byte *cp, *cpend;
    register bgpPeer *bnp;
    register rt_entry *rt;
    rt_head *rth;
    as_path *asp = NULL;
    bgpPeerGroup *bgp;
    as_path_info api;
    rt_parms rtparms;
    sockaddr_un nexthop;
    sockaddr_un dest;
    sockaddr_un *mask;
    size_t len;
    int type;
    byte *attrp;
    size_t attrlen;
    u_int routelen;
    int error_code;
    byte *error_data;
    int count, maybemore = 1;
    int dopref = 0;
    pref_t defpref;
    u_long max_octets;
    u_long start_octets;
    u_long start_updates;
    u_int routes_in = 0;
    bgp_sync *bsp = (bgp_sync *) 0;
    int event = BGPEVENT_RECVUPDATE;

    /*
     * Log this for posterity
     */
    bnp = (bgpPeer *)tp->task_data;
    bgp = bnp->bgp_group;
    trace_tp(bnp->bgp_task,
	     TR_NORMAL,
	     0,
	     ("bgp_recv_v2or3_update: receiving updates from %s",
	      bnp->bgp_name));
 
    /*
     * If this is an internal routing guy, fetch his structure
     */
    if (bgp->bgpg_type == BGPG_INTERNAL_RT) {
	bsp = bgp->bgpg_sync;
	assert(bsp);
    }

    /*
     * Fetch the default preference.  If this is an internal group and
     * the setpref option was used we'll need to modify this.
     */
    defpref = bnp->bgp_preference;
    if (BGP_GROUP_INTERNAL(bgp) && BIT_TEST(bnp->bgp_options, BGPO_SETPREF)) {
	dopref = 1;
    }

    /*
     * Go around forever trying to read the socket.  We break when we can't
     * get enough to make a full packet.
     */
    bzero((caddr_t) &rtparms, sizeof(rtparms));
    rtparms.rtp_dest = &dest;
    rtparms.rtp_n_gw = 1;
    rtparms.rtp_router = &nexthop;
    rtparms.rtp_gwp = &bnp->bgp_gw;
    rtparms.rtp_preference2 = bnp->bgp_preference2;
    if (bgp->bgpg_type == BGPG_INTERNAL || bgp->bgpg_type == BGPG_INTERNAL_RT) {
	rtparms.rtp_state = RTS_INTERIOR|RTS_EXTERIOR;
    } else if (bgp->bgpg_type == BGPG_INTERNAL_IGP) {
	rtparms.rtp_state = RTS_INTERIOR|RTS_EXTERIOR|RTS_NOTINSTALL|RTS_NOADVISE;
    } else {
	rtparms.rtp_state = RTS_EXTERIOR;
    }
    rtparms.rtp_rtd = NULL;
    if (TRACE_DETAIL_RECV_TP(tp, BGP_UPDATE, BGP_PACKET_MAX, bgp_trace_masks)) {
	max_octets = bnp->bgp_in_octets + BGPMAXREADTRACEPKTS;
    } else {
	max_octets = bnp->bgp_in_octets + BGPMAXREAD;
    }
    start_octets = bnp->bgp_in_octets;
    start_updates = bnp->bgp_in_updates;
    sockclear_in(&dest);
    rt_open(tp);

    for (;;) {
	/*
	 * Try to read some.  We keep track of whether we got a full
	 * buffer or not so we'll know whether there is a good chance
	 * of getting some next time around.
	 */
	count = bgp_recv(tp, &bnp->bgp_inbuf, 0, bnp->bgp_name);
	if (count < 0) {
	    event = BGPEVENT_CLOSED;
	    goto deadduck;
	}

	cp = bnp->bgp_bufpos;
	if (BGPBUF_SPACE(bnp) > 0) {
	    maybemore = 0;
	}

	/*
	 * Now loop reading messages out of this buffer.  We do this until
	 * we haven't got enough data for a full message.
	 */
	for (;;) {
	    /*
	     * See if we've got a full header now.  If not break
	     * out.  If so, decode it.
	     */
	    if (BGPBUF_LEFT(bnp, cp) < BGP_HEADER_LEN) {
		break;
	    }

	    BGP_GET_HDRLEN(len, cp);

	    /*
	     * Check for a strange length.
	     */
	    if (len < BGP_HEADER_LEN || len > BGPMAXPACKETSIZE) {
		trace_log_tp(tp,
			     0,
			     LOG_WARNING,
			     ("bgp_recv_v2or3_update: peer %s: strange message header length %d",
			      bnp->bgp_name,
			      len));
	    	bgp_send_notify_word(bnp,
				     BGP_ERR_HEADER,
				     BGP_ERRHDR_LENGTH,
				     (int) len);
		event = BGPEVENT_ERROR;	/* not quite right */
		goto deadduck;
	    }

	    /*
	     * See if we've got the full message buffered.  If not,
	     * break out of this.
	     */
	    if (BGPBUF_LEFT(bnp, cp) < len) {
		break;
	    }

	    /*
	     * If we are tracing, do it now.  This is as uneditted as
	     * the packet gets.
	     */
	    if (TRACE_PACKET_RECV_TP(tp,
				     BGP_UPDATE,
				     BGP_PACKET_MAX,
				     bgp_trace_masks)) {
		bgp_trace(bnp,
			  (bgpProtoPeer *) NULL,
			  "BGP RECV ",
			  0,
			  cp,
			  TRACE_DETAIL_RECV_TP(tp,
					       BGP_UPDATE,
					       BGP_PACKET_MAX,
					       bgp_trace_masks));
	    }

	    /*
	     * Check for an in-range type.  If not complain about this.
	     */
	    BGP_GET_HDRTYPE(type, cp);
	    if (type == 0 || type > BGP_KEEPALIVE) {
	        trace_log_tp(tp,
			     0,
			     LOG_WARNING,
			     ("bgp_recv_v2or3_update: peer %s unrecognized message type %d",
			      bnp->bgp_name,
			      type));
		bgp_send_notify_byte(bnp, BGP_ERR_HEADER,
				     BGP_ERRHDR_TYPE, type);
		event = BGPEVENT_ERROR;	/* not quite right */
		goto deadduck;
	    }

	    /*
	     * Record that we received something
	     */
	    bnp->bgp_last_rcvd = time_sec;

	    /*
	     * Check message authentication.  If failed we're dead.
	     */
	    if (!BIT_TEST(bnp->bgp_options, BGPO_NOAUTHCHECK)
	      && !BGP_CHECK_AUTH(&(bnp->bgp_authinfo),
				 bnp->bgp_task,
				 cp,
				 len)) {
		bgp_send_notify_none(bnp, BGP_ERR_HEADER, BGP_ERRHDR_UNSYNC);
		event = BGPEVENT_CLOSED;	/* not quite right */
		goto deadduck;
	    }

	    /*
	     * Check the type.  This sets the event and filters out
	     * everything except updates.  For keepalives we go
	     * around again.
	     */
	    bnp->bgp_in_octets += (u_long) len;
	    switch(type) {
	    case BGP_OPEN:
		trace_log_tp(tp,
			     0,
			     LOG_ERR,
			     ("bgp_recv_v2or3_update: received OPEN message from %s, state is ESTABLISHED",
			      bnp->bgp_name));
		bnp->bgp_in_notupdates++;
		bgp_send_notify_none(bnp, BGP_ERR_FSM, 0);
		event = BGPEVENT_RECVOPEN;
		goto deadduck;

	    case BGP_KEEPALIVE:
		bnp->bgp_in_notupdates++;
		if (len != BGP_HEADER_LEN) {
		    bgp_send_notify_word(bnp,
					 BGP_ERR_HEADER,
					 BGP_ERRHDR_LENGTH,
					 (int) len);
		    event = BGPEVENT_RECVKEEPALIVE;
		    goto deadduck;
		}
		bnp->bgp_lastevent = BGPEVENT_RECVKEEPALIVE;
		break;

	    case BGP_NOTIFY:
		bnp->bgp_in_notupdates++;
		bgp_log_notify(bnp->bgp_trace_options,
			       bnp->bgp_name,
			       cp,
			       len,
			       0);
		event = BGPEVENT_RECVNOTIFY;
		goto deadduck;

	    case BGP_UPDATE:
	        bnp->bgp_in_updates++;
		break;
	    }

	    cpend = cp + len;
	    BGP_SKIP_HEADER(cp);
	    if (type == BGP_KEEPALIVE) {
		bnp->bgp_bufpos = cp;
		continue;
	    }

	    /*
	     * Here we have an update message.  Make sure we have enough
	     * length for the attributes, then fetch the attributes and
	     * their length.
	     */
	    if ((cpend - cp) < BGP_UPDATE_MIN_LEN) {
		bgp_send_notify_word(bnp,
				     BGP_ERR_HEADER,
				     BGP_ERRHDR_LENGTH,
				     (int) len);
		trace_log_tp(tp,
			     0,
			     LOG_WARNING,
			     ("bgp_recv_v2or3_update: peer %s UPDATE length %d too small",
			      bnp->bgp_name,
			      len));
		goto deadduck;
	    }

	    /*
	     * Fetch the attribute length and a pointer to the attributes.
	     * If this length was screwed up, or what it leaves us isn't
	     * divisible by 4, we're dead.
	     */
	    BGP_GET_UPDATE(attrlen, attrp, cp);
	    routelen = cpend - cp;
	    if (cpend < cp || (routelen & (BGP_ROUTE_LENGTH-1)) != 0) {
		trace_log_tp(tp,
			     0,
			     LOG_WARNING,
			     ("bgp_recv_v2or3_update: peer %s UPDATE path attributes malformed",
			      bnp->bgp_name));
		bgp_send_notify_none(bnp, BGP_ERR_UPDATE, BGP_ERRUPD_ATTRLIST);
		goto deadduck;
	    }

	    /*
	     * From here on the going gets a little expensive.  For TEST
	     * peers, don't bother with the rest of the message.
	     */
	    if (bnp->bgp_group->bgpg_type == BGPG_TEST) {
		cp = cpend;
	        bnp->bgp_bufpos = cp;
		continue;
	    }

	    /*
	     * Now fetch the AS path.  If this detects an error, barf.
	     */
	    api.api_metric = BGP_METRIC_NONE;
	    api.api_nexthop = &nexthop;
	    asp = aspath_attr(attrp,
			      attrlen,
#ifdef	PATH_VERSION_4
			      PATH_VERSION_2OR3,
#endif	/* PATH_VERSION_4 */
			      bnp->bgp_local_as,
			      &api,
			      &error_code,
			      &error_data);
	    if (asp == NULL) {
		bgp_path_attr_error(bnp, error_code, error_data,
		  "bgp_recv_v2or3_update");
		goto deadduck;
	    }

	    /*
	     * If the next hop wasn't included, complain about this.
	     */
	    if (!BIT_TEST(api.api_flags, APIF_NEXTHOP)) {
		trace_log_tp(tp,
			     0,
			     LOG_WARNING,
			     ("bgp_recv_v2or3_update: peer %s UPDATE no next hop found",
			      bnp->bgp_name));
		bgp_send_notify_byte(bnp, BGP_ERR_UPDATE,
				     BGP_ERRUPD_MISSING, PA_TYPE_NEXTHOP);
		goto deadduck;
	    }


	    /*
	     * Process unreachables separately since we aren't overly
	     * fussy about path attributes in this case and we can gain
	     * a bit of speed processing these.
	     */
	    if (BIT_TEST(api.api_flags, APIF_UNREACH)) {
	        u_int route_count = 0;
		byte netclass;

		while (cp < cpend) {
		    route_count++;
		    BGP_GET_ADDR(&dest, cp);
		    netclass = inet_class_flags(&dest);
		    mask = inet_mask_natural(&dest);
		    if (!(netclass & INET_CLASSF_NETWORK)
			|| sock2host(&dest, mask) != 0) {
			trace_log_tp(tp,
				     0,
				     LOG_ERR,
				     ("bgp_recv_v2or3_update: peer %s update included invalid net %A (%u of %u)",
				      bnp->bgp_name,
				      &dest,
				      route_count,
				      routelen/BGP_ROUTE_LENGTH));
			bgp_send_notify_none(bnp, BGP_ERR_UPDATE,
					     BGP_ERRUPD_BADNET);
			goto deadduck;
		    }

		    /*
		     * Try to find existing net from this guy.  If it doesn't
		     * exist, complain if we think we have them all.
		     */
		    rth = rt_table_locate(&dest, mask);
		    if (!rth) {
			rt = (rt_entry *) 0;
		    } else {
			RT_ALLRT(rt, rth) {
			    if (rt->rt_gwp == &(bnp->bgp_gw)) {
				break;
			    }
			} RT_ALLRT_END(rt, rth);
		    }

		    if (!rt) {
			if (BIT_MATCH(bnp->bgp_options,
				      BGPO_KEEPALL|BGPO_ANALRETENTIVE)) {
			    trace_log_tp(tp,
					 0,
					 LOG_WARNING,
					 ("bgp_recv_v2or3_update: peer %s unreachable net %A not sent by him (%u of %u)",
					  bnp->bgp_name,
					  &dest,
					  route_count,
					  routelen/BGP_ROUTE_LENGTH));
			}
		    } else if (BIT_TEST(rt->rt_state, RTS_DELETE)) {
			if (BIT_TEST(bnp->bgp_options, BGPO_ANALRETENTIVE)) {	
			    trace_log_tp(tp,
					 0,
					 LOG_WARNING,
					 ("bgp_recv_v2or3_update: peer %s net %A already deleted!",
					 bnp->bgp_name,
					 &dest));
			}
		    } else {
			if (bsp) {
			    bgp_sync_rt_delete(bsp, rt);
			} else {
			    rt_delete(rt);
			}
		    }
		}
		routes_in += route_count;
	    } else {
		int hasasout, nouse;
		int noinstall = 0;
		u_int route_count = 0;

		/*
		 * See if one of our AS's is in the path.  If so this
		 * is unusable.
		 */
		nouse = hasasout = 0;
		if (bgp->bgpg_type != BGPG_INTERNAL_IGP
		  && bgp->bgpg_type != BGPG_INTERNAL_RT) {
		    if (asp->path_looped) {
			nouse = 1;
			if (AS_LOCAL_TEST(asp->path_looped, bgp->bgpg_asbit)) {
			    hasasout = 1;
			}
		    }
		}

		/*
		 * If this route has hasasout set it will never be usable
		 * unless a reconfiguration changes the loop limit (if it
		 * just has nouse set it may become usable if a
		 * reconfiguration deletes that AS).  If we haven't been
		 * told to keep these routes in the table (note that not
		 * keeping them screws up SNMP a little bit) we don't
		 * install them.  This can save beaucoup de memory.
		 */
		if (bgp->bgpg_type == BGPG_INTERNAL_IGP) {
		    nouse = 1;
		} else if (hasasout
			&& !BIT_TEST(bnp->bgp_options, BGPO_KEEPALL)) {
		    noinstall = 1;
		} else if (nouse && BIT_TEST(bnp->bgp_options, BGPO_KEEPNONE)) {
		    noinstall = 1;
		}

		/*
		 * We need to be a bit more picky here about the path
		 * attributes he sent us.  If it is an external peer,
		 * make sure the first AS in the path is the one he
		 * is operating in.  If it is external or internal
		 * make sure that the next hop in the packet is on
		 * the shared network.
		 */
		if (bgp->bgpg_type == BGPG_EXTERNAL) {
		    if (asp->path_len == 0) {
			trace_log_tp(tp,
				     0,
				     LOG_WARNING,
				     ("bgp_recv_v2or3_update: external peer %s sent zero length AS path",
				      bnp->bgp_name));
			nouse = 1;
			if (BIT_TEST(bnp->bgp_options, BGPO_KEEPNONE)) {
			    noinstall = 1;
			}
		    } else if ((as_t)(ntohs(*PATH_SHORT_PTR(asp)))
		      != bnp->bgp_peer_as
			&& !BIT_TEST(bnp->bgp_options, BGPO_IGNOREFIRSTASHOP)) {
			trace_log_tp(tp,
				     0,
				     LOG_ERR,
				     ("bgp_recv_v2or3_update: peer %s AS %u received path with first AS %u",
				      bnp->bgp_name,
				      (u_int)bnp->bgp_peer_as,
				      (u_int)(ntohs(*PATH_SHORT_PTR(asp)))));
			nouse = 1;
			}
			if (BIT_TEST(bnp->bgp_options, BGPO_KEEPNONE)) {
			    noinstall = 1;
			}
		    
		}

		/*
		 * Make sure the next hop he sent isn't one of our's.
		 * If it is we'll need to mark the route unusable.  If it
		 * isn't on a local interface but is required to be,
		 * mark the route unusable.
		 */
		if (bgp->bgpg_type == BGPG_INTERNAL
		  || bgp->bgpg_type == BGPG_EXTERNAL) {
		    if (!BIT_TEST(bnp->bgp_options, BGPO_GATEWAY)) {
		    	if (!bgp_is_on_if(bnp->bgp_ifap, &nexthop)) {
			    /*
			     * Write warning so people don't think this is
			     * right.
			     */
			    trace_log_tp(tp,
					 0,
					 LOG_WARNING,
					 ("bgp_recv_v2or3_update: peer %s next hop %A improper, ignoring routes in this update",
					  bnp->bgp_name,
					  &nexthop));
			    nouse = 1;
			} else if (sockaddrcmp_in(&nexthop,
					bnp->bgp_ifap->ifa_addr_local)) {
			    if (!nouse) {
				trace_log_tp(tp,
					     0,
					     LOG_WARNING,
					     ("bgp_recv_v2or3_update: peer %s next hop %A ours, ignoring routes in this update",
					      bnp->bgp_name,
					      &nexthop));
			    }
			    nouse = 1;
			}
		    } else {
			sock2ip(&nexthop) = sock2ip(bnp->bgp_gateway);
		    }
		} else if (bgp->bgpg_type == BGPG_INTERNAL_RT) {
		    /*
		     * Check to make sure this isn't a local address.
		     */
		    if (if_withlcladdr(&nexthop, TRUE)) {
			trace_log_tp(tp,
				     0,
				     LOG_WARNING,
				     ("bgp_recv_v2or3_update: peer %s next hop %A local, ignoring routes in this update",
				      bnp->bgp_name,
				      &nexthop));
			nouse = 1;
		    }
		/* } else if (bgp->bgpg_type == BGPG_INTERNAL_IGP) { */
		}

		if (nouse) {
		    rtparms.rtp_preference = -1;
		}

		/*
		 * Record the path in the structure
		 */
		rtparms.rtp_asp = asp;

		rtparms.rtp_metric = rtparms.rtp_metric2 = BGP_METRIC_NONE;
		if (BIT_TEST(api.api_flags, APIF_METRIC)) {
		    if (bgp->bgpg_type == BGPG_EXTERNAL) {
			rtparms.rtp_metric = api.api_metric;
		    } else {
			rtparms.rtp_metric2 = api.api_metric;
		    }
		}
		if (dopref) {
		    defpref = BGP_V3METRIC_TO_PREF(rtparms.rtp_metric2,
						   bnp->bgp_setpref,
						   bnp->bgp_preference);
		}

		/*
		 * By now we should be happy as a pig in shit with the
		 * AS path attributes.  Go dig out the networks.
		 */
		while (cp < cpend) {
		    byte netclass;
		    int nousenet = 0;

		    route_count++;
		    BGP_GET_ADDR(&dest, cp);
		    netclass = inet_class_flags(&dest);
		    rtparms.rtp_dest_mask = inet_mask_natural(&dest);
		    if (!BIT_TEST(netclass, INET_CLASSF_NETWORK|INET_CLASSF_LOOPBACK)
			|| sock2host(&dest, rtparms.rtp_dest_mask) != 0) {
			trace_log_tp(tp,
				     0,
				     LOG_ERR,
				     ("bgp_recv_v2or3_update: peer %s update included invalid net %A (%u of %u)",
				      bnp->bgp_name,
				      &dest,
				      route_count,
				      routelen/BGP_ROUTE_LENGTH));
			bgp_send_notify_none(bnp, BGP_ERR_UPDATE,
					     BGP_ERRUPD_BADNET);
			goto deadduck;
		    }

		    /*
		     * Handle special case networks.  Usually we just complain,
		     * though we should probably dump the asshole if he sent
		     * us a loopback network.
		     */
		    if (BIT_TEST(netclass, INET_CLASSF_DEFAULT|INET_CLASSF_LOOPBACK)) {
			if (BIT_TEST(netclass, INET_CLASSF_DEFAULT)) {
			    if (sock2ip(&dest) != INADDR_DEFAULT) {
				trace_log_tp(tp,
					     0,
					     LOG_ERR,
					     ("bgp_recv_v2or3_update: peer %s update included invalid default %A (%u of %u)",
					      bnp->bgp_name,
					      &dest,
					      route_count,
					      routelen/BGP_ROUTE_LENGTH));
				bgp_send_notify_none(bnp, BGP_ERR_UPDATE,
						     BGP_ERRUPD_BADNET);
				goto deadduck;
			    }
			    rtparms.rtp_dest_mask = inet_mask_default;
		        } else {
			    trace_log_tp(tp,
					 0,
					 LOG_WARNING,
					 ("bgp_recv_v2or3_update: ignoring loopback route from peer %s (%u of %u)",
					  bnp->bgp_name,
					  route_count,
					  routelen/BGP_ROUTE_LENGTH));
			    nousenet++;
			    rtparms.rtp_preference = -1;
			}
		    }

		    /*
		     * Find any existing route to the destination from
		     * this guy.
		     */
		    rth = rt_table_locate(&dest, rtparms.rtp_dest_mask);
		    if (!rth) {
			rt = (rt_entry *) 0;
		    } else {
			RT_ALLRT(rt, rth) {
			    if (BIT_TEST(rt->rt_state, RTS_DELETE)) {
				rt = (rt_entry *) 0;
				break;
			    }
			    if (rt->rt_gwp == &(bnp->bgp_gw)) {
				break;
			    }
			} RT_ALLRT_END(rt, rth);
		    }

		    if (noinstall || (nousenet
		      && BIT_TEST(bnp->bgp_options, BGPO_KEEPNONE))) {
			/*
			 * Just delete any existing route.
			 */
			if (rt) {
			    if (bsp) {
				bgp_sync_rt_delete(bsp, rt);
			    } else {
				rt_delete(rt);
			    }
			}
			continue;
		    }

		    if (rt) {
			int is_same = 0;

			if (rtparms.rtp_metric == rt->rt_metric
			  && rtparms.rtp_metric2 == rt->rt_metric2
			  && asp == rt->rt_aspath) {
			    if (bgp->bgpg_type == BGPG_INTERNAL_RT) {
				if (BSY_NEXTHOP((bsy_ibgp_rt *)(rt->rt_data))
				  == sock2ip(&nexthop)) {
				    is_same = 1;
				}
			    } else {
				if (sockaddrcmp_in(RT_ROUTER(rt), &nexthop)) {
				    is_same = 1;
				}
			    }
			}

			if (is_same) {
			    if (BIT_TEST(bnp->bgp_options, BGPO_ANALRETENTIVE)) {
				trace_log_tp(tp,
					     0,
					     LOG_WARNING,
					     ("bgp_recv_v2or3_update: peer %s sent %A unchanged!",
					      bnp->bgp_name,
					      rt->rt_dest));
			    }
			    continue;
			}
		    }

		    /*
		     * If this is an unuseable route, or if this is an
		     * internal peer associated with an IGP, we may not
		     * need to call the policy routines, otherwise we
		     * will.
		     */
		    if (!nouse && !nousenet) {
			rtparms.rtp_preference = defpref;
		        nousenet = !import(&dest,
					   rtparms.rtp_dest_mask,
					   bnp->bgp_import,
					   bgp_import_aspath_list,
					   (adv_entry *)0,
					   &rtparms.rtp_preference,
					   bnp->bgp_ifap,
					   (void_t) asp);
		    }

		    if (nousenet && BIT_TEST(bnp->bgp_options, BGPO_KEEPNONE)) {
			/*
			 * Just delete any existing route.
			 */
			if (rt) {
			    if (bsp) {
				bgp_sync_rt_delete(bsp, rt);
			    } else {
				rt_delete(rt);
			    }
			}
			continue;
		    }


		    /*
		     * If we've got a route already and it isn't deleted,
		     * change it to match this.  Otherwise, add it.
		     */
		    if (rt) {
			if (bsp) {
			    (void) bgp_sync_rt_change(bsp,
						      bnp,
						      rt,
						      rtparms.rtp_metric,
						      rtparms.rtp_metric2,
						      rtparms.rtp_tag,
						      rtparms.rtp_preference,
						      rtparms.rtp_preference2,
						      1, &api.api_nexthop,
						      asp);
			} else {
			    (void) rt_change_aspath(rt,
						    rtparms.rtp_metric,
						    rtparms.rtp_metric2,
						    rtparms.rtp_tag,
						    rtparms.rtp_preference,
						    rtparms.rtp_preference2,
						    1, &api.api_nexthop,
						    asp);
			    rt_refresh(rt);
			}
		    } else {
			if (bsp) {
			    (void) bgp_sync_rt_add(bsp, bnp, rth, &rtparms);
			} else {
			    (void) rt_add(&rtparms);
			}
		    }
		}	/* end of while (cp < cpend) */
		routes_in += route_count;
	    }		/* end of if (unreach) */
	    ASPATH_FREE(asp);
	    asp = NULL;
	}		/* end of for (;;) for messages in buffer */

	/*
	 * Delete the messages we just processed from the buffer, then
	 * go around for more if there is likely to be any and we haven't
	 * read our fill.
	 */
	bnp->bgp_bufpos = cp;
	if (!maybemore || bnp->bgp_in_octets >= max_octets) {
	    break;
	}
    }			/* end of for (;;) read more stuff */
    rt_close(bnp->bgp_task, &bnp->bgp_gw, 0, NULL);

    trace_tp(bnp->bgp_task,
	     TR_NORMAL,
	     0,
	     ("bgp_recv_v2or3_update: %s %s received %u octet%s %u update%s %u route%s",
	      (maybemore ? "counted out" : "done with"),
	      bnp->bgp_name,
	      (bnp->bgp_in_octets - start_octets),
	      (((bnp->bgp_in_octets - start_octets) == 1) ? "" : "s"),
	      (bnp->bgp_in_updates - start_updates),
	      (((bnp->bgp_in_updates - start_updates) == 1) ? "" : "s"),
	      routes_in,
	      ((routes_in == 1) ? "" : "s")));
 
    return;

    /*
     * We come here for errors which have screwed us up beyond repair.
     * Delete any AS path we have and close us down.
     */
deadduck:
    if (asp != NULL) {
	ASPATH_FREE(asp);
    }
    rt_close(bnp->bgp_task, &bnp->bgp_gw, 0, NULL);
    bgp_peer_close(bnp, event);
}



/*
 * bgp_recv_v4_update - receive an update from a peer.  This is actually
 *		     the task_read routine when we are in established state
 */
static void
bgp_recv_v4_update(tp)
task *tp;
{
    register byte *cp, *cpend;
    register bgpPeer *bnp;
    register rt_entry *rt;
    rt_head *rth;
    int bitlen;
    as_path *asp = NULL;
    bgpPeerGroup *bgp;
    as_path_info api;
    rt_parms rtparms;
    sockaddr_un nexthop;
    sockaddr_un dest;
    sockaddr_un *mask;
    size_t len;
    int type;
    byte *attrp;
    size_t attrlen;
    int routelen;
    int unreachlen;
    int error_code;
    byte *error_data;
    int count, maybemore = 1;
    int dopref = 0;
    pref_t defpref;
    u_long max_octets;
    u_long start_octets;
    u_long start_updates;
    u_int routes_in = 0;
    bgp_sync *bsp = (bgp_sync *) 0;
    int event = BGPEVENT_RECVUPDATE;

    /*
     * Trace this for posterity
     */
    bnp = (bgpPeer *)tp->task_data;
    bgp = bnp->bgp_group;
    trace_tp(bnp->bgp_task,
	     TR_NORMAL,
	     0,
	     ("bgp_recv_v4_update: receiving updates from %s",
	      bnp->bgp_name));

    /*
     * If this is an internal routing guy, fetch his structure
     */
    if (bgp->bgpg_type == BGPG_INTERNAL_RT) {
	bsp = bgp->bgpg_sync;
	assert(bsp);
    }

    /*
     * Fetch the default preference.  If this is an internal group and
     * the setpref option was used we'll need to modify this.
     */
    defpref = bnp->bgp_preference;
    if (BGP_GROUP_INTERNAL(bgp) && BIT_TEST(bnp->bgp_options, BGPO_SETPREF)) {
	dopref = 1;
    }

    /*
     * Go around forever trying to read the socket.  We break when we can't
     * get enough to make a full packet.
     */
    bzero((caddr_t) &rtparms, sizeof(rtparms));
    rtparms.rtp_dest = &dest;
    rtparms.rtp_n_gw = 1;
    rtparms.rtp_router = &nexthop;
    rtparms.rtp_gwp = &bnp->bgp_gw;
    rtparms.rtp_preference2 = bnp->bgp_preference2;
    if (bgp->bgpg_type == BGPG_INTERNAL || bgp->bgpg_type == BGPG_INTERNAL_RT) {
	rtparms.rtp_state = RTS_INTERIOR|RTS_EXTERIOR;
    } else if (bgp->bgpg_type == BGPG_INTERNAL_IGP) {
	rtparms.rtp_state = RTS_INTERIOR|RTS_EXTERIOR|RTS_NOTINSTALL|RTS_NOADVISE;
    } else {
	rtparms.rtp_state = RTS_EXTERIOR;
    }
    rtparms.rtp_rtd = NULL;
    if (TRACE_DETAIL_RECV_TP(tp, BGP_UPDATE, BGP_PACKET_MAX, bgp_trace_masks)) {
	max_octets = bnp->bgp_in_octets + BGPMAXREADTRACEPKTS;
    } else {
	max_octets = bnp->bgp_in_octets + BGPMAXREAD;
    }
    start_octets = bnp->bgp_in_octets;
    start_updates = bnp->bgp_in_updates;
    sockclear_in(&dest);
    rt_open(tp);

    for (;;) {
	/*
	 * Try to read some.  We keep track of whether we got a full
	 * buffer or not so we'll know whether there is a good chance
	 * of getting some next time around.
	 */
	count = bgp_recv(tp, &bnp->bgp_inbuf, 0, bnp->bgp_name);
	if (count < 0) {
	    event = BGPEVENT_CLOSED;
	    goto deadduck;
	}

	cp = bnp->bgp_bufpos;
	if (BGPBUF_SPACE(bnp) > 0) {
	    maybemore = 0;
	}

	/*
	 * Now loop reading messages out of this buffer.  We do this until
	 * we haven't got enough data for a full message.
	 */
	for (;;) {
	    /*
	     * See if we've got a full header now.  If not break
	     * out.  If so, decode it.
	     */
	    if (BGPBUF_LEFT(bnp, cp) < BGP_HEADER_LEN) {
		break;
	    }

	    BGP_GET_HDRLEN(len, cp);

	    /*
	     * Check for a strange length.
	     */
	    if (len < BGP_HEADER_LEN || len > BGPMAXPACKETSIZE) {
		trace_log_tp(tp,
			     0,
			     LOG_WARNING,
			     ("bgp_recv_v4_update: peer %s: strange message header length %d",
			      bnp->bgp_name,
			      len));
	    	bgp_send_notify_word(bnp,
				     BGP_ERR_HEADER,
				     BGP_ERRHDR_LENGTH,
				     (int) len);
		event = BGPEVENT_ERROR;	/* not quite right */
		goto deadduck;
	    }

	    /*
	     * See if we've got the full message buffered.  If not,
	     * break out of this.
	     */
	    if (BGPBUF_LEFT(bnp, cp) < len) {
		break;
	    }

	    /*
	     * If we are tracing, do it now.  This is as uneditted as
	     * the packet gets.
	     */
	    if (TRACE_PACKET_RECV_TP(tp,
				     BGP_UPDATE,
				     BGP_PACKET_MAX,
				     bgp_trace_masks)) {
		bgp_trace(bnp,
			  (bgpProtoPeer *) NULL,
			  "BGP RECV ",
			  0,
			  cp,
			  TRACE_DETAIL_RECV_TP(tp,
					       BGP_UPDATE,
					       BGP_PACKET_MAX,
					       bgp_trace_masks));
	    }

	    /*
	     * Check for an in-range type.  If not complain about this.
	     */
	    BGP_GET_HDRTYPE(type, cp);
	    if (type == 0 || type > BGP_KEEPALIVE) {
	        trace_log_tp(tp,
			     0,
			     LOG_WARNING,
			     ("bgp_recv_v4_update: peer %s unrecognized message type %d",
			      bnp->bgp_name,
			      type));
		bgp_send_notify_byte(bnp, BGP_ERR_HEADER,
				     BGP_ERRHDR_TYPE, type);
		event = BGPEVENT_ERROR;	/* not quite right */
		goto deadduck;
	    }

	    /*
	     * Record that we received something
	     */
	    bnp->bgp_last_rcvd = time_sec;

	    /*
	     * Check message authentication.  If failed we're dead.
	     */
	    if (!BGP_CHECK_AUTH(&(bnp->bgp_authinfo), bnp->bgp_task, cp, len)) {
		bgp_send_notify_none(bnp, BGP_ERR_HEADER, BGP_ERRHDR_UNSYNC);
		event = BGPEVENT_CLOSED;	/* not quite right */
		goto deadduck;
	    }

	    /*
	     * Check the type.  This sets the event and filters out
	     * everything except updates.  For keepalives we go
	     * around again.
	     */
	    bnp->bgp_in_octets += (u_long) len;
	    switch(type) {
	    case BGP_OPEN:
		trace_log_tp(tp,
			     0,
			     LOG_ERR,
			     ("bgp_recv_v4_update: received OPEN message from %s, state is ESTABLISHED",
			      bnp->bgp_name));
		bnp->bgp_in_notupdates++;
		bgp_send_notify_none(bnp, BGP_ERR_FSM, 0);
		event = BGPEVENT_RECVOPEN;
		goto deadduck;

	    case BGP_KEEPALIVE:
		bnp->bgp_in_notupdates++;
		if (len != BGP_HEADER_LEN) {
		    bgp_send_notify_word(bnp,
					 BGP_ERR_HEADER,
					 BGP_ERRHDR_LENGTH,
					 (int) len);
		    event = BGPEVENT_RECVKEEPALIVE;
		    goto deadduck;
		}
		bnp->bgp_lastevent = BGPEVENT_RECVKEEPALIVE;
		break;

	    case BGP_NOTIFY:
		bnp->bgp_in_notupdates++;
		bgp_log_notify(bnp->bgp_trace_options,
			       bnp->bgp_name,
			       cp,
			       len,
			       0);
		event = BGPEVENT_RECVNOTIFY;
		goto deadduck;

	    case BGP_UPDATE:
	        bnp->bgp_in_updates++;
		break;
	    }

	    cpend = cp + len;
	    BGP_SKIP_HEADER(cp);
	    if (type == BGP_KEEPALIVE) {
		bnp->bgp_bufpos = cp;
		continue;
	    }

	    /*
	     * Here we have an update message.  Make sure we have enough
	     * length for the attributes, then fetch the attributes and
	     * their length.
	     */
	    if ((cpend - cp) < BGP_V4UPDATE_MIN_LEN) {
		bgp_send_notify_word(bnp,
				     BGP_ERR_HEADER,
				     BGP_ERRHDR_LENGTH,
				     (int) len);
		trace_log_tp(tp,
			     0,
			     LOG_WARNING,
			     ("bgp_recv_v4_update: peer %s UPDATE length %d too small",
			      bnp->bgp_name,
			      len));
		goto deadduck;
	    }

	    /*
	     * See if we have an unreachable count.  If so, make sure it
	     * makes sense.
	     *
	     * XXX there is no error code for this
	     */
	    BGP_GET_V4UPDATE_UNREACH(unreachlen, cp);
	    if (((cpend - cp) - unreachlen) < BGP_ATTR_SIZE_LEN) {
		trace_log_tp(tp,
			     0,
			     LOG_WARNING,
			     ("bgp_recv_v4_update: peer %s UPDATE unreachable prefix length %d exceeds packet length %d",
			      bnp->bgp_name,
			      unreachlen,
			      (cpend - cp)));
		bgp_send_notify_none(bnp, BGP_ERR_UPDATE, BGP_ERRUPD_ATTRLIST);
		goto deadduck;
	    }
	    /*
	     * Process any unreachables, unless we're running as a test
	     * peer in which case this is too much work.
	     */
	    if (bgp->bgpg_type == BGPG_TEST) {
		cp += unreachlen;
	    } else if (unreachlen > 0) {
		byte netclass;
		byte *cpstart = cp;
		byte *cpendu = cp + unreachlen;

		while (cp < cpendu) {
		    BGP_GET_BITCOUNT(bitlen, cp);
		    if (!BGP_OKAY_BITCOUNT(bitlen)) {
			trace_log_tp(tp,
				     0,
				     LOG_ERR,
				     ("bgp_recv_v4_update: peer %s UPDATE unreachable prefix length %d too long",
				      bnp->bgp_name,
				      bitlen));
			bgp_send_notify_none(bnp,
					     BGP_ERR_UPDATE,
					     BGP_ERRUPD_BADNET);
			goto deadduck;
		    }
		    if (BGP_PREFIX_LEN(bitlen) > (cpendu - cp)) {
			trace_log_tp(tp,
				     0,
				     LOG_ERR,
				     ("bgp_recv_v4_update: peer %s UPDATE prefix length %d exceeds unreachable prefix data remaining (%d bytes)",
				      bnp->bgp_name,
				      bitlen,
				      (cpendu - cp)));
			bgp_send_notify_none(bnp,
					     BGP_ERR_UPDATE,
					     BGP_ERRUPD_BADNET);
			goto deadduck;
		    }
		    BGP_GET_PREFIX(bitlen, &dest, cp);
		    routes_in++;
		    netclass = inet_class_flags(&dest);
		    mask = inet_mask_prefix(bitlen);
		    if (!BIT_TEST(netclass, INET_CLASSF_NETWORK)
			|| sock2host(&dest, mask) != 0) {
			trace_log_tp(tp,
				     0,
				     LOG_ERR,
				     ("bgp_recv_v4_update: peer %s UPDATE ignoring invalid unreachable route %A/%A (%d of %d)",
				      bnp->bgp_name,
				      &dest,
				      mask,
				      (cpendu - cp),
				      (cpstart - cp)));
			continue;
		    }

		    /*
		     * Try to find existing net from this guy.  If it doesn't
		     * exist, complain if we think we have them all.
		     */
		    rth = rt_table_locate(&dest, mask);
		    if (!rth) {
			rt = (rt_entry *) 0;
		    } else {
			RT_ALLRT(rt, rth) {
			    if (rt->rt_gwp == &(bnp->bgp_gw)) {
				break;
			    }
			} RT_ALLRT_END(rt, rth);
		    }

		    if (!rt) {
			if (BIT_MATCH(bnp->bgp_options,
				      BGPO_KEEPALL|BGPO_ANALRETENTIVE)) {
			    trace_log_tp(tp,
					 0,
					 LOG_WARNING,
					 ("bgp_recv_v4_update: peer %s unreachable route %A/%A not sent by him (%d of %d)",
					  bnp->bgp_name,
					  &dest,
					  mask,
					  (cpendu - cp),
					  (cpstart - cp)));
			}
		    } else if (BIT_TEST(rt->rt_state, RTS_DELETE)) {
			if (BIT_TEST(bnp->bgp_options, BGPO_ANALRETENTIVE)) {
			    trace_log_tp(tp,
					 0,
					 LOG_WARNING,
					 ("bgp_recv_v4_update: peer %s route %A/%A already deleted!",
					 bnp->bgp_name,
					 &dest,
					 mask));
			}
		    } else {
			if (bsp) {
			    bgp_sync_rt_delete(bsp, rt);
			} else {
			    rt_delete(rt);
			}
		    }
		}
	    }

	    /*
	     * Fetch the attribute length and a pointer to the attributes.
	     * If this length was screwed up, or what it leaves us isn't
	     * divisible by 4, we're dead.
	     */
	    BGP_GET_UPDATE(attrlen, attrp, cp);
	    if (attrlen == 0) {
		if (cp != cpend) {
		    trace_log_tp(tp,
				 0,
				 LOG_WARNING,
				 ("bgp_recv_v4_update: peer %s UPDATE zero attribute length followed by %d bytes of garbage",
				  bnp->bgp_name,
				  (cpend - cp)));
		    cp = cpend;
		    bnp->bgp_bufpos = cp;
		}
		continue;
	    }

	    routelen = cpend - cp;
	    if (routelen < 0) {
		trace_log_tp(tp,
			     0,
			     LOG_WARNING,
			     ("bgp_recv_v4_update: peer %s UPDATE path attribute length %d too large (%d bytes remaining)",
			      bnp->bgp_name,
			      attrlen,
			      (attrlen + routelen)));
		bgp_send_notify_none(bnp, BGP_ERR_UPDATE, BGP_ERRUPD_ATTRLIST);
		goto deadduck;
	    }

	    /*
	     * From here on the going gets a little expensive.  For TEST
	     * peers, don't bother with the rest of the message.
	     */
	    if (bgp->bgpg_type == BGPG_TEST) {
		cp = cpend;
	        bnp->bgp_bufpos = cp;
		continue;
	    }

	    /*
	     * Now fetch the AS path.  If this detects an error, barf.
	     */
	    api.api_nexthop = &nexthop;
	    asp = aspath_attr(attrp,
			      attrlen,
			      PATH_VERSION_4,
			      bnp->bgp_local_as,
			      &api,
			      &error_code,
			      &error_data);
	    if (asp == NULL) {
		bgp_path_attr_error(bnp, error_code, error_data,
		  "bgp_recv_v4_update");
		goto deadduck;
	    }

	    /*
	     * If the next hop wasn't included, complain about this.
	     */
	    if (!BIT_TEST(api.api_flags, APIF_NEXTHOP)) {
		trace_log_tp(tp,
			     0,
			     LOG_ERR,
			     ("bgp_recv_v4_update: peer %s UPDATE no next hop found",
			      bnp->bgp_name));
		bgp_send_notify_byte(bnp, BGP_ERR_UPDATE,
				     BGP_ERRUPD_MISSING, PA4_TYPE_NEXTHOP);
		goto deadduck;
	    }

	    rtparms.rtp_metric = rtparms.rtp_metric2 = BGP_METRIC_NONE;

	    if (BGP_GROUP_EXTERNAL(bgp)) {
		if (BIT_TEST(api.api_flags, APIF_LOCALPREF)) {
		    trace_log_tp(tp,
				 0,
				 LOG_WARNING,
				 ("bgp_recv_v4_update: external peer %s UPDATE included LOCALPREF attribute",
				  bnp->bgp_name));
		}
	    } else {
		if (!BIT_TEST(api.api_flags, APIF_LOCALPREF)) {
		    trace_log_tp(tp,
				 0,
				 LOG_ERR,
				 ("bgp_recv_v4_update: peer %s UPDATE no localpref attribute found",
				  bnp->bgp_name));
		    bgp_send_notify_byte(bnp,
					 BGP_ERR_UPDATE,
					 BGP_ERRUPD_MISSING,
					 PA4_TYPE_LOCALPREF);
		    goto deadduck;
		}
		rtparms.rtp_metric2 = BGP_LOCALPREF_TO_GATED(api.api_localpref);
		if (dopref) {
		    defpref = BGP_LOCALPREF_TO_PREF(rtparms.rtp_metric2,
						    bnp->bgp_setpref,
						    bnp->bgp_preference);
		}
	    }

	    if (BIT_TEST(api.api_flags, APIF_METRIC)) {
		rtparms.rtp_metric = BGP_V4METRIC_TO_GATED(api.api_metric);
	    }

	    /*
	     * Process unreachables separately since we aren't overly
	     * fussy about path attributes in this case and we can gain
	     * a bit of speed processing these.
	     */
	    if (routelen == 0) {
		trace_log_tp(tp,
			     0,
			     LOG_WARNING,
			     ("bgp_recv_v4_update: peer %s UPDATE has path attributes but no reachable prefixes!",
			      bnp->bgp_name));
	    } else {
#ifdef	notdef
		register int i;
		as_entry *ase;
#endif	/* notdef */
		int hasasout, nouse;
		int noinstall = 0;

		/*
		 * See if one of our AS's is in the path.  If so this
		 * is unusable.
		 */
		nouse = hasasout = 0;
		if (bgp->bgpg_type != BGPG_INTERNAL_IGP
		  && bgp->bgpg_type != BGPG_INTERNAL_RT) {
		    if (asp->path_looped) {
			nouse = 1;
			if (AS_LOCAL_TEST(asp->path_looped, bgp->bgpg_asbit)) {
			    hasasout = 1;
			}
		    }
		}

		/*
		 * If this route has hasasout set it will never be usable
		 * unless a reconfiguration changes the loop limit (if it
		 * just has nouse set it may become usable if a
		 * reconfiguration deletes that AS).  If we haven't been
		 * told to keep these routes in the table (note that not
		 * keeping them screws up SNMP a little bit) we don't
		 * install them.  This can save beaucoup de memory.
		 */
		if (bgp->bgpg_type == BGPG_INTERNAL_IGP) {
		    nouse = 1;
		} else if (hasasout
			&& !BIT_TEST(bnp->bgp_options, BGPO_KEEPALL)) {
		    noinstall = 1;
		} else if (nouse && BIT_TEST(bnp->bgp_options, BGPO_KEEPNONE)) {
		    noinstall = 1;
		}

		/*
		 * We need to be a bit more picky here about the path
		 * attributes he sent us.  If it is an external peer,
		 * make sure the first AS in the path is the one he
		 * is operating in.  If it is external or internal
		 * make sure that the next hop in the packet is on
		 * the shared network.
		 */
		if (bgp->bgpg_type == BGPG_EXTERNAL) {
		    if (asp->path_len == 0
		      || ((as_t)(ntohs(*PATH_SHORT_PTR(asp)))
			!= bnp->bgp_peer_as) 
			&& !BIT_TEST(bnp->bgp_options, BGPO_IGNOREFIRSTASHOP)) {
			trace_log_tp(tp,
				     0,
				     LOG_ERR,
				     ("bgp_recv_v4_update: peer %s AS %u received path with first AS %u",
				      bnp->bgp_name,
				      (u_int)bnp->bgp_peer_as,
				      (u_int)(ntohs(*PATH_SHORT_PTR(asp)))));
			nouse = 1;
			}
			if (BIT_TEST(bnp->bgp_options, BGPO_KEEPNONE)) {
			    noinstall = 1;
			}
		   
                }	

		/*
		 * Make sure the next hop he sent isn't one of our's.
		 * If it is we'll need to mark the route unusable.  If it
		 * isn't on a local interface but is required to be,
		 * mark the route unusable.
		 */
		if (bgp->bgpg_type == BGPG_INTERNAL
		  || bgp->bgpg_type == BGPG_EXTERNAL) {
		    if (!BIT_TEST(bnp->bgp_options, BGPO_GATEWAY)) {
		    	if (!bgp_is_on_if(bnp->bgp_ifap, &nexthop)) {
			    /*
			     * Write warning so people don't think this is
			     * right.
			     */
			    trace_log_tp(tp,
					 0,
					 LOG_WARNING,
					 ("bgp_recv_v4_update: peer %s next hop %A improper, ignoring routes in this update",
					  bnp->bgp_name,
					  &nexthop));
			    nouse = noinstall = 1;
			} else if (sockaddrcmp_in(&nexthop,
					bnp->bgp_ifap->ifa_addr_local)) {
			    if (!nouse &&
			      BIT_TEST(bnp->bgp_options, BGPO_ANALRETENTIVE)) {
				trace_log_tp(tp,
					     0,
					     LOG_WARNING,
					     ("bgp_recv_v4_update: peer %s next hop %A ours, ignoring routes in this update",
					      bnp->bgp_name,
					      &nexthop));
			    }
			    nouse = 1;
			}
		    } else {
			sock2ip(&nexthop) = sock2ip(bnp->bgp_gateway);
		    }
		} else if (bgp->bgpg_type == BGPG_INTERNAL_RT) {
		    /*
		     * Check to make sure this isn't a local address.
		     */
		    if (if_withlcladdr(&nexthop, TRUE)) {
			trace_log_tp(tp,
				     0,
				     LOG_WARNING,
				     ("bgp_recv_v4_update: peer %s next hop %A local, ignoring routes",
				      bnp->bgp_name,
				      &nexthop));
			nouse = 1;
		    }
		/* } else if (bgp->bgpg_type == BGPG_INTERNAL_IGP) { */
		}

		if (nouse) {
		    rtparms.rtp_preference = -1;
		}

		/*
		 * Record the path in the structure
		 */
		rtparms.rtp_asp = asp;

		/*
		 * By now we should be happy as a pig in shit with the
		 * AS path attributes.  Go dig out the networks.
		 */
		while (cp < cpend) {
		    byte netclass;
		    int nousenet = 0;
		    byte *cpstart = cp;

		    BGP_GET_BITCOUNT(bitlen, cp);
		    if (!BGP_OKAY_BITCOUNT(bitlen)) {
			trace_log_tp(tp,
				     0,
				     LOG_ERR,
				     ("bgp_recv_v4_update: peer %s UPDATE prefix length %d too long",
				      bnp->bgp_name,
				      bitlen));
			bgp_send_notify_none(bnp,
					     BGP_ERR_UPDATE,
					     BGP_ERRUPD_BADNET);
			goto deadduck;
		    }
		    if (BGP_PREFIX_LEN(bitlen) > (cpend - cp)) {
			trace_log_tp(tp,
				     0,
				     LOG_ERR,
				     ("bgp_recv_v4_update: peer %s UPDATE prefix length %d exceeds prefix data remaining (%d bytes)",
				      bnp->bgp_name,
				      bitlen,
				      (cpend - cp)));
			bgp_send_notify_none(bnp,
					     BGP_ERR_UPDATE,
					     BGP_ERRUPD_BADNET);
			goto deadduck;
		    }
		    BGP_GET_PREFIX(bitlen, &dest, cp);
		    routes_in++;
		    netclass = inet_class_flags(&dest);
		    rtparms.rtp_dest_mask = inet_mask_prefix(bitlen);
		    if (!BIT_TEST(netclass, INET_CLASSF_NETWORK|INET_CLASSF_LOOPBACK)
			|| sock2host(&dest, rtparms.rtp_dest_mask) != 0) {
			trace_log_tp(tp,
				     0,
				     LOG_ERR,
				     ("bgp_recv_v4_update: peer %s update included invalid route %A/%A (%d of %d)",
				      bnp->bgp_name,
				      &dest,
				      rtparms.rtp_dest_mask,
				      (cpend - cp),
				      (cpend - cpstart)));
			bgp_send_notify_none(bnp, BGP_ERR_UPDATE,
					     BGP_ERRUPD_BADNET);
			goto deadduck;
		    }

		    /*
		     * Handle special case networks.  Usually we just complain,
		     * though we should probably dump the asshole if he sent
		     * us a loopback network.
		     */
		    if (BIT_TEST(netclass, INET_CLASSF_DEFAULT|INET_CLASSF_LOOPBACK)) {
			if (BIT_TEST(netclass, INET_CLASSF_DEFAULT)) {
			    if (inet_prefix_mask(rtparms.rtp_dest_mask) >= 8) {
				trace_log_tp(tp,
					     0,
					     LOG_WARNING,
					     ("bgp_recv_v4_update: ignoring network 0 route %A/%A from peer %s (%d of %d)",
					      rtparms.rtp_dest,
					      rtparms.rtp_dest_mask,
					      bnp->bgp_name,
					      (cpend - cp),
					      (cpend - cpstart)));
				nousenet++;
				rtparms.rtp_preference = -1;
			    }
		        } else {
			    trace_log_tp(tp,
					 0,
					 LOG_WARNING,
					 ("bgp_recv_v4_update: ignoring loopback route from peer %s (%d of %d)",
					  bnp->bgp_name,
					  (cpend - cp),
					  (cpend - cpstart)));
			    nousenet++;
			    rtparms.rtp_preference = -1;
			}
		    }

		    /*
		     * Find any existing route from this guy.  Since our
		     * preference depends only on the route and the AS path
		     * we can avoid calling the policy routine if the AS
		     * path is the same.  This way we can short circuit the
		     * rest.
		     */
		    rth = rt_table_locate(&dest, rtparms.rtp_dest_mask);
		    if (!rth) {
			rt = (rt_entry *) 0;
		    } else {
			RT_ALLRT(rt, rth) {
			    if (BIT_TEST(rt->rt_state, RTS_DELETE)) {
				rt = (rt_entry *) 0;
				break;
			    }
			    if (rt->rt_gwp == &(bnp->bgp_gw)) {
				break;
			    }
			} RT_ALLRT_END(rt, rth);
		    }

		    if (noinstall) {
			/*
			 * Just delete any existing route.
			 */
			if (rt) {
			    if (bsp) {
				bgp_sync_rt_delete(bsp, rt);
			    } else {
				rt_delete(rt);
			    }
			}
			continue;
		    }

		    /*
		     * If this is an unuseable route, or if this is an
		     * internal peer associated with an IGP, we may not
		     * need to call the policy routines, otherwise we
		     * will.
		     */
		    if (!nouse && !nousenet) {
			rtparms.rtp_preference = defpref;
		        nousenet = !import(&dest,
					   rtparms.rtp_dest_mask,
					   bnp->bgp_import,
					   bgp_import_aspath_list,
					   (adv_entry *)0,
					   &rtparms.rtp_preference,
					   bnp->bgp_ifap,
				 	   (void_t) asp);
		    }

		    if (nousenet && BIT_TEST(bnp->bgp_options, BGPO_KEEPNONE)) {
			if (rt) {
			    if (bsp) {
				bgp_sync_rt_delete(bsp, rt);
			    } else {
				rt_delete(rt);
			    }
			}
			continue;
		    }

		    /*
		     * If we've got a route already and it isn't deleted,
		     * change it to match this.  Otherwise, add it.
		     */
		    if (rt) {
			if (bsp) {
			    (void) bgp_sync_rt_change(bsp,
						      bnp,
						      rt,
						      rtparms.rtp_metric,
						      rtparms.rtp_metric2,
						      rtparms.rtp_tag,
						      rtparms.rtp_preference,
						      rtparms.rtp_preference2,
						      1, &api.api_nexthop,
						      asp);
			} else {
			    (void) rt_change_aspath(rt,
						    rtparms.rtp_metric,
						    rtparms.rtp_metric2,
						    rtparms.rtp_tag,
						    rtparms.rtp_preference,
						    rtparms.rtp_preference2,
						    1, &api.api_nexthop,
						    asp);
			    rt_refresh(rt);
			}
		    } else {
			if (bsp) {
			    (void) bgp_sync_rt_add(bsp, bnp, rth, &rtparms);
			} else {
			    (void) rt_add(&rtparms);
			}
		    }
		}	/* end of while (cp < cpend) */
	    }		/* end of if (unreach) */
	    ASPATH_FREE(asp);
	    asp = NULL;
	}		/* end of for (;;) for messages in buffer */

	/*
	 * Delete the messages we just processed from the buffer, then
	 * go around for more if there is likely to be any and we haven't
	 * read our fill.
	 */
	bnp->bgp_bufpos = cp;
	if (!maybemore || bnp->bgp_in_octets >= max_octets) {
	    break;
	}
    }			/* end of for (;;) read more stuff */
    rt_close(bnp->bgp_task, &bnp->bgp_gw, 0, NULL);
    trace_tp(bnp->bgp_task,
	     TR_NORMAL,
	     0,
	     ("bgp_recv_v4_update: %s %s received %u octet%s %u update%s %u route%s",
	      (maybemore ? "counted out" : "done with"),
	      bnp->bgp_name,
	      (bnp->bgp_in_octets - start_octets),
	      (((bnp->bgp_in_octets - start_octets) == 1) ? "" : "s"),
	      (bnp->bgp_in_updates - start_updates),
	      (((bnp->bgp_in_updates - start_updates) == 1) ? "" : "s"),
	      routes_in,
	      ((routes_in == 1) ? "" : "s")));
    return;

    /*
     * We come here for errors which have screwed us up beyond repair.
     * Delete any AS path we have and close us down.
     */
deadduck:
    if (asp != NULL) {
	ASPATH_FREE(asp);
    }
    rt_close(bnp->bgp_task, &bnp->bgp_gw, 0, NULL);
    bgp_peer_close(bnp, event);
}





/*
 * bgp_rt_reinit - redo all policy after a reinit.  The things which can
 *     change which effect us are the AS list, the KEEPALL flag being
 *     turned off, and the policy changing.
 */
static void
bgp_rt_reinit __PF1(tp, task *)
{
    bgpPeer *bnp = (bgpPeer *)(tp->task_data);
    register rt_entry *rt;
    register rtq_entry *rtq = &(bnp->bgp_gw.gw_rtq);
    register int changes = 0;
    register bgp_sync *bsp = (bgp_sync *) 0;
    as_path *asp;
    pref_t preference;
    int preftype = 0;
    int keepall = BIT_TEST(bnp->bgp_options, BGPO_KEEPALL);
    int keepnone = BIT_TEST(bnp->bgp_options, BGPO_KEEPNONE);
    int isexternal = (bnp->bgp_group->bgpg_type == BGPG_EXTERNAL);
    int checkloop = (bnp->bgp_group->bgpg_type != BGPG_INTERNAL_RT);
    int res;
    int delete_it = 0;

    if (bnp->bgp_group->bgpg_type == BGPG_INTERNAL_RT) {
	bsp = bnp->bgp_group->bgpg_sync;
	assert(bsp);
    }
    if (BGP_GROUP_INTERNAL(bnp->bgp_group)
      && BIT_TEST(bnp->bgp_options, BGPO_SETPREF)) {
	if (bnp->bgp_version == BGP_VERSION_4) {
	    preftype = 1;
	} else {
	    preftype = 2;
	}
    }

    /* XXX needs work! */
    if (bnp->bgp_gw.gw_n_routes) {

	rt_open(bnp->bgp_task);

	RTQ_LIST(rtq, rt) {
	    res = 0;
	    asp = rt->rt_aspath;
	    if (isexternal && (asp->path_len == 0
	      || ((as_t)(ntohs(*PATH_SHORT_PTR(asp))) != bnp->bgp_peer_as 
	      && !BIT_TEST(bnp->bgp_options, BGPO_IGNOREFIRSTASHOP)))) {
		/* dead meat */
	    } else if (checkloop && asp->path_looped) {
		if (!keepall && AS_LOCAL_TEST(asp->path_looped,
					      bnp->bgp_group->bgpg_asbit)) {
		    delete_it = 1;
		}
	    } else if (!bsp && (rt->rt_n_gw == 0 ||
	      sock2ip(RT_ROUTER(rt)) == sock2ip(inet_addr_loopback))) {
		/* dead meat */
	    } else if (checkloop && rt->rt_preference == (-1)
	      && (!bgp_is_on_if(bnp->bgp_ifap, RT_ROUTER(rt))
		|| sockaddrcmp_in(bnp->bgp_ifap->ifa_addr_local,
				  RT_ROUTER(rt)))) {
		/* dead meat */
	    } else if (sock2ip(rt->rt_dest) == INADDR_DEFAULT
	      && inet_prefix_mask(rt->rt_dest_mask) >= 8) {
		/* dead meat */
	    } else {
		switch(preftype) {
		case 0:
		    preference = bnp->bgp_preference;
		    break;
		case 1:
		    preference = BGP_LOCALPREF_TO_PREF(rt->rt_metric2,
						       bnp->bgp_setpref,
						       bnp->bgp_preference);
		    break;
		case 2:
		    preference = BGP_V3METRIC_TO_PREF(rt->rt_metric2,
						      bnp->bgp_setpref,
						      bnp->bgp_preference);
		    break;
		}
		res = import(rt->rt_dest,
			     rt->rt_dest_mask,
			     bnp->bgp_import,
			     bgp_import_aspath_list,
			     (adv_entry *)0,
			     &preference,
			     bnp->bgp_ifap,
			     (void_t) asp);
	    }

	    if (!res) {
		preference = -1;
		if (keepnone) {
		    delete_it = 1;
		}
	    }
	    if (delete_it) {
		changes++;
		if (bsp) {
		    bgp_sync_rt_delete(bsp, rt);
		} else {
		    rt_delete(rt);
		}
		delete_it = 0;
	    } else if (bsp) {
		(void) bgp_sync_rt_change(bsp,
					  bnp,
					  rt,
					  rt->rt_metric,
					  rt->rt_metric2,
					  rt->rt_tag,
					  preference,
					  bnp->bgp_preference2,
					  rt->rt_n_gw,
					  rt->rt_routers,
					  asp);
	    } else if (rt->rt_preference != preference
	      || rt->rt_preference2 != bnp->bgp_preference2) {
		changes++;
		(void) rt_change_aspath(rt,
					rt->rt_metric,
					rt->rt_metric2,
					rt->rt_tag,
					preference,
					bnp->bgp_preference2,
					rt->rt_n_gw,
					rt->rt_routers,
					asp);
	    }
	} RTQ_LIST_END(rtq, rt);

	rt_close(bnp->bgp_task, &bnp->bgp_gw, changes, NULL);
    }

    /*
     * Check out default generation.  The _add() before and the
     * _delete() after the rt_close() for maximum consistancy.
     */
    if (!BIT_TEST(bnp->bgp_options, BGPO_NOGENDEFAULT) &&
	!BIT_TEST(bnp->bgp_flags, BGPF_GENDEFAULT)) {
	rt_default_add();
	BIT_SET(bnp->bgp_flags, BGPF_GENDEFAULT);
    }

    if (BIT_TEST(bnp->bgp_options, BGPO_NOGENDEFAULT) &&
	BIT_TEST(bnp->bgp_flags, BGPF_GENDEFAULT)) {
	rt_default_delete();
	BIT_RESET(bnp->bgp_flags, BGPF_GENDEFAULT);
    }
}


/*
 * bgp_rt_init_mem - initialize task blocks used by BGP routing
 */
static void
bgp_rt_init_mem __PF0(void)
{
    if (!bgp_hash_block) {
	bgp_hash_block = task_block_init(PATHHASHSIZE*sizeof(bgp_asp_list *),
					 "bgp_asp_hash");
	bgp_mets_block = task_block_init(sizeof(bgp_metrics_node),
					 "bgp_metrics_node");
	bgp_asp_block = task_block_init(sizeof(bgp_asp_list),
					"bgp_asp_list");
	bgp_rti_block = task_block_init(sizeof(bgp_rti_entry),
					"bgp_rti_entry");
	bgp_ent_block = task_block_init(sizeof(bgp_adv_entry),
					"bgp_adv_entry");
    }
}

/*
 * bgp_rt_peer_init - initialize the route queues for a peer.  This
 * allocates the AS path hash array and makes sure the pointers are zeroed.
 */
static void
bgp_rt_peer_init __PF1(bnp, bgpPeer *)
{
    register bgp_rt_queue *qp = &bnp->bgp_asp_queue;

    if (!bgp_hash_block) {
	bgp_rt_init_mem();
    }

    if (!bgp_rto_block) {
	bgp_rto_block = task_block_init(sizeof(bgp_rto_entry), "bgp_rto_entry");
    }
    qp->bgpq_asp_hash = BRT_HASH_ALLOC();
    BGP_RTQ_INIT(qp);
    BGP_ADVQ_INIT(&(bnp->bgp_queue));
    bnp->bgp_rto_next_time = (time_t) 0;
    BGP_RTI_INIT(bnp);

    /*
     * Set us to receive flash updates
     */
    bgp_set_flash(bnp->bgp_task, bgp_peer_flash, bgp_peer_newpolicy);

    /*
     * Set us to receive reinit requests
     */
    bgp_set_reinit(bnp->bgp_task, bgp_rt_reinit);

    /*
     * Fetch an rtbit
     */
    bnp->bgp_task->task_rtbit = rtbit_alloc(bnp->bgp_task,
					    FALSE,
					    BRT_TSI_SIZE,
					    (void_t) bnp,
					    bgp_peer_tsi_dump);

    /*
     * Set us as the receiver for incoming data.
     */
    if (bnp->bgp_version == BGP_VERSION_4) {
	bgp_recv_change(bnp, bgp_recv_v4_update, "bgp_recv_v4_update");
    } else {
	bgp_recv_change(bnp, bgp_recv_v2or3_update, "bgp_recv_v2or3_update");
    }
}


/*
 * bgp_rto_peer_terminate - terminate a peer's outgoing routing
 */
static void
bgp_rto_peer_terminate __PF1(bnp, bgpPeer *)
{
    register bgp_adv_entry *entp;
    u_int rtbit = bnp->bgp_task->task_rtbit;

    assert(bnp->bgp_group->bgpg_type == BGPG_EXTERNAL);

    /*
     * Blow away any queued route structures
     */
    if (!BGP_RTQ_EMPTY(&(bnp->bgp_asp_queue))) {
	register bgp_asp_list *aspl;
	register bgp_rt_queue *qp = bnp->bgp_asp_first;
	register bgp_rto_entry *brto;
	register bgp_asp_list **hash = bnp->bgp_asp_hash;

	while (qp != &(bnp->bgp_asp_queue)) {
	    aspl = ASPL_FROM_QUEUE(qp);
	    qp = qp->bgpq_next;
	    if (aspl->bgpl_asp) {
		hash[aspl->bgpl_asp->path_hash] = (bgp_asp_list *) 0;
	    }

	    brto = aspl->bgpl_rto_next;
	    while (brto != (bgp_rto_entry *)aspl) {
		register bgp_rto_entry *brto_next = brto->bgpo_next;
		BRT_RTO_FREE(brto);
		brto = brto_next;
	    }
	    BRT_ASPL_FREE(aspl);
	}
	BGP_RTQ_INIT(qp);
    }

    BRT_HASH_FREE_CLEAR(bnp->bgp_asp_hash);
    bnp->bgp_asp_hash = (bgp_asp_list **) 0;
    bnp->bgp_rto_next_time = (time_t) 0;

    /*
     * Blow away our routing bit from all routes we've announced.
     * Clear the tsi field while we're at it.
     */
    rt_open(bnp->bgp_task);
#define	BRTVTOENT(v)	((bgp_adv_entry *)(v))
    entp = BRTVTOENT(bnp->bgp_queue.bgpv_next);
    while (entp != BRTVTOENT(&(bnp->bgp_queue))) {
	register bgp_adv_entry *entp_next = BRTVTOENT(entp->bgpe_next);

	BRT_TSI_CLEAR(entp->bgpe_rt->rt_head, rtbit);
	rtbit_reset(entp->bgpe_rt, rtbit);
	if (entp->bgpe_metrics) {
	    BGPM_FREE(entp->bgpe_metrics);
	}
	BRT_ENT_FREE(entp);
	entp = entp_next;
    }
#undef BRTVTOENT
    rt_close(bnp->bgp_task, &bnp->bgp_gw, 0, NULL);
    BGP_ADVQ_INIT(&(bnp->bgp_queue));

    /*
     * Free our rtbit
     */
    rtbit_free(bnp->bgp_task, rtbit);

    /*
     * Make sure we don't get flashed again
     */
    bgp_reset_flash(bnp->bgp_task);
}


/*
 * bgp_rti_peer_terminate - terminate a peer's incoming routing
 */
static void
bgp_rti_peer_terminate __PF1(bnp, bgpPeer *)
{
    bgp_sync *bsp = (bgp_sync *) 0;

    /*
     * Determine is this is a synchronized peer
     */
    if (bnp->bgp_group->bgpg_type == BGPG_INTERNAL_RT) {
	bsp = bnp->bgp_group->bgpg_sync;
	assert(bsp);
    }

    /*
     * Blow away any incoming queue
     */
    if (bnp->bgp_rti_next != bnp->bgp_rti_prev) {
	register bgp_rti_entry *brti, *brti_next;
	register bgp_rti_entry *brti_q = (bgp_rti_entry *) &(bnp->bgp_rti_next);

	for (brti = bnp->bgp_rti_next; brti != brti_q; brti = brti_next) {
	    brti_next = brti->bgpi_next;
	    if (brti->bgpi_old_rt) {
		brti->bgpi_old_rt->rt_data = (void_t) 0;
	    }
	    if (brti->bgpi_new_rt) {
		brti->bgpi_new_rt->rt_data = (void_t) 0;
	    }
	    BRT_RTI_FREE(brti);
	}
	BGP_RTI_INIT(bnp);
    }

    /*
     * Now blow away any routes which belong to us.
     */
    if (bnp->bgp_gw.gw_n_routes) {
	register rt_entry *rt;
	register rtq_entry *rtq = &(bnp->bgp_gw.gw_rtq);
	register int changes = 0;

	rt_open(bnp->bgp_task);

	RTQ_LIST(rtq, rt) {
	    if (bsp) {
		bgp_sync_rt_delete(bsp, rt);
	    } else {
		rt_delete(rt);
	    }
	    changes++;
	} RTQ_LIST_END(rtq, rt);

	rt_close(bnp->bgp_task, &bnp->bgp_gw, changes, NULL);
    }

    /*
     * Finally, prevent us from receiving future reinit requests
     * if the peer is a type which received them
     */
    switch(bnp->bgp_group->bgpg_type) {
    case BGPG_EXTERNAL:
    case BGPG_INTERNAL:
    case BGPG_INTERNAL_RT:
	bgp_reset_reinit(bnp->bgp_task);
	break;
    default:
	break;
    }

    /*
     * Remove knowlege of the synchronization structure from
     * the peer's gw_entry.
     */
    if (bsp) {
	bnp->bgp_gw.gw_data = (void_t) 0;
    }
}


/*
 * bgp_rt_group_init - initialize a peer group's queue structures
 */
static void
bgp_rt_group_init __PF1(bgp, bgpPeerGroup *)
{
    if (!bgp_hash_block) {
	bgp_rt_init_mem();
    }
    if (!bgpg_rto_block) {
	bgpg_rto_block = task_block_init(sizeof(bgpg_rto_entry), "bgp_rto_entry");
    }
    if (bgp->bgpg_type == BGPG_INTERNAL_RT) {
	bgp->bgpg_sync = bgp_sync_init(bgp);
    }

    bgp->bgpg_asp_hash = BRT_HASH_ALLOC();
    BGP_ADVQ_INIT(&(bgp->bgpg_queue));
    BGP_RTQ_INIT(&(bgp->bgpg_asp_queue));
    bgp->bgpg_rto_next_time = (time_t) 0;
    if (bgp->bgpg_oinfo_blk) {
	assert(bgp->bgpg_idx_size > 0);
    } else {
	register u_int n;

	/*
	 * Compute the initial size of the bit mask we need based
	 * on the number of peers in the group.
	 */
	n = (bgp->bgpg_n_peers + BGPB_BITSBITS - 1) / BGPB_BITSBITS;
	if (n == 0) {
	    n = 1;
	}
	bgp->bgpg_idx_size = n;

	/*
	 * Initialize the group bits structures.  We malloc the little
	 * bits of memory it needs when we have more than a single word
	 * of bits.
	 */
	if (n == 1) {
	    bgp->bgpg_v3_bits.bgp_gr_bits = (bgp_bits) 0;
	    bgp->bgpg_v4_bits.bgp_gr_bits = (bgp_bits) 0;
	    bgp->bgpg_v3_sync.bgp_gr_bits = (bgp_bits) 0;
	    bgp->bgpg_v4_sync.bgp_gr_bits = (bgp_bits) 0;
	} else {
	    register bgp_bits *cp =
		task_mem_calloc(bgp->bgpg_task, 4*n, sizeof(bgp_bits));
	    bgp->bgpg_v3_bits.bgp_gr_bitptr = cp; cp += n;
	    bgp->bgpg_v4_bits.bgp_gr_bitptr = cp; cp += n;
	    bgp->bgpg_v3_sync.bgp_gr_bitptr = cp; cp += n;
	    bgp->bgpg_v4_sync.bgp_gr_bitptr = cp; cp += n;
	}

	bgp->bgpg_oinfo_blk = task_block_init(sizeof(bgpg_rtinfo_entry)
	    + (n - 1) * sizeof(bgp_bits), "bgpg_rtinfo_entry");
    }

    bgp->bgpg_idx_maxalloc = 0;
    bgp->bgpg_n_v3_bits = 0;
    bgp->bgpg_n_v4_bits = 0;
    bgp->bgpg_n_v3_sync = 0;
    bgp->bgpg_n_v4_sync = 0;

    /*
     * Set us to receive flashes
     */
    if (bgp->bgpg_type != BGPG_INTERNAL_IGP) {
	bgp_set_flash(bgp->bgpg_task, bgp_group_flash, bgp_group_newpolicy);
    }

    /*
     * Fetch an rtbit
     */
    bgp->bgpg_task->task_rtbit = rtbit_alloc(bgp->bgpg_task,
					     FALSE,
					     BRT_TSI_SIZE,
					     (void_t) bgp,
					     bgp_group_tsi_dump);
}


/*
 * bgp_rt_group_terminate - free the route queue on a group
 */
static void
bgp_rt_group_terminate __PF1(bgp, bgpPeerGroup *)
{
    register bgp_adv_entry *entp;
    u_int rtbit = bgp->bgpg_task->task_rtbit;


    assert(bgp->bgpg_type != BGPG_EXTERNAL);

    /*
     * Blow away any queued route structures
     */
    if (!BGP_RTQ_EMPTY(&(bgp->bgpg_asp_queue))) {
	register bgp_asp_list *aspl;
	register bgp_rt_queue *qp = bgp->bgpg_asp_first;
	register bgpg_rto_entry *bgrto;
	register bgp_asp_list **hash = bgp->bgpg_asp_hash;

	while (qp != &(bgp->bgpg_asp_queue)) {
	    aspl = ASPL_FROM_QUEUE(qp);
	    qp = qp->bgpq_next;
	    if (aspl->bgpl_asp) {
		hash[aspl->bgpl_asp->path_hash] = (bgp_asp_list *) 0;
	    }

	    bgrto = aspl->bgpl_grto_next;
	    while (bgrto != (bgpg_rto_entry *)aspl) {
		if (bgrto->bgpgo_info) {
		    register bgpg_rtinfo_entry *infop, *info_next;

		    for (infop = bgrto->bgpgo_info; infop; infop = info_next) {
			info_next = infop->bgp_info_next;
			BRT_INFO_FREE(bgp, infop);
		    }
		}
		{
		    register bgpg_rto_entry *bgrto_next = bgrto->bgpgo_next;
		    BRT_GRTO_FREE(bgrto);
		    bgrto = bgrto_next;
		}
	    }
	    BRT_ASPL_FREE(aspl);
	}
	BGP_RTQ_INIT(qp);
    }

    if (bgp->bgpg_asp_hash) {
	BRT_HASH_FREE_CLEAR(bgp->bgpg_asp_hash);
	bgp->bgpg_asp_hash = (bgp_asp_list **) 0;
    }

    /*
     * Blow away our routing bit from all routes we've announced.
     * Clear the tsi field while we're at it.
     */
    rt_open(bgp->bgpg_task);
#define	BRTVTOENT(v)	((bgp_adv_entry *)(v))
    entp = BRTVTOENT(bgp->bgpg_queue.bgpv_next);
    while (entp != BRTVTOENT(&(bgp->bgpg_queue))) {
	register bgp_adv_entry *entp_next = BRTVTOENT(entp->bgpe_next);

	BRT_TSI_CLEAR(entp->bgpe_rt->rt_head, rtbit);
	rtbit_reset(entp->bgpe_rt, rtbit);
	if (entp->bgpe_metrics) {
	    BGPM_FREE(entp->bgpe_metrics);
	}
	BRT_ENT_FREE(entp);
	entp = entp_next;
    }
#undef BRTVTOENT
    rt_close(bgp->bgpg_task, (gw_entry *)0, 0, NULL);
    BGP_ADVQ_INIT(&(bgp->bgpg_queue));

    if (bgp->bgpg_oinfo_blk) {
	register size_t n = bgp->bgpg_idx_size;

	bgp->bgpg_idx_maxalloc = 0;
	BGPB_ZERO(BGPG_GETBITS(bgp->bgpg_v3_bits, n), n);
	BGPB_ZERO(BGPG_GETBITS(bgp->bgpg_v4_bits, n), n);
	BGPB_ZERO(BGPG_GETBITS(bgp->bgpg_v3_sync, n), n);
	BGPB_ZERO(BGPG_GETBITS(bgp->bgpg_v4_sync, n), n);
    }

    bgp->bgpg_n_v3_bits = 0;
    bgp->bgpg_n_v4_bits = 0;
    bgp->bgpg_n_v3_sync = 0;
    bgp->bgpg_n_v4_sync = 0;
    bgp->bgpg_rto_next_time = (time_t) 0;

    /*
     * Free our rtbit
     */
    rtbit_free(bgp->bgpg_task, rtbit);

    /*
     * Reset the flash routines so we don't get them any more
     */
    if (bgp->bgpg_type != BGPG_INTERNAL_IGP) {
	bgp_reset_flash(bgp->bgpg_task);
    }

    /*
     * If this is a synchronized group, terminate synchronization
     */
    if (bgp->bgpg_type == BGPG_INTERNAL_RT) {
	bgp_sync_terminate(bgp->bgpg_sync);
    }
}


/*
 * bgp_rt_group_expand - grow the number of bits in a group queue
 */
static void
bgp_rt_group_expand __PF1(bgp, bgpPeerGroup *)
{
    register bgp_asp_list *aspl;
    register u_int n, n_old, i;
    register block_t oblk, nblk;
    register bgp_bits *np;

    if (bgp->bgpg_idx_size == 0) {
	/* Shouldn't happen but ... */
	assert(FALSE);
    }

    n_old = bgp->bgpg_idx_size;
    n = (bgp->bgpg_n_peers + BGPB_BITSBITS - 1) / BGPB_BITSBITS;
    if (n <= n_old) {
	n = n_old + 1;
    }
    bgp->bgpg_idx_size = n;

    oblk = bgp->bgpg_oinfo_blk;
    nblk = task_block_init(sizeof(bgpg_rtinfo_entry)
	+ (n - 1) * sizeof(bgp_bits), "bgpg_rtinfo_entry");
    bgp->bgpg_oinfo_blk = nblk;

    /*
     * Expand everything on the queue
     */
    if ((aspl = BGP_ASPL_FIRST(&(bgp->bgpg_asp_queue))) != (bgp_asp_list *)0) {
	register size_t osize =
	    sizeof(bgpg_rtinfo_entry) + (n_old - 1) * sizeof(bgp_bits);
	register bgpg_rtinfo_entry *info_old, *info_new;
	register bgpg_rto_entry *bgrto;

	do {
	    for (bgrto = aspl->bgpl_grto_next;
		 bgrto != (bgpg_rto_entry *)aspl;
		 bgrto = bgrto->bgpgo_next) {
		if (bgrto->bgpgo_info) {
		    info_old = bgrto->bgpgo_info;
		    info_new = task_block_alloc(nblk);
		    bcopy((void_t) info_old, (void_t) info_new, osize);
		    bgrto->bgpgo_info = info_new;
		    task_block_free(oblk, info_old);
		    while (info_new->bgp_info_next) {
			info_old = info_new->bgp_info_next;
			info_new->bgp_info_next = task_block_alloc(nblk);
			info_new = info_new->bgp_info_next;
			bcopy((void_t) info_old, (void_t) info_new, osize);
			task_block_free(oblk, info_old);
		    }
		}
	    }
	} while ((aspl = BGP_ASPL_NEXT(&(bgp->bgpg_asp_queue), aspl)));
    }

    /*
     * Now expand the group bits
     */
    np = task_mem_malloc(bgp->bgpg_task, 4 * n * sizeof(bgp_bits));
    if (n_old == 1) {
	*np = bgp->bgpg_v3_bits.bgp_gr_bits;
	bgp->bgpg_v3_bits.bgp_gr_bitptr = np++;
	for (i = (n - 1); i > 0; i--) {
	    *np++ = (bgp_bits) 0;
	}
	*np = bgp->bgpg_v4_bits.bgp_gr_bits;
	bgp->bgpg_v4_bits.bgp_gr_bitptr = np; np += n;
	for (i = (n - 1); i > 0; i--) {
	    *np++ = (bgp_bits) 0;
	}
	*np = bgp->bgpg_v3_sync.bgp_gr_bits;
	bgp->bgpg_v3_sync.bgp_gr_bitptr = np; np += n;
	for (i = (n - 1); i > 0; i--) {
	    *np++ = (bgp_bits) 0;
	}
	*np = bgp->bgpg_v4_sync.bgp_gr_bits;
	bgp->bgpg_v4_sync.bgp_gr_bitptr = np; np += n;
	for (i = (n - 1); i > 0; i--) {
	    *np++ = (bgp_bits) 0;
	}
    } else {
	register bgp_bits *op = bgp->bgpg_v3_bits.bgp_gr_bitptr;
	bgp_bits *oldp = op;

	bgp->bgpg_v3_bits.bgp_gr_bitptr = np;
	for (i = n_old; i > 0; i--) {
	    *np++ = *op++;
	}
	for (i = (n - n_old); i > 0; i--) {
	    *np++ = (bgp_bits) 0;
	}

	bgp->bgpg_v4_bits.bgp_gr_bitptr = np;
	for (i = n_old; i > 0; i--) {
	    *np++ = *op++;
	}
	for (i = (n - n_old); i > 0; i--) {
	    *np++ = (bgp_bits) 0;
	}

	bgp->bgpg_v3_sync.bgp_gr_bitptr = np;
	for (i = n_old; i > 0; i--) {
	    *np++ = *op++;
	}
	for (i = (n - n_old); i > 0; i--) {
	    *np++ = (bgp_bits) 0;
	}

	bgp->bgpg_v4_sync.bgp_gr_bitptr = np;
	for (i = n_old; i > 0; i--) {
	    *np++ = *op++;
	}
	for (i = (n - n_old); i > 0; i--) {
	    *np++ = (bgp_bits) 0;
	}

	task_mem_free(bgp->bgpg_task, (void_t) oldp);
    }
}


/*
 * bgp_rt_group_delete - delete anything in the group related to route queues
 */
void
bgp_rt_group_delete __PF1(bgp, bgpPeerGroup *)
{
    if (bgp->bgpg_asp_hash) {
	bgp_rt_group_terminate(bgp);
    }

    if (bgp->bgpg_oinfo_blk) {
	task_mem_free(bgp->bgpg_task, (void_t) bgp->bgpg_v3_bits.bgp_gr_bitptr);
	bgp->bgpg_v3_bits.bgp_gr_bits = (bgp_bits) 0;
	bgp->bgpg_v4_bits.bgp_gr_bits = (bgp_bits) 0;
	bgp->bgpg_v3_sync.bgp_gr_bits = (bgp_bits) 0;
	bgp->bgpg_v4_sync.bgp_gr_bits = (bgp_bits) 0;
	bgp->bgpg_oinfo_blk = (block_t) 0;
	bgp->bgpg_idx_size = bgp->bgpg_idx_maxalloc = 0;
    }
}


/*
 * bgp_rt_group_peer_init - prepare a group peer for routing.  Prepare the
 *			    group if it isn't ready.
 */
static void
bgp_rt_group_peer_init __PF1(bnp, bgpPeer *)
{
    register u_int i, n;
    bgpPeerGroup *bgp = bnp->bgp_group;

    assert(bgp->bgpg_type != BGPG_EXTERNAL);

    if (!bgp->bgpg_asp_hash) {
	bgp_rt_group_init(bgp);
    }

    if (bgp->bgpg_type == BGPG_INTERNAL_RT) {
	bnp->bgp_gw.gw_data = (void_t) bgp->bgpg_sync;
    }

    n = bgp->bgpg_idx_size;
    i = bgp->bgpg_idx_maxalloc;
    if (i >= n * BGPB_BITSBITS) {
	if ((bgp->bgpg_n_v3_bits + bgp->bgpg_n_v4_bits) >= n * BGPB_BITSBITS) {
	    bgp_rt_group_expand(bgp);
	    bgp->bgpg_idx_maxalloc++;
	    n = bgp->bgpg_idx_size;
	} else {
	    register bgp_bits *v3p = BGPG_GETBITS(bgp->bgpg_v3_bits, n);
	    register bgp_bits *v4p = BGPG_GETBITS(bgp->bgpg_v4_bits, n);
	    register s = n * BGPB_BITSBITS;

	    /*
	     * Find an unset bit somewhere.  If none, expand the bits
	     */
	    s *= BGPB_BITSBITS;
	    for (i = 0; i < s; i++) {
		if (!BGPB_BTEST(v3p, i) && !BGPB_BTEST(v4p, i)) {
		    break;
		}
	    }
	    assert(i < s);
	}
    } else {
	/*
	 * The maxalloc bit is fine
	 */
	bgp->bgpg_idx_maxalloc++;
    }

    if (bnp->bgp_version == BGP_VERSION_4) {
	BGPB_BSET(BGPG_GETBITS(bgp->bgpg_v4_bits, n), i);
	bgp->bgpg_n_v4_bits++;
    } else {
	BGPB_BSET(BGPG_GETBITS(bgp->bgpg_v3_bits, n), i);
	bgp->bgpg_n_v3_bits++;
    }
    bnp->bgp_group_bit = i;
    BGP_RTI_INIT(bnp);

    /*
     * Set us to receive reinit requests if this is a group which needs them
     */
    switch (bgp->bgpg_type) {
    case BGPG_INTERNAL:
    case BGPG_INTERNAL_RT:
        bgp_set_reinit(bnp->bgp_task, bgp_rt_reinit);
	break;
    default:
	break;
    }

    /*
     * Set the receiver for incoming packets
     */
    if (bnp->bgp_version == BGP_VERSION_4) {
	bgp_recv_change(bnp, bgp_recv_v4_update, "bgp_recv_v4_update");
    } else {
	bgp_recv_change(bnp, bgp_recv_v2or3_update, "bgp_recv_v2or3_update");
    }
}


/*
 * bgp_rto_group_v4_terminate - remove the rtbit from all v4-only routes
 *				when the last v4 peer goes away.
 */
static void
bgp_rto_group_v4_terminate __PF1(bgp, bgpPeerGroup *)
{
    u_int rtbit = bgp->bgpg_task->task_rtbit;
    bgp_adv_queue *vqp = &(bgp->bgpg_queue);
    bgp_adv_entry *entp, *entp_next;

    for (entp = BGP_ADV_FIRST(vqp); entp; entp = entp_next) {
	entp_next = BGP_ADV_NEXT(vqp, entp);

	/*
	 * If this isn't a version 4 route, continue
	 */
	if (!BIT_TEST(entp->bgpe_flags, BGPEF_V4_ONLY)) {
	    continue;
	}

	/*
	 * Version 4 route.  Blow away the metrics.  If the route
	 * is queued, blow away the queue structures.  Then clear
	 * the tsi, reset our bit and blow away the entry.
	 */
	if (entp->bgpe_metrics) {
	    BGPM_FREE(entp->bgpe_metrics);
	}
	if (BIT_TEST(entp->bgpe_flags, BGPEF_QUEUED)) {
	    register bgp_asp_list *aspl;
	    register bgpg_rto_entry *bgrto;
	    bgp_adv_entry *nentp;

	    BRT_TSI_GET_GROUP(entp->bgpe_rt->rt_head, rtbit, bgrto, nentp);
	    assert(bgrto && nentp == entp);

	    if (bgrto->bgpgo_info) {
		register bgpg_rtinfo_entry *infop, *info_next;
		for (infop = bgrto->bgpgo_info; infop; infop = info_next) {
		    info_next = infop->bgp_info_next;
		    BRT_INFO_FREE(bgp, infop);
		}
	    }
	    aspl = (bgp_asp_list *)bgrto->bgpgo_prev;
	    BGP_GRTO_UNLINK(bgrto);
	    BRT_GRTO_FREE(bgrto);
	    if (aspl->bgpl_grto_prev == aspl->bgpl_grto_next) {
		BGP_ASPL_REMOVE(&(bgp->bgpg_asp_queue), aspl);
		BRT_ASPL_FREE(aspl);
	    }
	}

	BRT_TSI_CLEAR(entp->bgpe_rt->rt_head, rtbit);
	rtbit_reset(entp->bgpe_rt, rtbit);
	BRT_ENT_FREE(entp);
    }
}



/*
 * bgp_rto_group_peer_terminate - remove a group peer from routing.  If
 *			          there is a queue, delete this peer's bit.
 */
static void
bgp_rto_group_peer_terminate __PF1(bnp, bgpPeer *)
{
    register int word, wbit;
    register int res;
    bgpPeerGroup *bgp = bnp->bgp_group;
    bgp_bits *sbits;

    /*
     * Make sure this peer is actually in the group routing.  Turn
     * off it's bits if so.
     */
    word = BGPB_WORD(bnp->bgp_group_bit);
    wbit = BGPB_WBIT(bnp->bgp_group_bit);

    if (bnp->bgp_version == BGP_VERSION_4) {
	assert(bgp->bgpg_n_v4_bits > 0);
	if (bgp->bgpg_n_v4_sync > 0) {
	    sbits = BGPG_GETBITS(bgp->bgpg_v4_sync, bgp->bgpg_idx_size);
	    if (BGPB_WB_TEST(sbits, word, wbit)) {
		BGPB_WB_RESET(sbits, word, wbit);
		bgp->bgpg_n_v4_sync--;
	    }
	}
	sbits = BGPG_GETBITS(bgp->bgpg_v4_bits, bgp->bgpg_idx_size);
	assert(BGPB_WB_TEST(sbits, word, wbit));
	BGPB_WB_RESET(sbits, word, wbit);
	bgp->bgpg_n_v4_bits--;
    } else {
	assert(bgp->bgpg_n_v3_bits > 0);
	if (bgp->bgpg_n_v3_sync > 0) {
	    sbits = BGPG_GETBITS(bgp->bgpg_v3_sync, bgp->bgpg_idx_size);
	    if (BGPB_WB_TEST(sbits, word, wbit)) {
		BGPB_WB_RESET(sbits, word, wbit);
		bgp->bgpg_n_v3_sync--;
	    }
	}
	sbits = BGPG_GETBITS(bgp->bgpg_v3_bits, bgp->bgpg_idx_size);
	assert(BGPB_WB_TEST(sbits, word, wbit));
	BGPB_WB_RESET(sbits, word, wbit);
	bgp->bgpg_n_v3_bits--;
    }

    /*
     * If we turned off the last bit, and the group routing structures
     * still exist, blow away the whole group.  Otherwise we pick through
     * the queue turning off our bit.
     */
    if (bgp->bgpg_n_v4_bits == 0 && bgp->bgpg_n_v3_bits == 0) {
	bgp_rt_group_terminate(bgp);
    } else if (!BGP_RTQ_EMPTY(&(bgp->bgpg_asp_queue))) {
	register bgp_asp_list *aspl;
	register bgpg_rto_entry *bgrto;
	register bgpg_rtinfo_entry *infop;
	register bgp_rt_queue *qp = bgp->bgpg_asp_first;
	register bgp_adv_entry *entp;
	u_int rtbit = bgp->bgpg_task->task_rtbit;

	/*
	 * If we're the last version 4 peer to go, blow away all the
	 * version 4 only routes first.
	 */
	rt_open(bgp->bgpg_task);

	if (bnp->bgp_version == BGP_VERSION_4 && bgp->bgpg_n_v4_bits == 0) {
	    bgp_rto_group_v4_terminate(bgp);
	}

	while (qp != &(bgp->bgpg_asp_queue)) {
	    aspl = ASPL_FROM_QUEUE(qp);
	    qp = qp->bgpq_next;
	    bgrto = aspl->bgpl_grto_next;
	    while (bgrto != (bgpg_rto_entry *) aspl) {
		register bgpg_rto_entry *next = bgrto->bgpgo_next;

		infop = bgrto->bgpgo_info;
		while (infop) {
		    if (BGPB_WB_TEST(infop->bgp_info_bits, word, wbit)) {
			break;
		    }
		    infop = infop->bgp_info_next;
		}

		if (infop) {
		    BGPB_WB_RESET(infop->bgp_info_bits, word, wbit);
		    BGPB_CHECK_CLEAR(infop->bgp_info_bits,
				     bgp->bgpg_idx_size,
				     res);
		    if (res) {
			if (bgrto->bgpgo_info == infop) {
			    bgrto->bgpgo_info = infop->bgp_info_next;
			} else {
			    register bgpg_rtinfo_entry *tmp;

			    tmp = bgrto->bgpgo_info;
			    while (tmp->bgp_info_next != infop) {
				tmp = tmp->bgp_info_next;
			    }
			    tmp->bgp_info_next = infop->bgp_info_next;
			}
			BRT_INFO_FREE(bgp, infop);
		    }
		}

		if (!bgrto->bgpgo_info) {
		    entp = bgrto->bgpgo_advrt;
		    if (!entp->bgpe_metrics) {
			/*
			 * Delete here, blow away the rtbit and the
			 * entry.
			 */
			BRT_TSI_CLEAR(entp->bgpe_rt->rt_head, rtbit);
			rtbit_reset(entp->bgpe_rt, rtbit);
			BGP_ADV_DEQUEUE(entp);
			BRT_ENT_FREE(entp);
		    } else {
			BIT_RESET(entp->bgpe_flags, BGPEF_QUEUED);
			BRT_TSI_PUT_ENT(entp->bgpe_rt->rt_head, rtbit, entp);
		    }
		    BGP_GRTO_UNLINK(bgrto);
		    BRT_GRTO_FREE(bgrto);
		}
		bgrto = next;
	    }

	    /*
	     * See if the AS path list now empty
	     */
	    if (BGP_ASPL_EMPTY(aspl)) {
		BGP_ASPL_REMOVE(&(bgp->bgpg_asp_queue), aspl);
		BRT_ASPL_FREE(aspl);
	    }
	}
	rt_close(bgp->bgpg_task, (gw_entry *)0, 0, NULL);
    }
    bnp->bgp_rto_next_time = (time_t) 0;
}


/*
 * bgp_rt_send_init - initialize the peer by allocating a routing bit,
 *   if necessary, and sending an initial blast of routes.
 */
void
bgp_rt_send_init __PF1(bnp, bgpPeer *)
{
    bgpPeerGroup *bgp;
    rt_list *rtl = (rt_list *) 0;
    int res;

    bgp = bnp->bgp_group;
    BIT_SET(bnp->bgp_flags, BGPF_INITIALIZING);

    if (bgp->bgpg_type == BGPG_EXTERNAL) {
	/*
	 * Initialize routing for the peer
	 */
	bgp_rt_peer_init(bnp);

        /*
         * Now update this guy's routes
         */
	rtl = rthlist_active(AF_INET);
	if (rtl) {
	    res = bgp_rt_policy_peer(bnp, rtl, BRTUPD_INITIAL);
	} else {
	    res = 0;
	}
	if (!BIT_TEST(bnp->bgp_options, BGPO_NOGENDEFAULT)) {
	    rt_default_add();
	    BIT_SET(bnp->bgp_flags, BGPF_GENDEFAULT);
	}
    } else {
	int what;

	/*
	 * Decide how this guy should be updated.
	 */
	what = BRTINI_ADD;
	if (bgp->bgpg_n_v3_bits == 0 && bgp->bgpg_n_v4_bits == 0) {
	    what = BRTINI_FIRST;
	} else if (bnp->bgp_version == BGP_VERSION_4
		&& bgp->bgpg_n_v4_bits == 0) {
	    what = BRTINI_FIRST_V4;
	}

	/*
	 * Initialize routing for this peer.
	 */
	bgp_rt_group_peer_init(bnp);

	/*
	 * Now send the initial set of routes
	 */
	if (what == BRTINI_ADD) {
	    res = bgp_rt_policy_add_group_peer(bnp);
	} else {
	    rtl = rthlist_active(AF_INET);
	    if (rtl) {
		res = bgp_rt_policy_init_group_peer(bnp, rtl, what);
	    } else {
		res = 0;
	    }
	}
    }

    if (rtl) {
	RTLIST_RESET(rtl);
    }

    /*
     * If we have nothing to send, send a keepalive.  Yakov used
     * to use a keepalive at the end of the initial blast for
     * something, it doesn't hurt to send one, and this'll make
     * sure we get the last_sent time correct.
     *
     * If we have something to send, schedule a write routine to
     * flush it out.
     */
    bnp->bgp_last_sent = bgp_time_sec;
    if (res == 0) {
	res = bgp_send_keepalive(bnp, 1);
	bnp->bgp_last_keepalive = bgp_time_sec;
	if (res < 0) {
	    bgp_peer_close(bnp, BGPEVENT_ERROR);
	    return;
	}
	BIT_RESET(bnp->bgp_flags, BGPF_INITIALIZING);
	if (res == 0) {
	bgp_set_write(bnp);
	} else if (bgp->bgpg_type != BGPG_EXTERNAL) {
	    bgp_rt_sync(bnp);
	}
    } else {
	bgp_set_write(bnp);
    }
}


/*
 * bgp_rt_terminate - delete all the routes in the table from the
 *		      specified peer.
 */
void
bgp_rt_terminate __PF1(bnp, bgpPeer *)
{
    bgpPeerGroup *bgp;

    bgp = bnp->bgp_group;

    /*
     * Shitcan this guy's incoming routing.
     */
    bgp_rti_peer_terminate(bnp);

    /*
     * If this is an external peer, terminate his incoming routing
     * and quit generating default.  If this is a group peer dump
     * him from the group's routing.
     */
    if (bgp->bgpg_type == BGPG_EXTERNAL) {
	bgp_rto_peer_terminate(bnp);
	if (BIT_TEST(bnp->bgp_flags, BGPF_GENDEFAULT)) {
	    rt_default_delete();
	    BIT_RESET(bnp->bgp_flags, BGPF_GENDEFAULT);
	}
    } else {
	bgp_rto_group_peer_terminate(bnp);
    }
}

/*
 * bgp_rt_send_ready - called when a previously write-blocked peer has
 *		       unblocked.
 */
void
bgp_rt_send_ready __PF1(bnp, bgpPeer *)
{
    /*
     * Call the appropriate flush routine to write some more out.
     * It'll take care of the rest.
     */
    if (bnp->bgp_group->bgpg_type == BGPG_EXTERNAL) {
	bgp_rt_peer_flush(bnp);
    } else {
	bgp_rt_group_flush(bnp->bgp_group, bnp);
    }
}


/*
 * bgp_rt_peer_timer - called when peer route timer expires.  Flush
 *		       anything ready-to-go to this (external) peer.
 */
void
bgp_rt_peer_timer __PF1(bnp, bgpPeer *)
{
    assert(bnp->bgp_group->bgpg_type == BGPG_EXTERNAL);
    if (BIT_TEST(bnp->bgp_flags, BGPF_WRITEFAILED|BGPF_SEND_RTN)) {
	bnp->bgp_rto_next_time = (time_t) 0;
    } else {
	bgp_rt_peer_flush(bnp);
    }
}


/*
 * bgp_rt_group_timer - called when the group route timer expires.  Flush
 *			anything ready-to-go out of this group.
 */
void
bgp_rt_group_timer __PF1(bgp, bgpPeerGroup *)
{
    if ((bgp->bgpg_n_v3_sync == 0 && bgp->bgpg_n_v4_sync == 0)
      || BGP_RTQ_EMPTY(&(bgp->bgpg_asp_queue))) {
	bgp->bgpg_rto_next_time = (time_t) 0;
    } else {
	bgp_rt_group_flush(bgp, (bgpPeer *) 0);
    }
}


/*
 * bgp_rt_init - initialize the BGP routing module
 */
void
bgp_rt_init __PF0(void)
{
    /*
     * Initialize the task_block pointers which are used by all peers/groups
     */
    if (!bgp_hash_block) {
	bgp_rt_init_mem();
    }
}

/*
 * bgp_rt_peer_delete - called when a peer structure is being deleted.
 *			Currently does nothing.
 */
void
bgp_rt_peer_delete __PF1(bnp, bgpPeer *)
{
    /* nothing */
}
