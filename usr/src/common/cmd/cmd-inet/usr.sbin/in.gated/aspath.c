#ident	"@(#)aspath.c	1.3"
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

/*
 * This stuff supports the decoding and manipulation of AS path attribute
 * data in the form BGP version 2/3 and, separately, BGP version 4 likes it.
 * It is implemented as a separate module since it is expected that other
 * protocols (e.g. IS-IS?) may be pressed into service to carry this gunk
 * between border gateways.
 *
 * The code maintains the (variable length) path attributes in a number
 * of fixed sized structures which are obtained using the task_block*()
 * facility.  For attributes to large to fit in these we malloc() enough
 * space and free when we no longer need them.  There is code here to read
 * raw attribute data into structures and return a pointer to that structure.
 * The code also maintains this stuff so that only one structure exists for
 * each distinct set of AS path attributes, this allowing users to test for
 * attribute equality by comparing pointers.
 *
 * The code is intended to be fully general, but is greatly optimized for
 * situations where the longest attribute list doesn't exceed the size of
 * the largest fixed-size structure we allocate (if you get more than a
 * few that do, change the code to allocate larger ones) and where there
 * are either no AS path attributes we don't understand, or where the
 * unknown attributes are sorted into ascending order when they arrive.
 */

#include "include.h"
#ifdef PROTO_INET
#include "inet.h"
#ifdef PROTO_OSPF
#include "ospf.h"
#endif /* PROTO_OSPF */
#ifdef PROTO_BGP
#include "bgp.h"
#endif /* PROTO_BGP */
#endif /* PROTO_INET */


/*
 * Memory allocation.  We allocate structures with 32 and 64 byte
 * data areas.  Note our use of the data area is sloppy, this to
 * avoid having to copy anything.
 */
static struct path_size_list {
    int len;		/* length of data area */
    block_t block_indx;	/* block index for this size */
    int num_allocated;
} path_size_list[] = {
	{ 32,	0,	0 },
	{ 64,	0,	0 }
};

#define	NUMPATHSIZES	(sizeof (path_size_list)/sizeof (struct path_size_list))

/*
 * Statistics for tuning.
 */
int path_num_mallocs;
int path_num_frees;
int path_max_size;

/*
 * We hash the path attributes into buckets to avoid long searches
 * and many comparisons.  We use the low order bits of the most distant
 * AS in the path for this.
 *
 * N.B. PATHHASHSIZE is set in aspath.h
 */
#ifndef PATHHASHSIZE
#define	PATHHASHSIZE	128
#endif	/* PATHHASHSIZE */
#define	PATHHASHMASK	(PATHHASHSIZE-1)
/* N.B. watch which bits in path_flags are included in hash */
#define	PATHHASH(asp) \
    (((((asp)->path_len > 0) ? \
      ((*(PATH_PTR((asp))+(asp)->path_len-1)) ^ (asp)->path_len) : 0) \
      ^ ((asp)->path_origin << 4) ^ ((asp)->path_flags << 5) \
      ^ (asp)->path_attr_len) & PATHHASHMASK)

static as_path *path_list[PATHHASHSIZE] = { 0 };
static int path_list_members = 0;	/* number of paths we know about */

/*
 * Macros for scanning the AS paths we have.
 */
#define	ASPATH_LIST(asp) \
    do { \
	int Xi; \
	as_path *Xasp; \
	for (Xi = 0; Xi < PATHHASHSIZE; Xi++) { \
	    if (!(Xasp = path_list[Xi])) { \
		continue; \
	    } \
	    do { \
		(asp) = Xasp; \
		Xasp = Xasp->path_next; do

#define	ASPATH_LIST_END(asp) \
		while (0); \
	    } while (Xasp); \
	} \
    } while (0)


/*
 * Local AS numbers.  We keep track of this in an array, since the number
 * of local AS numbers we keep track of is rather small.  We copy the
 * array at reconfig time so that we can determine if anything has changed
 * or not.
 */
static as_local as_local_info[PATH_N_LOCAL_AS];
static int n_local_as = 0;
static int n_as_all = 0;

static as_local old_as_local_info[PATH_N_LOCAL_AS];
static int old_n_local_as;

#define	AS_LOCAL_FINDBIT(as, i) \
    do { \
	register int Xi; \
	register as_t Xas = (as); \
	for (Xi = 0; Xi < n_local_as; Xi++) { \
	    if (as_local_info[Xi].asl_as == Xas) { \
		break; \
	    } \
	} \
	assert(Xi < n_local_as); \
	(i) = AS_LOCAL_BIT(Xi); \
    } while (0)


/*
 * Structures that are on the hash list are identified by an integer
 * path ID.  This is defined here.
 */
static u_long lastpathid = 0;

/*
 * Free a path structure.  We do this a fair bit.
 */
#define	PATH_FREE_PATH(pathp) \
    do { \
	if ((pathp)->path_size == 0) { \
	    path_num_frees++; \
	    task_mem_free(path_task, (caddr_t) (pathp)); \
	} else { \
	    register struct path_size_list *pslp; \
	    pslp = &path_size_list[(pathp)->path_size-1]; \
	    pslp->num_allocated--; \
	    task_block_free(pslp->block_indx, (void_t) (pathp)); \
	} \
    } while(0)

/*
 * Find a path structure in the hash list.  The asp argument ends up
 * set to NULL if the path isn't found.
 */
#define	PATH_FIND_PATH(pathp, asp) \
    do { \
	for ((asp) = path_list[(pathp)->path_hash]; \
	  (asp); (asp) = (asp)->path_next) { \
	    if (PATH_SAME((pathp), (asp))) { \
		break; \
	    } \
	} \
    } while (0)

/*
 * Add a path to the hash structure
 */
#define	PATH_ADD(pathp) \
    do { \
	register as_path *Xasp = (pathp); \
	Xasp->path_id = ++lastpathid; \
	Xasp->path_next = path_list[Xasp->path_hash]; \
	path_list[Xasp->path_hash] = Xasp; \
	path_list_members++; \
    } while (0)

/*
 * Remove a path from the hash structure
 */
#define	PATH_REMOVE(pathp) \
    do { \
	register as_path *Xasp = (pathp); \
	register as_path *Xasp_find = path_list[Xasp->path_hash]; \
	if (Xasp == Xasp_find) { \
	    path_list[Xasp->path_hash] = Xasp->path_next; \
	} else { \
	    while (Xasp != Xasp_find->path_next) { \
		Xasp_find = Xasp_find->path_next; \
	    } \
	    Xasp_find->path_next = Xasp->path_next; \
	} \
	path_list_members--; \
	if (Xasp->path_id == lastpathid) { \
	    lastpathid--; \
	} \
	Xasp->path_id = 0; \
    } while (0)



/*
 * Maximum size an AS path could be
 */
#ifdef	BGPHMAXPACKETSIZE
#define	ASPATHMAXSIZE	(BGPHMAXPACKETSIZE / 2)
#else	/* BGPHMAXPACKETSIZE */
#define	ASPATHMAXSIZE	2048
#endif	/* BGPHMAXPACKETSIZE */

/*
 * The task pointer is used as an indicator that we are already
 * initialized.
 */
static task *path_task = (task *) 0;

/*
 * We keep track of received attributes in a bit array so we can
 * figure out if something is sent twice.  These are in aid of
 * that end.
 */
#define	ATTR_BITARRAY_SIZE	(256 / NBBY)
#define	ATTR_SET(attr, a)	((a)[(attr) / NBBY] |= 1 << ((attr) % NBBY))
#define ATTR_ISSET(attr, a)	((a)[(attr) / NBBY] & 1 << ((attr) % NBBY))

/*
 * Gunk for pretty printing path attributes
 */
bits path_Orgs[] = {
	{ PATH_ORG_IGP,	"IGP" },
	{ PATH_ORG_EGP,	"EGP" },
	{ PATH_ORG_XX,	"Incomplete" },
	{ 0 }
};


/*
 * Block pointer for allocating aggregate entries.
 */
static block_t as_path_list_block = (block_t) 0;


/*
 * Some external stuff we may use someday.
 */
trace *path_trace_options = (trace *) 0;	/* trace flags from parser(?) */

/*
 * Protocol-specific operations.  Only OSPF requires these for now.
 */
#ifdef PROTO_OSPF
#define	PATH_GET_OSPF_TAG(rt)	(rt)->rt_tag
#endif /* PROTO_OSPF */



/*
 * path_id_comp - called by qsort() when sorting paths by ID
 */
static int
aspath_id_comp __PF2(v1, const VOID_T,
		     v2, const VOID_T)
{
    as_path *pa1 = *((as_path * const *) v1);
    as_path *pa2 = *((as_path * const *) v2);

    if (pa1->path_id < pa2->path_id) {
	return -1;
    } else if (pa1->path_id > pa2->path_id) {
	return 1;
    } else {
	return 0;
    }
}


/*
 * path_dump_all - dump AS path info
 */
static void
aspath_dump_all __PF2(tp, task *,
		      fd, FILE *)
{
    register int i;
    register u_int nb;
    register byte *cp;
    register int as;
    register u_short *seg;
    register int nseg;
    register as_path *pa;
    as_path **p_list;
    int aggr_printed = 0;
    size_t needmem;

    /*
     * If no path data present, print this.  Else acquire enough
     * memory to sort the list of attributes by path ID
     */
    if (path_list_members == 0) {
	fprintf(fd, "\tNo path attributes in data base\n");
	return;
    }

    needmem = (size_t)(path_list_members * sizeof(as_path *));
    if (needmem <= (size_t) task_send_buffer_len) {
	p_list = (as_path **)task_send_buffer;
    } else {
        p_list = (as_path **) task_mem_malloc(tp, needmem);
    }

    nb = 0;
    for (i = 0; i < PATHHASHSIZE; i++) {
	pa = path_list[i];
	while (pa) {
	    if (nb < path_list_members) {
		p_list[nb] = pa;
	    }
	    nb++;
	    pa = pa->path_next;
	}
    }

    assert(nb == path_list_members);

    qsort((caddr_t) p_list, nb, sizeof (pa), aspath_id_comp);

    /*
     * We now have the list sorted.  Run through and print the vital
     * statistics for each member.
     */
    for (nb = 0; nb < path_list_members; nb++) {
	pa = p_list[nb];
	fprintf(fd,
		"\tId %lu\tRefs %u Hash %u  Lengths: Path %u, Seg %u, Attr %u, Aggr %u, Alloc'd %u\n\t",
		pa->path_id,
		pa->path_refcount,
		pa->path_hash,
		pa->path_len,
		pa->path_seg_len,
		pa->path_attr_len,
		pa->path_aggr_len,
		(pa->path_size > 0)
		? path_size_list[pa->path_size - 1].len
		: (pa->path_len + pa->path_seg_len + pa->path_attr_len));
	cp = PATH_PTR(pa);
	fprintf(fd, "\tPath: ");
	if (pa->path_local_as) {
	    register int first = 1;

	    for (i = 0; i < n_as_all; i++) {
		if (AS_LOCAL_TEST(pa->path_local_as, i)) {
		    (void) fprintf(fd, "%c%u",
				   (first ? '(' : ' '),
				   as_local_info[i].asl_as);
		    first = 0;
		}
	    }
	    (void) fprintf(fd, ") ");
	}
	seg = PATH_SEG_PTR(pa);
	nseg = pa->path_seg_len/PA4_LEN_SEGMENT;
	while (nseg-- > 0) {
	    i = PATH_SEG_LEN(*seg);
	    if (PATH_ISSET(*seg)) {
		fprintf(fd, "[");
	    }
	    while (i-- > 0) {
	        as = (int)(*cp++) << 8;
	        as += (int)(*cp++);
		if (i > 0) {
		    fprintf(fd, "%u ", as);
		} else {
		    fprintf(fd, "%u", as);
		}
	    }
	    if (PATH_ISSET(*seg)) {
		fprintf(fd, "] ");
	    } else {
		fprintf(fd, " ");
	    }
	    seg++;
	}
	switch (pa->path_origin) {
	case PATH_ORG_IGP:
	    fprintf(fd, "IGP");
	    break;

	case PATH_ORG_EGP:
	    fprintf(fd, "EGP");
	    break;

	case PATH_ORG_XX:
	    fprintf(fd, "Incomplete");
	    break;

	default:
	    fprintf(fd, "<0x%02x>",
		    pa->path_origin);
	    break;
	}

	if (pa->path_aggr_len != 0) {
	    byte *bp = PATH_ATTR_PTR(pa) + 3;

	    if (PATH_ATTR_LEN(2) == pa->path_aggr_len
	      || PATH_ATTR_LEN(6) == pa->path_aggr_len) {
		aggr_printed = 1;
		PATH_GET_AS(as, bp);
		if (PATH_ATTR_LEN(2) == pa->path_aggr_len) {
		    fprintf(fd, " Agg: %u", as);
		} else {
		    sockaddr_un addr;
		    byte *ap;

		    sockclear_in(&addr);
		    ap = (byte *) &sock2ip(&addr);
		    *ap++ = *bp++;
		    *ap++ = *bp++;
		    *ap++ = *bp++;
		    *ap++ = *bp++;
		    fprintf(fd, " Agg: %u %A", as, &addr);
		}
	    }
	}

	if (BIT_TEST(pa->path_flags,
	  PATH_FLAG_ASLOOP|PATH_FLAG_ATOMIC_AGG|PATH_FLAG_LOCAL_AGG)) {
	    i = 0;
	    fprintf(fd, " <");
	    if (BIT_TEST(pa->path_flags, PATH_FLAG_ATOMIC_AGG)) {
		fprintf(fd, "Atomic");
		i = 1;
	    }
	    if (BIT_TEST(pa->path_flags, PATH_FLAG_LOCAL_AGG)) {
		if (i) {
		    fprintf(fd, ",");
		}
		fprintf(fd, "Local");
		i = 1;
	    }
	    if (BIT_TEST(pa->path_flags, PATH_FLAG_ASLOOP)) {
		if (i) {
		    fprintf(fd, ",");
		}
		fprintf(fd, "ASLoop");
		i = 1;
	    }
	    fprintf(fd, ">");
	}
	if (pa->path_looped) {
	    register int first = 1;

	    for (i = 0; i < n_local_as; i++) {
		if (AS_LOCAL_TEST(pa->path_looped, i)) {
		    (void) fprintf(fd, "%s%u",
				   (first ? " (Looped: " : " "),
				   as_local_info[i].asl_as);
		    first = 0;
		}
	    }
	    (void) fprintf(fd, ") ");
	}
	fprintf(fd, "\n");

	cp += pa->path_seg_len;
	if (pa->path_aggr_len != 0 && aggr_printed) {
	    cp += pa->path_aggr_len;
	}
	while (cp < (PATH_ATTR_PTR(pa) + pa->path_attr_len)) {
	    u_int flags, code;
	    size_t len;

	    GET_PATH_ATTR(flags, code, len, cp);
	    fprintf(fd, "\t\tAttr flags %02x code %02x:",
		    flags,
		    code);
	    while (len-- > 0)
		fprintf(fd, " %02x", *cp++);
	    fprintf(fd, "\n");
	}
    }

    /*
     * Shit, glad that's done.  Free up the list space and return
     */
    if (needmem > (size_t) task_send_buffer_len) {
	(void) task_mem_free(tp, (caddr_t) p_list);
    }
}


/*
 * path_dump - pretty print an AS path for the sake of someone else's
 *	       dump routine
 */
void
aspath_dump __PF4(fd, FILE *,
		  pa, as_path *,
		  str, const char *,
		  endstr, const char *)
{

    (void) fprintf(fd, "%sAS Path: ", str);

    if (pa->path_local_as) {
	register int i;
	register int first = 1;

	for (i = 0; i < n_as_all; i++) {
	    if (AS_LOCAL_TEST(pa->path_local_as, i)) {
		(void) fprintf(fd, "%c%u",
			       (first ? '(' : ' '),
			       as_local_info[i].asl_as);
		first = 0;
	    }
	}
	(void) fprintf(fd, ") ");
    }

    if (pa->path_len) {
	register byte *cp;
	register int i, as;
	register u_short *seg;
	register int nseg;

	cp = PATH_PTR(pa);
	seg = PATH_SEG_PTR(pa);
	nseg = pa->path_seg_len/PA4_LEN_SEGMENT;
	while (nseg-- > 0) {
	    i = PATH_SEG_LEN(*seg);
	    if (PATH_ISSET(*seg)) {
		(void) fprintf(fd, "[");
	    }
	    while (i-- > 0) {
		as = (int)(*cp++) << 8;
		as += (int)(*cp++);
		if (i > 0) {
		    (void) fprintf(fd, "%u ", as);
		} else {
		    (void) fprintf(fd, "%u", as);
		}
	    }
	    if (PATH_ISSET(*seg)) {
		(void) fprintf(fd, "] ");
	    } else {
		(void) fprintf(fd, " ");
	    }
	    seg++;
	}
    }

    switch (pa->path_origin) {
    case PATH_ORG_IGP:
	fprintf(fd, "IGP (Id %lu)%s",
		pa->path_id,
		endstr);
	break;

    case PATH_ORG_EGP:
	fprintf(fd, "EGP (Id %lu)%s",
		pa->path_id,
		endstr);
	break;

    case PATH_ORG_XX:
	fprintf(fd, "Incomplete (Id %lu)%s",
		pa->path_id,
		endstr);
	break;

    default:
	fprintf(fd, "%02x (Id %lu)%s",
		pa->path_origin,
		pa->path_id,
		endstr);
	break;
    }
}



/*
 * path_trace - write out raw AS path data when told to
 */
void
aspath_trace __PF5(trp, trace *,
		   comment, const char *,
		   version, int,
		   bufp, byte *,
		   buflen, int)
{
    register byte *cp;
    register byte *end;
    register int i;
    register u_int code;
    register u_int flags;
    register size_t len;
    u_long metric;
    byte *bpattr;
    int defprint;
    sockaddr_un addr;

    sockclear_in(&addr);
    cp = bufp;
    end = cp + buflen;

    /*
     * Check version for sanity.
     */
    assert(PATH_OKAY_VERSION(version));

    while (cp < end) {
	if (PATH_ATTR_MINLEN(*cp) > (end - cp)) {
	    trace_only_tf(trp,
			  0,
			  ("%sshort attribute: flags 0x%02x, %d data bytes left",
			   comment,
			   (int) *cp,
			   (end - cp)));
	    return;
	}

	GET_PATH_ATTR(flags, code, len, cp);
	if ((len + cp) > end) {
	    trace_only_tf(trp,
			  0,
			  ("%sshort attribute data: flags 0X%02x, code %d, length %d (> %d)",
			   comment,
			   flags,
			   code,
			   len,
			   (end - cp)));
	    return;
	}

	defprint = 1;
	switch (code) {
	case PA_TYPE_ORIGIN:	/* also PA4_TYPE_ORIGIN */
	    if (len != 1) {
		break;
	    }
	    i = (int)*cp++;
	    trace_only_tf(trp,
	    		  0,
			  ("%sflags 0x%02x code Origin(%d): %s",
			   comment,
			   flags,
			   code,
			   trace_state(path_Orgs, i)));
	    defprint = 0;
	    break;

	case PA_TYPE_ASPATH:	/* also PA4_TYPE_ASPATH */
	    if (len & 0x01) {
		break;
	    }
	    tracef("%sflags 0x%02x code ASPath(%d):",
		   comment, flags, code);
	    len >>= 1;
	    defprint = 0;
	    if (len == 0) {
		trace_only_tf(trp,
			      0,
			      (" <null>"));
		break;
	    }

	    if (version == PATH_VERSION_4) {
		register u_int seg_type;
		register size_t seg_len;

		while (len != 0) {
		    seg_type = (u_int)(*cp++);
		    seg_len = (size_t)(*cp++);
		    len--;
		    if ((seg_type != PA_PATH_SET && seg_type != PA_PATH_SEQ)
		      || seg_len > len) {
			tracef(" (0x%02x 0x%02x", seg_type, seg_len);
			while (len != 0) {
			    u_int tmp1 = (u_int)(*cp++);
			    u_int tmp2 = (u_int)(*cp++);
			    tracef(" 0x%02x 0x%02x", tmp1, tmp2);
			    len--;
			}
			tracef(")");
		    } else if (seg_len == 0) {
			if (seg_type == PA_PATH_SET) {
			    tracef(" []");
			} else {
			    tracef(" -");
			}
		    } else {
			len -= seg_len;
			i = (*cp++) << 8;
			i |= *cp++;
			if (seg_type == PA_PATH_SET) {
			    tracef(" [%d", i);
			} else {
			    tracef(" %d", i);
			}
			while (--seg_len > 0) {
			    i = (*cp++) << 8;
			    i |= *cp++;
			    tracef(" %d", i);
			}
			if (seg_type == PA_PATH_SET) {
			    tracef("]");
			}
		    }
		}
	    } else {
	        while (len-- > 0) {
		    i = (*cp++) << 8;
		    i |= *cp++;
		    tracef(" %d", i);
	        }
	    }
	    trace_only_tf(trp,
			  0,
			  (NULL));
	    break;

	case PA_TYPE_NEXTHOP:	/* also PA4_TYPE_NEXTHOP */
	    if (len != PA_LEN_NEXTHOP) {
		break;
	    }

	    bpattr = (byte *) &sock2ip(&addr);
	    *bpattr++ = *cp++;
	    *bpattr++ = *cp++;
	    *bpattr++ = *cp++;
	    *bpattr++ = *cp++;
	    trace_only_tf(trp,
			  0,
			  ("%sflags 0x%02x code NextHop(%d): %A",
			   comment,
			   flags,
			   code,
			   &addr));
	    defprint = 0;
	    break;

	case PA_TYPE_UNREACH:	/* also PA4_TYPE_METRIC, the stupid dorks */
	    if (version == PATH_VERSION_4) {
		if (len != 4) {
		    break;
		}
		metric = ((u_int32) *cp++) << 24;
		metric |= ((u_int32) *cp++) << 16;
		metric |= ((u_int32) *cp++) << 8;
		metric |= *cp++;
		trace_only_tf(trp,
			      0,
			      ("%sflags 0x%02x code MultiExitDisc(%d): %ul",
			       comment,
			       flags,
			       code,
			       metric));
	    } else {
		if (len != 0) {
		    break;
		}
		trace_only_tf(trp,
			      0,
			      ("%sflags 0x%02x code Unreachable(%d)",
			       comment,
			       flags,
			       code));
	    }
	    defprint = 0;
	    break;

	case PA_TYPE_METRIC:	/* also PA4_TYPE_LOCALPREF */
	    if (version == PATH_VERSION_4) {
		if (len != 4) {
		    break;
		}
		metric = ((u_int32) *cp++) << 24;
		metric |= ((u_int32) *cp++) << 16;
		metric |= ((u_int32) *cp++) << 8;
		metric |= *cp++;
		trace_only_tf(trp,
			      0,
			      ("%sflags 0x%02x code LocalPref(%d): %lu",
			       comment,
			       flags,
			       code,
			       metric));
	    } else {
		if (len != 2) {
		    break;
		}
		i = (*cp++) << 8;
		i |= *cp++;
		trace_only_tf(trp,
			      0,
			      ("%sflags 0x%02x code Metric(%d): %d",
			       comment,
			       flags,
			       code,
			       i));
	    }
	    defprint = 0;
	    break;

	case PA4_TYPE_ATOMICAGG:
	    if (version == PATH_VERSION_2OR3 || len != 0) {
		break;
	    }
	    trace_only_tf(trp,
			  0,
			  ("%sflags 0x%02x code Atomic_Aggregate(%d)",
			  comment,
			  flags,
			  code));
	    defprint = 0;
	    break;

	case PA4_TYPE_AGGREGATOR:
	    if (len != 2 && len != 6) {
		break;
	    }
	    i = (*cp++) << 8;
	    i |= *cp++;
	    if (len == 6) {
		bpattr = (byte *) &sock2ip(&addr);
		*bpattr++ = *cp++;
		*bpattr++ = *cp++;
		*bpattr++ = *cp++;
		*bpattr++ = *cp++;
		trace_only_tf(trp,
			      0,
			      ("%sflags 0x%02x code Aggregator(%d): %d %A",
			       comment,
			       flags,
			       code,
			       i,
			       &addr));
	    } else {
		trace_only_tf(trp,
			      0,
			      ("%sflags 0x%02x code Aggregator(%d): %d",
			       comment,
			       flags,
			       code,
			       i));
	    }
	    defprint = 0;
	    break;

	default:
	    break;
	}

	if (defprint) {
	    tracef("%sflags 0x%02x code %d length %d:",
		   comment,
		   flags,
		   code,
		   len);
	    while (len-- > 0) {
		tracef(" %02x", (int)*cp++);
	    }
	    trace_only_tf(trp,
			  0,
			  (NULL));
	}
    }
}


/*
 * aslocal_set - set a local AS number, along with his loop count
 */
void
aslocal_set __PF2(as, as_t,
		  loops, size_t)
{
    register int i;

    for (i = 0; i < n_local_as; i++) {
	if (as_local_info[i].asl_as == as) {
	    assert(loops == 0 || loops == as_local_info[i].asl_loop);
	    return;
	}
	if (as_local_info[i].asl_as > as) {
	    break;
	}
    }

    assert(n_local_as < PATH_N_LOCAL_AS);
    if (i < n_local_as) {
	register int j = n_local_as;

	while (j-- > i) {
	    as_local_info[j+1] = as_local_info[j];	/* Struct copy */
	}
    }
    as_local_info[i].asl_as = as;
    as_local_info[i].asl_as_net = (as_t) htons((u_int16)(as));
    if (loops == 0) {
	as_local_info[i].asl_loop = 1;
    } else {
	as_local_info[i].asl_loop = (u_int16)loops;
    }
    n_local_as++;
    n_as_all = n_local_as;
}


/*
 * aslocal_bit - fetch the bit number for a local AS
 */
int
aslocal_bit __PF1(as, as_t)
{
    register int i;

    for (i = 0; i < n_local_as; i++) {
	if (as_local_info[i].asl_as == as) {
	    return i;
	}
    }

    assert(FALSE);
    return (-1);
}


/*
 * aslocal_cleanup - prepare for a reconfig.
 */
static void
aslocal_cleanup __PF1(tp, task *)
{
    /*
     * Copy the current local AS list into the old storage
     */
    if (n_local_as > 0) {
	bcopy((void_t)as_local_info,
	      (void_t)old_as_local_info,
	      (sizeof(as_local_info[0]) * n_local_as));
    }
    old_n_local_as = n_local_as;
    n_local_as = 0;
    n_as_all = 0;

    /*
     * XXX maybe shouldn't be here, but what the heck.
     */
    trace_freeup(path_task->task_trace);
}


/*
 * aslocal_reinit - fix up paths in database after a reinit
 */
static void
aslocal_reinit __PF0(void)
{
    register int i, j;
    int changed_bits = 0;
    int changed_loop = 0;

    if (path_list_members == 0 || !old_n_local_as) {
	/*
	 * No problem, return.
	 */
	return;
    }

    /*
     * Scan through the list looking to see if anything changed.
     * Copy all old guys who aren't in the new list to the end of
     * the new list.  Record the bit numbers the old guys have been
     * assigned.
     */
    i = 0;
    for (j = 0; j < old_n_local_as; ) {
	if (i == n_local_as
	    || old_as_local_info[j].asl_as < as_local_info[i].asl_as) {
	    /*
	     * Must copy the old info to the end of the new list.
	     */
	    assert(n_as_all < PATH_N_LOCAL_AS);
	    as_local_info[n_as_all] = old_as_local_info[j];
	    old_as_local_info[j++].asl_found = n_as_all++;
	    changed_bits++;
	} else if (as_local_info[i].asl_as == old_as_local_info[j].asl_as) {
	    /*
	     * Found a match.  Record the new bit number in the old structure.
	     */
	    old_as_local_info[j].asl_found = i;
	    if (i != j) {
		changed_bits++;
	    }
	    if (as_local_info[i].asl_loop != old_as_local_info[j].asl_loop) {
		changed_loop++;
	    }
	    i++, j++;
	} else {
	    /*
	     * New AS we didn't have before.
	     */
	    changed_loop++;
	    i++;
	}
    }

    /*
     * If we've had changes we'll need to go through the AS paths we
     * know about making adjustments.
     */
    if (changed_bits || changed_loop) {
	register as_path *asp;

	if (changed_loop) {
	    for (i = 0; i < n_local_as; i++) {
		as_local_info[i].asl_found = 0;
	    }
	}

	ASPATH_LIST(asp) {
	    register aslocal_t local_as = 0;
	    register aslocal_t loop = 0;

	    for (j = 0; j < old_n_local_as; j++) {
		i = old_as_local_info[j].asl_found;
		if (AS_LOCAL_TEST(asp->path_local_as, j)) {
		    AS_LOCAL_SET(local_as, i);
		}
		if (!changed_loop && i < n_local_as
		  && AS_LOCAL_TEST(asp->path_looped, j)) {
		    AS_LOCAL_SET(loop, i);
		}
	    }
	    asp->path_local_as = local_as;

	    if (changed_loop && asp->path_len > 0) {
		register u_short *ap = PATH_SHORT_PTR(asp);

		j = asp->path_len / sizeof(u_short);
		while (j--) {
		    local_as = *ap++;
		    for (i = 0; i < n_local_as; i++) {
			if (local_as == as_local_info[i].asl_as_net) {
			    as_local_info[i].asl_found++;
			    break;
			}
		    }
		}

		for (i = 0; i < n_local_as; i++) {
		    if (as_local_info[i].asl_found
		      >= as_local_info[i].asl_loop) {
			AS_LOCAL_SET(loop, i);
		    }
		    as_local_info[i].asl_found = 0;
		}
	    }
	    asp->path_looped = loop;
	} ASPATH_LIST_END(asp);
    }
}



/*
 * path_init - initialize the path task so that dumps print path info
 */
void
aspath_init __PF0(void)
{
    trace_inherit(path_task->task_trace, path_trace_options);
    if (path_list_members > 0) {
	aslocal_reinit();
    }
}


/*
 * aspath_family_init - initialize the aspath family module
 */
void
aspath_family_init __PF0(void)
{
    int i;

    /*
     * The path task is needed only for dumps of the AS path
     * data base.  Allocate the task and list the dump routine.
     */
    path_task = task_alloc("ASPaths",
			   TASKPRI_EXTPROTO,
			   path_trace_options);
    task_set_dump(path_task, aspath_dump_all);
    task_set_cleanup(path_task, aslocal_cleanup);
    /* XXX - reinit to reset paths at init time? */
    if (!task_create(path_task)) {
	task_quit(EINVAL);
    }
    for (i = 0; i < NUMPATHSIZES; i++) {
	if (path_size_list[i].block_indx == NULL) {
	    path_size_list[i].block_indx = 
	      task_block_init((size_t) (path_size_list[i].len + sizeof (as_path)), "as_path");
	}
    }
    init_asmatch_alloc();
}


/*
 * path_alloc - allocate a path structure of at least the specified length
 */
as_path *
aspath_alloc __PF1(length, size_t)
{
    register as_path *asp;
    register int ps;

    for (ps = 0; ps < NUMPATHSIZES; ps++) {
	if (path_size_list[ps].len >= length) {
	    /*
	     * Got one of the standard sizes.  Get it and fill
	     * the size in.
	     */
	    asp = (as_path *) task_block_alloc(path_size_list[ps].block_indx);
	    path_size_list[ps].num_allocated++;
	    asp->path_size = ps + 1;
	    return asp;
	}
    }

    /*
     * Just allocate a big one which is the correct size.
     */
    asp = (as_path *) task_mem_calloc(path_task,
				      1,
				      (sizeof (as_path) + length));
    path_num_mallocs++;
    return asp;
}


/*
 * aspath_free - like aspath_unlink, except that it will deal with
 *		 a path which has just been aspath_alloc()'d and
 *		 not linked to the hash list.
 */
void
aspath_free __PF1(pathp, as_path *)
{
    if (pathp->path_refcount == 0) {
	assert(pathp->path_id == 0);
    } else {
	if (--(pathp->path_refcount) > 0) {
	    return;
	}
	PATH_REMOVE(pathp);
    }

    PATH_FREE_PATH(pathp);
}


/*
 * path_unlink - free a path structure which may have be obtained from
 *	       the task_block stuff or may have been malloc'd.
 */
void
aspath_unlink __PF1(pathp, register as_path *)
{
    assert(pathp->path_refcount > 0);
    if (--(pathp->path_refcount) > 0) {
	return;
    }

    /*
     * Remove the structure from the hash list
     */
    PATH_REMOVE(pathp);

    /*
     * Now free the path structure.
     */
    PATH_FREE_PATH(pathp);
}


/*
 * path_find - search for an existing path in the table which matches
 *   the argument.  If found, free the argument path and return the
 *   linked one instead.  A path which has been found by path_find()
 *   should always be the one returned by path_rt_link() or path_rt_build().
 */
as_path *
aspath_find __PF1(pathp, register as_path *)
{
    register as_path *asp;

    if (pathp->path_refcount == 0) {
	PATH_FIND_PATH(pathp, asp);

	/*
	 * If one was found, free the old one and return this instead.
	 */
	if (asp) {
	    PATH_FREE_PATH(pathp);
	    ASPATH_ALLOC(asp);
	    return (asp);
	}
	PATH_ADD(pathp);
    }

    /*
     * Alloc and return the path we were called with.
     */
    ASPATH_ALLOC(pathp);
    return (pathp);
}



#ifdef notdef
static void
aspath_rt_link __PF2(pl_rt, rt_entry *,
		     pathp, register as_path *)
{
    /* Release the previous path */
    if (pl_rt->rt_aspath) {
	ASPATH_FREE(pl_rt->rt_aspath);
    }

    assert(pathp->path_refcount);
    pl_rt->rt_aspath = pathp;
    ASPATH_ALLOC(pathp);
}
#endif

#define	aspath_rt_link(pl_rt, pathp) \
    do { \
	register rt_entry *Xrt = (pl_rt); \
	register as_path *Xasp_new  = (pathp); \
	if (Xrt->rt_aspath) { \
	    ASPATH_FREE(Xrt->rt_aspath); \
	} \
	assert(Xasp_new->path_refcount); \
	Xrt->rt_aspath = Xasp_new; \
    } while (0)


void
aspath_rt_free __PF1(rt, rt_entry *)
{
    /* Release the path */
    if (rt->rt_aspath) {
	ASPATH_FREE(rt->rt_aspath);
	rt->rt_aspath = (as_path *) 0;
    }

#if	defined(PROTO_BGP) && defined(PROTO_OSPF)
    if (rt->rt_gwp->gw_proto == RTPROTO_BGP &&
      BIT_TEST(rt->rt_gwp->gw_flags, GWF_AUXPROTO)) {
	rt_entry *rt1;
	u_long id = ((bgpPeer *)(rt->rt_gwp->gw_task->task_data))->bgp_id;

	RT_ALLRT(rt1, rt->rt_head) {
	    /* XXX need to check for group membership someday */
	    if (BIT_TEST(rt1->rt_state, RTS_DELETE)) {
		return;
	    }
	    if (rt1->rt_gwp->gw_proto == RTPROTO_OSPF_ASE
		&& id == ORT_ADVRTR(rt1)) {
		/* Null path */
		assert(rt1->rt_aspath);
		(void) rt_change_aspath(rt1,
					rt1->rt_metric,
					rt1->rt_metric2,
					rt1->rt_tag,
					rt1->rt_preference,
					rt1->rt_preference2,
					rt1->rt_n_gw,
					rt1->rt_routers,
					(as_path *) 0);
		return;
	    }
	} RT_ALLRT_END(rt1, rt->rt_head) ;
    }
#endif	/* defined(PROTO_BGP) && defined(PROTO_OSPF) */
}


/*
 * path_insert_aspath - insert the AS path in front of the attributes
 */
static void
aspath_insert_aspath __PF1(asp, as_path *)
{
    register byte *bpi, *bpo;
    register int i;

    /*
     * Make space for the path list (which isn't there but which has its
     * lengths set right).  Do the copy from the end since the buffers
     * may overlap.
     */
    i = asp->path_attr_len;
    bpi = PATH_PTR(asp) + i;
    bpo = bpi + asp->path_len + asp->path_seg_len;
    while (i-- > 0) {
	*(--bpo) = *(--bpi);
    }
}


/*
 * path_insert_attr - find where to insert an attribute into the list.
 *		      Do it.
 */
static void
aspath_insert_attr __PF5(asp, as_path *,
			 newflags, u_int,
			 newcode, u_int,
			 newlen, size_t,
			 datap, byte *)
{
    register byte *bp, *bpend;
    register byte *bpattr;
    size_t len;
    u_int flags, code;

    /*
     * Find where to insert the attribute.
     */
    bp = PATH_ATTR_PTR(asp);
    bpattr = bp;
    bpend = bp + asp->path_attr_len;
    while (bp < bpend) {
	GET_PATH_ATTR(flags, code, len, bp);
	if (code > newcode) {
	    break;
	}
	bp += len;
	bpattr = bp;
    }

    /*
     * If this wasn't the end of the list, we need to make space
     * for the new attributes.  bpattr points to where the space
     * needs to be made.
     */
    if (bpattr < bpend) {
	bp = bpend + PATH_ATTR_LEN(newlen);
	while (bpend > bpattr) {
	    *(--bp) = *(--bpend);
	}
    }

    /*
     * By now, one way or another, bpend points to the place to insert
     * this.  Copy it in.
     */
    PATH_PUT_ATTR(newflags, newcode, newlen, bpend);
    bpattr = datap;
    while (bpend < bp) {
	*bpend++ = *bpattr++;
    }
    asp->path_attr_len += PATH_ATTR_LEN(newlen);
}


/*
 * path_attr - decode path attributes and return a pointer to an
 *	       attribute structure
 */
as_path *
aspath_attr __PF7(bufp, byte *,
		  buflen, size_t,
		  version, int,
		  my_as, as_t,
		  api, as_path_info *,
		  error_code, int *,
		  error_data, byte **)
{
    register byte *bp, *bpend;
    register byte *ap;
    register as_path *asp;
    register byte *bpattr;
    u_int flags;
    size_t len;
    u_int code, lastcode;
    int is_aggr = 0;
    byte received[ATTR_BITARRAY_SIZE];
    static byte missing_attr;
    as_path *nasp;

    /*
     * Local macro.  We do this alot.
     */
#define	PA_RETURN_ERROR(code, data) \
    *error_code = (code); \
    *error_data = (data); \
    goto belch

    /*
     * Check version for sanity
     */
    assert(PATH_OKAY_VERSION(version));

    /*
     * Initialize return values
     */
    api->api_flags = 0;
    lastcode = 0;

    /*
     * Initialize received array
     */
    bzero((caddr_t) received, sizeof (received));

    /*
     * Initialize buffer pointers
     */
    bp = bufp;
    bpend = bp + buflen;

    /*
     * Get an attribute structure with buflen bytes of data area.
     * Note that we will waste some of this since the total length
     * we store in here will certainly be less than buflen bytes.
     * Maybe try to do this better later on.
     */
    asp = aspath_alloc(buflen);
    ap = PATH_PTR(asp);

    /*
     * Include the AS of the caller in the local AS mask.  If we are
     * running in several ASes the AS paths should be treated as distinct.
     */
    if (my_as) {
	register int i;

	for (i = 0; i < n_local_as; i++) {
	    if (as_local_info[i].asl_as == my_as) {
		break;
	    }
	}
	assert(i < n_local_as);
	AS_LOCAL_SET(asp->path_local_as, i);
    }

    /*
     * Walk through the buffered data, picking off and processing
     * individual attributes.
     */
    while (bp < bpend) {
	/*
	 * Make sure we've got enough data for the
	 * flags-code-length triple.
	 */
	if ((bpend - bp) < PATH_ATTR_MINLEN(*bp)) {
	    /*
	     * XXX This error wasn't considered in the spec.  Use
	     * malformed for now.
	     */
	    PA_RETURN_ERROR(PA_ERR_MALFORMED, NULL) ;
	}

	/*
	 * Save the start of the data for error reporting, then
	 * decode flags-code-length.
	 */
	bpattr = bp;
	GET_PATH_ATTR(flags, code, len, bp);

	/*
	 * XXX If there is insufficient data in the packet for the
	 * length given, return an error.
	 */
	if ((bpend - bp) < len) {
	    PA_RETURN_ERROR(PA_ERR_MALFORMED, NULL) ;
	}

	/*
	 * Check to see if we got this one before.  If so this
	 * is an error.
	 */
	if (ATTR_ISSET(code, received)) {
	    PA_RETURN_ERROR(PA_ERR_MALFORMED, NULL) ;
	}
	ATTR_SET(code, received);

	/*
	 * Check for the easy flags error
	 */
	if (!BIT_TEST(flags, PA_FLAG_OPT)) {
	    if (!BIT_TEST(flags, PA_FLAG_TRANS) || BIT_TEST(flags, PA_FLAG_PARTIAL)) {
		PA_RETURN_ERROR(PA_ERR_FLAGS, bpattr) ;
	    }
	}

	/*
	 * Do code-specific processing
	 */
	switch (code) {
	case PA_TYPE_INVALID:	/* also PA4_TYPE_INVALID */
	    /*
	     * I think this is always invalid.  XXX No suitable
	     * error for this either.
	     */
	    PA_RETURN_ERROR(PA_ERR_MALFORMED, NULL) ;
	    /*NOTREACHED*/

	case PA_TYPE_ORIGIN:	/* also PA4_TYPE_ORIGIN */
	    if (len != PA_LEN_ORIGIN) {
		PA_RETURN_ERROR(PA_ERR_LENGTH, bpattr) ;
	    }
	    if ((flags & PA_FLAG_ALL) != PA_FLAG_TRANS) {
		/* Well known attribute, transitive flag only set */
		PA_RETURN_ERROR(PA_ERR_FLAGS, bpattr) ;
	    }
	    asp->path_origin = *bp++;
	    if (asp->path_origin != PATH_ORG_IGP &&
		asp->path_origin != PATH_ORG_EGP &&
		asp->path_origin != PATH_ORG_XX) {
		PA_RETURN_ERROR(PA_ERR_ORIGIN, bpattr) ;
	    }
	    break;

	case PA_TYPE_ASPATH:	/* also PA4_TYPE_ASPATH */
	    if (len & 0x1) {
		/* Odd length? */
		PA_RETURN_ERROR(PA_ERR_LENGTH, bpattr) ;
	    }
	    if ((flags & PA_FLAG_ALL) != PA_FLAG_TRANS) {
		/* Well known attribute, transitive flag only set */
		PA_RETURN_ERROR(PA_ERR_FLAGS, bpattr) ;
	    }

	    if (version == PATH_VERSION_4) {
		/*
		 * The BGP4 AS_PATH attribute is a list of
		 *    <type> <length> <list_of_ASes>
		 * We turn this into a single list of ASes
		 * (whose length in bytes is pa->path_len) followed
		 * by an array of two-byte <type,length> values
		 * (whose length in bytes is pa->path_seg_len).
		 *
		 * What we do is:
		 * (1) Scan the list once to determine the number of
		 *     type/length pairs we are likely to need to
		 *     store.  This tells us what pa->path_len and
		 *     pa->path_seg_len will be.
		 * (2) Make enough space for this by copying any previously
		 *     received path attributes (only need to do this
		 *     if pa->path_attr_len != 0).
		 * (3) Rescan the list, copying AS numbers into the
		 *     pa->path_len space and making corresponding
		 *     segment entries in the pa->path_seg_len space.
		 * (4) Sort ASes in AS_SETs into ascending order by
		 *     AS number.
		 * Note that we combine consecutive AS_SEQUENCEs into
		 * a single sequence, and try to rid ourselves of grot
		 * like zero-length AS_whatever's.
		 *
		 * Try to remember through all of this that a path with
		 * a single AS_SEQUENCE will be common, and that we should
		 * optimize the case where the incoming AS_SET is already
		 * sorted.
		 */
		register int nas, nseg;
		register byte *pp;
		register u_short *segp;
		u_short segtype, seglen;
		u_short lastsegtype;

		pp = bp;
		lastsegtype = PA_PATH_NOTSETORSEQ;
		nas = nseg = 0;
		while ((pp - bp) < len) {
		    segtype = (u_short)(*pp++);
		    seglen = (u_short)(*pp++);

		    /*
		     * Check for segment type and length
		     * sanity.  Return the infamous malformed
		     * AS_PATH if there is trouble.
		     */
		    pp += seglen << 1;
		    if ((segtype != PA_PATH_SET && segtype != PA_PATH_SEQ)
		      || (pp - bp) > len) {
			PA_RETURN_ERROR(PA_ERR_ASPATH, NULL) ;
		    }

		    /*
		     * Zero length segments can be ignored
		     */
		    if (seglen == 0) {
			continue;
		    }

		    /*
		     * Add the AS count to the total.  If this segment
		     * is a sequence and so was the last we will combine
		     * these into a single sequence later, otherwise
		     * bump the sequence count.
		     */
		    nas += seglen;
		    if (segtype != PA_PATH_SEQ || lastsegtype != PA_PATH_SEQ) {
			nseg++;
			lastsegtype = segtype;
		    }
		}

		/*
		 * So far so good.  We've checked type and length bytes
		 * for sanity so we can trust them this time.  Set the
		 * lengths of the path and segment info into the storage,
		 * make room for this much data and copy stuff in.  
		 */
		asp->path_len = nas << 1;
		asp->path_seg_len = nseg << 1;
		if (nseg == 0) {
		    break;	/* Zero length AS path, don't bother. */
		}
		ap += (asp->path_len + asp->path_seg_len);
		if (asp->path_attr_len != 0) {
		    aspath_insert_aspath(asp);
		}

		segp = PATH_SEG_PTR(asp);
		pp = PATH_PTR(asp);
		lastsegtype = PA_PATH_NOTSETORSEQ;

		while (nas > 0) {
		    segtype = (u_short)(*bp++);
		    seglen = (u_short)(*bp++);

		    /*
		     * If the segment is zero length, ignore it.
		     */
		    if (seglen == 0) {
			continue;
		    }
		    nas -= seglen;

		    /*
		     * If the segment is a sequence, make a new segment
		     * entry if the previous wasn't a sequence as well.
		     * Then copy in the ASes in the sequence.  If the
		     * segment is a set we always make a new segment
		     * entry, but ensure that the ASes in the sequence
		     * are sorted into ascending order while we copy.
		     * In the latter case, try to optimize the situation
		     * when the ASes are already sorted.
		     */
		    if (segtype == PA_PATH_SEQ) {
			if (lastsegtype == PA_PATH_SEQ) {
			    /* Add this segment's length into the total */
			    segp[-1] += (u_short)seglen;
			} else {
			    *segp++ = (u_short)seglen;
			    lastsegtype = PA_PATH_SEQ;
			}
			while (seglen-- > 0) {
			    *pp++ = *bp++;
			    *pp++ = *bp++;
			}
		    } else {
			register int oldas, newas;
			byte *segstart;

			/*
			 * XXX This is a load of shit (then again the BGP4
			 * AS path is a load of shit).  Rethink later.
			 */
			*segp++ = ((u_short)seglen) | PATH_AS_SET;
			lastsegtype = PA_PATH_SET;
			segstart = pp;
			oldas = (-1);
			while (seglen-- > 0) {
			    PATH_GET_AS(newas, bp);
			    /* loop detection will catch == case later */
			    if (newas >= oldas) {
				PATH_PUT_AS(newas, pp);
				oldas = newas;
			    } else {
				register byte *qp = pp - 2;
				int checkas;

				*pp++ = *qp;
				*pp++ = *(qp+1);
				while (segstart < qp) {
				    checkas = ((int)*(qp - 2)) << 8;
				    checkas = (int)*(qp - 1);
				    if (newas >= checkas) {
					PATH_PUT_AS(newas, qp);
					break;
				    }
				    *qp = (byte)((checkas >> 8) & 0xff);
				    *(qp+1) = (byte)(checkas & 0xff);
				    qp -= 2;
				}
				if (segstart == qp) {
				    PATH_PUT_AS(newas, qp);
				}
			    }
			}
		    }
		}
	    } else {
		/*
		 * Version 2/3 AS path.  This is a simple array
		 * of ASes which must be treated as an AS sequence.
		 * Reserve the length of the AS path array plus two
		 * for the segment descriptor (unless the AS path is
		 * zero length, in which case we don't need anything).
		 */
		asp->path_len = len;
		if (len == 0) {
		    asp->path_seg_len = 0;
		    break;
		}
		asp->path_seg_len = PA4_LEN_SEGMENT;

		if (asp->path_attr_len == 0) {	/* most frequent case */
		    register int i;

		    len >>= 1;
		    for (i = 0; i < len; i++) {
		        *ap++ = *bp++;
		        *ap++ = *bp++;
		    }
		    *PATH_SEG_PTR(asp) = (u_short) len;
		    ap += PA4_LEN_SEGMENT;
		} else {
		    register int i;
		    register byte *pp;

		    aspath_insert_aspath(asp);
		    ap += len + PA4_LEN_SEGMENT;
		    len >>= 1;
		    pp = PATH_PTR(asp);
		    for (i = 0; i < len; i++) {
		        *pp++ = *bp++;
		        *pp++ = *bp++;
		    }
		    *PATH_SEG_PTR(asp) = (u_short) len;
		}
	    }
	    break;

	case PA_TYPE_NEXTHOP:	/* also PA4_TYPE_NEXTHOP */
	    if (len != PA_LEN_NEXTHOP) {
		/* Invalid length? */
		PA_RETURN_ERROR(PA_ERR_LENGTH, bpattr) ;
	    }
	    if ((flags & PA_FLAG_ALL) != PA_FLAG_TRANS) {
		/* Well known attribute, transitive flag only set */
		PA_RETURN_ERROR(PA_ERR_FLAGS, bpattr) ;
	    }
	    /*
	     * Copy address in.  It's already in network
	     * byte order.  Use bpattr as a tmp.
	     */
	    BIT_SET(api->api_flags, APIF_NEXTHOP);
	    sockclear_in(api->api_nexthop);
	    bpattr = (byte *) &sock2ip(api->api_nexthop);
	    *bpattr++ = *bp++;
	    *bpattr++ = *bp++;
	    *bpattr++ = *bp++;
	    *bpattr++ = *bp++;

	    /*
	     * Don't bother checking this here.  We'll
	     * leave that to the consumer of the data.
	     */
	    break;

	case PA_TYPE_UNREACH:	/* also PA4_TYPE_METRIC, the dorks */
	    if (version == PATH_VERSION_2OR3) {
		/* PA_TYPE_UNREACH */
		if (len != 0) {
		    /* Invalid length? */
		    PA_RETURN_ERROR(PA_ERR_LENGTH, bpattr) ;
		}
		if ((flags & PA_FLAG_ALL) != PA_FLAG_TRANS) {
		    /* Well known attribute, transitive flag only set */
		    PA_RETURN_ERROR(PA_ERR_FLAGS, bpattr) ;
		}
		BIT_SET(api->api_flags, APIF_UNREACH);
	    } else {
		/* PA4_TYPE_METRIC */
		if (len != PA4_LEN_METRIC) {
		    /* Invalid length? */
		    PA_RETURN_ERROR(PA_ERR_LENGTH, bpattr);
		}
		if ((flags & PA_FLAG_ALL) != PA_FLAG_OPT) {
		    PA_RETURN_ERROR(PA_ERR_FLAGS, bpattr) ;
		}
	        BIT_SET(api->api_flags, APIF_METRIC);
		PATH_GET_V4_METRIC(api->api_metric, bp);
	    }
	    break;
	    
	case PA_TYPE_METRIC:	/* also PA4_TYPE_LOCALPREF */
	    if (version == PATH_VERSION_2OR3) {
		/* PA_TYPE_METRIC */
		if (len != PA_LEN_METRIC) {
		    /* Invalid length? */
		    PA_RETURN_ERROR(PA_ERR_LENGTH, bpattr) ;
		}
		if ((flags & PA_FLAG_ALL) != PA_FLAG_OPT) {
		    PA_RETURN_ERROR(PA_ERR_FLAGS, bpattr) ;
		}
		BIT_SET(api->api_flags, APIF_METRIC);
		api->api_metric = (*bp++) << 8;
		api->api_metric |= (*bp++);
	    } else {
		/* PA4_TYPE_LOCALPREF */
		if (len != PA4_LEN_LOCALPREF) {
		    /* Invalid length? */
		    PA_RETURN_ERROR(PA_ERR_LENGTH, bpattr) ;
		}
		if ((flags & PA_FLAG_ALL) != PA_FLAG_TRANS) {
		    /* Should be well known */
		    PA_RETURN_ERROR(PA_ERR_FLAGS, bpattr) ;
		}
		BIT_SET(api->api_flags, APIF_LOCALPREF);
		PATH_GET_V4_METRIC(api->api_localpref, bp);
	    }
	    break;

	case PA4_TYPE_ATOMICAGG:
	    if (version == PATH_VERSION_2OR3) {
		/*
		 * Compatability.  If the attribute is marked well known
		 * return unrecognized well-known attribute, else return
		 * optional attribute error.
		 */
		if (!BIT_TEST(flags, PA_FLAG_OPT)) {
		    PA_RETURN_ERROR(PA_ERR_UNKNOWN, bpattr) ;
		}
		PA_RETURN_ERROR(PA_ERR_OPTION, bpattr) ;
	    }
	    if (len != 0) {
		/* Invalid length? */
		PA_RETURN_ERROR(PA_ERR_LENGTH, bpattr) ;
	    }
	    if ((flags & PA_FLAG_ALL) != PA_FLAG_TRANS) {
		/* Well known attribute, transitive flag only set */
		PA_RETURN_ERROR(PA_ERR_FLAGS, bpattr) ;
	    }
	    BIT_SET(asp->path_flags, PATH_FLAG_ATOMIC_AGG);
	    break;

	case PA4_TYPE_AGGREGATOR:
	    if ((flags & PA_FLAG_OPTTRANS) != PA_FLAG_OPTTRANS) {
		/* Optional attribute which we know */
		PA_RETURN_ERROR(PA_ERR_FLAGS, bpattr) ;
	    }

	    /*
	     * Since this is an optional transitive attribute
	     * we don't worry too much about its format.  Fall
	     * through to the optional attribute processing.  Note
	     * that it is the aggregator attribute, however, to
	     * make sure we get the length set.
	     */
	    asp->path_aggr_len = PATH_ATTR_LEN(len);
	    is_aggr = 1;
	    /*FALLSTHROUGH*/

	default:
	    /*
	     * Everything we don't know about comes here.
	     * The OPTIONAL flag must be set (since we don't
	     * know it).  If the TRANSITIVE flag is clear
	     * we simply ignore the attribute (after making
	     * sure the PARTIAL bit wasn't set).  If the
	     * TRANSITIVE flag is set we set the PARTIAL bit
	     * and insert in the buffer in the proper spot.
	     */
	    if (!BIT_TEST(flags, PA_FLAG_OPT)) {
		PA_RETURN_ERROR(PA_ERR_UNKNOWN, bpattr) ;
	    }
	    if (!BIT_TEST(flags, PA_FLAG_TRANS)) {
		if (BIT_TEST(flags, PA_FLAG_PARTIAL)) {
		    PA_RETURN_ERROR(PA_ERR_FLAGS, bpattr) ;
		}
		bp += len;      /* skip over the data */
		break;	/* Forget it */
	    }
	    if (!is_aggr) {
	        BIT_SET(flags, PA_FLAG_PARTIAL);
	    } else {
		is_aggr = 0;
	    }
	    if (code > lastcode) {		/* fast path */
		asp->path_attr_len += PATH_ATTR_LEN(len);
		PATH_PUT_ATTR(flags, code, len, ap);
		while (len-- > 0) {
		    *ap++ = *bp++;
		}
		lastcode = code;
	    } else {
		aspath_insert_attr(asp, flags, code, len, bp);
		bp += len;
		ap += PATH_ATTR_LEN(len);
	    }
	    break;
	}
    }

    /*
     * From our point of view there are only two mandatory attributes,
     * origin and AS path.  Return an error if either of these didn't
     * show up.
     */
    if (!ATTR_ISSET(PA_TYPE_ORIGIN, received) ||
	!ATTR_ISSET(PA_TYPE_ASPATH, received)) {
	if (!ATTR_ISSET(PA_TYPE_ORIGIN, received)) {
	    missing_attr = PA_TYPE_ORIGIN;
	} else {
	    missing_attr = PA_TYPE_ASPATH;
	}
	PA_RETURN_ERROR(PA_ERR_MISSING, &missing_attr) ;
    }
    
    /*
     * Done that.  Set the path hash and see if we have one like this
     * already in the hash table.  If not we'll need to evaluate the
     * path for loops.
     */
    asp->path_hash = PATHHASH(asp);
    nasp = aspath_find(asp);
    if (nasp == asp) {
	register u_short *p, *pend;

	p = PATH_SHORT_PTR(asp);
	pend = p + (asp->path_len / 2);
	if (n_local_as == 1) {
	    register as_t try_as = as_local_info[0].asl_as_net;

	    as_local_info[0].asl_found = 0;
	    while (p < pend) {
		if (*p++ == try_as) {
		    if ((++as_local_info[0].asl_found)
		      == as_local_info[0].asl_loop) {
			AS_LOCAL_SET(asp->path_looped, 0);
		    }
		}
	    }
	} else {
	    register int i;

	    for (i = 0; i < n_local_as; i++) {
		as_local_info[i].asl_found = 0;
	    }

	    while (p < pend) {
		register as_t try_as = *p++;
		for (i = 0; i < n_local_as; i++) {
		    if (as_local_info[i].asl_as_net == try_as) {
			if ((++as_local_info[i].asl_found)
			  == as_local_info[i].asl_loop) {
			    AS_LOCAL_SET(asp->path_looped, i);
			}
		    }
		}
	    }
	}

	if (asp->path_len > 2) {
	    p = PATH_SHORT_PTR(asp);
	    do {
		register u_short chkas, *p2;
		p2 = p + 1;
		chkas = *p++;
		do {
		    if (*p2++ == chkas) {
			BIT_SET(asp->path_flags, PATH_FLAG_ASLOOP);
			p = pend - 1;
			break;
		    }
		} while (p2 != pend);
	    } while (p != (pend - 1));
	}
    } else {
	asp = nasp;
    }
    return asp;

    /*
     * We come here for errors.  Free the attribute structure and return
     * nothing.
     */
belch:
    aspath_free(asp);
    return (as_path *) 0;

#undef	PA_RETURN_ERROR
}


/*
 * path_format - format a set of version 2/3 path attributes into an outgoing
 *   message.  This routine is responsible for ensuring that the AS path is
 *   updated appropriately in the outgoing message.
 */
byte *
aspath_format __PF5(myas, as_t,
		    asp, as_path *,
		    asip, as_path_info *,
		    next_hop_ptr, byte **,
		    bufp, byte *)
{
    register byte *cp;
    u_int len;

    /*
     * If it is a minimum unreachable packet, do this quickly.  Otherwise
     * we do it the long way.  We write the attributes into the packet in
     * ascending numeric order.
     */
    cp = bufp;
    if (BIT_TEST(asip->api_flags, APIF_UNREACH) && asp == NULL) {
	/*
	 * First the origin
	 */
	PATH_PUT_ATTR(PA_FLAG_TRANS, PA_TYPE_ORIGIN, PA_LEN_ORIGIN, cp);
	*cp++ = PATH_ORG_XX;

	/*
	 * Next the AS path.  This includes the local AS if this is
	 * external, or a zero-length AS path otherwise.
	 */
	if (BIT_TEST(asip->api_flags, APIF_INTERNAL)) {
	    PATH_PUT_ATTR(PA_FLAG_TRANS, PA_TYPE_ASPATH, 0, cp);
	} else {
	    PATH_PUT_ATTR(PA_FLAG_TRANS, PA_TYPE_ASPATH, PA_LEN_AS, cp);
	    PATH_PUT_AS(myas, cp);
	}

	/*
	 * Next the next_hop.  If he didn't include one in the path_info
	 * just skip the field.
	 */
	if (BIT_TEST(asip->api_flags, APIF_NEXTHOP)) {
	    PATH_PUT_ATTR(PA_FLAG_TRANS, PA_TYPE_NEXTHOP, PA_LEN_NEXTHOP, cp);
	    if (next_hop_ptr != NULL) {
		*next_hop_ptr = cp;
	    }
	    if (asip->api_nexthop) {
		PATH_PUT_NEXTHOP(asip->api_nexthop, cp);
	    } else {
		cp += PA_LEN_NEXTHOP;
	    }
	}

	/*
	 * Last is the unreachable attribute.
	 */
	PATH_PUT_ATTR(PA_FLAG_TRANS, PA_TYPE_UNREACH, PA_LEN_UNREACH, cp);
    } else {
	register u_short *pathp;
	register aslocal_t las = asp->path_local_as;

	/*
	 * Origin
	 */
	PATH_PUT_ATTR(PA_FLAG_TRANS, PA_TYPE_ORIGIN, PA_LEN_ORIGIN, cp);
	*cp++ = asp->path_origin;

	/*
	 * AS path.  We've got to compute the length once we've massaged
	 * the thing, so figure out what we need to do now.  We add an
	 * AS to the path for every local AS bit set in path_local_as, except
	 * for the bit which corresponds to myas.
	 */
	len = asp->path_len;
	if (las) {
	    if (n_local_as == 1) {
		assert(myas == as_local_info[0].asl_as);
		las = 0;
	    } else {
		register int i;

		for (i = 0; i < n_local_as; i++) {
		    if (as_local_info[i].asl_as == myas) {
			AS_LOCAL_RESET(las, i);
			if (las == 0) {
			    break;
			}
		    } else if (AS_LOCAL_TEST(las, i)) {
			len += PA_LEN_AS;
		    }
		}
	    }
	}

	if (!BIT_TEST(asip->api_flags, APIF_INTERNAL)) {
	    len += PA_LEN_AS;
	}

	PATH_PUT_ATTR(PA_FLAG_TRANS, PA_TYPE_ASPATH, len, cp);
	pathp = PATH_SHORT_PTR(asp);
	if (!BIT_TEST(asip->api_flags, APIF_INTERNAL)) {
	    PATH_PUT_AS(myas, cp);
	    len -= PA_LEN_AS;
	}
	if (las) {
	    register int i;

	    for (i = 0; i < n_local_as; i++) {
		if (AS_LOCAL_TEST(las, i)) {
		    PATH_PUT_AS(as_local_info[i].asl_as, cp);
		    len -= PA_LEN_AS;
		}
	    }
	}
	bcopy((caddr_t)pathp, (caddr_t)cp, (size_t)len);
	cp += len;

	/*
	 * Next hop.
	 */
	if (BIT_TEST(asip->api_flags, APIF_NEXTHOP)) {
	    PATH_PUT_ATTR(PA_FLAG_TRANS, PA_TYPE_NEXTHOP, PA_LEN_NEXTHOP, cp);
	    if (next_hop_ptr != NULL) {
		*next_hop_ptr = cp;
	    }
	    if (asip->api_nexthop) {
		PATH_PUT_NEXTHOP(asip->api_nexthop, cp);
	    } else {
		cp += PA_LEN_NEXTHOP;
	    }
	}

	/*
	 * Unreachable
	 */
	if (BIT_TEST(asip->api_flags, APIF_UNREACH)) {
	    PATH_PUT_ATTR(PA_FLAG_TRANS, PA_TYPE_UNREACH, PA_LEN_UNREACH, cp);
	}

	/*
	 * Metric
	 */
	if (BIT_TEST(asip->api_flags, APIF_METRIC) && asip->api_metric != (flag_t) -1) {
	    PATH_PUT_ATTR(PA_FLAG_OPT, PA_TYPE_METRIC, PA_LEN_METRIC, cp);
	    PATH_PUT_METRIC(asip->api_metric, cp);
	}

	/*
	 * Now any optional gunk we're carrying.
	 */
	if (asp->path_attr_len > 0) {
	    register size_t attr_len;

	    attr_len = (size_t)(asp->path_attr_len - asp->path_aggr_len);
	    if (attr_len) {
		register byte *attr_cp;
		attr_cp = PATH_ATTR_PTR(asp) + asp->path_aggr_len;
		bcopy((caddr_t)attr_cp, (caddr_t)cp, attr_len);
		cp += attr_len;
	    }
	}
    }

    /*
     * We're done.  Return.
     */
    return (cp);
}


/*
 * path_format_v4 - format a set of version 4 path attributes into an outgoing
 *   message.  This routine is responsible for ensuring that the AS path is
 *   updated appropriately in the outgoing message.
 */
byte *
aspath_format_v4 __PF5(myas, as_t,
		       asp, as_path *,
		       asip, as_path_info *,
		       next_hop_ptr, byte **,
		       bufp, byte *)
{
    register byte *cp, *pp;
    register u_short *segp;
    register u_int len;
    u_int seglen;
    aslocal_t las;
    int attr_len;
    byte *attr_start;
    int n_local_as_to_add = 0;
    int first_sequence_length = 0;
    as_t add_path_as = 0;

    cp = bufp;
    attr_len = asp->path_attr_len;
    attr_start = PATH_ATTR_PTR(asp);

    /*
     * Origin
     */
    PATH_PUT_ATTR(PA_FLAG_TRANS, PA4_TYPE_ORIGIN, PA4_LEN_ORIGIN, cp);
    *cp++ = asp->path_origin;

    /*
     * AS path.  We've got to compute the length once we've massaged
     * the thing, so figure out what we need to do now.
     */
    segp = PATH_SEG_PTR(asp);
    len = asp->path_len + asp->path_seg_len;

    /*
     * Check to see if we've got any segments longer than 255.  If so
     * we'll need to add segment headers.
     */
    if (asp->path_len > (u_int16) (PA_PATH_MAXSEGLEN * PA_LEN_AS)) {
	register u_short *tmpp = segp;
	register int i = asp->path_seg_len >> 1;

	while (i--) {
	    if ((seglen = PATH_SEG_LEN(*tmpp)) > PA_PATH_MAXSEGLEN) {
		do {
		    len += PA4_LEN_SEGMENT;
		    seglen -= PA_PATH_MAXSEGLEN;
		} while (seglen > PA_PATH_MAXSEGLEN);

		if (tmpp == segp
		  && PATH_ISSEQUENCE(*segp)
		  && seglen != PA_PATH_MAXSEGLEN) {
		    first_sequence_length = seglen;
		}
	    }
	    tmpp++;
	}
    } else if (len && PATH_ISSEQUENCE(*segp)) {
	first_sequence_length = PATH_SEG_LEN(*segp);
	if (first_sequence_length == PA_PATH_MAXSEGLEN) {
	    first_sequence_length = 0;
	}
    }

    /*
     * We may need to add local ASes.  Determine this now.
     */
    las = asp->path_local_as;
    if (las) {
	if (n_local_as == 1) {
	    AS_LOCAL_RESET(las, 0);
	    assert(as_local_info[0].asl_as == myas && las == 0);
	} else {
	    register int i;

	    for (i = 0; i < n_local_as; i++) {
		if (as_local_info[i].asl_as == myas) {
		    AS_LOCAL_RESET(las, i);
		    if (las == 0) {
			break;
		    }
		} else if (AS_LOCAL_TEST(las, i)) {
		    n_local_as_to_add++;
		    add_path_as = as_local_info[i].asl_as;
		}
	    }
	    if (n_local_as_to_add) {
		len += n_local_as_to_add * PA_LEN_AS;
		if (n_local_as_to_add > 1) {
		    first_sequence_length = 0;
		    len += PA4_LEN_SEGMENT;
		    add_path_as = 0;
		} else if (first_sequence_length == 0) {
		    len += PA4_LEN_SEGMENT;
		    first_sequence_length = 1;
		} else {
		    first_sequence_length++;
		    if (first_sequence_length == PA_PATH_MAXSEGLEN) {
			first_sequence_length = 0;
		    }
		}
	    }
	}
    }

    /*
     * If this isn't an internal session we'll need to add the
     * local AS to an initial sequence.  Determine what this
     * adds to the length.
     */
    if (!BIT_TEST(asip->api_flags, APIF_INTERNAL)) {
	len += PA_LEN_AS;
	if (first_sequence_length == 0) {
	    len += PA4_LEN_SEGMENT;
	    first_sequence_length = 1;
	} else {
	    first_sequence_length++;
	}
    }

    /*
     * Now begin to form the AS path in the output buffer.  Output
     * the attribute header with our carefully computed total length,
     * then reclaim the "len" variable to hold the number of segments.
     * Then follow through building the attribute.
     */
    PATH_PUT_ATTR(PA_FLAG_TRANS, PA4_TYPE_ASPATH, len, cp);
    len = asp->path_seg_len >> 1;

    if (first_sequence_length > 0) {
	/*
	 * Make an AS_SEQUENCE of length first_sequence_length.
	 * Fill it in with our local AS(s), we'll suck the
	 * rest off the first sequence in the path.
	 */
	*cp++ = PA_PATH_SEQ;
	*cp++ = (byte)first_sequence_length;
	if (!BIT_TEST(asip->api_flags, APIF_INTERNAL)) {
	    PATH_PUT_AS(myas, cp);
	    first_sequence_length--;
	}
	if (first_sequence_length && add_path_as) {
	    PATH_PUT_AS(add_path_as, cp);
	    first_sequence_length--;
	    add_path_as = 0;
	}
    }

    if (n_local_as_to_add > 1) {
	register int i;

	assert(first_sequence_length == 0);
	*cp++ = PA_PATH_SET;
	*cp++ = (byte)n_local_as_to_add;
	for (i = 0; i < n_local_as; i++) {
	    if (AS_LOCAL_TEST(las, i)) {
		PATH_PUT_AS(as_local_info[i].asl_as, cp);
	    }
	}
    } else if (add_path_as) {
	*cp++ = PA_PATH_SEQ;
	*cp++ = PA_PATH_MAXSEGLEN;
	PATH_PUT_AS(add_path_as, cp);
	first_sequence_length = PA_PATH_MAXSEGLEN - 1;
    }

    pp = PATH_PTR(asp);
    if (first_sequence_length > 0) {
	assert(len > 0);
	seglen = PATH_SEG_LEN(*segp);
	assert(PATH_ISSEQUENCE(*segp) && seglen >= first_sequence_length);
	seglen -= first_sequence_length;
	do {
	    *cp++ = *pp++;
	    *cp++ = *pp++;
	} while ((--first_sequence_length) > 0);

	if (seglen == 0) {
	    len--;
	    segp++;
	    if (len > 0) {
		seglen = PATH_SEG_LEN(*segp);
	    }
	}
    } else if (len > 0) {
	seglen = PATH_SEG_LEN(*segp);
    } else {
	seglen = 0;
    }

    while (len > 0) {
	register int i;

	if (PATH_ISSET(*segp)) {
	    *cp++ = PA_PATH_SET;
	} else {
	    *cp++ = PA_PATH_SEQ;
	}

	if (seglen > PA_PATH_MAXSEGLEN) {
	    if ((i = seglen % PA_PATH_MAXSEGLEN) == 0) {
		*cp++ = PA_PATH_MAXSEGLEN;
		seglen -= PA_PATH_MAXSEGLEN;
		i = PA_PATH_MAXSEGLEN;
	    } else {
		*cp++ = (byte) i;
		seglen -= i;
	    }
	} else {
	    i = seglen;
	    *cp++ = (byte) i;
	    seglen = 0;
	}

	i *= PA_LEN_AS;
	bcopy((void_t)pp, (void_t)cp, (size_t) i);
	pp += i;
	cp += i;

	if (seglen == 0) {
	    len--;
	    segp++;
	    if (len > 0) {
		seglen = PATH_SEG_LEN(*segp);
	    }
	}
    }

    /*
     * Next hop.
     */
    if (BIT_TEST(asip->api_flags, APIF_NEXTHOP)) {
	PATH_PUT_ATTR(PA_FLAG_TRANS, PA4_TYPE_NEXTHOP, PA4_LEN_NEXTHOP, cp);
	if (next_hop_ptr != NULL) {
	    *next_hop_ptr = cp;
	}
	if (asip->api_nexthop) {
	    PATH_PUT_NEXTHOP(asip->api_nexthop, cp);
	} else {
	    cp += PA_LEN_NEXTHOP;
	}
    }

    /*
     * Metric (MULTI_EXIT_DISC in BGP4-speak)
     */
    if (BIT_TEST(asip->api_flags, APIF_METRIC)) {
	PATH_PUT_ATTR(PA_FLAG_OPT, PA4_TYPE_METRIC, PA4_LEN_METRIC, cp);
	PATH_PUT_V4_METRIC(asip->api_metric, cp);
    }

    /*
     * Local preference
     */
    if (BIT_TEST(asip->api_flags, APIF_LOCALPREF)) {
	PATH_PUT_ATTR(PA_FLAG_TRANS, PA4_TYPE_LOCALPREF, PA4_LEN_LOCALPREF, cp);
	PATH_PUT_V4_METRIC(asip->api_localpref, cp);
    }

    /*
     * Atomic aggregate
     */
    if (BIT_TEST(asp->path_flags, PATH_FLAG_ATOMIC_AGG)) {
	PATH_PUT_ATTR(PA_FLAG_TRANS, PA4_TYPE_ATOMICAGG, PA4_LEN_ATOMICAGG, cp);
    }

    /*
     * Aggregator.  If we did this locally include our AS, otherwise
     * forward the received attribute (if any).
     */
    if (BIT_TEST(asp->path_flags, PATH_FLAG_LOCAL_AGG)) {
	PATH_PUT_ATTR(PA_FLAG_OPT|PA_FLAG_TRANS, PA4_TYPE_AGGREGATOR,
	  PA4_LEN_AGGREGATOR, cp);
	PATH_PUT_AS(myas, cp);
	if (BIT_TEST(asip->api_flags, APIF_LOCALID)) {
	    PATH_PUT_ID(asip->api_localid, cp);
	} else {
	    PATH_PUT_ID(0, cp);
	}
    } else if (asp->path_aggr_len != 0) {
	bcopy((void_t) attr_start, (void_t) cp, (size_t) asp->path_aggr_len);
	cp += asp->path_aggr_len;
	attr_start += asp->path_aggr_len;
	attr_len -= asp->path_aggr_len;
    }

    /*
     * Now any optional gunk we're carrying.
     */
    if (attr_len > 0) {
	bcopy((void_t)attr_start, (void_t)cp, (size_t)attr_len);
	cp += attr_len;
    }

    /*
     * We're done.  Return.
     */
    return (cp);
}


/*
 * aspath_v4_estimate_len - make a good guess at the compiled length
 *			    of a version 4 AS path.
 */
size_t
aspath_v4_estimate_len __PF3(myas, as_t,
			     asp, as_path *,
			     asip, as_path_info *)
{
    register size_t len;
    register u_int seglen;
    register u_int first_is_seq;
    register u_short *segp;

    /*
     * First the AS path.  The length will be close to the length of
     * data in the path_len section plus the segment length.
     */
    segp = PATH_SEG_PTR(asp);
    len = asp->path_len + asp->path_seg_len;

    /*
     * If we have segments longer than 255 we'll have to add in
     * extra segment headers.  Check that now.
     */
    if (asp->path_len > (u_int16) (PA_PATH_MAXSEGLEN * PA_LEN_AS)) {
	register int i = (asp->path_seg_len >> 1);
	register u_short *tmpp = segp;

	while (i--) {
	    if ((seglen = PATH_SEG_LEN(*tmpp)) > PA_PATH_MAXSEGLEN) {
		len += (seglen / PA_PATH_MAXSEGLEN) * PA4_LEN_SEGMENT;
	    }
	}

	/*
	 * Fetch the actual length of the first segment, we'll need to
	 * know this later.
	 */
	seglen = PATH_SEG_LEN(*segp) % ((u_int)PA_PATH_MAXSEGLEN);
	if (seglen == 0) {
	    seglen = PA_PATH_MAXSEGLEN;
	}
	first_is_seq = PATH_ISSEQUENCE(*segp);
    } else if (len > 0) {
	seglen = PATH_SEG_LEN(*segp);
	first_is_seq = PATH_ISSEQUENCE(*segp);
    } else {
	seglen = 0;
	first_is_seq = 0;
    }

    /*
     * We may need to add an AS to the path in the first segment.
     * If so add it in.  Otherwise we may have a local set at the
     * start.
     */
    if (asp->path_local_as && n_local_as > 1) {
	register int i;
	register aslocal_t las = asp->path_local_as;
	register int n_to_add = 0;

	for (i = 0; i < n_local_as; i++) {
	    if (as_local_info[i].asl_as == myas) {
		AS_LOCAL_RESET(las, i);
		if (las == 0) {
		    break;
		}
	    } else if (AS_LOCAL_TEST(las, i)) {
		n_to_add++;
	    }
	}

	if (n_to_add == 1) {
	    len += PA_LEN_AS;
	    if (!first_is_seq || seglen == PA_PATH_MAXSEGLEN) {
		len += PA4_LEN_SEGMENT;
		first_is_seq = 1;
		seglen = 1;
	    } else {
		seglen++;
	    }
	} else if (n_to_add > 0) {
	    len += (PA_LEN_AS * n_to_add) + PA4_LEN_SEGMENT;
	    first_is_seq = 0;
	}
    }

    /*
     * Okay, worked that out.  Now determine what to do with our AS
     */
    if (!BIT_TEST(asip->api_flags, APIF_INTERNAL)) {
	len += PA_LEN_AS;
	if (!first_is_seq || seglen == PA_PATH_MAXSEGLEN) {
	    len += PA4_LEN_SEGMENT;
	}
    }

    /*
     * Finally, add in the length of the attribute descriptor.
     */
    len = PATH_ATTR_LEN(len);

    /*
     * We know every packet we send will include an origin, so
     * add this in.
     */
    len += PATH_ATTR_LEN(PA4_LEN_ORIGIN);

    /*
     * Do the easy options in the path info.
     */
    if (BIT_TEST(asip->api_flags, APIF_LOCALPREF)) {
	len += PATH_ATTR_LEN(PA4_LEN_LOCALPREF);
    }
    if (BIT_TEST(asip->api_flags, APIF_METRIC)) {
	len += PATH_ATTR_LEN(PA4_LEN_METRIC);
    }
    if (BIT_TEST(asip->api_flags, APIF_NEXTHOP)) {
	len += PATH_ATTR_LEN(PA4_LEN_NEXTHOP);
    }

    /*
     * If we have an atomic aggregate, add that in.  If we have a local
     * aggregate flag, add that in.
     */
    if (BIT_TEST(asp->path_flags, PATH_FLAG_ATOMIC_AGG)) {
	len += PATH_ATTR_LEN(PA4_LEN_ATOMICAGG);
    }
    if (BIT_TEST(asp->path_flags, PATH_FLAG_LOCAL_AGG)) {
	assert(asp->path_aggr_len == 0);
	len += PATH_ATTR_LEN(PA4_LEN_AGGREGATOR);
    }

    /*
     * Now add in all the optional gunk we'll be sending
     */
    len += asp->path_attr_len;

    /*
     * That's it, got it.
     */
    return len;
}


/*
 * aspath_aggr_brief - do shortcut aggregation on the path list.  Find
 *		       the longest initial sequence/set and use this.
 */
as_path *
aspath_do_aggregation __PF1(rtah, rt_aggr_head *)
{
    register as_path_list *aplp;
    register as_path *asp;
    register size_t i, j;
    u_short *segp;
    u_short *ap, *apstart;
    u_short add_seg = 0;
    size_t plen;
    size_t nseg;
    as_path_list *list;
    int atomic = 0;
    union {
	as_path path_aggr_aspath;
	byte path_aggr_buf[sizeof(as_path) + ASPATHMAXSIZE];
    } path_buf;
#define	buf_asp		(&(path_buf.path_aggr_aspath))

    /*
     * Here we've had a change.  If there are no AS paths on the
     * list (i.e. the last route contributing to the aggregate
     * went away) return nil.  If there is only one AS path
     * represented in the list, return it.
     */
    aplp = list = rtah->rtah_aplp;
    if (!BIT_TEST(rtah->rtah_flags, RTAHF_ASPCHANGED)) {
	if (asp = rtah->rtah_rta_rt->rt_aspath)
	    ASPATH_ALLOC(asp);
	return asp;
    }
    BIT_RESET(rtah->rtah_flags, RTAHF_ASPCHANGED);
    if (!aplp) {
	return (as_path *) 0;
    }
    if (!(aplp->apl_next)) {
	asp = aplp->apl_asp;
	ASPATH_ALLOC(asp);
	return asp;
    }
    if (BIT_TEST(rtah->rtah_flags, RTAHF_GENERATE)) {
	asp = rtah->rtah_rta_forw->rta_rt->rt_aspath;
	ASPATH_ALLOC(asp);
	return asp;
    }

    /*
     * Zero the AS path structure we're building
     */
    bzero((void_t) buf_asp, sizeof(as_path));

    /*
     * Walk down the list once to build the longest common set of
     * AS numbers and to discover the local AS compliment.
     */
    apstart = PATH_SHORT_PTR(buf_asp);
    asp = aplp->apl_asp;
    buf_asp->path_local_as |= asp->path_local_as;
    if (asp->path_len > 0) {
	bcopy((void_t)PATH_SHORT_PTR(asp),
	      (void_t)apstart,
	      asp->path_len);
	ap = apstart + (asp->path_len / PA_LEN_AS);
    } else {
	ap = apstart;
    }
    buf_asp->path_origin = asp->path_origin;
    buf_asp->path_flags = (asp->path_flags & PATH_FLAG_ASLOOP);

    for (aplp = aplp->apl_next; aplp; aplp = aplp->apl_next) {
	asp = aplp->apl_asp;
	if (ap != apstart) {
	    register u_short *aptmp = apstart;
	    buf_asp->path_local_as |= asp->path_local_as;

	    segp = PATH_SHORT_PTR(asp);
	    plen = asp->path_len;

	    while (aptmp < ap && plen != 0) {
		if (*aptmp != *segp) {
		    break;
		}
		aptmp++; segp++;
		plen -= PA_LEN_AS;
	    }
	    if (aptmp != ap) {
		ap = aptmp;
		atomic = 1;
	    }
	}
	if (buf_asp->path_origin < asp->path_origin) {
	    buf_asp->path_origin = asp->path_origin;
	}
	buf_asp->path_flags |= (asp->path_flags & PATH_FLAG_ASLOOP);
    }

    /*
     * Okay, at this point we have the longest common leading list
     * of AS's in the paths, but have ignored segments.  There are a couple
     * of things we must do.  If some of these ASes are a part of an AS_SET,
     * but not the whole set, we must shorten to delete the set.  If
     * the segment structure is not the same in each path we must shorten
     * to the lowest common denominator.
     */
    aplp = list;
    asp = aplp->apl_asp;
    segp = PATH_SEG_PTR(asp);
    if (ap == apstart) {
	nseg = 0;
    } else {
	register u_short *his_segp;

	nseg = asp->path_seg_len / sizeof(u_short);

	his_segp = segp;
	plen = ap - apstart;
	for (i = 0; i < nseg; i++) {
	    j = PATH_SEG_LEN(*his_segp);
	    if (plen > j) {
		plen -= j;
		his_segp++;
		continue;
	    }
	    nseg = i + 1;
	    if (plen < j && PATH_ISSET(*his_segp)) {
		ap -= plen;
		nseg--;
		atomic = 1;
	    }
	    break;
	}

	while (ap != apstart && (aplp = aplp->apl_next)) {
	    register u_short *tmp_segp;

	    asp = aplp->apl_asp;
	    his_segp = PATH_SEG_PTR(asp);
	    tmp_segp = segp;
	    plen = ap - apstart;
	    for (i = 0; i < nseg; i++) {
		j = PATH_SEG_LEN(*his_segp);
		if (*his_segp != *tmp_segp) {
		    nseg = i + 1;
		    if (PATH_ISSET(*his_segp) || PATH_ISSET(*tmp_segp)) {
			ap -= plen;
			nseg--;
		    } else if (j < plen) {
			ap -= (plen - j);
		    }
		    atomic = 1;
		    break;
		}
		his_segp++;
		tmp_segp++;
		if (j < plen) {
		    plen -= j;
		} else {
		    assert(i == (nseg - 1));
		}
	    }
	}
    }

    /*
     * If atomic is set we have deleted some of the AS's in the
     * contributing paths from the aggregate.  If we are doing
     * full aggregation then collect these into a set at the end.
     *
     * XXX this could be done fancier by looking for common sequences
     * in the remaining AS's and splitting these out.  This is
     * expensive, though, and it appears that the common case for
     * aggregation will be that such sequences won't exist.
     */
    plen = ap - apstart;
    if (atomic && !BIT_TEST(rtah->rtah_flags, RTAHF_BRIEF)) {
	u_short total[ASPATHMAXSIZE/sizeof(u_short)];
	u_short new[ASPATHMAXSIZE/sizeof(u_short)];
	u_short new_pos[ASPATHMAXSIZE/sizeof(u_short)];
	size_t ntotal = 0;
	register u_short *np, *sp;

	for (aplp = list; aplp; aplp = aplp->apl_next) {
	    asp = aplp->apl_asp;
	    j = (asp->path_len / sizeof(u_short)) - plen;
	    if (j == 0) {
		continue;
	    }

	    sp = PATH_SHORT_PTR(asp) + plen;
	    new[0] = ntohs(*sp);
	    sp++;
	    for (i = 1; i < j; i++) {
		register u_short tmp = ntohs(*sp);

		if (i == 1) {
		    if (new[0] <= tmp) {
			new[1] = tmp;
		    } else {
			new[1] = new[0];
			new[0] = tmp;
		    }
		} else {
		    register size_t kmin = 0;
		    register size_t kmax = i - 1;
		    register size_t k = (i) / 2;

		    while (kmin < kmax) {
			if (tmp < new[k]) {
			    kmax = (kmin < k) ? (k - 1) : kmin;
			} else {
			    kmin = k;
			}
			k = (kmax + kmin + 1) / 2;
		    }
		    if (tmp >= new[kmin]) {
			k = kmin + 1;
		    }

		    /*
		     * AS goes into new[k]
		     */
		    if (k < i) {
			np = &new[i - 1];
			do {
			    *(np + 1) = *np;
			    np--;
			} while (np >= &new[k]);
		    }
		    new[k] = tmp;
		}
		sp++;
	    }

	    if (ntotal == 0) {
		ntotal = j;
		bcopy((void_t)new, (void_t)total, j * sizeof(u_short));
	    } else {
		register size_t kmin = 0;
		register size_t kmax;
		register size_t insert = 0;

		np = new_pos;
		for (i = 0; i < j && new[i] < total[0]; i++) {
		    *np++ = 0;
		    insert++;
		}

		for (; i < j; i++) {
		    register size_t k;

		    kmax = (ntotal - 1);
		    k = (kmin + kmax + 1) / 2;
		    while (kmin < kmax) {
			if (new[i] < total[k]) {
			    kmax = (kmin < k) ? (k - 1) : k;
			} else {
			    kmin = k;
			}
			k = (kmax + kmin + 1) / 2;
		    }
		    if (new[i] > total[kmin]) {
			*np++ = (kmin + 1);
			insert++;
		    } else {
			*np++ = (u_short)(-1);
			/*
			 * Deal with duplicates in new
			 */
			while (i < (j - 1) && new[i+1] == new[i]) {
			    i++;
			    if (kmin < (ntotal - 1)) {
				if (total[kmin+1] > new[i]) {
				    *np++ = kmin + 1;
				    insert++;
				} else {
				    kmin++;
				    *np++ = (u_short)(-1);
				}
			    } else {
				*np++ = kmin + 1;
				insert++;
			    }
			}
		    }
		}

		if (insert > 0) {
		    i = ntotal;
		    ntotal += insert;
		    sp = &new[j];
		    while (insert) {
			register size_t tmp = *(--np);

			sp--;
			if (tmp == (u_short)(-1)) {
			    continue ;
			}
			while (i > tmp) {
			    i--;
			    total[i + insert] = total[i];
			}
			insert--;
			total[i + insert] = *sp;
		    }
		}
	    }
	}

	add_seg = (u_short)(ntotal);
	np = total;
	do {
	    *ap++ = htons(*np);
	    np++;
	} while ((--ntotal) > 0);
	atomic = 0;
    }

    buf_asp->path_len = ((byte *)ap) - ((byte *)apstart);
    buf_asp->path_seg_len = nseg * PA4_LEN_SEGMENT;
    if (plen > 0) {
	for (i = 0; i < nseg; i++) {
	    j = PATH_SEG_LEN(*segp);
	    if (j <= plen) {
		*ap++ = *segp++;
		plen -= j;
		assert(j < plen || i == (nseg - 1));
	    } else {
		*ap++ = plen;		/* Must be sequence! */
		assert(i == (nseg - 1));
	    }
	}
    }
    if (add_seg) {
	while (add_seg > PA_PATH_MAXSEGLEN) {
	    *ap++ = PA_PATH_MAXSEGLEN | PATH_AS_SET;
	    add_seg -= PA_PATH_MAXSEGLEN;
	    buf_asp->path_seg_len += PA4_LEN_SEGMENT;
	}
	*ap++ = add_seg | PATH_AS_SET;
	buf_asp->path_seg_len += PA4_LEN_SEGMENT;
    }

    if (atomic) {
	/*
	 * If there was an AS loop in one of the component paths,
	 * check to see if this has been eliminated by the
	 * truncation.
	 */
	if (buf_asp->path_flags && buf_asp->path_len > 2) {
	    register u_short *pend;

	    segp = PATH_SHORT_PTR(buf_asp);
	    pend = segp + (buf_asp->path_len / 2);
	    do {
		register u_short *p;

		add_seg = *segp++;
		p = segp;
		do {
		    if (*p == add_seg) {
			goto found_loop;
		    }
		} while ((++p) != pend);
	    } while (segp != (pend - 1));

	    buf_asp->path_flags = 0;
	}
found_loop:
	BIT_SET(buf_asp->path_flags, PATH_FLAG_ATOMIC_AGG);
	buf_asp->path_origin = PATH_ORG_XX;
    }
    BIT_SET(buf_asp->path_flags, PATH_FLAG_LOCAL_AGG);
    plen = ((byte *)ap) - ((byte *)apstart);
    asp = aspath_alloc(plen);
    buf_asp->path_size = asp->path_size;
    bcopy((void_t) buf_asp, (void_t) asp, (plen + sizeof(as_path)));
    asp->path_hash = PATHHASH(asp);
    asp = aspath_find(asp);
    return asp;
#undef	buf_asp
}



/*
 * aspath_aggregate - a new route is being added to an aggregate,
 *		      return an AS path for it.
 */
int
aspath_aggregate_changed __PF3(rtah, rt_aggr_head *,
			       old_asp, as_path *,
			       new_asp, as_path *)
{
    register as_path_list *aplp;
    int new_changed = 0;
    int old_changed = 0;

    /*
     * If the old and new AS paths are the same, return the existing route.
     */
    if (old_asp == new_asp) {
	if (BIT_TEST(rtah->rtah_flags, RTAHF_ASPCHANGED)) {
	    return TRUE;
	}
	return FALSE;
    }

    if (old_asp) {
	as_path_list *apl_prev = (as_path_list *) 0;

	/*
	 * Find the path on the list.  It must be there.
	 */
	aplp = rtah->rtah_aplp;
	while (aplp) {
	    if (aplp->apl_asp == old_asp) {
		break;
	    }
	    apl_prev = aplp;
	    aplp = aplp->apl_next;
	}
	assert(aplp);

	/*
	 * If the reference count has dropped to zero, blow the
	 * list away and note that we've changed.
	 */
	if (--(aplp->apl_refcount) == 0) {
	    if (!apl_prev) {
		rtah->rtah_aplp = aplp->apl_next;
	    } else {
		apl_prev->apl_next = aplp->apl_next;
	    }
	    task_block_free(as_path_list_block, (void_t) aplp);
	    old_changed = 1;
	}
    }

    if (new_asp) {
	if (!as_path_list_block) {
	    as_path_list_block = task_block_init(sizeof(as_path_list),
						 "as_path_list");
	}

	/*
	 * See if the route is on the AS path list.  If so this
	 * is easy.
	 */
	aplp = rtah->rtah_aplp;
	while (aplp) {
	    if (aplp->apl_asp == new_asp) {
		/*
		 * A match, just bump the reference count and break
		 * out of this.
		 */
		aplp->apl_refcount++;
		break;
	    }
	    aplp = aplp->apl_next;
	}

	/*
	 * If not on the list, add a new list entry for this AS path
	 */
	if (!aplp) {
	    aplp = (as_path_list *) task_block_alloc(as_path_list_block);
	    aplp->apl_refcount = 1;
	    aplp->apl_asp = new_asp;
	    aplp->apl_next = rtah->rtah_aplp;
	    rtah->rtah_aplp = aplp;
	    new_changed = 1;
	}
    }

    if (rtah->rtah_rta_forw->rta_rt == rtah->rtah_rta_rt)
	assert(!rtah->rtah_aplp);

    /*
     * If doing a generation only the AS path on the first route in
     * the list matters.
     */
    if (BIT_TEST(rtah->rtah_flags, RTAHF_GENERATE)) {
	if (BIT_TEST(rtah->rtah_flags, RTAHF_ASPCHANGED)
	  || rtah->rtah_rta_forw->rta_rt == rtah->rtah_rta_rt
	  || rtah->rtah_rta_forw->rta_rt->rt_aspath
	    != rtah->rtah_rta_rt->rt_aspath) {
	    BIT_SET(rtah->rtah_flags, RTAHF_ASPCHANGED);
	    return TRUE;
	}
	return FALSE;
    }

    /*
     * If we had a change, set his bit.
     */
    if (old_changed || new_changed) {
	BIT_SET(rtah->rtah_flags, RTAHF_ASPCHANGED);
	return TRUE;
    }
    if (BIT_TEST(rtah->rtah_flags, RTAHF_ASPCHANGED)) {
	return TRUE;
    }
    return FALSE;
}


/*
 * aspath_aggregate_free - free the AS path list attached to this head
 */
void
aspath_aggregate_free __PF1(rtah, rt_aggr_head *)
{
    register as_path_list *aplp, *aplp_next;

    for (aplp = rtah->rtah_aplp; aplp; aplp = aplp_next) {
	aplp_next = aplp->apl_next;
	task_block_free(as_path_list_block, (void_t) aplp);
    }
    rtah->rtah_aplp = (as_path_list *) 0;
}

/*
 * aspath_list_dump - dump the AS path list for someone else's code.
 */
void
aspath_list_dump __PF2(fd, FILE *,
		       rtah, rt_aggr_head *)
{
    as_path_list *list = rtah->rtah_aplp;

    if (list) {
	(void) fprintf(fd, "\t\t\tAS Path List:\n");

	do {
	    aspath_dump(fd, list->apl_asp, "\t\t\t\t", " Refcount: ");
	    (void) fprintf(fd, "%u\n", list->apl_refcount);
	} while ((list = list->apl_next));
    }
}


/*
 * path_rt_build - take an arbitrary route and fill in the path attribute
 *	  	   pointer to the appropriate structure.
 */
void
aspath_rt_build __PF2(rt, register rt_entry *,
		      asp, register as_path *)
{
    register gw_entry *gwp;
    register as_t as;
    register int origin;

    /*
     * If the entry already has a path associated with it, complain
     * and return.
     */
    assert(!rt->rt_aspath);

    /*
     * Fetch a pointer to the gw entry.  Evaluate what we need to
     * do based on protocol (external OSPF is the only one we worry
     * about specially at this point).
     */
    gwp = rt->rt_gwp;

    /*
     * If the aspath pointer is non-null this is essentially an
     * aspath_rt_link().  If the route came from an IBGP neighbour, however,
     * we need to search through looking for the corresponding OSPF_ASE
     * route.  If we find it we have some work to do.
     */
    if (asp) {
	ASPATH_ALLOC(asp);
	aspath_rt_link(rt, asp);
#if	defined(PROTO_BGP) && defined(PROTO_OSPF)
	if (gwp->gw_proto == RTPROTO_BGP
	  && BIT_TEST(gwp->gw_flags, GWF_AUXPROTO)) {
	    rt_entry *rt1;
	    u_long id = ((bgpPeer *)(gwp->gw_task->task_data))->bgp_id;

	    RT_ALLRT(rt1, rt->rt_head) {
	        /* XXX need to check for group membership someday */
		if (BIT_TEST(rt1->rt_state, RTS_DELETE)) {
		    return;
		}
		if (rt1->rt_gwp->gw_proto == RTPROTO_OSPF_ASE
		    && id == ORT_ADVRTR(rt1)) {
		    (void) rt_change_aspath(rt1,
					    rt1->rt_metric,
					    rt1->rt_metric2,
					    rt1->rt_tag,
					    rt1->rt_preference,
					    rt1->rt_preference2,
					    rt1->rt_n_gw,
					    rt1->rt_routers,
					    asp);
		    return;
		}
	    } RT_ALLRT_END(rt1, rt->rt_head) ;
	}
#endif	/* defined(PROTO_BGP) && defined(PROTO_OSPF) */
	return;
    }

    switch (gwp->gw_proto) {
#ifdef PROTO_OSPF
    case RTPROTO_OSPF_ASE:
	/*
	 * We'll need to check out the tag.  Fetch it from the OSPF
	 * data base.  This is a crock of shit, I wish the tag definitions
	 * were closer to real life.
	 */
        {
	    u_long tag = PATH_GET_OSPF_TAG(rt);
	    
	    if (PATH_OSPF_ISTRUSTED(tag)) {
		switch(tag & PATH_OSPF_TAG_LEN_MASK) {
		case PATH_OSPF_TAG_LEN_0:
		    as = 0;
		    if (PATH_OSPF_ISCOMPLETE(tag)) {
		    	origin = PATH_ORG_IGP;
		    } else {
		    	origin = PATH_ORG_EGP;
		    }
		    break;

		case PATH_OSPF_TAG_LEN_1:
		    as = tag & PATH_OSPF_TAG_AS_MASK;
		    if (PATH_OSPF_ISCOMPLETE(tag)) {
		    	origin = PATH_ORG_IGP;
		    } else {
		    	origin = PATH_ORG_EGP;
		    }
		    break;

		case PATH_OSPF_TAG_LEN_2:
		    if (PATH_OSPF_ISCOMPLETE(tag)) {
#ifdef	PROTO_BGP
			/*
			 *  The tag says that we need to wait for the IBGP route to get the
			 *  path info.  So look for a valid IBGP route with the same router ID
			 */
			rt_entry *rt1;
			u_long adv_rtr = ORT_ADVRTR(rt);

			RT_ALLRT(rt1, rt->rt_head) {
			    /* XXX need to check for group membership someday */
			    if (rt1->rt_gwp->gw_proto == RTPROTO_BGP &&
				BIT_TEST(rt1->rt_gwp->gw_flags, GWF_AUXPROTO) &&
				!BIT_TEST(rt1->rt_state, RTS_DELETE) &&
				adv_rtr == ((bgpPeer *)(rt1->rt_gwp->gw_task->task_data))->bgp_id &&
				(asp = rt1->rt_aspath)) {
				/* This is the one!  Bump the refcount and return this path */

				ASPATH_ALLOC(asp);
    				aspath_rt_link(rt, asp);
				return;
			    }
			} RT_ALLRT_END(rt1, rt->rt_head) ;

			/*
			 * Couldn't find a good route, when the IBGP route arrives it will
			 * fill in the pointer.
			 */
#endif	/* PROTO_BGP */
			return;
		    }

		    as = tag & PATH_OSPF_TAG_AS_MASK;
		    origin = PATH_ORG_XX;
		    if (gwp->gw_local_as != 0 && gwp->gw_local_as == as) {
			as = 0;
		    }
		    break;

		default:	/* really case PATH_OSPF_TAG_LEN_X: */
		    /*
		     * This is invalid, complain and return a NULL path.
		     */
		    trace_log_tf(trace_global,
				 0,
				 LOG_WARNING,
				 ("path_rt: OSPF ASE tag 0x%08x has invalid length",
				  tag));
		    return;
		}
	    } else {
		/*
		 * What can we do here?  Make up a local, incomplete path.
		 */
		as = 0;
		origin = PATH_ORG_XX;
	    }
	}
	break;
#endif /* PROTO_OSPF */

#ifdef PROTO_BGP
    case RTPROTO_BGP:
	/*
	 * This can't happen.  Complain and die.
	 */
	assert(FALSE);
#endif /* PROTO_BGP */	

    case RTPROTO_AGGREGATE:
	/*
	 * This guy always knows his path.  Just leave him null.
	 */
	return;

    default:
	/*
	 * Here we need to decide whether this was received via an EGP
	 * or an IGP and set the tags accordingly.
	 */
	as = gwp->gw_peer_as;
	if (as != 0) {
	    origin = PATH_ORG_EGP;
	} else {
	    origin = PATH_ORG_IGP;
	}
	break;
    }


    /*
     * Okay, we have an origin, and possibly an AS.  Fill in the AS path.
     */
    asp = aspath_alloc((size_t)(PA_LEN_AS + PA4_LEN_SEGMENT));	/* XXX IS-IS? */
    if (gwp->gw_local_as) {
	register int i;

	for (i = 0; i < n_local_as; i++) {
	    if (as_local_info[i].asl_as == gwp->gw_local_as) {
		break;
	    }
	}
	assert(i < n_local_as);
	AS_LOCAL_SET(asp->path_local_as, i);
    }

    asp->path_origin = origin;
    if (as != 0) {
        register u_short *cp = PATH_SHORT_PTR(asp);
	
	asp->path_len = PA_LEN_AS;
	*cp++ = htons(as);
	asp->path_seg_len = PA4_LEN_SEGMENT;
	*cp = 1;
    }
    asp->path_hash = PATHHASH(asp);
    asp = aspath_find(asp);

    /*
     * Connect path to route
     */
    aspath_rt_link(rt, asp);
}


/*
 * aspath_create - create an AS path with a single AS in the path.
 *		   XXX what a hack.  Blow it away.
 */
as_path *
aspath_create __PF1(as, as_t)
{
    register as_path *asp;
    register u_short *cp;

    if (!as) {
	return (as_path *) 0;
    }

    asp = aspath_alloc((size_t)(PA_LEN_AS + PA4_LEN_SEGMENT));
    asp->path_origin = PATH_ORG_IGP;
    asp->path_len = PA_LEN_AS;
    asp->path_seg_len = PA4_LEN_SEGMENT;
    cp = PATH_SHORT_PTR(asp);
    *cp++ = htons(as);
    *cp = 1;
    asp->path_hash = PATHHASH(asp);
    asp = aspath_find(asp);
    return asp;
}


/*
 * path_prefer - determine which route is preferred based on their AS paths.
 *   Return <0 if the first, >0 if the second, and == 0 if they are the same.
 */
int
aspath_prefer __PF2(rt1, rt_entry *,
		    rt2, rt_entry *)
{
    as_path *asp1, *asp2;

    /*
     * Fetch the AS path pointers.
     */
    asp1 = rt1->rt_aspath;
    asp2 = rt2->rt_aspath;

    /*
     * If both pointers are null, return no preference.  If one is
     * null, return the other.
     */
    if (asp1 == NULL) {
	if (asp2 == NULL) {
	    return (0);
	}
	return (1);
    } else if (asp2 == NULL) {
	return (-1);
    }

    /*
     * If these came from the same AS, and were received by the
     * same protocol, their metrics should be comparable.  This
     * covers the case where they both are from local EGP neighbours,
     * or from local external BGP neighbours, or where one arrived
     * via external BGP and the other arrived via internal BGP after
     * being received from a BGP or EGP neighbour.  Notably absent
     * is the case where one came from local EGP and the other from
     * remote EGP via BGP.
     */
    if (asp1->path_len > 0 && asp2->path_len > 0
      && (!PATH_ISSET(*PATH_SEG_PTR(asp1)) && !PATH_ISSET(*PATH_SEG_PTR(asp2)))
      && (*PATH_SHORT_PTR(asp1) == *PATH_SHORT_PTR(asp2))
      && rt1->rt_gwp->gw_proto == rt2->rt_gwp->gw_proto
      && rt1->rt_metric != (metric_t) -1 && rt2->rt_metric != (metric_t) -1) { /*XXX Fix me*/
	if (rt1->rt_metric < rt2->rt_metric) {
	    return (-1);
	} else if (rt1->rt_metric > rt2->rt_metric) {
	    return (1);
	}
    }

    /*
     * If the AS path pointers are different, see if we can make
     * something of the differences.
     */
    if (asp1 != asp2) {
	/*
	 * See if the origins are distinguishable.  If so, return the
	 * one with the smallest numeric code (see codes above).
	 */
	if (asp1->path_origin != asp2->path_origin) {
	    if (asp1->path_origin < asp2->path_origin) {
		return (-1);
	    }
	    return (1);
	}

	/*
	 * See if the AS paths are different lengths.  If so prefer the
	 * shorter one.
	 */
	if (asp1->path_len != asp2->path_len) {
	    if (asp1->path_len < asp2->path_len) {
		return (-1);
	    }
	    return (1);
	}
    }

    /*
     * Here we have absolutely no idea of which to prefer.  Let someone
     * else take care of this.
     */
    return (0);
}


#ifdef	PROTO_OSPF
/*
 * path_tag_dump - produce a printable version of an OSPF tag
 */
char *
aspath_tag_dump __PF2(as, as_t,
		      tag, u_long)
{
    static char buf[60];	/* as much as we need? */
    const char *origin;
    as_t tag_as;
    u_int tag_tag;

    if (BIT_TEST(tag, PATH_OSPF_TAG_TRUSTED)) {
	/*
	 * Fetch the origin based on the length and the complete bit
	 */
	if ((tag & PATH_OSPF_TAG_LEN_MASK) == PATH_OSPF_TAG_LEN_X) {
	    /* Invalid */
	    (void) sprintf(buf, "Invalid tag: %08x", tag);
	    return (buf);
	} else if ((tag & PATH_OSPF_TAG_LEN_MASK) == PATH_OSPF_TAG_LEN_2) {
	    origin = BIT_TEST(tag, PATH_OSPF_TAG_COMPLETE)
	      ? "IBGP" : "Incomplete";
	} else {
	    origin = BIT_TEST(tag, PATH_OSPF_TAG_COMPLETE) ? "IGP" : "EGP";
	}
	tag_as = tag & PATH_OSPF_TAG_AS_MASK;
	tag_tag = (tag & PATH_OSPF_TAG_USR_MASK) >> PATH_OSPF_TAG_USR_SHIFT;
	(void) sprintf(buf, "%u Path: (%u) %u %s",
	  tag_tag,
	  as,
	  tag_as,
	  origin);
    } else {
	sprintf(buf, "%u", tag);
    }
    return buf;
}


/*
 * path_tag_ospf - produce a suitable tag for OSPF based on the route
 *		   attributes.
 */
u_long
aspath_tag_ospf __PF3(as, as_t,
		      rt, rt_entry *,
		    arbtag, metric_t)
{
    register gw_entry *gwp;
    register u_long tag = (arbtag << PATH_OSPF_TAG_USR_SHIFT) & PATH_OSPF_TAG_USR_MASK;

    /*
     * We need to treat BGP and routes from other OSPF's specially
     * here.  Otherwise we can do a stock thing.
     */
    gwp = rt->rt_gwp;
    switch (gwp->gw_proto) {
#ifdef PROTO_BGP
	register as_path *asp;
	register int send_via_bgp;
	int bitno;

    case RTPROTO_BGP:
	/*
	 * Fetch the AS path.  If the path is zero or one AS's long,
	 * and there are no special attributes, we can encode it entirely
	 * within the tag.  Otherwise we need to tag it so the receiver
	 * knows to wait for internal BGP.
	 */
	send_via_bgp = 0;
	asp = rt->rt_aspath;
	AS_LOCAL_FINDBIT(as, bitno);
	if ((asp->path_local_as) & (~bitno)) {
	    register aslocal_t las = (asp->path_local_as) & (~bitno);
	    register int i;

	    for (i = 0; i < n_local_as; i++) {
		if (AS_LOCAL_TEST(las, i)) {
		    break;
		}
	    }
	    if (i < n_local_as) {
		tag |= ((u_long)(as_local_info[i].asl_as)) & PATH_OSPF_TAG_AS_MASK;
	    }
	    if (asp->path_len > 0 || asp->path_attr_len > 0) {
		send_via_bgp = 1;
	    }
	} else if (asp->path_len > 0) {
	    register byte *cp = PATH_PTR(asp);
	    
	    tag |= (*cp++) << 8;
	    tag |= *cp;
	    if (asp->path_len > 2 || asp->path_attr_len > 0) {
		send_via_bgp = 1;
	    }
	} else {
	    if (asp->path_attr_len > 0) {
		send_via_bgp = 1;
	    }
	}

	if (send_via_bgp) {
	    /*
	     * Fixed upper attributes here.
	     */
	    tag |= (PATH_OSPF_TAG_TRUSTED|PATH_OSPF_TAG_COMPLETE|PATH_OSPF_TAG_LEN_2);
	} else if (asp->path_origin == PATH_ORG_XX) {
	    /*
	     * This one is incomplete
	     */
	    tag |= (PATH_OSPF_TAG_TRUSTED|PATH_OSPF_TAG_LEN_2);
	} else {
	    /*
	     * Set the length to zero or one based on whether we have
	     * an AS or not.  Mark it complete if the origin is IGP.
	     */
	    if (tag == 0) {
	        tag |= (PATH_OSPF_TAG_TRUSTED|PATH_OSPF_TAG_LEN_0);
	    } else {
		tag |= (PATH_OSPF_TAG_TRUSTED|PATH_OSPF_TAG_LEN_1);
	    }
	    
	    if (asp->path_origin == PATH_ORG_IGP) {
		tag |= PATH_OSPF_TAG_COMPLETE;
	    }
	}
	break;
#endif /* PROTO_BGP */

    case RTPROTO_OSPF_ASE:
	/*
	 * This is a slightly tricky case.  If the tag is user-set we
	 * send it on as incomplete.  If both OSPF's are operating
	 * in the same AS we send the tag through unchanged.  Otherwise,
	 * if it is a length 0 tag we send it on as a length 1 tag with
	 * the route's AS appended.  Otherwise, we send it on tagged for
	 * BGP.
	 */
	tag |= PATH_GET_OSPF_TAG(rt) & (~((u_long)(PATH_OSPF_TAG_USR_MASK)));
	if (!(tag & PATH_OSPF_TAG_TRUSTED)) {
	    tag = PATH_OSPF_TAG_TRUSTED|PATH_OSPF_TAG_LEN_2;
	}

	if (gwp->gw_local_as != as) {
	    if ((tag & PATH_OSPF_TAG_LEN_MASK) == PATH_OSPF_TAG_LEN_0) {
		tag &= ~(PATH_OSPF_TAG_AS_MASK|PATH_OSPF_TAG_LEN_MASK);
		tag |= PATH_OSPF_TAG_LEN_1|
		    (gwp->gw_local_as & PATH_OSPF_TAG_AS_MASK);
	    } else if ((tag & (PATH_OSPF_TAG_LEN_MASK|PATH_OSPF_TAG_COMPLETE))
		       == PATH_OSPF_TAG_LEN_2 && (tag & PATH_OSPF_TAG_AS_MASK) == 0) {
		tag |= (gwp->gw_local_as & PATH_OSPF_TAG_AS_MASK);
	    } else {
		tag = PATH_OSPF_TAG_TRUSTED |
		    PATH_OSPF_TAG_LEN_2 |
			PATH_OSPF_TAG_COMPLETE
			    |(gwp->gw_local_as & PATH_OSPF_TAG_AS_MASK);
	    }
	}	
	break;

    default:
	/*
	 * The standard place we end up.  Here the route protocol is either
	 * operating as an EGP or an IGP, and may or may not be operating
	 * in the same AS as the OSPF which is importing the route.
	 */
	if (gwp->gw_local_as == 0 || (gwp->gw_peer_as == 0 && gwp->gw_local_as == as)) {
	    /*
	     * Assume its from our IGP
	     */
	    tag |= PATH_OSPF_TAG_TRUSTED |
		PATH_OSPF_TAG_COMPLETE |
		    PATH_OSPF_TAG_LEN_0;
	} else if (gwp->gw_peer_as == 0) {
	    /*
	     * From an IGP running in another AS
	     */
	    tag |= PATH_OSPF_TAG_TRUSTED |
		PATH_OSPF_TAG_COMPLETE |
		    PATH_OSPF_TAG_LEN_1 |
			(gwp->gw_local_as & PATH_OSPF_TAG_AS_MASK);
	} else if (gwp->gw_local_as == as) {
	    /*
	     * From an EGP running in this AS
	     */
	    tag |= PATH_OSPF_TAG_TRUSTED |
		PATH_OSPF_TAG_LEN_1 |
		    (gwp->gw_peer_as & PATH_OSPF_TAG_AS_MASK);
	} else {
	    /*
	     * All that is left is that we received the route from an
	     * EGP whose local AS is not that of the OSPF.  We'll need
	     * to send this via BGP to get it right.
	     */
	    tag |= PATH_OSPF_TAG_TRUSTED |
		PATH_OSPF_TAG_COMPLETE |
		    PATH_OSPF_TAG_LEN_2 |
			(gwp->gw_local_as & PATH_OSPF_TAG_AS_MASK);
	}
	break;
    }
    return htonl(tag);
}
#endif /* PROTO_OSPF */


#ifdef PROTO_BGP
/*
 * path_adv_ibgp - determine if this route should be advertised via
 *		   internal BGP or not.
 * XXX igp_proto should actually be a pointer to something which describes
 *     the state of the IGP.  e.g. IS-IS might or might not carry all
 *     path attributes internally.
 * XXX what about milnet?  Think about route server.
 */
int
aspath_adv_ibgp __PF3(as, as_t,
		      igp_proto, proto_t,
		      rt, rt_entry *)
{
    register gw_entry *gwp;
#ifdef	PROTO_OSPF
    register u_long tag;
    register as_path *asp;
    int bitno;
#endif	/* PROTO_OSPF */

    /*
     * So far we only know how to handle OSPF
     */
    gwp = rt->rt_gwp;

    switch (igp_proto) {
#ifdef PROTO_OSPF
    case RTPROTO_OSPF_ASE:
    case RTPROTO_OSPF:
	switch(gwp->gw_proto) {
	case RTPROTO_BGP:
	    /*
	     * If there are extra attributes, it goes via internal BGP.
	     * If the path wouldn't fit in an OSPF tag it goes via IBGP.
	     */
	    asp = rt->rt_aspath;
	    if (asp->path_local_as) {
		AS_LOCAL_FINDBIT(as, bitno);
	    } else {
		bitno = 0;
	    }
	    if (asp->path_attr_len > 0
		|| ((asp->path_local_as & ~bitno) && asp->path_len > 0)
		|| (asp->path_len > 2)) {
		return TRUE;
	    }
	    return FALSE;
	    
	case RTPROTO_OSPF_ASE:
	    /*
	     * Check out the tag.  If this route was received
	     * by an OSPF operating in our same AS, don't send
	     * via IBGP.  Else if the route was received directly
	     * by the other AS, don't send either.
	     */
	    tag = PATH_GET_OSPF_TAG(rt);
	    if (!PATH_OSPF_ISTRUSTED(tag)) {
		return FALSE;
	    }
	    if (gwp->gw_local_as == as) {
		/* can send via OSPF using same tag */
		return FALSE;
	    }
	    if ((tag & PATH_OSPF_TAG_LEN_MASK) == PATH_OSPF_TAG_LEN_0
		|| (((tag & (PATH_OSPF_TAG_COMPLETE|PATH_OSPF_TAG_LEN_MASK))
		     == (PATH_OSPF_TAG_LEN_2))
		    && ((tag & PATH_OSPF_TAG_AS_MASK) == 0))) {
		/* IGP/EGP with zero length AS */
		return FALSE;
	    }
	    break;

	default:
	    /*
	     * In the normal case the only thing we need to send
	     * via IBGP is stuff that came from an EGP operating in
	     * another AS
	     */
	    if (gwp->gw_peer_as == 0 || gwp->gw_local_as == as) {
	        return FALSE;
	    }
	    break;
	}
#endif /* PROTO_OSPF */

    default:
	/*
	 * Just barf, we can't deal with anything else yet.
	 */
	assert(FALSE);
    }

    return TRUE;
}
#endif	/* PROTO_BGP */


/**/
/* Tag support */
tag_t
tag_rt __PF1(rt, rt_entry *)
{
    tag_t tag;

    switch (rt->rt_gwp->gw_proto) {
#ifdef	PROTO_OSPF
    case RTPROTO_OSPF_ASE:
	tag = PATH_GET_OSPF_TAG(rt);
	if (PATH_OSPF_ISTRUSTED(tag)) {
	    tag = (tag & PATH_OSPF_TAG_USR_MASK) >> PATH_OSPF_TAG_USR_SHIFT;
	}
	break;
#endif	/* PROTO_OSPF */

    default:
	tag = 0;
    }

    return tag;
}
