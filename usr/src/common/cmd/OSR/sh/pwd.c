#ident	"@(#)OSRcmds:sh/pwd.c	1.1"
#pragma comment(exestr, "@(#) pwd.c 25.4 95/03/09 ")
/*
 *	      UNIX is a registered trademark of AT&T
 *		Portions Copyright 1976-1989 AT&T
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1995 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */
/*	Copyright (c) 1984, 1986, 1987, 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)sh:pwd.c	1.4" */
/* 
 *	UNIX shell
 *
 *	Modification History
 *
 *	L001	scol!harveyt	22nd July 1991
 *	- Long pathname support; changed MAXPWD from 512 to PATHSIZE
 *	  and included sys/param.h for this value.
 *	- Quadrupled the number of "../" in dotdot string, so we can
 *	  initially compute a much greater directory depth (100 dirs deep).
 *
 *	L002	scol!markhe	5th March, 1992
 *	- correct the behaviour of cd-ing along symbolic links.
 *	  Symbolic links are now expanded out, giving `cd symlink' under
 *	  bourne shell the same behaviour as `cd -P symlink' under korn
 *	  shell.  With each `cd pathname', the components of pathname
 *	  are check to see if they are symlinks.  This has no noticeable
 *	  effect on the speed of `cd', and is the safest way to handle
 *	  symlinks.
 *
 *	L003	scol!ianw	4th August 1993
 *	- netsysent() (which calls the xnet_sys() system call) when called
 *	  with a command of ISREMOTE returns 1 if remote and zero if not.
 *	  Corrected the check of the return value from netsysent(), there
 *	  is only a remote machine name to copy if 1 is returned. Also
 *	  removed the unnecessary first call to netsysent().
 *
 *	S004	sco!darinc	16 November 1993
 *	- Fix L002 for symlinks created by automounter for direct map
 *	  entries which can be 1K in size!  readlink(S) returns the size
 *	  of the link (1K) and can overflow the buffer if the size is used
 *	  without care.
 *
 *	S005	sco!vinces	16 March, 1995
 *	- Added logical path processing to argument syntax:
 *	  - rewrote cwdprint()
 *	  - removed cwd(), rmslash(), pwd(), remote(), netname(), del_mchnm(),
 *	    skip_delim(), and netsyscall()
 *	  - added new function cd()
 *	  - modified pwdalmost()
 */

#include		"mac.h"
#include	<sys/types.h>
#include	<dirent.h>
#include	<sys/stat.h>
#include	<sys/param.h>		/*L001*/
#include	"../include/osr.h"

/*
*	S000 start
*	this is really from XNET file nsysent.h but
*	that's not currently available to the build so -
*/
#define ISREMOTE	0x0086		/* is current dir remote */
#define REMCD		0x0093		/* get remote current directory */
/* Network system call */
#define netsysent(command, argp)	netsyscall(command, argp, 0, 0)

struct isnet
{
	int	n_delsiz;
	char	n_del1;
	char	n_del2;
	char	n_netnm[16];
};
/*S000 end */

#define	DOT		'.'
#undef NULL			/* L001 - sys/param.h defines NULL */
#define NULL		0
#define	SLASH		'/'
/*								   L002 begin
 * as a pathname is not always fully processed before being written/appended
 * to cwdname[], and the fact that symbolic links can expand or contract a
 * pathname's length, allow cwdname to be a little larger
 * (ie. give cwdname a temporary overflow space)
 */
#define MAXPWD		(PATHSIZE+(PATHSIZE/2))	/* L001 */	/* L002 end */
#define MACH_MAX	19	 /* 16 chars, 2 delimiters and a null S000 */

extern unsigned char	longpwd[];
static char machine[MACH_MAX];					/* S000 */
static struct isnet net;

static unsigned char cwdname[MAXPWD];

								/* S005 begin */
/*
 *	Print the current working directory.
 */
cwdprint(physical)
unsigned int physical;
{
	unsigned char tmp_cwdname[MAXPWD];

	memset(tmp_cwdname, 0, MAXPWD-1);
        if (physical) {
                if (getcwd(tmp_cwdname, MAXPWD-1) < 0) {
                        error("cwdprint");
                }
        }
        else {
                if (!strlen((char *)cwdname)) {
                        if (getcwd(cwdname, MAXPWD-1) < 0) {
                                error("cwdprint");
                        }
                }
                strcpy((char *)tmp_cwdname, (char *)cwdname);
        }

	prs_buff(tmp_cwdname);
	prc_buff(NL);
	return;
}								/* S005 end */

/*								   L002 begin
 * We have an `almost' valid path in cwdname.
 * Components in the path could be symbolic links that need expanding
 * This module was derived from path_physical() in the korn shell
 */
static
								/* S005 */
pwdalmost(unsigned char *cwdname, unsigned char *validto, unsigned int physical)
{
	register unsigned char *cp = validto;
	register char *savecp;
	struct stat buf;
	char buffer[PATHSIZE];
	int c, n;

	memset(buffer,0,sizeof buffer);				/* S004 */
	while (*cp) {
		/* skip over '/' */
		savecp = (char *)cp+1;
		while (*cp == '/')
			cp++;
		/* eliminate multiple slashes */
		if ((char *)cp > savecp) {
			movstr(cp, savecp);
			cp = (unsigned char *)savecp;
		}
		/* check for .. */
		if (*cp == '.') {
			switch(cp[1]) {
				case '\0':
				case '/':
					/* eliminate /. */
					cp--;
					movstr(cp+2, cp);
					continue;
				case '.':
					if (cp[2] == '/' || cp[2] == '\0') {
						/* backup, but not past root */
						savecp = (char *)cp+2;
						cp--;
						while (cp > cwdname && *--cp != '/')
							;
						movstr(savecp, cp);
								/* S005 begin */
						if (lstat((char *)cwdname, &buf) == -1 || (S_ISLNK(buf.st_mode) && (n = readlink(cwdname, buffer, PATHSIZE)) == -1)) {
							return;
						}		/* S005 end */
						continue;
					}
					break;
			}
		}
		savecp = (char *)cp;
		/* goto end of component */
		while (*cp && *cp != '/')
			cp++;
		c = *cp;
		*cp = '\0';
		if (lstat((char *)cwdname, &buf) == -1 || (S_ISLNK(buf.st_mode) && (n = readlink(cwdname, buffer, PATHSIZE)) == -1)) {
			return;
		}

		*cp = c;
		if (S_ISLNK(buf.st_mode) && n > 0 && physical)	/* S005 */
		{
			/* process a symbolic link */
	/* The automounter, for a direct map entry, creates a symlink  S004
	 * 1K (aka PATHSIZE) in size.  Don't just blindly write into
	 * buffer without checking the size.  If overflow, try to recover
	 * by setting n to the length of the pathname returned by readlink().
	 * Now let's just hope that the data in the block holding the
	 * automounter's symlink is zero'ed out so that strlen() returns
	 * a reasonable value.
	 */
			if (n+strlen((char *)cp) >= PATHSIZE) {		/* S004 v */
				n=strlen(buffer);
				if (n+strlen((char *)cp) >= PATHSIZE) {
					return;
				}
			}					/* S004 ^ */
			movstr(cp, buffer+n);
			if (*buffer == '/') {
				movstr(buffer, cwdname);
				cp = cwdname;
			} else {
				/* check for path overflow */
				cp = (unsigned char *)savecp;
				if ((length(buffer)+(cp-cwdname)-1) >= MAXPWD) {
					return;
				}
				movstr(buffer, cp);
			}
		}
	}
	if (cp==cwdname) {
		*cp = '/';
		*++cp = '\0';
	} else
		while (--cp > cwdname && *cp == '/')
			/* eliminate trailing slashes */
			*cp = '\0';

	/* all of cwdname is now valid */
	validto = cp;

	return;
}								/* L002 end */

								/* S005 begin */
cd(dir, physical)
	register char *dir;
	unsigned int physical;
{
	register rval = -1;
        unsigned char tmp_cwdname[MAXPWD], *validto;

        if (!physical) {
                memset((char *)tmp_cwdname, 0, MAXPWD);

                if (*dir == '/' ) {
                        strcpy((char *)tmp_cwdname, dir);
			validto = tmp_cwdname;
		}
                else {
                        if (!strlen((char *)cwdname))
                                getcwd(cwdname, MAXPWD-1);
                        sprintf(tmp_cwdname, "%s/%s", cwdname, dir);
			validto = &tmp_cwdname[strlen((char *)cwdname)];
                }

                pwdalmost(tmp_cwdname, validto, physical);
        }
        else
                strcpy((char *)tmp_cwdname, dir);

        if ((rval = physical_cd(tmp_cwdname)) >= 0) {
                if (physical)
                        getcwd(cwdname, MAXPWD-1);
                else
			strcpy((char *)cwdname, (char *)tmp_cwdname);
	}

	return(rval);
}								/* S005 end */
