#ident	"@(#)OSRcmds:ksh/shlib/rjust.c	1.1"
#pragma comment(exestr, "@(#) rjust.c 25.3 93/01/20 ")
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
 * Modification History
 *
 *	L000	scol!markhe	18 Nov 92
 *	- use isblank() to identify a blank character
 */

/*
 *   NAM_RJUST.C
 *
 *   Programmer:  D. G. Korn
 *
 *        Owner:  D. A. Lambeth
 *
 *         Date:  April 17, 1980
 *
 *
 *
 *   NAM_RJUST (STR, SIZE, FILL)
 *
 *      Right-justify STR so that it contains no more than 
 *      SIZE non-blank characters.  If necessary, pad with
 *      the character FILL.
 *
 *
 *
 *   See Also:  
 */

#ifdef KSHELL
#include	"shtype.h"
#else
#include	<ctype.h>
#endif	/* KSHELL */

/*
 *   NAM_RJUST (STR, SIZE, FILL)
 *
 *        char *STR;
 *
 *        int SIZE;
 *
 *        char FILL;
 *
 *   Right-justify STR so that it contains no more than
 *   SIZE characters.  If STR contains fewer than SIZE
 *   characters, left-pad with FILL.  Trailing blanks
 *   in STR will be ignored.
 *
 *   If the leftmost digit in STR is not a digit, FILL
 *   will default to a blank.
 */

void	nam_rjust(str,size,fill)
char *str,fill;
int size;
{
	register int n;
	register char *cp,*sp;
	n = strlen(str);

	/* ignore trailing blanks */
#ifdef	INTL							/* L000 begin */
	for (cp=str+n;n && *--cp && isblank(*cp);n--);
#else
	for (cp=str+n;n && *--cp == ' '; n--);
#endif	/* INTL */						/* L000 end */

	if (n == size) return;
	if(n > size)
        {
        	*(str+n) = 0;
        	for (sp = str, cp = str+n-size; sp <= str+size; *sp++ = *cp++);
        	return;
        }
	else *(sp = str+size) = 0;
	if (n == 0)  
        {
        	while (sp > str)
               		*--sp = ' ';
        	return;
        }
	while(n--)
	{
		sp--;
		*sp = *cp--;
	}
	if(!isdigit(*str))
		fill = ' ';
	while(sp>str)
		*--sp = fill;
	return;
}

