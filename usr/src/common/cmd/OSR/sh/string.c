#ident	"@(#)OSRcmds:sh/string.c	1.1"
#pragma comment(exestr, "@(#) string.c 25.1 92/09/16 ")
/*
 *	      UNIX is a registered trademark of AT&T
 *		Portions Copyright 1976-1989 AT&T
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1992 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */
/*	Copyright (c) 1984, 1986, 1987, 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)sh:string.c	1.4" */
/*
 * UNIX shell
 */

/*
 * Modification History
 *
 *	L000	scol!markhe	15th March, 1992
 *	- two new functions to support symbolic mode arguments for umask
 *	  (see module xec.c, mod L005)
 *
 */

#include	"defs.h"
#include	<sys/types.h>					/* L000 */
#include	<sys/stat.h>					/* L000 */
#include	"../include/sym_mode.h"				/* L000 */

static char permstr[18];	/* space to hold symbolic mode  L000 */

/* ========	general purpose string handling ======== */


unsigned char *
movstr(a, b)
register unsigned char	*a, *b;
{
	while (*b++ = *a++);
	return(--b);
}

any(c, s)
register unsigned char	c;
unsigned char	*s;
{
	register unsigned char d;

	while (d = *s++)
	{
		if (d == c)
			return(TRUE);
	}
	return(FALSE);
}

cf(s1, s2)
register unsigned char *s1, *s2;
{
	while (*s1++ == *s2)
		if (*s2++ == 0)
			return(0);
	return(*--s1 - *s2);
}

length(as)
unsigned char	*as;
{
	register unsigned char	*s;

	if (s = as)
		while (*s++);
	return(s - as);
}

unsigned char *
movstrn(a, b, n)
	register unsigned char *a, *b;
	register int n;
{
	while ((n-- > 0) && *a)
		*b++ = *a++;

	return(b);
}


int								/* L000 begin */
strperm(char *expr, mode_t old_umask, mode_t *new_umask)
{
	action_atom *action;

	if ((action = comp_mode(expr, 0)) == (action_atom *) 0)
		return(-1);

	*new_umask = exec_mode(old_umask, action);

	free((char *) action);

	return(1);
}

/*
 * produce a symbolic string based upon the file mode creation mask
 */
char *
permtostr(register mode_t flag)
{
	register char *str = permstr;
	int i;

	flag = ~flag;

	for (i=0; i < 3; flag = (flag << 3) & 0777, i++) {
		switch(i) {
			case 0 :
				*str++ = 'u';
				break;
			case 1 :
				*str++ = 'g';
				break;
			default:
				*str++ = 'o';
				/* FALLTHROUGH */
		}
		*str++ = '=';

		if (flag & S_IRUSR)
			*str++ = 'r';
		if (flag & S_IWUSR)
			*str++ = 'w';
		if (flag & S_IXUSR)
			*str++ = 'x';
		*str++ = ',';
	}
	*--str = '\0';

	return(permstr);
}								/* L000 end */
