#ident	"@(#)rt_radix.c	1.3"
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


#define	INCLUDE_RT_VAR
#include "include.h"

/*
 * How this works (before I forget):
 *
 * At the time of writing gated uses radix trees in three places: here,
 * the policy code and in bgp_sync.c.  The basic algorithm used is
 * identical in each place, though the things that are done with the
 * tree are different enough to have made it worth using custom code
 * in each case.  This comment exists because radix trees are really
 * pretty simple, but have a reputation for complexity derived from a
 * particular kernel implementation which does things a bit differently
 * than this, and I thought it might help to explain how this implementation
 * functions in detail.
 *
 * These trees work a follows.  The tree is made up of "internal" and
 * "external" nodes, where the "external" nodes are the address/mask
 * associated information you want to find/sort in the tree and the
 * "internal" nodes are the radix tree bookkeeping structure.  Each node
 * in the tree is identified by a bit number.  Bit number 0 refers to the
 * most significant (leftmost) bit in the address/mask.  Each internal
 * node has two child pointers, a left and a right pointer, and optionally
 * a parent pointer which points towards the root of the tree (you don't
 * need the parent pointer, and it wasn't used in the policy version of the
 * radix tree code, but it makes deletions and certain other operations
 * slightly easier).  The bit number of a child is always numerically
 * larger than that of its parent.
 *
 * When you install a route in the tree you attach it to an internal node
 * which has a bit number equal to the position of the first zero bit in
 * the route's mask (e.g. a default route is attached to an internal node
 * with bit number 0, while an IP host route is attached to an internal
 * node with bit number 32).  If you only deal with routes with contiguous
 * masks you will end up with no more than one route attached to any
 * internal node, otherwise you need to chain together all routes whose
 * addresses agree up the the first zero bit in the masks.  Some internal
 * nodes (<= half) will have no attached external information, I call
 * these "splits".  These are guaranteed to have both a left and right
 * child.  Nodes with external information may have zero, one or two
 * children.
 *
 * The tree guarantees that, given a route attached to an internal node
 * in the tree, all less specific routes which match the same destination
 * will be located on the path from that internal node to the root of the
 * tree.  Conversely, all routes to matching but more specific destinations
 * will be located below that internal node in the tree.
 *
 * There are two generic types of lookups in the tree, one for an
 * extact match (i.e. you are given an address/mask and asked to find this
 * exact route in the tree) and one for a best match, where you are given
 * a target address/mask and asked to find an address/mask in the tree
 * such that (t_address & mask) == address and (mask & ~t_mask) == 0
 * (lookups for host addresses are obvious a special case of the latter,
 * with an all-ones t_mask).  To find an exact match first determine the bit
 * number of the first zero bit in the target mask.  Then search down
 * the tree, testing the bit in the target address corresponding to
 * the bit number of the current internal node and branch left if the bit
 * is zero, right if it is one, until you find a node whose number is equal
 * to the target mask bit number.  If you don't find a node whose bit number
 * is equal, or if the node has no external info attached the search fails.
 * If you find a node with the correct bit number and with external
 * info, this is the only possible match in the tree.  Compare the
 * target address to the external address and, if noncontigous masks are
 * allowed, the target mask to the external mask on this node to see if
 * they really do match.
 *
 * To do the best match search, allocate a stack array to the maximum
 * depth of the tree (i.e. the bit number of a maximum-length host
 * address, plus one).  Determine the first zero bit in the target
 * mask (or the bit length of a host address).  Then do the same
 * bit-wise search of the tree, stacking a pointer each internal node
 * with attached external info you find on the way down, until you find
 * an internal node with a bit number greater than or equal to that
 * determined from the target mask.  Stop the search here, all possible
 * matches will now be on the stack.  Take the last guy on the stack
 * and compare his address to your address to determine the bit number
 * where they first differ (i.e. do an exclusive or of the addresses
 * and find the bit position of the first 1 bit).  Then search back
 * up the stack to find an external node attached to an internal node
 * with a bit number less than or equal to the bit number you just
 * determined.  If you only have contiguous masks in the tree this
 * will be your best match, otherwise you'll need to do some additional
 * address/mask matches in the parts of the address beyond this bit
 * number to determine if a noncontiguous mask/address matches.
 *
 * To walk the tree in SNMP lexical order, go left when you can and
 * right when you can't.  I like to do it with a stack since it is
 * easier (you can also do it with the parent pointers but it is
 * more work).  Start with the root node and:
 *
 * 	process current node;
 *	if (there is a left child) {
 *		if (there is a right child) {	
 *			push right child on stack;
 *		}
 *		current node is left child;
 *	} else if (there is a right child) {
 *		current node is right child;
 *	} else if (something on stack) {
 *		pop stack and make this current node;
 *	} else {
 *		all done;
 *	}
 *
 *
 * When you do insertions in the tree you'll be adding zero, one or
 * two internal nodes.  Find the first zero bit in the mask, this'll be
 * the bit number of the internal node you'll need to attach to.  If
 * you have no nodes in the tree the insertion is easy, just allocate a
 * new internal node, put the bit number in there, and make it the
 * root node.  Otherwise search down the tree until you find an
 * internal node with a bit number greater than or equal to your bit
 * number (with an external node attached if you don't store addresses/keys
 * in the internal node itself), or until you can't go any further.
 * Compare your address to his address, to the position
 * MIN(his_bit_number, your_bit_number), to find the bit number of the
 * first bit where you differ.  There are a number of possibilities
 * for the result:
 *
 * The difference bit is >= his bit number and your bit number is > his.
 * You go below him in the tree, make your new internal node his child
 * on the correct side.
 *
 * The difference bit is < his bit number, but >= yours.  You go above
 * him in the tree.  You may be able to attach to an existing internal
 * node if you can find one with your bit number, otherwise insert one
 * at the correct position (the one you insert will only have one child)
 * with your bit number and attach yourself to it.
 *
 * The difference bit is < his bit number and < yours.  You'll need to
 * insert a "split" node whose bit number is equal to the difference
 * bit number.  Your internal node (the second one you will add in
 * this case) will go on one side of the split, the remainder of the
 * branch you searched down on the other.
 *
 * In the two cases above you may fall off the top of the tree, in
 * which case one of the internal nodes you're inserting (the split
 * node if you put one in, the node you attached the external stuff
 * to otherwise) will become the new root of the tree.
 *
 * When removing nodes from the tree you'll be removing zero, one or
 * two internal nodes from the tree.  Remove your external stuff from
 * the node it is attached to.  If this node has two children you are
 * done, it stays in the tree.  If the node has one child, remove the
 * internal node from the tree and put a pointer to the child in
 * the removed node's position in the parent.  If your node has no
 * children, remove the node and just zero the pointer to removed node in
 * its parent.  If the parent has no external node attached it may
 * also be removed from the tree by promoting its child into its parent.
 *
 * You'll find insertion, deletion and match routines below.  You'll
 * also find a routine to do SNMP get_next's.  Since I've taken the
 * trouble to explain the data structure in copious detail I will expect
 * bug reports concerning SNMP get_next's in the form of patches which
 * correct the code.  Thanks.
 */

/*
 * Radix search tree node layout.
 */

typedef struct _radix_node {
    struct _radix_node	*rnode_left;		/* child when bit clear */
    struct _radix_node	*rnode_right;		/* child when bit set */
    struct _radix_node	*rnode_parent;		/* parent */
    u_short	rnode_bit;			/* bit number for node/mask */
    u_char	rnode_tbyte;			/* byte to test in address */
    u_char	rnode_tbit;			/* bit to test in byte */
    rt_head	*rnode_rth;			/* our external info */
} radix_node;

/*
 * Routing table structure.  This points to the current root of the
 * radix trie and contains some information which we need to know
 * to do installations.
 */
typedef struct _rt_table {
    struct _rt_table	*rt_table_next;	/* next table in list */
    radix_node	*rt_table_tree;		/* current root of tree */
    struct sock_info *rt_table_sinfo;	/* stuff we need to grok sockaddr's */
    u_long	rt_table_inodes;	/* number of internal nodes in tree */
    u_long	rt_table_routes;	/* number of routes in tree */
} rt_table;

/*
 * XXX Assumes NBBY is 8.  If it isn't we're in trouble anyway.
 */
#define	RNBBY	8
#define	RNSHIFT	3
#define	RNBIT(x)	(0x80 >> ((x) & (RNBBY-1)))
#define	RNBYTE(x)	((x) >> RNSHIFT)
#define	MAXKEYBITS	(SOCK_MAXADDRLEN * RNBBY)
#define	MAXDEPTH	(MAXKEYBITS+1)

#define	RN_SETBIT(rn, bitlen, lastmask) \
    do { \
	register radix_node *Xrn = (rn); \
	register u_short Xlen = (u_short)(bitlen); \
	Xrn->rnode_bit = Xlen; \
	if ((lastmask)) { \
	    Xrn->rnode_tbyte = 0; \
	    Xrn->rnode_tbit = 0; \
	} else { \
	    Xrn->rnode_tbyte = RNBYTE(Xlen); \
	    Xrn->rnode_tbit = RNBIT(Xlen); \
	} \
    } while (0)

#define	RN_BYTELEN(x)	((unsigned) ((x) + RNBBY - 1) >> RNSHIFT)

#define	RN_ADDR(addr, a_offset)		(((byte *)(addr)) + a_offset)

#define	RT_TABLE_PTR(af)	rt_tables[af]

#define	RT_TREETOP(af)	(rt_tables[(af)] ? rt_tables[af]->rt_table_tree : (radix_node *) 0)

#define	RT_TABLES(af) \
    do { \
	register int af; \
	register rt_table *Xrt_table; \
	for (Xrt_table = rt_table_list; \
	     Xrt_table; \
	     Xrt_table = Xrt_table->rt_table_next) { \
	    af = Xrt_table->rt_table_sinfo->si_family;

#define	RT_TABLES_END(af)    } } while (0)

#define	RT_ALLTABLES(rtt) \
    do { \
	rt_table *Xrt_table = rt_table_list; \
	while (Xrt_table) { \
	    (rtt) = Xrt_table; \
	    Xrt_table = Xrt_table->rt_table_next; \
	    do

#define	RT_ALLTABLES_END(rtt) \
	    while (0); \
	} \
    } while (0)


/*
 * Routines to build and maintain radix trees for routing lookups.
 */

static block_t rt_node_block_index;
static block_t rt_table_block_index;

static rt_table *rt_tables[AF_MAX] = {0};
static rt_table *rt_table_list = (rt_table *) 0;

#define	RT_WALK(head, rth) \
    do { \
	radix_node *Xstack[MAXDEPTH]; \
	radix_node **Xsp = Xstack; \
	radix_node *Xrn = (head); \
	while (Xrn) { \
	    if (((rth) = Xrn->rnode_rth)) do

#define	RT_WALK_END(head, rth) \
	    while (0); \
	    if (Xrn->rnode_left) { \
		if (Xrn->rnode_right) { \
		    *Xsp++ = Xrn->rnode_right; \
		} \
		Xrn = Xrn->rnode_left; \
	    } else if (Xrn->rnode_right) { \
		Xrn = Xrn->rnode_right; \
	    } else if (Xsp != Xstack) { \
		Xrn = *(--Xsp); \
	    } else { \
		Xrn = (radix_node *) 0; \
		(rth) = (rt_head *) 0; \
	    } \
	} \
    } while (0)

/*
 *	Macro to scan through entire active routing table
 */
#define RT_WHOLE(head, rt)	{ rt_head *Xrth; RT_WALK(head, Xrth) RT_ALLRT(rt, Xrth)

#define	RT_WHOLE_END(head, rt)	RT_ALLRT_END(rt, Xrth) RT_WALK_END(head, Xrth); }


#define RT_TABLE(head, rt)	{ rt_head *Xrth; RT_WALK(head, Xrth) RT_IFRT(rt, Xrth)

#define	RT_TABLE_END(head, rt)	RT_IFRT_END RT_WALK_END(head, Xrth); }


/*
 *	Locate the rt_head for the given destination
 */
rt_head *
rt_table_locate __PF2(dst, sockaddr_un *,
		      mask, sockaddr_un *)
{
    register radix_node *rn;
    register byte *ap, *ap2;
    register u_short bitlen;
    register struct sock_info *si;
    register rt_table *rtt;
    rt_head *rth;

    /*
     * If there is no table, or nothing to do, assume nothing found.
     */
    if (!(rtt = RT_TABLE_PTR(socktype(dst))) || !(rn = rtt->rt_table_tree)) {
	return (rt_head *) 0;
    }

    /*
     * Compute the bit length of the mask.
     */
    si = rtt->rt_table_sinfo;
    bitlen = mask_to_prefix_si(si, mask);
    assert(bitlen != (u_short) -1);

    /*
     * Search down the tree until we find a node which
     * has a bit number the same as ours.
     */

    ap = RN_ADDR(dst, si->si_offset);
    while (rn->rnode_bit < bitlen) {
	if (rn->rnode_tbit & ap[rn->rnode_tbyte]) {
	    rn = rn->rnode_right;
	} else {
	    rn = rn->rnode_left;
	}
	if (!rn) {
	    break;
	}
    }

    /*
     * If we didn't find an exact bit length match, we're gone.
     * If there is no rth on this node, we're gone too.
     */
    if (!rn || rn->rnode_bit != bitlen || !(rth = rn->rnode_rth)) {
	return (rt_head *) 0;
    }

    /*
     * So far so good.  Fetch the address and see if we have an
     * exact match.
     */
    ap2 = RN_ADDR(rth->rth_dest, si->si_offset);
    bitlen = RN_BYTELEN(bitlen);
    while (bitlen--) {
	if (*ap++ != *ap2++) {
	    return (rt_head *) 0;
	}
    }
    return rth;
}

/*
 *	Return a list of potential routes to this destination, most specific first
 */
rt_list *
rthlist_match __PF1(dst, sockaddr_un *)
{
    rt_list *rtl = (rt_list *) 0;
    register radix_node *rn;
    register byte *ap, *ap2;
    register radix_node **sp;
    radix_node *stack[MAXDEPTH];
    register u_short bitlen, dbit;
    struct sock_info *si;
    rt_table *rtt;

    /*
     * If there is no table, or nothing to do, assume nothing found.
     */
    if (!(rtt = RT_TABLE_PTR(socktype(dst))) || !(rn = rtt->rt_table_tree)) {
	return (rt_list *) 0;
    }

    /*
     * Fetch the address we're looking for.
     */
    si = rtt->rt_table_sinfo;
    ap = RN_ADDR(dst, si->si_offset);

    /*
     * Search the tree, recording all rth's we come across on the
     * way down
     */
    sp = stack;
    while (rn) {
	if (rn->rnode_rth) {
	    *sp++ = rn;
	}
	if (rn->rnode_tbit == 0) {
	    break;
	}
	if (ap[rn->rnode_tbyte] & rn->rnode_tbit) {
	    rn = rn->rnode_right;
	} else {
	    rn = rn->rnode_left;
	}
    }

    /*
     * Take the last guy on the stack (there is guaranteed to
     * be at least one) and compute the first bit different.  We
     * won't match a node with a bit number larger than this.
     */
    rn = *(--sp);
    bitlen = rn->rnode_bit;
    ap2 = RN_ADDR(rn->rnode_rth->rth_dest, si->si_offset);
    for (dbit = 0; dbit < bitlen; dbit += RNBBY) {
	if (*ap != *ap2) {
	    dbit += first_bit_set[*ap ^ *ap2];
	    break;
	}
	ap++; ap2++;
    }

    /*
     * Throw anyone off the stack with a bit number larger than this.
     */
    for (; rn->rnode_bit > dbit; rn = *(--sp)) {
	if (sp == stack) {
	    return (rt_list *) 0;
	}
    }

    /*
     * Anyone remaining is a match.  Add them to the rt_list.
     */
    RTLIST_ADD(rtl, rn->rnode_rth);
    while (sp != stack) {
	rn = *(--sp);
	RTLIST_ADD(rtl, rn->rnode_rth);
    }
    return rtl;
}


/**/

/*
 *	Insert this head into the tree
 */
void
rt_table_add __PF1(rth, rt_head *)
{
    register radix_node *rn, *rn_prev, *rn_add, *rn_new;
    register u_short bitlen, bits2chk, dbit;
    register byte *addr, *his_addr;
    register u_int i;
    struct sock_info *si;
    rt_table *rtt;

    rtt = RT_TABLE_PTR(socktype(rth->rth_dest));
    assert(rtt);

    /*
     * Compute the bit length of the mask.
     */
    si = rtt->rt_table_sinfo;
    bitlen = mask_to_prefix_si(si, rth->rth_dest_mask);
    assert(bitlen != (u_short) -1);

    rn_prev = rtt->rt_table_tree;
    rtt->rt_table_routes++;

    /*
     * If there is no existing root node, this is it.  Catch this
     * case now.
     */
    if (!rn_prev) {
	rn = (radix_node *) task_block_alloc(rt_node_block_index);
	rtt->rt_table_tree = rth->rth_radix_node = rn;
	RN_SETBIT(rn, bitlen, (rth->rth_dest_mask == si->si_mask_max));
	rn->rnode_rth = rth;
	rtt->rt_table_inodes++;
	return;
    }

    /*
     * Search down the tree as far as we can, stopping at a node
     * with a bit number >= ours which has an rth attached.  It
     * is possible we won't get down the tree this far, however,
     * so deal with that as well.
     */
    addr = RN_ADDR(rth->rth_dest, si->si_offset);
    rn = rn_prev;
    while (rn->rnode_bit < bitlen || !(rn->rnode_rth)) {
	if (BIT_TEST(addr[rn->rnode_tbyte], rn->rnode_tbit)) {
	    if (!(rn->rnode_right)) {
		break;
	    }
	    rn = rn->rnode_right;
	} else {
	    if (!(rn->rnode_left)) {
		break;
	    }
	    rn = rn->rnode_left;
	}
    }

    /*
     * Now we need to find the number of the first bit in our address
     * which differs from his address.
     */
    bits2chk = MIN(rn->rnode_bit, bitlen);
    his_addr = RN_ADDR(rn->rnode_rth->rth_dest, si->si_offset);
    for (dbit = 0; dbit < bits2chk; dbit += RNBBY) {
	i = dbit >> RNSHIFT;
	if (addr[i] != his_addr[i]) {
	    dbit += first_bit_set[addr[i] ^ his_addr[i]];
	    break;
	}
    }

    /*
     * If the different bit is less than bits2chk we will need to
     * insert a split above him.  Otherwise we will either be in
     * the tree above him, or attached below him.
     */
    if (dbit > bits2chk) {
	dbit = bits2chk;
    }
    rn_prev = rn->rnode_parent;
    while (rn_prev && rn_prev->rnode_bit >= dbit) {
	rn = rn_prev;
	rn_prev = rn->rnode_parent;
    }

    /*
     * Okay.  If the node rn points at is equal to our bit number, we
     * may just be able to attach the rth to him.  Check this since it
     * is easy.
     */
    if (dbit == bitlen && rn->rnode_bit == bitlen) {
	assert(!(rn->rnode_rth));
	rn->rnode_rth = rth;
	rth->rth_radix_node = rn;
	return;
    }

    /*
     * Allocate us a new node, we are sure to need it now.
     */
    rn_add = (radix_node *) task_block_alloc(rt_node_block_index);
    rtt->rt_table_inodes++;
    RN_SETBIT(rn_add, bitlen, (rth->rth_dest_mask == si->si_mask_max));
    rn_add->rnode_rth = rth;
    rth->rth_radix_node = rn_add;

    /*
     * There are a couple of possibilities.  The first is that we
     * attach directly to the thing pointed to by rn.  This will be
     * the case if his bit is equal to dbit.
     */
    if (rn->rnode_bit == dbit) {
	assert(dbit < bitlen);
	rn_add->rnode_parent = rn;
	if (BIT_TEST(addr[rn->rnode_tbyte], rn->rnode_tbit)) {
	    assert(!(rn->rnode_right));
	    rn->rnode_right = rn_add;
	} else {
	    assert(!(rn->rnode_left));
	    rn->rnode_left = rn_add;
	}
	return;
    }

    /*
     * The other case where we don't need to add a split is where
     * we were on the same branch as the guy we found.  In this case
     * we insert rn_add into the tree between rn_prev and rn.  Otherwise
     * we add a split between rn_prev and rn and append the node we're
     * adding to one side of it.
     */
    if (dbit == bitlen) {
	if (BIT_TEST(his_addr[rn_add->rnode_tbyte], rn_add->rnode_tbit)) {
	    rn_add->rnode_right = rn;
	} else {
	    rn_add->rnode_left = rn;
	}
	rn_new = rn_add;
    } else {
	rn_new = (radix_node *) task_block_alloc(rt_node_block_index);
	rtt->rt_table_inodes++;
	RN_SETBIT(rn_new, dbit, FALSE);
	rn_add->rnode_parent = rn_new;
	if (BIT_TEST(addr[rn_new->rnode_tbyte], rn_new->rnode_tbit)) {
	    rn_new->rnode_right = rn_add;
	    rn_new->rnode_left = rn;
	} else {
	    rn_new->rnode_left = rn_add;
	    rn_new->rnode_right = rn;
	}
    }
    rn->rnode_parent = rn_new;
    rn_new->rnode_parent = rn_prev;

    /*
     * If rn_prev is NULL this is a new root node, otherwise it
     * is attached to the guy above in the place where rn was.
     */
    if (!rn_prev) {
	rtt->rt_table_tree = rn_new;
    } else if (rn_prev->rnode_right == rn) {
	rn_prev->rnode_right = rn_new;
    } else {
	assert(rn_prev->rnode_left == rn);
	rn_prev->rnode_left = rn_new;
    }
}


/*
 *	Remove this head from the tree
 */
void
rt_table_delete __PF1(rth, rt_head *)
{
    radix_node *rn, *rn_next, *rn_prev;
    rt_table *rtt;

    assert((rtt = RT_TABLE_PTR(socktype(rth->rth_dest))));
    rtt->rt_table_routes--;
    rn = rth->rth_radix_node;
    rth->rth_radix_node = (radix_node *) 0;

    /*
     * Catch the easy case.  If this guy has nodes on both his left
     * and right, he stays in the tree.
     */
    if (rn->rnode_left && rn->rnode_right) {
	rn->rnode_rth = (rt_head *) 0;
	return;
    }

    /*
     * If this guy has no successor he's a goner.  The guy above
     * him will be too, unless he's got external stuff attached to
     * him.
     */
    if (!(rn->rnode_left) && !(rn->rnode_right)) {
	rn_prev = rn->rnode_parent;
	task_block_free(rt_node_block_index, (void_t) rn);
	rtt->rt_table_inodes--;

	if (!rn_prev) {
	    /*
	     * Last guy in the tree, remove the root node pointer
	     */
	    RT_TABLE_PTR(socktype(rth->rth_dest))->rt_table_tree = (radix_node *) 0;
	    return;
	}

	if (rn_prev->rnode_left == rn) {
	    rn_prev->rnode_left = (radix_node *) 0;
	} else {
	    assert(rn_prev->rnode_right == rn);
	    rn_prev->rnode_right = (radix_node *) 0;
	}

	if (rn_prev->rnode_rth) {
	    return;
	}
	rn = rn_prev;
    }

    /*
     * Here we have a one-way brancher with no external stuff attached
     * (either we just removed the external stuff or one of his child
     * nodes).  Remove him, promoting his one remaining child.
     */
    rn_prev = rn->rnode_parent;
    if (rn->rnode_left) {
	rn_next = rn->rnode_left;
    } else {
	rn_next = rn->rnode_right;
    }
    rn_next->rnode_parent = rn_prev;

    if (!rn_prev) {
	/*
	 * Our guy's a new root node, put him in.
	 */
	RT_TABLE_PTR(socktype(rth->rth_dest))->rt_table_tree = rn_next;
    } else {
	/*
	 * Find the pointer to our guy in the parent and replace
	 * it with the pointer to our former child.
	 */
	if (rn_prev->rnode_left == rn) {
	    rn_prev->rnode_left = rn_next;
	} else {
	    assert(rn_prev->rnode_right == rn);
	    rn_prev->rnode_right = rn_next;
	}
    }

    /*
     * Done, blow this one away as well.
     */
    task_block_free(rt_node_block_index, (void_t) rn);
    rtt->rt_table_inodes--;
}


/**/

block_t rtlist_block_index = 0;

	    
/* Build a change list of all active routes */
/* 	Routes not to be advised and hidden routes are not located */
rt_list *
rthlist_active __PF1(af, int)
{
    rt_entry *rt;
    rt_list *rtl = (rt_list *) 0;

    switch (af) {
    case AF_UNSPEC:
	RT_TABLES(af1) {
	    radix_node *head = RT_TREETOP(af1);

	    if (head) {
		RT_TABLE(head, rt) {
		    if (BIT_TEST(rt->rt_state, RTS_NOADVISE|RTS_PENDING)) {
			/* Not to be announced */
			continue;
		    }
		    if (BIT_TEST(rt->rt_state, RTS_HIDDEN|RTS_DELETE)) {
			/* Deleted or hidden, unless in holddown */
			continue;
		    }
		    RTLIST_ADD(rtl, rt->rt_head);
		} RT_TABLE_END(head, rt) ;
	    }
	} RT_TABLES_END(af1) ;
	break;

    default:
	RT_TABLE(RT_TREETOP(af), rt) {
	    if (BIT_TEST(rt->rt_state, RTS_NOADVISE|RTS_PENDING)) {
		/* Not to be announced */
		continue;
	    }
	    if (BIT_TEST(rt->rt_state, RTS_HIDDEN|RTS_DELETE)) {
		/* Deleted or hidden, unless in holddown */
		continue;
	    }
	    RTLIST_ADD(rtl, rt->rt_head);
	} RT_TABLE_END(RT_TREETOP(af), rt) ;
    }

    return rtl ? rtl->rtl_root : rtl;
}


rt_list *
rtlist_all __PF1(af, int)
{
    register rt_entry *rt;
    register rt_list *rtl = (rt_list *) 0;

    switch (af) {
    case AF_UNSPEC:
	RT_TABLES(af1) {
	    radix_node *head = RT_TREETOP(af1);

	    if (head) {
		RT_WHOLE(head, rt) {
		    RTLIST_ADD(rtl, rt);
		} RT_WHOLE_END(head, rt) ;
	    }
	} RT_TABLES_END(af1) ;
	break;

    default:
	RT_WHOLE(RT_TREETOP(af), rt) {
	    RTLIST_ADD(rtl, rt);
	} RT_WHOLE_END(RT_TREETOP(af), rt) ;
    }

    return rtl ? rtl->rtl_root : rtl;
}


rt_list *
rthlist_all __PF1(af, int)
{
    register rt_head *rth;
    register rt_list *rtl = (rt_list *) 0;

    switch (af) {
    case AF_UNSPEC:
	RT_TABLES(af1) {
	    radix_node *head = RT_TREETOP(af1);

	    if (head) {
		RT_WALK(head, rth) {
		    RTLIST_ADD(rtl, rth);
		} RT_WALK_END(head, rth) ;
	    }
	} RT_TABLES_END(af1) ;
	break;

    default:
	RT_WALK(RT_TREETOP(af), rth) {
	    RTLIST_ADD(rtl, rth);
	} RT_WALK_END(RT_TREETOP(af), rth) ;
    }

    return rtl ? rtl->rtl_root : rtl;
}


#ifdef	DEBUG
PROTOTYPE(rthlist_print,
	  void,
	  (rt_list *));
void
rthlist_print __PF1(rtl, rt_list *)
{
    rt_head *rth;

    (void) fprintf(stderr,
		   "rthlist_print: %u entries:\n",
		   rtl ? rtl->rtl_count : 0);
    
    RT_LIST(rth, rtl, rt_head) {
	rt_entry *rt;
	
	(void) fprintf(stderr, "rthlist_print:\t%A/%u\tentries %u bits set %u depth %u\n",
		       rth->rth_dest,
		       mask_to_prefix(rth->rth_dest_mask),
		       rth->rth_entries,
		       rth->rth_n_announce,
		       rth->rth_aggregate_depth);

	RT_ALLRT(rt, rth) {
	    const char *active;
	    
	    if (rt == rth->rth_active && rt == rth->rth_last_active) {
		active = "*";
	    } else if (rt == rth->rth_active) {
		active = "+";
	    } else if (rt == rth->rth_last_active) {
		active = "-";
	    } else {
		active = "";
	    }

	    (void) fprintf(stderr, "\t%s%s\tpreference %d\tmetric %d/%d\tState: <%s>\n",
			   active,
			   trace_state(rt_proto_bits, rt->rt_gwp->gw_proto),
			   rt->rt_preference,
			   rt->rt_metric,
			   rt->rt_metric2,
			   trace_bits(rt_state_bits, rt->rt_state));
	} RT_ALLRT_END(rt, rth) ;
    } RT_LIST_END(rth, rtl, rt_head) ;
}
#endif	/* DEBUG */

#ifdef	RT_SANITY
/*
 *	Check the integrity of the routing table
 */
void
rt_sanity __PF0(void)
{
    rt_head *rth;
    rt_entry *rt;
    int bounds_error = FALSE;
    byte *high = (byte *) sbrk(0);
    extern byte *end;

#define	BOUNDS(p)	((byte *) (p) < end || (byte *) (p) > high)

    RT_TABLES(af) {
	radix_node *head = RT_TREETOP(af);

	if (head) {
	    RT_WALK(head, rth) {
		int route = 0;
		int head_error = FALSE;

		/* First check for bounds on radix tree */
		if (BOUNDS(Xrn)) {
		    trace_only_tf(trace_global,
				  0,
				  ("rt_sanity: bounds check on radix node pointer (%X)",
				   Xrn));
		    head_error = TRUE;
		    bounds_error = TRUE;
		} else if (BOUNDS(Xrn->rnode_parent)) {
		    trace_only_tf(trace_global,
				  0,
				  ("rt_sanity: bounds check on parent tree pointer (%X)",
				   Xrn->rnode_parent));
		    head_error = TRUE;
		    bounds_error = TRUE;
		    /* XXX - Some more checks on the tree */
		} else if (BOUNDS(rth)) {
		    trace_only_tf(trace_global,
				  0,
				  ("rt_sanity: bounds check on head pointer (%X)",
				   rth));
		    head_error = TRUE;
		    bounds_error = TRUE;
		} else {
		    /* If there is an active route, it had better be the first one */
		    if (rth->rth_active && rth->rth_active != rth->rt_forw) {
			trace_only_tf(trace_global,
				      0,
				      ("rt_sanity: active route (%X != %X)",
				       rth->rth_active,
				       rth->rt_forw));
			head_error = TRUE;
		    }

		    /* Then check the routes */
		    RT_ALLRT(rt, rth) {
			int route_error = FALSE;
		
			/* First check for bounds check on pointers */
			if (BOUNDS(rt)) {
			    trace_only_tf(trace_global,
					  0,
					  ("rt_sanity: bounds check on route pointer (%X)",
					   rt));
			    route_error = TRUE;
			    bounds_error = TRUE;
			} else {
			    if (rt->rt_head != rth) {
				trace_only_tf(trace_global,
					      0,
					      ("rt_sanity: upward pointer (%X != %X)",
					       rt->rt_head,
					       rth));
				route_error = TRUE;
			    }

			    if (BOUNDS(rt->rt_forw)) {
				trace_only_tf(trace_global,
					      0,
					      ("rt_sanity: bounds check on forward route pointer (%X)",
					       rt->rt_forw));
				route_error = TRUE;
				bounds_error = TRUE;
			    } else if (rt->rt_forw->rt_back != rt) {
				trace_only_tf(trace_global,
					      0,
					      ("rt_sanity: forward route pointer (%X != %X)",
					       rt->rt_forw->rt_back,
					       rt));
				route_error = TRUE;
			    }
			
			    if (BOUNDS(rt->rt_back)) {
				trace_only_tf(trace_global,
					      0,
					      ("rt_sanity: bounds check on backward route pointer (%X)",
					       rt->rt_back));
				route_error = TRUE;
				bounds_error = TRUE;
			    } else if (rt->rt_back->rt_forw != rt) {
				trace_only_tf(trace_global,
					      0,
					      ("rt_sanity: backward route pointer (%X != %X)",
					       rt->rt_back->rt_forw,
					       rt));
				route_error = TRUE;
			    }
			}
		    
			if (route_error) {
			    trace_only_tf(trace_global,
					  0,
					  ("rt_sanity: route %d",
					   route));
			    break;
			}
		
			route++;
		    } RT_ALLRT_END(rt, rth) ;

		    if (head_error) {
			trace_only_tf(trace_global,
				      0,
				      ("rt_sanity: dest %A mask %A",
				       rth->rth_dest,
				       rth->rth_dest_mask));
		    }
		}
	    } RT_WALK_END(head, rth) ;
	}
    } RT_TABLES_END(af) ;

    if (bounds_error) {
	trace_only_tf(trace_global,
		      0,
		      ("rt_sanity: bounds: low %#X high %#X",
		       end,
		       high));
    }
}
#endif	/* RT_SANITY */


#ifdef	PROTO_SNMP
static const byte rt_last_bit_set[256] = {
    0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
};

/*
 * rt_table_get - return a route which matches the requested route,
 *	and which is likely to be the same route that a getnext might
 *	have returned.  This deals as best it can with the fact that
 *	sometimes SNMP doesn't know the route masks.
 */
rt_entry *
rt_table_get(dest, mask, match_rtn, match_data)
sockaddr_un *dest;
sockaddr_un *mask;
_PROTOTYPE(match_rtn,
	   rt_entry *,
	   (rt_head *,
	    void_t));
void_t match_data;
{
    register radix_node *rn;
    register byte *ap, *ap2;
    register u_short bitlen;
    register int i;
    struct sock_info *si;
    rt_table *rtt;
    rt_head *rth;
    rt_entry *rt;

    /*
     * If there is no table, or nothing to do, assume nothing found.
     */
    if (!(rtt = RT_TABLE_PTR(socktype(dest))) || !(rn = rtt->rt_table_tree)) {
	return (rt_entry *) 0;
    }

    /*
     * If the route has a mask, find the mask's bit length.  Otherwise
     * find the minimum mask length this route might match.
     */
    si = rtt->rt_table_sinfo;
    ap = RN_ADDR(dest, si->si_offset);
    if (mask) {
	bitlen = mask_to_prefix_si(si, mask);
	assert(bitlen != (u_short) -1);
    } else {
	ap2 = ((byte *)dest) + socksize(dest);
	bitlen = (ap2 - ap) * RNBBY;
	while (ap2-- > ap) {
	    if (*ap2 != 0) {
		bitlen -= rt_last_bit_set[*ap2];
		break;
	    }
	    bitlen -= RNBBY;
	}
    }

    /*
     * Search down to a node matching the bit number of this guy's
     * address.
     */
    while (rn->rnode_bit < bitlen) {
	if (ap[rn->rnode_tbyte] & rn->rnode_tbit) {
	    rn = rn->rnode_right;
	} else {
	    rn = rn->rnode_left;
	}
	if (!rn) {
	    return (rt_entry *) 0;
	}
    }

    /*
     * If there is a mask, and either we didn't find a node with his
     * mask, or there is no external rt_head, or the addresses don't
     * match, we're done.
     */
    if (mask) {
	if (bitlen != rn->rnode_bit || !(rth = rn->rnode_rth)) {
	    return (rt_entry *) 0;
	}
    } else {
	while (rn->rnode_rth == (rt_head *) 0) {
	    rn = rn->rnode_left;
	    if (!rn) {
		return (rt_entry *) 0;
	    }
	}
	rth = rn->rnode_rth;
	bitlen = rn->rnode_bit;
    }

    ap2 = RN_ADDR(rth->rth_dest, si->si_offset);
    for (i = 0; i < bitlen; i += RNBBY) {
	if (*ap++ != *ap2++) {
	    return (rt_entry *) 0;
	}
    }

    /*
     * See if there is a matched route.  If there is, or if he
     * included a mask, return this result.
     */
    rt = match_rtn(rth, match_data);
    if (rt || mask) {
	return rt;
    }

    /*
     * If he didn't include a mask, search down looking for other
     * possible matches.  We know these will all be on the left
     * of where we are since all the bits in the address from
     * here on are zero.
     */
    while ((rn = rn->rnode_left)) {
	if ((rth = rn->rnode_rth)) {
	    ap = RN_ADDR(dest, si->si_offset);
	    ap2 = RN_ADDR(rth->rth_dest, si->si_offset);
	    for (i = 0; i < rn->rnode_bit; i += RNBBY) {
		if (*ap++ != *ap2++) {
		    return (rt_entry *) 0;
		}
	    }
	    rt = match_rtn(rth, match_data);
	    if (rt) {
		return rt;
	    }
	}
    }

    return (rt_entry *) 0;
}


/*
 * rt_table_getnext - return a route which is after the given route,
 *	in lexigraphic order.  The code assumes that "lexigraphic order"
 *	means that you return an address which sorts larger than the given
 *	address, or the same address but with a longer mask.  If the mask
 *	is specified as NULL it is assumed to be equal to a host mask, so
 *	all entries with the same address but different masks are skipped.
 *	If the dest is NULL it assumes he wants the first route in the table.
 */
rt_entry *
rt_table_getnext (dest, mask, af, match_rtn, match_data)
sockaddr_un *dest;
sockaddr_un *mask;
int af;
_PROTOTYPE(match_rtn,
	   rt_entry *,
	   (rt_head *,
	    void_t));
void_t match_data;
{
    register radix_node *rn;
    register byte *ap, *ap2;
    u_short bitlen, bits2chk, dbit;
    struct sock_info *si;
    rt_table *rtt;
    rt_entry *rt;

    /*
     * If there is no table, or nothing to do, assume nothing found.
     */
    if (!(rtt = RT_TABLE_PTR(af)) || !(rn = rtt->rt_table_tree)) {
	return (rt_entry *) 0;
    }

    /*
     * The first job here is to find a node in the tree which is
     * known to be after the given dest/mask in lexigraphic order.
     * If no mask was given use a host mask, as this guarantees we
     * will skip all same-address entries in the table.  First compute
     * the bit length of the mask.  If he gave us no destination we
     * start from the top of the tree, otherwise we search.
     */
    si = rtt->rt_table_sinfo;
    if (!mask) {
	mask = si->si_mask_max;
    }
    bitlen = mask_to_prefix_si(si, mask);
    assert(bitlen != (u_short) -1);

    if (dest) {
	/*
	 * Search down the tree as far as we can until we find a node
	 * which has a bit number the same or larger than ours which has
	 * an rth attached.
	 */
	ap = RN_ADDR(dest, si->si_offset);
	while (rn->rnode_bit < bitlen || rn->rnode_rth == (rt_head *) 0) {
	    if (rn->rnode_tbit & ap[rn->rnode_tbyte]) {
		if (!(rn->rnode_right)) {
		    break;
		}
		rn = rn->rnode_right;
	    } else {
		if (!(rn->rnode_left)) {
		    break;
		}
		rn = rn->rnode_left;
	    }
	}

	/*
	 * Determine the bit position of the first bit which differs between
	 * the destination we found and the one we were given, as this will
	 * suggest where we should search.  Often this will be an exact
	 * match.
	 */
	bits2chk = MIN(rn->rnode_bit, bitlen);
	ap2 = RN_ADDR(rn->rnode_rth->rth_dest, si->si_offset);
	for (dbit = 0; dbit < bits2chk; dbit += RNBBY) {
	    register int i = dbit >> RNSHIFT;
	    if (ap[i] != ap2[i]) {
		dbit += first_bit_set[ap[i] ^ ap2[i]];
		break;
	    }
	}

	/*
	 * If we got an exact match, this is either our node (if his mask
	 * is longer than ours) or we only need to find the next node in
	 * the tree.  Do this now since this may be the normal case and
	 * is fairly easy.
	 */
	if (dbit >= bits2chk) {
	    if (rn->rnode_bit <= bitlen) {
		if (rn->rnode_left) {
		    rn = rn->rnode_left;
		} else if (rn->rnode_right) {
		    rn = rn->rnode_right;
		} else {
		    register radix_node *rn_next;
		    do {
			rn_next = rn;
			rn = rn->rnode_parent;
			if (!rn) {
			    return (rt_entry *) 0;
			}
		    } while (!(rn->rnode_right) || rn->rnode_right == rn_next);
		    rn = rn->rnode_right;
		}
	    }
	} else {
	    register radix_node *rn_next;

	    /*
	     * Here we found a node which differs from our target destination
	     * in the low order bits.  We need to determine whether our guy
	     * is too big, or too small, for the branch we are in.  If
	     * he is too big, walk up until we find a node with a smaller
	     * bit number than dbit where we branched left and search to
	     * the right of this.  Otherwise all the guys in this branch
	     * will be larger than us, so walk up the tree to the first
	     * node we a bit number > dbit and search from there
	     */
	    if (ap[RNBYTE(dbit)] & RNBIT(dbit)) {
		do {
		    rn_next = rn;
		    rn = rn_next->rnode_parent;
		    if (!rn) {
			return (rt_entry *) 0;
		    }
		    assert(rn->rnode_bit != dbit);
		} while (rn->rnode_bit > dbit || (!(rn->rnode_right)
		    || rn->rnode_right == rn_next));
		rn = rn->rnode_right;
	    } else {
		rn_next = rn->rnode_parent;
		while (rn_next && rn_next->rnode_bit > dbit) {
		    rn = rn_next;
		    rn_next = rn_next->rnode_parent;
		}
	    }
	}
    }

    /*
     * If we have a pointer to a radix node which is in an area of the
     * tree where the address/masks are larger than our own.  Walk the
     * tree from here, checking each node with an rth attached until
     * we find one which matches our criteria.
     */
    for (;;) {
	if (rn->rnode_rth) {
	    rt = (match_rtn)(rn->rnode_rth, match_data);
	    if (rt) {
		break;
	    }
	}

	if (rn->rnode_left) {
	    rn = rn->rnode_left;
	} else if (rn->rnode_right) {
	    rn = rn->rnode_right;
	} else {
	    register radix_node *rn_next;
	    do {
		rn_next = rn;
		rn = rn->rnode_parent;
		if (!rn) {
		    return (rt_entry *) 0;
		}
	    } while (!(rn->rnode_right) || rn->rnode_right == rn_next);
	    rn = rn->rnode_right;
	}
    }

    return rt;
}
#endif	/* PROTO_SNMP */


/*
 *	Display information about the radix tree, including a visual
 *	representation of the tree itself 
 */
void
rt_table_dump __PF2(tp, task *,
		    fd, FILE *)
{
    struct {
	radix_node *rn;
	int	state;
	char	right;
    } stack[MAXDEPTH], *sp;
    char prefix[MAXDEPTH];
    int i = MAXDEPTH;
    rt_table *rtt;
    char number[4];

    while (i--) {
	prefix[i] = ' ';
    }

    RT_ALLTABLES(rtt) {
	sp = stack;
	(void) fprintf(fd, "\tRadix trie for %s (%d) inodes %u routes %u:",
		       gd_lower(trace_value(task_domain_bits,
				(int)rtt->rt_table_sinfo->si_family)),
		       rtt->rt_table_sinfo->si_family,
		       rtt->rt_table_inodes,
		       rtt->rt_table_routes);
	if (rtt->rt_table_inodes > 200) {
	    (void) fprintf(fd, " (too large to print)\n\n");
	} else if (!(sp->rn = rtt->rt_table_tree)) {
	    (void) fprintf(fd, " (empty)\n\n");
	} else {
	    /* If the tree is small enough, format it */
	    (void) fprintf(fd, "\n\n");
	    sp->right = TRUE;
	    sp->state = 0;
	    do {
		radix_node *rn = sp->rn;
		int ii = (sp - stack) * 3;

		switch (sp->state) {
		case 0:
		    sp->state++;
		    if (rn->rnode_right) {
			sp++;
			sp->rn = rn->rnode_right;
			sp->right = TRUE;
			sp->state = 0;
			continue;
		    }
		    /* Fall through */

		case 1:
		    sp->state++;
	
		    (void) sprintf(number, "%3d", rn->rnode_bit);
		    for (i = 0; i < 3 && number[i] == ' '; i++) {
			number[i] = '-';
		    }
		    (void) fprintf(fd, "\t\t%.*s+-%s%u+",
				   ii, prefix,
				   (rn->rnode_bit > 9) ? "" : "-",
				   rn->rnode_bit);
		    if (rn->rnode_rth) {
			(void) fprintf(fd, "--{%A\n",
				       rn->rnode_rth->rth_dest);
		    } else {
			(void) fprintf(fd, "\n");
		    }

		    switch (sp->right) {
		    case TRUE:
			prefix[ii] = '|';
			break;

		    case FALSE:
			prefix[ii] = ' ';
			break;
		    }

		    if (rn->rnode_left) {
			sp++;
			sp->rn = rn->rnode_left;
			sp->right = FALSE;
			sp->state = 0;
			continue;
		    }

		case 2:
		    /* Pop the stack */
		    sp--;
		}
	    } while (sp >= stack) ;
	}
    } RT_ALLTABLES_END(rtt) ;

    (void) fprintf(fd, "\n");
}


void
rt_table_init_family __PF1(af, int)
{
    rt_table *rtt, *rtt_prev, *rtt_next;

    trace_only_tf(trace_global,
		  0,
		  ("rt_table_init_family: Initializing radix tree for %s",
		   trace_value(task_domain_bits, af)));

    rtt = (rt_table *) task_block_alloc(rt_table_block_index);
    rtt->rt_table_sinfo = &(sock_info[af]);
    rt_tables[af] = rtt;

    rtt_prev = (rt_table *) 0;
    rtt_next = rt_table_list;
    while (rtt_next && rtt_next->rt_table_sinfo->si_family < af) {
	rtt_prev = rtt_next;
	rtt_next = rtt_prev->rt_table_next;
    }

    rtt->rt_table_next = rtt_next;
    if (rtt_prev) {
	rtt_prev->rt_table_next = rtt;
    } else {
	rt_table_list = rtt;
    }

    rt_tables[af] = rtt;

    trace_only_tf(trace_global,
		  0,
		  (NULL));
}


/*
 *	Initialize rt heads for all protocols 
 */
void
rt_table_init __PF0(void)
{
    rt_table_block_index = task_block_init(sizeof (rt_table), "rt_table");
    rt_node_block_index = task_block_init(sizeof (radix_node), "radix_node");
}
