#ident	"@(#)myfopen.c	11.1"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident "@(#)myfopen.c	1.13 'attmail mail(1) command'"
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include "rcv.h"

#undef	fopen
#undef	fclose

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Local version of fopen() and fclose(). These maintain a list of
 * file pointers which can be run down when we need to close
 * all files, such as before executing external commands.
 */

static void addnode ARGS((FILE *fp));
static void delnode ARGS((FILE *fp));

/* add a new node to the list */
static void
addnode(fp)
FILE *fp;
{
	register NODE *new = (NODE *)malloc(sizeof(NODE));
	nomemcheck(new);
	new->fp = fp;
	new->next = fplist;
	fplist = new;
}

/* delete the given NODE from the list */
static void
delnode(fp)
FILE *fp;
{
	register NODE *cur, *prev;

	for (prev = cur = fplist; cur != (NODE *)NULL; prev = cur, cur = cur->next) {
		if (cur->fp == fp) {
			if (cur == fplist) {
				fplist = cur->next;
			} else {
				prev->next = cur->next;
			}
			free((char*)cur);
			break;
		}
	}
}

/* open a file and record it */
FILE *
my_fopen(filename, mode)
const char *filename, *mode;
{
	FILE *fp;

	if ((fp = fopen(filename, mode)) != (FILE *)NULL)
		addnode(fp);
	return(fp);
}

/* close a file and remove our record of it */
int
my_fclose(iop)
register FILE *iop;
{
	if (!iop) return -1;
	delnode(iop);
	return fclose(iop);
}
