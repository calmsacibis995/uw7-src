#ident	"@(#)krt.c	1.6"
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


/* krt.c
 *
 * Kernel routing table interface routines
 */

#define	INCLUDE_IOCTL
#define	INCLUDE_FILE
#define	INCLUDE_KVM
#define	INCLUDE_ROUTE
#define	INCLUDE_IF
#define	INCLUDE_IF_TYPES
#include "include.h"
#ifdef	PROTO_INET
#include "inet.h"
#endif	/* PROTO_INET */
#ifdef	PROTO_ISO
#include "iso.h"
#endif	/* PROTO_ISO */
#include "krt.h"
#include "krt_var.h"
#ifdef	PROTO_SCRAM
#include "scram.h"
#endif	/* PROTO_SCRAM */

/* What flags mean it was a redirect */
#ifdef	RTF_MODIFIED
#define	RTF_REDIRECT	(RTF_DYNAMIC|RTF_MODIFIED)
#else	/* RTF_MODIFIED */
#define	RTF_REDIRECT	RTF_DYNAMIC
#endif	/* RTF_MODIFIED */


char *krt_version_kernel = 0;		/* OS version of the kernel */
trace *kernel_trace_options = { 0 };	/* Trace flags */

flag_t krt_options;			/* krt options */
time_t krt_t_expire;			/* default remnant expiration delay */

u_long krt_n_routes = 0;
u_long krt_limit_routes = KRT_COUNT_UNLIMITED;

task *krt_task = 0;		/* Task for kernel routing table */
task_timer *krt_timer_ifcheck = (task_timer *) 0;
gw_entry *krt_gwp = 0;		/* Gateway structure for kernel static routes */
gw_entry *krt_gwp_remnant = 0;	/* Gateway structure for kernel remnant routes */
gw_entry *krt_gwp_temp = 0;	/* Gateway entry for temporary routes */

gw_entry *krt_gw_list = 0;		/* List of gateways for static routes */


/*
 * Kernel remnant deletion.  We blow out kernel remnants as soon as they
 * are overridden by a protocol route.  Trouble is, we only discover this
 * in the flash routine which means we can't delete them immediately.  We
 * hence keep a list of kernel remnants which need to be deleted and
 * blow them away in a foreground task job.
 */
typedef struct _krt_remnant_rt {
    struct _krt_remnant_rt *krt_rem_next;
    rt_entry *krt_rem_rt;
} krt_remnant_rt;

static task_job *krt_remnant_job = (task_job *) 0;
static krt_remnant_rt *krt_remnant_list = (krt_remnant_rt *) 0;
static block_t krt_remnant_rt_block = (block_t) 0;

#define	KRT_REMNANT_RT_INIT() \
    krt_remnant_rt_block = task_block_init(sizeof(krt_remnant_rt), "krt_remnant_rt")

#define	KRT_REMNANT_RT_ALLOC() \
    ((krt_remnant_rt *) task_block_alloc(krt_remnant_rt_block))

#define	KRT_REMNANT_RT_FREE(remp) \
    task_block_free(krt_remnant_rt_block, (void_t)(remp))

#define	KRT_REMNANT_RT_GET(remp) \
    do { \
	if (!krt_remnant_rt_block) { \
	    KRT_REMNANT_RT_INIT(); \
	} \
	(remp) = KRT_REMNANT_RT_ALLOC(); \
    } while (0)

/*
 * Macro to queue the deletion of a kernel remnant
 */
#define	KRT_REMNANT_DELETE(rt) \
    do { \
	rt_entry *Xrt = (rt); \
	if (!(Xrt->rt_data)) { \
	    krt_remnant_rt *Xremp; \
	    KRT_REMNANT_RT_GET(Xremp); \
	    Xrt->rt_data = (void_t) Xremp; \
	    Xremp->krt_rem_rt = Xrt; \
	    Xremp->krt_rem_next = krt_remnant_list; \
	    krt_remnant_list = Xremp; \
	    if (!krt_remnant_job) { \
		krt_remnant_job = task_job_create(krt_task, \
						  TASK_JOB_FG, \
						  "Remnant Deletion", \
						  krt_remnant_remove, \
						  (void_t) 0); \
	    } \
	} \
    } while (0)


/*
 * The kernel route queue structure.  We use these to track
 * kernel routes which need to be updated, along with the
 * state of any route to the same destination which might already
 * be installed in the kernel.
 */
typedef struct _krt_q_entry {
    struct _krt_q_entry *krtq_forw;	/* Forward link pointer */
    struct _krt_q_entry *krtq_back;	/* Backward link pointer */
    int krtq_op;			/* Operation to be performed */
    union {
	time_t krtq_U_time;		/* Try this operation after this time */
	int krtq_U_n_queued;		/* Number of queue entries */
    } krtq_U;
#define	krtq_time	krtq_U.krtq_U_time
#define	krtq_n_queued	krtq_U.krtq_U_n_queued
    rt_entry *krtq_rt;			/* Route with our bit set */
#ifdef	IP_MULTICAST_ROUTING
    pref_t krtq_preference;		/* Preference of route */
    metric_t krtq_metric;		/* Metric of route */
#endif	/* IP_MULTICAST_ROUTING */
    proto_t krtq_proto;			/* Protocol which installed kernel rt */
    flag_t krtq_state;			/* State of kernel route */
    short krtq_queue;			/* Where this route is queued */
    short krtq_n_gw;			/* Number of next hops */
    sockaddr_un *krtq_routers[RT_N_MULTIPATH];
#define	krtq_router	krtq_routers[0]
    if_addr *krtq_ifaps[RT_N_MULTIPATH];
#define	krtq_ifap	krtq_ifaps[0]
} krt_q_entry;

#define	KRTQ_OP_NOP	0		/* Queue bookkeeping entry */
#define	KRTQ_OP_DELETE	1		/* Route delete from kernel */
#define	KRTQ_OP_ADD	2		/* Route add to kernel */
#define	KRTQ_OP_CHANGE	3		/* Delete and add */

/*
 * We keep the queue in priority order.  First are interface routes,
 * then high priority deletes, then high priority changes, then
 * high priority adds, then low priority deletes, then low priority
 * changes, then low priority adds.  The following are bookkeeping
 * queue entries to keep track of where routes should be inserted.
 */
#define	KRTQ_H_INT	0
#define	KRTQ_H_DEL	1
#define	KRTQ_H_CHNG	2
#define	KRTQ_H_ADD	3
#define	KRTQ_L_DEL	4
#define	KRTQ_L_CHNG	5
#define	KRTQ_L_ADD	6
#define	KRTQ_N_PRIO	7

#define	KRTQ_INT_LAST	KRTQ_H_INT
#define	KRTQ_H_LAST	KRTQ_H_ADD
#define	KRTQ_L_LAST	KRTQ_L_ADD

static krt_q_entry krt_queue[KRTQ_N_PRIO];
static int krt_n_queued;		/* Number of things on the queue */
static int krt_n_deferred;		/* Number of deferred on the queue */
static time_t krt_last_q_run;		/* Last time we ran the queue */

static krt_q_entry * const Xkrt_q_next[KRTQ_N_PRIO] = {
    &(krt_queue[1]),
    &(krt_queue[2]),
    &(krt_queue[3]),
    &(krt_queue[4]),
    &(krt_queue[5]),
    &(krt_queue[6]),
    &(krt_queue[0])
};

#ifdef	nodef
static krt_q_entry * const Xkrt_q_prev[KRTQ_N_PRIO] = {
    &(krt_queue[6]),
    &(krt_queue[0]),
    &(krt_queue[1]),
    &(krt_queue[2]),
    &(krt_queue[3]),
    &(krt_queue[4]),
    &(krt_queue[5])
};
#endif	/* notdef */

#define	KRT_Q_END(q)	(Xkrt_q_next[(q)])
#define	KRT_Q_START(q)	(&krt_queue[(q)])

#define	KRT_Q_PREV(q)	(Xkrt_q_prev[(q)])
#define	KRT_Q_NEXT(q)	KRT_Q_END(q)

#define	KRT_N_QUEUED(q)	(krt_queue[(q)].krtq_n_queued)

/*
 * Add an entry to the queue at the end of its priority list
 */
#define	KRT_ENQUEUE(q, ent) \
    do { \
	register krt_q_entry *Xkqp = KRT_Q_END((q)); \
	register krt_q_entry *Xkqp_add = (ent); \
	Xkqp_add->krtq_queue = (q); \
	KRT_N_QUEUED((q))++; \
	Xkqp_add->krtq_forw = Xkqp; \
	Xkqp_add->krtq_back = Xkqp->krtq_back; \
	Xkqp->krtq_back = Xkqp_add; \
	Xkqp_add->krtq_back->krtq_forw = Xkqp_add; \
	krt_n_queued++; \
    } while (0)

/*
 * Remove an entry from the queue
 */
#define	KRT_DEQUEUE(ent) \
    do { \
	register krt_q_entry *Xkqp = (ent); \
	Xkqp->krtq_back->krtq_forw = Xkqp->krtq_forw; \
	Xkqp->krtq_forw->krtq_back = Xkqp->krtq_back; \
	KRT_N_QUEUED(Xkqp->krtq_queue)--; \
	krt_n_queued--; \
    } while (0)

/*
 * Defer the install of a queued route.
 */
#define	KRT_DEFER(ent, t) \
    do { \
	(ent)->krtq_time = (time_t) (t); \
	krt_n_deferred++; \
    } while (0)

/*
 * Undefer a deferred, queued route.
 */
#define	KRT_UNDEFER(ent) \
    do { \
	(ent)->krtq_time = (time_t) 0; \
	krt_n_deferred--; \
    } while (0)


/*
 * Determine if the route is a high priority or low priority
 * route.  For the moment assume it is low priority if RTS_EXTERIOR
 * is set in the state.
 */
#define	KRT_IS_HIPRIO(rt)	(!BIT_TEST((rt)->rt_state, RTS_EXTERIOR))

/*
 * Route state bits we care about
 */
#define	KRT_RTS_NO_NEXTHOP	(RTS_REJECT|RTS_BLACKHOLE)
#define	KRT_RTS_BITS		(RTS_GATEWAY|RTS_REJECT|RTS_BLACKHOLE|RTS_STATIC)

/*
 * Macros for fetching queue pointers out of the tsi field,
 * and sticking them back in.
 *
 * XXX Dump 0xdeadbeef when code works.  Non-debugging version below.
 */
#define	KRT_NO_Q_ENT	((krt_q_entry *) GS2A(0xdeadbeef))
#define	KRT_GET_Q_ENT(rth, rtbit, kqp) \
    do { \
	krt_q_entry *Xkqp; \
	rttsi_get((rth), (rtbit), (byte *) &Xkqp); \
	if (Xkqp == KRT_NO_Q_ENT) { \
	    (kqp) = (krt_q_entry *) 0; \
	} else { \
	    assert(Xkqp); \
	    (kqp) = Xkqp; \
	} \
    } while (0)

#define	KRT_GET_Q_ENT_NOCHECK(rth, rtbit, kqp) \
    do { \
	krt_q_entry *Xkqp; \
	rttsi_get((rth), (rtbit), (byte *) &Xkqp); \
	if (Xkqp == KRT_NO_Q_ENT) { \
	    (kqp) = (krt_q_entry *) 0; \
	} else { \
	    (kqp) = Xkqp; \
	} \
    } while (0)


#define	KRT_PUT_Q_ENT(rth, rtbit, kqp) \
    do { \
	krt_q_entry *Xkqp = (kqp); \
	rttsi_set((rth), (rtbit), (byte *) &Xkqp); \
    } while (0)

#define	KRT_CLR_Q_ENT(rth, rtbit) \
    KRT_PUT_Q_ENT((rth), (rtbit), KRT_NO_Q_ENT)

#ifdef	notdef
#define	KRT_GET_Q_ENT(rth, rtbit, kqp) \
    do { \
	krt_q_entry *Xkqp; \
	rttsi_get((rth), (rtbit), (byte *) &Xkqp); \
	(kqp) = Xkqp; \
    } while (0)

#define	KRT_GET_Q_ENT_NOCHECK(rth, rtbit, kqp) \
	KRT_GET_Q_ENT((rth), (rtbit), (kqp))

#define	KRT_PUT_Q_ENT(rth, rtbit, kqp) \
    do { \
	krt_q_entry *Xkqp = (kqp); \
	rttsi_set((rth), (rtbit), (byte *) &Xkqp); \
    } while (0)

#define	KRT_CLR_Q_ENT(rth, rtbit)	rttsi_reset((rth), (rtbit))
#endif	/* notdef */


/*
 * Return codes for krt_q_flush()
 */
#define	KRT_Q_FLUSH_DONE	0	/* Entire list completed */
#define	KRT_Q_FLUSH_FULL	1	/* Terminated by kernel full */
#define	KRT_Q_FLUSH_COUNT	2	/* Terminated by count */
#define	KRT_Q_FLUSH_BLOCKED	3	/* Terminated by block */

/*
 * State information used by the queuing code.
 */
static int krt_state;			/* Our current state */
#define	KRT_STATE_NORMAL	0	/* All okay */
#define	KRT_STATE_FULL		1	/* Kernel is full, take action */
#define	KRT_STATE_BLOCKED	2	/* Kernel code can't take any more */

static int krt_terminating;		/* Non-zero when we're terminating */

static time_t krt_q_deferred;		/* Time to retry flushes */
static time_t krt_q_time;		/* Time timer set to expire */
static task_timer *krt_q_tip;		/* Pointer to timeout timer */

static task_job *krt_q_job;		/* Points to active background job */

/*
 * Defer times we use.  Defer individual route installs/deletes by
 * KRT_Q_DEFER_TIME seconds.  Defer retries after failures by
 * KRT_Q_RETRY_TIME seconds.
 */
#define	KRT_Q_DEFER_TIME	4	/* Try again 4 seconds after defer */
#define	KRT_Q_TERMINATE_RETRY	1	/* 1 second after defer when terminating */
#define	KRT_Q_RETRY_TIME	30	/* Retry every 30 seconds after failure */

#define	KRT_TERMINATE_RETRY_COUNT	4	/* Number of flush retries before termination */

/*
 * For forward reference
 */
PROTOTYPE(krt_q_run,
	  static void,
	  (task_job *));

/*
 * Task block index for queue entries.
 */
static block_t krt_q_block_index;

int krt_flash_routes = KRT_FLASH_DEFAULT;	
u_long krt_flash_install_count = KRT_DEF_FLASH_INSTALL_COUNT;
int krt_install_priority = KRT_INSTALL_PRIO_DEFAULT;
u_long krt_install_count = KRT_DEF_INSTALL_COUNT;

#define	KRT_TERMINATE_INSTALL_COUNT	KRT_COUNT_UNLIMITED

#define	KRT_JOB_PRIO_LOW	(TASK_JOB_PRIO_FLASH+1)
#define	KRT_JOB_PRIO_FLASH	(TASK_JOB_PRIO_FLASH)
#define	KRT_JOB_PRIO_HIGH	(TASK_JOB_PRIO_FLASH-1)

static krt_q_entry *krt_flash_q_end;
static int krt_bg_priority;

static const int krt_bg_prio_map[3] = {
    KRT_JOB_PRIO_LOW,		/* low priority */
    KRT_JOB_PRIO_FLASH,		/* flash priority */
    KRT_JOB_PRIO_HIGH		/* high priority */
};



#ifndef	KVM_TYPE_NONE
#ifdef	KVM_WITH_KD
kvm_t	*kd;
#else	/* KVM_WITH_KD */
int	kd;
#endif	/* KVM_WITH_KD */
#endif	/* KVM_TYPE_NONE */

static const bits kernel_option_bits[] = {
    { KRT_OPT_NOCHANGE,		"NoChange" },
    { KRT_OPT_NOFLUSH,		"NoFlush" },
    { KRT_OPT_NOINSTALL,	"NoInstall" },
    { 0 }
};

static const bits kernel_queue_ops[] = {
    { KRTQ_OP_NOP,	"nop" },
    { KRTQ_OP_DELETE,	"delete" },
    { KRTQ_OP_ADD,	"add" },
    { KRTQ_OP_CHANGE,	"change" }
};

const bits kernel_support_bits[] = {
    { KRTS_REJECT,	"Reject" },
    { KRTS_BLACKHOLE,	"Blackhole" },
    { KRTS_VAR_MASK,	"VarMask" },
    { KRTS_HOST,	"Host" },
    { KRTS_MULTIPATH,	"Multipath" },
    { 0 }
};

static const bits kernel_state_bits[] = {
    { KRT_STATE_NORMAL,	"normal" },
    { KRT_STATE_FULL,	"full, retrying periodically" },
    { KRT_STATE_BLOCKED,	"blocked by kernel I/O" },
    { 0 }
};

static const bits kernel_flash_install_bits[] = {
    { KRT_FLASH_INTERFACE,	"interface" },
    { KRT_FLASH_INTERNAL,	"interface+internal" },
    { KRT_FLASH_ALL,		"all" },
    { 0 }
};

static const bits kernel_install_prio_bits[] = {
    { KRT_INSTALL_PRIO_LOW,	"low" },
    { KRT_INSTALL_PRIO_FLASH,	"flash" },
    { KRT_INSTALL_PRIO_HIGH,	"high" },
    { 0 }
};

static const bits kernel_queue_name_bits[] = {
	{ KRTQ_H_INT,	"Interface add/delete/change queue" },
	{ KRTQ_H_DEL,	"High priority deletion queue" },
	{ KRTQ_H_CHNG,	"High priority change queue" },
	{ KRTQ_H_ADD,	"High priority add queue" },
	{ KRTQ_L_DEL,	"Normal priority deletion queue" },
	{ KRTQ_L_CHNG,	"Normal priority change queue" },
	{ KRTQ_L_ADD,	"Normal priority add queue" },
	{ 0 }
};

const bits kernel_trace_types[] = {
    { TR_KRT_SYMBOLS,	"symbols" },
    { TR_KRT_REMNANTS,	"remnants" },
    { TR_KRT_IFLIST,	"interface-list" },
#ifdef	KRT_RT_SOCK
    { TR_KRT_REQUEST,	"request" },
    { TR_KRT_INFO,	"info" },
#endif	/* KRT_RT_SOCK */
    { TR_DETAIL,	"detail packets" },
    { TR_DETAIL_SEND,	"detail send packets" },
    { TR_DETAIL_RECV,	"detail recv packets" },
    { TR_PACKET,		"packets" },
    { TR_PACKET_SEND,	"send packets" },
    { TR_PACKET_RECV,	"recv packets" },
    { TR_DETAIL_1,	"detail route" },
    { TR_DETAIL_SEND_1,	"detail send route" },
    { TR_DETAIL_RECV_1,	"detail recv route" },
    { TR_PACKET_1,	"route" },
    { TR_PACKET_SEND_1,	"send route" },
    { TR_PACKET_RECV_1,	"recv route" },
#ifdef	KRT_RT_SOCK
    { TR_DETAIL_2,	"detail redirect" },
    { TR_DETAIL_SEND_2,	"detail send redirect" },
    { TR_DETAIL_RECV_2,	"detail recv redirect" },
    { TR_PACKET_2,	"redirect" },
    { TR_PACKET_SEND_2,	"send redirect" },
    { TR_PACKET_RECV_2,	"recv redirect" },
    { TR_DETAIL_3,	"detail iflist" },
    { TR_DETAIL_SEND_3,	"detail send iflist" },
    { TR_DETAIL_RECV_3,	"detail recv iflist" },
    { TR_PACKET_3,	"iflist" },
    { TR_PACKET_SEND_3,	"send iflist" },
    { TR_PACKET_RECV_3,	"recv iflist" },
    { TR_DETAIL_4,	"detail info" },
    { TR_DETAIL_SEND_4,	"detail send info" },
    { TR_DETAIL_RECV_4,	"detail recv info" },
    { TR_PACKET_4,	"info" },
    { TR_PACKET_SEND_4,	"send info" },
    { TR_PACKET_RECV_4,	"recv info" },
    { TR_DETAIL_5,	"detail redirect" },
    { TR_DETAIL_SEND_5,	"detail send redirect" },
    { TR_DETAIL_RECV_5,	"detail recv redirect" },
    { TR_PACKET_5,	"redirect" },
    { TR_PACKET_SEND_5,	"send redirect" },
    { TR_PACKET_RECV_5,	"recv redirect" },
#endif	/* KRT_RT_SOCK */
    { 0, NULL }
};


const bits krt_flag_bits[] =
{
    {RTF_UP, "UP"},
    {RTF_GATEWAY, "GW"},
    {RTF_HOST, "HOST"},
#ifdef	RTF_DYNAMIC
    {RTF_DYNAMIC, "DYN"},
#endif	/* RTF_DYNAMIC */
#ifdef	RTF_MODIFIED
    {RTF_MODIFIED, "MOD"},
#endif	/* RTF_MODIFIED */
#ifdef	RTF_DONE
    {RTF_DONE, "DONE"},
#endif	/* RTF_DONE */
#ifdef	RTF_MASK
    {RTF_MASK, "MASK"},
#endif	/* RTF_MASK */
#ifdef	RTF_CLONING
    {RTF_CLONING, "CLONING"},
#endif	/* RTF_CLONING */
#ifdef	RTF_XRESOLVE
    {RTF_XRESOLVE, "XRESOLVE"},
#endif	/* RTF_XRESOLVE */
#ifdef	RTF_LLINFO
    {RTF_LLINFO, "LLINFO"},
#endif	/* RTF_LLINFO */
#ifdef	RTF_REJECT
    {RTF_REJECT, "REJECT"},
#endif	/* RTF_REJECT */
#ifdef	RTF_STATIC
    {RTF_STATIC, "STATIC"},
#endif	/* RTF_STATIC */
#ifdef	RTF_BLACKHOLE
    {RTF_BLACKHOLE, "BLACKHOLE"},
#endif	/* RTF_BLACKHOLE */
#ifdef	RTF_PROTO1
    {RTF_PROTO1, "PROTO1"},
    {RTF_PROTO2, "PROTO2"},
#endif	/* RTF_PROTO1 */
    {0}
};


static const bits krt_support_msgs[] =
{
    {KRTS_REJECT,	"reject routes not supported"},
    {KRTS_BLACKHOLE,	"blackhole routes not supported"},
    {KRTS_HOST,		"host routes not supported"},
    {KRTS_VAR_MASK,	"variable subnet masks not supported"},
    {0}
};



flag_t
krt_flags_to_state __PF1(flags, flag_t)
{
    register flag_t state = 0;

    if (BIT_TEST(flags, RTF_GATEWAY)) {
	BIT_SET(state, RTS_GATEWAY);
    }
#ifdef	RTF_REJECT
    if (BIT_TEST(flags, RTF_REJECT)) {
	BIT_SET(state, RTS_REJECT);
    }
#endif	/* RTF_REJECT */
#ifdef	RTF_STATIC
    if (BIT_TEST(flags, RTF_STATIC)) {
	BIT_SET(state, RTS_STATIC);
    }
#endif	/* RTF_STATIC */
#ifdef	RTF_BLACKHOLE
    if (BIT_TEST(flags, RTF_BLACKHOLE)) {
	BIT_SET(state, RTS_BLACKHOLE);
    }
#endif	/* RTF_BLACKHOLE */

    return state;
}

flag_t
krt_state_to_flags __PF1(state, flag_t)
{
    register flag_t flags = 0;

    if (BIT_TEST(state, RTS_GATEWAY)) {
	BIT_SET(flags, RTF_GATEWAY);
    }
#ifdef	RTF_REJECT
    if (BIT_TEST(state, RTS_REJECT)) {
	BIT_SET(flags, RTF_REJECT);
    }
#endif	/* RTF_REJECT */
#ifdef	RTF_STATIC
    if (BIT_TEST(state, RTS_STATIC)) {
	BIT_SET(flags, RTF_STATIC);
    }
#endif	/* RTF_STATIC */
#ifdef	RTF_BLACKHOLE
    if (BIT_TEST(state, RTS_BLACKHOLE)) {
	BIT_SET(flags, RTF_BLACKHOLE);
    }
#endif	/* RTF_BLACKHOLE */

    return flags;
}


#ifdef	IFT_OTHER
int
krt_type_to_ll __PF1(type, int)
{
    struct if_types {
	int type;
	int ll_type;
    } ;
    static struct if_types tt[] = {
	{ IFT_OTHER,	LL_OTHER },
#ifdef	IFT_1822
	{ IFT_1822,	LL_OTHER },
#endif	/* IFT_1822 */
#ifdef	IFT_HDH1822
	{ IFT_HDH1822,	LL_OTHER },
#endif	/* IFT_HDH1822 */
#ifdef	IFT_X25DDN
	{ IFT_X25DDN,	LL_OTHER },
#endif	/* IFT_X25DDN */
#ifdef	IFT_X25
	{ IFT_X25,	LL_X25 },
#endif	/* IFT_X25 */
#ifdef	IFT_ETHER
	{ IFT_ETHER,	LL_8022 },
#endif	/* IFT_ETHER */
#ifdef	IFT_ISO88023
	{ IFT_ISO88023,	LL_8022 },
#endif	/* IFT_ISO88023 */
#ifdef	IFT_ISO88024
	{ IFT_ISO88024,	LL_8022 },
#endif	/* IFT_ISO88024 */
#ifdef	IFT_ISO88025
	{ IFT_ISO88025,	LL_8022 },
#endif	/* IFT_ISO88025 */
#ifdef	IFT_ISO88026
	{ IFT_ISO88026,	LL_8022 },
#endif	/* IFT_ISO88026 */
#ifdef	IFT_STARLAN
	{ IFT_STARLAN,	LL_8022 },
#endif	/* IFT_STARLAN */
#ifdef	IFT_P10
	{ IFT_P10,	LL_PRONET },
#endif	/* IFT_P10 */
#ifdef	IFT_P80
	{ IFT_P80,	LL_PRONET },
#endif	/* IFT_P80 */
#ifdef	IFT_HY
	{ IFT_HY,	LL_HYPER },
#endif	/* IFT_HY */
#ifdef	IFT_FDDI
	{ IFT_FDDI,	LL_8022 },
#endif	/* IFT_FDDI */
#ifdef	IFT_LAPB
	{ IFT_LAPB,	LL_OTHER },
#endif	/* IFT_LAPB */
#ifdef	IFT_SDLC
	{ IFT_SDLC,	LL_OTHER },
#endif	/* IFT_SDLC */
#ifdef	IFT_T1
	{ IFT_T1,	LL_OTHER },
#endif	/* IFT_T1 */
#ifdef	IFT_CEPT
	{ IFT_CEPT,	LL_OTHER },
#endif	/* IFT_CEPT */
#ifdef	IFT_ISDNBASIC
	{ IFT_ISDNBASIC,	LL_OTHER },
#endif	/* IFT_ISDNBASIC */
#ifdef	IFT_ISDNPRIMARY
	{ IFT_ISDNPRIMARY,	LL_OTHER },
#endif	/* IFT_ISDNPRIMARY */
#ifdef	IFT_PTPSERIAL
	{ IFT_PTPSERIAL,	LL_OTHER },
#endif	/* IFT_PTPSERIAL */
#ifdef	IFT_LOOP
	{ IFT_LOOP,	LL_OTHER },
#endif	/* IFT_LOOP */
#ifdef	IFT_EON
	{ IFT_EON,	LL_OTHER },
#endif	/* IFT_EON */
#ifdef	IFT_XETHER
	{ IFT_XETHER,	LL_8022 },
#endif	/* IFT_XETHER */
#ifdef	IFT_NSIP
	{ IFT_NSIP,	LL_OTHER },
#endif	/* IFT_NSIP */
#ifdef	IFT_SLIP
	{ IFT_SLIP,	LL_OTHER },
#endif	/* IFT_SLIP */
#ifdef	IFT_ULTRA
	{ IFT_ULTRA,	LL_OTHER },
#endif	/* IFT_ULTRA */
#ifdef	IFT_DS3
	{ IFT_DS3,	LL_OTHER },
#endif	/* IFT_DS3 */
#ifdef	IFT_SIP
	{ IFT_SIP,	LL_OTHER },
#endif	/* IFT_SIP */
#ifdef	IFT_FRELAY
	{ IFT_FRELAY,	LL_OTHER },
#endif	/* IFT_FRELAY */
	{ 0, LL_OTHER }
    };
    register struct if_types *tp = tt;

    do {
	if (tp->type == type) {
	    break;
	}
    } while ((++tp)->type) ;

    return tp->ll_type;
}
#endif	/* IFT_OTHER */


/*
 *		Convert kernel interface flags to gated interface flags
 */
flag_t
krt_if_flags __PF1(k_flags, int)
{
    flag_t state = 0;

    if (BIT_TEST(k_flags, IFF_UP)) {
	BIT_SET(state, IFS_UP);
    }
    if (BIT_TEST(k_flags, IFF_BROADCAST)) {
	BIT_SET(state, IFS_BROADCAST);
    }
    if (BIT_TEST(k_flags, IFF_POINTOPOINT)) {
	BIT_SET(state, IFS_POINTOPOINT);
    }
#ifdef	IFF_LOOPBACK
    if (BIT_TEST(k_flags, IFF_LOOPBACK)) {
	BIT_SET(state, IFS_LOOPBACK);
#ifdef	_IBMR2
	if (BIT_TEST(state, IFS_BROADCAST)) {
	    /* Some AIX brain damage */

	    BIT_RESET(state, IFS_BROADCAST);
	}
#endif	/* _IBMR2 */
    }
#endif	/* IFF_LOOPBACK */

#ifdef	IFF_MULTICAST
    if (BIT_TEST(k_flags, IFF_MULTICAST)) {
	BIT_SET(state, IFS_MULTICAST);
    }
#endif	/* IFF_MULTICAST */

#ifdef	IFF_ALLMULTI
    if (BIT_TEST(k_flags, IFF_ALLMULTI)) {
	BIT_SET(state, IFS_ALLMULTI);
    }
#endif	/* IFF_ALLMULTI */

#ifdef	IFF_SIMPLEX
    if (BIT_TEST(k_flags, IFF_SIMPLEX)) {
	BIT_SET(state, IFS_SIMPLEX);
    }
#endif	/* IFF_SIMPLEX */

    return state;
}


/*
 * XXX shouldn't be here
 */
sockaddr_un *krt_make_router __PF2(af, int,
				   state, flag_t)
{
    switch (af) {
#ifdef	PROTO_INET
    case AF_INET:
	if (BIT_TEST(state, RTS_REJECT)) {
	    return inet_addr_reject;
	} else if (BIT_TEST(state, RTS_BLACKHOLE)) {
	    return inet_addr_blackhole;
	}
	break;
#endif	/* PROTO_INET */

    default:
	break;
    }
    
    return (sockaddr_un *) 0;
}



void
krt_trace __PF9(tp, task *,
		direction, const char *,
		type, const char *,
		dest, sockaddr_un *,
		mask, sockaddr_un *,
		router, sockaddr_un *,
		flags, flag_t,
		error, const char *,
		pri, int)
{
    if (mask) {
	tracef("KRT %-4s %-6s %-15A mask %-15A router %-15A flags <%s>%x",
	       direction,
	       type,
	       dest,
	       mask,
	       router,
	       trace_bits(krt_flag_bits, flags),
	       flags);
    } else {
	tracef("KRT %-4s %-6s %-15A router %-15A flags <%s>%x",
	       direction,
	       type,
	       dest,
	       router,
	       trace_bits(krt_flag_bits, flags),
	       flags);
    }

    if (error) {
	tracef(": %s",
	       error);
    }
    if (pri) {
	trace_log_tp(tp,
		     TRC_NOSTAMP,
		     pri,
		     (NULL));
    } else {
	trace_only_tp(tp,
		      TRC_NOSTAMP,
		      (NULL));
    }
}


static void
krt_cleanup __PF1(tp, task *)
{
    if (inet_addr_reject) {
	sockfree(inet_addr_reject);
    }
    if (inet_addr_blackhole) {
	sockfree(inet_addr_blackhole);
    }
#ifdef	PROTO_INET
    BIT_RESET(krt_rt_support, KRTS_REJECT|KRTS_BLACKHOLE);
#endif	/* PROTO_INET */
    trace_freeup(tp->task_trace);
    trace_freeup(kernel_trace_options);
}

/* Final cleanup before we go away */
static void
krt_exit __PF1(tp, task *)
{
#ifndef	KVM_TYPE_NONE
    if (kd && (KVM_CLOSE(kd) < 0)) {
	trace_log_tp(tp,
		     0,
		     LOG_ERR,
		     ("kvm_terminate: %s",
		      KVM_GETERR(kd, "kvm_close error")));
    }
#endif	/* KVM_TYPE_NONE */

    krt_cleanup(tp);

    task_delete(tp);
}


/**/

/*
 * krt_kernel_rt - given an rth, returns the route currently installed
 *		   in the kernel for this destination.
 */
krt_parms *
krt_kernel_rt __PF1(rth, rt_head *)
{
    rt_entry *rt;
    krt_q_entry *kqp;
    rt_changes *rtc;
    u_int rtbit = krt_task->task_rtbit;
    static krt_parms myparms;

    /*
     * See if our bit is set somewhere.  If not there is no
     * kernel route.
     */
    rt = (rt_entry *) 0;
    if (rth->rth_n_announce) {
	RT_ALLRT(rt, rth) {
	    if (rtbit_isset(rt, rtbit)) {
		break;
	    }
	} RT_ALLRT_END(rt, rth) ;
    }
    if (!rt) {
	return (krt_parms *) 0;
    }

    /*
     * Fetch the queue entry.  If there is one we can copy the
     * state out of there.  Otherwise we'll need to determine
     * the state from the route.
     */
    KRT_GET_Q_ENT(rth, rtbit, kqp);
    if (kqp) {
	if (kqp->krtq_op == KRTQ_OP_ADD) {
	    return (krt_parms *) 0;
	}
	myparms.krtp_protocol = kqp->krtq_proto;
	myparms.krtp_state = kqp->krtq_state;
#ifdef	IP_MULTICAST_ROUTING
 	myparms.krtp_preference = kqp->krtq_preference;
 	myparms.krtp_metric = kqp->krtq_metric;
#endif	/* IP_MULTICAST_ROUTING */
	myparms.krtp_n_gw = kqp->krtq_n_gw;
	myparms.krtp_routers = &(kqp->krtq_routers[0]);
	myparms.krtp_ifaps = &(kqp->krtq_ifaps[0]);
    } else {
	myparms.krtp_protocol = rt->rt_gwp->gw_proto;
	myparms.krtp_state = rt->rt_state & KRT_RTS_BITS;
#ifdef	IP_MULTICAST_ROUTING
 	myparms.krtp_preference = rt->rt_preference;
 	myparms.krtp_metric = rt->rt_metric;
#endif	/* IP_MULTICAST_ROUTING */
	if (rt->rt_state & KRT_RTS_NO_NEXTHOP) {
	    myparms.krtp_n_gw = 0;
	} else if (rt == rth->rth_last_active
	    && (rtc = rth->rth_changes) != (rt_changes *) 0
	    && BIT_TEST(rtc->rtc_flags, RTCF_NEXTHOP)) {
#if	RT_N_MULTIPATH > 1
	    if (BIT_TEST(krt_rt_support, KRTS_MULTIPATH)) {
		myparms.krtp_n_gw = rtc->rtc_n_gw;
		myparms.krtp_routers = &(rtc->rtc_routers[0]);
		myparms.krtp_ifaps = &(rtc->rtc_ifaps[0]);
	    } else {
#endif	/* RT_N_MULTIPATH > 1 */
		myparms.krtp_n_gw = 1;
		myparms.krtp_routers = &RTC_ROUTER(rtc);
		myparms.krtp_ifaps = &RTC_IFAP(rtc);
#if	RT_N_MULTIPATH > 1
	    }
#endif	/* RT_N_MULTIPATH > 1 */
	} else {
#if	RT_N_MULTIPATH > 1
	    if (BIT_TEST(krt_rt_support, KRTS_MULTIPATH)) {
		myparms.krtp_n_gw = rt->rt_n_gw;
		myparms.krtp_routers = &(rt->rt_routers[0]);
		myparms.krtp_ifaps = &(rt->rt_ifaps[0]);
	    } else {
#endif	/* RT_N_MULTIPATH > 1 */
		myparms.krtp_n_gw = 1;
		myparms.krtp_routers = &RT_ROUTER(rt);
		myparms.krtp_ifaps = &RT_IFAP(rt);
#if	RT_N_MULTIPATH > 1
	    }
#endif	/* RT_N_MULTIPATH > 1 */
	}
    }

    return &myparms;
}


/*
 * krt_make_rtactive - install a temporary active route which will
 *		       be blown away once we diddle with it.
 */
static rt_entry *
krt_make_rtactive __PF2(dest, sockaddr_un *,
		       destmask, sockaddr_un *)
{
    rt_parms rtp;

    rtp.rtp_dest = dest;
    rtp.rtp_dest_mask = destmask;
    rtp.rtp_n_gw = 0;
    rtp.rtp_gwp = krt_gwp_temp;
    rtp.rtp_metric = rtp.rtp_metric2 = (metric_t) 0;
    rtp.rtp_tag = (tag_t) 0;
    rtp.rtp_state = RTS_GATEWAY|RTS_NOADVISE;
    rtp.rtp_preference = RTPREF_KERNEL_TEMP;
    rtp.rtp_preference2 = (pref_t) 0;
    rtp.rtp_rtd = (void_t) 0;
#ifdef	PROTO_ASPATHS
    rtp.rtp_asp = (as_path *) 0;
#endif	/* PROTO_ASPATHS */

    return rt_add(&rtp);
}
/*
 * Verify that the specified address is OK
 * Returns:
 */

int
krt_addrcheck __PF1(rtp, rt_parms *)
{
    if (!rtp->rtp_dest
	|| !rtp->rtp_dest_mask) {
	/* Probably an unsupported address family */

	return KRT_ADDR_IGNORE;
    }
	
    switch (socktype(rtp->rtp_dest)) {
#ifdef	PROTO_INET
    case AF_INET:
	switch (inet_class_of_byte((byte *) &sock2in(rtp->rtp_dest))) {
	case INET_CLASSC_A:
	case INET_CLASSC_B:
	case INET_CLASSC_C:
	case INET_CLASSC_E:
	case INET_CLASSC_C_SHARP:
	    if (!rtp->rtp_router) {
		return KRT_ADDR_IGNORE;
	    }
	    break;

#ifdef	IP_MULTICAST
	case INET_CLASSC_MULTICAST:
#ifdef	KRT_IPMULTI_RTSOCK
	    return KRT_ADDR_MC;
#else	/* KRT_IPMULTI_RTSOCK */
	    if (rtp->rtp_dest_mask == inet_mask_host) {
		/* This is a default interface specification for this group */

		return KRT_ADDR_MC;
	    } else {
		/* This is the default multicast specification */
		
		return KRT_ADDR_IGNORE;
	    }
#endif	/* KRT_IPMULTI_RTSOCK */
#endif	/* IP_MULTICAST */

	default:
	    /* Bogus, delete it */
	    return KRT_ADDR_BOGUS;
	}
	break;
#endif	/* PROTO_INET */

#ifdef	PROTO_ISO
    case AF_ISO:
 	if (sockaddrcmp(rtp->rtp_dest, sockbuild_iso(iso_default_prefix, ISO_MAXADDRLEN))) {
	    /* yank routes to an all zeros (default) prefix */

	    return KRT_ADDR_BOGUS;
 	}
	if (!rtp->rtp_router) {
	    return KRT_ADDR_IGNORE;
	}
	break;
#endif	/* PROTO_ISO */

    default:
	/* Unknown address family */
	return KRT_ADDR_IGNORE;
    }

    return KRT_ADDR_OK;
}


/* Common routine to add kernel routes to the gated routing table */
struct krt_delq_entry {
    struct krt_delq_entry *kd_forw;
    struct krt_delq_entry *kd_back;
    sockaddr_un *kd_dest;
    sockaddr_un *kd_mask;
    flag_t kd_state;
    int kd_n_gw;
    sockaddr_un *kd_routers[RT_N_MULTIPATH];
};

static struct krt_delq_entry krt_delq_queue = { &krt_delq_queue, &krt_delq_queue };
static block_t krt_delq_index;


void
krt_delq_add __PF1(rtp, rt_parms *)
{
    register int i;
    struct krt_delq_entry *kdp = (struct krt_delq_entry *) task_block_alloc(krt_delq_index);

    kdp->kd_dest = sockdup(rtp->rtp_dest);
    kdp->kd_mask = rtp->rtp_dest_mask;
    kdp->kd_state = rtp->rtp_state;
    kdp->kd_n_gw = rtp->rtp_n_gw;
    for (i = 0; i < rtp->rtp_n_gw; i++) {
	kdp->kd_routers[i] = sockdup(rtp->rtp_routers[i]);
    }
    INSQUE(kdp, krt_delq_queue.kd_back);
}


/* Process any queued deletions */
static void
krt_delq_process __PF1(tp, task *)
{
    struct krt_delq_entry *kdp = krt_delq_queue.kd_forw;

    while (kdp != &krt_delq_queue) {
	struct krt_delq_entry *c_kdp = kdp;
	register int i;

	/* Point at the next one */
	kdp = kdp->kd_forw;

	/* Delete the route */
	krt_delete_dst(tp,
		       c_kdp->kd_dest,
		       c_kdp->kd_mask,
		       RTPROTO_KERNEL,
		       c_kdp->kd_state,
		       c_kdp->kd_n_gw,
		       c_kdp->kd_routers,
		       (if_addr **) 0);

	/* Remove this one from the queue */
	REMQUE(c_kdp);

	/* And release it */
	sockfree(c_kdp->kd_dest);
	for (i = 0; i < c_kdp->kd_n_gw; i++) {
	    sockfree(c_kdp->kd_routers[i]);
	}
	task_block_free(krt_delq_index, (void_t) c_kdp);
    }
}


/*
 * Returns:
 *	0	Delete this route
 *	1	OK
 */
const char *
krt_rtadd __PF2(rtp, rt_parms *,
		flags, flag_t)
{
    rt_entry *rt;
    const char *errmsg = (const char *) 0;

    /* Indicate it is installed in the kernel */
    krt_n_routes++;

    /* Make sure the mask is contiguous */
    if (!mask_contig(rtp->rtp_dest_mask)) {
	/* Danger Will Robinson! */

	errmsg = "queuing delete for non-contiguous mask";
	goto Delete;
    }
    
    switch (socktype(rtp->rtp_dest)) {
#ifdef	PROTO_INET
    case AF_INET:
#endif	/* PROTO_INET */
#ifdef	PROTO_ISO
    case AF_ISO:
#endif	/* PROTO_ISO */
#if	defined(PROTO_INET) || defined(PROTO_ISO)
	if (BIT_TEST(flags, RTF_REDIRECT)) {
	    /* Delete any redirects */

 	    errmsg = "queuing delete for redirect";
 	    goto Delete;
	} else {
	    if (BIT_TEST(rtp->rtp_state, RTS_STATIC)) {
		/* Intended to be a static route */

		rtp->rtp_gwp = krt_gwp;
		rtp->rtp_preference = RTPREF_KERNEL;
		BIT_SET(rtp->rtp_state, RTS_RETAIN);
	    } else {
		/* Assume left over from a previous run */

		rtp->rtp_gwp = krt_gwp_remnant;
		rtp->rtp_preference = RTPREF_KERNEL_REMNANT;
		BIT_SET(rtp->rtp_state, RTS_NOADVISE);
	    }
	}
	break;
#endif	/* defined(PROTO_INET) || defined(PROTO_ISO) */

    default:
	assert(FALSE);
	break;
    }

    /*
     *	If Kernel route already exists, delete this one, the kernel uses the
     *	first one
     */
    rt = rt_locate(rtp->rtp_state,
		   rtp->rtp_dest,
		   rtp->rtp_dest_mask,
		   RTPROTO_BIT(rtp->rtp_gwp->gw_proto));
    if (rt && !BIT_TEST(rt->rt_state, RTS_DELETE)) {
	errmsg = "queuing delete for duplicate entry";
	goto Delete;
    }
    /*
     *	If there was a problem adding the route, delete the kernel route
     */
    rt = rt_add(rtp);
    if (!rt) {
	/* Could not add */

	errmsg = "queueing delete for rt_add() failure";
	goto Delete;
    } 

    /* Indicate it is installed in the kernel */
    krt_rth_reset(rt->rt_head, rt->rt_state, rt->rt_n_gw, rt->rt_routers, rt->rt_ifaps);

    if (rt != rt->rt_active) {
	/* Already a better route (probably an interface) */
	/* Delete our route */

	rt_delete(rt);
    }

    return errmsg;

 Delete:
    krt_delq_add(rtp);
    return errmsg;
}

#if	!defined(KRT_RTREAD_KINFO)
/* Front end for krt_rtadd() on systems that do not use routing socket message format */
/* when reading routes from the kernel */
void
krt_rtread_add __PF5(tp, task *,
		     rtp, rt_parms *,
		     krt_flags, flag_t,
		     bad, int,
		     type, const char *)
{
    const char *errmsg = (char *) 0;
    int pri = 0;

    if (bad) {
	errmsg = "ignoring wrong type";
	pri = LOG_INFO;
	goto Trace;
    }

    if (!rtp->rtp_dest_mask) {
	/* Non-contiguous mask */

	errmsg = "deleting non-contiguous mask";
	pri = LOG_WARNING;
	krt_delq_add(rtp);
	goto Trace;
    }

    if (BIT_TEST(rtp->rtp_state, RTS_GATEWAY)) {
	BIT_SET(rtp->rtp_state, RTS_EXTERIOR);
    } else {
	BIT_SET(rtp->rtp_state, RTS_INTERIOR);
    }

    switch (krt_addrcheck(rtp)) {
    case KRT_ADDR_OK:
	/* Address is OK */
	break;

    case KRT_ADDR_IGNORE:
	/* Ignore it */
	errmsg = "ignoring";
	pri = LOG_INFO;
	goto Trace;

    case KRT_ADDR_BOGUS:
	/* Delete it */
	errmsg = "deleting bogus";
	pri = LOG_WARNING;
	krt_delq_add(rtp);
	goto Trace;

#ifdef	IP_MULTICAST
    case KRT_ADDR_MC:
	/* Multicast specification */
	if (krt_multicast_install(rtp->rtp_dest, rtp->rtp_router)) {
	    errmsg = "deleting multicast";
	    krt_delq_add(rtp);
	    goto Trace;
	}
	errmsg = "ignoring multicast";
	pri = LOG_INFO;
	goto Trace;
#endif	/* IP_MULTICAST */
    }

    /* Add route to our routing table */
    errmsg = krt_rtadd(rtp, krt_flags);
    if (errmsg) {
 	/* It was deleted */
	pri = LOG_WARNING;
    }

 Trace:
    if (errmsg
	|| TRACE_TP(tp, TR_KRT_REMNANTS)) {
	/* Always trace in detail */

	krt_trace(tp,
		  "READ",
		  type,
		  rtp->rtp_dest,
		  rtp->rtp_dest_mask,
		  rtp->rtp_router,
		  (flag_t) krt_flags,
		  errmsg,
		  pri);
    }
}
#endif	/* KRT_RTREAD_KINFO */


/*
 * krt_remnant_delete - delete overridden remnants on the remnant
 *			list from the routing table.
 */
static void
krt_remnant_delete __PF0(void)
{
    krt_remnant_rt *remp, *remp_next;

    rt_open(krt_task);
    remp_next = krt_remnant_list;
    while ((remp = remp_next)) {
	remp_next = remp->krt_rem_next;
	remp->krt_rem_rt->rt_data = (void_t) 0;
	rt_delete(remp->krt_rem_rt);
	KRT_REMNANT_RT_FREE(remp);
    }
    krt_remnant_list = (krt_remnant_rt *) 0;
    rt_close(krt_task, krt_gwp_remnant, 0, NULL);
}


/*
 * krt_remnant_remove - task job to remove deletion-deferred kernel remnants
 */
static void
krt_remnant_remove __PF1(jp, task_job *)
{
    if (krt_remnant_list) {
	krt_remnant_delete();
    }
    krt_remnant_job = (task_job *) 0;
}


/*
 * krt_q_flush - flush entries from krt route queue into kernel
 */
static int
krt_q_flush __PF5(startp, krt_q_entry *,
		  endp, krt_q_entry *,
		  isflash, int,
		  delete_only, int,
		  todo, u_long *)
{
    krt_q_entry *kqp, *kqp_start;
    rt_entry *rt;
    rt_head *rth;
    krt_parms okrt, nkrt;
    krt_parms *okrtp, *nkrtp;
    int res;
    u_int rtbit = krt_task->task_rtbit;
    int alldone = 0;
    int deleting_only = 0;
    time_t deferred = 0;

    /*
     * Open the routing table, we be dicking around with tsi's
     * and playing with route bits.  Also assume there'll be
     * no crisis.
     */
    krt_state = KRT_STATE_NORMAL;
    krt_last_q_run = time_sec;

    /*
     * Loop around doing the work.
     */
    kqp_start = startp;
    while (!todo || *todo > 0) {
	kqp = kqp_start->krtq_forw;

	/*
	 * See if we've reached the end.  If so, we're done.
	 */
	if (kqp == startp || kqp == endp) {
	    alldone = 1;
	    break;
	}

	/*
	 * If this is a bookkeeping entry advance past it
	 */
	if (kqp->krtq_op == KRTQ_OP_NOP) {
	    kqp_start = kqp;
	    continue;
	}

	/*
	 * If this is a deferred entry, note the expiry
	 * time and advance past it.
	 */
	if (kqp->krtq_time != (time_t) 0) {
	    if (kqp->krtq_time > time_sec) {
		if (deferred == 0 || kqp->krtq_time < deferred) {
		    deferred = kqp->krtq_time;
		}
		kqp_start = kqp;
		continue;
	    }
	    KRT_UNDEFER(kqp);
	}

	/*
	 * Okay, we've may have something to do here.  Determine what we
	 * need to tell the change routine.
	 */
	if (kqp->krtq_op == KRTQ_OP_DELETE) {
	    okrtp = &okrt;
	    nkrtp = (krt_parms *) 0;
	} else if (kqp->krtq_op == KRTQ_OP_ADD) {
	    if (delete_only) {
		kqp_start = kqp;
		continue;
	    }
	    okrtp = (krt_parms *) 0;
	    nkrtp = &nkrt;
	} else if (kqp->krtq_op == KRTQ_OP_CHANGE) {
	    okrtp = &okrt;
	    if (delete_only) {
		deleting_only = 1;
		nkrtp = (krt_parms *) 0;
	    } else {
		nkrtp = &nkrt;
	    }
	} else {
	    assert(FALSE);
	    return 0;		/* XXX For mouthy compilers */
	}

	/*
	 * Fetch route and route head pointers, we'll need them later.
	 */
	rt = kqp->krtq_rt;
	rth = rt->rt_head;

	/*
	 * Fill in the structures.  The old route data comes out of
	 * the queue entry, the new route data is gathered from the
	 * route being pointed to
	 */
	if (nkrtp) {
	    /*
	     * If the route has changed and we're outside the
	     * flash routine we may want to defer this until
	     * we are flashed.
	     */
	    if (!isflash && BIT_TEST(rth->rth_state, RTS_ONLIST)) {
		kqp_start = kqp;
		continue;
	    }

	    nkrt.krtp_protocol = rt->rt_gwp->gw_proto;
	    nkrt.krtp_state = rt->rt_state & KRT_RTS_BITS;
	    if (rt->rt_state & KRT_RTS_NO_NEXTHOP) {
		nkrt.krtp_n_gw = 0;
#if	RT_N_MULTIPATH > 1
	    } else if (BIT_TEST(krt_rt_support, KRTS_MULTIPATH)) {
		nkrt.krtp_n_gw = rt->rt_n_gw;
		nkrt.krtp_routers = &(rt->rt_routers[0]);
		nkrt.krtp_ifaps = &(rt->rt_ifaps[0]);
#endif	/* RT_N_MULTIPATH > 1 */
	    } else {
		nkrt.krtp_n_gw = 1;
		nkrt.krtp_routers = &RT_ROUTER(rt);
		nkrt.krtp_ifaps = &RT_IFAP(rt);
	    }
	}

	if (okrtp) {
	    okrt.krtp_protocol = kqp->krtq_proto;
	    okrt.krtp_state = kqp->krtq_state;
	    if ((okrt.krtp_n_gw = kqp->krtq_n_gw) > 0) {
		okrt.krtp_routers = &(kqp->krtq_routers[0]);
		okrt.krtp_ifaps = &(kqp->krtq_ifaps[0]);
	    }
	}

	/*
	 * We've got the route information tabulated, call the
	 * change routine to do the work.
	 */
	if (BIT_TEST(krt_options, KRT_OPT_NOCHANGE) && okrtp && nkrtp) {
	    /*
	     * A change, when we aren't allowed to do changes.  Do a
	     * delete/add instead.  If the delete works but the add
	     * fails, make sure the partial operation is noted.
	     */
	    res = krt_change(krt_task,
			     rth->rth_dest,
			     rth->rth_dest_mask,
			     okrtp,
			     (krt_parms *) 0);
	    if (res == KRT_OP_SUCCESS) {
		if (todo && *todo > 1) {
		    (*todo)--;
		}
		res = krt_change(krt_task,
				 rth->rth_dest,
				 rth->rth_dest_mask,
				 (krt_parms *) 0,
				 nkrtp);
		if (res != KRT_OP_SUCCESS) {
		    BIT_SET(res, KRT_OP_PARTIAL);
		}
	    }
	} else {
	    res = krt_change(krt_task,
			     rth->rth_dest,
			     rth->rth_dest_mask,
			     okrtp,
			     nkrtp);
	}
	if (todo) {
	    (*todo)--;
	}

	/*
	 * Okdk.  If we had success and we weren't intentionally doing a
	 * partial op we can blow away the queue entry entirely.  Otherwise
	 * we'll need to take some action.
	 */
	if (res == KRT_OP_SUCCESS && !deleting_only) {
	    KRT_CLR_Q_ENT(rth, rtbit);
	    KRT_DEQUEUE(kqp);

	    if (okrtp) {			/* i.e. not an add */
		register int i = kqp->krtq_n_gw;

		while (--i >= 0) {
		    sockfree(kqp->krtq_routers[i]);
		    IFA_FREE(kqp->krtq_ifaps[i]);
		}
	    }

	    if (!nkrtp) {			/* i.e. a delete */
		rtbit_reset(rt, rtbit);
	    }

	    task_block_free(krt_q_block_index, (void_t) kqp);
	} else {
	    if (BIT_TEST(res, KRT_OP_PARTIAL)
	      || (res == KRT_OP_SUCCESS && deleting_only)) {
		register int i = kqp->krtq_n_gw;
		int which;

		/*
		 * Delete succeeded but add failed.  Change the
		 * queue entry from a change to an add.
		 */
		KRT_DEQUEUE(kqp);

		while (--i >= 0) {
		    sockfree(kqp->krtq_routers[i]);
		    IFA_FREE(kqp->krtq_ifaps[i]);
		}

		kqp->krtq_op = KRTQ_OP_ADD;
		if (KRT_IS_HIPRIO(kqp->krtq_rt)) {
		    if (BIT_TEST(kqp->krtq_rt->rt_state, RTS_GATEWAY)) {
			which = KRTQ_H_ADD;
		    } else {
			which = KRTQ_H_INT;
		    }
		} else {
		    which = KRTQ_L_ADD;
		}
		KRT_ENQUEUE(which, kqp);

		if (deleting_only) {
		    deleting_only = 0;
		} else {
		    BIT_RESET(res, KRT_OP_PARTIAL);
		}
	    }

	    /*
	     * If this is a defer, just defer this route for later.
	     * If we failed, mark the fact that we failed and return,
	     * we've done what we can.
	     */
	    if (res == KRT_OP_DEFER) {
		KRT_DEFER(kqp, time_sec + KRT_Q_DEFER_TIME);
		if (deferred == (time_t) 0 || deferred > kqp->krtq_time) {
		    deferred = kqp->krtq_time;
		}
	    } else if (res != KRT_OP_SUCCESS) {
		if (res == KRT_OP_FULL) {
		    krt_state = KRT_STATE_FULL;
		} else {
		    krt_state = KRT_STATE_BLOCKED;
		}
		break;
	    }
	}
    }

    /*
     * Update our estimate of when we need to run to install deferred routes.
     */
    if (deferred) {
	if (!krt_q_deferred || krt_q_deferred > deferred) {
	    krt_q_deferred = deferred;
	}
    }

    /*
     * Return an indication of what has happened.
     */
    if (alldone) {
	return (KRT_Q_FLUSH_DONE);
    }
    if (krt_state == KRT_STATE_FULL) {
	return (KRT_Q_FLUSH_FULL);
    }
    if (krt_state == KRT_STATE_BLOCKED) {
	return (KRT_Q_FLUSH_BLOCKED);
    }
    return (KRT_Q_FLUSH_COUNT);
}


/*
 * krt_q_schedule - (re)schedule a queue run if we need it
 */
static void
krt_q_schedule __PF1(try_now, int)
{
    time_t next = (time_t) 0;

    switch (krt_state) {
    case KRT_STATE_BLOCKED:
	/*
	 * Here the kernel dependent stuff has told us not to
	 * send him any more until he calls krt_q_unblock().
	 * Remove any job and make sure no timer is running.
	 */
	next = (time_t) 0;
	if (krt_terminating) {
	    krt_terminating = 1;	/* Don't let terminate count us out */
	}
	break;

    case KRT_STATE_FULL:
	/*
	 * Here we're in crisis mode.  Don't bother trying to
	 * install the remaining routes in the kernel unless he's
	 * told us to try, just start a timer for a retry some
	 * time in the future if not.
	 */
	if (krt_terminating) {
	    next = (time_t) 0;
	    if (try_now) {
		if (BIT_TEST(krt_options, KRT_OPT_NOFLUSH)) {
		    if ((krt_n_queued
		      - KRT_N_QUEUED(KRTQ_H_DEL)
		      - KRT_N_QUEUED(KRTQ_L_DEL)) > krt_n_deferred) {
			next = time_sec;
		    }
		} else if (krt_n_queued > krt_n_deferred) {
		    next = time_sec;
		}
	    }
	    break;
	}
	if (try_now && krt_n_queued > krt_n_deferred) {
	    next = time_sec;
	} else {
	    next = krt_last_q_run + KRT_Q_DEFER_TIME;
	}
	break;

    case KRT_STATE_NORMAL:
	/*
	 * See if we have anything to do immediately.  If
	 * so make sure we have a job queued for it.  Otherwise,
	 * if we have anything deferred make sure we have a
	 * timer running for that.  Otherwise don't do anything.
	 */
	if (krt_terminating && BIT_TEST(krt_options, KRT_OPT_NOFLUSH)) {
	    if ((krt_n_queued - KRT_N_QUEUED(KRTQ_H_DEL)
	      - KRT_N_QUEUED(KRTQ_L_DEL)) > krt_n_deferred) {
		next = time_sec;
	    } else if (krt_n_deferred > 0) {
		assert(krt_q_deferred != (time_t) 0);
		next = krt_q_deferred;
		if (next > (time_sec + KRT_Q_TERMINATE_RETRY)) {
		    next = time_sec + KRT_Q_TERMINATE_RETRY;
		}
	    } else {
		next = (time_t) 0;
	    }
	} else if (krt_n_queued > krt_n_deferred) {
	    next = time_sec;
	} else if (krt_n_deferred > 0) {
	    assert(krt_q_deferred != (time_t) 0);
	    next = krt_q_deferred;
	    if (krt_terminating
	      && next > (time_sec + KRT_Q_TERMINATE_RETRY)) {
		next = time_sec + KRT_Q_TERMINATE_RETRY;
	    }
	} else {
	    next = (time_t) 0;
	}
	break;
    }

    if (next == (time_t) 0 || next > time_sec) {
	if (krt_q_job) {
	    task_job_delete(krt_q_job);
	    krt_q_job = (task_job *) 0;
	    assert(krt_q_time == (time_t) 0);
	}
	if (next == (time_t) 0) {
	    if (krt_q_time != (time_t) 0) {
		task_timer_reset(krt_q_tip);
		krt_q_time = (time_t) 0;
	    }
	} else if (krt_q_time != next) {
	    /*
	     * There is no timer running, or the
	     * expiry is too far off.  Set a new
	     * expiry time.
	     */
	    task_timer_set(krt_q_tip,
			   (time_t) 0,
			   (next - time_sec));
	    krt_q_time = next;
	}
    } else {
	if (krt_q_time != (time_t) 0) {
	    task_timer_reset(krt_q_tip);
	    krt_q_time = (time_t) 0;
	    assert(!krt_q_job);
	}
	if (!krt_q_job) {
	    krt_q_job = task_job_create(krt_task,
					krt_bg_priority,
					"Route Install",
					krt_q_run,
					(void_t) 0);
	}
    }

    if (krt_terminating) {
	if (next == (time_t) 0 || krt_terminating >= KRT_TERMINATE_RETRY_COUNT) {
	    /* XXX log if krt_terminating >= KRT_TERIMATE_RETRY_COUNT? */
	    krt_exit(krt_task);
	}
    }
}


/*
 * krt_unblock - used by kernel dependent code to tell us it is
 *		 ready to receive routes again.  Change the state
 *		 to normal and reschedule a queue run.
 */
void
krt_unblock __PF0(void)
{
    if (krt_state == KRT_STATE_BLOCKED) {
	/* XXX log the state change? */
	krt_state = KRT_STATE_NORMAL;
	krt_q_schedule(FALSE);
    }
}


/*
 * krt_q_run - run the queue
 */
static void
krt_q_run __PF1(jp, task_job *)
{
    u_long todo = (krt_terminating ? KRT_TERMINATE_INSTALL_COUNT : krt_install_count);
    int rc;
    int end_rc;

    assert(krt_state != KRT_STATE_BLOCKED);
    assert(krt_q_time == (time_t) 0);

    if (time_sec >= krt_q_deferred) {
	krt_q_deferred = (time_t) 0;
    }

    /*
     * Open the routing table and tell the kernel independent
     * code we're going to be sending routes down.
     */
    rc = krt_change_start(krt_task);
    if (rc != KRT_OP_SUCCESS) {
	switch (rc) {
	case KRT_OP_FULL:
	    krt_state = KRT_STATE_FULL;
	    break;
	case KRT_OP_BLOCKED:
	    krt_state = KRT_STATE_BLOCKED;
	    break;
	default:
	    /* XXX log rc? */
	    assert(FALSE);
	}
	krt_last_q_run = time_sec;
	krt_q_schedule(FALSE);
	return;
    }
    rt_open(krt_task);

    if (krt_terminating && BIT_TEST(krt_options, KRT_OPT_NOFLUSH)) {
	switch (krt_state) {
	default:
	    /*
	     * Non-crisis mode.  Do interface route changes, and adds
	     * and changes.  Don't bother doing deletes.
	     */
	    if (KRT_N_QUEUED(KRTQ_H_INT) > 0) {
		rc = krt_q_flush(KRT_Q_START(KRTQ_H_INT),
				 KRT_Q_END(KRTQ_H_INT),
				 FALSE,
				 FALSE,
				 &todo);
	    } else {
		rc = KRT_Q_FLUSH_DONE;
	    }
	    if (rc == KRT_Q_FLUSH_DONE) {
		rc = krt_q_flush(KRT_Q_START(KRTQ_H_CHNG),
				 KRT_Q_END(KRTQ_H_ADD),
				 FALSE,
				 FALSE,
				 &todo);
		if (rc == KRT_Q_FLUSH_DONE) {
		    rc = krt_q_flush(KRT_Q_START(KRTQ_L_CHNG),
				     KRT_Q_END(KRTQ_L_ADD),
				     FALSE,
				     FALSE,
				     &todo);
		}
	    }
	    if (rc != KRT_Q_FLUSH_FULL) {
		break;
	    }
	    /* FALLSTHROUGH */

	case KRT_STATE_FULL:
	    /*
	     * First do high priority deletes, see if that gets
	     * us enough.
	     */
	    if (KRT_N_QUEUED(KRTQ_H_DEL) > 0) {
		rc = krt_q_flush(KRT_Q_START(KRTQ_H_DEL),
				 KRT_Q_END(KRTQ_H_DEL),
				 FALSE,
				 TRUE,
				 &todo);
		if (rc != KRT_Q_FLUSH_DONE) {
		    break;
		}
	    }

	    /*
	     * Now try high priority changes/adds.
	     */
	    rc = krt_q_flush(KRT_Q_START(KRTQ_H_INT),
			     KRT_Q_END(KRTQ_H_ADD),
			     FALSE,
			     FALSE,
			     &todo);
	    /*
	     * Try low priority changes/adds.
	     */
	    if (rc == KRT_Q_FLUSH_DONE) {
		rc = krt_q_flush(KRT_Q_START(KRTQ_L_CHNG),
				 KRT_Q_END(KRTQ_L_ADD),
				 FALSE,
				 FALSE,
				 &todo);
	    }

	    /*
	     * If that didn't work, do the low priority deletions
	     */
	    if (rc == KRT_Q_FLUSH_FULL) {
		rc = krt_q_flush(KRT_Q_START(KRTQ_L_DEL),
				 KRT_Q_END(KRTQ_L_CHNG),
				 FALSE,
				 TRUE,
				 &todo);
		if (rc != KRT_Q_FLUSH_DONE) {
		    break;
		}
		rc = krt_q_flush(KRT_Q_START(KRTQ_H_INT),
				 KRT_Q_END(KRTQ_L_ADD),
				 FALSE,
				 FALSE,
				 &todo);
	    }
	    break;
	}
    } else {
	switch (krt_state) {
	default:
	    /*
	     * Non-crisis mode.  Try to flush the whole queue in order
	     */
	    rc = krt_q_flush(KRT_Q_START(KRTQ_H_INT),
			     KRT_Q_END(KRTQ_L_ADD),
			     FALSE,
			     FALSE,
			     &todo);
	    if (rc != KRT_Q_FLUSH_FULL) {
		break;
	    }
	    /* FALLSTHROUGH */

	case KRT_STATE_FULL:
	    /*
	     * Crisis mode here.  Do deletes first to try
	     * to clear kernel space.  High priority deletes
	     * first.
	     */
	    if (KRT_N_QUEUED(KRTQ_H_DEL) > 0) {
		rc = krt_q_flush(KRT_Q_START(KRTQ_H_DEL),
				 KRT_Q_END(KRTQ_H_DEL),
				 FALSE,
				 TRUE,
				 &todo);
		if (rc != KRT_Q_FLUSH_DONE) {
		    break;
		}
	    }

	    /*
	     * Now do low priority deletes and delete old
	     * routes for changes.
	     */
	    rc = krt_q_flush(KRT_Q_START(KRTQ_L_DEL),
			     KRT_Q_END(KRTQ_L_CHNG),
		 	     FALSE,
			     TRUE,
			     &todo);
	    if (rc != KRT_Q_FLUSH_DONE) {
		break;
	    }

	    /*
	     * Now do adds/changes from the beginning, doing
	     * the high priority stuff first.
	     */
	    rc = krt_q_flush(KRT_Q_START(KRTQ_H_INT),
			     KRT_Q_END(KRTQ_L_ADD),
			     FALSE,
			     FALSE,
			     &todo);
	    break;
	}
    }

    /*
     * Close the routing table, and tell the kernel-dependent
     * code that this is it.
     */
    rt_close(krt_task, (gw_entry *) 0, 0, NULL);
    end_rc = krt_change_end(krt_task);

    if (rc == KRT_Q_FLUSH_COUNT && end_rc == KRT_OP_SUCCESS) {
	/*
	 * More to do and no failures, just return so we'll
	 * run again.
	 */
	return;
    }

    /*
     * Set the state if the change end routine indicated that
     * something was amiss.
     */
    if (end_rc != KRT_OP_SUCCESS) {
	switch (end_rc) {
	case KRT_OP_FULL:
	    krt_state = KRT_STATE_FULL;
	    break;
	case KRT_OP_BLOCKED:
	    krt_state = KRT_STATE_BLOCKED;
	    break;
	default:
	    /* XXX log end_rc? */
	    assert(FALSE);
	}
    }

    /*
     * If we're terminating, bump the terminate counter to indicate
     * we've done a queue run.
     */
    if (krt_terminating) {
	krt_terminating++;
    }

    /*
     * Reconsider when we'll need to run again.
     */
    krt_q_schedule(FALSE);
}


/*
 * krt_q_timeout - deal with a queue timeout expiry
 */
static void
krt_q_timeout __PF2(tip, task_timer *,
		    interval, time_t)
{
    /*
     * Job should not be queued already.
     */
    assert(!krt_q_job);

    /*
     * Mark the timer as inactive and queue up the
     * background job
     */
    krt_q_time = (time_t) 0;
    krt_q_job = task_job_create(krt_task,
				krt_bg_priority,
				"Route Install (Timeout)",
				krt_q_run,
				(void_t) 0);
}


static flag_t krt_warned = 0;

/* Process the change list generated by routing table changes. */
void
krt_flash __PF1(rtl, rt_list *)
{
    rt_head *rth;
    u_int rtbit = krt_task->task_rtbit;
    rt_entry *new_rt;
    rt_entry *old_rt;
    rt_changes *rtc;
    krt_q_entry *kqp = (krt_q_entry *) 0;
    flag_t warning = 0;
    int dequeue = 0;
    int unfill = 0;
    int freeit = 0;
    int enqueue = 0;
    rt_entry *fill_rt = (rt_entry *) 0;
    int release_fill_rt = 0;
    rt_changes *fill_rtc = (rt_changes *) 0;
    int deletes_queued = 0;
    int missing_rejects = !BIT_MATCH(krt_rt_support, KRTS_REJECT|KRTS_BLACKHOLE);
#ifdef	PROTO_INET
    int old_kernel = !BIT_MATCH(krt_rt_support, KRTS_HOST|KRTS_VAR_MASK);
#endif	/* PROTO_INET */

    /*
     * Open the routing table for changes
     */
    rt_open(krt_task);

    RT_LIST(rth, rtl, rt_head) {

	/*
	 * See if we have our bit set somewhere
	 */
	old_rt = (rt_entry *) 0;
	if (rth->rth_n_announce) {
	    RT_ALLRT(old_rt, rth) {
		if (rtbit_isset(old_rt, rtbit)) {
		    break;
		}
	    } RT_ALLRT_END(old_rt, rth) ;
	}

	/*
	 * If we found a route with our bit set, and it is both the
	 * last_active and the active route, ignore this unless the
	 * next hop changed.  If the active route is the same as the
	 * last_active route, and our bit is *not* set, ignore this unless
	 * the next hop has changed.
	 */
	rtc = rth->rth_changes;
	if (rth->rth_last_active == rth->rth_active
	    && (!rtc || !BIT_TEST(rtc->rtc_flags, RTCF_NEXTHOP))
	    && old_rt && old_rt == rth->rth_active) {
	    continue;
	}

	/*
	 * Not so easy, see if we have a queue entry attached to the
	 * route.  If there isn't one, or if there is one and it is not
	 * a delete, the old route must be the last active route.
	 * XXX remove check once this works?
	 */
	if (old_rt) {
	    KRT_GET_Q_ENT(rth, rtbit, kqp);
	    if (kqp) {
		assert(kqp->krtq_rt == old_rt);
	    }
	}

	/*
	 * Check to see if there is a new route which is installable in
	 * the kernel we're running with.
	 */
	new_rt = rth->rth_active;
	if (new_rt) {
	    if (BIT_TEST(new_rt->rt_state, RTS_NOTINSTALL)) {
		new_rt = (rt_entry *) 0;
		goto dooldroute;
	    }

	    if (missing_rejects) {
		if (BIT_TEST(new_rt->rt_state, RTS_REJECT)
		    && !BIT_TEST(krt_rt_support, KRTS_REJECT)) {
		    warning = KRTS_REJECT;
		} else if (BIT_TEST(new_rt->rt_state, RTS_BLACKHOLE)
			   && !BIT_TEST(krt_rt_support, KRTS_BLACKHOLE)) {
		    warning = KRTS_BLACKHOLE;
		}
	    }

#ifdef	PROTO_INET
	    if (!old_rt && warning == 0 && old_kernel
		&& socktype(rth->rth_dest) == AF_INET) {
		if (rth->rth_dest_mask == inet_mask_host) {
		    if (!BIT_TEST(krt_rt_support, KRTS_HOST|KRTS_VAR_MASK)) {
			warning = KRTS_HOST;
		    }
		} else if (!BIT_TEST(krt_rt_support, KRTS_VAR_MASK)) {
		    if_addr *ifap = inet_ifwithnet(rth->rth_dest);

		    if (rth->rth_dest_mask == inet_mask_natural(rth->rth_dest)) {
			if (ifap && ifap->ifa_netmask != rth->rth_dest_mask) {
			    warning = KRTS_VAR_MASK;
			}
		    } else if (!ifap || ifap->ifa_netmask != rth->rth_dest_mask) {
			warning = KRTS_VAR_MASK;
		    }
		}
	    }
#endif	/* PROTO_INET */

	    if (warning != 0) {
		krt_trace(krt_task,
			  "RT",
			  "ERROR",
			  rth->rth_dest,
			  rth->rth_dest_mask,
			  RT_ROUTER(new_rt),
			  krt_state_to_flags(new_rt->rt_state),
			  trace_bits(krt_support_msgs, warning),
			  0);
		if (!BIT_TEST(krt_warned, warning)) {
		    BIT_SET(krt_warned, warning);
		    trace_log_tp(krt_task,
				 TRC_NL_BEFORE|TRC_NL_AFTER,
				 LOG_ERR,
				 ("krt_flash: WARNING! %s!",
				  gd_upper(trace_bits(krt_support_msgs, warning))));
		}
		new_rt = (rt_entry *) 0;
		warning = 0;
	    }
	}


dooldroute:
	/*
	 * If we have an old route which has been overridden, and the
	 * old route is a remnant, schedule it for deletion.
	 */
	if (old_rt
	    && old_rt->rt_gwp == krt_gwp_remnant
	    && !BIT_TEST(old_rt->rt_state, RTS_DELETE)
	    && old_rt != new_rt) {
	    KRT_REMNANT_DELETE(old_rt);
	}

	/*
	 * If we haven't either an old route or a new route,
	 * we're done here.  If we don't have a new route this
	 * is at best a delete.
	 */
	if (!new_rt) {
	    if (!old_rt) {
		continue;
	    }

	    if (kqp) {
		/*
		 * If this is a delete already, we're done.  Otherwise,
		 * if there is no route in the kernel already, we no
		 * longer need to do this.  Otherwise simply convert the
		 * operation into a delete and requeue it.
		 */
		if (kqp->krtq_op != KRTQ_OP_DELETE) {
		    dequeue = 1;
		    if (kqp->krtq_op == KRTQ_OP_ADD) {
			KRT_CLR_Q_ENT(rth, rtbit);
			(void) rtbit_reset(old_rt, rtbit);
			freeit = 1;
		    } else {
			kqp->krtq_op = KRTQ_OP_DELETE;
			enqueue = 1;
		    }
		}
	    } else {
		/*
		 * This is a deletion of a route which is already
		 * in the kernel.  Fetch a new queue entry and record
		 * the current state of the route in the kernel,
		 * then queue up the deletion.
		 */
		kqp = (krt_q_entry *)task_block_alloc(krt_q_block_index);
		kqp->krtq_op = KRTQ_OP_DELETE;
		kqp->krtq_rt = old_rt;
		fill_rt = old_rt;
		fill_rtc = rtc;
		KRT_PUT_Q_ENT(rth, rtbit, kqp);
		enqueue = 1;
	    }

	    /*
	     * If we have an active route and we're leaving our bit
	     * set on the old route, move the bit to the active route
	     * instead to avoid holding the old route.
	     */
	    if (!freeit && rth->rth_active && (old_rt != rth->rth_active)) {
		kqp->krtq_rt = rth->rth_active;
		if (!enqueue) {
		    dequeue = 1;
		    enqueue = 1;
		}
		rtbit_set(kqp->krtq_rt, rtbit);
		if (fill_rt) {
		    release_fill_rt = 1;
		} else {
		    rtbit_reset(old_rt, rtbit);
		}
	    }
	} else {
	    /*
	     * Here we've got a new route which we're permitted to add.
	     * Set the bit on the new route if it isn't the same as the old route.
	     */
	    if (new_rt != old_rt) {
		rtbit_set(new_rt, rtbit);
	    }

	    /*
	     * If we have an old route (i.e. there is a route
	     * to this destination in the kernel) this will be a change,
	     * though we may not need to do anything if the new route
	     * matches what is in the kernel already.  Otherwise it
	     * will be a simple add.  Do the easy case first.
	     */
	    if (!old_rt) {
		kqp = (krt_q_entry *)task_block_alloc(krt_q_block_index);
		kqp->krtq_op = KRTQ_OP_ADD;
		kqp->krtq_rt = new_rt;
		KRT_PUT_Q_ENT(rth, rtbit, kqp);
		enqueue = 1;
	    } else if (!kqp) {
		flag_t ostate;
#if	RT_N_MULTIPATH > 1
		int n_gw;
		sockaddr_un **routers;
		if_addr **ifaps;

		if (BIT_TEST(krt_rt_support, KRTS_MULTIPATH)) {
		    /*
		     * Find old next hop(s)
		     */
		    if (rtc && BIT_TEST(rtc->rtc_flags, RTCF_NEXTHOP)) {
			n_gw = rtc->rtc_n_gw;
			routers = rtc->rtc_routers;
			ifaps = rtc->rtc_ifaps;
		    } else {
			n_gw = old_rt->rt_n_gw;
			routers = old_rt->rt_routers;
			ifaps = old_rt->rt_ifaps;
		    }
		} else {
		    n_gw = 0;
		    ifaps = (if_addr **) 0;
		    routers = (sockaddr_un **) 0;
		}
#endif	/* RT_N_MULTIPATH > 1 */

		/*
		 * Here there is a route already in the kernel, whose
		 * state must be determined from the old route.  First
		 * determine if the state of the new route matches the
		 * state of the old route.
		 */
		ostate = old_rt->rt_state & KRT_RTS_BITS;
		if (ostate == (new_rt->rt_state & KRT_RTS_BITS)) {
		    int match = 0;

		    if (BIT_TEST(ostate, KRT_RTS_NO_NEXTHOP)) {
			match = 1;
#if	RT_N_MULTIPATH > 1
		    } else if (BIT_TEST(krt_rt_support, KRTS_MULTIPATH)) {
			register int i;

			for (i = 0; i < n_gw; i++) {
			    if (!sockaddrcmp(new_rt->rt_routers[i], routers[i])) {
				break;
			    }
			    if (new_rt->rt_ifaps[i] != ifaps[i]) {
				break;
			    }
			}

			if (i >= n_gw) {
			    match = 1;
			}
#endif	/* RT_N_MULTIPATH > 1 */
		    } else {
			if (rtc && BIT_TEST(rtc->rtc_flags, RTCF_NEXTHOP)) {
			    if (sockaddrcmp(RT_ROUTER(new_rt), RTC_ROUTER(rtc))
			      && RT_IFAP(new_rt) == RTC_IFAP(rtc)) {
				match = 1;
			    }
			} else {
			    if (sockaddrcmp(RT_ROUTER(new_rt), RT_ROUTER(old_rt))
			      && RT_IFAP(new_rt) == RT_IFAP(old_rt)) {
				match = 1;
			    }
			}
		    }

		    if (match) {
			/*
			 * Nothing to do here, just turn off the bit on the old
			 * route and continue.
			 */
			if (new_rt != old_rt) {
			    (void) rtbit_reset(old_rt, rtbit);
			}
			continue;
		    }
		}

		/*
		 * We have a new route which doesn't match the state of the
		 * kernel.  Create a queue entry for it and fill in the
		 * state.
		 */
		kqp = (krt_q_entry *)task_block_alloc(krt_q_block_index);
		kqp->krtq_op = KRTQ_OP_CHANGE;
		kqp->krtq_rt = new_rt;
		fill_rt = old_rt;
		fill_rtc = rtc;

		/*
		 * Release old route, put the queue entry in the tsi and
		 * mark that this should be enqueued.
		 */
		if (new_rt != old_rt) {
		    release_fill_rt = 1;
		}
		KRT_PUT_Q_ENT(rth, rtbit, kqp);
		enqueue = 1;
	    } else {	
		/*
		 * Here we have an existing queue entry.  We check to see
		 * if the new route matches the state of the kernel route
		 * and, if so, we free the queue entry.  Otherwise we
		 * requeue the entry at the end of the queue, pointing
		 * at the new route.
		 */
		if (new_rt != old_rt) {
		    (void) rtbit_reset(old_rt, rtbit);		/* don't need old route */
		}
		dequeue = 1;

		if (kqp->krtq_op != KRTQ_OP_ADD
		  && kqp->krtq_state == (new_rt->rt_state & KRT_RTS_BITS)) {
		    if (BIT_TEST(kqp->krtq_state, KRT_RTS_NO_NEXTHOP)) {
			unfill = freeit = 1;
#if	RT_N_MULTIPATH > 1
		    } else if (BIT_TEST(krt_rt_support, KRTS_MULTIPATH)) {
			if (kqp->krtq_n_gw == new_rt->rt_n_gw) {
			    register int i;

			    for (i = 0; i < kqp->krtq_n_gw; i++) {
				if (!sockaddrcmp(new_rt->rt_routers[i],
				  kqp->krtq_routers[i])) {
				    break;
				}
				if (new_rt->rt_ifaps[i] != kqp->krtq_ifaps[i]) {
				    break;
				}
			    }
			    if (i >= kqp->krtq_n_gw) {
				unfill = freeit = 1;
			    }
			}
#endif	/* RT_N_MULTIPATH > 1 */
		    } else {
			if (sockaddrcmp(RT_ROUTER(new_rt), kqp->krtq_router)
			      && RT_IFAP(new_rt) == kqp->krtq_ifap) {
				unfill = freeit = 1;
			}
		    }
		}

		if (freeit) {
		    KRT_CLR_Q_ENT(rth, rtbit);
		} else {
		    /*
		     * Simply point the queue entry at the new route.  If this
		     * was a delete turn it into a change.
		     */
		    kqp->krtq_rt = new_rt;
		    if (kqp->krtq_op == KRTQ_OP_DELETE) {
			kqp->krtq_op = KRTQ_OP_CHANGE;
		    }
		    if (kqp->krtq_time) {
			KRT_UNDEFER(kqp);
		    }
		    enqueue = 1;
		}
	    }
	}

	if (dequeue) {
	    if (kqp->krtq_time) {
		KRT_UNDEFER(kqp);
	    }
	    KRT_DEQUEUE(kqp);
	    dequeue = 0;
	}

	if (unfill) {
	    register int i = kqp->krtq_n_gw;

	    while (--i >= 0) {
		sockfree(kqp->krtq_routers[i]);
		IFA_FREE(kqp->krtq_ifaps[i]);
	    }
	    unfill = 0;
	}

	if (fill_rt) {
	    kqp->krtq_proto = fill_rt->rt_gwp->gw_proto;
	    kqp->krtq_state = fill_rt->rt_state & KRT_RTS_BITS;
#ifdef	IP_MULTICAST_ROUTING
 	    kqp->krtq_preference = fill_rt->rt_preference;
 	    kqp->krtq_metric = fill_rt->rt_metric;
#endif	/* IP_MULTICAST_ROUTING */
	    if (BIT_TEST(kqp->krtq_state, KRT_RTS_NO_NEXTHOP)) {
		kqp->krtq_n_gw = 0;
	    } else if (fill_rtc && BIT_TEST(fill_rtc->rtc_flags, RTCF_NEXTHOP)) {
#if	RT_N_MULTIPATH > 1
		if (BIT_TEST(krt_rt_support, KRTS_MULTIPATH)) {
		    register int i;

		    kqp->krtq_n_gw = fill_rtc->rtc_n_gw;
		    for (i = 0; i < kqp->krtq_n_gw; i++) {
			kqp->krtq_routers[i] = sockdup(fill_rtc->rtc_routers[i]);
			IFA_ALLOC(kqp->krtq_ifaps[i] = fill_rtc->rtc_ifaps[i]);
		    }
		} else {
#endif	/* RT_N_MULTIPATH > 1 */
		    kqp->krtq_n_gw = 1;
		    kqp->krtq_router = sockdup(RTC_ROUTER(fill_rtc));
		    IFA_ALLOC(kqp->krtq_ifap = RTC_IFAP(fill_rtc));
#if	RT_N_MULTIPATH > 1
		}
#endif	/* RT_N_MULTIPATH > 1 */
	    } else {
#if	RT_N_MULTIPATH > 1
		if (BIT_TEST(krt_rt_support, KRTS_MULTIPATH)) {
		    register int i;

		    kqp->krtq_n_gw = fill_rt->rt_n_gw;
		    for (i = 0; i < kqp->krtq_n_gw; i++) {
			kqp->krtq_routers[i] = sockdup(fill_rt->rt_routers[i]);
			IFA_ALLOC(kqp->krtq_ifaps[i] = fill_rt->rt_ifaps[i]);
		    }
		} else {
#endif	/* RT_N_MULTIPATH > 1 */
		    kqp->krtq_n_gw = 1;
		    kqp->krtq_router = sockdup(RT_ROUTER(fill_rt));
		    IFA_ALLOC(kqp->krtq_ifap = RT_IFAP(fill_rt));
#if	RT_N_MULTIPATH > 1
		}
#endif	/* RT_N_MULTIPATH > 1 */
	    }

	    if (release_fill_rt) {
		rtbit_reset(fill_rt, rtbit);
		release_fill_rt = 0;
	    }
	    fill_rt = (rt_entry *) 0;
	    fill_rtc = (rt_changes *) 0;
	}

	if (freeit) {
	    task_block_free(krt_q_block_index, (void_t) kqp);
	    freeit = 0;
	} else if (enqueue) {
	    register int which;

	    if (!BIT_TEST(kqp->krtq_rt->rt_state, RTS_GATEWAY)) {
		which = KRTQ_H_INT;
		if (kqp->krtq_op == KRTQ_OP_DELETE) {
		    deletes_queued++;
		}
	    } else if (KRT_IS_HIPRIO(kqp->krtq_rt)) {
		switch (kqp->krtq_op) {
		default:
		    assert(FALSE);
		case KRTQ_OP_DELETE:
		    which = KRTQ_H_DEL;
		    deletes_queued++;
		    break;
		case KRTQ_OP_ADD:
		    which = KRTQ_H_ADD;
		    break;
		case KRTQ_OP_CHANGE:
		    which = KRTQ_H_CHNG;
		    break;
		}
	    } else {
		switch (kqp->krtq_op) {
		default:
		    assert(FALSE);
		case KRTQ_OP_DELETE:
		    which = KRTQ_L_DEL;
		    deletes_queued++;
		    break;
		case KRTQ_OP_ADD:
		    which = KRTQ_L_ADD;
		    break;
		case KRTQ_OP_CHANGE:
		    which = KRTQ_L_CHNG;
		    deletes_queued++;
		    break;
		}
	    }
	    KRT_ENQUEUE(which, kqp);
	    enqueue = 0;
	}
    } RT_LIST_END(rth, rtl, rt_head) ;


    /*
     * If we're currently full, and we added some deletes, force
     * the scheduler to queue the job.  Otherwise, if we're in
     * normal mode and have a hiprio install count, install some
     * high priority routes right now.
     */
    freeit = FALSE;
    if (krt_state == KRT_STATE_FULL) {
	if (deletes_queued) {
	    freeit = TRUE;
	}
    } else if (krt_state == KRT_STATE_NORMAL && krt_flash_install_count > 0) {
	u_long todo = krt_flash_install_count;
	int rc;

	rc = krt_change_start(krt_task);
	if (rc != KRT_OP_SUCCESS) {
	    switch (rc) {
	    case KRT_OP_FULL:
		krt_state = KRT_STATE_FULL;
		break;
	    case KRT_OP_BLOCKED:
		krt_state = KRT_STATE_BLOCKED;
		break;
	    default:
		/* XXX log rc? */
		assert(FALSE);
	    }
	    krt_last_q_run = time_sec;
	} else {
	    (void) krt_q_flush(KRT_Q_START(KRTQ_H_INT),
			       krt_flash_q_end,
			       TRUE,
			       FALSE,
			       &todo);

	    rc = krt_change_end(krt_task);

	    if (rc != KRT_OP_SUCCESS) {
		switch(rc) {
		case KRT_OP_FULL:
		    krt_state = KRT_STATE_FULL;
		    break;
		case KRT_OP_BLOCKED:
		    krt_state = KRT_STATE_BLOCKED;
		    break;
		default:
		    /* XXX log rc? */
		    assert(FALSE);
		}
	    }
	}
    }

    /* Close the routing table */
    rt_close(krt_task, (gw_entry *) 0, 0, NULL);

    /*
     * Schedule a queue run
     */
    krt_q_schedule(freeit);
}


/*
 * krt_rth_reset - given a pointer to a route head, and a route state and (set of)
 *		   next hop(s) which represent the actual next hop and state
 *		   of the route to this destination in the kernel, do the right
 *		   thing such that the state of the routing table and the state
 *		   of the kernel are brought into agreement.
 */
void
krt_rth_reset __PF5(rth, rt_head *,
		    state, flag_t,
		    n_gw, int,
		    routers, sockaddr_un **,
		    ifaps, if_addr **)
{
    register rt_entry *rt;
    register krt_q_entry *kqp;
    int no_krt;
    int no_krt_bit;	/* No krt bit set */
    u_int rtbit = krt_task->task_rtbit;
    int no_nexthop_bits = (state & KRT_RTS_NO_NEXTHOP);
    int match = 0;	/* Route matches active */
    int enqueue = 0;	/* Enqueue q entry on krt q */
    int dequeue = 0;	/* Dequeue q entry from krt q */
    int unfill = 0;	/* Free sockaddr/ifaps in q entry */
    int fill = 0;	/* Copy sockaddr/ifaps into q entry */
    int freeit = 0;	/* Free queue entry when done */

    /*
     * Basic sanity
     */
    if (!BIT_TEST(krt_rt_support, KRTS_MULTIPATH)) {
	assert(n_gw <= 1);
    }
    if (no_nexthop_bits) {
	assert(!BIT_TEST(no_nexthop_bits, RTS_REJECT) || BIT_TEST(krt_rt_support, KRTS_REJECT));
	assert(!BIT_TEST(no_nexthop_bits, RTS_BLACKHOLE) || BIT_TEST(krt_rt_support, KRTS_BLACKHOLE));
    }

    if (!BIT_TEST(state, KRT_RTS_NO_NEXTHOP) && n_gw == 0) {
	no_krt = 1;
    } else {
	no_krt = 0;
    }

    /*
     * Determine if the krt bit is set somewhere
     */
    rt = (rt_entry *) 0;
    if (rth->rth_n_announce) {
	RT_ALLRT(rt, rth) {
	    if (rtbit_isset(rt, rtbit)) {
		break;
	    }
	} RT_ALLRT_END(rt, rth) ;
    }

    if (rt) {
	no_krt_bit = 0;
    } else {
	no_krt_bit = 1;
	rt = rth->rth_active;
    }

    /*
     * See if the active route matches what is currently in
     * the kernel.  Only bother doing this if the route is not
     * on the flash list, or if we are initializing.
     */
    if (rt && rt == rth->rth_active && !no_krt
	&& !BIT_TEST(rt->rt_state, RTS_NOTINSTALL)
	&& (!BIT_TEST(rth->rth_state, RTS_ONLIST)
	    || BIT_TEST(task_state, TASKS_INIT))) {
	if (BIT_TEST(rt->rt_state, KRT_RTS_NO_NEXTHOP)) {
	    if (no_nexthop_bits == (rt->rt_state & KRT_RTS_NO_NEXTHOP)) {
		match = 1;
	    }
#if	RT_N_MULTIPATH > 1
	} else if (BIT_TEST(krt_rt_support, KRTS_MULTIPATH)) {
	    if (n_gw == rt->rt_n_gw) {
		register int i = n_gw;

		while (--i >= 0) {
		    if (rt->rt_routers[i] != routers[i]
			&& !sockaddrcmp(rt->rt_routers[i], routers[i])) {
			break;
		    }
		    if (ifaps) {
			if (rt->rt_ifaps[i] != ifaps[i]) {
			    break;
			}
		    }
		}
		if (i < 0) {
		    match = 1;
		}
	    }
#endif	/* RT_N_MULTIPATH > 1 */
	} else if (rt->rt_n_gw > 0 && n_gw == 1) {
	    if (RT_ROUTER(rt) == routers[0]
		|| sockaddrcmp(RT_ROUTER(rt), routers[0])) {
		if (!ifaps || (RT_IFAP(rt) == ifaps[0])) {
		    match = 1;
		}
	    }
	}
    }

    /*
     * If there is no old route, the routing table believes
     * there is no route to the destination in the kernel.
     * Update it with the new information about what is in
     * the kernel.
     */
    if (no_krt_bit) {
	int delete_active = 0;

	/*
	 * If he's telling us there is no route in the kernel,
	 * we knew that already.
	 */
	if (no_krt) {
	    return;
	}

	/*
	 * If we match the active route, and if we don't have a bit
	 * set on it, and we're initializing, just set the bit on it.
	 */
	if (match && BIT_TEST(task_state, TASKS_INIT)) {
	    rtbit_set(rt, rtbit);
	    KRT_CLR_Q_ENT(rth, rtbit);
	    return;
	}

	/*
	 * No easy way out.  What we do here is set our bit on the active
	 * route and queue a delete for it with the kernel state in the
	 * delete.  This will cause the kernel route to be deleted unless
	 * there is an intervening flash.  Note that the fact that no bit
	 * was set anywhere should mean that either the active route was
	 * one we disliked and shouldn't have in the kernel, or that
	 * there will be a flash and we'll get to reevaluate kernel
	 * matches shortly.
	 */
	kqp = (krt_q_entry *)task_block_alloc(krt_q_block_index);
	kqp->krtq_op = KRTQ_OP_DELETE;
	kqp->krtq_time = (time_t) 0;
	kqp->krtq_proto = RTPROTO_KERNEL;
	fill = enqueue = 1;

	/*
	 * So far so good.  If there is no active route, make one temporarily.
	 */
	if (!rt) {
	    rt = krt_make_rtactive(rth->rth_dest, rth->rth_dest_mask);
	    delete_active = 1;
	}

	/*
	 * Set the routing bit on the route and store the queue pointer away.
	 */
	kqp->krtq_rt = rt;
	rtbit_set(rt, rtbit);
	KRT_PUT_Q_ENT(rth, rtbit, kqp);

	/*
	 * If we created a temporary active route, blow it away.
	 */
	if (delete_active) {
	    rt_delete(rt);
	}
    } else {
	/*
	 * Here we already have a route with our bit set.  If we have a
	 * queue pointer in the route this is easier done.
	 */
	KRT_GET_Q_ENT(rth, rtbit, kqp);

	if (kqp) {
	    assert(kqp->krtq_rt == rt);

	    if (kqp->krtq_op == KRTQ_OP_DELETE) {
		/*
		 * If there is now no route in the kernel, we don't need
	 	 * to do this.  Blow the entry entirely away and return.
		 * Otherwise rewrite the queue entry with the kernel's
		 * current state.
	 	 */
	 	if (no_krt) {
		    dequeue = unfill = freeit = 1;
		    KRT_CLR_Q_ENT(rth, rtbit);
		    rtbit_reset(rt, rtbit);
		} else {
		    fill = unfill = 1;
		    if (rth->rth_active && rth->rth_active != rt) {
			/*
			 * Move the bit to the active route.
			 */
			dequeue = enqueue = 1;
			kqp->krtq_rt = rth->rth_active;
			rtbit_set(kqp->krtq_rt, rtbit);
			rtbit_reset(rt, rtbit);
			rt = kqp->krtq_rt;
		    }
		}
	    } else {
		/*
		 * Change or an add here.  If there is no route in the
		 * kernel do nothing if an add, or convert the change
		 * into an add.  Otherwise, if the route in the kernel
		 * matches what we are changing to, just dequeue the
		 * operation.  Otherwise, rewrite the queue entry with
		 * the current kernel state.
		 */
		if (no_krt) {
		    if (kqp->krtq_op == KRTQ_OP_CHANGE) {
			kqp->krtq_op = KRTQ_OP_ADD;
			dequeue = enqueue = unfill = 1;
		    }
		} else {
		    if (match) {
			dequeue = freeit = 1;
			if (kqp->krtq_op  == KRTQ_OP_CHANGE) {
			    unfill = 1;
			}
			KRT_CLR_Q_ENT(rth, rtbit);
		    } else if (kqp->krtq_op == KRTQ_OP_ADD) {
			kqp->krtq_op = KRTQ_OP_CHANGE;
			dequeue = enqueue = fill = 1;
		    } else {
			unfill = fill = 1;
		    }
		}
	    }
	} else {
	    /*
	     * Here we have the krt bit set, but no queue entry.  This
	     * means we think there is a route installed in the kernel
	     * which matches the route with the bit set.  If there is
	     * no route in the kernel we will need to queue an add.  Otherwise
	     * we will need to queue a change with the present kernel
	     * state listed in the queue entry, except in the case where
	     * the current kernel state matches the route in which case
	     * we need not do anything.
	     */
	    if (match) {
		return;
	    }
	    kqp = (krt_q_entry *)task_block_alloc(krt_q_block_index);
	    kqp->krtq_time = (time_t) 0;
	    kqp->krtq_rt = rt;
	    KRT_PUT_Q_ENT(rth, rtbit, kqp);
	    enqueue = 1;
	    if (no_krt) {
		kqp->krtq_op = KRTQ_OP_ADD;
	    } else {
		kqp->krtq_op = KRTQ_OP_CHANGE;
		fill = 1;
	    }
	}
    }

    /*
     * Time to do things.  Dequeue the q entry first.
     */
    if (dequeue) {
	KRT_DEQUEUE(kqp);
    }

    /*
     * Delete/free the sockaddr's/ifap's
     */
    if (unfill) {
	register int i = kqp->krtq_n_gw;

	while (--i >= 0) {
	    sockfree(kqp->krtq_routers[i]);
	    IFA_FREE(kqp->krtq_ifaps[i]);
	}
    }

    /*
     * Fill them back in with the kernel state.  If the
     * kernel state is something we can't deal with (either
     * there are more gateways than we are compiled for,
     * or we can't find an interface for one of the routes)
     * queue this with a NULL ifap so the kernel code knows
     * to delete the route.
     */
    if (fill) {
	register int i;
	int must_delete = 0;

	kqp->krtq_proto = RTPROTO_KERNEL;
	kqp->krtq_state = state & KRT_RTS_BITS;
	if (no_nexthop_bits) {
	    kqp->krtq_n_gw = 0;
	} else if (n_gw > RT_N_MULTIPATH) {
	    must_delete = 1;
	} else if (!ifaps) {
	    for (i = 0; i < n_gw; i++) {
		kqp->krtq_ifaps[i] = if_withroute(rth->rth_dest,
						  routers[i],
						  state);
		if (kqp->krtq_ifaps[i] == (if_addr *) 0) {
		    must_delete = 1;
		    break;
		}
	    }
	}

	if (must_delete) {
	    kqp->krtq_n_gw = 1;
	    kqp->krtq_routers[0] = sockdup(routers[0]);
	    kqp->krtq_ifaps[0] = (if_addr *) 0;
	} else if (!no_nexthop_bits) {
	    kqp->krtq_n_gw = n_gw;
	    for (i = 0; i < n_gw; i++) {
		kqp->krtq_routers[i] = sockdup(routers[i]);
		if (ifaps) {
		    kqp->krtq_ifaps[i] = ifaps[i];
		}
		IFA_ALLOC(kqp->krtq_ifaps[i]);
	    }
	}
    }

    /*
     * Either enqueuing the pointer or deleting it left.
     * Check the delete first so bugs will core dump easier.
     */
    if (freeit) {
	task_block_free(krt_q_block_index, (void_t) kqp);
    } else if (enqueue) {
	int which;

	if (!BIT_TEST(rt->rt_state, RTS_GATEWAY)) {
	    which = KRTQ_H_INT;
	} else if (KRT_IS_HIPRIO(rt)) {
	    switch (kqp->krtq_op) {
	    default:
		assert(FALSE);
	    case KRTQ_OP_DELETE:
		which = KRTQ_H_DEL;
		break;
	    case KRTQ_OP_ADD:
		which = KRTQ_H_ADD;
		break;
	    case KRTQ_OP_CHANGE:
		which = KRTQ_H_CHNG;
		break;
	    }
	} else {
	    switch (kqp->krtq_op) {
	    default:
		assert(FALSE);
	    case KRTQ_OP_DELETE:
		which = KRTQ_L_DEL;
		break;
	    case KRTQ_OP_ADD:
		which = KRTQ_L_ADD;
		break;
	    case KRTQ_OP_CHANGE:
		which = KRTQ_L_CHNG;
		break;
	    }
	}
	KRT_ENQUEUE(which, kqp);
    }

    /* Process any changes we have made */
    
    krt_q_schedule(FALSE);
    /*
     * Done!
     */
}

/*
 * krt_dst_reset - called to reset a route when there is no existing
 *		   route to the destination.  This makes a temporary
 *		   active route, resets this and deletes it.
 */
void
krt_dst_reset __PF6(dest, sockaddr_un *,
		    destmask, sockaddr_un *,
		    state, flag_t,
		    n_gw, int,
		    routers, sockaddr_un **,
		    ifaps, if_addr **)
{
    rt_entry *rt;

    rt = krt_make_rtactive(dest, destmask);
    krt_rth_reset(rt->rt_head,
		  state,
		  n_gw,
		  routers,
		  ifaps);
    rt_delete(rt);
}


/*
 *	If we support the routing socket, remove any static routes we
 *	installed. 
 *
 *	Reset the announcement bits on any routes installed in the kernel.
 *	If they don't have RTS_RETAIN set they get removed from the kernel.
 */
static void
krt_terminate __PF1(tp, task *)
{
#ifdef	KRT_RT_SOCK
    int changes = 0;
    rt_entry *rt;

    rt_open(tp);

    RTQ_LIST(&krt_gwp->gw_rtq, rt) {
	if (!BIT_TEST(rt->rt_state, RTS_STATIC|RTS_NOADVISE)) {
	    /* Remove a route we installed */
	    rt_delete(rt);
	    changes++;
	}
    } RTQ_LIST_END(&krt_gwp->gw_rtq, rt) ;

    rt_close(tp, (gw_entry *) 0, changes, NULL);
#endif	/* KRT_RT_SOCK */

    /* XXX - Kill interface scan job */
}

static void
krt_shutdown __PF1(tp, task *)
{
    /*
     * Just set the terminate bit and reschedule a queue run.
     */
    krt_terminating = 1;
    krt_q_schedule(TRUE);
}


/**/

static task_job *krt_ifcheck_job;

static void
krt_ifcheck_run __PF1(jp, task_job *)
{
    (void) krt_ifread(task_state);

    krt_ifcheck_job = (task_job *) 0;
}

static void
krt_ifcheck_timer __PF2(tip, task_timer *,
		  interval, time_t)
{
    task *tp = tip->task_timer_task;
    
    if (!krt_ifcheck_job) {
	krt_ifcheck_job = task_job_create(tp,
					  TASK_JOB_FG,
					  "Interface scan",
					  krt_ifcheck_run,
					  (void_t) 0);
    }
}

void
krt_ifcheck __PF0(void)
{
    if (!krt_ifcheck_job) {
	krt_ifcheck_job = task_job_create(krt_task,
					  TASK_JOB_FG,
					  "Interface scan",
					  krt_ifcheck_run,
					  (void_t) 0);
    }
}

/**/

/* Figure out what the maximum value for a kernel socket buffer is */
static int
krt_get_maxpacket __PF1(tp, task *)
{
#ifdef	SO_RCVBUF
    u_int I;
    u_int U = (u_int) -1 >> 1;
    u_int L = 0;

    if (tp->task_socket < 0) {
	/* No socket, just wing it */

	task_maxpacket = 32767;
	return 0;
    }

    do {
	I = (U + L) / 2;
	if (setsockopt(tp->task_socket,
		       SOL_SOCKET,
		       SO_RCVBUF,
		       (caddr_t) &I,
		       (int) (sizeof (I))) < 0) {
	    /* Failed, reduce the upper limit */

	    U = I - 1;
	} else if (L == (u_int) -1) {
	    /* As big as we can get */

	    break;
	} else {
	    /* Success, increase the lower limit */

	    L = I + 1;
	}
    } while (U >= L);

    if (setsockopt(tp->task_socket,
		   SOL_SOCKET,
		   SO_RCVBUF,
		   (caddr_t) &I,
		   (int) (sizeof (I))) < 0) {
	/* Failed - reduce by one */

	I--;
    }

    task_maxpacket = I;

    if (TRACE_TP(tp, TR_KRT_SYMBOLS)) {
	trace_only_tp(tp,
		      TRC_NL_AFTER,
		      ("krt_get_maxpacket: Maximum kernel receive/send packet size is %u",
		       task_maxpacket));
    }
#else	/* SO_RCVBUF */
    task_maxpacket = 32767;
#endif	/* SO_RCVBUF */
    
    return 0;
}


/*
 *	Deal with an interface state change
 */
static void
krt_ifachange __PF2(tp, task *,
		    ifap, if_addr *)
{
#ifdef	KRT_RT_SOCK
    int changes = 0;

    rt_entry *rt;

    rt_open(tp);
#endif	/* KRT_RT_SOCK */
    
    switch (ifap->ifa_change) {
    case IFC_NOCHANGE:
    case IFC_ADD:
	if (BIT_TEST(ifap->ifa_state, IFS_UP)) {
	Up:;
#ifdef	KRT_RT_SOCK
	    RTQ_LIST(&krt_gwp->gw_rtq, rt) {
		if (RT_IFAP(rt) == ifap) {
		    /* Restore static routes by making their preference positive again */
		    if (rt->rt_preference < 0) {
			(void) rt_change(rt,
					 rt->rt_metric,
					 rt->rt_metric2,
					 rt->rt_tag,
					 -rt->rt_preference,
					 rt->rt_preference2,
					 0, (sockaddr_un **) 0);
			rt_refresh(rt);
		    }
		}
	    } RTQ_LIST_END(&krt_gwp->gw_rtq, rt) ;
#endif	/* KRT_RT_SOCK */
	}
	break;

    case IFC_DELETE:
	break;
	
    case IFC_DELETE|IFC_UPDOWN:
#ifdef	KRT_RT_SOCK
	RTQ_LIST(&krt_gwp->gw_rtq, rt) {
	    if (RT_IFAP(rt) == ifap) {
		/* Delete any static routes we learned via this interface */
		rt_delete(rt);
	    }
	} RTQ_LIST_END(&krt_gwp->gw_rtq, rt) ;
#endif	/* KRT_RT_SOCK */
	break;

    default:
	/* Something has changed */

	if (BIT_TEST(ifap->ifa_change, IFC_UPDOWN)) {
	    /* Up <-> Down transition */

	    if (BIT_TEST(ifap->ifa_state, IFS_UP)) {
		/* It's up now */

		goto Up;
	    } else {
		/* It's down now */

#ifdef	KRT_RT_SOCK
		RTQ_LIST(&krt_gwp->gw_rtq, rt) {
		    if (RT_IFAP(rt) == ifap) {
			/* Hide any static routes by giving them negative preferences */
			if (rt->rt_preference > 0) {
			    (void) rt_change(rt,
					     rt->rt_metric,
					     rt->rt_metric2,
					     rt->rt_tag,
					     -rt->rt_preference,
					     rt->rt_preference2,
					     0, (sockaddr_un **) 0);
			    rt_refresh(rt);
			}
		    }
		} RTQ_LIST_END(&krt_gwp->gw_rtq, rt) ;
#endif	/* KRT_RT_SOCK */
	    }
	}

	/* METRIC - We don't care about */
	/* BROADCAST - We don't care about */
	/* LOCALADDR - I don't think we care about */

#ifdef	KRT_RT_SOCK
	if (BIT_TEST(ifap->ifa_change, IFC_NETMASK)) {
	    RTQ_LIST(&krt_gwp->gw_rtq, rt) {
		if (RT_IFAP(rt) == ifap &&
		    ifap != if_withdstaddr(RT_ROUTER(rt))) {
		    /* Delete any static routes whose nexthop gateway */
		    /* is no longer on this interface */

		    rt_delete(rt);
		    changes++;
		}
	    } RTQ_LIST_END(&krt_gwp->gw_rtq, rt) ;
	}
#endif	/* KRT_RT_SOCK */
	break;
    }

#ifdef	PROTO_INET
    if ((inet_addr_reject != (sockaddr_un *) 0)
	!= BIT_MATCH(krt_rt_support, KRTS_REJECT)) {
	if (inet_addr_reject) {
	    BIT_SET(krt_rt_support, KRTS_REJECT);
	} else {
	    BIT_RESET(krt_rt_support, KRTS_REJECT);
	}
    }
#endif	/* PROTO_INET */
#ifdef	PROTO_INET
    if ((inet_addr_blackhole != (sockaddr_un *) 0)
	!= BIT_MATCH(krt_rt_support, KRTS_BLACKHOLE)) {
	if (inet_addr_blackhole) {
	    BIT_SET(krt_rt_support, KRTS_BLACKHOLE);
	} else {
	    BIT_RESET(krt_rt_support, KRTS_BLACKHOLE);
	}
    }
#endif	/* PROTO_INET */
    
#ifdef	KRT_RT_SOCK
    rt_close(tp, (gw_entry *) 0, changes, NULL);
#endif	/* KRT_RT_SOCK */
}


/*
 *  Age out kernel remnant routes
 */

static task_timer *krt_age_tip = (task_timer *) 0;


static void
krt_age __PF2(tip, task_timer *,
	      interval, time_t)
{
    time_t nexttime = time_sec;

    if (krt_remnant_list) {
	/* If there are remnants queued for deletion, do these first */
	krt_remnant_delete();
    }

    if (krt_gwp_remnant->gw_n_routes && time_sec > krt_t_expire) {
	time_t expire_to = time_sec - krt_t_expire;
	rt_entry *rt;

	rt_open(krt_task);
    
	RTQ_LIST(&krt_gwp_remnant->gw_rtq, rt) {
	    if (rt->rt_time <= expire_to) {
		/* This route has expired */
		
		rt_delete(rt);
	    } else {
		/* This is the next route to expire */
		if (rt->rt_time < nexttime) {
		    nexttime = rt->rt_time;
		}
		break;
	    }
	} RTQ_LIST_END(&krt_gwp_remnant->gw_rtq, rt) ;

	rt_close(krt_task, krt_gwp_remnant, 0, NULL);
    } else if (krt_gwp_remnant->gw_n_routes) {
	nexttime = krt_gwp_remnant->gw_rtq.rtq_forw->rtq_time;
    }

    if (!krt_gwp_remnant->gw_n_routes) {
	if (krt_age_tip) {
	    task_timer_delete(krt_age_tip);
	    krt_age_tip = (task_timer *) 0;
	}
    } else if (krt_age_tip) {
	task_timer_set(krt_age_tip,
		       (time_t) 0,
		       nexttime + krt_t_expire - time_sec);
    } else {
	krt_age_tip = task_timer_create(krt_task,
					"Age",
					(flag_t) 0,
					(time_t) 0,
					nexttime + krt_t_expire - time_sec,
					krt_age,
					(void_t) 0);
    }
}


void
krt_age_create __PF0(void)
{
    if (!krt_age_tip) {
	krt_age_tip = task_timer_create(krt_task,
					"Age",
					(flag_t) 0,
					(time_t) 0,
					(krt_t_expire ?
					  krt_t_expire : (time_t) 1),
					krt_age,
					(void_t) 0);
    }
}


static void
krt_tsi_dump __PF4(fp, FILE *,
		   rth, rt_head *,
		   data, void_t,
		   pfx, const char *)
{
    register krt_q_entry *kqp;

    KRT_GET_Q_ENT_NOCHECK(rth, krt_task->task_rtbit, kqp);

    if (kqp) {
	(void) fprintf(fp, "%sKRT %s %s\n",
		       pfx,
		       trace_state(kernel_queue_ops, kqp->krtq_op),
		       (kqp->krtq_time ? "deferred" : "pending"));
    }
}


/*
 *	Dump the routing socket queue
 */
static void
krt_dump __PF2(tp, task *,
	       fd, FILE *)
{
    krt_q_entry *kqp;
    rt_entry *rt;
    rt_changes *rtc;
    char buf[20];
    const char *dumpold = (const char *) 0;
    const char *dumpnew = (const char *) 0;

    /*
     * Dump state
     */

    (void) fprintf(fd, "\tOptions: <%s>\tKernel support: <%s>\n",
		   trace_bits(kernel_option_bits, krt_options),
		   trace_bits(kernel_support_bits, krt_rt_support));
    (void) fprintf(fd, "\tRemnant timeout: %#T\n",
		   krt_t_expire);
    if (krt_limit_routes == KRT_COUNT_UNLIMITED) {
	(void) strcpy(buf, "unlimited");
    } else {
	(void) sprintf(buf, "%d", krt_limit_routes);
    }
    (void) fprintf(fd, "\tRoutes in kernel: %u\tLimit: %s\n",
		   krt_n_routes,
		   buf);
    if (krt_flash_install_count == KRT_COUNT_UNLIMITED) {
	(void) strcpy(buf, "unlimited");
    } else {
	(void) sprintf(buf, "%d", krt_flash_install_count);
    }
    (void) fprintf(fd, "\tFlash install limit: %s\tFlash install routes: %s\n",
		   buf,
		   trace_state(kernel_flash_install_bits, krt_flash_routes));
    if (krt_install_count == KRT_COUNT_UNLIMITED) {
	(void) strcpy(buf, "unlimited");
    } else {
	(void) sprintf(buf, "%d", krt_install_count);
    }
    (void) fprintf(fd, "\tBackground install limit: %s\tBackground install routes: %s\n",
		   buf,
		   trace_state(kernel_install_prio_bits, krt_install_priority));
    (void) fprintf(fd, "\tKernel install state: %s\n\n",
		   trace_state(kernel_state_bits, krt_state));

    /*
     * Dump the queue
     */
    kqp = KRT_Q_START(KRTQ_H_INT);
    do {
	if (kqp->krtq_op == KRTQ_OP_NOP) {
	    (void) fprintf(fd, "\t%s: %d queued\n",
		trace_state(kernel_queue_name_bits, kqp->krtq_queue),
		kqp->krtq_n_queued);
	} else {
	    rt = kqp->krtq_rt;
	    switch (kqp->krtq_op) {
	    default:
		assert(FALSE);
	    case KRTQ_OP_ADD:
		dumpnew = "        ADD";
		break;
	    case KRTQ_OP_DELETE:
		dumpold = "     DELETE";
		break;
	    case KRTQ_OP_CHANGE:
		dumpold = "CHANGE FROM";
		dumpnew = "         TO";
		break;
	    }

	    if (dumpold) {
		(void) fprintf(fd, "\t\t%s %A/%A state <%s>",
			       dumpold,
			       rt->rt_dest,
			       rt->rt_dest_mask,
			       trace_bits(rt_state_bits, kqp->krtq_state));
		if (kqp->krtq_n_gw > 0) {
		    register int i;

		    (void) fprintf(fd, " gateway%s",
				   ((kqp->krtq_n_gw == 1) ? "" : "s"));
		    for (i = 0; i < kqp->krtq_n_gw; i++) {
			(void) fprintf(fd, " %A", kqp->krtq_routers[i]);
		    }
		}
		(void) fprintf(fd, "\n");
		dumpold = (const char *) 0;
	    }

	    if (dumpnew) {
		(void) fprintf(fd, "\t\t%s %A/%A state <%s>",
			       dumpnew,
			       rt->rt_dest,
			       rt->rt_dest_mask,
			       trace_bits(rt_state_bits, (rt->rt_state & KRT_RTS_BITS)));
		rtc = rt->rt_head->rth_changes;
		if (!BIT_TEST(rt->rt_state, KRT_RTS_NO_NEXTHOP)) {
#if	RT_N_MULTIPATH > 1
		    if (BIT_TEST(krt_rt_support, KRTS_MULTIPATH)) {
			register int i;
			int n_gw;
			sockaddr_un **routers;

			if (rtc) {
			    n_gw = rtc->rtc_n_gw;
			    routers = &(rtc->rtc_routers[0]);
			} else {
			    n_gw = rt->rt_n_gw;
			    routers = &(rt->rt_routers[0]);
			}

			(void) fprintf(fd, " gateway%s",
				       ((n_gw <= 1) ? "" : "s"));
			if (n_gw == 0) {
			    (void) fprintf(fd, " none!");
			} else {
			    for (i = 0; i < n_gw; i++) {
				(void) fprintf(fd, " %A", routers[i]);
			    }
			}
		    } else {
#endif	/* RT_N_MULTIPATH > 1 */
			if ((rtc ? rtc->rtc_n_gw : rt->rt_n_gw) == 0) {
			    (void) fprintf(fd, " gateway none!");
			} else {
			    (void) fprintf(fd, " gateway %A",
					   (rtc ? RTC_ROUTER(rtc) : RT_ROUTER(rt)));
			}
#if	RT_N_MULTIPATH > 1
		    }
#endif	/* RT_N_MULTIPATH > 1 */
		}

		(void) fprintf(fd, "\n");
		dumpnew = (const char *) 0;
	    }
	}

	kqp = kqp->krtq_forw;
    } while (kqp != KRT_Q_START(KRTQ_H_INT));


    /*
     * Dump the static gateways
     */
    if (krt_gw_list) {
	(void) fprintf(fd, "\tGateways referenced by kernel routes:\n");

	gw_dump(fd,
		"\t\t",
		krt_gw_list,
		tp->task_rtproto);

	(void) fprintf(fd, "\n");
    }

#if	defined(IP_MULTICAST) && !defined(KRT_IPMULTI_RTSOCK)
    krt_multicast_dump(tp, fd);
#endif	/* IP_MULTICAST  && !KRT_IPMULTI_RTSOCK */
}


static void
krt_reinit __PF1(tp, task *)
{
#ifdef	PROTO_SCRAM
    scram_reinit(tp);
#endif	/* PROTO_SCRAM */
    krt_warned = (flag_t) 0;
    if (krt_gwp_remnant->gw_n_routes) {
	krt_age((task_timer *) 0, (time_t) 0);
    }
}


void
krt_var_init __PF0(void)
{
    krt_flash_routes = KRT_FLASH_DEFAULT;	
    krt_flash_install_count = KRT_DEF_FLASH_INSTALL_COUNT;
    krt_install_priority = KRT_INSTALL_PRIO_DEFAULT;
    krt_install_count = KRT_DEF_INSTALL_COUNT;
    krt_limit_routes = KRT_COUNT_UNLIMITED;
    krt_t_expire = KRT_T_EXPIRE_DEFAULT;
    krt_options &= KRT_OPT_NOINSTALL;
}


void
krt_init __PF0(void)
{
    static int first = TRUE;
    
    /* Make sure we have the current tracing options */
    trace_inherit_global(kernel_trace_options, kernel_trace_types, TR_KRT_SYMBOLS|TR_KRT_IFLIST);
    trace_set(krt_task->task_trace, kernel_trace_options);

#ifdef	PROTO_INET
#ifdef	RTF_REJECT
    inet_addr_reject = sockdup(inet_addr_loopback);
    BIT_SET(krt_rt_support, KRTS_REJECT);
#endif	/* RTF_REJECT */
#ifdef	RTF_BLACKHOLE
    inet_addr_blackhole = sockdup(inet_addr_loopback);
    BIT_SET(krt_rt_support, KRTS_BLACKHOLE);
#endif	/* RTF_BLACKHOLE */
#endif	/* PROTO_INET */

    /* Only read kernel routing table once */
    if (first) {
	first = FALSE;
	
	rt_open(krt_task);

	/* Read the kernel's routing table */
	errno = krt_rtread(krt_task);
	if (errno
	    && !BIT_TEST(task_state, TASKS_TEST)) {
	    task_quit(errno);
	}

	/* Delete any routes we do not want to keep in kernel */
	krt_delq_process(krt_task);
	
	rt_close(krt_task, (gw_entry *) 0, 0, NULL);
    }

    if (krt_bg_priority != krt_bg_prio_map[krt_install_priority]) {
	krt_bg_priority = krt_bg_prio_map[krt_install_priority];
	if (krt_q_job) {
	    task_job_delete(krt_q_job);
	    krt_q_job = (task_job *) 0;
	    krt_q_schedule(FALSE);
	}
    }

    switch (krt_flash_routes) {
    default:
	assert(FALSE);
    case KRT_FLASH_INTERFACE:
	krt_flash_q_end = KRT_Q_END(KRTQ_INT_LAST);
	break;
    case KRT_FLASH_INTERNAL:
	krt_flash_q_end = KRT_Q_END(KRTQ_H_LAST);
	break;
    case KRT_FLASH_ALL:
	krt_flash_q_end = KRT_Q_END(KRTQ_L_LAST);
	break;
    }

#ifdef	IP_MULTICAST_ROUTING
    krt_init_mfc();
#endif	/* IP_MULTICAST_ROUTING */
}


 /*  Initilize the kernel routing table function.  First, create a	*/
 /*  task to hold the socket used in manipulating the kernel routing	*/
 /*  table. */
void
krt_family_init __PF0(void)
{
    flag_t save_task_state = task_state;
    register krt_q_entry *kqp;
    register int i;
#ifndef	KVM_TYPE_NONE
    KVM_OPEN_DEFINE(open_msg);
#endif	/* KVM_TYPE_NONE */

    trace_inherit_global(kernel_trace_options, kernel_trace_types, TR_KRT_SYMBOLS|TR_KRT_IFLIST);
    krt_task = task_alloc("KRT",
			  TASKPRI_KERNEL,
			  kernel_trace_options);
    krt_task->task_proto = IPPROTO_RAW;
    krt_task->task_rtproto = RTPROTO_KERNEL;
    task_set_terminate(krt_task, krt_terminate);
    task_set_shutdown(krt_task, krt_shutdown);
    BIT_RESET(task_state, TASKS_INIT|TASKS_TEST);
    task_set_ifachange(krt_task, krt_ifachange);
    task_set_dump(krt_task, krt_dump);
    task_set_cleanup(krt_task, krt_cleanup);
    task_set_reinit(krt_task, krt_reinit);
#ifdef	KRT_RT_SOCK
    task_set_recv(krt_task, krt_recv);
#endif	/* KRT_RT_SOCK */

#ifdef	SCO_OSR5
    /* We do not support a well-defined routing socket, but a
     * functionally equivalent end-point which serves our purpose
     * krt_task->task_socket = task_get_socket(PF_ROUTE, SOCK_RAW, AF_UNSPEC);
     */
    while ((krt_task->task_socket = open(_PATH_ROUTE, O_RDWR|O_NDELAY, 0)) < 0) {
	switch (errno) {
	case EAGAIN:	break;
	default:	trace_log_tp(krt_task, 0, LOG_ERR,
                         ("krt_family_init: %s: %m", _PATH_ROUTE));
			task_quit(errno);
			break;
	}
    }
    (void) ioctl (krt_task->task_socket, I_SRDOPT, RMSGD);

#else	/* SCO_OSR5 */
    krt_task->task_socket = task_get_socket(krt_task, KRT_SOCKET_TYPE);
#endif	/* SCO_OSR5 */
    task_state = save_task_state;

    /* Insure we have a socket */
    assert(BIT_TEST(task_state, TASKS_TEST) || krt_task->task_socket >= 0);

    krt_task->task_rtbit = rtbit_alloc(krt_task,
				       FALSE,
				       sizeof(krt_q_entry *),
				       (void_t) 0,
				       krt_tsi_dump);	/* Allocate a bit */
    if (!task_create(krt_task)) {
	task_quit(EINVAL);
    }

    /*
     * Initialize the queue structures
     */
    kqp = &krt_queue[KRTQ_N_PRIO-1];
    for (i = 0; i < KRTQ_N_PRIO; i++) {
	krt_queue[i].krtq_back = kqp;
	kqp = &krt_queue[i];
	kqp->krtq_forw = KRT_Q_NEXT(i);
	kqp->krtq_op = KRTQ_OP_NOP;
	kqp->krtq_queue = i;
	kqp->krtq_n_queued = 0;
    }

    /*
     * Explicitly initialize the state.
     */
    krt_state = KRT_STATE_NORMAL;
    krt_q_deferred = krt_q_time = (time_t) 0;
    krt_q_job = (task_job *) 0;
    krt_terminating = 0;
    krt_bg_priority = krt_bg_prio_map[krt_install_priority];

    switch (krt_flash_routes) {
    default:
	assert(FALSE);
    case KRT_FLASH_INTERFACE:
	krt_flash_q_end = KRT_Q_END(KRTQ_INT_LAST);
	break;
    case KRT_FLASH_INTERNAL:
	krt_flash_q_end = KRT_Q_END(KRTQ_H_LAST);
	break;
    case KRT_FLASH_ALL:
	krt_flash_q_end = KRT_Q_END(KRTQ_L_LAST);
	break;
    }
    /*
     * Fetch a task block for the queue entries
     */
    krt_q_block_index = task_block_init(sizeof(krt_q_entry), "krt_q_entry");
    krt_delq_index = task_block_init(sizeof(struct krt_delq_entry), "krt_delq_entry");

#if	RT_N_MULTIPATH > 1
    /* XXX nothing wrong with this */
    if (!BIT_MATCH(task_state, TASKS_TEST|TASKS_NODUMP)) {
	trace_log_tp(krt_task,
		     0,
		     LOG_WARNING,
		     ("krt_init: Configured for %d multipath routes, kernel only supports one!",
		      RT_N_MULTIPATH));
    }
#endif	/* RT_N_MULTIPATH > 1 */

    krt_gwp = gw_init((gw_entry *) 0,
		      krt_task->task_rtproto,
		      krt_task,
		      (as_t) 0,
		      (as_t) 0,
		      (sockaddr_un *) 0,
		      GWF_NOHOLD);

    krt_gwp_remnant = gw_init((gw_entry *) 0,
			      krt_task->task_rtproto,
			      krt_task,
			      (as_t) 0,
			      (as_t) 0,
			      (sockaddr_un *) 0,
			      (flag_t) 0);

    krt_gwp_temp =  gw_init((gw_entry *) 0,
			    krt_task->task_rtproto,
			    krt_task,
			    (as_t) 0,
			    (as_t) 0,
			    (sockaddr_un *) 0,
			    (flag_t) 0);

    krt_q_tip = task_timer_create(krt_task,
				  "Timeout",
				  (flag_t) 0,
				  (time_t) 0,
				  (time_t) 0,
				  krt_q_timeout,
				  (void_t) 0);

    krt_timer_ifcheck = task_timer_create(krt_task,
					  "IfCheck",
					  (flag_t) 0,
					  KRT_T_IFCHECK,
					  KRT_T_IFCHECK,	/* Prevent immediate firing */
					  krt_ifcheck_timer,
					  (void_t) 0);

#ifndef	KVM_TYPE_NONE
    kd = KVM_OPENFILES(NULL, NULL, NULL, O_RDONLY, open_msg);
    if (!kd) {
	if (!BIT_MATCH(task_state, TASKS_TEST|TASKS_NODUMP)) {
	    trace_log_tp(krt_task,
			 0,
			 LOG_ERR,
			 ("krt_init: %s",
			  KVM_OPEN_ERROR(open_msg)));
	}

	if (!BIT_TEST(task_state, TASKS_TEST)) {
	    task_quit(errno);
	}
    }
#endif	/* KVM_TYPE_NONE */

    errno = krt_symbols(krt_task);
    if (errno
	&& !BIT_TEST(task_state, TASKS_TEST)) {
	task_quit(errno);
    }

#ifdef	KRT_NETOPTS
    /* Check some kernel variables */
    errno = krt_netopts(krt_task);
    if (errno
	&& !BIT_TEST(task_state, TASKS_TEST)) {
	task_quit(errno);
    }
#endif	/* KRT_NETOPTS */

    /* Figure out the maximum packet size we can use */
    errno = krt_get_maxpacket(krt_task);
    if (errno
	&& !BIT_TEST(task_state, TASKS_TEST)) {
	task_quit(errno);
    }
    
#ifdef	PROTO_SCRAM    
    /* Hack */
    scram_init();
#endif	/* PROTO_SCRAM */

#ifdef	KRT_RT_SOCK
    /* Allocate buffer space */
    task_alloc_recv(krt_task, KRT_PACKET_MAX);
    task_alloc_send(krt_task, KRT_PACKET_MAX);

    if (task_set_option(krt_task,
			TASKOPTION_NONBLOCKING,
			TRUE) < 0) {
	task_quit(errno);
    }

    /* Set our receive buffer as high as possible so we don't miss any packets */
    if (task_set_option(krt_task,
			TASKOPTION_RECVBUF,
			task_maxpacket) < 0) {
	task_quit(errno);
    }

#ifdef	SCO_OSR5
    {
	/* Indicate we do not want to see our packets */
	struct strioctl sioc;
	sioc.ic_cmd = RTSTR_USELOOPBACK;
	sioc.ic_len = 0;
	sioc.ic_dp = (char *)0;
	sioc.ic_timout = -1;
	if (ioctl(krt_task->task_socket, I_STR, &sioc) < 0) {
            task_quit(errno);
	}
    }
#else	SCO_OSR5
    /* Indicate we do not want to see our packets */
    if (task_set_option(krt_task,
			TASKOPTION_USELOOPBACK,
			FALSE) < 0) {
	task_quit(errno);
    }
#endif	/* SCO_OSR5 */
#endif	/* KRT_RT_SOCK */

    /* Read the interface configuration */
    trace_only_tp(krt_task,
		  TRC_NL_BEFORE,
		  ("krt_init: Read kernel interface list"));
    (void) krt_ifread(save_task_state);
    trace_only_tp(krt_task,
		  0,
		  (NULL));

}
