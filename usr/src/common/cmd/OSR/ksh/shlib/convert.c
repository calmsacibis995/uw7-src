#ident	"@(#)OSRcmds:ksh/shlib/convert.c	1.1"
#pragma comment(exestr, "@(#) convert.c 25.4 94/08/24 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1990-1994.
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
 *	L000	scol!markhe	12 Nov 92
 *	- the toupper(), tolower() macros in <ctype.h> check case before
 *	  converting, so reduce code in ltou() and utol()
 *	L001	scol!anthonys	24 Aug 94
 *	- correct the argument passed to toupper() and tolower().
 */
/*
 *   CONVERT.C
 *
 *
 *   LTOU (STR1, STR2)
 *        Copy STR1 to STR2, changing lower case to upper case.
 *
 *   UTOL (STR1, STR2)
 *        Copy STR1 to STR2, changing upper case to lower case.
 */

#ifdef KSHELL
#include	"shtype.h"
#else
#include	<ctype.h>
#endif	/* KSHELL */

/* 
 *   LTOU (STR1, STR2)
 *        char *STR1;
 *        char *STR2;
 *
 *   Copy STR1 to STR2, converting uppercase alphabetics to
 *   lowercase.  STR2 should be big enough to hold STR1.
 *
 *   STR1 and STR2 may point to the same place.
 *
 */

void ltou(str1,str2)
register char *str1,*str2;
{
	for(; *str1; str1++,str2++)
	{
#ifdef	INTL							/* L000 */
		*str2 = toupper((unsigned char)*str1);		/* L000 */ /* L001 */
#else								/* L000 */
		if(islower(*str1))
			*str2 = toupper((unsigned char)*str1);	/* L001 */
		else
			*str2 = *str1;
#endif	/* INTL */						/* L000 */
	}
	*str2 = 0;
}


/*
 *   UTOL (STR1, STR2)
 *        char *STR1;
 *        char *STR2;
 *
 *   Copy STR1 to STR2, converting lowercase alphabetics to
 *   uppercase.  STR2 should be big enough to hold STR1.
 *
 *   STR1 and STR2 may point to the same place.
 *
 */

void utol(str1,str2)
register char *str1,*str2;
{
	for(; *str1; str1++,str2++)
	{
#ifdef	INTL							/* L000 */
		*str2 = tolower((unsigned char)*str1);		/* L000 */ /* L001 */
#else								/* L000 */
		if(isupper(*str1))
			*str2 = tolower((unsigned char)*str1);	/* L001 */
		else
			*str2 = *str1;
#endif	/* INTL */						/* L000 */
	}
	*str2 = 0;
}

