#ident	"@(#)rt_static.c	1.3"
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
#ifdef	PROTO_INET
#include "inet.h"
#endif	/* PROTO_INET */
#ifdef	PROTO_ISO
#include "iso.h"
#endif	/* PROTO_ISO */


/* Parser code for adding static routes */

typedef struct _rt_static {
    struct _rt_static *rts_next;
    rt_entry	*rts_route;
    adv_entry	*rts_intfs;
    adv_entry	*rts_gateways;
    rt_parms	rts_parms;
} rt_static;

static block_t rt_static_block_index = (block_t) 0;

gw_entry *rt_gw_list = (gw_entry *) 0;			/* List of gateways for static routes */
static gw_entry *rt_static_gwp = (gw_entry *) 0;	/* fill in */

/*
 * Radix search tree node layout.
 */

typedef struct _sradix_node {
    struct _sradix_node	*rnode_left;		/* child when bit clear */
    struct _sradix_node	*rnode_right;		/* child when bit set */
    struct _sradix_node	*rnode_parent;		/* parent */
    u_short	rnode_bit;			/* bit number for node/mask */
    u_char	rnode_tbyte;			/* byte to test in address */
    u_char	rnode_tbit;			/* bit to test in byte */
    rt_static	*rnode_rts;			/* our external info */
} sradix_node;

/*
 * Static route routing table structure.  This points to the current root of 
 * the radix trie and contains some information which we need to know
 * to do installations.
 */
typedef struct _rt_static_table {
    struct _rt_static_table	*rt_table_next;	/* next table in list */
    sradix_node	*rt_table_tree;		/* current root of tree */
    struct sock_info *rt_table_sinfo;	/* stuff we need to grok sockaddr's */
    u_long	rt_table_inodes;	/* number of internal nodes in tree */
    u_long	rt_table_routes;	/* number of routes in tree */
    u_long	rt_table_oroutes;	/* number of routes before reconfig */
} rt_static_table;

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
	register sradix_node *Xrn = (rn); \
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

#define	RT_TABLE_PTR(af)	rt_static_tables[af]

#define	RT_ALLTABLES(rtt) \
    do { \
	rt_static_table *Xrt_table = rt_static_table_list; \
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

static block_t rt_static_node_block_index;
static block_t rt_static_table_block_index;

static rt_static_table *rt_static_tables[AF_MAX] = {0};
static rt_static_table *rt_static_table_list = (rt_static_table *) 0;

#define	RT_WALK(head, rts) \
    do { \
	sradix_node *Xstack[MAXDEPTH]; \
	sradix_node **Xsp = Xstack; \
	sradix_node *Xrn = (head); \
	while (Xrn) { \
	    if (((rts) = Xrn->rnode_rts)) do

#define	RT_WALK_END(head, rts) \
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
		Xrn = (sradix_node *) 0; \
		(rts) = (rt_static *) 0; \
	    } \
	} \
    } while (0)


/*
 *	Display information about the radix tree, including a visual
 *	representation of the tree itself 
 */
static void
rt_static_table_dump __PF3(tp, task *,
			   fd, FILE *,
			   rtt, rt_static_table *)
{
    struct {
	sradix_node *rn;
	int	state;
	char	right;
    } stack[MAXDEPTH], *sp;
    char prefix[MAXDEPTH];
    int i = MAXDEPTH;
    char number[4];

    while (i--) {
	prefix[i] = ' ';
    }

    sp = stack;
    (void) fprintf(fd, "\t\tRadix trie for %s (%d) inodes %u routes %u:",
		   gd_lower(trace_value(task_domain_bits,
					(int) rtt->rt_table_sinfo->si_family)),
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
	    sradix_node *rn = sp->rn;
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
		(void) fprintf(fd, "\t\t\t%.*s+-%s%u+",
			       ii, prefix,
			       (rn->rnode_bit > 9) ? "" : "-",
			       rn->rnode_bit);
		if (rn->rnode_rts) {
		    (void) fprintf(fd, "--{%A %s\n",
				   rn->rnode_rts->rts_parms.rtp_dest,
				   rn->rnode_rts->rts_next ? "(several)" : " ");
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

    (void) fprintf(fd, "\n");
}


void
rt_static_init_family __PF1(af, int)
{
    rt_static_table *rtt, *rtt_prev, *rtt_next;

    trace_only_tf(trace_global,
		  0,
		  ("rt_static_table_init_family: Initializing static route radix tree for %s",
		   trace_value(task_domain_bits, af)));

    rtt = (rt_static_table *) task_block_alloc(rt_static_table_block_index);
    rtt->rt_table_sinfo = &(sock_info[af]);

    rtt_prev = (rt_static_table *) 0;
    rtt_next = rt_static_table_list;
    while (rtt_next && rtt_next->rt_table_sinfo->si_family < af) {
	rtt_prev = rtt_next;
	rtt_next = rtt_prev->rt_table_next;
    }

    rtt->rt_table_next = rtt_next;
    if (rtt_prev) {
	rtt_prev->rt_table_next = rtt;
    } else {
	rt_static_table_list = rtt;
    }

    RT_TABLE_PTR(af) = rtt;

    trace_only_tf(trace_global,
		  0,
		  (NULL));
}


/*
 *	Initialize tries.
 */
static void
rt_static_table_init __PF0(void)
{
    rt_static_table_block_index = task_block_init(sizeof (rt_static_table), "rt_static_table");
    rt_static_node_block_index = task_block_init(sizeof (sradix_node), "sradix_node");
}


/*
 * rt_static_free - delete specified statics from tree.  The "specified"
 *		    statics may be those with the RTS_DELETE flag set (after
 *		    reconfiguraton), or those without the retain flag
 *		    (at termination).
 */
static void
rt_static_free __PF2(srtt, rt_static_table *,
		     terminating, int)
{
    register rt_static *rtsp, *rtsp_prev, *local_rts_next;
    register sradix_node *rn, *rn_prev, *rn_next;
    sradix_node *rn_do_next;
    sradix_node *stack[MAXDEPTH];
    sradix_node **sp = stack;

    rn_do_next = srtt->rt_table_tree;
    while ((rn = rn_do_next)) {
	if ((rn_do_next = rn->rnode_left)) {
	    if (rn->rnode_right) {
		*sp++ = rn->rnode_right;
	    }
	} else if (!(rn_do_next = rn->rnode_right)) {
	    if (sp != stack) {
		rn_do_next = *(--sp);
	    }
	}


 for (rtsp = rn->rnode_rts, rtsp_prev = (rt_static *) 0;
            rtsp;
            rtsp = local_rts_next) {
           local_rts_next = rtsp->rts_next;
           if (terminating
               || BIT_TEST(rtsp->rts_parms.rtp_state, RTS_DELETE)) {

               /*
                * Delete the route if we need to
                */
               if (rtsp->rts_route
                   && (!terminating
                       || !BIT_TEST(rtsp->rts_parms.rtp_state, RTS_RETAIN))) {
                   rt_delete(rtsp->rts_route);
               }
 
               /*
                * Free policy lists and addresses
                */
               if (rtsp->rts_intfs) {
                   adv_free_list(rtsp->rts_intfs);
               }
               if (rtsp->rts_gateways) {
                   adv_free_list(rtsp->rts_gateways);
               }
#ifdef        PROTO_ASPATHS
               if (rtsp->rts_parms.rtp_asp) {
                   ASPATH_FREE(rtsp->rts_parms.rtp_asp);
               }
#endif        /* PROTO_ASPATHS */
               sockfree(rtsp->rts_parms.rtp_dest);
 
               /*
                * Unlink the static route from the list corresponding to this
                * radix node.
                */
               if (rtsp == rn->rnode_rts) {
                   rn->rnode_rts = rtsp->rts_next;
               } else {
                   assert(rtsp_prev);
                   rtsp_prev->rts_next = rtsp->rts_next;
                }

               /*
                * Free the route's memory
                */
               task_block_free(rt_static_block_index, (void_t) rtsp);
               srtt->rt_table_routes--;

               /*
                * Still at least one route attached, no need to delete
                * radix node.
                */
               if (rn->rnode_rts) {
                   continue;
                }
 
                /*
                * See if we can get rid of the radix node.  If it
                * has two children it'll stay.
                */
               if (rn->rnode_left && rn->rnode_right) {
                    continue;
                }
 
                /*
                * If it has no children it's a goner, and the
                * guy above may be to.
                */
               if (!(rn->rnode_left) && !(rn->rnode_right)) {
                   rn_prev = rn->rnode_parent;
                   task_block_free(rt_static_node_block_index, (void_t) rn);
                   srtt->rt_table_inodes--;
 
                   if (!rn_prev) {
                       /*
                        * Last guy in table, remove root node pointer
                        */
                       assert(!rn_do_next);
                       srtt->rt_table_tree = (sradix_node *) 0;
                       break;
                   }

		   /* 
		    * Null the pointer in the guy above.
		    */
                   if (rn_prev->rnode_left == rn) {
                       rn_prev->rnode_left = (sradix_node *) 0;
                   }  else {
                       assert(rn_prev->rnode_right == rn);
                       rn_prev->rnode_right = (sradix_node *) 0;
                   }

                   /*
                    * If node above has a route pointer, he'll stay,
                    * so go no further.
                    */
                   if (rn_prev->rnode_rts) {
                       continue;
                   }

                   /*
                    * What is left is a node with one child and no
                    * attached route.  Point rn at it.
                    */
                   rn = rn_prev;
               }

               /*
                * One way or the other, rn points at a node with no
                * attached route and one child.  Remove rn, promoting
                * the child into rn's spot in the parent.
                */
               rn_prev = rn->rnode_parent;
               if (rn->rnode_left) {
                   rn_next = rn->rnode_left;
               } else {
                   rn_next = rn->rnode_right;
               }
               task_block_free(rt_static_node_block_index, (void_t) rn);
               srtt->rt_table_inodes--;
               rn_next->rnode_parent = rn_prev;

               if (!rn_prev) {
                   /*
                    * New root node
                    */
                   assert(srtt->rt_table_tree == rn);
                   srtt->rt_table_tree = rn_next;
               } else if (rn_prev->rnode_left == rn) {
                   rn_prev->rnode_left = rn_next;
               } else {
                   assert(rn_prev->rnode_right == rn);
                   rn_prev->rnode_right = rn_next;
               }
            } else {
               rtsp_prev = rtsp;
            }
        }
      }
}

static inline int
rt_static_compare __PF3(rtp, rt_parms *,
			rt, rt_entry *,
			ifaps, if_addr **)
{
    register int i = rtp->rtp_n_gw;

    while (i--) {
	if (ifaps[i] != rt->rt_ifaps[i]) {
	    return FALSE;
	}
    }

    return TRUE;
}

#ifdef IBM_6611
void
#else
static void
#endif  /* IBM_6611 */
rt_static_update __PF1(tp, task *)
{
    rt_static *rtsp;
    rt_static_table *srtt;
    sradix_node *shead;

    rt_open(tp);

    RT_ALLTABLES(srtt) {
      shead = srtt->rt_table_tree;		/* current root of tree */
      RT_WALK(shead, rtsp) {

         /* For each configured static route */
         int route_found = FALSE;
         while (rtsp) {
             register int j = 0;
             register rt_parms *rtp = &rtsp->rts_parms;
             register rt_entry *rt = rtsp->rts_route;
             if_addr *ifaps[RT_N_MULTIPATH];

             rtp->rtp_n_gw = 0;                /* No valid gateways yet */
             /*
              * If the lowest preference static route to this destination has
              * already been found, allow any other routes to be deleted.
              */
             if (!route_found) {
                 if (BIT_TEST(rtp->rtp_state, RTS_GATEWAY)) {
                     /* Static gateway route */
                     register adv_entry *adv;

                     /* Select the first RT_N_MULTIPATH routers on up interfaces */
                     ADV_LIST(rtsp->rts_gateways, adv) {
                         ifaps[j] = if_withroute(rtp->rtp_dest,
adv->adv_gwp->gw_addr, rtp->rtp_state);
                         if (ifaps[j]
                             && (!rtsp->rts_intfs
                                 || if_policy_match(ifaps[j], rtsp->rts_intfs))) {
                             rtp->rtp_routers[j++] = adv->adv_gwp->gw_addr;
                             if (j == RT_N_MULTIPATH) {
                                 break;
                             }
                         }
                     } ADV_LIST_END(rtsp->gateways, adv) ;

                 } else {
                     /* Static interface route */

                     register if_addr *ifap;
 
                     IF_ADDR(ifap) {
                         if (BIT_TEST(ifap->ifa_state, IFS_UP)
                             && if_policy_match(ifap, rtsp->rts_intfs)) {
#ifdef        P2P_RT_REMOTE
                            if (BIT_TEST(ifap->ifa_state, IFS_POINTOPOINT)) {
                                /* A network route to a P2P interface */
                                /* may end up pointing at the wrong */
                                /* interface in the kernel if we use */
                                /* the local address, so use the */
                                /* destination address */
                                rtp->rtp_router =
sockishost(rtp->rtp_dest, rtp->rtp_dest_mask)
                                    ? ifap->ifa_addr
                                        : ifap->ifa_addr_local;
                            } else {
                                rtp->rtp_router = ifap->ifa_addr;
                            }
#else /* P2P_RT_REMOTE */
                            rtp->rtp_router = ifap->ifa_addr_local;
#endif        /* P2P_RT_REMOTE */
                            ifaps[0] = ifap;
                            j++;
                            break;
                        }
                    } IF_ADDR_END(ifap) ;
                }
            }

            rtp->rtp_n_gw = j;

            if (rtp->rtp_n_gw) {
                int new_if = FALSE;
                int new_state = FALSE;

                /* At least one interface is up */

                if (rt && ((new_if = !rt_static_compare(rtp, rt, ifaps))
                           || (new_state =
BIT_MASK_MATCH(rtp->rtp_state, rt->rt_state, RTS_RETAIN|RTS_REJECT|RTS_BLACKHOLE))
                            || rtp->rtp_preference != rt->rt_preference
                            || rtp->rtp_metric != rt->rt_metric
#ifdef        PROTO_ASPATHS
                           || (rtp->rtp_asp
                               && rtp->rtp_asp != rt->rt_aspath)
                           || (!rtp->rtp_asp
                               && rt->rt_aspath
                               && rt->rt_aspath->path_len)
#endif        /* PROTO_ASPATHS */
                           || rtp->rtp_n_gw != rt->rt_n_gw
                           || !rt_routers_compare(rt, rtp->rtp_routers))) {
                    /* Try to change the existing route */
 
                    switch (new_if) {
                    case FALSE:
                        if (!new_state
                            && rt_change_aspath(rt,
                                                rtp->rtp_metric,
                                                rtp->rtp_metric2,
                                                rtp->rtp_tag,
                                                rtp->rtp_preference,
                                                (pref_t) 0,
                                                rtp->rtp_n_gw, rtp->rtp_routers,
                                                rtp->rtp_asp)) {
                            /* Change succeeded */

                            rt_refresh(rt);
                            break;
                        }
                        /* Fall through to delete and re-add */

                    case TRUE:
                        /* Delete the old route */
                        rt_delete(rt);
                        rt = (rt_entry *) 0;
                        break;
                    }
                }

                if (!rt) {
                    /* No old route or change failed, add a new one */

                    rt = rt_add(rtp);
                }
                route_found = TRUE;
            } else {
                /* No interfaces up or lower preference static route
to destination already found */
 
                if (rt) {
                    /* Delete the route */

                    rt_delete(rt);
                    rt = (rt_entry *) 0;
                }
            }
            rtsp->rts_route = rt;
            rtsp = rtsp->rts_next;
        }
      } RT_WALK_END(shead, rtsp);
 } RT_ALLTABLES_END(srtt);
 
 rt_close(tp, (gw_entry *) 0, 0, NULL);

}


static inline int
rt_parse_route_change __PF4(rtp, rt_parms *,
			    intfs, adv_entry *,
			    gateways, adv_entry *,
			    err_msg, char *)
{
    register rt_static *rtsp;
    register sradix_node *rn, *rn_prev, *rn_add, *rn_new;
    register u_short bitlen, bits2chk, dbit;
    register byte *addr, *his_addr;
    register u_int i;
    struct sock_info *si;
    rt_static_table *rtt;

    rtt = RT_TABLE_PTR(socktype(rtp->rtp_dest));
    assert(rtt);

#define	COPY_INFO(rtsp, rtp) \
    do { \
	(rtsp)->rts_parms = *(rtp); /* struct copy */\
	(rtsp)->rts_parms.rtp_dest = sockdup((rtp)->rtp_dest); \
	(rtsp)->rts_intfs = intfs; \
	(rtsp)->rts_gateways = gateways; \
    } while (0)

    /*
     * Compute the bit length of the mask.
     */
    si = rtt->rt_table_sinfo;
    bitlen = mask_to_prefix_si(si, rtp->rtp_dest_mask);
    assert(bitlen != (u_short) -1);

    rn_prev = rtt->rt_table_tree;

    /*
     * If there is no existing root node, this is it.  Catch this
     * case now.
     */
    if (!rn_prev) {
	rtsp = (rt_static *) task_block_alloc(rt_static_block_index);
	rtt->rt_table_routes++;
	COPY_INFO(rtsp, rtp);
	rn = (sradix_node *) task_block_alloc(rt_static_node_block_index);
	rtt->rt_table_tree = rn;
	RN_SETBIT(rn, bitlen, (rtsp->rts_parms.rtp_dest_mask == si->si_mask_max));
	rn->rnode_rts = rtsp;
	rtt->rt_table_inodes++;
	return FALSE;
    }

    /*
     * Search down the tree as far as we can, stopping at a node
     * with a bit number >= ours which has an rts attached.  It
     * is possible we won't get down the tree this far, however,
     * so deal with that as well.
     */
    addr = RN_ADDR(rtp->rtp_dest, si->si_offset);
    rn = rn_prev;
    while (rn->rnode_bit < bitlen || !(rn->rnode_rts)) {
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
    his_addr = RN_ADDR(rn->rnode_rts->rts_parms.rtp_dest, si->si_offset);
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
     * may just be able to attach the rts to him.  Check this since it
     * is easy.
     */
    if (dbit == bitlen && rn->rnode_bit == bitlen) {
      rt_static *rtsp_prev = (rt_static *) 0;

      /*
       * When multiple static routes to the same destination/mask are
       * defined, they
       * are sorted in order of ascending preference. Among the statics routes
       * to a given destination/mask with available next hops, the
       * one with the lowest
       * preference is installed.
       */
      rtsp = rn->rnode_rts;
      while (rtsp &&
             rtp->rtp_preference > rtsp->rts_parms.rtp_preference) {
          rtsp_prev = rtsp;
          rtsp = rtsp->rts_next;
      }

      if (!rtsp
          || rtp->rtp_preference != rtsp->rts_parms.rtp_preference) {
          rt_static *rtsp_local;
          rtsp_local = (rt_static *) task_block_alloc(rt_static_block_index);

          /*
           * Link the static route structure into the list for the
           * destination/mask in order of ascending preference.
           */
          if (!rtsp) {
              if (!rn->rnode_rts) {
                  rn->rnode_rts = rtsp_local;
              } else {
                  rtsp_prev->rts_next = rtsp_local;
              }
          } else {
              if (!rtsp_prev) {
                  rn->rnode_rts = rtsp_local;
                  rtsp_local->rts_next = rtsp;
              } else {
                  rtsp_local->rts_next = rtsp_prev->rts_next;
                  rtsp_prev->rts_next = rtsp_local;
              }
          }
          rtt->rt_table_routes++;
          COPY_INFO(rtsp_local, rtp);
          return FALSE;
      }

      if (!BIT_TEST(rtsp->rts_parms.rtp_state, RTS_DELETE)) {
          if (err_msg) {
              (void) sprintf(err_msg, "duplicate preference %d static route to %A",
                             rtp->rtp_preference,
                             rtp->rtp_dest);
          }
          if (intfs) {
              adv_free_list(intfs);
	    }
	    if (gateways) {
		adv_free_list(gateways);
	    }
	    return TRUE;
	}

	/* Refresh or change old one */

	/* Free allocated data */
	sockfree(rtsp->rts_parms.rtp_dest);

#ifdef	PROTO_ASPATHS
	if (rtsp->rts_parms.rtp_asp) {
	    ASPATH_FREE(rtsp->rts_parms.rtp_asp);
	    rtsp->rts_parms.rtp_asp = (as_path *) 0;
	}
#endif	/* PROTO_ASPATHS */

	COPY_INFO(rtsp, rtp);
	return FALSE;
    }

    /*
     * Allocate us a new node; we are sure to need it now.
     */
    rtsp = (rt_static *) task_block_alloc(rt_static_block_index);
    rtt->rt_table_routes++;
    COPY_INFO(rtsp, rtp);
    rn_add = (sradix_node *) task_block_alloc(rt_static_node_block_index);
    rtt->rt_table_inodes++;
    RN_SETBIT(rn_add, bitlen, (rtsp->rts_parms.rtp_dest_mask == si->si_mask_max));
    rn_add->rnode_rts = rtsp;

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
	return FALSE;
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
	rn_new = (sradix_node *) task_block_alloc(rt_static_node_block_index);
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

    return FALSE;
}


int
#ifdef	PROTO_ASPATHS
rt_parse_route_aspath __PF8(dest, sockaddr_un *,
			    mask, sockaddr_un *,
			    gateways, adv_entry *,
			    intfs, adv_entry *,
			    pref, pref_t,
			    state, flag_t,
			    asp, as_path *,
			    err_msg, char *)
#else	/* PROTO_ASPATHS */
rt_parse_route __PF7(dest, sockaddr_un *,
		     mask, sockaddr_un *,
		     gateways, adv_entry *,
		     intfs, adv_entry *,
		     pref, pref_t,
		     state, flag_t,
		     err_msg, char *)
#endif	/* PROTO_ASPATHS */
{
    adv_entry *adv;
    rt_parms rtparms;

    bzero((caddr_t) &rtparms, sizeof (rtparms));

    rtparms.rtp_dest = dest;
    rtparms.rtp_dest_mask = mask;
    /* Set the task on all the gateways */
    ADV_LIST(gateways, adv) {
	adv->adv_gwp->gw_task = rt_task;
    } ADV_LIST_END(gateways, adv) ;
    rtparms.rtp_gwp = rt_static_gwp;
    rtparms.rtp_state = state | RTS_INTERIOR;
    rtparms.rtp_preference = pref;
#ifdef	PROTO_ASPATHS
    rtparms.rtp_asp = asp;
#endif	/* PROTO_ASPATHS */
			    
    return rt_parse_route_change(&rtparms, intfs, gateways, err_msg);
}

#ifdef        IBM_6611
int
rt_static_delete __PF4(dest, sockaddr_un *,
                     mask, sockaddr_un *,
                     preference, pref_t,
                     tp, task *)
{
    register sradix_node *rn;
    register byte *ap, *ap2;
    register u_short bitlen;
    register struct sock_info *si;
    register rt_static_table *rtt;
    register rt_static *rtsp;
 
    /*
     * If there is no table, or nothing to do, assume nothing found.
     */
    if (!(rtt = RT_TABLE_PTR(socktype(dest))) || !(rn = rtt->rt_table_tree)) {
      return FALSE;
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

    ap = RN_ADDR(dest, si->si_offset);
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
     * If there is no static route on this node, we're gone too.
     */
    if (!rn || rn->rnode_bit != bitlen || !(rtsp = rn->rnode_rts)) {
      return FALSE;
    }

    /*
     * So far so good.  Fetch the address and see if we have an
     * exact match.
     */
    ap2 = RN_ADDR(rtsp->rts_parms.rtp_dest, si->si_offset);
    bitlen = RN_BYTELEN(bitlen);
    while (bitlen--) {
      if (*ap++ != *ap2++) {
          return FALSE;
      }
    }
    /*
     * Find the route corresponding to the specified preference. If
     * no preference is specified, delete the first or only route.
     */
    if (preference != (pref_t) UNDEFINED_PREF) {
      while (rtsp) {
          if (preference == rtsp->rts_parms.rtp_preference) {
              break;
          }
          rtsp = rtsp->rts_next;
      }
      if (!rtsp) {
          return FALSE;
      }
    }

    /*
     * Mark the route for deletion and invoke
     * the static route update routine.
     */
    BIT_SET(rtsp->rts_parms.rtp_state, RTS_DELETE);
    rt_open(tp);
    rt_static_free(rtt, FALSE);
    rt_close(tp, (gw_entry *) 0, 0, NULL);
    return TRUE;
}
#endif        /* IBM_6611 */

void
rt_static_cleanup __PF1(tp, task *)
{
    register rt_static *rtsp;
    rt_static_table *srtt;
    sradix_node *shead;
    u_long oroutes;

    RT_ALLTABLES(srtt) {
	shead = srtt->rt_table_tree;		/* current root of tree */
	oroutes = 0;
	RT_WALK(shead, rtsp) {
        while (rtsp) {
            oroutes++;
            BIT_SET(rtsp->rts_parms.rtp_state, RTS_DELETE);
            if (rtsp->rts_intfs) {
                adv_free_list(rtsp->rts_intfs);
                rtsp->rts_intfs = (adv_entry *) 0;
            }
            if (rtsp->rts_gateways) {
                adv_free_list(rtsp->rts_gateways);
                rtsp->rts_gateways = (adv_entry *) 0;
            }
#ifdef        PROTO_ASPATHS
            if (rtsp->rts_parms.rtp_asp) {
                ASPATH_FREE(rtsp->rts_parms.rtp_asp);
                rtsp->rts_parms.rtp_asp = (as_path *) 0;
            }
#endif        /* PROTO_ASPATHS */
            rtsp = rtsp->rts_next;
        }
	} RT_WALK_END(shead, rtsp);
	assert(srtt->rt_table_routes == oroutes);
	srtt->rt_table_oroutes = oroutes;
    } RT_ALLTABLES_END(srtt);
}


void
rt_static_reinit __PF1(tp, task *)
{
    rt_static_table *srtt;
    int rt_is_open = 0;

    RT_ALLTABLES(srtt) {
	if (srtt->rt_table_oroutes) {
	    if (!rt_is_open) {
		rt_open(tp);
		rt_is_open = 1;
	    }
	    rt_static_free(srtt, FALSE);
	    srtt->rt_table_oroutes = 0;
	}
    } RT_ALLTABLES_END(srtt);

    if (rt_is_open) {
	rt_close(tp, (gw_entry *) 0, 0, NULL);
    }
    /* Let ifachange routine do update */
}


void
rt_static_ifachange __PF1(tp, task *)
{
    rt_static_update(tp);
}

void
rt_static_terminate __PF1(tp, task *)
{
    rt_static_table *srtt;
    int rt_is_open = 0;

    RT_ALLTABLES(srtt) {
	if (srtt->rt_table_tree) {
	    if (!rt_is_open) {
		rt_open(tp);
		rt_is_open = 1;
	    }
	    rt_static_free(srtt, TRUE);
	}
    } RT_ALLTABLES_END(srtt);

    if (rt_is_open) {
	rt_close(tp, (gw_entry *) 0, 0, NULL);
    }
}


void
rt_static_dump __PF2(tp, task *,
		     fp, FILE *)
{
    register rt_static *rtsp;
    rt_static_table *srtt;
    sradix_node *shead;

    RT_ALLTABLES(srtt) {

	fprintf(fp,
		"\tStatic routes for family %s: (* indicates gateway(s) in use)\n",
		trace_value(task_domain_bits, (int) srtt->rt_table_sinfo->si_family));

	rt_static_table_dump(tp, fp, srtt);
	
	shead = srtt->rt_table_tree;		/* current root of tree */
	RT_WALK(shead, rtsp) {

          while (rtsp) {
              register rt_parms *rtp = &rtsp->rts_parms;
              adv_entry *adv;

              fprintf(fp, "\t\t%-15A mask %-15A  preference %d\tstate <%s>\n",
                      rtp->rtp_dest,
                      rtp->rtp_dest_mask,
                      rtp->rtp_preference,
                      trace_bits(rt_state_bits, rtp->rtp_state));

              /* Print the Gateways */
              if (rtsp->rts_gateways) {
                  fprintf(fp, "\t\t\tGateway%s\t",
                          rtp->rtp_n_gw > 1 ? "s" : "");
 
                  ADV_LIST(rtsp->rts_gateways, adv) {
                      rt_entry *rt = rtsp->rts_route;
                      const char *active = "";
 
                      if (rt) {
                          int j = rt->rt_n_gw;

                          while (j--) {
                              if (sockaddrcmp(rt->rt_routers[j],
adv->adv_gwp->gw_addr)) {
                                  active = "*";
                                  break;
                              }
                          }
                      }

                      fprintf(fp, "%A(%s) ",
                              adv->adv_gwp->gw_addr,
                              active);
                  } ADV_LIST_END(rtsp->rts_gateway, adv) ;
                  fprintf(fp, "\n");
              }

              /* Print the interfaces */
              if (rtsp->rts_intfs
                  && (rtsp->rts_intfs->adv_flag & ADVF_TYPE) != ADVFT_ANY) {
                  /* Print the interface restrictions */

                  fprintf(fp, "\t\t\tInterfaces ");

                  ADV_LIST(rtsp->rts_intfs, adv) {
                      switch (adv->adv_flag & ADVF_TYPE) {
                      case ADVFT_IFN:
                          fprintf(fp, "%A ",
                                  adv->adv_ifn->ifae_addr);
                          break;

                      case ADVFT_IFAE:
                          fprintf(fp, "%A ",
                                  adv->adv_ifae->ifae_addr);
                          break;
                      }
                  } ADV_LIST_END(rtsp->rts_intfs, adv) ;

                  fprintf(fp, "\n");
              }

              fprintf(fp, "\n");
 
              rtsp = rtsp->rts_next;
          }
 
      } RT_WALK_END(shead, rtsp);
    } RT_ALLTABLES_END(srtt);

    fprintf(fp, "\n");
}


void
rt_static_init __PF1(tp, task *)
{
    rt_static_gwp = gw_init((gw_entry *) 0,
			    RTPROTO_STATIC,
			    rt_task,
			    (as_t) 0,
			    (as_t) 0,
			    (sockaddr_un *) 0,
			    GWF_NOHOLD);

    rt_static_block_index = task_block_init(sizeof (rt_static), "rt_static");
    rt_static_table_init();
}
