#ident	"@(#)OSRcmds:sh/expand.c	1.2"
#pragma comment(exestr, "@(#) expand.c 25.2 95/03/20 ")
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

/*
 *	MODIFICATION HISTORY
 *
 *	L001	scol!hughd	25aug94
 *	- when built ELF with DLLs (or with any new libraries?),
 *	  "echo *" corrupted pwd's cwdname[] and other fields
 *	- the Bourne shell uses the library readdir(), but provides
 *	  its own versions of opendir() and closedir(), presumably to
 *	  avoid problems with its own malloc(); and the latest readdir()
 *	  assumes that opendir() has allocated DIR and dirent adjacently
 *	- so use struct dirplus from DS's dirm.h, to force these adjacent
 *	L002	scol!clareoa	16 Mar 95
 *	- Modified opendir() and closedir() to handle versioned files.
 */

/* #ident	"@(#)sh:expand.c	1.4" */
/*
 *	UNIX shell
 *
 */

#include	"defs.h"
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<dirent.h>
#include	<fcntl.h>					/* L002 */
#include	<errno.h>					/* L002 */

#define	SHOWVFLAG	0x40000000				/* L002 */

char *getenv ();						/* L002 */

/*
 * globals (file name generation)
 *
 * "*" in params matches r.e ".*"
 * "?" in params matches r.e. "."
 * "[...]" in params matches character class
 * "[...a-z...]" in params matches a through z.
 *
 */
extern int	addg();


expand(as, rcnt)
	unsigned char	*as;
{
	int	count; 
	DIR	*dirf;
	BOOL	dir = 0;
	unsigned char	*rescan = 0;
	unsigned char 	*slashsav = 0;
	register unsigned char	*s, *cs;
	int quotflag = 0;
	int quotflag2 = 0;
	unsigned char *s2 = 0;
	struct argnod	*schain = gchain;
	struct stat statb;
	BOOL	slash;

	if (trapnote & SIGSET)
		return(0);
	s = cs = as;
	/*
	 * check for meta chars
	 */
	{
		register BOOL open;

		slash = 0;
		open = 0;
		do
		{
			switch (*cs++)
			{
			case 0:
				if (rcnt && slash)
					break;
				else
					return(0);

			case '/':
				slash++;
				open = 0;
				continue;

			case '[':
				open++;
				continue;

			case ']':
				if (open == 0)
					continue;

			case '?':
			case '*':
				if (rcnt > slash)
					continue;
				else
					cs--;
				break;


			case '\\':
				cs++;
			default:
				continue;
			}
			break;
		} while (TRUE);
	}

	for (;;)
	{
		if (cs == s)
		{
			s = nullstr;
			break;
		}
		else if (*--cs == '/')
		{
			cs--;
			if(cs >= s && *cs == '\\') {
				*cs = 0; /* null out backslash before / */
				slash++; /* increment slash count because
					    slash will be unquoted in expansion
					    string */
				quotflag = 1;
			} else
				*++cs = 0;
			if (s == cs)
				s = (unsigned char *)"/";
			else {
				s2 = cpystak(s);
				trim(s2);
				s = s2;
			}
			break;
		}
	}

	if ((dirf = opendir(*s ? (char *)s : ".")) != 0)
		dir++;

	/* Let s point to original string because it will be trimmed later */
	if(s2)
		s = as;
	count = 0;
	if (*cs == 0) {
		slashsav = cs++; /* remember where first slash in as is */
		if(quotflag)
			cs++; /* advance past / */
	}
	if (dir)		/* check for rescan */
	{
		register unsigned char *rs;
		struct dirent *e;

		rs = cs;
		do /* find next / in as */
		{
			if (*rs == '/')
			{
				if(*--rs != '\\') {
					rescan = ++rs;
					*rs = 0;
				} else {
					quotflag2 = 1;
					*rs = 0;
					rescan = rs + 1; /* advance past / */
				}
				gchain = 0;
			}
		} while (*rs++);

		while ((e = readdir(dirf)) && (trapnote & SIGSET) == 0)
		{
			if (e->d_name[0] == '.' && *cs != '.')
				continue;

			if (gmatch(e->d_name, cs))
			{
				addg(s, e->d_name, rescan, slashsav);
				count++;
			}
		}
		(void)closedir(dirf);

		if (rescan)
		{
			register struct argnod	*rchain;

			rchain = gchain;
			gchain = schain;
			if (count)
			{
				count = 0;
				while (rchain)
				{
					count += expand(rchain->argval, slash + 1);
					rchain = rchain->argnxt;
				}
			}
			if(quotflag2)
				*--rescan = '\\';
			else
				*rescan = '/';
		}
	}

	if(slashsav) {
		if(quotflag)
			*slashsav = '\\';
		else
			*slashsav = '/';
	}
	return(count);
}

/*
	opendir -- C library extension routine

*/

extern int	close(), fstat();

#define NULL	0

struct dirplus {		/* from DS dirm.h */	/* L001 begin */
	DIR	dirdir;
	union {
		struct dirent align;
		char buf[DIRBUF];
	} u;
	int	lock[2];	/* for safety (dependent on ifdefs) */
};

DIR *
opendir(const char *filename)
{
	static struct dirplus dirplus;
	register DIR	*dirp = &dirplus.dirdir;	/* L001 end */
	register int	fd;		/* file descriptor for read */
	struct stat	sbuf;		/* result of fstat() */
	char 		*p;					/* L002 begin */

	if ( (fd = open( filename, O_RDONLY|O_NONBLOCK )) < 0 )
		return NULL;
	/* POSIX mandated behavior
	 * close on exec if using file descriptor 
 	 */
	if (fcntl(fd, F_SETFD, FD_CLOEXEC) < 0) 
		return NULL;
	if ((fstat (fd, &sbuf) < 0) || ((sbuf.st_mode & S_IFMT) != S_IFDIR))
	{
		if ((sbuf.st_mode & S_IFMT) != S_IFDIR)
			errno = ENOTDIR;
		
		(void)close( fd );
		return NULL;		
	}

	dirp->dd_fd = fd;
	dirp->dd_loc = dirp->dd_size = 0;	/* refill needed */
	dirp->dd_buf = dirplus.u.buf;			/* L001 */

	/* SHOWVERSIONS specifies whether the versioned files are generally
	 * visible or not.
	 * If SHOWVERSIONS=0, only the current version is visible
	 * or no versions are shown if there is no current version).
	 * If SHOWVERSIONS=1, all versions are visible. Versioned files are
	 * represented as filename;version
	 */

	if ((p = getenv("SHOWVERSIONS")) && (*p == '1') && (*(p+1) == '\0')) {

		/* undelete support assumes that there will never be a
		 * directory 0x40000000 bytes (SHOWVFLAG) in size, or larger.
		 * The library call will try and seek to this offset.
		 * If the underlying filesystem supports versioning, it will
		 * provide directory entries starting at this offset. If it
		 * doesn't it will fail the seek. Reading directory entries at
		 * this offset will return all files and their versions present.
		 * SHOWVFLAG does not need documenting.
		 * There are no OS dependancies on these changes. If undelete
		 * is not supported, the seek will fail and nothing different
		 * will happen.
		 */

		lseek(fd, SHOWVFLAG, 0);
		if (getdents(fd, &dirplus.u.align, DIRBUF) <= 0) {
			lseek(fd, 0L, 0);
		} else {
			lseek(fd, SHOWVFLAG, 0);
		}
	}							/* L002 end */

	return dirp;
}

/*
	closedir -- C library extension routine

*/


extern int	close();

closedir(dirp) 
register DIR	*dirp;		/* stream from opendir() */
{
	register int 	tmp_fd = dirp->dd_fd;			/* L002 begin */

	dirp->dd_fd= -1;	/* see readdir() for explanation */
	return(close( tmp_fd ));				/* L002 end */
}

static int
addg(as1, as2, as3, as4)
unsigned char	*as1, *as2, *as3, *as4;
{
	register unsigned char	*s1, *s2;

	s2 = locstak() + BYTESPERWORD;
	s1 = as1;
	if(as4) {
		while (*s2 = *s1++)
			s2++; 
	/* Restore first slash before the first metacharacter if as1 is not "/" */
		if(as4 + 1 == s1)
			*s2++ = '/';
	}
/* add matched entries, plus extra \\ to escape \\'s */
	s1 = as2;
	while (*s2 = *s1++) {
		if(*s2 == '\\')
			*++s2 = '\\';
		s2++;
	}
	if (s1 = as3)
	{
		*s2++ = '/';
		while (*s2++ = *++s1);
	}
	makearg(endstak(s2));
}

makearg(args)
	register struct argnod *args;
{
	args->argnxt = gchain;
	gchain = args;
}


