#ident	"@(#)asmatch.c	1.3"
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
#include "parse.h"


#ifdef TEST_ASMATCH
/* quick and dirty test setup */
static const char *test_asmatch_patterns[] = {
    "\021\026\074",
    "\021\026\023",
    "\021",
    "\022\021\023",
    "\021\020",
    "\021\026\020",
    "\021\020\144",
    "\021\026\144",
    "\020",
    "\020\026",
    "\020\074",
    "\020\026\074",
    "\020\023",
    "\020\026\023",
    "\144",
    "\144\026",
    "\144\074",
    "\144\026\074",
    "\144\023",
    "\144\026\023",
    0
};
#endif


/*
 *  macro versions of a few avl functions - originally from avl.h
 */

#define FIND_AVL_ENTRY(entry, init, lookfor, compare, found, type) \
{ \
    int cmp; \
    entry = init; \
    found = (type) 0; \
    while (entry) { \
	cmp = compare(lookfor, entry); \
	if (cmp > 0) \
	    entry = entry->forw; \
	else if (cmp < 0) { \
	    if (entry->mid && (compare(lookfor, entry->mid) == 0)) { \
		found = (type) entry->mid; \
		break; \
	    } \
	    entry = entry->back; \
	} else { \
	    found = (type) entry; \
	    break; \
	} \
    } \
}

#define FIND_LESS_OR_EQUAL_AVL_ENTRY(entry, init, lookfor, compare, found, type) \
{ \
    int cmp; \
    entry = init; \
    found = (type) 0; \
    while (entry) { \
	cmp = compare(lookfor, entry); \
	if (cmp > 0) { \
	    found = (type) entry; \
	    entry = entry->forw; \
	} else if (cmp < 0) { \
	    if (entry->mid && (compare(lookfor, entry->mid) >= 0)) { \
		found = (type) entry->mid; \
		break; \
	    } \
	    entry = entry->back; \
	} else { \
	    found = (type) entry; \
	    break; \
	} \
    } \
}

/*
 *  prototypes originally from avl.h
 */
PROTOTYPE(find_smallest_avl_entry,
	  static struct avl_tree *,
	  (struct avl_tree *head));
PROTOTYPE(find_largest_avl_entry,
	  static struct avl_tree *,
	  (struct avl_tree *head));
PROTOTYPE(insert_avl_entry,
	  static struct avl_tree *,
	  (struct avl_tree *head, struct avl_tree *e,
	   _PROTOTYPE(compare, int, (void *, struct avl_tree *))));
PROTOTYPE(remove_avl_entry,
	  static struct avl_tree *,
	  (struct avl_tree *head, struct avl_tree *e,
	   _PROTOTYPE(compare, int, (void *, struct avl_tree *))));
PROTOTYPE(free_avl_entry,
	  static void,
	  (struct avl_tree *entry,
	   _PROTOTYPE(freeit, void, (struct avl_tree *))));
PROTOTYPE(span_avl_tree,
	  static int,
	  (struct avl_tree *head,
	   _PROTOTYPE(func, int, (struct avl_tree *e, void *arg)),
	   void *arg));
PROTOTYPE(find_less_or_equal_avl_entry,
	  static struct avl_tree *,
	  (struct avl_tree *head, void *lookfor,
	   _PROTOTYPE(compare, int, (void *, struct avl_tree *))));


/*
 *  static functions used by the asmatch code
 */

PROTOTYPE(clone_asp_trans,
	  static asp_trans **,
	  (asp_trans **,
	   asp_trans *));
PROTOTYPE(clone_asp_table,
	  static asp_table *,
	  (asp_table *));
PROTOTYPE(asp_table_overlap,
	  static int,
	  (asp_table *, asp_table *));
PROTOTYPE(asp_table_equiv,
	  static int,
	  (asp_table *, asp_table *));
PROTOTYPE(merge_asp_table,
	  static asp_table *,
	  (asp_table *,
	   asp_table *));
PROTOTYPE(attach_asp_state,
	  static int,
	  (struct avl_tree *,
	   void *));
PROTOTYPE(adj_asp_repeat,
	  static int,
	  (struct avl_tree *,
	   void *));
PROTOTYPE(detach_from_asp_group,
	  static void,
	  (asp_state *));
PROTOTYPE(cmp_asp_table,
	  static int,
	  (void *,
	   struct avl_tree *));
PROTOTYPE(merge_asp_group,
	  static int,
	  (struct avl_tree *, void *));
PROTOTYPE(clone_as_tree,
	  static int,
	  (struct avl_tree *, void *));



PROTOTYPE(aspath_table_entry_print,
	  static int,
	  (struct avl_tree *,
	   void *));
PROTOTYPE(aspath_indent,
	  static char *,
	  (char *,
	   int));
PROTOTYPE(aspath_table_print,
	  static void,
	  (FILE *,
	   int,
	   asp_table *));
PROTOTYPE(aspath_trans_list_print,
	  static void,
	  (FILE *,
	   int,
	   asp_trans *));
PROTOTYPE(free_asp_table,
	  static void,
	  (asp_table *));
PROTOTYPE(free_asp_trans,
	  static void,
	  (asp_trans *));
PROTOTYPE(free_asp_avl_table_entry,
	  static void,
	  (struct avl_tree *));
PROTOTYPE(free_asp_state,
	  static void,
	  (asp_state *));


/*
 *  functions used to build an aspath regex
 */

static block_t asp_state_task_block;
static block_t asp_trans_task_block;
static block_t asp_table_task_block;
static block_t aspath_match_index;
static block_t asp_regex_match_block;


#define MAXFRAMES (((8192-16) - (2 * sizeof(void *))) \
		   / sizeof(struct regex_match_frame))

struct regex_match_frame {
    asp_table *tbl;
    as_t *asp;
    asp_trans *t;
    asp_state *grp;
    as_t *as_tmp;
    short tmp_remain;		/* safe to assume aspath < 32,000 */
    short iter;
};

struct regex_match_stack {
    struct regex_match_stack *blk;
    struct regex_match_frame *frame;
    struct regex_match_frame frames[MAXFRAMES];
};


void
init_asmatch_alloc __PF0(void)
{
    aspath_match_index = task_block_init(sizeof (asmatch_t),
					 "asmatch_t");
    asp_state_task_block = task_block_init(sizeof (asp_state),
					   "asp_state");
    asp_trans_task_block = task_block_init(sizeof (asp_trans),
					   "asp_trans");
    asp_table_task_block = task_block_init(sizeof (asp_table),
					   "asp_table");
    asp_regex_match_block = task_block_init(sizeof (struct regex_match_stack),
					    "struct regex_match_stack");
}

static void
free_asp_table __PF1(t, asp_table *)
{
    assert(t->use_count);
    if (!(--t->use_count)) {
	free_asp_trans(t->trans);
	task_block_free(asp_table_task_block, (void_t) t);
    }
}


static void
free_asp_trans __PF1(t, asp_trans *)
{
    asp_trans *next;

    while (t) {
	next = t->next;
	assert(t->state);
	free_asp_state(t->state);
	task_block_free(asp_trans_task_block, t);
	t = next;
    }
}


static void
free_asp_avl_table_entry __PF1(e, struct avl_tree *)
{
    free_asp_table((asp_table *) e);
}


static void
detach_from_asp_group __PF1(s, asp_state *)
{
    asp_state *ss;
    
    if (s->asps_grp) {
	s->asps_grp_next->asps_grp_prev = s->asps_grp_prev;
	s->asps_grp_prev->asps_grp_next = s->asps_grp_next;
	if (s == s->asps_grp) {
	    for (ss = s->asps_grp_next; ; ) {
		assert(ss);
		ss->asps_grp = s->asps_grp_next;
		ss = ss->asps_grp_next;
		if (ss == s->asps_grp_next)
		    break;
	    }
	}
    }
}


static void
free_asp_state __PF1(s, asp_state *)
{
    assert(s->asps_use_count);
    if (!(--s->asps_use_count)) {
	detach_from_asp_group(s);
	if (s->asps_tree)
	    free_avl_entry(&s->asps_tree->search, free_asp_avl_table_entry);
	task_block_free(asp_state_task_block, s);
    }
}


#define trace_unload() \
  { trace_trace(trace_global, TRC_NOSTAMP); trace_clear(); }

#define TRACE_ASP_REGEX_PROGRESS(fmt) \
  { trace_tf(trace_global, \
	     TR_ADV, \
	     0, \
	     (fmt, \
	      parse_where())); \
    if (TRACE_TF(trace_global, TR_ADV)) { \
	aspath_adv_print((FILE *)0, aspath_current); \
    } \
  }


static asmatch_t *aspath_current;

void
aspath_init_regex()
{
    aspath_current = (asmatch_t *) task_block_alloc(aspath_match_index);
}


asmatch_t *
aspath_consume_current __PF1(head, asp_stack)
{
    asmatch_t *rtn = aspath_current;
    
    aspath_current = (asmatch_t *) 0;
    rtn->first = head;
#if defined(TEST_ASMATCH) || defined(TIME_ASMATCH)
    {
	union {
	    as_path_data as_path_data;
	    struct {
		as_path asp_data;
		as_t asp[8];
	    } fudge;
	} stuff;
	as_t *asp = &stuff.fudge.asp[0];
	int i;
#ifdef TEST_ASMATCH
	const char **x;
	const char *pt;
#endif
#ifdef TIME_ASMATCH
	struct timeval dawn, dusk;
	struct timezone tz;
	double elapsed;
#endif
	
	rtn->origin_mask = (flag_t) -1;
#ifdef TEST_ASMATCH
	trace_only_tf(trace_global, TRC_NOSTAMP, ("DONE ++++++++++++++"));
	for (x = &test_asmatch_patterns[0]; *x; ++x) {
	    asp = &stuff.fudge.asp[0];
	    for (i = 0, pt = *x; *pt; ++i)
		*asp++ = htons(*pt++);
	    stuff.fudge.asp_data.path_len = i * sizeof(as_t);
	    for ( ; i < 8; ++i)
		*asp++ = 0;	/* not null terminated */
	    asp = &stuff.fudge.asp[0];
	    trace_only_tf(trace_global, TRC_NOSTAMP,
			  ("RESULT: %d %d %d %d %d %d %d %d -> %d",
			   ntohs(asp[0]), ntohs(asp[1]),
			   ntohs(asp[2]), ntohs(asp[3]),
			   ntohs(asp[4]), ntohs(asp[5]),
			   ntohs(asp[6]), ntohs(asp[7]),
			   aspath_adv_match(rtn, (as_path *)&stuff)));
	}
	trace_only_tf(trace_global, TRC_NOSTAMP, ("DONE --------------"));
#endif
#ifdef TIME_ASMATCH
	gettimeofday(&dawn, &tz);
	asp[0] = htons(17);
	asp[1] = htons(22);
	asp[2] = htons(60);
	asp[3] = 0;
	stuff.fudge.asp_data.path_len = 3 * sizeof(as_t);
	for (i = 0; i < (1024*1024); ++i) {
	    asp[0] = htons(777);
	    aspath_adv_match(rtn, (as_path *)&stuff);
	    asp[0] = htons(17);
	    aspath_adv_match(rtn, (as_path *)&stuff);
	    if (!(i & 8191)) {
		gettimeofday(&dusk, &tz);
		if (dusk.tv_sec - dawn.tv_sec > 10)
		    break;
	    }
	}
	gettimeofday(&dusk, &tz);
	elapsed = (double)(dusk.tv_sec - dawn.tv_sec);
	if (dusk.tv_usec > dawn.tv_usec) {
	    elapsed += (.000001 *
			((double)(dusk.tv_usec - dawn.tv_usec)));
	} else {
	    elapsed -= 1;
	    elapsed += (.000001 *
			((double)(1000000 + dusk.tv_usec - dawn.tv_usec)));
	}
	trace_only_tf(trace_global, TRC_NOSTAMP,
		      ("DONE with %d iter in %d.%d sec",
		       i, (int)elapsed,
		      (int)((elapsed - (int)elapsed) * .000001)));
	trace_only_tf(trace_global, TRC_NOSTAMP,
		      ("DONE %d iter/sec",
		       (int)(((double)i) / elapsed)));
#endif
    }
#endif
    return rtn;
}


void
aspath_simple_regex __PF2(regex, asp_stack *,
			  new, asp_stack *)
{
    aspath_current->first = *regex = *new;
    TRACE_ASP_REGEX_PROGRESS("%s: REDUCED:       regex : sum");
}


void
aspath_copy_regex __PF2(regex, asp_stack *,
			new, asp_stack *)
{
    aspath_current->first = *regex = *new;
    TRACE_ASP_REGEX_PROGRESS("%s: REDUCED:       regex : sum");
}


static asp_trans **
clone_asp_trans __PF2(pprev, asp_trans **,
		      clone, asp_trans *)
{
    asp_trans *trans;
    
    for ( ; clone; clone = clone->next) {
	trans = (asp_trans *) task_block_alloc(asp_trans_task_block);
	*pprev = trans;
	pprev = &trans->next;
	trans->state = clone->state;
	assert(trans->state);
	assert(trans->state->asps_use_count);
	++trans->state->asps_use_count;
	trans->flags = clone->flags;
    }
    return pprev;
}


static asp_table *
clone_asp_table __PF1(old, asp_table *)
{
    asp_table * clone = task_block_alloc(asp_table_task_block);
    
    assert(old->use_count);
    clone->lo_as = old->lo_as;
    clone->hi_as = old->hi_as;
    clone->use_count = 1;
    return clone;
}


static int
cmp_asp_table __PF2(arg1, void *,
		    arg2, struct avl_tree *)
{
    asp_table *t1 = (asp_table *) arg1;
    asp_table *t2 = (asp_table *) arg2;
    
    return ((int) t1->lo_as) - (int)t2->lo_as;
}


static int
asp_table_overlap __PF2(t1, asp_table *,
			t2, asp_table *)
{
    if (t1->lo_as <= t2->lo_as) {
	if (t2->lo_as <= t1->hi_as)
	    return 1;
	return 0;
    }
    if (t1->lo_as <= t2->hi_as)
	return 1;
    return 0;
}


static int
asp_table_equiv __PF2(t1, asp_table *,
		      t2, asp_table *)
{
    if (t1 == t2)
	return 1;
    /* we could be a lot smarter about this */
    return 0;
}


static asp_table *
merge_asp_table __PF2(prev, asp_table *,
		      next, asp_table *)
{
    asp_table *near, *move, *olap;
    int save_lo;
    
    assert(prev->use_count);
    while (next) {
	assert(next->use_count);
	/*  find the smallest entry and and remove it */
	move = (asp_table *) find_smallest_avl_entry(&next->search);
	next = (asp_table *)
	    remove_avl_entry(&next->search, &move->search, cmp_asp_table);
	/* find the nearest entry in the next tree */
	near = (asp_table *)
	    find_less_or_equal_avl_entry(&prev->search,
					 (void *)move, cmp_asp_table);
	if (!near) {
	    near = (asp_table *)
		find_smallest_avl_entry(&prev->search);
	    if (!near) {
		prev = move;	/* must be an empty list */
		continue;
	    }
	}
	/* look for the easy cases first */
	if (!asp_table_overlap(move, near)) {
	    if (move->lo_as == move->hi_as) {
		/* easy case - no overlap, just add the entry */
		prev = (asp_table *)  
		    insert_avl_entry(&prev->search,
				     &move->search, cmp_asp_table);
		continue;
	    }
	    save_lo = move->lo_as;
	    move->lo_as = move->hi_as;
	    near = (asp_table *)
		find_less_or_equal_avl_entry(&prev->search,
					     (void *)move, cmp_asp_table);
	    move->lo_as = save_lo;
	    if ((!near) || !asp_table_overlap(move, near)) {
		/* still no overlap, just add the entry */
		prev = (asp_table *)  
		    insert_avl_entry(&prev->search,
				     &move->search, cmp_asp_table);
		continue;
	    }
	}
	/* the tougher case - we have an overlap */
	if (near->lo_as != move->lo_as
	    && near->hi_as != move->hi_as) {
	    /* we now have three disjoint sets */
	    olap = task_block_alloc(asp_table_task_block);
	    olap->use_count = 1;
	    clone_asp_trans(clone_asp_trans(&olap->trans, near->trans),
			    move->trans);
	    if (near->lo_as < move->lo_as) {
		olap->lo_as = move->lo_as;
		if (near->hi_as < move->hi_as) {
		    /* disjoint sets */
		    olap->hi_as = near->hi_as;
		    move->lo_as = olap->hi_as + 1;
		    /* put the remainder back and look for more overlaps */
		    next = (asp_table *)  
			insert_avl_entry(&next->search,
					 &move->search, cmp_asp_table);
		} else {	/* we already know they are not equal */
		    /* move is a subset of near - forming three regions */
		    olap->hi_as = move->hi_as;
		    move->lo_as = olap->hi_as + 1;
		    move->hi_as = near->hi_as;
		    free_asp_trans(move->trans);
		    clone_asp_trans(&move->trans, near->trans);
		    prev = (asp_table *)  
			insert_avl_entry(&prev->search,
					 &move->search, cmp_asp_table);
		}
		near->hi_as = olap->lo_as - 1;
	    } else {
		olap->lo_as = near->lo_as;
		/* remove near since it's lo entry will change */
		prev = (asp_table *)
		    remove_avl_entry(&prev->search,
				     &near->search, cmp_asp_table);
		if (move->hi_as < near->hi_as) {
		    /* disjoint sets */
		    olap->hi_as = move->hi_as;
		    near->lo_as = olap->hi_as + 1;
		} else if (move->hi_as > near->hi_as) {
		    /* near is a subset of move - forming three regions */
		    olap->hi_as = near->hi_as;
		    near->lo_as = olap->hi_as + 1;
		    near->hi_as = move->hi_as;
		    free_asp_trans(near->trans);
		    clone_asp_trans(&near->trans, move->trans);
		}
		/* put back the excess so it can be rechecked for overlap */
		next = (asp_table *)  
		    insert_avl_entry(&next->search,
				     &move->search, cmp_asp_table);
		/* put the near entry back in */
		prev = (asp_table *)  
		    insert_avl_entry(&prev->search,
				     &near->search, cmp_asp_table);
		move->hi_as = olap->lo_as - 1;
	    }
	    prev = (asp_table *)  
		insert_avl_entry(&prev->search, &olap->search, cmp_asp_table);
	    continue;
	}
	if (near->lo_as < move->lo_as
	    || near->hi_as != move->hi_as) {
	    /* partial overlap - adjust the subset */
	    if (near->lo_as < move->lo_as) {
		near->hi_as = move->lo_as - 1;
	    } else {
		near->lo_as = move->hi_as + 1;
	    }
	    clone_asp_trans(&move->trans, near->trans);
	    prev = (asp_table *)  
		insert_avl_entry(&prev->search, &move->search, cmp_asp_table);
	    continue;
	}
	/* exact overlap */
	clone_asp_trans(&near->trans, move->trans);
	free_asp_table(move);
    }
    return prev;
}


/*
 *  note that some of the asserts here cover cases that will not work until
 *  a seaparate group struct is created and chaining of groups is allowed
 *  such that a state may be in more than one group
 */
static int
merge_asp_group __PF2(_entry, struct avl_tree *,
		      _pgrp, void *)
{
    asp_table *entry;
    asp_trans *trans;
    asp_state **pgrp, *grp, *state, *prev, *next;
    
    entry = (asp_table *) _entry;
    pgrp = (asp_state **) _pgrp;
    grp = *pgrp;
    for (trans = entry->trans; trans; trans = trans->next) {
	state = trans->state;
	if (state->asps_tree) {
	    /* could occur with ( { 1 2 } | 3 ) ...etc */
	    assert(FALSE);
	}
	if (grp == state)
	    continue;
	if (state->asps_grp) {
	    if (grp && ((grp->asps_min != state->asps_min)
			|| (grp->asps_max != state->asps_max)
			|| (grp->asps_tree != state->asps_tree))) {
		/* could occur with ( { ( 1 | 2 | 3 )+ } | 4 ) ...etc */
		assert(FALSE);
	    }
	    /* remove state from the old group */
	    detach_from_asp_group(state);
	}
	if (!grp) {
	    grp = state;
	    *pgrp = state;
	    state->asps_grp = state;
	    state->asps_grp_next = state;
	    state->asps_grp_prev = state;
	} else {
	    state->asps_grp = grp;
	    next = grp->asps_grp_next;
	    prev = grp->asps_grp_prev;
	    next->asps_grp_prev = state;
	    state->asps_grp_next = next;
	    prev->asps_grp_next = state;
	    state->asps_grp_prev = prev;
	}
    }
    return 1;
}


void
aspath_merge_regex  __PF3(regex, asp_stack *,
			  _sum, asp_stack *,
			  _new, asp_stack *)
{
    asp_table *new = *_new;
    asp_table *sum = *_sum;
    asp_state *grp = 0;
    
    sum = merge_asp_table(sum, new);
    span_avl_tree(&sum->search, merge_asp_group, &grp);
    aspath_current->first = sum;
    if (regex)
	*regex = sum;
    TRACE_ASP_REGEX_PROGRESS("%s: REDUCED:	regex : sum | regex");
}


static int
clone_as_tree __PF2(_entry, struct avl_tree *,
		    _arg, void *)
{
    asp_table *entry = (asp_table *) _entry;
    asp_table **tree = (asp_table **) _arg;
    asp_table *clone = clone_asp_table(entry);
    asp_trans *t, *tt;
    asp_trans **pprev = &clone->trans;
    
    for (t = entry->trans; t; t = t->next) {
	tt = (asp_trans *) task_block_alloc(asp_trans_task_block);
	tt->flags = t->flags;
	*pprev = tt;
	pprev = &tt->next;
	tt->state = t->state;
	assert(t->state);
	assert(t->state->asps_use_count);
	++tt->state->asps_use_count;
    }
    if (*tree)
	*tree = (asp_table *)
	    insert_avl_entry(&(*tree)->search, &clone->search, cmp_asp_table);
    else
	*tree = clone;
    return 1;
}


struct aspath_attach_args {
    asp_table *attach;
    asp_table **prev;
};


static int
attach_asp_state __PF2(_entry, struct avl_tree *,
		       _arg, void *)
{
    asp_table *entry = (asp_table *) _entry;
    struct aspath_attach_args *arg = (struct aspath_attach_args *)_arg;
    asp_table *attach = arg->attach;
    asp_table **prev;
    asp_trans *trans;
    asp_state *state;
    
    for (trans = entry->trans; trans; trans = trans->next) {
	if (trans->flags & ASP_SKIP_SET)
	    continue;
	state = trans->state;
	if (!state->asps_tree) {
	    state->asps_tree = attach;
	    ++attach->use_count;
	    if (state->asps_min == 0) {
		asp_table *clone;
		clone = 0;
		span_avl_tree(&attach->search, clone_as_tree, &clone);
		prev = arg->prev;
		if (*prev)
		    *prev = merge_asp_table(*prev, clone);
		else
		    *prev = clone;
	    }
	} else if (state->asps_tree != attach) {
	    struct aspath_attach_args push;
	    asp_table *merge = 0;
	    push.attach = attach;
	    push.prev = &merge;
	    span_avl_tree(&state->asps_tree->search,
			  attach_asp_state, (void *) &push);
	    if (merge) {
		assert(merge->use_count);
		state->asps_tree = merge_asp_table(state->asps_tree, merge);
	    }
	}
    }
    return 1;
}


int
aspath_prepend_regex __PF3(regex, asp_stack *,
			   _sum, asp_stack *,
			   _new, asp_stack *)
{
    asp_table *sum = *_sum;
    asp_table *new = *_new;
    asp_table *merge = 0;
    struct aspath_attach_args push;
    
    /* attach new to all terminating transitions */
    push.attach = new;
    push.prev = &merge;
    span_avl_tree(&sum->search, attach_asp_state, (void *) &push);
    if (merge) {
	assert(merge->use_count);
	sum = merge_asp_table(sum, merge);
    }
    aspath_current->first = *regex = sum;
    TRACE_ASP_REGEX_PROGRESS("%s: REDUCED:	sum : term sum");
    return 1;
}


static int
adj_asp_repeat __PF2(_entry, struct avl_tree *,
		     _range, void *)
{
    asp_table *entry = (asp_table *) _entry;
    asp_range *range = (asp_range *) _range;
    asp_trans *trans;
    asp_state *state;
    
    for (trans = entry->trans; trans; trans = trans->next) {
	if (trans->flags & ASP_SKIP_SET)
	    continue;
	state = trans->state;
	state->asps_min = range->begin;
	state->asps_max = range->end;
    }
    return 1;
}


void
aspath_zero_or_more_term __PF2(regex, asp_stack *,
			       _old, asp_stack *)
{
    asp_range range;
    
    range.begin = 0;
    range.end = 0;
    span_avl_tree(&(*_old)->search, adj_asp_repeat, (void *) &range);
    aspath_current->first = *regex = *_old;
    TRACE_ASP_REGEX_PROGRESS("%s: REDUCED:	term : symbol *");
}


void
aspath_one_or_more_term __PF2(regex, asp_stack *,
			      _old, asp_stack *)
{
    asp_range range;
    
    range.begin = 1;
    range.end = 0;
    span_avl_tree(&(*_old)->search, adj_asp_repeat, (void *) &range);
    aspath_current->first = *regex = *_old;
    TRACE_ASP_REGEX_PROGRESS("%s: REDUCED:	term : symbol +");
}


void
aspath_zero_or_one_term __PF2(regex, asp_stack *,
			      _old, asp_stack *)
{
    asp_range range;
    
    range.begin = 0;
    range.end = 1;
    span_avl_tree(&(*_old)->search, adj_asp_repeat, (void *) &range);
    aspath_current->first = *regex = *_old;
    TRACE_ASP_REGEX_PROGRESS("%s: REDUCED:	term : symbol ?");
}


int
aspath_range_term __PF3(regex, asp_stack *,
			_old, asp_stack *,
			range, asp_range *)
{
    span_avl_tree(&(*_old)->search, adj_asp_repeat, (void *) range);
    aspath_current->first = *regex = *_old;
    TRACE_ASP_REGEX_PROGRESS("%s: REDUCED:	term : symbol { range }");
    return 1;
}


int
aspath_as_transition __PF2(regex, asp_stack *,
			   as_num, int)
{
    asp_table *table = task_block_alloc(asp_table_task_block);
    asp_trans *trans = task_block_alloc(asp_trans_task_block);
    asp_state *state = task_block_alloc(asp_state_task_block);
    
    state->asps_grp = 0;
    state->asps_grp_prev = 0;
    state->asps_grp_next = 0;
    state->asps_min = 1;
    state->asps_max = 1;
    state->asps_tree = 0;
    state->asps_use_count = 1;
    trans->next = 0;
    trans->state = state;
    state->asps_use_count = 1;
    table->lo_as = as_num;
    table->hi_as = as_num;
    table->trans = trans;
    table->use_count = 1;
    aspath_current->first = *regex = table;
    trace_tf(trace_global,
	     TR_ADV,
	     0,
	     ("%s: REDUCED:	symbol : %d ",
	      parse_where(),
	      as_num));
    if (TRACE_TF(trace_global, TR_ADV)) {
	aspath_adv_print(0, aspath_current);
    }
    return 1;
}


int
aspath_any_transition __PF1(regex, asp_stack *)
{
    asp_table *table = task_block_alloc(asp_table_task_block);
    asp_trans *trans = task_block_alloc(asp_trans_task_block);
    asp_state *state = task_block_alloc(asp_state_task_block);
    
    state->asps_grp = 0;
    state->asps_grp_prev = 0;
    state->asps_grp_next = 0;
    state->asps_min = 1;
    state->asps_max = 1;
    state->asps_tree = 0;
    state->asps_use_count = 1;
    trans->next = 0;
    trans->state = state;
    state->asps_use_count = 1;
    table->lo_as = 1;
    table->hi_as = (as_t) -1;
    table->trans = trans;
    table->use_count = 1;
    aspath_current->first = *regex = table;
    TRACE_ASP_REGEX_PROGRESS("%s: REDUCED:	symbol : .");
    return 1;
}

char *
aspath_adv_origins __PF1(mask, flag_t)
{
    static char line[BUFSIZ];
    
    if (mask == (flag_t) -1) {
	(void) strcpy(line, "all");
    } else {
	bits *p;
	
	*line = (char) 0;
	for (p = path_Orgs; p->t_name; p++) {
	    if ((1 << p->t_bits) & mask) {
		if (*line) {
		    (void) strcat(line, " | ");
		}
		(void) strcat(line, gd_lower(p->t_name));
	    }
	}
    }

    return line;
}

void
aspath_adv_free __PF1(psp, void_t)
{
    asmatch_t *asmp = (asmatch_t *) psp;

    free_asp_table(asmp->first);
    task_block_free(aspath_match_index, psp);
}


#define trace_or_print(fd, line) \
    if (fd) \
	fprintf(fd, "%s", line); \
    else \
	tracef("%s", line);

void
aspath_adv_print __PF2(fd, FILE *,
		       psp, void_t)
{
    asmatch_t *asmp = (asmatch_t *) psp;
    char line[BUFSIZ];

    sprintf(line, "[ start: ");
    trace_or_print(fd, line);
    aspath_table_print(fd, 1, asmp->first);
    sprintf(line, " ] origins %s",
	    aspath_adv_origins(asmp->origin_mask));
    trace_or_print(fd, line);
    if (!fd)
	trace_unload();
}


struct print_context {
    FILE *fd;
    int depth;
};


static char *
aspath_indent __PF2(line, char *,
		    depth, int)
{
    int i;

    *line++ = '\n';
    for (i = depth; i > 0; ) {
	if (i >= 4) {
	    i -= 4;
	    *line++ = '\t';
	} else {
	    --i;
	    *line++ = ' ';
	    *line++ = ' ';
	}
    }
    return line;
}


static int
aspath_table_entry_print __PF2(e, struct avl_tree *,
			       arg, void *)
{
    asp_table *entry = (asp_table *)e;
    char line[BUFSIZ], *pt;
    struct print_context * ctx = (struct print_context *)arg;
    FILE *fd = ctx->fd;
    int depth = ctx->depth;

    assert(entry->use_count);
    pt = aspath_indent(line, depth);
    if (entry->lo_as == entry->hi_as)
	sprintf(pt, "AS %d", entry->lo_as);
    else
	sprintf(pt, "AS %d - %d", entry->lo_as, entry->hi_as);
    trace_or_print(fd, line);
    aspath_trans_list_print(fd, depth, entry->trans);
    if (!fd)
	trace_unload();
    return 1;
}


static void
aspath_table_print __PF3(fd, FILE *,
			 depth, int,
			 table, asp_table *)
{
    struct print_context ctx;
    char line[BUFSIZ], *pt;

    if (!table)
	return;
    ctx.fd = fd;
    ctx.depth = depth;
    pt = aspath_indent(line, depth - 1);
    sprintf(pt, " {");
    trace_or_print(fd, line);
    span_avl_tree(&table->search, aspath_table_entry_print, &ctx);
    sprintf(line, " }");
    trace_or_print(fd, line);
}


static void
aspath_trans_list_print __PF3(fd, FILE *,
			      depth, int,
			      trans, asp_trans *)
{
    char line[BUFSIZ], *pt;
    asp_trans *t;
    asp_state *s;

    pt = line;
    sprintf(pt, " -> (");
    trace_or_print(fd, line);
    pt = line;
    *pt = 0;
    if (!trans)
	return;
    for (t = trans; t; t = t->next) {
	s = t->state;
	assert(s);
	assert(s->asps_use_count);
	if (s->asps_grp) {
	    sprintf(pt, "[%x]", s->asps_grp);
	    pt = &pt[strlen(pt)];
	}
	sprintf(pt, "%d-%d-> ", s->asps_min, s->asps_max);
	pt = &pt[strlen(pt)];
	if (s->asps_tree) {
	    trace_or_print(fd, line);
	    pt = line;
	    *pt = 0;
	    aspath_table_print(fd, depth + 1, s->asps_tree);
	} else {
	    sprintf(pt, "* ");
	    pt = &pt[strlen(pt)];
	}
    }
    sprintf(pt, ") ");
    pt = &pt[strlen(pt)];
    trace_or_print(fd, line);
}


int
aspath_adv_compare __PF2(psp1, void_t,
			 psp2, void_t)
{
    asmatch_t *asmp1 = (asmatch_t *) psp1;
    asmatch_t *asmp2 = (asmatch_t *) psp2;

    /* XXX - need to be a bit more intellegent about this */
    if (asmp1 == asmp2)
	return 1;
    if (asmp1->origin_mask != asmp2->origin_mask)
	return 0;
    return asp_table_equiv(asmp1->first, asmp2->first);
}


#if defined(TEST_ASMATCH) && !defined(TIME_ASMATCH)
#define debug_asmatch(x, y, z) trace_only_tf(x, y, z)
#else
#define debug_asmatch(x, y, z) /* nada! */
#endif

#define LOCATE_ASP_TABLE(arg1, arg2) \
    (((int) arg1) - (int)(((asp_table *)arg2)->lo_as))

#define push_stack(stack, _tbl, _asp, _t, _iter, _grp, _tmp_r, _tmp_asp) \
{ \
    struct regex_match_frame *Xframe; \
    if ((stack)->frame == &(stack)->blk->frames[MAXFRAMES]) { \
	struct regex_match_stack *Xnewstk \
	    = (struct regex_match_stack *) \
	    task_block_alloc(asp_regex_match_block); \
	Xnewstk->blk = (stack)->blk; \
	(stack)->blk = Xnewstk; \
	(stack)->frame = &Xnewstk->frames[0]; \
    } \
    Xframe = (stack)->frame; \
    Xframe->tbl = (_tbl); \
    Xframe->asp = (_asp); \
    Xframe->t = (_t); \
    Xframe->as_tmp = (_tmp_asp); \
    Xframe->tmp_remain = (_tmp_r); \
    Xframe->iter = (_iter); \
    Xframe->grp = (_grp); \
    debug_asmatch(trace_global, TRC_NOSTAMP, \
		  ("  match: push %x %d %x %d %d %d", \
		   (stack)->frame, \
		   (((stack)->frame - &(stack)->blk->frames[0]) \
		    / sizeof((stack)->frames[0])), \
		   (_t), (_iter), ntohs(*(_asp)), \
		   (_iter) >= 0 ? ntohs(*(_tmp_asp)) : -1)); \
    ++(stack)->frame; \
}

#define pop_stack(stack) \
{ \
    struct regex_match_frame *Xframe; \
    if ((stack)->frame == &(stack)->blk->frames[0]) { \
	struct regex_match_stack *Xoldstk = (stack)->blk; \
	if (Xoldstk == (stack)) \
	    break;		/* this is only used in one place */ \
	(stack)->blk = Xoldstk->blk; \
	task_block_free(asp_regex_match_block, Xoldstk); \
	(stack)->frame = &(stack)->blk->frames[MAXFRAMES]; \
    } \
    --(stack)->frame; \
    Xframe = (stack)->frame; \
    tbl = Xframe->tbl; \
    asp = Xframe->asp; \
    t = Xframe->t; \
    tmp_remaining = Xframe->tmp_remain; \
    as_tmp = Xframe->as_tmp; \
    iter = Xframe->iter; \
    use_grp = Xframe->grp; \
    debug_asmatch(trace_global, TRC_NOSTAMP, \
		  ("  match: pop %x %d %x %d %d %d", \
		   (stack)->frame, \
		   (((stack)->frame - &(stack)->blk->frames[0]) \
		    / sizeof((stack)->frames[0])), \
		   t, iter, ntohs(*asp), \
		   iter >= 0 ? ntohs(*as_tmp) : -1)); \
}

#define cleanup_stack(stack) \
{ \
    struct regex_match_stack *Xoldstk = (stack)->blk; \
    while (Xoldstk != (stack)) { \
	(stack)->blk = Xoldstk->blk; \
	task_block_free(asp_regex_match_block, Xoldstk); \
	Xoldstk = (stack)->blk; \
    } \
}


/*
 *  recoded using iteration
 */
static int
aspath_regex_match __PF3(asp, as_t *,
			 tbl, asp_table *,
			 remaining, u_int)
{
    int lookfor, looktmp, tmp_remaining;
    struct avl_tree *entry;
    asp_table *found;
    asp_trans *t;
    asp_state *state, *use_grp, *grp;
    as_t *as_tmp;
    int iter;
    struct regex_match_stack _stack, *stack;
    
    assert(asp);
    stack = &_stack;
    stack->blk = stack;
    stack->frame = &stack->frames[0];
    iter = -1;
    t = 0;
    state = 0;			/* initializing keeps the compiler happy */
    as_tmp = 0;
    tmp_remaining = remaining;
    use_grp = 0;
    for (;;) {
	debug_asmatch(trace_global, TRC_NOSTAMP,
		      ("match: %x %d %x %d %d %d %d-%d",
		       stack->frame,
		       ((stack->frame - &stack->blk->frames[0])
			/ sizeof(stack->frames[0])),
		       t, iter, ntohs(*asp),
		       iter >= 0 ? ntohs(*as_tmp) : -1,
		       t ? t->state->asps_min : -1,
		       t ? t->state->asps_max : -1));
	if (t == 0 && iter == -1) {
	    /*
	     *  if we are at the end or the RE and the AS path, we succed.
	     *  if we are at the end of one and not the other we fail
	     */
	    if (remaining)
		lookfor = ntohs(*asp);
	    else
		lookfor = -1;
	    if (!tbl) {
		if (lookfor < 0) {
		    cleanup_stack(stack);
		    return 1;
		}
		goto PopStack;
	    }
	    if (lookfor < 0) {
		for (t = tbl->trans; t; t = t->next) {
		    state = t->state;
		    assert(state);
		    if (state->asps_min == 0 && !state->asps_tree) {
			cleanup_stack(stack);
			return 1;
		    }
		}
		goto PopStack;
	    }
	    /*
	     *  find the table entry for the current token
	     */
	    FIND_LESS_OR_EQUAL_AVL_ENTRY(entry, &tbl->search, lookfor,
					 LOCATE_ASP_TABLE, found, asp_table *);
	    if (!found)
		goto PopStack;
	    if (lookfor < found->lo_as || lookfor > found->hi_as)
		goto PopStack;
	    t = found->trans;
	    continue;
	}
	lookfor = ntohs(*asp);
	/*
	 *  each transition represents the use of the current AS
	 *  to enter a different (useful) state
	 */
	state = t->state;
	assert(state);
	if (iter == -1) {
	    if ((!use_grp) && state->asps_grp) {
		if (t->next)
		    push_stack(stack, tbl, asp, t->next, -1,
			       use_grp, 0, (as_t *)0);
		use_grp = state->asps_grp;
	    }
	    /*
	     *  insure that we meet the minimum number of matches criteria
	     */
	    iter = 0;
	    as_tmp = asp;
	    tmp_remaining = remaining;
	    for ( ; iter < state->asps_min; ++as_tmp, --tmp_remaining, ++iter) {
		if (!tmp_remaining)
		    break;
		looktmp = ntohs(*as_tmp);
		if (looktmp == lookfor)
		    continue;
		FIND_LESS_OR_EQUAL_AVL_ENTRY(entry, &tbl->search, looktmp,
					     LOCATE_ASP_TABLE, found,
					     asp_table *);
		if (!found)
		    break;
		if (looktmp < found->lo_as || looktmp > found->hi_as)
		    break;
		grp = state->asps_grp;
		if ((use_grp && (grp != use_grp))
		    || ((!use_grp) && grp))
		    break;
	    }
	    if (iter < state->asps_min) {
		goto NextIter;
	    }
	    /*
	     *  match the rest of the AS path to the rest of the pattern
	     */
	    goto PushStack;
	}
	/*
	 *  see if the current token state can consume more AS numbers
	 */
	if ((!state->asps_max) || (iter <= state->asps_max)) {
	    if (!tmp_remaining)
		goto IncrPushStack;
	    looktmp = ntohs(*as_tmp);
	    if (looktmp == lookfor)
		goto IncrPushStack;
	    FIND_LESS_OR_EQUAL_AVL_ENTRY(entry, &tbl->search, looktmp,
					 LOCATE_ASP_TABLE, found, asp_table *);
	    if (!found)
		goto NextIter;
	    if (looktmp < found->lo_as || looktmp > found->hi_as)
		goto NextContinue;
	    grp = state->asps_grp;
	    if ((use_grp && (grp != use_grp))
		|| ((!use_grp) && grp))
		goto Continue;
	    /*
	     *  we can consume another AS so try to match the rest
	     */
	    goto IncrPushStack;
	}
	goto NextIter;
    NextIter:
	debug_asmatch(trace_global, TRC_NOSTAMP, ("match: NextIter"));
	iter = -1;
	if (!(t = t->next))
	    goto PopStack;
	continue;
    IncrPushStack:
	debug_asmatch(trace_global, TRC_NOSTAMP, ("match: IncrPushStack"));
	if (tmp_remaining) {
	    push_stack(stack, tbl, asp, t, iter + 1,
		       use_grp, tmp_remaining - 1, as_tmp + 1);
	    goto Continue;
	}
    NextContinue:
	t = t->next;
	if (t)
	    push_stack(stack, tbl, asp, t, -1, use_grp, 0, (as_t *)0);
	/* else next depth and then pop */
	goto Continue;
    PushStack:
	debug_asmatch(trace_global, TRC_NOSTAMP, ("match: PushStack"));
	push_stack(stack, tbl, asp, t, iter, use_grp, tmp_remaining, as_tmp);
    Continue:
	tbl = state->asps_tree;
	asp = as_tmp;
	remaining = tmp_remaining;
	iter = -1;
	use_grp = 0;
	t = 0;
	continue;
    PopStack:
	debug_asmatch(trace_global, TRC_NOSTAMP, ("match: PopStack"));
	pop_stack(stack);
	continue;
    }
    return 0;
}


int
aspath_adv_match __PF2(psp, void_t,
		       asp, as_path *)
{
    as_t *p = PATH_SHORT_PTR(asp);
    asmatch_t *asmp = (asmatch_t *) psp;

    if (!asp) {
	return FALSE;
    }
    if (!psp) {
	return TRUE;
    }
    return aspath_regex_match(p, asmp->first, asp->path_len / sizeof(*p));
}


/*
 *  functions originally from avl.c
 */

static struct avl_tree *
find_smallest_avl_entry __PF1(head, struct avl_tree *)
{
    if (!head)
	return 0;
    while (head->back)
	head = head->back;
    if (head->mid)
	head = head->mid;
    return head;
}

static struct avl_tree *
find_largest_avl_entry __PF1(head, struct avl_tree *)
{
    if (!head)
	return 0;
    while (head->forw)
	head = head->forw;
    return head;
}

static struct avl_tree *
    remove_avl_entry(head, e, compare)
struct avl_tree *head;
struct avl_tree *e;
_PROTOTYPE(compare, int, (void *, struct avl_tree *));
{
    int cmp = (*compare)(e, head);
    
    if (cmp > 0) {
	struct avl_tree *mid = head->mid, *forw;
	if (!head->forw) {
	    tracef("remove_avl_entry(): no forw pointer");
	    trace_unload();
	    assert(0);
	}
	forw = remove_avl_entry(head->forw, e, compare);
	if (mid) {
	    mid->back = head->back;
	    head->back = 0;
	    head->mid = 0;
	    head->forw = 0;
	    mid->forw = insert_avl_entry(forw, head, compare);
	    return mid;
	}
	head->mid = find_largest_avl_entry(head->back);
	head->back = remove_avl_entry(head->back, head->mid, compare);
	head->forw = forw;
	return head;
    } else if (cmp < 0) {
	struct avl_tree *mid = head->mid, *forw;
	if (mid) {
	    if ((*compare)(mid, e) == 0) {
		head->mid = 0;
		return head;
	    }
	    if (!head->back) {
		tracef("remove_avl_entry(): mid pointer but no back pointer");
		trace_unload();
		assert(0);
	    }
	    head->back = remove_avl_entry(head->back, e, compare);
	    head->back = insert_avl_entry(head->back, mid, compare);
	    head->mid = 0;
	    return head;
	}
	if (!head->back) {
	    tracef("remove_avl_entry(): no mid pointer or back pointer");
	    trace_unload();
	    assert(0);
	}
	forw = find_smallest_avl_entry(head->forw);
	forw->forw = remove_avl_entry(head->forw, forw, compare);
	forw->back = remove_avl_entry(head->back, e, compare);
	forw->mid = head;
	head->back = 0;
	head->forw = 0;
	return forw;
    } else {
	struct avl_tree *mid = head->mid, *forw;
	if (mid) {
	    mid->back = head->back;
	    mid->forw = head->forw;
	    head->back = 0;
	    head->mid = 0;
	    head->forw = 0;
	    return mid;
	}
	if (!head->forw) {
	    return 0;
	}
	forw = find_smallest_avl_entry(head->forw);
	forw->forw = remove_avl_entry(head->forw, forw, compare);
	forw->mid = find_largest_avl_entry(head->back);
	forw->back = remove_avl_entry(head->back, forw->mid, compare);
	head->back = 0;
	head->forw = 0;
	return forw;
    }
}

static struct avl_tree *
    insert_avl_entry(head, e, compare)
struct avl_tree *head;
struct avl_tree *e;
_PROTOTYPE(compare, int, (void *, struct avl_tree *));
{
    int cmp;

    e->back = 0;
    e->mid = 0;
    e->forw = 0;
    if (!head)
	return e;
    cmp = (*compare)(e, head);
    if (cmp > 0) {
	struct avl_tree *mid = head->mid;
	if (!mid) {
	    struct avl_tree *back = head->back, *forw;
	    if (!back) {
		e->mid = head;
		return e;
	    }
	    forw = insert_avl_entry(head->forw, e, compare);
	    mid = head;
	    head->forw = 0;
	    head->back = 0;
	    head = find_smallest_avl_entry(forw);
	    forw = remove_avl_entry(forw, head, compare);
	    head->back = back;
	    head->mid = mid;
	    head->forw = forw;
	    return head;
	}

	head->mid = 0;
	head->forw = insert_avl_entry(head->forw, e, compare);
	head->back = insert_avl_entry(head->back, mid, compare);
	return head;
    } else if (cmp < 0) {
	struct avl_tree *mid = head->mid;
	if (!mid) {
	    if (!head->back) {
		head->mid = e;
		return head;
	    }
	    mid = find_largest_avl_entry(head->back);
	    if ((*compare)(e, mid) > 0) {
		head->mid = e;
		return head;
	    }
	    head->back = remove_avl_entry(head->back, mid, compare);
	    head->mid = mid;
	    head->back = insert_avl_entry(head->back, e, compare);
	    return head;
	}
	if ((*compare)(e, mid) > 0) {
	    struct avl_tree *forw = head->forw, *back = head->back;
	    head->back = 0;
	    head->mid = 0;
	    head->forw = 0;
	    e->forw = insert_avl_entry(forw, head, compare);
	    e->back = insert_avl_entry(back, mid, compare);
	    return e;
	} else if ((*compare)(e, mid) < 0) {
	    struct avl_tree *forw = head->forw, *back = head->back;
	    head->back = 0;
	    head->mid = 0;
	    head->forw = 0;
	    mid->forw = insert_avl_entry(forw, head, compare);
	    mid->back = insert_avl_entry(back, e, compare);
	    return mid;
	} else {
	    tracef("entry listed more than once: same as mid");
	    free_asp_avl_table_entry(e);
	    return head;
	}
    } else {
	tracef("entry listed more than once: same as head");
	free_asp_avl_table_entry(e);
	return head;
    }
}

static void
    free_avl_entry(entry, freeit)
struct avl_tree *entry;
_PROTOTYPE(freeit, void, (struct avl_tree *));
{
    if (!entry)
	return;
    free_avl_entry(entry->back, freeit);
    free_avl_entry(entry->mid, freeit);
    free_avl_entry(entry->forw, freeit);
    (*freeit)(entry);
}

/*
 *  used recursion.  iteration is faster.
 *  should also be available as a macro.
 */
static int
    span_avl_tree(head, func, arg)
struct avl_tree *head;
_PROTOTYPE(func, int, (struct avl_tree *e, void *arg));
void *arg;
{
    int count, got;

    if (!head)
	return 0;
    if (0 > (count = span_avl_tree(head->back, func, arg)))
	return 0;
    if (0 > (got = span_avl_tree(head->mid, func, arg)))
	return 0;
    count += got;
    if (0 > (got = (*func)(head, arg)))
	return 0;
    count += got;
    if (0 > (got = span_avl_tree(head->forw, func, arg)))
	return 0;
    count += got;
    return count;
}

static struct avl_tree *
    find_less_or_equal_avl_entry(entry, lookfor, compare)
struct avl_tree *entry;
void *lookfor;
_PROTOTYPE(compare, int, (void *, struct avl_tree *));
{
    struct avl_tree *found = 0;
    int cmp;
    
    while (entry) {
	cmp = (*compare)(lookfor, entry);
	if (cmp > 0) {
	    found = entry;	/* unless we find something better */
	    entry = entry->forw;
	} else if (cmp < 0) {
	    if (entry->mid && ((*compare)(lookfor, entry->mid) >= 0)) {
		found = entry->mid;
		break;
	    }
	    entry = entry->back;
	} else {
	    found = entry;
	    break;
	}
    }
    return found;
}
