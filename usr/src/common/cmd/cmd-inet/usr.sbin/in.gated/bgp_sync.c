#ident	"@(#)bgp_sync.c	1.4"
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
 * IBGP hack synchronization.  This module provides support for determining
 * immediate next hop(s) for routes learned in an IBGP "hack" group, by
 * tracking active IGP routes and looking up next hops which arrive with
 * IBGP routes in the tracked IGP information.  The immediate next hop(s)
 * for the IBGP route is (are) determined from the best-matching IGP route.
 * Note that IGP's which compute equal-cost multipath routes are fully
 * supported, the full set of next hops is inherited by the IBGP route.
 *
 * Both IBGP next hops and IGP routes are stored in a single radix trie.
 * IBGP next hops obviously end up attached to leaf nodes (i.e. 32-bit
 * mask nodes) while IGP routes fill in spaces closer to the root.  The
 * IGP route for an IBGP next hop can hence be determined by searching up
 * the tree towards the root until the first IGP route is encountered.
 * All IBGP routes affected by an IGP route change can be found by walking
 * the full tree below the IGP route, pruning at nodes which have other
 * IGP routes attached.  Since IGP route changes are discovered only during
 * flash updates, the IBGP nodes affected by IGP changes are queued and
 * processed by a foreground job after the flash completes.
 */

/*
 * Allocating and freeing next hop structures
 */
#define	BSY_NH_ALLOC(nh)	((nh)->bsynh_refcount++)

#define	BSY_NH_FREE(bsp, nh) \
    do { \
	if ((nh) == bsy_nexthop_if) { \
	    assert(bsy_nexthop_if->bsynh_refcount > 1); \
	    bsy_nexthop_if->bsynh_refcount--; \
	} else if ((nh)->bsynh_refcount == 1) { \
	    bgp_sync_nh_free((bsp), (nh)); \
	} else { \
	    (nh)->bsynh_refcount--; \
	} \
    } while (0)

/*
 * Memory allocation blocks and macros
 */
static block_t bsy_nexthop_block = (block_t) 0;
static block_t bsy_rt_internal_block = (block_t) 0;
static block_t bsy_igp_rt_block = (block_t) 0;
static block_t bsy_ibgp_rt_block = (block_t) 0;
static block_t bsy_nh_entry_block = (block_t) 0;
static block_t bgp_sync_block = (block_t) 0;

#define	BSY_BLOCK_INIT() \
    do { \
	bsy_nexthop_block = \
	    task_block_init(sizeof(bsy_nexthop), "bsy_nexthop"); \
	bsy_rt_internal_block = \
	    task_block_init(sizeof(bsy_rt_internal), "bsy_rt_internal"); \
	bsy_igp_rt_block = \
	    task_block_init(sizeof(bsy_igp_rt), "bsy_igp_rt"); \
	bsy_ibgp_rt_block = \
	    task_block_init(sizeof(bsy_ibgp_rt), "bsy_ibgp_rt"); \
	bsy_nh_entry_block = \
	    task_block_init(sizeof(bsy_nh_entry), "bsy_nh_entry"); \
	bgp_sync_block = task_block_init(sizeof(bgp_sync), "bgp_sync"); \
    } while (0)

#define	BSY_NEXTHOP_ALLOC() \
	((bsy_nexthop *)task_block_alloc(bsy_nexthop_block))
#define	BSY_RT_INTERNAL_ALLOC() \
	((bsy_rt_internal *)task_block_alloc(bsy_rt_internal_block))
#define	BSY_IGP_RT_ALLOC() \
	((bsy_igp_rt *)task_block_alloc(bsy_igp_rt_block))
#define	BSY_IBGP_RT_ALLOC() \
	((bsy_ibgp_rt *)task_block_alloc(bsy_ibgp_rt_block))
#define	BSY_NH_ENTRY_ALLOC() \
	((bsy_nh_entry *)task_block_alloc(bsy_nh_entry_block))
#define	BSY_SYNC_ALLOC() \
	((bgp_sync *)task_block_alloc(bgp_sync_block))


#define	BSY_NEXTHOP_FREE(arg) \
	task_block_free(bsy_nexthop_block, (void_t)(arg))
#define	BSY_RT_INTERNAL_FREE(arg) \
	task_block_free(bsy_rt_internal_block, (void_t)(arg))
#define	BSY_IGP_RT_FREE(bsp, arg) \
    do { \
	register bsy_igp_rt *Xirt = (arg); \
	if (Xirt->bsy_igp_nexthop) { \
	    BSY_NH_FREE((bsp), Xirt->bsy_igp_nexthop); \
	} \
	task_block_free(bsy_igp_rt_block, (void_t)Xirt); \
    } while (0)
#define	BSY_IBGP_RT_FREE(arg) \
	task_block_free(bsy_ibgp_rt_block, (void_t)(arg))
#define	BSY_NH_ENTRY_FREE(bsp, arg) \
    do { \
	register bsy_nh_entry *Xnhp = (arg); \
	if (Xnhp->bsyn_igp_nexthop) { \
	    BSY_NH_FREE((bsp), Xnhp->bsyn_igp_nexthop); \
	} \
	task_block_free(bsy_nh_entry_block, (void_t)Xnhp); \
    } while (0)
#define	BSY_SYNC_FREE(arg) \
	task_block_free(bgp_sync_block, (void_t)(arg))

/*
 * Macros to add/delete IBGP route entries from the next hop entry
 * they are attached to.  The first one adds an IBGP route to the
 * start of the list.
 */
#define	BSY_IBGP_RT_ADD_START(nhe, brt) \
    do { \
	register bsy_ibgp_rt *Xbrt = (brt); \
	register bsy_nh_entry *Xnhe = (nhe); \
	Xbrt->bsyb_prev = (bsy_ibgp_rt *)Xnhe; \
	(Xbrt->bsyb_next = Xnhe->bsyn_next)->bsyb_prev = Xbrt; \
	Xnhe->bsyn_next = Xbrt; \
    } while (0)

/*
 * This one adds the IBGP route entry to the end of the list
 */
#define	BSY_IBGP_RT_ADD_END(nhe, brt) \
    do { \
	register bsy_ibgp_rt *Xbrt = (brt); \
	register bsy_nh_entry *Xnhe = (nhe); \
	Xbrt->bsyb_next = (bsy_ibgp_rt *)Xnhe; \
	(Xbrt->bsyb_prev = Xnhe->bsyn_prev)->bsyb_next = Xbrt; \
	Xnhe->bsyn_prev = Xbrt; \
    } while (0)

/*
 * This one removes an IBGP route entry from the list it is on
 */
#define	BSY_IBGP_RT_REMOVE(brt) \
    do { \
	register bsy_ibgp_rt *Xbrt = (brt); \
	Xbrt->bsyb_next->bsyb_prev = Xbrt->bsyb_prev; \
	Xbrt->bsyb_prev->bsyb_next = Xbrt->bsyb_next; \
	Xbrt->bsyb_prev = Xbrt->bsyb_next = (bsy_ibgp_rt *) 0; \
    } while (0)

/*
 * Initialize a next hop entry
 */
#define	BSY_NH_ENTRY_INIT(nhe) \
    do { \
	register bsy_nh_entry *Xnhe = (nhe); \
	Xnhe->bsyn_next = Xnhe->bsyn_prev = (bsy_ibgp_rt *)Xnhe; \
    } while (0)

/*
 * Check to see if the last IBGP route was removed from a next hop entry.
 */
#define	BSY_NHE_ISEMPTY(nhe)	((nhe)->bsyn_next == ((bsy_ibgp_rt *)(nhe)))

/*
 * Preference we use for "unusable" routes
 */
#define	BSY_NO_PREF	(-1)

/*
 * Route TSI support.  For IGP/interface routes we are tracking we set an
 * rtbit and store a pointer to the igp_rt entry for the route in the
 * tsi field so we can find it again.
 */
#define	BSY_TSI_CLEAR(rth, bit)		rttsi_reset((rth), (bit))

#define	BSY_TSI_GET(rth, bit, irt) \
    do { \
	bsy_igp_rt *Xirt; \
	rttsi_get((rth), (bit), (byte *)(&Xirt)); \
	(irt) = Xirt; \
    } while (0)

#define	BSY_TSI_PUT(rth, bit, irt) \
    do { \
	bsy_igp_rt *Xirt = (irt); \
	rttsi_set((rth), (bit), (byte *)(&Xirt)); \
    } while (0)

#define	BSY_RTBIT_ALLOC(bsp) \
    (rtbit_alloc(bsp->bgp_sync_task, \
		 FALSE, \
		 sizeof(bsy_igp_rt *), \
		 (void_t) bsp, \
		 bgp_sync_tsi_dump))

/*
 * Maximum depth of the tree, in bits
 */
#define	BSY_DEPTH_HOST	(sizeof(bgp_nexthop) * NBBY)
#define	BSY_MAXDEPTH	(BSY_DEPTH_HOST + 1)

/*
 * Masks and bits.  We use these to build the tree.
 */
static const u_int32 bsy_bits[BSY_MAXDEPTH] = {
    0x80000000, 0x40000000, 0x20000000, 0x10000000,
    0x08000000, 0x04000000, 0x02000000, 0x01000000,
    0x00800000, 0x00400000, 0x00200000, 0x00100000,
    0x00080000, 0x00040000, 0x00020000, 0x00010000,
    0x00008000, 0x00004000, 0x00002000, 0x00001000,
    0x00000800, 0x00000400, 0x00000200, 0x00000100,
    0x00000080, 0x00000040, 0x00000020, 0x00000010,
    0x00000008, 0x00000004, 0x00000002, 0x00000001,
    0x00000000
};

static const u_int32 bsy_masks[BSY_MAXDEPTH] = {
    0x00000000,
    0x80000000, 0xc0000000, 0xe0000000, 0xf0000000,
    0xf8000000, 0xfc000000, 0xfe000000, 0xff000000,
    0xff800000, 0xffc00000, 0xffe00000, 0xfff00000,
    0xfff80000, 0xfffc0000, 0xfffe0000, 0xffff0000,
    0xffff8000, 0xffffc000, 0xffffe000, 0xfffff000,
    0xfffff800, 0xfffffc00, 0xfffffe00, 0xffffff00,
    0xffffff80, 0xffffffc0, 0xffffffe0, 0xfffffff0,
    0xfffffff8, 0xfffffffc, 0xfffffffe, 0xffffffff
};


/*
 * Determine the bit position of the first bit which differs in two
 * words.  This does a binary search to find the first byte, then
 * does a table lookup to find the bit within the byte.
 */
static const byte bsy_firstbit_lookup[256] = {
    8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

#define	BSY_FIND_FIRST_DIFFERENT(word1, word2, bit) \
    do { \
	register u_int32 Xtmp = (word1) ^ (word2); \
	if (Xtmp & 0xffff0000) { \
	    if (Xtmp & 0xff000000) { \
		(bit) = bsy_firstbit_lookup[Xtmp >> 24]; \
	    } else { \
		(bit) = bsy_firstbit_lookup[Xtmp >> 16] + 8; \
	    } \
	} else { \
	    if (Xtmp & 0x0000ff00) { \
		(bit) = bsy_firstbit_lookup[Xtmp >> 8] + 16; \
	    } else { \
		(bit) = bsy_firstbit_lookup[Xtmp] + 24; \
	    } \
	} \
    } while (0)

/*
 * Next hop kludge.  We keep next hops as vanilla IP addresses, filling
 * in the following sockaddr's with the next hops as needed to call
 * the rt_* routines.
 */
static sockaddr_un bsy_sockaddrs[RT_N_MULTIPATH];
static sockaddr_un *bsy_routers[RT_N_MULTIPATH];

/*
 * Macro to initialize the above structures.
 */
#define	BSY_ROUTERS_INIT() \
    do { \
	register int Xi; \
	for (Xi = 0; Xi < RT_N_MULTIPATH; Xi++) { \
	    sockclear_in(&(bsy_sockaddrs[Xi])); \
	    bsy_routers[Xi] = &(bsy_sockaddrs[Xi]); \
	} \
    } while (0)

/*
 * Yet Another Kludge (some people would call this a "design").  When
 * we match an interface route we can use the next hop which came
 * with the BGP route directly.  This is a "distinguished" next hop
 * structure with no contents to indicate this case.
 */
static bsy_nexthop	bsy_nexthop_interface;
#define	bsy_nexthop_if	(&bsy_nexthop_interface)

/*
 * Initialize the interface "next hop".
 */
#define	BSY_NEXTHOP_IF_INIT() \
    do { \
	bsy_nexthop_interface.bsynh_next = 0; \
	bsy_nexthop_interface.bsynh_refcount = 1; \
	bsy_nexthop_interface.bsynh_hash = 0xffff; \
	bsy_nexthop_interface.bsynh_n_gw = 0; \
    } while (0)

/*
 * Macro to set the next hops in the sockaddr structures, and
 * return the number of gateways, given a bsy_nh_entry struct.
 * Try to get the compiler to optimize this for the RT_N_MULTIPATH == 1
 * case.
 */
#define	BSY_GET_ROUTERS(nhe, n_gw) \
    do { \
	register bsy_nh_entry *Xnhe = (nhe); \
	register bsy_nexthop *Xnhp = Xnhe->bsyn_igp_nexthop; \
	if (Xnhp == bsy_nexthop_if) { \
	    sock2ip(&(bsy_sockaddrs[0])) \
	      = htonl(Xnhe->bsyn_ibgp_rti->bsyi_dest); \
	    (n_gw) = 1; \
	} else if (RT_N_MULTIPATH == 1)	{ \
	    sock2ip(&(bsy_sockaddrs[0])) = Xnhp->bsynh_nexthop[0]; \
	    (n_gw) = 1; \
	} else { \
	    register u_int Xi; \
	    for (Xi = 0; Xi < Xnhp->bsynh_n_gw; Xi++) { \
		sock2ip(&(bsy_sockaddrs[Xi])) = Xnhp->bsynh_nexthop[Xi]; \
	    } \
	    (n_gw) = Xi; \
	} \
    } while (0)


/*
 * bgp_sync_nh_alloc - find/create a next hop structure for the specified
 *		       next hops.
 */
static bsy_nexthop *
bgp_sync_nh_alloc __PF3(bsp, bgp_sync *,
			n_gw, int,
			gws, sockaddr_un **)
{
    register bsy_nexthop *nhp;
    register u_short hash;
    register int i;


    /*
     * Compute the hash and see if we can find a match for these
     */
    BSYNH_HASH(n_gw, gws, hash);
    for (nhp = bsp->bgp_sync_nh_hash[hash]; nhp; nhp = nhp->bsynh_next) {
	if (nhp->bsynh_n_gw == n_gw) {
	    for (i = 0; i < n_gw; i++) {
		if (sock2ip(gws[i]) != nhp->bsynh_nexthop[i]) {
		    break;
		}
	    }
	    if (i == n_gw) {
		BSY_NH_ALLOC(nhp);	/* matched it, return it */
		return nhp;
	    }
	}
    }

    /*
     * No match, create a new structure, record the next hops, and
     * return it.
     */
    nhp = BSY_NEXTHOP_ALLOC();
    nhp->bsynh_refcount = 1;
    nhp->bsynh_hash = hash;
    nhp->bsynh_n_gw = n_gw;
    for (i = 0; i < n_gw; i++) {
	nhp->bsynh_nexthop[i] = sock2ip(gws[i]);
    }
    nhp->bsynh_next = bsp->bgp_sync_nh_hash[hash];
    bsp->bgp_sync_nh_hash[hash] = nhp;
    bsp->bgp_sync_n_hashed++;
    return nhp;
}


/*
 * bgp_sync_nh_free - decrement the refcount on a next hop structure,
 *		      free it if the reference count drops to zero.
 */
static void
bgp_sync_nh_free __PF2(bsp, bgp_sync *,
		       nhp, bsy_nexthop *)
{
    register bsy_nexthop *nh_prev;

    if (--(nhp->bsynh_refcount) > 0) {
	return;
    }
    assert(nhp != bsy_nexthop_if);

    nh_prev = bsp->bgp_sync_nh_hash[nhp->bsynh_hash];
    assert(nh_prev);
    if (nh_prev == nhp) {
	bsp->bgp_sync_nh_hash[nhp->bsynh_hash] = nhp->bsynh_next;
    } else {
	while (nh_prev->bsynh_next != nhp) {
	    nh_prev = nh_prev->bsynh_next;
	}
	nh_prev->bsynh_next = nhp->bsynh_next;
    }
    bsp->bgp_sync_n_hashed--;
    BSY_NEXTHOP_FREE(nhp);
}


/*
 * bgp_sync_remove_internal - remove an internal node from the tree.
 *			      This removal may take out a second node
 *			      as well.
 */
static void
bgp_sync_remove_internal __PF2(bsp, bgp_sync *,
			       remove_inp, bsy_rt_internal *)
{
    register bsy_rt_internal *inp = remove_inp;
    register bsy_rt_internal *inp_next, *inp_prev;

    assert(!(inp->bsyi_nexthop) && !(inp->bsyi_igp_rt));

    /*
     * If this guy has no successors he's a goner, as may be the guy
     * above him.
     */
    if (!(inp->bsyi_left) && !(inp->bsyi_right)) {
	inp_prev = inp->bsyi_parent;
	BSY_RT_INTERNAL_FREE(inp);

	if (!inp_prev) {
	    /*
	     * Last guy in table, zero the tree.
	     */
	    bsp->bgp_sync_trie = (bsy_rt_internal *) 0;
	    return;
	}

	if (inp_prev->bsyi_left == inp) {
	    inp_prev->bsyi_left = (bsy_rt_internal *) 0;
	} else {
	    assert(inp_prev->bsyi_right == inp);
	    inp_prev->bsyi_right = (bsy_rt_internal *) 0;
	}

	if (inp_prev->bsyi_igp_rt) {
	    return;
	}
	inp = inp_prev;
    }

    /*
     * Here we have a one-way brancher with no external stuff attached
     * (either because this is what we were called with, or we made him
     * a one-way brancher above).  Remove him, promoting his remaining
     * child.
     */
    inp_prev = inp->bsyi_parent;
    if (inp->bsyi_left) {
	inp_next = inp->bsyi_left;
    } else {
	inp_next = inp->bsyi_right;
    }
    inp_next->bsyi_parent = inp_prev;

    if (!inp_prev) {
	/*
	 * Promoted to root node!
	 */
	bsp->bgp_sync_trie = inp_next;
    } else {
	/*
	 * We go where the deleted guy was before.
	 */
	if (inp_prev->bsyi_left == inp) {
	    inp_prev->bsyi_left = inp_next;
	} else {
	    assert(inp_prev->bsyi_right == inp);
	    inp_prev->bsyi_right = inp_next;
	}
    }

    /*
     * inp fully detached from table, blow him away too.
     */
    BSY_RT_INTERNAL_FREE(inp);
}


/*
 * bgp_sync_add_internal - add an internal node to the tree for the given
 *			   destination.  This is called by routines which
 *			   are adding IBGP next hop structures to the tree
 *			   so the dest is actually a host address and the
 *			   node we add will be a leaf.  The other type of
 *			   internal node add we do, for IGP routes, is done
 *			   inline in bgp_sync_igp_rt(), since it is
 *			   sufficiently different to merit its own code.
 */
static bsy_rt_internal *
bgp_sync_add_internal __PF3(bsp, bgp_sync *,
			    dest, u_int32,
			    depth, u_int)
{
    register bsy_rt_internal *inp, *inp_prev, *inp_add, *inp_new;
    register u_int32 split_bit, mybit, maxbit;
    int split;

    /*
     * Fetch the tree route, if there isn't one just allocate it
     * and return.
     */
    inp_prev = bsp->bgp_sync_trie;
    mybit = bsy_bits[depth];
    if (!inp_prev) {
	inp = BSY_RT_INTERNAL_ALLOC();
	inp->bsyi_dest = dest;
	inp->bsyi_bit = mybit;
	bsp->bgp_sync_trie = inp;
	return inp;
    }

    /*
     * Search down the tree until we can't go any further, or we
     * get to a place where the bit number is >= ours.
     */
    inp = inp_prev;
    while (inp->bsyi_bit > mybit) {
	if (BIT_TEST(dest, inp->bsyi_bit)) {
	    if (!(inp->bsyi_right)) {
		break;
	    }
	    inp = inp->bsyi_right;
	} else {
	    if (!(inp->bsyi_left)) {
		break;
	    }
	    inp = inp->bsyi_left;
	}
    }

    /*
     * Find where we differ from the guy we found
     */
    BSY_FIND_FIRST_DIFFERENT(dest, inp->bsyi_dest, split);
    split_bit = bsy_bits[split];
    maxbit = MAX(mybit, inp->bsyi_bit);
    if (split_bit < maxbit) {
	split_bit = maxbit;
    }

    /*
     * If the split_bit is greater than maxbit we will need to
     * insert a split above this guy.  Otherwise we're in the tree
     * above this guy or attached below him.
     */
    inp_prev = inp->bsyi_parent;
    while (inp_prev && inp_prev->bsyi_bit <= split_bit) {
	inp = inp_prev;
	inp_prev = inp->bsyi_parent;
    }

    /*
     * The IGP will call when it wants to find a node.  If there
     * is already a node in the tree to attach him to, do it now.
     */
    if (split_bit == mybit && inp->bsyi_bit == mybit) {
	assert(!(inp->bsyi_igp_rt));
	return inp;
    }

    /*
     * Allocate an internal node, at this point we're sure to
     * need one.
     */
    inp_add = BSY_RT_INTERNAL_ALLOC();
    inp_add->bsyi_dest = dest;
    inp_add->bsyi_bit = mybit;

    /*
     * We may attach below the guy we found.  This will be the case
     * if the bit number on the guy we found is equal to the split_bit
     */
    if (inp->bsyi_bit == split_bit) {
	assert(split_bit > mybit);
	inp_add->bsyi_parent = inp;
	if (BIT_TEST(dest, split_bit)) {
	    assert(!(inp->bsyi_right));
	    inp->bsyi_right = inp_add;
	} else {
	    assert(!(inp->bsyi_left));
	    inp->bsyi_left = inp_add;
	}
	return inp_add;
    }

    /*
     * Just two other cases.  If we are on the same branch as the
     * guy we found, we won't need to add a split node.  In this
     * case we insert inp_add between inp_prev and inp.  Otherwise
     * we add a split between inp_prev and inp, and append our node
     * to one side of it.
     */
    if (split_bit == mybit) {
	if (BIT_TEST(inp->bsyi_dest, mybit)) {
	    inp_add->bsyi_right = inp;
	} else {
	    inp_add->bsyi_left = inp;
	}
	inp_new = inp_add;
    } else {
	inp_new = BSY_RT_INTERNAL_ALLOC();
	inp_new->bsyi_dest = dest & bsy_masks[split];
	inp_new->bsyi_bit = split_bit;
	inp_add->bsyi_parent = inp_new;
	if (BIT_TEST(dest, split_bit)) {
	    inp_new->bsyi_right = inp_add;
	    inp_new->bsyi_left = inp;
	} else {
	    inp_new->bsyi_left = inp_add;
	    inp_new->bsyi_right = inp;
	}
    }

    inp->bsyi_parent = inp_new;
    inp_new->bsyi_parent = inp_prev;

    /*
     * If inp_prev is zero this is a new root node, otherwise he is
     * attached to the guy above in the place where inp was.
     */
    if (!inp_prev) {
	bsp->bgp_sync_trie = inp_new;
    } else if (inp_prev->bsyi_right == inp) {
	inp_prev->bsyi_right = inp_new;
    } else {
	assert(inp_prev->bsyi_left == inp);
	inp_prev->bsyi_left = inp_new;
    }

    /*
     * Done!  Return the new node.
     */
    return inp_add;
}



/*
 * bgp_sync_rt_add - replacement for rt_add() for the IBGP hack.  This
 *		     discovers the appropriate immediate next hops to
 *		     use for the route, makes preference comparisons
 *		     against any other IBGP routes to the same destination
 * 		     and then adds the route, either with the preference
 *		     set to the policy preference, or to (-1) if the
 *		     route is not usable.
 */
rt_entry *
bgp_sync_rt_add __PF4(bsp, bgp_sync *,
		      bnp, bgpPeer *,
		      rth, rt_head *,
		      rtp, rt_parms *)
{
    rt_parms rtparms;
    u_int32 nh;
    rt_entry *rt;
    bsy_rt_internal *inp;
    bsy_nh_entry *nhe;
    bsy_ibgp_rt *brt, *his_brt;

    /*
     * Copy over his parameters.
     */
    rtparms = *rtp;

    /*
     * Now find his internal node in the tree if there is one, if not add it.
     */
    nh = ntohl(sock2ip(rtparms.rtp_router));
    inp = bsp->bgp_sync_trie;
    while (inp && inp->bsyi_bit) {
	if (BIT_TEST(nh, inp->bsyi_bit)) {
	    inp = inp->bsyi_right;
	} else {
	    inp = inp->bsyi_left;
	}
    }

    if (!inp || inp->bsyi_dest != nh) {
	inp = bgp_sync_add_internal(bsp, nh, BSY_DEPTH_HOST);
    }

    /*
     * Fetch his next hop entry from the internal node.  If there
     * isn't one make one and search up the tree to find his nexthop/metric.
     */
    nhe = inp->bsyi_nexthop;
    if (!nhe) {
	nhe = BSY_NH_ENTRY_ALLOC();
	BSY_NH_ENTRY_INIT(nhe);
	nhe->bsyn_ibgp_rti = inp;
	inp->bsyi_nexthop = nhe;
	do {
	    if (inp->bsyi_igp_rt) {
		nhe->bsyn_igp_metric = inp->bsyi_igp_rt->bsy_igp_metric;
		nhe->bsyn_igp_nexthop = inp->bsyi_igp_rt->bsy_igp_nexthop;
		BSY_NH_ALLOC(nhe->bsyn_igp_nexthop);
		break;
	    }
	    inp = inp->bsyi_parent;
	} while (inp);
    }

    /*
     * Okay, we've got his next hop entry.  Allocate him an IBGP route
     * entry and determine if there is another route to this destination
     * from the same IBGP group.  This'll let us set the ALT flag correctly,
     * and we'll also need to figure out which of these guys is better.
     */
    brt = BSY_IBGP_RT_ALLOC();
    brt->bsyb_nh = nhe;
    brt->bsyb_pref = rtparms.rtp_preference;

    if (!rth) {
	if (nhe->bsyn_igp_nexthop) {
	    BIT_SET(brt->bsyb_flags, BSYBF_ACTIVE);
	}
    } else {
	register int n = 0;

	his_brt = (bsy_ibgp_rt *) 0;
	RT_ALLRT(rt, rth) {
	    if (BIT_TEST(rt->rt_state, RTS_DELETE)) {
		rt = (rt_entry *) 0;
		break;
	    }
	    if (rt->rt_gwp->gw_proto == RTPROTO_BGP
	      && rt->rt_gwp->gw_data == (void_t) bsp) {
		BIT_SET(brt->bsyb_flags, BSYBF_ALT);
		his_brt = (bsy_ibgp_rt *)(rt->rt_data);
		assert(his_brt);
		if (!BIT_TEST(his_brt->bsyb_flags, BSYBF_ALT)) {
		    /*
		     * Won't be any more.  Set the alt bit, move him
		     * to the front of his next hop list and break.
		     */
		    assert(n == 0);
		    BIT_SET(his_brt->bsyb_flags, BSYBF_ALT);
		    BSY_IBGP_RT_REMOVE(his_brt);
		    BSY_IBGP_RT_ADD_START(his_brt->bsyb_nh, his_brt);
		    if (!BIT_TEST(his_brt->bsyb_flags, BSYBF_ACTIVE)) {
			rt = (rt_entry *) 0;
		    }
		    break;
		}
		if (!(nhe->bsyn_igp_nexthop)) {
		    break;
		}
		if (BIT_TEST(his_brt->bsyb_flags, BSYBF_ACTIVE)) {
		    break;
		}
		n++;
	    }
	} RT_ALLRT_END(rt, rth);

	if (nhe->bsyn_igp_nexthop) {
	    if (!rt) {
		BIT_SET(brt->bsyb_flags, BSYBF_ACTIVE);
	    } else {
		/*
		 * This guy is active, see if we like our route better than his.
		 * Otherwise compare metrics to determine who is better.
		 */
		int i_am_better = 0;

		/*
		 * XXX this is an amazing crock of shit.  Convert the
		 * IBGP3 to better higher as soon as rcp_routed is gone!
		 */
		metric_t his_metric, my_metric;
		bgpPeer *his_bnp;

		his_bnp = (bgpPeer *)(rt->rt_gwp->gw_task->task_data);
		if (BGP_VERSION_2OR3(bnp->bgp_version)) {
		    my_metric = BGP_METRIC_3TO4(rtparms.rtp_metric2);
		} else {
		    my_metric = rtparms.rtp_metric2;
		}
		if (BGP_VERSION_2OR3(his_bnp->bgp_version)) {
		    his_metric = BGP_METRIC_3TO4(rt->rt_metric2);
		} else {
		    his_metric = rt->rt_metric2;
		}

		if (my_metric > his_metric) {
		    i_am_better = 1;
		} else if (my_metric == his_metric) {
		    if (rtparms.rtp_metric != BGP_METRIC_NONE
		      && (rt->rt_metric == BGP_METRIC_NONE
			|| rt->rt_metric > rtparms.rtp_metric)) {
			i_am_better = 1;
		    } else if (rtparms.rtp_metric == rt->rt_metric) {
			if (his_brt->bsyb_nh->bsyn_igp_metric
			  > nhe->bsyn_igp_metric) {
			    i_am_better = 1;
			} else if (his_brt->bsyb_nh->bsyn_igp_metric
				  == nhe->bsyn_igp_metric) {
			    if (ntohl(bnp->bgp_id) < ntohl(his_bnp->bgp_id)) {
				i_am_better = 1;
			    }
			}
		    }
		}

		if (i_am_better) {
		    BIT_SET(brt->bsyb_flags, BSYBF_ACTIVE);
		    BIT_RESET(his_brt->bsyb_flags, BSYBF_ACTIVE);
		    if (rt->rt_preference >= 0) {
			(void) rt_change_aspath(rt,
						rt->rt_metric,
						rt->rt_metric2,
						rt->rt_tag,
						BSY_NO_PREF,
						rt->rt_preference2,
						rt->rt_n_gw,
						rt->rt_routers,
						rt->rt_aspath);
		    }
		}
	    }
	}
    }

    /*
     * Finally.  Add the route and queue up its entry.
     */
    rtparms.rtp_rtd = (void_t) brt;
    if (nhe->bsyn_igp_nexthop) {
	register int i;

	BSY_GET_ROUTERS(nhe, i);
	rtparms.rtp_n_gw = i;
	while (i--) {
	    rtparms.rtp_routers[i] = &(bsy_sockaddrs[i]);
	}
    } else {
	rtparms.rtp_n_gw = 0;
    }

    if (!BIT_TEST(brt->bsyb_flags, BSYBF_ACTIVE)) {
	rtparms.rtp_preference = BSY_NO_PREF;
    }

    rt = rt_add(&rtparms);
    assert(rt);
    brt->bsyb_rt = rt;

    /*
     * One job left.  Add the IBGP route entry to the next hop list.
     * Add it at the start of the list if it is an alt entry, the
     * end of the list otherwise.
     */
    if (BIT_TEST(brt->bsyb_flags, BSYBF_ALT)) {
	BSY_IBGP_RT_ADD_START(nhe, brt);
    } else {
	BSY_IBGP_RT_ADD_END(nhe, brt);
    }

    return rt;
}



/*
 * bgp_sync_rt_change - replacement for rt_change() for the IBGP hack.  This
 *			discovers the appropriate immediate next hops to
 *			use for the route, makes preference comparisons
 *			against any other IBGP routes to the same destination
 * 			and then changes the route, either with the preference
 *			set to the policy preference, or to (-1) if the
 *			route is not usable.
 */
rt_entry *
bgp_sync_rt_change __PF11(bsp, bgp_sync *,
		 	  bnp, bgpPeer *,
			  rt, rt_entry *,
			  metric, metric_t,
			  localpref, metric_t,
			  tag, tag_t,
			  pref, pref_t,
			  pref2, pref_t,
			  n_gw, int,
			  nexthops, sockaddr_un **,
			  asp, as_path *)
{
    u_int32 nh;
    bsy_rt_internal *inp;
    bsy_nh_entry *nhe;
    bsy_ibgp_rt *brt, *his_brt;
    metric_t new_metric, old_metric;
    pref_t preference;
    int nh_changed = 0;
    rt_head *rth = rt->rt_head;

    /*
     * A few sanity checks.  Then fetch his route pointer
     */
    assert(rt->rt_data);
    brt = (bsy_ibgp_rt *) (rt->rt_data);
    nhe = brt->bsyb_nh;
    inp = nhe->bsyn_ibgp_rti;

    /*
     * Catch an easy case.  If he's giving us the next hops from
     * the route we know the only thing which might have changed is the
     * preference.  This will only have an effect on anything if
     * the preference actually changed and if the route is active.
     */
    if (n_gw == rt->rt_n_gw && nexthops == rt->rt_routers) {
	if (brt->bsyb_pref != pref || pref2 != rt->rt_preference2) {
	    brt->bsyb_pref = pref;
	    if (BIT_TEST(brt->bsyb_flags, BSYBF_ACTIVE)) {
		return rt_change_aspath(rt,
					metric,
					localpref,
					tag,
					pref,
					pref2,
					n_gw,
					nexthops,
					asp);
	    }
	}
	return rt;
    }
    assert(n_gw == 1);

    /*
     * If his next hop changed we've got to rearrange his position
     * in the tree.  Do this now.
     */
    nh = ntohl(sock2ip(nexthops[0]));
    if (inp->bsyi_dest != nh) {
	/*
	 * Now find his internal node in the tree if there is one, if not
	 * add it.
	 */
	register bsy_rt_internal *new_inp = bsp->bgp_sync_trie;
	register bsy_nh_entry *new_nhe;

	while (new_inp && new_inp->bsyi_bit) {
	    if (BIT_TEST(nh, new_inp->bsyi_bit)) {
		new_inp = new_inp->bsyi_right;
	    } else {
		new_inp = new_inp->bsyi_left;
	    }
	}

	if (!new_inp || new_inp->bsyi_dest != nh) {
	    new_inp = bgp_sync_add_internal(bsp, nh, BSY_DEPTH_HOST);
	}

	/*
	 * Fetch the new next hop entry from the internal node.  If there
	 * isn't one allocate a new one and search up the tree to find
	 * the corresponding IGP next hop/metrics.
	 */
	new_nhe = new_inp->bsyi_nexthop;
	if (!new_nhe) {
	    new_nhe = BSY_NH_ENTRY_ALLOC();
	    BSY_NH_ENTRY_INIT(new_nhe);
	    new_nhe->bsyn_ibgp_rti = new_inp;
	    new_inp->bsyi_nexthop = new_nhe;
	    do {
		if (new_inp->bsyi_igp_rt) {
		    new_nhe->bsyn_igp_metric
		      = new_inp->bsyi_igp_rt->bsy_igp_metric;
		    new_nhe->bsyn_igp_nexthop
		      = new_inp->bsyi_igp_rt->bsy_igp_nexthop;
		    BSY_NH_ALLOC(new_nhe->bsyn_igp_nexthop);
		    break;
		}
		new_inp = new_inp->bsyi_parent;
	    } while (new_inp);
	}

	/*
	 * Okay, we now have a new next hop entry for him.  Detach
	 * his IBGP RT entry from the old next hop entry and attach
	 * it to the new one.  Note that the detach may make the old
	 * next hop entry, and even the associated internal node, go away.
	 */
	BSY_IBGP_RT_REMOVE(brt);
	if (BIT_TEST(brt->bsyb_flags, BSYBF_ALT)) {
	    BSY_IBGP_RT_ADD_START(new_nhe, brt);
	} else {
	    BSY_IBGP_RT_ADD_END(new_nhe, brt);
	}
	brt->bsyb_nh = new_nhe;
	if (BSY_NHE_ISEMPTY(nhe)) {
	    BSY_NH_ENTRY_FREE(bsp, nhe);
	    inp->bsyi_nexthop = (bsy_nh_entry *) 0;
	    if (!inp->bsyi_igp_rt) {
		bgp_sync_remove_internal(bsp, inp);
	    }
	}
	nhe = new_nhe;
	nh_changed = 1;
    }

    /*
     * If the next hop (and hence the IGP metric) changed, or if the
     * localpref changed, and if there are alternate routes to the
     * same destination, we will need to see how the current route
     * compares to the other routes to the same destination.  If there
     * are no alternate routes then a change to the next hop may make
     * this route active/inactive.  Check it out
     */
    brt->bsyb_pref = pref;
    if (nh_changed && !BIT_TEST(brt->bsyb_flags, BSYBF_ALT)) {
	if (!nhe->bsyn_igp_nexthop) {
	    BIT_RESET(brt->bsyb_flags, BSYBF_ACTIVE);
	} else {
	    BIT_SET(brt->bsyb_flags, BSYBF_ACTIVE);
	}
    } else if (BIT_TEST(brt->bsyb_flags, BSYBF_ALT) && (nh_changed
      || rt->rt_metric2 != localpref || rt->rt_metric != metric)) {
	register rt_entry *his_rt, *best_rt;
	register bsy_ibgp_rt *best_brt;
	metric_t best_metric;

	if (nh_changed
	  && !BIT_TEST(brt->bsyb_flags, BSYBF_ACTIVE)
	  && !nhe->bsyn_igp_nexthop) {
	    goto do_change;
	}

	/*
	 * XXX Crock of shit warning
	 */
	if (BGP_VERSION_2OR3(bnp->bgp_version)) {
	    old_metric = BGP_METRIC_3TO4(rt->rt_metric2);
	    new_metric = BGP_METRIC_3TO4(localpref);
	} else {
	    old_metric = rt->rt_metric2;
	    new_metric = localpref;
	}

	/*
	 * If the next hop didn't change we may be able to avoid
	 * doing this.  If he is active and the metric got better,
	 * don't bother.  If he is inactive and the metric got
	 * worse, don't bother either.
	 */
	if (!nh_changed) {
	    if (BIT_TEST(brt->bsyb_flags, BSYBF_ACTIVE)) {
		if (new_metric > old_metric) {
		    goto do_change;
		}
		if (new_metric == old_metric
		  && (rt->rt_metric == BGP_METRIC_NONE
		  || (metric != BGP_METRIC_NONE && metric < rt->rt_metric))) {
		    goto do_change;
		}
	    } else {
		if (new_metric < old_metric) {
		    goto do_change;
		}
		if (new_metric == old_metric && (metric == BGP_METRIC_NONE
		  || (rt->rt_metric != BGP_METRIC_NONE
		  && metric >= rt->rt_metric))) {
		    goto do_change;
		}
	    }
	}

	/*
	 * See if there is a better guy.  If so, he'll go active
	 * and we'll go inactive.
	 */
	if (nhe->bsyn_igp_nexthop) {
	    best_rt = rt;
	    best_brt = brt;
	    best_metric = metric;
	} else {
	    best_rt = (rt_entry *) 0;
	    best_brt = (bsy_ibgp_rt *) 0;
	    best_metric = BGP_METRIC_NONE;
	}

	RT_ALLRT(his_rt, rth) {
	    if (BIT_TEST(his_rt->rt_state, RTS_DELETE)) {
		break;
	    }
	    if (his_rt == rt) {
		continue;
	    }
	    if (his_rt->rt_gwp->gw_proto == RTPROTO_BGP
	     && his_rt->rt_gwp->gw_data == (void_t) bsp) {
		bgpPeer *his_bnp;
		int his_is_better = 0;

		his_brt = (bsy_ibgp_rt *)(his_rt->rt_data);
		assert(his_brt && BIT_TEST(his_brt->bsyb_flags, BSYBF_ALT));
		if (!(his_brt->bsyb_nh->bsyn_igp_nexthop)) {
		    continue;
		}
		his_bnp = (bgpPeer *)(his_rt->rt_gwp->gw_task->task_data);
		if (BGP_VERSION_2OR3(his_bnp->bgp_version)) {
		    old_metric = BGP_METRIC_3TO4(his_rt->rt_metric2);
		} else {
		    old_metric = his_rt->rt_metric2;
		}

		if (!best_rt) {
		    his_is_better = 1;
		} else {
		    if (old_metric > new_metric) {
			his_is_better = 1;
		    } else if (old_metric == new_metric) {
			if (his_rt->rt_metric != BGP_METRIC_NONE
			  && (best_metric == BGP_METRIC_NONE
			    || his_rt->rt_metric < best_metric)) {
			    his_is_better = 1;
			} else if (best_metric == his_rt->rt_metric) {
			    if (his_brt->bsyb_nh->bsyn_igp_metric
			      < brt->bsyb_nh->bsyn_igp_metric) {
				his_is_better = 1;
			    } else if (his_brt->bsyb_nh->bsyn_igp_metric
			      == brt->bsyb_nh->bsyn_igp_metric) {
				if (ntohl(his_bnp->bgp_id)
				  < ntohl(bnp->bgp_id)) {
				    his_is_better = 1;
				}
			    }
			}
		    }
		}

		if (his_is_better) {
		    best_rt = his_rt;
		    best_brt = his_brt;
		    new_metric = old_metric;
		    best_metric = his_rt->rt_metric;
		    if (BIT_TEST(his_brt->bsyb_flags, BSYBF_ACTIVE)) {
			break;
		    }
		} else if (BIT_TEST(his_brt->bsyb_flags, BSYBF_ACTIVE)) {
		    BIT_RESET(his_brt->bsyb_flags, BSYBF_ACTIVE);
		    (void) rt_change_aspath(his_rt,
					    his_rt->rt_metric,
					    his_rt->rt_metric2,
					    his_rt->rt_tag,
					    BSY_NO_PREF,
					    his_rt->rt_preference2,
					    his_rt->rt_n_gw,
					    his_rt->rt_routers,
					    his_rt->rt_aspath);
		    rt_refresh(his_rt);
		}
	    }
	} RT_ALLRT_END(his_rt, rth);

	if (best_rt == rt) {
	    BIT_SET(brt->bsyb_flags, BSYBF_ACTIVE);
	} else {
	    BIT_RESET(brt->bsyb_flags, BSYBF_ACTIVE);
	    if (best_rt && !BIT_TEST(best_brt->bsyb_flags, BSYBF_ACTIVE)) {
		BIT_SET(best_brt->bsyb_flags, BSYBF_ACTIVE);
		(void) rt_change_aspath(best_rt,
					best_rt->rt_metric,
					best_rt->rt_metric2,
					best_rt->rt_tag,
					best_brt->bsyb_pref,
					best_rt->rt_preference2,
					best_rt->rt_n_gw,
					best_rt->rt_routers,
					best_rt->rt_aspath);
		rt_refresh(best_rt);
	    }
	}
    }

do_change:
    if (BIT_TEST(brt->bsyb_flags, BSYBF_ACTIVE)) {
	preference = pref;
    } else {
	preference = BSY_NO_PREF;
    }

    if (nh_changed) {
	register int n;

	if (nhe->bsyn_igp_nexthop) {
	    BSY_GET_ROUTERS(nhe, n);
	} else {
	    n = 0;
	}

	rt_refresh(rt);
	return rt_change_aspath(rt,
				metric,
				localpref,
				tag,
				preference,
				pref2,
				n,
				bsy_routers,
				asp);
    }

    rt_refresh(rt);
    return rt_change_aspath(rt,
			    metric,
			    localpref,
			    tag,
			    preference,
			    pref2,
			    rt->rt_n_gw,
			    rt->rt_routers,
			    asp);
}


/*
 * bgp_sync_rt_delete - replacement for rt_delete() for IGP-synchronized
 *			peers.  This manages the deletion of the
 *			synchronization data structures involved with
 *			this route, and the promotion to active of
 *			any alternate route to the destination.  An
 *			rt_delete() is then performed on the route.
 */
void
bgp_sync_rt_delete __PF2(bsp, bgp_sync *,
			 rt, rt_entry *)
{
    bsy_rt_internal *inp;
    bsy_nh_entry *nhe;
    bsy_ibgp_rt *brt, *best_brt;
    rt_entry *best_rt, *other_rt;
    rt_head *rth = rt->rt_head;
    int check_for_new_active;

    /*
     * Fetch pointers to the useful stuff
     */
    brt = (bsy_ibgp_rt *)(rt->rt_data);
    assert(brt);
    nhe = brt->bsyb_nh;
    inp = nhe->bsyn_ibgp_rti;

    /*
     * Remove the IBGP route entry from the tree.  This may cause us
     * to delete the next hop entry it was attached to, and perhaps
     * the internal node it was attached to.
     */
    BSY_IBGP_RT_REMOVE(brt);
    if (BSY_NHE_ISEMPTY(nhe)) {
	BSY_NH_ENTRY_FREE(bsp, nhe);
	inp->bsyi_nexthop = (bsy_nh_entry *) 0;
	if (!inp->bsyi_igp_rt) {
	    bgp_sync_remove_internal(bsp, inp);
	}
    }

    /*
     * So far so good.  If there are no alternate routes, we're done with
     * this guy.  If there is an alternate route someone else may become
     * active if this guy was active, or we may need to reset the state
     * of the alternate bit on the other guy if he is the last left.
     */
    if (BIT_TEST(brt->bsyb_flags, BSYBF_ALT)) {
	int n = 0;
	metric_t best_metric;
	rt_entry *his_rt;
	bsy_ibgp_rt *his_brt;
	bgpPeer *best_bnp;

	if (BIT_TEST(brt->bsyb_flags, BSYBF_ACTIVE)) {
	    check_for_new_active = 1;
	} else {
	    check_for_new_active = 0;
	}
	best_rt = (rt_entry *) 0;
	best_brt = (bsy_ibgp_rt *) 0;
	best_metric = 0;
	best_bnp = (bgpPeer *) 0;

	other_rt = (rt_entry *) 0;
	RT_ALLRT(his_rt, rth) {
	    if (BIT_TEST(his_rt->rt_state, RTS_DELETE)) {
		break;
	    }
	    if (his_rt->rt_gwp->gw_proto != RTPROTO_BGP
	     || his_rt->rt_gwp->gw_data != (void_t) bsp
	     || his_rt == rt) {
		continue;
	    }

	    if (++n > 1) {
		other_rt = (rt_entry *) 0;
		if (!check_for_new_active) {
		    break;
		}
	    } else {
		other_rt = his_rt;
	    }

	    if (check_for_new_active) {
		register bgpPeer *his_bnp;
		register int his_is_better = 0;
		register int his_metric;

		his_brt = (bsy_ibgp_rt *)(his_rt->rt_data);
		assert(his_brt && BIT_TEST(his_brt->bsyb_flags, BSYBF_ALT));
		if (!(his_brt->bsyb_nh->bsyn_igp_nexthop)) {
		    continue;
		}
		his_bnp = (bgpPeer *)(his_rt->rt_gwp->gw_task->task_data);
		if (BGP_VERSION_2OR3(his_bnp->bgp_version)) {
		    his_metric = BGP_METRIC_3TO4(his_rt->rt_metric2);
		} else {
		    his_metric = his_rt->rt_metric2;
		}

		if (!best_rt) {
		    his_is_better = 1;
		} else {
		    if (his_metric > best_metric) {
			his_is_better = 1;
		    } else if (his_metric == best_metric) {
			if (his_rt->rt_metric != BGP_METRIC_NONE
			  && (best_rt->rt_metric == BGP_METRIC_NONE
			    || his_rt->rt_metric < best_rt->rt_metric)) {
			    his_is_better = 1;
			} else if (his_rt->rt_metric == best_rt->rt_metric) {
			    if (his_brt->bsyb_nh->bsyn_igp_metric
			      < best_brt->bsyb_nh->bsyn_igp_metric) {
				his_is_better = 1;
			    } else if (his_brt->bsyb_nh->bsyn_igp_metric
			      == best_brt->bsyb_nh->bsyn_igp_metric) {
				if (ntohl(his_bnp->bgp_id)
				  < ntohl(best_bnp->bgp_id)) {
				    his_is_better = 1;
				}
			    }
			}
		    }
		}

		if (his_is_better) {
		    best_rt = his_rt;
		    best_brt = his_brt;
		    best_metric = his_metric;
		    best_bnp = his_bnp;
		}
	    }
	} RT_ALLRT_END(his_rt, rth);

	if (other_rt) {
	    register bsy_ibgp_rt *obrt = (bsy_ibgp_rt *)(other_rt->rt_data);

	    /*
	     * Move it to the end of the list for its next hop
	     * entry.
	     */
	    BIT_RESET(obrt->bsyb_flags, BSYBF_ALT);
	    BSY_IBGP_RT_REMOVE(obrt);
	    BSY_IBGP_RT_ADD_END(obrt->bsyb_nh, obrt);
	}
	if (best_rt) {
	    BIT_SET(best_brt->bsyb_flags, BSYBF_ACTIVE);
	    (void) rt_change_aspath(best_rt,
				    best_rt->rt_metric,
				    best_rt->rt_metric2,
				    best_rt->rt_tag,
				    best_brt->bsyb_pref,
				    best_rt->rt_preference2,
				    best_rt->rt_n_gw,
				    best_rt->rt_routers,
				    best_rt->rt_aspath);
	    rt_refresh(best_rt);
	}
    }

    /*
     * Now free the IBGP route entry and blow away the route.
     */
    rt->rt_data = (void_t) 0;
    BSY_IBGP_RT_FREE(brt);
    rt_delete(rt);
}


/*
 * bgp_sync_igp_change - run from a task job to process changes to IBGP
 *			 routes resulting from an earlier change to an IGP
 *			 route.
 */
static void
bgp_sync_igp_change __PF1(jp, task_job *)
{
    bsy_nh_entry *nhe, *nhe_next;
    bsy_ibgp_rt *brt;
    bgp_sync *bsp = (bgp_sync *)(jp->task_job_data);
    rt_entry *rt;
    pref_t pref;
    int nh_changed, pref_changed;
    int n_gw, n;
    sockaddr_un **new_nexthops;

    assert(bsp && bsp->bgp_sync_nh_changes);
    nhe = bsp->bgp_sync_nh_changes;
    nhe_next = nhe->bsyn_change_next;

    rt_open(bsp->bgp_sync_task);

    do {
	register bsy_rt_internal *inp = nhe->bsyn_ibgp_rti;
	register bsy_igp_rt *irt;
	register bsy_nexthop *nhp;

	nhe->bsyn_change_next = (bsy_nh_entry *) 0;

	/*
	 * Check to see if this next hop entry is empty.  If so, just
	 * remove it, remove its internal node from the tree if we need
	 * to, then continue.
	 */
	if (BSY_NHE_ISEMPTY(nhe)) {
	    if (nhe_next == nhe) {
		nhe_next = (bsy_nh_entry *) 0;
	    }
	    BSY_NH_ENTRY_FREE(bsp, nhe);
	    inp->bsyi_nexthop = (bsy_nh_entry *) 0;
	    if (!inp->bsyi_igp_rt) {
		bgp_sync_remove_internal(bsp, inp);
	    }
	    continue;
	}

	/*
	 * Walk up the tree to find the current IGP route for this guy,
	 * then determine what has changed.  The possibilities are the
	 * next hop, the local preference (should an IGP metric have
	 * changed, or an IGP route been added/deleted) or both.
	 */
	irt = (bsy_igp_rt *) 0;
	do {
	    if ((irt = inp->bsyi_igp_rt)) {
		break;
	    }
	} while ((inp = inp->bsyi_parent));

	nh_changed = 1;
	pref_changed = 0;
	nhp = (bsy_nexthop *) 0;
	if (irt) {
	    nhp = irt->bsy_igp_nexthop;
	    if (nhe->bsyn_igp_nexthop) {
		if (nhp != bsy_nexthop_if && nhe->bsyn_igp_nexthop == nhp) {
		    nh_changed = 0;
		}
		if (nhe->bsyn_igp_metric > irt->bsy_igp_metric) {
		    pref_changed = 1;
		} else if (nhe->bsyn_igp_metric < irt->bsy_igp_metric) {
		    pref_changed = (-1);
		}
		BSY_NH_FREE(bsp, nhe->bsyn_igp_nexthop);
	    } else {
		pref_changed = 1;
	    }
	    BSY_NH_ALLOC(nhe->bsyn_igp_nexthop = nhp);
	    nhe->bsyn_igp_metric = irt->bsy_igp_metric;
	} else if (!(nhe->bsyn_igp_nexthop)) {
	    nh_changed = 0;
	} else {
	    BSY_NH_FREE(bsp, nhe->bsyn_igp_nexthop);
	    nhe->bsyn_igp_nexthop = (bsy_nexthop *) 0;
	    nhe->bsyn_igp_metric = (metric_t) 0;
	    pref_changed = (-1);
	}

	/*
	 * If we've had changes we'll need to process the routes attached
	 * to the next hop entry.  If not just continue.
	 */
	if (!nh_changed && !pref_changed) {
	    continue;
	}

	/*
	 * Move the new next hops into our array if we have them, we'll
	 * need to change each route to these.
	 */
	n_gw = 0;
	if (nh_changed && nhp) {
	    BSY_GET_ROUTERS(nhe, n_gw);
	}

	for (brt = nhe->bsyn_next;
	     brt != ((bsy_ibgp_rt *)(nhe));
	     brt = brt->bsyb_next) {
	    /*
	     * Process the case where there are no alternate routes
	     * separately.  This is a fair bit easier.
	     */
	    rt = brt->bsyb_rt;
	    if (!BIT_TEST(brt->bsyb_flags, BSYBF_ALT)) {
		if (!nh_changed) {
		    break;		/* All done! */
		}
		if (!irt) {
		    BIT_RESET(brt->bsyb_flags, BSYBF_ACTIVE);
		} else {
		    BIT_SET(brt->bsyb_flags, BSYBF_ACTIVE);
		}
	    } else {
		/*
		 * If the preference didn't change, or if he's already
		 * active and the preference got better, or if he's
		 * not active and the preference got worse, we'll simply
		 * need to give this guy a new set of next hops.  Otherwise
		 * we'll need to know how he competes with the alternates.
		 */
		if (!BIT_TEST(brt->bsyb_flags, BSYBF_ACTIVE)) {
		    if (pref_changed > 0) {
			register rt_entry *his_rt;
			register bsy_ibgp_rt *his_brt;
			int i_am_better = 0;

			/*
			 * This guy may have become active.  Find the
			 * current active route and see if this guy
			 * is now better.
			 */
			RT_ALLRT(his_rt, rt->rt_head) {
			    assert(!BIT_TEST(his_rt->rt_state, RTS_DELETE));
			    if (his_rt->rt_gwp->gw_proto == RTPROTO_BGP
			      && his_rt->rt_gwp->gw_data == (void_t) bsp) {
				break;
			    }
			} RT_ALLRT_END(his_rt, rt->rt_head);

			assert(his_rt);
			his_brt = (bsy_ibgp_rt *)(his_rt->rt_data);
			assert(BIT_TEST(his_brt->bsyb_flags, BSYBF_ALT));
			if (!BIT_TEST(his_brt->bsyb_flags, BSYBF_ACTIVE)) {
			    BIT_SET(brt->bsyb_flags, BSYBF_ACTIVE);
			} else {
			    register bgpPeer *bnp, *his_bnp;
			    register metric_t metric, his_metric;

			    bnp = (bgpPeer *)(rt->rt_gwp->gw_task->task_data);
			    his_bnp = (bgpPeer *)
			      (his_rt->rt_gwp->gw_task->task_data);
			    if (BGP_VERSION_2OR3(his_bnp->bgp_version)) {
				his_metric =BGP_METRIC_3TO4(his_rt->rt_metric2);
			    } else {
				his_metric = his_rt->rt_metric2;
			    }
			    if (BGP_VERSION_2OR3(bnp->bgp_version)) {
				metric  =BGP_METRIC_3TO4(rt->rt_metric2);
			    } else {
				metric = rt->rt_metric2;
			    }

			    if (metric > his_metric) {
				i_am_better = 1;
			    } else if (metric == his_metric) {
				if (rt->rt_metric != BGP_METRIC_NONE
				  && (his_rt->rt_metric == BGP_METRIC_NONE
				  || his_rt->rt_metric > rt->rt_metric)) {
				    i_am_better = 1;
				} else if (his_rt->rt_metric == rt->rt_metric) {
				    if (brt->bsyb_nh->bsyn_igp_metric
				      < his_brt->bsyb_nh->bsyn_igp_metric) {
					i_am_better = 1;
				    } else if (brt->bsyb_nh->bsyn_igp_metric
					 == his_brt->bsyb_nh->bsyn_igp_metric) {
					if (ntohl(bnp->bgp_id)
					  < ntohl(his_bnp->bgp_id)) {
					    i_am_better = 1;
					}
				    }
				}
			    }

			    if (i_am_better) {
				BIT_SET(brt->bsyb_flags, BSYBF_ACTIVE);
				BIT_RESET(his_brt->bsyb_flags, BSYBF_ACTIVE);
				(void) rt_change_aspath(his_rt,
							his_rt->rt_metric,
							his_rt->rt_metric2,
							his_rt->rt_tag,
							BSY_NO_PREF,
							his_rt->rt_preference2,
							his_rt->rt_n_gw,
							his_rt->rt_routers,
							his_rt->rt_aspath);
				rt_refresh(his_rt);
			    }
			}
		    }
		} else if (pref_changed < 0) {
		    rt_entry *best_rt = (rt_entry *) 0;
		    rt_entry *his_rt;
		    bsy_ibgp_rt *best_brt = (bsy_ibgp_rt *) 0;
		    metric_t best_metric = 0;
		    bgpPeer *best_bnp = (bgpPeer *) 0;

		    /*
		     * Run through the whole list looking for the
		     * best guy we can find.
		     */
		    RT_ALLRT(his_rt, rt->rt_head) {
			register bgpPeer *his_bnp;
			register bsy_ibgp_rt *his_brt;
			register int his_is_better = 0;
			register metric_t his_metric;

			if (BIT_TEST(his_rt->rt_state, RTS_DELETE)) {
			    break;
			}
			if (his_rt->rt_gwp->gw_proto != RTPROTO_BGP
			  || his_rt->rt_gwp->gw_data != (void_t) bsp) {
			    continue;
			}

			his_brt = (bsy_ibgp_rt *)(his_rt->rt_data);
			assert(his_brt
			       && BIT_TEST(his_brt->bsyb_flags, BSYBF_ALT));
			if (!(his_brt->bsyb_nh->bsyn_igp_nexthop)) {
			    continue;
			}
			his_bnp = (bgpPeer *)(his_rt->rt_gwp->gw_task->task_data);
			if (BGP_VERSION_2OR3(his_bnp->bgp_version)) {
			    his_metric = BGP_METRIC_3TO4(his_rt->rt_metric2);
			} else {
			    his_metric = his_rt->rt_metric2;
			}

			if (!best_rt) {
			    his_is_better = 1;
			} else {
			    if (his_metric > best_metric) {
				his_is_better = 1;
			    } else if (his_metric == best_metric) {
				if (his_rt->rt_metric != BGP_METRIC_NONE
				  && (best_rt->rt_metric == BGP_METRIC_NONE
				  || best_rt->rt_metric > his_rt->rt_metric)) {
				    his_is_better = 1;
				} else if (best_rt->rt_metric
				  == his_rt->rt_metric) {
				    if (his_brt->bsyb_nh->bsyn_igp_metric
				      < best_brt->bsyb_nh->bsyn_igp_metric) {
					his_is_better = 1;
				    } else if (his_brt->bsyb_nh->bsyn_igp_metric
				      == best_brt->bsyb_nh->bsyn_igp_metric) {
					if (ntohl(his_bnp->bgp_id)
					  < ntohl(best_bnp->bgp_id)) {
					    his_is_better = 1;
					}
				    }
				}
			    }
			}

			if (his_is_better) {
			    best_rt = his_rt;
			    best_brt = his_brt;
			    best_metric = his_metric;
			    best_bnp = his_bnp;
			    his_is_better = 0;
			}
		    } RT_ALLRT_END(his_rt, rt->rt_head);

		    if (best_rt != rt) {
			BIT_RESET(brt->bsyb_flags, BSYBF_ACTIVE);
			if (best_rt) {
			    BIT_SET(best_brt->bsyb_flags, BSYBF_ACTIVE);
			    (void) rt_change_aspath(best_rt,
						    best_rt->rt_metric,
						    best_rt->rt_metric2,
						    best_rt->rt_tag,
						    best_brt->bsyb_pref,
						    best_rt->rt_preference2,
						    best_rt->rt_n_gw,
						    best_rt->rt_routers,
						    best_rt->rt_aspath);
			    rt_refresh(best_rt);
			}
		    }
		}
	    }

	    if (BIT_TEST(brt->bsyb_flags, BSYBF_ACTIVE)) {
		pref = brt->bsyb_pref;
	    } else {
		pref = BSY_NO_PREF;
	    }
	    if (nh_changed) {
		n = n_gw;
		new_nexthops = bsy_routers;
	    } else {
		if (pref == rt->rt_preference) {
		    continue;
		}
		n = rt->rt_n_gw;
		new_nexthops = rt->rt_routers;
	    }

	    (void) rt_change_aspath(rt,
				    rt->rt_metric,
				    rt->rt_metric2,
				    rt->rt_tag,
				    pref,
				    rt->rt_preference2,
				    n,
				    new_nexthops,
				    rt->rt_aspath);
	    rt_refresh(rt);
	}
    } while (nhe_next && (nhe_next = (nhe = nhe_next)->bsyn_change_next));

    /*
     * Done.  Zero out the the changes pointer and close the routing table.
     */
    bsp->bgp_sync_nh_changes = (bsy_nh_entry *) 0;
    rt_close(bsp->bgp_sync_task, (gw_entry *)0, 0, NULL);
}




/*
 * bgp_sync_igp_rt - process a change to an IGP route we are concerned
 *		     with.
 */
void
bgp_sync_igp_rt __PF2(bsp, bgp_sync *,
		      rth, rt_head *)
{
    rt_entry *new_rt, *old_rt;
    bsy_igp_rt *irt;
    u_int rtbit;
    u_int32 nmetric = 0;
    bsy_nexthop *nnh = (bsy_nexthop *) 0;
    bgp_ifap_list *bgpl = (bgp_ifap_list *) 0;
    bsy_rt_internal *inp = (bsy_rt_internal *) 0;
    bsy_rt_internal *stack[BSY_MAXDEPTH];
    bsy_rt_internal **sp;
    bsy_nh_entry *change_list;
    int changed = 1;

    /*
     * Determine if there is an existing route with our bit set.
     */
    rtbit = bsp->bgp_sync_task->task_rtbit;
    old_rt = rth->rth_last_active;
    irt = (bsy_igp_rt *) 0;
    if (old_rt) {
	if (rtbit_isset(old_rt, rtbit)) {
	    BSY_TSI_GET(rth, rtbit, irt);
	    assert(irt && irt->bsy_igp_rt == old_rt);
	} else {
	    old_rt = (rt_entry *) 0;
	}
    }

    new_rt = rth->rth_active;
    if (new_rt) {
	/*
	 * If this is an interface route see if the interface appears
	 * on the group's interface list.  If not it is unimportant
	 * to us.
	 */
	if (!BIT_TEST(new_rt->rt_state, RTS_GATEWAY)) {
	    bgpl = bsp->bgp_sync_group->bgpg_ifap_list;

	    while (bgpl) {
		if (bgpl->bgp_if_ifap == new_rt->rt_ifaps[0]) {
		    break;
		}
		bgpl = bgpl->bgp_if_next;
	    }
	    if (!bgpl) {
		new_rt = (rt_entry *) 0;
	    }
	} else if (new_rt->rt_gwp->gw_proto != bsp->bgp_sync_proto) {
	    new_rt = (rt_entry *) 0;
	}

	/*
	 * If we still have a new route, fetch its metrics and next hops
	 */
	if (new_rt) {
	    if (bgpl) {
		nmetric = bgpl->bgp_if_metric;
		nnh = bsy_nexthop_if;
		BSY_NH_ALLOC(bsy_nexthop_if);
	    } else {
		nmetric = new_rt->rt_metric;
		nnh = bgp_sync_nh_alloc(bsp,
					new_rt->rt_n_gw,
					new_rt->rt_routers);
	    }
	}
    }

    /*
     * If we don't have either a new route or an old route just return
     */
    if (!new_rt && !old_rt) {
	return;
    }

    if (!old_rt) {
	int depth;
	register bsy_rt_internal *in_check;

	/*
	 * An add, there is a new route to stick in the tree.
	 * Allocate it, fill it in and find out where it goes
	 */
	irt = BSY_IGP_RT_ALLOC();
	irt->bsy_igp_rt = new_rt;
	irt->bsy_igp_metric = nmetric;
	irt->bsy_igp_nexthop = nnh;
	rtbit_set(new_rt, rtbit);
	BSY_TSI_PUT(rth, rtbit, irt);

	depth = inet_prefix_mask(rth->rth_dest_mask);
	assert(depth >= 0);
	inp = bgp_sync_add_internal(bsp,
				    ntohl(sock2ip(rth->rth_dest)),
				    (u_int) depth);
	irt->bsy_igp_rti = inp;
	inp->bsyi_igp_rt = irt;

	in_check = inp;
	while ((in_check = in_check->bsyi_parent)) {
	    if (in_check->bsyi_igp_rt) {
		if (nnh == in_check->bsyi_igp_rt->bsy_igp_nexthop
		  && nmetric == in_check->bsyi_igp_rt->bsy_igp_metric) {
		    changed = 0;
		}
		break;
	    }
	}
    } else if (new_rt) {
	/*
	 * Here we have a change.  Move our rtbit to the new route, if
	 * necessary.  If this does not involve a change to the IGP metric
	 * or to the next hops we won't need to do anything more.
	 */
	if (old_rt != new_rt) {
	    irt->bsy_igp_rt = new_rt;
	    rtbit_set(new_rt, rtbit);
	    rtbit_reset(old_rt, rtbit);
	}

	if (irt->bsy_igp_metric == nmetric && irt->bsy_igp_nexthop == nnh) {
	    BSY_NH_FREE(bsp, nnh);
	    return;
	}
	BSY_NH_FREE(bsp, irt->bsy_igp_nexthop);
	irt->bsy_igp_nexthop = nnh;
	irt->bsy_igp_metric = nmetric;
	inp = irt->bsy_igp_rti;
    } else {
	register bsy_rt_internal *in_check;

	/*
	 * Here we have a delete.  If there is a route above us in the
	 * tree the IBGP routes below us will inherit that route's
	 * metrics/nexthop.  Determine if there is.  This'll let us
	 * know if the routes below actually need changing.
	 */
	nnh = (bsy_nexthop *) 0;
	nmetric = (u_int32) 0;
	inp = in_check = irt->bsy_igp_rti;
	while ((in_check = in_check->bsyi_parent)) {
	    if (in_check->bsyi_igp_rt) {
		if (nnh == in_check->bsyi_igp_rt->bsy_igp_nexthop
		  && nmetric == in_check->bsyi_igp_rt->bsy_igp_metric) {
		    changed = 0;
		}
		break;
	    }
	}
    }

    /*
     * If we got this far we have a changed IGP route, which irt
     * points at, with inp pointing at its internal node.  If this
     * changed something our guys will care about, we walk
     * the tree below this point, adding every nh_entry we come
     * across to the change list, if it isn't there already.  We
     * walk left when we can, recording the places we could have
     * gone right so we can visit there next.  If we put something
     * on the change list we make sure a task_job is queued to make
     * the changes.
     */
    if (changed) {
	change_list = bsp->bgp_sync_nh_changes;
	sp = stack;

	do {
	    register bsy_nh_entry *nhe;

	    if ((nhe = inp->bsyi_nexthop) && !(nhe->bsyn_change_next)) {
		if (change_list) {
		    nhe->bsyn_change_next = change_list;
		} else {
		    nhe->bsyn_change_next = nhe;
		}
		change_list = nhe;
	    }

	    if (inp->bsyi_left && !(inp->bsyi_left->bsyi_igp_rt)) {
		if (inp->bsyi_right && !(inp->bsyi_right->bsyi_igp_rt)) {
		    *sp++ = inp->bsyi_right;
		}
		inp = inp->bsyi_left;
	    } else if (!(inp = inp->bsyi_right) || inp->bsyi_igp_rt) {
		if (sp != stack) {
		    inp = *(--sp);
		} else {
		    inp = (bsy_rt_internal *) 0;
		}
	    }
	} while (inp);

	if (change_list) {
	    if (!(bsp->bgp_sync_nh_changes)) {
		(void) task_job_create(bsp->bgp_sync_task,
				       TASK_JOB_FG,
				       "IGP change processing",
				       bgp_sync_igp_change,
				       (void_t) bsp);
	    }
	    bsp->bgp_sync_nh_changes = change_list;
	}
    }

    /*
     * That is done.  If this was a delete clear the TSI and reset
     * our routing bit, then remove the external node.  We may be able
     * to delete one or two internal nodes as well.
     */
    if (!new_rt) {
	BSY_TSI_CLEAR(rth, rtbit);
	rtbit_reset(old_rt, rtbit);
	inp = irt->bsy_igp_rti;
	BSY_IGP_RT_FREE(bsp, irt);
	inp->bsyi_igp_rt = (bsy_igp_rt *) 0;

	if (!(inp->bsyi_nexthop) && (!(inp->bsyi_left) || !(inp->bsyi_right))) {
	    bgp_sync_remove_internal(bsp, inp);
	}
    }
}


/*
 * bgp_sync_terminate - terminate BGP synchronization for a group,
 *			presumably because the last peer in the group
 *			has gone away.  This deconstructs the radix
 *			tree, turns off our rtbit on all IGP routes
 *			and blows away any remaining memory allocation.
 */
void
bgp_sync_terminate __PF1(bsp, bgp_sync *)
{
    register bsy_rt_internal *inp, *inp_next;
    register bsy_igp_rt *irt;
    bsy_rt_internal *stack[(sizeof(bgp_nexthop) * NBBY) + 1];
    register bsy_rt_internal **sp = stack;
    register u_int rtbit = bsp->bgp_sync_task->task_rtbit;

    /*
     * Walk the tree, blowing away each internal node and
     * cleaning up any attached IGP routes.  If there are
     * nh_entries attached to the tree at this point they
     * should be empty.
     */
    rt_open(bsp->bgp_sync_task);
    inp = bsp->bgp_sync_trie;
    while (inp) {
	/*
	 * Determine next node to visit.
	 */
	if (inp->bsyi_left) {
	    if (inp->bsyi_right) {
		*sp++ = inp->bsyi_right;
	    }
	    inp_next = inp->bsyi_left;
	} else if (inp->bsyi_right) {
	    inp_next = inp->bsyi_right;
	} else if (sp != stack) {
	    inp_next = *(--sp);
	} else {
	    inp_next = (bsy_rt_internal *) 0;
	}

	/*
	 * If there is a next hop entry, blow it off.  It must
	 * be empty by now.
	 */
	if (inp->bsyi_nexthop) {
	    assert(BSY_NHE_ISEMPTY(inp->bsyi_nexthop));
	    BSY_NH_ENTRY_FREE(bsp, inp->bsyi_nexthop);
	}

	/*
	 * If there is an IGP route remove our rtbit and clean
	 * up the TSI field.
	 */
	if ((irt = inp->bsyi_igp_rt)) {
	    BSY_TSI_CLEAR(irt->bsy_igp_rt->rt_head, rtbit);
	    rtbit_reset(irt->bsy_igp_rt, rtbit);
	    BSY_IGP_RT_FREE(bsp, irt);
	}

	/*
	 * Removed all external info by now, blow off the
	 * internal node.
	 */
	BSY_RT_INTERNAL_FREE(inp);
	inp = inp_next;
    }

    rt_close(bsp->bgp_sync_task, (gw_entry *)0, 0, NULL);

    /*
     * We've reduced our tree to nothing.  The nexthop hash structure
     * should be empty by now too.  Dump the rtbit and the task next,
     * then free the bgp_sync structure itself.
     */
    assert(bsp->bgp_sync_n_hashed == 0);
    rtbit_free(bsp->bgp_sync_task, rtbit);
    task_delete(bsp->bgp_sync_task);
    bsp->bgp_sync_group->bgpg_sync = (bgp_sync *) 0;
    BSY_SYNC_FREE(bsp);
}


/*
 * bgp_sync_dump - dump the synchronization tree
 */
static void
bgp_sync_dump __PF2(tp, task *,
		    fd, FILE *)
{
    bgp_sync *bsp = (bgp_sync *)(tp->task_data);
    sockaddr_un dest;
    bsy_rt_internal *inp;
    bsy_rt_internal *stack[BSY_MAXDEPTH];
    bsy_rt_internal **sp;

    sockclear_in(&dest);
    (void) fprintf(fd, "\tIGP Protocol: %s\tBGP Group: %s\n\n",
		   trace_state(rt_proto_bits, bsp->bgp_sync_proto),
		   bsp->bgp_sync_group->bgpg_name);

    /*
     * Okay, walk the tree looking for IGP routes.  For each IGP route
     * found, walk the tree below it to find guys who are using this route.
     */
    inp = bsp->bgp_sync_trie;
    if (inp) {
	sp = stack;
	(void) fprintf(fd, "\tSync Tree (* == active, + == active with alternate, - == inactive with alternate:\n");
	while (inp) {
	    bsy_igp_rt *irt;
	    int walkit = 0;

	    if ((irt = inp->bsyi_igp_rt)) {
		rt_entry *rt = irt->bsy_igp_rt;

		sock2ip(&dest) = htonl(inp->bsyi_dest);
		(void) fprintf(fd, "\tNode %A/%u route %A/%A metric %d",
			       &dest,
			       inp->bsyi_bit,
			       rt->rt_dest,
			       rt->rt_dest_mask,
			       irt->bsy_igp_metric);

		if (irt->bsy_igp_nexthop == bsy_nexthop_if) {
		    (void) fprintf(fd, " interface\n");
		} else {
		    if (RT_N_MULTIPATH == 1
		      || irt->bsy_igp_nexthop->bsynh_n_gw == 1) {
			sock2ip(&dest) = irt->bsy_igp_nexthop->bsynh_nexthop[0];
			(void) fprintf(fd, " next hop %A\n", &dest);
		    } else {
			register u_int i;
			bsy_nexthop *nhp = irt->bsy_igp_nexthop;

			(void) fprintf(fd, " next hops");
			for (i = 0; i < nhp->bsynh_n_gw; i++) {
			    sock2ip(&dest) = nhp->bsynh_nexthop[i];
			    (void) fprintf(fd, " %A", &dest);
			}
			(void) fprintf(fd, "\n");
		    }
		}
		walkit = 1;
	    } else if (inp == bsp->bgp_sync_trie) {
		walkit = 1;
	    }

	    if (walkit) {
		register bsy_rt_internal *dinp;
		bsy_rt_internal **dstack = sp;
		bsy_rt_internal **dsp;
		int did_one = 0;

		dsp = dstack;
		dinp = inp;
		do {
		    bsy_nh_entry *nhe;
		    register bsy_ibgp_rt *brt;

		    if ((nhe = dinp->bsyi_nexthop)) {
			if (!did_one && !irt) {
			    (void) fprintf(fd, "\tOrphaned routes\n");
			}
			did_one++;
			sock2ip(&dest) = htonl(dinp->bsyi_dest);
			(void) fprintf(fd, "\t\tForwarding address %A",
				       &dest);
			if (nhe->bsyn_change_next) {
			    (void) fprintf(fd, " changed");
			}
			if (irt && !(nhe->bsyn_igp_nexthop)) {
			    (void) fprintf(fd, " orphaned");
			}
			if (nhe->bsyn_igp_nexthop && (!irt
			  || nhe->bsyn_igp_metric != irt->bsy_igp_metric)) {
			    (void) fprintf(fd, " metric %d",
					   nhe->bsyn_igp_metric);
			}
			if (nhe->bsyn_igp_nexthop && (!irt
			  || nhe->bsyn_igp_nexthop != irt->bsy_igp_nexthop)) {
			    if (nhe->bsyn_igp_nexthop == bsy_nexthop_if) {
				(void) fprintf(fd, " next hop direct\n");
			    } else if (RT_N_MULTIPATH == 1
			      || nhe->bsyn_igp_nexthop->bsynh_n_gw == 1) {
				sock2ip(&dest) =
				  nhe->bsyn_igp_nexthop->bsynh_nexthop[0];
				(void) fprintf(fd, " next hop %A\n", &dest);
			    } else {
				register u_int i;
				bsy_nexthop *nhp = irt->bsy_igp_nexthop;

				(void) fprintf(fd, " next hops");
				for (i = 0; i < nhp->bsynh_n_gw; i++) {
				    sock2ip(&dest) = nhp->bsynh_nexthop[i];
				    (void) fprintf(fd, " %A", &dest);
				}
				(void) fprintf(fd, "\n");
			    }
			} else {
			    (void) fprintf(fd, "\n");
			}

			for (brt = nhe->bsyn_next;
			     brt != ((bsy_ibgp_rt *)nhe);
			     brt = brt->bsyb_next) {
			    const char *state;

			    if (BIT_TEST(brt->bsyb_flags, BSYBF_ALT)) {
				if (BIT_TEST(brt->bsyb_flags, BSYBF_ACTIVE)) {
				    state = "+";
				} else {
				    state = "-";
				}
			    } else if (BIT_TEST(brt->bsyb_flags, BSYBF_ACTIVE)) {
				state = "*";
			    } else {
				state = "";
			    }

			    (void) fprintf(fd,
					   "\t\t\t%A/%""A%s peer %A preference %d\n",
					   brt->bsyb_rt->rt_dest,
					   brt->bsyb_rt->rt_dest_mask,
					   state,
					   brt->bsyb_rt->rt_gwp->gw_addr,
					   brt->bsyb_pref);
			}
		    }

		    if (dinp->bsyi_left && !(dinp->bsyi_left->bsyi_igp_rt)) {
			if (dinp->bsyi_right
			  && !(dinp->bsyi_right->bsyi_igp_rt)) {
			    *dsp++ = dinp->bsyi_right;
			}
			dinp = dinp->bsyi_left;
		    } else if (dinp->bsyi_right
		      && !(dinp->bsyi_right->bsyi_igp_rt)) {
			dinp = dinp->bsyi_right;
		    } else if (dsp != dstack) {
			dinp = *(--dsp);
		    } else {
			dinp = (bsy_rt_internal *) 0;
		    }
		} while (dinp);
	    }

	    if (inp->bsyi_left) {
		if (inp->bsyi_right) {
		    *sp++ = inp->bsyi_right;
		}
		inp = inp->bsyi_left;
	    } else if (inp->bsyi_right) {
		inp = inp->bsyi_right;
	    } else if (sp != stack) {
		inp = *(--sp);
	    } else {
		inp = (bsy_rt_internal *) 0;
	    }
	}
    }

    if (bsp->bgp_sync_n_hashed || bsy_nexthop_if->bsynh_refcount != 1) {
	register int i;
	register bsy_nexthop *nhp;

	(void) fprintf(fd, "\tNext hop hash table (%d hashed):\n",
		       bsp->bgp_sync_n_hashed);

	if (bsy_nexthop_if->bsynh_refcount != 1) {
	    (void) fprintf(fd, "\t\tReferences %u Hash none Interface\n",
			   bsy_nexthop_if->bsynh_refcount - 1);
	}

	for (i = 0; i < BSYNH_HASH_SIZE; i++) {
	    for (nhp = bsp->bgp_sync_nh_hash[i];
		 nhp;
		 nhp = nhp->bsynh_next) {
		(void) fprintf(fd, "\t\tReferences %u Hash %u Next hop",
			       nhp->bsynh_refcount,
			       nhp->bsynh_hash);
		if (RT_N_MULTIPATH == 1 || nhp->bsynh_n_gw == 1) {
		    sock2ip(&dest) = nhp->bsynh_nexthop[0];
		    (void) fprintf(fd, " %A\n", &dest);
		} else {
		    register u_int j;

		    (void) fprintf(fd, "s");
		    for (j = 0; j < nhp->bsynh_n_gw; j++) {
			sock2ip(&dest) = nhp->bsynh_nexthop[j];
			(void) fprintf(fd, " %A", &dest);
		    }
		    (void) fprintf(fd, "\n");
		}
	    }
	}
    }
}



/*
 * bgp_sync_tsi_dump - dump the TSI field for a route we may have our
 *		       bit set on.
 */
static void
bgp_sync_tsi_dump __PF4(fp, FILE *,
			rth, rt_head *,
			data, void_t,
			pfx, const char *)
{
    bgp_sync *bsp = (bgp_sync *)data;
    bsy_igp_rt *irt;

    BSY_TSI_GET(rth, bsp->bgp_sync_task->task_rtbit, irt);
    if (irt) {
	register bsy_rt_internal *inp = irt->bsy_igp_rti;
	sockaddr_un dest;

	sockclear_in(&dest);
	sock2ip(&dest) = htonl(inp->bsyi_dest);
	(void) fprintf(fp, "%s%s dest %A/%u metric %d\n",
		       pfx,
		       bsp->bgp_sync_name,
		       &dest,
		       inp->bsyi_bit,
		       irt->bsy_igp_metric);
    }
}

/*
 * bgp_sync_fake_terminate - stub to keep the task code happy, we
 *			     actually wait for our group to terminate us.
 */
static void
bgp_sync_fake_terminate  __PF1(tp, task *)
{
    /* nothing */
}



/*
 * bgp_sync_init - initialize BGP synchronization for a group (probably
 *		   because the first group peer has just come up).
 *		   Initializes the module if this hasn't been done
 *		   already, then allocates and fills in a bgp_sync
 *		   structure for the group.
 */
bgp_sync *
bgp_sync_init __PF1(bgp, bgpPeerGroup *)
{
    register bgp_sync *bsp;
    register task *tp;

    if (bgp_sync_block == (block_t) 0) {
	/*
	 * First time through, initialize all module structures
	 */
	BSY_BLOCK_INIT();
	BSY_ROUTERS_INIT();
	BSY_NEXTHOP_IF_INIT();
    }

    /*
     * Make sure this guy hasn't been here before, for sanity.
     * Then fetch him a sync structure and fill in the easy stuff.
     */
    assert(!(bgp->bgpg_sync));
    bsp = BSY_SYNC_ALLOC();
    (void) sprintf(bsp->bgp_sync_name, "BGP_Sync_%u", bgp->bgpg_peer_as);
    bsp->bgp_sync_proto = bgp->bgpg_proto;
    bsp->bgp_sync_group = bgp;

    /*
     * Allocate a task to track the rtbit and to hang our dump routine
     * off of.
     */
    tp = task_alloc(bsp->bgp_sync_name,
		    (TASKPRI_EXTPROTO+1),
		    bgp->bgpg_trace_options);
    tp->task_rtproto = RTPROTO_BGP;
    tp->task_data = (void_t) bsp;
    task_set_terminate(tp, bgp_sync_fake_terminate);
    task_set_dump(tp, bgp_sync_dump);
    BIT_SET(tp->task_flags, TASKF_LOWPRIO);
    if (!task_create(tp)) {
	task_quit(EINVAL);
    }
    bsp->bgp_sync_task = tp;
    tp->task_rtbit = BSY_RTBIT_ALLOC(bsp);

    /*
     * We're done, return the sync structure.
     */
    return bsp;
}









