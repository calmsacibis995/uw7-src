#ident	"@(#)OSRcmds:csh/getpwent.c	1.1"
/*
 *	@(#) getpwent.c 1.4 88/11/11 
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

/* #ident	"@(#)csh:getpwent.c	1.2" */
#pragma comment(exestr, "@(#) getpwent.c 1.4 88/11/11 ")

/*
 *	@(#) getpwent.c 1.1 88/03/29 csh:getpwent.c
 */
/*
 * Modified version of getpwent which doesn't use stdio.
 * This is done to keep it (and hence csh) small at a small
 * cost in speed.
 *
 * This version also uses the UCB hashed password file if whoami.h
 * indicates that UCB_PWHASH is available.  In any case if it fails
 * it tries the regular linear search.
 *
 * Define BBGETPW (Bare Bones GETPW) if all you are interested in is
 * the name, uid, and directory fields.  This will make getpwent smaller
 * and a bit faster and is useful for things like csh.
 */
#include <pwd.h>
#include <whoami.h>

#ifdef UCB_PWHASH
#define pwf _pw_file				/* unlikely symbol name */
#endif

/*
 * predefined strings
 */

#define BUFSIZ 160

int	pwf = -1;			/* password file pointer */

char	line[BUFSIZ+1];			/* input buffer */

struct passwd	passwd;			/* password entry */

setpwent()
{
	if( pwf == -1 )
		pwf = open( "/etc/passwd", 0 );
	else
		lseek(pwf, 0l, 0);
}

endpwent()
{
	if( pwf != -1 ){
		close( pwf );
		pwf = -1;
	}
}

static char *
pwskip(p)
register char *p;
{
	while( *p && *p != ':' )
		++p;
	if( *p ) *p++ = 0;
	return(p);
}

struct passwd *
getpwent()
{
	register char *p, *q;
	register int i, j;

	if (pwf == -1) {
		if( (pwf = open( "/etc/passwd", 0 )) == -1 )
			return(0);
	}
	i = read(pwf, line, BUFSIZ);
	for (j = 0; j < i; j++)
		if (line[j] == '\n')
			break;
	if (j >= i)
		return(0);
	line[++j] = 0;
	lseek(pwf, (long) (j - i), 1);
	p = line;
	passwd.pw_name = p;
	p = pwskip(p);
#ifndef BBGETPW
	passwd.pw_passwd = p;
#endif
	p = pwskip(p);
	passwd.pw_uid = atoi(p);
	p = pwskip(p);
#ifndef BBGETPW
	passwd.pw_gid = atoi(p);
	passwd.pw_quota = 0;
	passwd.pw_comment = "";
#endif
	p = pwskip(p);
#ifndef BBGETPW
	passwd.pw_gecos = p;
#endif
	p = pwskip(p);
	passwd.pw_dir = p;
#ifndef BBGETPW
	p = pwskip(p);
	passwd.pw_shell = p;
 	while(*p && *p != '\n') p++;
	*p = '\0';
#endif
	return(&passwd);
}
