#ident	"@(#)OSRcmds:csh/getpwnam.c	1.1"
/*
 *	@(#) getpwnam.c 1.4 88/11/11 
 *
 *	      UNIX is a registered trademark of AT&T
 *		Portions Copyright 1976-1989 AT&T
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1989 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */
/*	Copyright (c) 1984, 1986, 1987, 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

/* #ident	"@(#)csh:getpwnam.c	1.2" */
#pragma comment(exestr, "@(#) getpwnam.c 1.4 88/11/11 ")

/*
 *	@(#) getpwnam.c 1.1 88/03/29 csh:getpwnam.c
 */
/*
 * Modified version of getpwnam which doesn't use stdio.
 * This is done to keep it (and hence csh) small at a small
 * cost in speed.
 */
#include <pwd.h>
#include <whoami.h>

#ifdef UCB_PWHASH
#include <sys/pwdtbl.h>
#include <sys/stat.h>
#define pwf _pw_file

static char	NTBL[] = "/etc/nam_pw_map";	/* name of name table */
static int	nf = -1;			/* name table */
static int	nusers;				/* number of users */
static off_t	nameloc ();			/* finds name from table */
struct stat	sbuf;				/* stat buffer */
#endif

extern int	pwf;				/* password file */

struct passwd *
getpwnam(name)
char *name;
{
	register struct passwd *p;
	struct passwd *getpwent();

#ifdef UCB_PWHASH
	off_t loc;
#endif

	if (pwf == -1)
		setpwent();
#ifdef UCB_PWHASH
	loc = nameloc (name);
	if (loc >= 0)  {
		lseek (pwf,loc,0);
		p = getpwent();
		if (strcmp(p->pw_name,name) == 0)
			return (p);
	}
#endif

						/* begin linear search */
	setpwent();
	while( (p = getpwent()) && strcmp(name,p->pw_name) );
	endpwent();
	return(p);
}

#ifdef UCB_PWHASH
static off_t
nameloc (name)				/* finds name from name->loc table */
char	*name;
{
	off_t		high;			/* high pointer to file */
	off_t		low;			/* low pointer to file */
	off_t		test;			/* current pointer to file */
	struct pwloc	p;			/* table entry from file */
	int		n;			/* temporary variable */

						/* get status of name table
						 * file and open it */
	if (nf == -1)  {
		if (stat (NTBL,&sbuf) || (nf = open (NTBL,0)) == -1)
			return ((off_t) -1);

						/* compute number of entries */
		nusers = sbuf.st_size/sizeof(struct pwloc);
	}

	low = 0;				/* initialize pointer */
	high = nusers;

compare:
	test = (high+low+1)/2;			/* find midpoint */
	if (test == high)			/* no valid midpoint */
		return ((off_t) -1);

						/* read table entry */
	lseek (nf,test*sizeof (struct pwloc),0);
	if (read(nf, &p, sizeof (struct pwloc)) != sizeof (struct pwloc))
		return ((off_t) -1);

						/* return if location found */
	if ((n = strcmp8 (p.pwl_name,name)) == 0)
		return (p.pwl_loc);
	
	if (n < 0)				/* adjust pointers */
		low = test;
	else
		high = test;
	goto compare;
}

strcmp8 (a,b)
register char	*a, *b;
{
	register char	*l;
	register int	n;

	l = a+8;
	while (a < l && (n = (*a++)-(*b++)) == 0);
	return (n);
}
#endif
