#ident	"@(#)OSRcmds:ksh/shlib/unassign.c	1.1"
#pragma comment(exestr, "@(#) unassign.c 25.3 93/01/20 ")
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
 *   UNASSIGN.C
 *
 *   Programmer:  D. G. Korn
 *
 *        Owner:  D. A. Lambeth
 *
 *         Date:  April 17, 1980
 *
 *
 *   UNASSIGN (NP)
 *        
 *        Nullify the value and the attributes of the namnod
 *        given by NP.
 *
 *   See Also:  nam_putval(III), nam_longput(III), nam_strval(III)
 */

#include	"defs.h"					/* L000 */
/* #include	"name.h" */					/* L000 */


/*
 *   NAM_FREE (NP)
 *
 *       struct namnod *NP;
 * 
 *   Set the value of NP to NULL, and nullify any attributes
 *   that NP may have had.  Free any freeable space occupied
 *   by the value of NP.  If NP denotes an array member, it
 *   will retain its attributes.  Any node that has the
 *   indirect (N_INDIRECT) attribute will retain that attribute.
 */

extern void	free();

void	nam_free(np)
register struct namnod *np;
{
	register union Namval *up = &np->value.namval;
	register struct Nodval *nv = 0;
	int next;
#ifdef NAME_SCOPE
	if (nam_istype (np, N_CWRITE))
	{
		np->value.namflg |= N_AVAIL;
		return;
	}
#endif /* NAME_SCOPE */
	do
	{
		if (nam_istype (np, N_ARRAY))
		{
			if((nv=array_find(np,A_DELETE))==0)
				return;
			up = &nv->namval;
		}
		if (nam_istype (np, N_INDIRECT))
			up = up->up;
		if ((!nam_istype (np, N_FREE)) && (!isnull (np)))
			free(up->cp);
		up->cp = NULL;
		if (nam_istype (np, N_ARRAY))
			next = array_next(np);
		else
		{
			np->value.namflg &= N_INDIRECT;
			np->value.namsz = 0;
			next = 0;
		}
		if(nv)
			free((char*)nv);
	}
	while(next);
}
