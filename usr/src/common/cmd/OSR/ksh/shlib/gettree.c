#ident	"@(#)OSRcmds:ksh/shlib/gettree.c	1.1"
#pragma comment(exestr, "@(#) gettree.c 25.3 93/01/20 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1990-1993.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*									*/
/*		   Copyright (C) AT&T, 1984-1992			*/
/*			All Rights Reserved				*/
/*									*/
/*	  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T.		*/
/*	    The copyright notice above does not evidence any		*/
/*	   actual or intended publication of such source code.		*/
/*									*/

/*
 *  Modification History
 *
 *	L000	scol!markhe	25 Nov 92
 *	- "name.h" needs the definition of 'MSG', so include "defs.h" (which
 *	  also happens to include "name.h")
 */

/*
 *   GETTREE.C
 *
 *   Programmer:  D. A. Lambeth
 *
 *        Owner:  D. A. Lambeth
 *
 *         Date:  April 17, 1980
 *
 *
 *
 *   GETTREE (MSIZE)
 *
 *        Create a shell associative memory with MSIZE buckets,
 *        and return a pointer to the root of the memory.
 *        MSIZE must be a power of 2.
 *
 *
 *
 *   See Also:  nam_link(III), nam_search(III), libname.h
 */

#include	"defs.h"					/* L000 */

/*
 *   GETTREE (MSIZE)
 *
 *      int MSIZE;
 *
 *   Create an associative memory containing MSIZE headnodes or
 *   buckets, and return a pointer to the root of the memory.
 *
 *   Algorithm:  Memory consists of a hash table of MSIZE buckets,
 *               each of which holds a pointer to a linked list
 *               of namnods.  Nodes are hashed into a bucket by
 *               namid.
 */

struct Amemory *gettree(msize)
register int msize;
{
	register struct Amemory *root;

	--msize;
	root = new_of(struct Amemory,msize*sizeof(struct namnod*));
	root->memsize = msize;
	root->nexttree = NULL;
	while (msize>=0)
		root->memhead[msize--] = NULL;
	return (root);
}
