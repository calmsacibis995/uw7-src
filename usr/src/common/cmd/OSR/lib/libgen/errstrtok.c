#ident	"@(#)OSRcmds:lib/libgen/errstrtok.c	1.1"
/*
 *	@(#) errstrtok.c 20.1 94/12/04 
 *
 *	      UNIX is a registered trademark of AT&T
 *		Portions Copyright 1976-1989 AT&T
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1995 The Santa Cruz Operation, Inc.
 *		      All Rights Reserved
 */
/*	Copyright (c) 1984, 1986, 1987, 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)libgen:errstrtok.c	1.3" */
#pragma comment(exestr, "@(#) errstrtok.c 20.1 94/12/04 ")

#ifdef __FROM_USL
#include <synonyms.h>
#endif

/*	This is exactly strtok(3C).
	It is here, with a different name, because using strtok() within
	the scope of errverb() can cause the loss of a currently remembered
	string (savept).
*/

/*LINTLIBRARY*/
/*
 * uses strpbrk and strspn to break string into tokens on
 * sequentially subsequent calls.  returns NULL when no
 * non-separator characters remain.
 * `subsequent' calls are calls with first argument NULL.
 */

#define	NULL	(char*)0

extern int strspn();
extern char *strpbrk();

char *
errstrtok(string, sepset)
char	*string, *sepset;
{
	register char	*p, *q, *r;
	static char	*savept;

	/*first or subsequent call*/
	p = (string == NULL)? savept: string;

	if(p == 0)		/* return if no tokens remaining */
		return(NULL);

	q = p + strspn(p, sepset);	/* skip leading separators */

	if(*q == '\0')		/* return if no tokens remaining */
		return(NULL);

	if((r = strpbrk(q, sepset)) == NULL)	/* move past token */
		savept = 0;	/* indicate this is last token */
	else {
		*r = '\0';
		savept = ++r;
	}
	return(q);
}
