#ident	"@(#)policy.c	1.4"
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
#include "include.h"
#include "parse.h"

unsigned int adv_n_allocated = 0;	/* Number of adv's allocated */
static const char *policy_tabs = "\t\t\t\t\t\t\t\t\t\t";

bits const gw_bits[] =
{
    {GWF_SOURCE,	"Source"},
    {GWF_TRUSTED,	"Trusted"},
    {GWF_ACCEPT,	"Accept"},
    {GWF_REJECT,	"Reject"},
    {GWF_QUERY,		"Query"},
    {GWF_IMPORT,	"ImportRestrict"},
    {GWF_FORMAT,	"Format"},
    {GWF_CHECKSUM,	"CheckSum"},
    {GWF_AUXPROTO,	"AuxProto"},
    {GWF_AUTHFAIL,	"AuthFail"},
    {GWF_NEEDHOLD,	"NeedHolddown"},
    {GWF_NOHOLD,	"NoHolddown"},
    {0, NULL}
};


static const bits dm_bits[] =
{
    {DMF_REFINE,	"Refine"},
    {DMF_EXACT,		"Exact"},
    {DMF_NONCONTIG,	"NonContig"},
    {0, NULL}
};

static const bits adv_flag_bits[] = {
    {ADVF_FIRST,	"first" },
    {ADVF_NO,		"no" },
    {ADVFOT_NONE,	"none" },
    {ADVFOT_METRIC,	"metric_t" },
    {ADVFOT_PREFERENCE,	"pref_t" },
    {ADVFOT_FLAG,	"flag_t" },
    {0,			NULL }
};

static const bits adv_type_bits[] = {
    {ADVFT_ANY,		"Any" },
    {ADVFT_GW,		"gw_entry" },
    {ADVFT_IFN,		"if_name_entry" },
    {ADVFT_IFAE,	"if_addr_entry" },
    {ADVFT_AS,		"as" },
    {ADVFT_DM,		"dest_mask" },
    {ADVFT_ASPATH,	"as_path" },
    {ADVFT_TAG,		"tag" },
    {ADVFT_PS,		"protocol_specific" },
    {0,			NULL }
};

adv_psfunc *control_psfunc[RTPROTO_MAX] = { 0 }; /* Array of protocol specific functions for dealing with adv data */
static block_t adv_block_index = (block_t) 0;
static block_t config_entry_index = (block_t) 0;
static block_t config_list_index = (block_t) 0;
static block_t dest_mask_internal_index = (block_t) 0;

/**/

static byte maskbits[NBBY] = {
    0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
};

#define	dmi_test_bit(ap, dmi)	(((ap)[(dmi)->dmi_offset]) & (dmi)->dmi_mask)

/* Return the bit position of the msb */
const byte first_bit_set[256] = {
    /* 0 - 15 */
    8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
    /* 16 - 31 */
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    /* 32 - 63 */
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    /* 64 - 127 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    /* 128 - 255 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};


#define	DMI_ADDR(addr, a_offset)		(((byte *)(addr)) + a_offset)

#define	DMI_CREATE(node, mlen, adv) \
	do { \
	     (node) = (dest_mask_internal *) task_block_alloc(dest_mask_internal_index); \
	     (node)->dmi_bit = (mlen); \
	     (node)->dmi_offset = (mlen) >> 3; \
	     (node)->dmi_mask = maskbits[mlen & (NBBY - 1)]; \
	     (node)->dmi_external = (adv); \
	 } while (0)

#define	DMI_NSHIFT	3
#define	DMI_NBIT(x)	(0x80 >> ((x) & (NBBY-1)))
#define	DMI_NBYTE(x)	((x) >> DMI_NSHIFT)
#define	MAXKEYBITS	(SOCK_MAXADDRLEN * NBBY)
#define	MAXDEPTH	(MAXKEYBITS+1)

/* Add to the destination mask internal tree */
adv_entry *
adv_destmask_insert __PF3(errmsg, char *,
			  list, adv_entry *,
			  new, adv_entry *)
{
    dest_mask_internal *tree = list ? list->adv_dm.dm_internal : (dest_mask_internal *) 0;
    register u_int i;
    dest_mask *dm = &new->adv_dm;
    sockaddr_un *dest = dm->dm_dest;
    sockaddr_un *mask = dm->dm_mask;
    register u_short bitlen = mask_to_prefix(mask);

    /*
     * Compute the bit length of the mask.
     */
    if (bitlen == (u_short) -1) {
	bitlen = mask_contig_bits(mask);
	BIT_SET(dm->dm_flags, DMF_NONCONTIG);
    }

    /*
     * If there is no existing root node, this is it.  Catch this
     * case now.
     */
    if (!tree) {
	DMI_CREATE(tree, bitlen, new);
    } else {
	dest_mask_internal *stack[MAXDEPTH];
	dest_mask_internal **sp = stack + 1;
	register dest_mask_internal *dmi, *dmi_add;
	register dest_mask_internal *dmi_prev;
	register dest_mask_internal *dmi_new;
	register u_short bits2chk, dbit;
	register byte *addr, *his_addr;
	u_int offset = sock_info[socktype(dest)].si_offset;

	/*
	 * Search down the tree as far as we can, stopping at a node
	 * with a bit number >= ours which has an rth attached.  It
	 * is possible we won't get down the tree this far, however,
	 * so deal with that as well.
	 */
	addr = DMI_ADDR(dest, offset);
	dmi = tree;
	while (dmi->dmi_bit < bitlen || !dmi->dmi_external) {
	    *sp++ = dmi;
	    if (BIT_TEST(addr[dmi->dmi_offset], dmi->dmi_mask)) {
		if (!dmi->dmi_right) {
		    break;
		}
		dmi = dmi->dmi_right;
	    } else {
		if (!dmi->dmi_left) {
		    break;
		}
		dmi = dmi->dmi_left;
	    }
	}

	/*
	 * Now we need to find the number of the first bit in our address
	 * which differs from his address.
	 */
	bits2chk = MIN(dmi->dmi_bit, bitlen);
	his_addr = DMI_ADDR(dmi->dmi_external->adv_dm.dm_dest, offset);
	for (dbit = 0; dbit < bits2chk; dbit += NBBY) {
	    i = DMI_NBYTE(dbit);
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
	while ((--sp) != stack && (*sp)->dmi_bit >= dbit) {
	    dmi = *sp;
	}
	dmi_prev = (sp == stack) ? (dest_mask_internal *) 0 : *sp;

	/*
	 * Okay.  If the node dmi points at is equal to our bit number, we
	 * may just be able to attach the rth to him.  Check this since it
	 * is easy.
	 */
	if (dbit == bitlen && dmi->dmi_bit == bitlen) {

	    /* If this node has no external information, we just attach */
	    /* this entry to him.  If there is external information we must */
	    /* find our place in the defined order.  Non-contigous masks come */
	    /* first (longest value first), then contiguous masks.  For the */
	    /* same mask they are ordered by flags */

	    if (!dmi->dmi_external) {
		/* Attach here */

		dmi->dmi_external = new;
	    } else {
		register adv_entry *adv = dmi->dmi_external;
		register adv_entry *last = (adv_entry *) 0;
		u_int mask_offset = offset + DMI_NBYTE(bitlen);
		u_int my_order = BIT_TEST(dm->dm_flags, DMF_ORDERMASK);

#define	DMI_LOOP(adv, last)	for (; (adv); (last) = (adv), (adv) = (adv)->adv_next)
#define	DMI_WHILE(adv, last, test)	DMI_LOOP(adv, last) { if (!(test)) break; }

		if (!BIT_TEST(dm->dm_flags, DMF_NONCONTIG)) {
		    /* Non-contigous masks are longer than contigous masks. */

		    DMI_WHILE(adv, last, socksize(adv->adv_dm.dm_mask) > socksize(mask));
		}

		/* If we are dealing with non-contigous masks we'll have to */
		/* compare them for numeric value.  We at least know that they */
		/* are identical up to the number of this node, so start after that. */
		if (adv && adv->adv_dm.dm_mask != mask) {
		    DMI_LOOP(adv, last) {
			byte *lp;
			register int diff = 0;

			addr = DMI_ADDR(mask, mask_offset);
			lp = DMI_ADDR(mask, socksize(mask));
			his_addr = DMI_ADDR(adv->adv_dm.dm_mask, mask_offset);
			while (addr < lp && !(diff = *addr++ - *his_addr++)) ;

			if (diff > 0) {
			    break;
			}
		    }
		}

		/* If these are the same mask, order the choices by flags */
		if (adv && adv->adv_dm.dm_mask == mask) {
		    u_int his_order = BIT_TEST(adv->adv_dm.dm_flags, DMF_ORDERMASK);

		    /* But first check for a duplicate */
		    if (my_order == his_order) {
			    if (errmsg) {
				sprintf(errmsg, "duplicate entry at %A mask %A %s",
					dest,
					mask,
					gd_lower(trace_bits(dm_bits, dm->dm_flags & (DMF_EXACT|DMF_REFINE))));
			    }
			    return (adv_entry *) 0;
		    }

		    DMI_WHILE(adv, last, my_order < his_order);
		}

		/* Insert here */
		new->adv_next = adv;
		if (last) {
		    last->adv_next = new;
		} else {
		    dmi->dmi_external = new;
		}
	    }
	} else {
	    /*
	     * Allocate us a new node, we are sure to need it now.
	     */
	    DMI_CREATE(dmi_add, bitlen, new);

	    /*
	     * There are a couple of possibilities.  The first is that we
	     * attach directly to the thing pointed to by dmi.  This will be
	     * the case if his bit is equal to dbit.
	     */
	    if (dmi->dmi_bit == dbit) {
		assert(dbit < bitlen);
		if (BIT_TEST(addr[dmi->dmi_offset], dmi->dmi_mask)) {
		    assert(!dmi->dmi_right);
		    dmi->dmi_right = dmi_add;
		} else {
		    assert(!dmi->dmi_left);
		    dmi->dmi_left = dmi_add;
		}
	    } else {
		/*
		 * The other case where we don't need to add a split is where
		 * we were on the same branch as the guy we found.  In this case
		 * we insert dmi_add into the tree between dmi_prev and dmi.  Otherwise
		 * we add a split between dmi_prev and dmi and append the node we're
		 * adding to one side of it.
		 */
		if (dbit == bitlen) {
		    if (BIT_TEST(his_addr[dmi_add->dmi_offset], dmi_add->dmi_mask)) {
			dmi_add->dmi_right = dmi;
		    } else {
			dmi_add->dmi_left = dmi;
		    }
		    dmi_new = dmi_add;
		} else {
		    DMI_CREATE(dmi_new, dbit, (adv_entry *) 0);
		    if (BIT_TEST(addr[dmi_new->dmi_offset], dmi_new->dmi_mask)) {
			dmi_new->dmi_right = dmi_add;
			dmi_new->dmi_left = dmi;
		    } else {
			dmi_new->dmi_left = dmi_add;
			dmi_new->dmi_right = dmi;
		    }
		}

		/*
		 * If dmi_prev is NULL this is a new root node, otherwise it
		 * is attached to the guy above in the place where dmi was.
		 */
		if (!dmi_prev) {
		    tree = dmi_new;
		} else if (dmi_prev->dmi_right == dmi) {
		    dmi_prev->dmi_right = dmi_new;
		} else {
		    assert(dmi_prev->dmi_left == dmi);
		    dmi_prev->dmi_left = dmi_new;
		}
	    }
	}
    }

    if (!list) {
	/* This is the first entry on the list */
	
	list = new;
    }

    /* Save tree pointer */
    list->adv_dm.dm_internal = tree;
    
    return list;
}


/* find the adv_entry matching a specific net */
adv_entry *
adv_destmask_match __PF3(list, adv_entry *,
			 target, sockaddr_un *,
			 mask, sockaddr_un *)
{
    register dest_mask_internal *tree = list ? list->adv_dm.dm_internal : (dest_mask_internal *) 0;
    dest_mask_internal *stack[SOCK_MAXADDRLEN*NBBY+1];
    register dest_mask_internal **sp;
    register byte *ap, *his_ap;
    register u_short mlen, tlen;
    size_t offset = SI_OFFSET(socktype(target));
    adv_entry *adv;
    dest_mask *dm;

    if (!tree) {
	goto not_matched;
    }

    /* Calculate mask length */
    mlen = mask_to_prefix(mask);
    assert(mlen != (u_short) -1);

    sp = stack;
    ap = DMI_ADDR(target, offset);
    do {
	if (mlen <= tree->dmi_bit) {
	    /* Reached our maximum length */
	    if (mlen == tree->dmi_bit && tree->dmi_external) {
		*sp++ = tree;
	    }
	    break;
	}
	if (tree->dmi_external) {
	    *sp++ = tree;
	}
	tree = dmi_test_bit(ap, tree) ? tree->dmi_right : tree->dmi_left;
    } while (tree);

    if (sp == stack) {
	/* Found nothing */
	goto not_matched;
    }

    /*
     * Find the first bit which differs from the last guy we found.
     * This'll allow us to toss anyone who can't possibly match us,
     * and will avoid having to do address and mask matches on
     * contiguous mask entries.
     */
    tree = *(--sp);
    adv = tree->dmi_external;
    his_ap = DMI_ADDR(adv->adv_dm.dm_dest, offset);
    for (tlen = 0; tlen < mlen; tlen += NBBY) {
	register u_int i = DMI_NBYTE(tlen);
	if (ap[i] != his_ap[i]) {
	    tlen += first_bit_set[ap[i] ^ his_ap[i]];
	    break;
	}
    }

    /*
     * Toss away anyone whose bit number is larger than the
     * first difference bit.
     */
    while (tlen < tree->dmi_bit) {
	if (sp == stack) {
	    goto not_matched;
	}
	tree = *(--sp);
    }

#ifdef TR_ADV_VERBOSE
    trace_tf(trace_global, TR_ADV_VERBOSE, 0,
	     ("adv_destmask_match: looking for %A %A (%u): %u entries",
	      target,
	      mask,
	      mlen,
	      ((sp - stack) + 1)));
#endif
    for (;;) {
	tlen = tree->dmi_bit;
#ifdef TR_ADV_VERBOSE
	trace_tf(trace_global, TR_ADV_VERBOSE, 0,
		 ("adv_destmask_match:	examine node %u, %u remain",
		  tree->dmi_bit,
		  (sp - stack)));
#endif

	adv = tree->dmi_external;
	if (mlen == tlen) {
	    /*
	     * We can't match either a DMF_NONCONTIG or a DMF_REFINE.
	     * Anything else is a match.
	     */
	    do {
		dm = &adv->adv_dm;
		if (!BIT_TEST(dm->dm_flags, DMF_NONCONTIG|DMF_REFINE)) {
		    goto matched;
		}
	    } while ((adv = adv->adv_next)) ;
	} else {
	    /*
	     * We can't match a DMF_EXACT.  We can match DMF_NONCONTIG,
	     * though this requires checking.  We match anything
	     * else we find.
	     */
	    do {
		dm = &adv->adv_dm;
		if (BIT_TEST(dm->dm_flags, DMF_NONCONTIG)) {
		    if (socksize(dm->dm_mask) <= socksize(mask)) {
			register byte *his_mp = DMI_ADDR(dm->dm_mask, offset);
			register byte *my_mp = DMI_ADDR(mask, offset);
			register u_int i = socksize(dm->dm_mask) - offset;
			his_ap = DMI_ADDR(dm->dm_dest, offset);

			while (i-- > 0) {
			    if ((his_mp[i] & my_mp[i]) != his_mp[i]) {
				break;
			    }
			    if ((ap[i] & his_mp[i]) != his_ap[i]) {
				break;
			    }
			    if (i == 0) {
				goto matched;
			    }
			}
		    }
		} else if (!BIT_TEST(dm->dm_flags, DMF_EXACT)) {
		    goto matched;
		}
	    } while ((adv = adv->adv_next)) ;
	}
	if (sp == stack) {
	    break;
	}
	tree = *(--sp);
    }

not_matched:
#ifdef	TR_ADV
    trace_tf(trace_global, TR_ADV, 0,
	    ("adv_destmask_match: no match for %A/%A found",
	     target,
	     mask));
#endif	/* TR_ADV */
    return (adv_entry *) 0;

matched:
#ifdef	TR_ADV
    trace_tf(trace_global, TR_ADV, 0,
	    ("adv_destmask_match: found %A %A matches %A %A %s",
	     target,
	     mask,
	     dm->dm_dest,
	     dm->dm_mask,
	     trace_bits(dm_bits, dm->dm_flags)));
#endif	/* TR_ADV */
    return adv;
}

/* find the aggregation adv_entry matching a route */
adv_entry *
adv_aggregate_match __PF3(list, adv_entry *,
			  rt, rt_entry *,
			  preference, pref_t *)
{
    register dest_mask_internal *tree = list ? list->adv_dm.dm_internal : (dest_mask_internal *) 0;
    dest_mask_internal *stack[SOCK_MAXADDRLEN*NBBY+1];
    register dest_mask_internal **sp;
    register byte *ap, *his_ap;
    register u_short mlen, tlen;
    size_t offset = SI_OFFSET(socktype(rt->rt_dest));
    sockaddr_un *target, *mask;
    adv_entry *proto;
    adv_entry *adv;
    dest_mask *dm;
    flag_t cant_match;
    int is_match;

    target = rt->rt_dest;
    mask = rt->rt_dest_mask;
    if (!tree) {
	goto not_matched;
    }

    /* Calculate mask length */
    mlen = mask_to_prefix(mask);
    assert(mlen != (u_short) -1);

    sp = stack;
    ap = DMI_ADDR(target, offset);
    do {
	if (mlen <= tree->dmi_bit) {
	    /* Reached our maximum length */
	    if (mlen == tree->dmi_bit && tree->dmi_external) {
		*sp++ = tree;
	    }
	    break;
	}
	if (tree->dmi_external) {
	    *sp++ = tree;
	}
	tree = dmi_test_bit(ap, tree) ? tree->dmi_right : tree->dmi_left;
    } while (tree);

    if (sp == stack) {
	/* Found nothing */
	goto not_matched;
    }

    /*
     * Find the first bit which differs from the last guy we found.
     * This'll allow us to toss anyone who can't possibly match us,
     * and will avoid having to do address and mask matches on
     * contiguous mask entries.
     */
    tree = *(--sp);
    adv = tree->dmi_external;
    his_ap = DMI_ADDR(adv->adv_dm.dm_dest, offset);
    for (tlen = 0; tlen < mlen; tlen += NBBY) {
	register u_int i = DMI_NBYTE(tlen);
	if (ap[i] != his_ap[i]) {
	    tlen += first_bit_set[ap[i] ^ his_ap[i]];
	    break;
	}
    }

    /*
     * Toss away anyone whose bit number is larger than the
     * first difference bit.
     */
    while (tlen < tree->dmi_bit) {
	if (sp == stack) {
	    goto not_matched;
	}
	tree = *(--sp);
    }

#ifdef TR_ADV_VERBOSE
    trace_tf(trace_global, TR_ADV_VERBOSE, 0,
	     ("adv_aggregate_match: looking for %A %A (%u): %u entries",
	      target,
	      mask,
	      mlen,
	      ((sp - stack) + 1)));
#endif
    cant_match = (DMF_NONCONTIG|DMF_REFINE);
    for (;;) {
	tlen = tree->dmi_bit;
#ifdef TR_ADV_VERBOSE
	trace_tf(trace_global, TR_ADV_VERBOSE, 0,
		 ("adv_aggregate_match:	examine node %u, %u remain",
		  tree->dmi_bit,
		  (sp - stack)));
#endif

	adv = tree->dmi_external;
	if (mlen != tlen) {
	    cant_match = DMF_EXACT;
	}
	do {
	    dm = &adv->adv_dm;
	    if (BIT_TEST(dm->dm_flags, cant_match)) {
		continue;
	    }
	    if (BIT_TEST(dm->dm_flags, DMF_NONCONTIG)) {
		if (socksize(dm->dm_mask) <= socksize(mask)) {
		    register byte *his_mp = DMI_ADDR(dm->dm_mask, offset);
		    register byte *my_mp = DMI_ADDR(mask, offset);
		    register u_int i = socksize(dm->dm_mask) - offset;
		    his_ap = DMI_ADDR(dm->dm_dest, offset);

		    is_match = 0;
		    while (i-- > 0) {
			if ((his_mp[i] & my_mp[i]) != his_mp[i]) {
			    break;
			}
			if ((ap[i] & his_mp[i]) != his_ap[i]) {
			    break;
			}
			if (i == 0) {
			    is_match = 1;
			    break;
			}
		    }
		    if (!is_match) {
			continue;
		    }
		} else {
		    continue;
		}
	    }

	    ADV_LIST(adv->adv_list, proto) {
		/* Check for a protocol match */

		if (proto->adv_proto == RTPROTO_ANY
		    || proto->adv_proto == rt->rt_gwp->gw_proto) {
		    adv_entry *padv = (adv_entry *) 0;

		    /* Right protocol, check further */

		    switch (proto->adv_flag & ADVF_TYPE) {
		    case ADVFT_ANY:
			break;

		    case ADVFT_AS:
			if (rt->rt_gwp->gw_peer_as != proto->adv_as) {
			    continue;
			}
			break;

#ifdef	PROTO_ASPATHS
		    case ADVFT_TAG:
			if (tag_rt(rt) != proto->adv_tag) {
			    continue;
			}
			break;

		    case ADVFT_ASPATH:
			if (!aspath_adv_match(proto->adv_aspath, rt->rt_aspath)) {
			    continue;
			}
			break;
#endif	/* PROTO_ASPATHS */

		    default:
			assert(FALSE);
			break;
		    }

		    if (!proto->adv_list
			|| (padv = adv_destmask_match(proto->adv_list,
						      target,
						      mask))) {
			/* Have a match, get preference */
			if ((padv
			    && BIT_TEST(padv->adv_flag, ADVF_NO))
			    || BIT_TEST(proto->adv_flag, ADVF_NO)) {
			    break;
			}
			if (padv
			    && BIT_TEST(padv->adv_flag, ADVFOT_PREFERENCE)) {
			    *preference = padv->adv_result.res_preference;
			} else if (BIT_TEST(proto->adv_flag, ADVFOT_PREFERENCE)) {
			    *preference = proto->adv_result.res_preference;
			} else if (BIT_TEST(adv->adv_flag, ADVFOT_PREFERENCE)) {
			    *preference = adv->adv_result.res_preference;
			}
			goto matched;
		    }
		}
	    } ADV_LIST_END(adv->adv_list, proto) ;
	} while ((adv = adv->adv_next)) ;

	if (sp == stack) {
	    break;
	}
	tree = *(--sp);
    }

not_matched:
#ifdef	TR_ADV
    trace_tf(trace_global, TR_ADV, 0,
	    ("adv_aggregate_match: no match for %A/%A found",
	     target,
	     mask));
#endif	/* TR_ADV */
    return (adv_entry *) 0;

matched:
#ifdef	TR_ADV
    trace_tf(trace_global, TR_ADV, 0,
	    ("adv_aggregate_match: found %A %A matches %A %A %s",
	     target,
	     mask,
	     dm->dm_dest,
	     dm->dm_mask,
	     trace_bits(dm_bits, dm->dm_flags)));
#endif	/* TR_ADV */
    return adv;
}


static void
adv_destmask_free_list __PF1(tree, dest_mask_internal *)
{
    dest_mask_internal *dmip;
    
    DMI_WALK(tree, dmip, FALSE) {
	adv_entry *adv = dmip->dmi_external;

	while (adv) {
	    adv_entry *next = adv->adv_next;

#ifdef	TR_ADV
	    trace_tf(trace_global,
		     TR_ADV,
		     0,
		     ("adv_destmask_free_list:	node %X proto %s flags %s<%s>%X refcount %d",
		      adv,
		      trace_state(rt_proto_bits, adv->adv_proto),
		      trace_state(adv_type_bits, adv->adv_flag & ADVF_TYPE),
		      trace_bits(adv_flag_bits, adv->adv_flag & ~ADVF_TYPE),
		      adv->adv_flag,
		      adv->adv_refcount));
#endif	/* TR_ADV */

	    if (adv->adv_dm.dm_dest) {
		sockfree(adv->adv_dm.dm_dest);
	    }
	    adv_free_list(adv->adv_list);
	    if (BIT_COMPARE(adv->adv_flag, ADVFO_TYPE, ADVFOT_CONFIG)) {
		config_list_free(adv->adv_config);
	    }
	    task_block_free(adv_block_index, (void_t) adv);
	    adv_n_allocated--;

	    adv = next;
	}
	task_block_free(dest_mask_internal_index, (void_t) dmip);
    } DMI_WALK_END(tree, dmip, FALSE) ;
}


#ifndef	adv_destmask_finish
/* Once we have the complete list we need to traverse the list to */
/* Chain the adv_next pointers into order.  We also reset the internal */
/* node pointers on each entry to point to the top of the tree */
adv_entry *
adv_destmask_finish __PF1(list, adv_entry *)
{
    adv_entry *first = (adv_entry *) 0;
    adv_entry **last = &first;
    dest_mask_internal *dmip;
    
    if (!list) {
	return list;
    }

    DMI_WALK(list->adv_dm.dm_internal, dmip, TRUE) {
	adv_entry *adv = dmip->dmi_external;
	adv_entry *adv_dup = adv;
	    
#ifdef	TR_ADV
	trace_tf(trace_global,
		 TR_ADV,
		 0,
		 ("adv_destmask_finish: %-15A %-15A %s",
		  adv->adv_dm.dm_dest,
		  adv->adv_dm.dm_mask,
		  trace_bits(dm_bits, adv->adv_dm.dm_flags)));
#endif	/* TR_ADV */

	/* Point all external nodes at the top of the tree */
	adv->adv_dm.dm_internal = list->adv_dm.dm_internal;

	/* Chain last pointer to this entry */
	*last = adv;

	/* Find end of duplicate list and make sure it is marked as such */
	while (adv_dup->adv_next
	       && !BIT_TEST(adv_dup->adv_dm.dm_flags, DMF_EOL)) {
	    adv_dup = adv_dup->adv_next;
#ifdef	TR_ADV
	    trace_tf(trace_global,
		     TR_ADV,
		     0,
		     ("adv_destmask_finish: %-15A %-15A %s",
		      adv_dup->adv_dm.dm_dest,
		      adv_dup->adv_dm.dm_mask,
		      trace_bits(dm_bits, adv_dup->adv_dm.dm_flags)));
#endif	/* TR_ADV */
	}
	BIT_SET(adv_dup->adv_dm.dm_flags, DMF_EOL);

	/* Remember where to chain the next one */
	last = &adv_dup->adv_next;
    } DMI_WALK_END(list->adv_dm.dm_internal, dmip, TRUE) ;

    /* Make sure the end of the chain is marked as such */
    *last = (adv_entry *) 0;

#ifdef	TR_ADV
    trace_tf(trace_global,
	     TR_ADV,
	     0,
	     ("adv_destmask_finish: first: %-15A %-15A %s",
	      first->adv_dm.dm_dest,
	      first->adv_dm.dm_mask,
	      trace_bits(dm_bits, first->adv_dm.dm_flags)));
#endif	/* TR_ADV */

    return first;
}
#endif	/* adv_destmask_finish */


/* Calculate the depth of each dest/mask entry on the tree */
void
adv_destmask_depth __PF1(list, adv_entry *)
{
    register dest_mask_internal *dmi = list ? list->adv_dm.dm_internal : (dest_mask_internal *) 0;
    dest_mask_internal *stack[MAXDEPTH];
    register dest_mask_internal **sp = stack;
    register int externals_stacked = 0;

    /* Our procedure is to walk to tree and set the depth of all */
    /* branch (non-leaf) nodes to 0.  At each leaf node we will */
    /* walk up the stack looking for branch nodes with external */
    /* information, assigning larger depths on the way up.  If we */
    /* find a branch node that already has a sufficient depth */
    /* we can limit our search */
    while (dmi) {
	if (!dmi->dmi_right && !dmi->dmi_left) {
	    register dest_mask_internal **xsp;
		
	    /* This is a leaf */

	    dmi->dmi_external->adv_result.res_metric = 1;
	    if (externals_stacked) {
		register u_int depth = 2;

		for (xsp = (sp - 1);
		     xsp >= stack;
		     xsp--) {
		    if ((*xsp)->dmi_external) {
			if ((*xsp)->dmi_external->adv_result.res_metric >= depth) {
			    break;
			}
			(*xsp)->dmi_external->adv_result.res_metric = depth++;
		    }
		}
	    }

	    /* Search up the tree for the next chance to go right */
	    for (;;) {
		register dest_mask_internal *next;

		if (sp == stack) {
		    dmi = (dest_mask_internal *) 0;
		    break;
		}

		next = sp[-1];
		if (next->dmi_left == dmi
		    && next->dmi_right) {
		    dmi = next->dmi_right;
		    break;
		} else {
		    dmi = next;
		    --sp;
		    if (dmi->dmi_external) {
			externals_stacked--;
		    }
		}
	    }
	} else {
	    /* Branch */

	    if (dmi->dmi_external) {
		dmi->dmi_external->adv_result.res_metric = 0;
		externals_stacked++;
	    }
	    *sp++ = dmi;
	    if (dmi->dmi_left) {
		dmi = dmi->dmi_left;
	    } else {
		dmi = dmi->dmi_right;
	    }
	}
    }
}

/**/

/* Add an entry to the psfunc table for a specific protocol */
void
adv_psfunc_add __PF2(proto, proto_t,
		     psp, adv_psfunc *)
{
    control_psfunc[proto] = psp;
}


/*
 *	Allocate an adv_entry.
 */
adv_entry *
adv_alloc __PF2(flags, flag_t,
		proto, proto_t)
{
    adv_entry *ale;

    /* Allocate an adv_list entry and put address into it */
    ale = (adv_entry *) task_block_alloc(adv_block_index);

    ale->adv_refcount = 1;
    ale->adv_flag = flags;
    ale->adv_proto = proto;
#ifdef	TR_ADV
    trace_tf(trace_global,
	     TR_ADV,
	     0,
	     ("adv_alloc: node %X proto %s flags %s<%s>%X refcount %d",
	      ale,
	      trace_state(rt_proto_bits, ale->adv_proto),
	      trace_state(adv_type_bits, ale->adv_flag & ADVF_TYPE),
	      trace_bits(adv_flag_bits, ale->adv_flag & ~ADVF_TYPE),
	      ale->adv_flag,
	      ale->adv_refcount));
#endif	/* TR_ADV */
    adv_n_allocated++;
    return ale;
}


/*	Free an adv_entry list	*/
void
adv_free_list __PF1(adv, adv_entry *)
{
    static int level = 0;
#ifdef	TR_ADV
    int allocated = adv_n_allocated;
#endif	/* TR_ADV */

    level++;
    
    while (adv) {
	register adv_entry *advn = adv;

	adv = adv->adv_next;
	if (!--advn->adv_refcount) {
#ifdef	TR_ADV
	    trace_tf(trace_global,
		     TR_ADV,
		     0,
		     ("adv_free_list:%.*snode %X proto %s flags %s<%s>%X refcount %d",
		      level, policy_tabs,
		      advn,
		      trace_state(rt_proto_bits, advn->adv_proto),
		      trace_state(adv_type_bits, advn->adv_flag & ADVF_TYPE),
		      trace_bits(adv_flag_bits, advn->adv_flag & ~ADVF_TYPE),
		      advn->adv_flag,
		      advn->adv_refcount));
#endif	/* TR_ADV */

	    /* Free any children */
	    adv_free_list(advn->adv_list);
	    if (BIT_COMPARE(advn->adv_flag, ADVFO_TYPE, ADVFOT_CONFIG)) {
		config_list_free(advn->adv_config);
	    }

	    /* Free any type specific info */
	    switch (advn->adv_flag & ADVF_TYPE) {

	    case ADVFT_PS:
		if (PS_FUNC_VALID(advn, ps_free)) {
		    PS_FUNC(advn, ps_free)(advn);
		}
		break;

	    case ADVFT_ANY:
	    case ADVFT_GW:
	    case ADVFT_AS:
	    case ADVFT_TAG:
		break;

	    case ADVFT_IFN:
	    case ADVFT_IFAE:
		ifae_free(advn->adv_ifae);
		break;

	    case ADVFT_DM:
		assert(advn->adv_dm.dm_internal);

		adv_destmask_free_list(advn->adv_dm.dm_internal);
		/* Prevent this element from being freed twice and */
		/* cause a break out of the loop */
		adv = advn = (adv_entry *) 0;
		break;

#ifdef	PROTO_ASPATHS
	    case ADVFT_ASPATH:
		aspath_adv_free(advn->adv_aspath);
		break;
#endif	/* PROTO_ASPATHS */

	    default:
		assert(FALSE);
	    }

	    if (advn) {
		task_block_free(adv_block_index, (void_t) advn);
		adv_n_allocated--;
	    }
	}
    }
    
#ifdef	TR_ADV
    if (allocated != adv_n_allocated) {
	trace_tf(trace_global,
		 TR_ADV,
		 0,
		 ("adv_free_list:%.*s%d of %d freed",
		  level, policy_tabs,
		  allocated - adv_n_allocated,
		  allocated));
    }
#endif	/* TR_ADV */
    
    level--;
}


/*
 *	Cleanup for a protocol
 */
void
adv_cleanup __PF7(proto, proto_t,
		  n_trusted, int *,
		  n_source, int *,
		  gw_list, gw_entry *,
		  intf_policy, adv_entry **,
		  import_list, adv_entry **,
		  export_list, adv_entry **)
{
    gw_entry *gwp;

    /* Reset trusted and source lists */
    if (n_trusted) {
	*n_trusted = 0;
    }
    if (n_source) {
	*n_source = 0;
    }

    /* Reset gateway list and trusted/source bits */
    GW_LIST(gw_list, gwp) {
	BIT_RESET(gwp->gw_flags, GWF_TRUSTED|GWF_SOURCE);
	adv_free_list(gwp->gw_import);
	gwp->gw_import = (adv_entry *) 0;
	adv_free_list(gwp->gw_export);
	gwp->gw_export = (adv_entry *) 0;
    } GW_LIST_END(gw_list, gwp) ;

    /* Free import an export lists */
    if (import_list && *import_list) {
	adv_free_list(*import_list);
	*import_list = (adv_entry *) 0;
    }
    if (export_list && *export_list) {
	adv_free_list(*export_list);
	*export_list = (adv_entry *) 0;
    }

    /* Free the interface policy */
    if (intf_policy && *intf_policy) {
	adv_free_list(*intf_policy);
	*intf_policy = (adv_entry *) 0;
    }

    if (proto) {
	/* Free the interface import/export policy */
	if (int_import[proto]) {
	    adv_free_list(int_import[proto]);
	    int_import[proto] = (adv_entry *) 0;
	}
	if (int_export[proto]) {
	    adv_free_list(int_export[proto]);
	    int_export[proto] = (adv_entry *) 0;
	}
    }
}


/*
 *	Determine if a route is valid to an interior protocol
 */
int
export __PF6(rt, rt_entry *,
	     proto, proto_t,
	     proto_list, adv_entry *,
	     int_list, adv_entry *,
	     gw_list, adv_entry *,
	     result, adv_results *)
{
    int n_list, success, metric, match = FALSE;
    adv_entry *adv = NULL;
    adv_entry *list = NULL;
    adv_entry *sublist = NULL;
    adv_entry *lists[3];

    /* Build an array of lists to ease processing */
    lists[0] = proto_list;
    lists[1] = int_list;
    lists[2] = gw_list;

    if (rt->rt_gwp->gw_proto == RTPROTO_DIRECT) {
	/* Default to always announcing interface routes */

	success = metric = TRUE;
    } else if (rt->rt_gwp->gw_proto == proto) {
	/* If same protocol */

	/* Default to announcing it */
	success = TRUE;

	/* Pass on metric and do not allow it to be tampered with */
	result->res_metric = rt->rt_metric;
	metric = FALSE;
    } else {
	success = FALSE;
	metric = TRUE;
    }

    /* Repeat for each list, gw, int and proto */
    n_list = 2;
    do {
	if (lists[n_list]) {
	    ADV_LIST(lists[n_list], list) {
		/* Null list */
		if (!list->adv_list) {

		    /* Check for global restrict */
		    if (BIT_TEST(list->adv_flag, ADVF_NO)) {
			success = FALSE;
			goto Return;
		    }

		    /* Examine next list */
		    continue;
		}
		ADV_LIST(list->adv_list, sublist) {
		    if (sublist->adv_proto == RTPROTO_ANY
			|| rt->rt_gwp->gw_proto == sublist->adv_proto) {

			switch (sublist->adv_flag & ADVF_TYPE) {
			case ADVFT_ANY:
			    match = TRUE;
			    break;

			case ADVFT_GW:
			    if (rt->rt_gwp == sublist->adv_gwp) {
				match = TRUE;
			    }
			    break;

			case ADVFT_IFN:
			    if (RT_IFAP(rt)->ifa_link->ifl_nameent == sublist->adv_ifn
				|| RT_IFAP(rt)->ifa_link->ifl_nameent_wild == sublist->adv_ifn) {
				match = TRUE;
			    }
			    break;

			case ADVFT_IFAE:
			    if (RT_IFAP(rt)->ifa_addrent == sublist->adv_ifae) {
				match = TRUE;
			    }
			    break;
			    
			case ADVFT_AS:
			    if (rt->rt_gwp->gw_peer_as == sublist->adv_as) {
				match = TRUE;
			    }
			    break;

			case ADVFT_DM:
			    assert(FALSE);
			    break;

#ifdef	PROTO_ASPATHS
			case ADVFT_TAG:
			    if (tag_rt(rt) == sublist->adv_tag) {
				match = TRUE;
			    }
			    break;

			case ADVFT_ASPATH:
			    if (aspath_adv_match(sublist->adv_aspath, rt->rt_aspath)) {
				match = TRUE;
			    }
			    break;
#endif	/* PROTO_ASPATHS */

			case ADVFT_PS:
			    if (PS_FUNC_VALID(sublist, ps_rtmatch) &&
				PS_FUNC(sublist, ps_rtmatch)(sublist->adv_ps, rt)) {
				match = TRUE;
			    }
			    break;
			}
			if (match) {
			    if (sublist->adv_list) {
				if ((adv = adv_destmask_match(sublist->adv_list,
							      rt->rt_dest,
							      rt->rt_dest_mask))) {
				    goto Match;
				}
				success = FALSE;
			    } else {
				success = TRUE;
				goto Match;
			    }
			    match = FALSE;
			}
		    }
		} ADV_LIST_END(list->adv_list, sublist) ;
	    } ADV_LIST_END(lists[n_list], list) ;
	    success = FALSE;
	}
    } while (n_list--);

 Match:
    if (match) {
	if ((adv && BIT_TEST(adv->adv_flag, ADVF_NO)) ||
	    BIT_TEST(sublist->adv_flag, ADVF_NO)) {
	    success = FALSE;
	} else {
	    success = TRUE;

	    /* Set metric */
	    if (metric) {
		if (adv && BIT_TEST(adv->adv_flag, ADVFOT_METRIC)) {
		    result->res_metric = adv->adv_result.res_metric;
		} else if (BIT_TEST(sublist->adv_flag, ADVFOT_METRIC)) {
		    result->res_metric = sublist->adv_result.res_metric;
		} else if (BIT_TEST(list->adv_flag, ADVFOT_METRIC)) {
		    result->res_metric = list->adv_result.res_metric;
		}
	    }

	    /* Set second metric */
	    if (adv && BIT_TEST(adv->adv_flag, ADVFOT_METRIC2)) {
		result->res_metric2 = adv->adv_result.res_metric2;
	    } else if (BIT_TEST(sublist->adv_flag, ADVFOT_METRIC2)) {
		result->res_metric2 = sublist->adv_result.res_metric2;
	    } else if (BIT_TEST(list->adv_flag, ADVFOT_METRIC2)) {
		result->res_metric2 = list->adv_result.res_metric2;
	    }

	    /* Set flag */
	    if (adv && BIT_TEST(adv->adv_flag, ADVFOT_FLAG)) {
		result->res_flag = adv->adv_result.res_flag;
	    } else if (BIT_TEST(sublist->adv_flag, ADVFOT_FLAG)) {
		result->res_flag = sublist->adv_result.res_flag;
	    } else if (BIT_TEST(list->adv_flag, ADVFOT_FLAG)) {
		result->res_flag = list->adv_result.res_flag;
	    }
	}
    }

 Return:
    return success;
}


/*
 *  Determine if a route is valid from an interior protocol.  The default
 *  action and preference should be preset into the preference and success
 *  arguments.
 */
int
import __PF8(dst, sockaddr_un *,
	     mask, sockaddr_un *,
	     proto_list, adv_entry *,
	     int_list, adv_entry *,
	     gw_list, adv_entry *,
	     preference, pref_t *,
	     ifap, if_addr *,
	     ps_data, void_t)
{
    int n_list, success = TRUE;
    adv_entry *adv = NULL;
    adv_entry *list = NULL;
    adv_entry *lists[3];

    /* Build an array of lists to ease processing */
    lists[0] = proto_list;
    lists[1] = int_list;
    lists[2] = gw_list;

    /* Repeat for each list, gw, int and proto */
    n_list = 2;
    do {
	if (lists[n_list]) {
	    ADV_LIST(lists[n_list], list) {
		switch (list->adv_flag & ADVF_TYPE) {
#ifdef	PROTO_ASPATHS
		case ADVFT_ASPATH:
		    if (!aspath_adv_match(list->adv_aspath, (as_path *) ps_data)) {
			continue;
		    }
		    goto common;
#endif	/* PROTO_ASPATHS */
		    
		case ADVFT_PS:
		    if (!PS_FUNC_VALID(list, ps_dstmatch) ||
			!PS_FUNC(list, ps_dstmatch)(list->adv_ps, dst, ps_data)) {
			continue;
		    }
		    goto common;

		case ADVFT_IFN:
		case ADVFT_IFAE:
		case ADVFT_AS:
		case ADVFT_ANY:
		common:
		    /* Null list */
		    if (!list->adv_list) {
			/* check for global restrict */

			if (BIT_TEST(list->adv_flag, ADVF_NO)) {
			    success = FALSE;
			    goto finish;
			}
			continue;
		    }

		    if ((adv = adv_destmask_match(list->adv_list,
						  dst,
						  mask))) {
			/* Have a match */
			
			if (BIT_TEST(list->adv_flag, ADVF_NO) ||
			    BIT_TEST(adv->adv_flag, ADVF_NO)) {
			    /* Negative match */
			    
			    success = FALSE;
			} else {
			    /* Positive match, check preference */
			    
			    success = TRUE;
			    if (BIT_TEST(adv->adv_flag, ADVFOT_PREFERENCE)) {
				*preference = adv->adv_result.res_preference;
			    } else if (BIT_TEST(list->adv_flag, ADVFOT_PREFERENCE)) {
				*preference = list->adv_result.res_preference;
			    }
			}
			goto finish;
		    }
		    break;

		default:
		    assert(FALSE);
		    break;
		}
	    } ADV_LIST_END(lists[n_list], list);

	    success = FALSE;
	}
    } while (n_list-- && !adv);

 finish:
    if (!success) {
	*preference = -1;
    }
    
    return success;
}


/**/

/* Martians */

/* Add an entry - used only at init time */
void
martian_add __PF4(dest, sockaddr_un *,
		  mask, sockaddr_un *,
		  adv_flag, flag_t,
		  dm_flag, flag_t)
{
    adv_entry *adv, *list;
    
    adv = adv_alloc(ADVFT_DM | adv_flag, (proto_t) 0);
    adv->adv_dm.dm_dest = sockdup(dest);
    adv->adv_dm.dm_mask = mask;
    adv->adv_dm.dm_flags = dm_flag;
    list = adv_destmask_insert((char *) 0, SI_MARTIANS(socktype(dest)), adv);
    if (list) {
	SI_MARTIANS(socktype(dest)) = list;
    }
}


/*  Return true if said network is a martian */
int
is_martian __PF2(addr, sockaddr_un *,
		 mask, sockaddr_un *)
{
    register adv_entry *adv = adv_destmask_match(SI_MARTIANS(socktype(addr)), addr, mask);

    return (adv && BIT_TEST(adv->adv_flag, ADVF_NO)) ? TRUE : FALSE;
}


/**/

static block_t gw_block_index;

/*
 *	Lookup a gateway entry
 */
static inline gw_entry *
gw_lookup __PF3(lookup_list, gw_entry **,
		lookup_proto, proto_t,
		lookup_addr, sockaddr_un *)
{
    register gw_entry *lookup_gwp;

    GW_LIST(*lookup_list, lookup_gwp) {
	if ((lookup_gwp->gw_proto == lookup_proto) &&
	    ((lookup_gwp->gw_addr && lookup_addr && sockaddrcmp(lookup_gwp->gw_addr, lookup_addr)) ||
	     (lookup_gwp->gw_addr == lookup_addr))) {
	    break;
	}
    } GW_LIST_END(*lookup_list, lookup_gwp) ;

    return lookup_gwp;
}


/*
 *	Add a gateway entry to end of the list
 */
static inline gw_entry *
gw_add __PF7(add_list, gw_entry **,
	     add_proto, proto_t,
	     add_tp, task *,
	     add_peer_as, as_t,
	     add_local_as, as_t,
	     add_addr, sockaddr_un *,
	     add_flags, flag_t)
{
    gw_entry *add_gwp, *gwp_new;

    gwp_new = (gw_entry *) task_block_alloc(gw_block_index);

    if (*add_list) {
	GW_LIST(*add_list, add_gwp) {
	    if (!add_gwp->gw_next) {
		add_gwp->gw_next = gwp_new;
		break;
	    }
	} GW_LIST_END(*add_list, add_gwp) ;
    } else {
	*add_list = gwp_new;
    }
    if (add_addr) {
	gwp_new->gw_addr = sockdup(add_addr);
	sockclean(gwp_new->gw_addr);
    }
    gwp_new->gw_proto = add_proto;
    gwp_new->gw_task = add_tp;
    gwp_new->gw_peer_as = add_peer_as;
    gwp_new->gw_local_as = add_local_as;
    gwp_new->gw_flags |= add_flags;
    gwp_new->gw_rtq.rtq_forw = gwp_new->gw_rtq.rtq_back = &gwp_new->gw_rtq;
    return gwp_new;
}


/*
 *	Find an existing gw_entry or create a new one
 */
gw_entry *
gw_locate __PF7(locate_list, gw_entry **,
		locate_proto, proto_t,
		locate_tp, task *,
		locate_peer_as, as_t,
		locate_local_as, as_t,
		locate_addr, sockaddr_un *,
		locate_flags, flag_t)
{
    gw_entry *locate_gwp;

    locate_gwp = gw_lookup(locate_list, locate_proto, locate_addr);
    if (locate_gwp) {
	locate_gwp->gw_flags |= locate_flags;
	locate_gwp->gw_task = locate_tp;
    } else {
	locate_gwp = gw_add(locate_list,
			    locate_proto,
			    locate_tp,
			    locate_peer_as,
			    locate_local_as,
			    locate_addr,
			    locate_flags);
    }
    return locate_gwp;
}


/*
 *	Update last heard from timer for a gateway
 */
gw_entry *
gw_timestamp __PF7(list, gw_entry **,
		   proto, proto_t,
		   tp, task *,
		   peer_as, as_t,
		   local_as, as_t,
		   addr, sockaddr_un *,
		   flags, flag_t)
{
    gw_entry *gwp;

    gwp = gw_locate(list, proto, tp, peer_as, local_as, addr, flags);
    gwp->gw_time = time_sec;
    return gwp;
}


/*
 *	Initialize a gateway structure
 */
gw_entry *
gw_init __PF7(gwp, gw_entry *,
	      proto, proto_t,
	      tp, task *,
	      peer_as, as_t,
	      local_as, as_t,
	      addr, sockaddr_un *,
	      flags, flag_t)
{
    gw_entry *list = (gw_entry *) 0;
    
    if (gwp) {
	gwp->gw_proto = proto;
	gwp->gw_task = tp;
	gwp->gw_peer_as = peer_as;
	gwp->gw_local_as = local_as;
	gwp->gw_flags |= flags;
	if (addr) {
	    gwp->gw_addr = sockdup(addr);
	}
    } else {
	gwp = gw_add(&list, proto, tp, peer_as, local_as, addr, flags);
    }

    return gwp;
}


/*
 *	Free a gw list
 */
void
gw_freelist __PF1(gwp, gw_entry *)
{
    while (gwp) {
	gw_entry *next_gwp = gwp->gw_next;

	if (gwp->gw_addr) {
	    sockfree(gwp->gw_addr);
	}
	task_block_free(gw_block_index, (void_t) gwp);

	gwp = next_gwp;
    }
}


/*
 *	Dump gateway information
 */
void
gw_dump __PF4(fd, FILE *,
	      name, const char *,
	      list, gw_entry *,
	      proto, proto_t)
{
    gw_entry *gwp;

    GW_LIST(list, gwp) {
	(void) fprintf(fd, name);
	if (gwp->gw_addr) {
	    (void) fprintf(fd, " %#-20A",
			   gwp->gw_addr);
	}
	if (gwp->gw_proto != proto) {
	    (void) fprintf(fd, " proto %s",
			   trace_state(rt_proto_bits, gwp->gw_proto));
	}
	if (gwp->gw_time) {
	    (void) fprintf(fd, " last update: %T",
			   gwp->gw_time);
	}
	if (gwp->gw_flags) {
	    (void) fprintf(fd, " flags: <%s>",
			   trace_bits(gw_bits, gwp->gw_flags));
	}
	(void) fprintf(fd, "\n%s\troutes: %u",
		       name,
		       gwp->gw_n_routes);
	if (gwp->gw_task) {
	    (void) fprintf(fd, " task: %s",
			   task_name(gwp->gw_task));
	}
	if (gwp->gw_peer_as) {
	    (void) fprintf(fd, " peer AS: %d",
			   gwp->gw_peer_as);
	}
	if (gwp->gw_local_as) {
	    (void) fprintf(fd, " local AS: %d",
			   gwp->gw_local_as);
	}
	(void) fprintf(fd, "\n");
    } GW_LIST_END(list, gwp);
}


/*
 *	Dump a dest/mask list displaying metric and preference if present
 */
void
control_dmlist_dump __PF5(fd, FILE *,
			  level, int,
			  list, adv_entry *,
			  dummy1, adv_entry *,
			  dummy2, adv_entry *)
{
    register dest_mask_internal *dmi;
    
    if (!list || !list->adv_dm.dm_internal) {
	return;
    }

    DMI_WALK_ALL(list->adv_dm.dm_internal, dmi, adv) {
	dest_mask *dp = &adv->adv_dm;

	(void) fprintf(fd, "%.*s%-15A  mask %-15A %s",
		       level, policy_tabs,
		       adv == dmi->dmi_external ? dp->dm_dest : sockbuild_str(""),
		       dp->dm_mask,
		       trace_bits(dm_bits, dp->dm_flags));
	if (BIT_TEST(adv->adv_flag, ADVF_NO)) {
	    (void) fprintf(fd, " restrict\n");
	} else {
	    switch (adv->adv_flag & ADVFO_TYPE) {
	    case ADVFOT_NONE:
		(void) fprintf(fd, "\n");
		break;

	    case ADVFOT_METRIC:
		(void) fprintf(fd, "  metric %d\n",
			       adv->adv_result.res_metric);
		break;

	    case ADVFOT_PREFERENCE:
		(void) fprintf(fd, "  preference %d\n",
			       adv->adv_result.res_preference);
		break;
	    }
	}

    } DMI_WALK_ALL_END(list->adv_dm.dm_internal, dmi, adv) ;
}


void
control_interface_import_dump __PF3(fd, FILE *,
				    level, int,
				    list, adv_entry *)
{
    ADV_LIST(list, list) {
	switch (list->adv_flag & ADVF_TYPE) {
	case ADVFT_ANY:
	    break;

	case ADVFT_IFN:
	{
	    const char *type = "*";
	    register char *cp = list->adv_ifn->ifae_addr->s.gs_string;

	    do {
		if (isdigit(*cp)) {
		    type = "";
		    break;
		}
	    } while (*cp++) ;
	    (void) fprintf(fd,
			   "%.*sInterface %""A%s\n",
			   level++, policy_tabs,
			   list->adv_ifn->ifae_addr,
			   type);
	}
	    break;

	case ADVFT_IFAE:
	    (void) fprintf(fd,
			   "%.*sInterface %A\n",
			   level++, policy_tabs,
			   list->adv_ifae->ifae_addr);
	    break;
			
	default:
	    assert(FALSE);
	}
	if (BIT_TEST(list->adv_flag, ADVF_NO)) {
	    (void) fprintf(fd, "%.*sRestrict\n",
			   level, policy_tabs);
	} else if (BIT_TEST(list->adv_flag, ADVFOT_PREFERENCE)) {
	    (void) fprintf(fd, "%.*sPreference %d:\n",
			   level, policy_tabs,
			   list->adv_result.res_preference);
	    level++;
	}
	control_dmlist_dump(fd, level, list->adv_list, (adv_entry *) 0, (adv_entry *) 0);
    } ADV_LIST_END(list, list) ;
}


void
control_import_dump __PF5(fd, FILE *,
			  level, int,
			  proto, proto_t,
			  proto_list, adv_entry *,
			  gw_list, gw_entry *)
{
    int lower;
    adv_entry *adv;
    gw_entry *gwp;

    if (proto_list ||
	proto ||
	gw_list) {
	(void) fprintf(fd, "%.*sImport controls:\n",
		       level++, policy_tabs);
    }
    if (proto_list) {
	ADV_LIST(proto_list, adv) {
	    lower = level;
	    if (BIT_TEST(adv->adv_flag, ADVF_NO)) {
		(void) fprintf(fd, "%.*sRestrict\n",
			       level, policy_tabs);
	    } else if (BIT_TEST(adv->adv_flag, ADVFOT_PREFERENCE)) {
		(void) fprintf(fd, "%.*sPreference %d:\n",
			       level, policy_tabs,
			       adv->adv_result.res_preference);
		lower++;
	    }
	    control_dmlist_dump(fd, lower, adv->adv_list, (adv_entry *) 0, (adv_entry *) 0);
	} ADV_LIST_END(proto_list, adv) ;
    }
    if (proto && int_import[proto]) {
	control_interface_import_dump(fd, level, int_import[proto]);
    }
    if (gw_list) {
	GW_LIST(gw_list, gwp) {
	    adv = gwp->gw_import;
	    if (!adv) {
		continue;
	    }
	    lower = level + 1;
	    (void) fprintf(fd, "%.*sGateway %A:\n",
			   level, policy_tabs,
			   gwp->gw_addr);
	    if (BIT_TEST(adv->adv_flag, ADVF_NO)) {
		(void) fprintf(fd, "%.*sRestrict\n",
			       level + 1, policy_tabs);
	    } else if (BIT_TEST(adv->adv_flag, ADVFOT_PREFERENCE)) {
		(void) fprintf(fd, "%.*sPreference %d:\n",
			       level + 1, policy_tabs,
			       adv->adv_result.res_preference);
		lower++;
	    }
	    control_dmlist_dump(fd, lower, adv->adv_list, (adv_entry *) 0, (adv_entry *) 0);
	} GW_LIST_END(gw_list, gwp);
    }
}


void
control_entry_dump __PF3(fd, FILE *,
			 level, int,
			 list, adv_entry *)
{
    int first = TRUE;
    adv_entry *adv;

    if (list) {
	(void) fprintf(fd, "%.*s", level, policy_tabs);
	if (list->adv_proto != RTPROTO_ANY) {
	    (void) fprintf(fd, "Protocol %s ",
			   trace_state(rt_proto_bits, list->adv_proto));
	}
	adv = list;
	if (adv) {
	    do {
		switch (adv->adv_flag & ADVF_TYPE) {

		case ADVFT_DM:
		    assert(FALSE);
		    break;

		case ADVFT_ANY:
		    break;

		case ADVFT_GW:
		    (void) fprintf(fd, " %s%A",
				   first ? "gateway " : "",
				   adv->adv_gwp->gw_addr);
		    break;

		case ADVFT_IFN:
		    (void) fprintf(fd, " %s%""A%s",
				   first ? "interface " : "",
				   adv->adv_ifn->ifae_addr,
				   ifn_wildcard(adv->adv_ifn) ? "" : "*");
		    break;

		case ADVFT_IFAE:
		    (void) fprintf(fd, " %s%A",
				   first ? "interface " : "",
				   adv->adv_ifae->ifae_addr);
		    break;
		    
		case ADVFT_AS:
		    (void) fprintf(fd, " %s%u",
				   first ? "as " : "",
				   adv->adv_as);
		    break;	

#ifdef	PROTO_ASPATHS
		case ADVFT_TAG:
		    (void) fprintf(fd, " %s%A",
				   first ? "tag " : "",
				   sockbuild_in(0, adv->adv_tag));
		    break;

		case ADVFT_ASPATH:
		    (void) fprintf(fd, " %s", first ? "aspath " : "");
		    aspath_adv_print(fd,
				     adv->adv_aspath);
		    break;
#endif	/* PROTO_ASPATHS */

		case ADVFT_PS:
		    if (PS_FUNC_VALID(adv, ps_print)) {
			(void) fprintf(fd, " %s",
				       PS_FUNC(adv, ps_print)(adv->adv_ps, first));
		    }
		    break;
		}
		first = FALSE;
	    } while ((adv = adv->adv_next) && !BIT_TEST(adv->adv_flag, ADVF_FIRST));
	}
	if (BIT_TEST(list->adv_flag, ADVF_NO)) {
 	    (void) fprintf(fd, " restrict");
	} else {
	    switch (list->adv_flag & ADVFO_TYPE) {
	    case ADVFOT_NONE:
		break;

	    case ADVFOT_METRIC:
		(void) fprintf(fd, " metric %d", list->adv_result.res_metric);
		break;

	    case ADVFOT_PREFERENCE:
		if (list->adv_result.res_preference < 0) {
		    (void) fprintf(fd, " restrict");
		} else {
		    (void) fprintf(fd, " preference %d",
				   list->adv_result.res_preference);
		}
		break;
	    }
	}
    }
    (void) fprintf(fd, "\n");
}


static void
control_export_list_dump __PF3(fd, FILE *,
			       level, int,
			       list, adv_entry *)
{
    int lower;
    adv_entry *adv;

    if (list) {
	ADV_LIST(list, list) {
	    lower = level;
	    if (BIT_TEST(list->adv_flag, ADVFOT_METRIC)) {
		(void) fprintf(fd, "%.*sMetric %d:\n",
			       level, policy_tabs,
			       list->adv_result.res_metric);
		lower++;
	    }
	    adv = list->adv_list;
	    if (adv) {
		do {
		    control_entry_dump(fd, lower, adv);
		    if (adv->adv_list) {
			control_dmlist_dump(fd, lower + 1, adv->adv_list, (adv_entry *) 0, (adv_entry *) 0);
		    }
		    do {
			adv = adv->adv_next;
		    } while (adv && !BIT_TEST(adv->adv_flag, ADVF_FIRST));
		} while (adv);
	    }
	} ADV_LIST_END(list, list) ;
    }
}


void
control_interface_export_dump __PF3(fd, FILE *,
				    level, int,
				    list, adv_entry *)
{
    ADV_LIST(list, list) {
	switch (list->adv_flag & ADVF_TYPE) {
	case ADVFT_ANY:
	    break;

	case ADVFT_IFN:
	    (void) fprintf(fd,
			   "%.*sInterface %""A%s\n",
			   level++, policy_tabs,
			   list->adv_ifn->ifae_addr,
			   ifn_wildcard(list->adv_ifn) ? "" : "*");
	    break;

	case ADVFT_IFAE:
	    (void) fprintf(fd,
			   "%.*sInterface %A\n",
			   level++, policy_tabs,
			   list->adv_ifae->ifae_addr);
	    break;
			
	default:
	    assert(FALSE);
	}
	if (BIT_TEST(list->adv_flag, ADVF_NO)) {
	    (void) fprintf(fd, "%.*sRestrict\n",
			   level, policy_tabs);
	} else if (BIT_TEST(list->adv_flag, ADVFOT_METRIC)) {
	    (void) fprintf(fd, "%.*sMetric %d:\n",
			   level, policy_tabs,
			   list->adv_result.res_metric);
	    level++;
	}
	control_export_list_dump(fd, level, list);
	level--;
    } ADV_LIST_END(list, list) ;
}


void    
control_export_dump __PF5(fd, FILE *,
			  level, int,
			  proto, proto_t,
			  proto_list, adv_entry *,
			  gw_list, gw_entry *)
{
    adv_entry *adv;
    gw_entry *gwp;

    if (proto_list ||
	proto ||
	gw_list) {
	(void) fprintf(fd, "%.*sExport controls:\n",
		       level++, policy_tabs);
    }
    control_export_list_dump(fd, level, proto_list);

    if (proto && int_export[proto]) {
	control_interface_export_dump(fd, level, int_export[proto]);
#ifdef	notdef
	if_addr *ifap;

	IF_ADDR(ifap) {
	    adv = ifap->ifa_ps[proto].ips_export;
	    if (adv) {
		(void) fprintf(fd, "%.*sInterface %s  Address %A:\n",
			       level, policy_tabs,
			       ifap->ifa_link->ifl_name,
			       ifap->ifa_addr);
		control_export_list_dump(fd, level + 1, adv);
	    }
	} IF_ADDR_END(ifap) ;
#endif	/* notdef */
    }
    if (gw_list) {
	GW_LIST(gw_list, gwp) {
	    adv = gwp->gw_export;
	    if (adv) {
		(void) fprintf(fd, "%.*sGateway %A:\n",
			       level, policy_tabs,
			       gwp->gw_addr);
		control_export_list_dump(fd, level + 1, adv);
	    }
	} GW_LIST_END(gw_list, gwp);
    }
}


void
control_interior_dump(fd, level, func, list)
FILE *fd;
int level;
_PROTOTYPE(func,
	   void,
	   (FILE *,
	    int,
	    proto_t,
	    adv_entry *,
	    gw_entry *));
adv_entry *list;
{
    adv_entry *adv;

    ADV_LIST(list, adv) {
	switch (adv->adv_flag & ADVF_TYPE) {
	case ADVFT_AS:
	    (void) fprintf(fd, "%.*sAS %u:\n",
			   level, policy_tabs,
			   adv->adv_as);
	    break;

#ifdef	PROTO_ASPATHS
	case ADVFT_TAG:
	    (void) fprintf(fd, "%.*sTAG %A:\n",
			   level, policy_tabs,
			   sockbuild_in(0, adv->adv_tag));
	    break;

	case ADVFT_ASPATH:
	    (void) fprintf(fd, "%.*sAS Path: ", level, policy_tabs);
	    aspath_adv_print(fd,
			     adv->adv_aspath);
	    break;
#endif	/* PROTO_ASPATHS */
			   
	case ADVFT_PS:
	    if (PS_FUNC_VALID(adv, ps_print)) {
		(void) fprintf(fd, "%.*s%s:\n",
			       level, policy_tabs,
			       PS_FUNC(adv, ps_print)(adv->adv_ps, TRUE));
	    }
	    break;
	}

	func(fd, level + 1, (proto_t) 0, adv, (gw_entry *) 0);
	(void) fprintf(fd, "\n");
    } ADV_LIST_END(list, adv) ;
}


void
control_exterior_dump(fd, level, func, list)
FILE *fd;
int level;
_PROTOTYPE(func,
	   void,
	   (FILE *,
	    int,
	    proto_t,
	    adv_entry *,
	    gw_entry *));
adv_entry *list;
{
    adv_entry *adv;

    ADV_LIST(list, adv) {
	switch (adv->adv_flag & ADVF_TYPE) {
	case ADVFT_AS:
	    (void) fprintf(fd, "%.*sAS %u:\n",
			   level, policy_tabs,
			   adv->adv_as);
	    break;

#ifdef	PROTO_ASPATHS
	case ADVFT_TAG:
	    (void) fprintf(fd, "%.*sTag %A:\n",
			   level, policy_tabs,
			   sockbuild_in(0, adv->adv_tag));
	    break;

	case ADVFT_ASPATH:
	    (void) fprintf(fd, "%.*sAS Path: ", level, policy_tabs);
	    aspath_adv_print(fd,
			     adv->adv_aspath);
	    break;
#endif	/* PROTO_ASPATHS */
			   
	case ADVFT_PS:
	    if (PS_FUNC_VALID(adv, ps_print)) {
		(void) fprintf(fd, "%.*s%s:\n",
			       level, policy_tabs,
			       PS_FUNC(adv, ps_print)(adv->adv_ps, TRUE));
	    }
	    break;
	}
			   
	func(fd, level + 1, (proto_t) 0, adv->adv_list, (gw_entry *) 0);
	(void) fprintf(fd, "\n");
    } ADV_LIST_END(list, adv) ;
}


adv_entry *
control_exterior_locate __PF2(list, adv_entry *,
			      as, as_t)
{
    adv_entry *exterior = (adv_entry *) 0;
    
    if (list) {
	ADV_LIST(list, list) {
	    if (list->adv_as == as) {
		exterior = list->adv_list;
		if (exterior) {
		    break;
		}
	    }
	} ADV_LIST_END(list, list) ;
    }

    return exterior;
}


void
control_interface_dump(fd, level, list, entry_dump)
FILE *fd;
int level;
adv_entry *list;
_PROTOTYPE(entry_dump,
	   void,
	   (FILE *,
	    config_entry *));
{

    if (list) {
	ADV_LIST(list, list) {
	    adv_entry *adv;
	    
	    /* Start the line */
	    (void) fprintf(fd,
			   "%.*s Interface",
			   level, policy_tabs);

	    /* Dump the interface names */
	    ADV_LIST(list, adv) {

		switch (adv->adv_flag & ADVF_TYPE) {
		case ADVFT_ANY:
		    (void) fprintf(fd, " all");
		    break;

		case ADVFT_IFN:
		    (void) fprintf(fd, " %""A%s",
				   adv->adv_ifn->ifae_addr,
				   ifn_wildcard(adv->adv_ifn) ? "" : "*");
		    break;

		case ADVFT_IFAE:
		    (void) fprintf(fd, " %A",
				   adv->adv_ifae->ifae_addr);
		    break;
			
		default:
		    assert(FALSE);
		}
		if (adv->adv_next && BIT_TEST(adv->adv_next->adv_flag, ADVF_FIRST)) {
		    break;
		}
	    } ADV_LIST_END(list, adv) ;

	    if (entry_dump && list->adv_config && list->adv_config->conflist_list) {

		entry_dump(fd, list->adv_config->conflist_list);
	    }
	    
	    /* Dump the contents of the first entry */
	    if (BIT_TEST(list->adv_flag, ADVFOT_FLAG)) {
		(void) fprintf(fd,
			       " <%s>",
			       trace_bits(if_state_bits, list->adv_result.res_flag));
	    }
	    if (BIT_TEST(list->adv_flag, ADVFOT_METRIC)) {
		(void) fprintf(fd,
			       " metric %d",
			       list->adv_result.res_metric);
	    }
	    if (BIT_TEST(list->adv_flag, ADVFOT_PREFERENCE)) {
		if (list->adv_result.res_preference < 0) {
		    (void) fprintf(fd,
				   " restrict");
		} else {
		    (void) fprintf(fd,
				   " preference %d",
				   list->adv_result.res_preference);
		}
	    }

	    /* and end the line */
	    (void) fprintf(fd, "\n");

	    if (!(list = adv)) {
		break;
	    }
	} ADV_LIST_END(list, list) ;

	(void) fprintf(fd, "\n");
    }
}


/**/

config_entry *
config_alloc __PF2(type, int,
		   data, void_t)
{
    config_entry *cp;

    cp = (config_entry *) task_block_alloc(config_entry_index);
    cp->config_type = type;
    cp->config_data = data;

    return cp;
}


config_list *
config_list_alloc(cp, entry_free)
config_entry *cp;
_PROTOTYPE(entry_free,
	   void,
	   (config_entry *));
{
    config_list *list;

    list = (config_list *) task_block_alloc(config_list_index);
    list->conflist_refcount++;
    list->conflist_free = entry_free;
    list->conflist_list = cp;

    return list;
}


config_list *
config_list_add(list, cp, entry_free)
config_list *list;
config_entry *cp;
_PROTOTYPE(entry_free,
	   void,
	   (config_entry *));
{
    if (!list) {
	list = config_list_alloc(cp, entry_free);
    } else {
	list->conflist_list = config_append(list->conflist_list,
					    cp);
    }

    return list->conflist_list ? list : (config_list *) 0;
}


void
config_list_free __PF1(list, config_list *)
{
    if (list && !--list->conflist_refcount) {
	config_entry *next = list->conflist_list;

	while (next) {
	    config_entry *cp = next;

	    next = next->config_next;

	    if (list->conflist_free) {
		list->conflist_free(cp);
	    }
	    task_block_free(config_entry_index, (void_t) cp);
	}

	task_block_free(config_list_index, (void_t) list);
    }
}


config_entry *
config_append __PF2(list, config_entry *,
		    new, config_entry *)
{

    if (list) {
	config_entry *last = (config_entry *) 0;
	register config_entry *cp;

	CONFIG_LIST(cp, list) {
	    if (new->config_type < cp->config_type) {
		/* Insert before this one */
		break;
	    } else if (cp->config_type == new->config_type) {
		/* XXX - Duplicate entry */
		return (config_entry *) 0;
	    }
	    last = cp;
	} CONFIG_LIST_END(cp, list) ;

	if (last) {
	    new->config_next = last->config_next;
	    last->config_next = new;
	} else {
	    new->config_next = list;
	    list = new;
	}
    } else {
	list = new;
    }

    return list;
}


void
config_resolv_free __PF2(list, config_entry **,
			 size, int)
{
    /* Free the array */
    task_block_reclaim((size_t) (size * sizeof (config_entry *)),
		       (void_t) list);
}


config_entry **
config_resolv_ifa __PF3(policy, adv_entry *,
			ifap, if_addr *,
			size, int)
{
    int entries = 0;
    register adv_entry *adv;
    config_entry **list;

    /* Allocate a block */
    if (size) {
	list = (config_entry **) task_block_malloc((size_t) (size * sizeof (config_entry)));
    } else {
	list = (config_entry **) GS2A(TRUE);
    }

    ADV_LIST(policy, adv) {
	int prio = 0;
	
	switch (adv->adv_flag & ADVF_TYPE) {
		
	case ADVFT_ANY:
	    prio = CONFIG_PRIO_ANY;
	    break;

	case ADVFT_IFN:
	    if (ifap->ifa_link->ifl_nameent == adv->adv_ifn) {
		prio = CONFIG_PRIO_NAME;
	    } else if (ifap->ifa_link->ifl_nameent_wild == adv->adv_ifn) {
		prio = CONFIG_PRIO_WILD;
	    }
	    break;

	case ADVFT_IFAE:
	    if (ifap->ifa_addrent == adv->adv_ifae) {
		prio = CONFIG_PRIO_ADDR;
	    }
	    break;

	default:
	    assert(FALSE);
	}

	if (prio) {
	    register config_entry *cp;

	    /* We have a match */
	    entries++;

	    if (!size) {
		/* Just looking for an interface match */

		break;
	    }
	    if (BIT_COMPARE(adv->adv_flag, ADVFO_TYPE, ADVFOT_CONFIG)) {

		/* Merge entries according to priority */

		CONFIG_LIST(cp, adv->adv_config->conflist_list) {
		    if (!list[cp->config_type] ||
			list[cp->config_type]->config_priority < prio) {
			(list[cp->config_type] = cp)->config_priority = prio;
		    }		
		} CONFIG_LIST_END(cp, adv->adv_config->conflist_list) ;
	    }
	}
    } ADV_LIST_END(policy, adv) ;

    if (!entries) {
	if (size) {
	    config_resolv_free(list, size);
	}
	list = (config_entry **) 0;
    }
    
    return list;
}


config_entry **
config_resolv_ifl __PF3(policy, adv_entry *,
			ifl, if_link *,
			size, int)
{
    int entries = 0;
    register adv_entry *adv;
    config_entry **list;

    /* Allocate a block */
    list = (config_entry **) task_block_malloc((size_t) (size * sizeof (config_entry)));

    ADV_LIST(policy, adv) {
	int prio = 0;
	
	switch (adv->adv_flag & ADVF_TYPE) {
		
	case ADVFT_ANY:
	    prio = CONFIG_PRIO_ANY;
	    break;

	case ADVFT_IFN:
	    if (ifl->ifl_nameent == adv->adv_ifn) {
		prio = CONFIG_PRIO_NAME;
	    } else if (ifl->ifl_nameent_wild == adv->adv_ifn) {
		prio = CONFIG_PRIO_WILD;
	    }
	    break;

	case ADVFT_IFAE:
	    if (ifl->ifl_addrent == adv->adv_ifae) {
		prio = CONFIG_PRIO_ADDR;
	    }
	    break;

	default:
	    assert(FALSE);
	}

	if (prio) {
	    register config_entry *cp;

	    /* We have a match */
	    entries++;

	    if (BIT_COMPARE(adv->adv_flag, ADVFO_TYPE, ADVFOT_CONFIG)) {

		/* Merge entries according to priority */

		CONFIG_LIST(cp, adv->adv_config->conflist_list) {
		    if (!list[cp->config_type] ||
			list[cp->config_type]->config_priority < prio) {
			(list[cp->config_type] = cp)->config_priority = prio;
		    }		
		} CONFIG_LIST_END(cp, adv->adv_config->conflist_list) ;
	    }
	}
    } ADV_LIST_END(policy, adv) ;

    if (!entries) {
	config_resolv_free(list, size);
	list = (config_entry **) 0;
    }
    
    return list;
}


void
policy_family_init __PF0(void)
{
    adv_block_index = task_block_init(sizeof (adv_entry),
				      "adv_entry");
    dest_mask_internal_index = task_block_init(sizeof(dest_mask_internal),
					       "dest_mask_internal");
    gw_block_index = task_block_init(sizeof (gw_entry), "gw_entry");
    config_entry_index = task_block_init(sizeof (config_entry), "config_entry");
    config_list_index = task_block_init(sizeof (config_list), "config_list");
}
