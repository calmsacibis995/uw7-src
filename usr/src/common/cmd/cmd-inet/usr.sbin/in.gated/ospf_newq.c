#ident	"@(#)ospf_newq.c	1.3"
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
 * ------------------------------------------------------------------------
 * 
 *                 U   U M   M DDDD     OOOOO SSSSS PPPPP FFFFF
 *                 U   U MM MM D   D    O   O S     P   P F
 *                 U   U M M M D   D    O   O  SSS  PPPPP FFFF
 *                 U   U M M M D   D    O   O     S P     F
 *                  UUU  M M M DDDD     OOOOO SSSSS P     F
 * 
 *     		          Copyright 1989, 1990, 1991
 *     	       The University of Maryland, College Park, Maryland.
 * 
 * 			    All Rights Reserved
 * 
 *      The University of Maryland College Park ("UMCP") is the owner of all
 *      right, title and interest in and to UMD OSPF (the "Software").
 *      Permission to use, copy and modify the Software and its documentation
 *      solely for non-commercial purposes is granted subject to the following
 *      terms and conditions:
 * 
 *      1. This copyright notice and these terms shall appear in all copies
 * 	 of the Software and its supporting documentation.
 * 
 *      2. The Software shall not be distributed, sold or used in any way in
 * 	 a commercial product, without UMCP's prior written consent.
 * 
 *      3. The origin of this software may not be misrepresented, either by
 *         explicit claim or by omission.
 * 
 *      4. Modified or altered versions must be plainly marked as such, and
 * 	 must not be misrepresented as being the original software.
 * 
 *      5. The Software is provided "AS IS". User acknowledges that the
 *         Software has been developed for research purposes only. User
 * 	 agrees that use of the Software is at user's own risk. UMCP
 * 	 disclaims all warrenties, express and implied, including but
 * 	 not limited to, the implied warranties of merchantability, and
 * 	 fitness for a particular purpose.
 * 
 *     Royalty-free licenses to redistribute UMD OSPF are available from
 *     The University Of Maryland, College Park.
 *       For details contact:
 * 	        Office of Technology Liaison
 * 		4312 Knox Road
 * 		University Of Maryland
 * 		College Park, Maryland 20742
 * 		     (301) 405-4209
 * 		FAX: (301) 314-9871
 * 
 *     This software was written by Rob Coltun
 *      rcoltun@ni.umd.edu
 */


#define	INCLUDE_TIME
#include "include.h"
#include "inet.h"
#include "ospf.h"

/**/
/* Neighbors */

#define OSPF_NBR_NOMASK     0xffffffff

/*
 * Lookup table to do an ffs() from the high order end (the ffs() routine
 * runs from the other end), on a byte.  This is useful to find the first
 * differing bit for the ospf_nbr tree.
 */
static const byte ospf_nbr_bit_table[256] = {
    0x00, 0x01, 0x02, 0x02, 0x04, 0x04, 0x04, 0x04,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80
};


/*
 * ospf_nbr_insert - add a node to the demux tree.  Note that
 *		   found_dp points to the node that was "found" when the
 *		   tree was searched for the address we are adding.
 *		   If it is NULL we do the search on our own.
 */
static void
ospf_nbr_insert  __PF4(intf, struct INTF *,
		       found_dp, ospf_nbr_node *,
		       key, u_int32,
		       ospf_nbr, struct NBR *)
{
    register ospf_nbr_node *dp;
    register ospf_nbr_node *dprev, *dcurr;
    register u_int32 mask, omask;

    /*
     * Fetch a new demux structure and initialize it
     */
    dp = (ospf_nbr_node *) task_block_alloc(ospf_nbr_node_index);
    dp->ospf_nbr_key = key;
    dp->ospf_nbr_nbr = ospf_nbr;

    /*
     * If he didn't include a "found" demux node, but there are nodes
     * to "find", "find" it.  If there are no nodes to find then
     * this is the first node in the tree and just needs to get
     * initialized appropriately.
     */
    if (!found_dp) {
	if (!(dcurr = intf->nbr_tree)) {
	    intf->nbr_tree = dp->right = dp->left = dp;
	    dp->ospf_nbr_mask = OSPF_NBR_NOMASK;
	    return;
        }
	mask = OSPF_NBR_NOMASK;
	while (mask > dcurr->ospf_nbr_mask) {
	    mask = dcurr->ospf_nbr_mask;
	    if (key & mask) {
		dcurr = dcurr->right;
	    } else {
		dcurr = dcurr->left;
	    }
	}
	found_dp = dcurr;
    }

    /*
     * Here we have an insertion to do.  Figure out the first
     * (most significant) bit which differs from the "found" node.  This
     * tells us the bit we need to test in the node we insert.
     *
     * Do a binary search to find the byte the first bit is in, then use
     * the table to form the mask.
     */
    mask = key ^ found_dp->ospf_nbr_key;
    if (mask >= 0x10000) {
	if (mask >= 0x1000000) {
	    mask = ((u_int32)(ospf_nbr_bit_table[mask >> 24])) << 24;
	} else {
	    mask = ((u_int32)(ospf_nbr_bit_table[mask >> 16])) << 16;
	}
    } else {
	if (mask >= 0x100) {
	    mask = ((u_int32)(ospf_nbr_bit_table[mask >> 8])) << 8;
	} else {
	    mask = (u_int32)(ospf_nbr_bit_table[mask]);
	}
    }

    /*
     * XXX Sanity.  This could only happen if a node with the same key
     * is in the tree already, which should never happen.
     */
    assert(mask != 0);
    dp->ospf_nbr_mask = mask;

    /*
     * Now we locate where this guy needs to be inserted.  Search down
     * the tree until we find either an end node (i.e. an upward link)
     * or a node with a mask smaller than ours.  We insert our node
     * above this (watch for the case where the node we find is the
     * current root node), making one of the pointers an uplink pointing
     * at the node itself.
     */
    dprev = (ospf_nbr_node *) 0;
    dcurr = intf->nbr_tree;
    omask = OSPF_NBR_NOMASK;
    while (omask > dcurr->ospf_nbr_mask) {
	omask = dcurr->ospf_nbr_mask;
	if (mask >= omask) {
	    break;
	}
	dprev = dcurr;
	if (key & omask) {
	    dcurr = dcurr->right;
	} else {
	    dcurr = dcurr->left;
	}
    }
    assert(mask != dcurr->ospf_nbr_mask);

    /*
     * Point the new node at the current node, and at itself.  The
     * pointer to the current node in the previous node is changed to
     * point at the new node.  Simple(?).
     */
    if (key & mask) {
	dp->right = dp;
	dp->left = dcurr;
    } else {
	dp->right = dcurr;
	dp->left = dp;
    }

    if (!dprev) {
	/* New root node */
	intf->nbr_tree = dp;
    } else if (dprev->right == dcurr) {
	dprev->right = dp;
    } else {
	dprev->left = dp;
    }
    /* done */
}


/*
 * ospf_nbr_remove - remove a node from the ospf nbr tree.
 */
static void
ospf_nbr_remove  __PF2(intf, struct INTF *,
		   remove_dp, ospf_nbr_node *)
{
    register ospf_nbr_node *dp = remove_dp;
    register ospf_nbr_node *tmp;
    register ospf_nbr_node *dprev, *dcurr;
    register u_int32 key;
    ospf_nbr_node *intprev;

    /*
     * Detect the trivial case, that there is only one guy in the tree,
     * and fix this up.
     */
    dcurr = intf->nbr_tree;
    if (dcurr->ospf_nbr_mask == OSPF_NBR_NOMASK) {
	assert(dcurr == dp);
	task_block_free(ospf_nbr_node_index, (void_t) dp);
	intf->nbr_tree = (ospf_nbr_node *) 0;
	return;
    }

    /*
     * We will be in the tree twice, once as an internal node and once
     * as an external (or up-)node.  Find the guy who points to our man
     * in the tree in both cases, and the guy who points to him in the
     * latter.
     */
    dprev = (ospf_nbr_node *) 0;
    intprev = (ospf_nbr_node *) 0;
    key = dp->ospf_nbr_key;
    for (;;) {
	if (key & dcurr->ospf_nbr_mask) {
	    tmp = dcurr->right;
	} else {
	    tmp = dcurr->left;
	}
	if (tmp == dp) {
	    if (dp->ospf_nbr_mask >= dcurr->ospf_nbr_mask) {
		break;		/* got it */
	    }
	    intprev = dcurr;	/* current is in front of internal */
	} else {
	    assert(tmp->ospf_nbr_mask < dcurr->ospf_nbr_mask);
	}
	dprev = dcurr;
	dcurr = tmp;
    }

    /*
     * Now we have dcurr pointing at the node which contains the up-link
     * to our deleted node, nprev pointing at the node which points to
     * ncurr and intprev pointing at the guy who precedes our node in its
     * internal incarnation.
     *
     * There are several cases here.  We know the information in dcurr
     * is no longer needed, and that the sibling pointer in dcurr will
     * be moved to dprev (replacing dcurr).  If our node is the special
     * one with no internal information, we make dcurr the special.
     * If dcurr is our node (i.e. intprev == dprev) then we simply
     * point nprev at our sibling and we're done.  Otherwise we
     * additionally copy our information to dcurr, and point intprev
     * at dcurr.
     *
     * There is probably a better way to do this, but I'm an engineer,
     * not a (mere) programmer.
     */
    if (dcurr->right == dp) {
	tmp = dcurr->left;
    } else {
	tmp = dcurr->right;
    }
    if (!dprev) {
	intf->nbr_tree = tmp;
    } else if (dprev->right == dcurr) {
	dprev->right = tmp;
    } else {
	dprev->left = tmp;
    }
    if (dcurr != dp) {
	if (dp->ospf_nbr_mask == OSPF_NBR_NOMASK) {
	    dcurr->ospf_nbr_mask = OSPF_NBR_NOMASK;
	    dcurr->left = dcurr->right = dcurr;
	} else {
	    dcurr->ospf_nbr_mask = dp->ospf_nbr_mask;
	    dcurr->left = dp->left;
	    dcurr->right = dp->right;
	    if (!intprev) {
		intf->nbr_tree = dcurr;
	    } else if (intprev->right == dp) {
		intprev->right = dcurr;
	    } else {
		intprev->left = dcurr;
	    }
	}
    }

    task_block_free(ospf_nbr_node_index, (void_t)dp);
}


#ifdef	notdef
/*
 * ospf_nbr_dump - dump entire ospf nbr tree
 */
void
ospf_nbr_dump  __PF2(fp, FILE *,
		     intf, struct INTF *)

{
    int i,n,doing_right;
    ospf_nbr_node *stack[sizeof(u_int32) * NBBY];
    register ospf_nbr_node *dp, *dprev = intf->nbr_tree;

    /* catch the case where only one node in the tree. */
    if (dprev->ospf_nbr_mask == OSPF_NBR_NOMASK) {
	fprintf(fp, "key %0x, mask %0x\n",
		dprev->ospf_nbr_key, dprev->ospf_nbr_mask);
        return;
    }

    doing_right = 0;
    i = n = 0;
    for (;;) {
        if (doing_right) {
            dp = dprev->right;
        } else {
            dp = dprev->left;
        }

        if (dp->ospf_nbr_mask >= dprev->ospf_nbr_mask) {

	    fprintf(fp, "key %0x, mask %0x\n",
		dp->ospf_nbr_key, dp->ospf_nbr_mask);

	    if (++n >= ospf_nbr_count) {
		/* All found */
		break;
	    }

            if (doing_right) {
                /*
                 * Finished right hand, back up stack for another
                 * node to be checked on the right.
                 */
                assert(i != 0);
                dprev = stack[--i];
            } else {
                /*
                 * Finished left, try right on next pass
                 */
                doing_right = 1;
            }
        } else {
            if (doing_right) {
                /*
                 * Node on right has children.  Step down the tree,
                 * starting on the left.  No need to stack the previous,
                 * we've already checked both children
                 */
                doing_right = 0;
            } else {
                /*
                 * Node on left has children, stack the previous node
                 * so we check its right child later.
                 */
                stack[i++] = dprev;
            }
            dprev = dp;
        }
    }
}
#endif	/* notdef */


/*
 * ospf_nbr_delete - delete an ospf_nbr from the tree
 */
void
ospf_nbr_delete  __PF2(intf, struct INTF *,
		       ospf_nbr, struct NBR *)
{
    register ospf_nbr_node *dp = intf->nbr_tree;
    register u_int32 key = NBR_ADDR(ospf_nbr);
    register u_int32 mask;

#ifdef	TR_OSPF_DEBUG
    trace_tf(ospf.trace_options,
	     TR_OSPF_DEBUG,
	     0,
	     ("ospf_nbr_delete: interface %A(%s) DELETE neighbor %A (ID %A)",
	      intf->ifap->ifa_addr,
	      intf->ifap->ifa_link->ifl_name,
	      ospf_nbr->nbr_addr,
	      ospf_nbr->nbr_id ? ospf_nbr->nbr_id : sockbuild_str("Unknown")));
#endif	/* TR_OSPF_DEBUG */

    /*
     * Find the address in the tree.  It must be there.
     */
    assert(dp);
    do {
	mask = dp->ospf_nbr_mask;
	if (key & mask) {
	    dp = dp->right;
	} else {
	    dp = dp->left;
	}
    } while (mask > dp->ospf_nbr_mask);
    assert(key == dp->ospf_nbr_key);

    /*
     * remove the node from the tree
     */
    ospf_nbr_remove(intf, dp);

    if (ospf_nbr->nbr_id) {
	sockfree(ospf_nbr->nbr_id);
    }
    if (ospf_nbr->nbr_addr) {
	sockfree(ospf_nbr->nbr_addr);
    }
    ospf_nh_free(&ospf_nbr->nbr_nh);
    task_block_free(ospf_nbr_index, (void_t) ospf_nbr);
    ospf.nbrcnt--;
}


/*
 * ospf_nbr_add - add an ospf nbr to the ospf nbr tree
 */
void
ospf_nbr_add  __PF2(intf, struct INTF *,
		    ospf_nbr, struct NBR *)
{
    register ospf_nbr_node *dp = intf->nbr_tree;
    register u_int32 key = NBR_ADDR(ospf_nbr);
    struct NBR *last_nbr;

    for (last_nbr = &intf->nbr; ; last_nbr = last_nbr->next) {
        if (last_nbr->next == NBRNULL ||
	    ntohl(NBR_ADDR(last_nbr->next)) > ntohl(NBR_ADDR(ospf_nbr))) {
	    ospf_nbr->next = last_nbr->next;
	    last_nbr->next = ospf_nbr;
	    break;
        }
    }

    ospf.nbrcnt++;
#ifdef	notdef
    ospf.nbr_sb_not_valid = TRUE;
#endif	/* notdef */

    /*
     * Try to find the address in the tree
     */
    if (dp) {
	register u_int32 mask;
	do {
	    mask = dp->ospf_nbr_mask;
	    if (key & mask) {
		dp = dp->right;
	    } else {
		dp = dp->left;
	    }
	} while (mask > dp->ospf_nbr_mask);
    }

    /*
     * Put the neighbour in the tree.  He may attach to an
     * existing node, or may require a new node inserted.
     */
    if (dp && dp->ospf_nbr_key == key) {
	/* should already be there */
	assert(NBR_ADDR(ospf_nbr) == NBR_ADDR(dp->ospf_nbr_nbr));
    } else {
	ospf_nbr_insert(intf, dp, key, ospf_nbr);
    }

#ifdef	TR_OSPF_DEBUG
    trace_tf(ospf.trace_options,
	     TR_OSPF_DEBUG,
	     0,
	     ("ospf_nbr_add: interface %A(%s) ADD neighbor %A (ID %A)",
	      intf->ifap->ifa_addr,
	      intf->ifap->ifa_link->ifl_name,
	      ospf_nbr->nbr_addr,
	      ospf_nbr->nbr_id ? ospf_nbr->nbr_id : sockbuild_str("Unknown")));
#endif	/* TR_OSPF_DEBUG */
}


/**/
inline struct ospf_lsdb_list *
find_db_ptr __PF2(nbr, struct NBR *,
		  db, struct LSDB *)
{
    register struct ospf_lsdb_list *lp = nbr->retrans[XHASH_QUEUE(db)].ptr[NEXT];

    while (lp && lp->lsdb < db) {
	lp = lp->ptr[NEXT];
    }

    if (lp && lp->lsdb == db) {
	return lp;
    }

    return (struct ospf_lsdb_list *) 0;
}

int
rem_db_ptr __PF2(nbr, struct NBR *,
		 db, struct LSDB *)
{
    register struct ospf_lsdb_list *lp = find_db_ptr(nbr, db);

    if (lp) {
	REM_DB_PTR(nbr, lp);
	return TRUE;
    }

    return FALSE;
}


#ifdef	notdef
/*
 * Event: new lsa generated and Area's LINK_LIST has pointer to
 *  all neighbors: link_ptr in nbr has to be removed
 *	- search for link pointer and remove it from a neighbor's list
 *	- remove from nbr
 */
int
rem_db_ptr __PF2(nbr, struct NBR *,
		 db, struct LSDB *)
{
    register struct ospf_lsdb_list *lp = nbr->retrans[XHASH_QUEUE(db)].ptr[NEXT];

    while (lp && lp->lsdb < db) {
	lp = lp->ptr[NEXT];
    }

    if (lp && lp->lsdb == db) {
	DEL_Q(lp, ospf_lsdblist_index);
	nbr->rtcnt--;
	return TRUE;
    }

    return FALSE;
}
#endif	/* notdef */


/*
 * Event: ack has been received and neighbor has pointer to LINK_LIST
 *	  or nbr's state has dropped and are freeing nbr's lsdb list
 *	- search for nbr pointer in lsdb's nbr list and remove it from list
 */
int
rem_nbr_ptr __PF2(db, struct LSDB *,
		  nbr, struct NBR *)
{
    struct NBR_LIST *nl;

    for (nl = db->lsdb_retrans; nl != NLNULL; nl = nl->ptr[NEXT]) {
	if (nl->nbr == nbr) {
	    REM_Q(db->lsdb_retrans, nl, ospf_nbrlist_index);
	    return TRUE;
	}
    }
    return FALSE;
}

void
ospf_freeq __PF2(qhp, struct Q **,
		 type, block_t)
{
    struct Q *q;

    for (q = *qhp; *qhp != QNULL;) {
	*qhp = q->ptr[NEXT];
	task_block_free(type, (void_t) q);
	q = *qhp;
    }
}

/*
 *  add_nbr_retrans - add an lsdb ptr to the nbr's retrans list
 */
void
add_nbr_retrans __PF2(nbr, struct NBR *,
		      db, struct LSDB *)
{
    struct ospf_lsdb_list *hp = &nbr->retrans[XHASH_QUEUE(db)];
    struct ospf_lsdb_list *ll;
    register struct ospf_lsdb_list *lp = hp;

    ll = (struct ospf_lsdb_list *) task_block_alloc(ospf_lsdblist_index);
    ll->lsdb = db;
    nbr->rtcnt++;

    if (!lp) {
	/* First entry on list */

	ADD_Q(hp, ll);
    } else {
	/* Insert in order */

	while (lp->ptr[NEXT] && lp->ptr[NEXT]->lsdb < db) {
	    lp = lp->ptr[NEXT] ;
	}

	ADD_Q(lp, ll);
    }
}

/*
 *  add_db_retrans - add a nbr ptr to the lsdb's retrans list
 */
void
add_db_retrans __PF2(db, struct LSDB *,
		     nbr, struct NBR *)
{
    struct NBR_LIST *nl;

    nl = (struct NBR_LIST *) task_block_alloc(ospf_nbrlist_index);
    nl->nbr = nbr;
    EN_Q(db->lsdb_retrans, nl);
}


/*
 * rem_db_retrans - clear lsdb's retrans list and free lsdb prtr from neighbors
 */
void
rem_db_retrans __PF1(db, struct LSDB *)
{
    struct NBR_LIST *nl, *next_nl;

    /* remove from all nbrs' lists */
    for (nl = db->lsdb_retrans; nl != NLNULL; nl = next_nl) {
	next_nl = nl->ptr[NEXT];
	/* remove from nbr */
	(void) rem_db_ptr(nl->nbr, db);
	/* remove from lsdb's nbr list */
	REM_Q(db->lsdb_retrans, nl, ospf_nbrlist_index);
    }
    db->lsdb_retrans = NLNULL;
}

/*
 * rem_nbr_retrans - remove all lsdb ptrs from nbr retrans list
 */
void
rem_nbr_retrans __PF1(nbr, struct NBR *)
{
    struct ospf_lsdb_list *ll;
    register struct ospf_lsdb_list *hp = (struct ospf_lsdb_list *) nbr->retrans;
    register struct ospf_lsdb_list *limit = hp + OSPF_HASH_QUEUE;

    /* remove from all nbrs' lists */
    do {
	while ((ll = hp->ptr[NEXT])) {

	    /* remove from lsdb */
	    (void) rem_nbr_ptr(ll->lsdb, nbr);

	    /* remove from nbr's retrans list */
	    DEL_Q(ll, ospf_lsdblist_index);
	    nbr->rtcnt--;
	}
    } while (++hp < limit) ;

    assert(!nbr->rtcnt);
}


/*
 * Free nbr's db summary list
 */
void
freeDbSum __PF1(nbr, struct NBR *)
{
    struct LSDB_SUM *nextd;

    for (nextd = nbr->dbsum; nextd != LSDB_SUM_NULL;) {
	nbr->dbsum = nextd->next;
	dbsum_free(nextd);
	nextd = nbr->dbsum;
    }
    nbr->dbsum = LSDB_SUM_NULL;
    nbr->dbcnt = 0;
}

/*
 * Free nbr's ls request list
 */
void
freeLsReq __PF1(nbr, struct NBR *)
{
    int type;

    for (type = LS_RTR; type <= LS_ASE; type++) {
	if (nbr->ls_req[type] != LS_REQ_NULL) {
	    ospf_freeq((struct Q **) &nbr->ls_req[type], ospf_lsreq_index);
	    nbr->ls_req[type] = LS_REQ_NULL;
	}
    }
    nbr->reqcnt = 0;
}

/*
 * Free list of acks to be sent to this nbr
 */
void
freeAckList __PF1(intf, struct INTF *)
{
    struct LS_HDRQ *ack = intf->acks.ptr[NEXT];

    intf->acks.ptr[NEXT] = (struct LS_HDRQ *) 0;

    while (ack) {
	struct LS_HDRQ *next = ack->ptr[NEXT];

	task_block_free(ospf_hdrq_index, (void_t) ack);

	ack = next;
    }
}


/*
 * Free list of NetRanges for area
 */
void
ospf_freeRangeList __PF1(area, struct AREA *)
{
    struct NET_RANGE *nr = area->nr.ptr[NEXT];
 
    area->nr.ptr[NEXT] = (struct NET_RANGE *) 0;
 
    while (nr) {
 	struct NET_RANGE *next = nr->ptr[NEXT];

	task_block_free(ospf_netrange_index, (void_t) nr);
 	
 	area->nrcnt--;
 	nr = next;
     }
}


/*
 * Free lists of hosts for area
 */
void
ospf_freeHostsList __PF1(area, struct AREA *)
{
    struct OSPF_HOSTS *hp = area->hosts.ptr[NEXT];

    area->hosts.ptr[NEXT] = HOSTSNULL;

    while (hp) {
	struct OSPF_HOSTS *next = hp->ptr[NEXT];

	task_block_free(ospf_hosts_index, (void_t) hp);

	area->hostcnt--;
	hp = next;
    }
}


/*
 * Add net range to area in sorted order for ease of MIB ness
 */
void
range_enq __PF2(area, struct AREA *,
		range, struct NET_RANGE *)
{
    struct NET_RANGE *nr = &area->nr;

    do {
        if (nr->ptr[NEXT] == NRNULL 
	    || ntohl(nr->ptr[NEXT]->net) > ntohl(range->net)
	    || (ntohl(nr->ptr[NEXT]->net) == ntohl(range->net)
		&& ntohl(nr->ptr[NEXT]->mask) > ntohl(range->mask))) {
	    ADD_Q(nr, range);
	    return;
        }
    } while ((nr = nr->ptr[NEXT])) ;
}

/*
 * Add host to area in sorted order for ease of MIB ness
 */
void
host_enq __PF2(area, struct AREA *,
	       host, struct OSPF_HOSTS *)
{
    struct OSPF_HOSTS *h;

    for(h = &area->hosts; ; h = h->ptr[NEXT]) {
        if (h->ptr[NEXT] == HOSTSNULL ||
	    ntohl(h->ptr[NEXT]->host_if_addr) > ntohl(host->host_if_addr)) {
	    ADD_Q(h, host);
	    return;
        }
    }
}
