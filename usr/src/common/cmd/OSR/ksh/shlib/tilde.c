#ident	"@(#)OSRcmds:ksh/shlib/tilde.c	1.1"
#pragma comment(exestr, "@(#) tilde.c 25.3 95/01/18 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1990-1995.
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
 *	L001		ashleyb@sco.com			17th Jan 1994
 *	- Use getpwent() to get users login and home directory.
 *	- Removed lots of unused code.
 */

/*
 *  sh_tilde - process tilde expansion
 *
 *   David Korn
 *   AT&T Bell Laboratories
 *   Room 3C-526B
 *   Murray Hill, N. J. 07974
 *   Tel. x7975
 *
 *   February, 1983
 *   revised March, 1988
 */

#include	"sh_config.h"
#ifdef KSHELL
#   include	"defs.h"
#else
    extern char	*strrchr();
    extern char	*strcpy();
#endif	/* KSHELL */

#include <pwd.h>

#define UNAME	20
#define LINSZ	256
static char u_name[UNAME];
static char u_logdir[PATH_MAX];					/* L001 */


char	*logdir();

/*
 * This routine is used to resolve ~ filenames.
 * If string starts with ~ then ~name is replaced with login directory of name.
 * A ~ by itself is replaced with the users login directory.
 * A ~- is replaced by the last working directory in Shell.
 * If string doesn't start with ~ then NULL returned.
 * If not found then the NULL string is returned.
 */

char *sh_tilde(string)
char *string;
{
	register char *sp = string;
	register char *cp;
	register int c;
	if(*sp++!='~')
		return(NULL);
	if((c = *sp)==0 || c=='/')
	{
		return("$HOME");
	}
#ifdef KSHELL
	if((c=='-' || c=='+') && ((c= *(sp+1))==0 || c=='/'))
	{
		if(*sp=='+')
			return("$PWD");
		else
			return("$OLDPWD");
	}
#endif	/* KSHELL */
	if((cp=(char *)strrchr(sp,'/')) != NULL)
		*cp = 0;
	sp = logdir(sp);
	if(cp)
		*cp = '/';
	return(sp);
}


/*
 * This routine returns a pointer to a null-terminated string that
 * contains the login directory of the given <user>.
 * NULL is returned if there is no entry for the given user in the
 * /etc/passwd file or if no room for directory entry.
 * The most recent login directory is saved for future access
 */

char *logdir(user)
char *user;
{
	if(strcmp(user,u_name))
	{
		struct passwd *pw;				/* L001 Begin */

		if ((pw = getpwnam(user)) == (struct passwd *)NULL)
			return((char *)NULL);
		strcpy(u_logdir, pw->pw_dir);
		strcpy(u_name, pw->pw_name);			/* L001 End */
	}
	return(u_logdir);
}
