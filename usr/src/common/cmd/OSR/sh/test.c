#ident	"@(#)OSRcmds:sh/test.c	1.1"
#pragma comment(exestr, "@(#) test.c 25.1 93/07/26 ")
/*
 *	      UNIX is a registered trademark of AT&T
 *		Portions Copyright 1976-1989 AT&T
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1993 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */
/*	Copyright (c) 1984, 1986, 1987, 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)sh:test.c	1.4" */
/*
 *      test expression
 *      [ expression ]
 */

/* MODIFICATION HISTORY
 *
 *	L000	3 January	scol!markhe
 *	- added addition primitive ( activated with -L or -h) to test if
 *	  a file is a symbolic link.  With all other primitives symbolic
 *	  links are followed by default.
 *	L001	25feb91		scol!hughd
 *	- replace lstat by statlstat so executable runs on versions
 *	  of the OS both with and without the lstat system call
 *	L002	28May93		scol!olafvb
 *	- added options -H and -M to check respectively if a file is
 *        a semaphore or shared memory.  Used -H because -S already used
 *        in ksh 
 *      - added additional argument to filtyp(), updated all calls accordingly
 * 
 */

#include	"defs.h"
#include <sys/types.h>
#include <sys/stat.h>
#include	"../include/osr.h"

static short SymLink;	/* Flag used to indicate -L/-h option */      /* L000 */
int	ap, ac;
unsigned char	**av;

test(argn, com)
unsigned char	*com[];
int	argn;
{
	ac = argn;
	av = com;
	ap = 1;
	if (eq(com[0],"["))
	{
		if (!eq(com[--ac], "]"))
			failed("test", "] missing");
	}
	com[ac] = 0;
	if (ac <= 1)
		return(1);
	return(expa() ? 0 : 1);
}

unsigned char *
nxtarg(mt)
{
	if (ap >= ac)
	{
		if (mt)
		{
			ap++;
			return(0);
		}
		failed("test", "argument expected");
	}
	return(av[ap++]);
}

expa()
{
	int	p1;
	unsigned char	*p2;

	p1 = e1();
	p2 = nxtarg(1);
	if (p2 != 0)
	{
		if (eq(p2, "-o"))
			return(p1 | expa());

		/* if (!eq(p2, ")"))
			failed("test", synmsg); */
	}
	ap--;
	return(p1);
}

e1()
{
	int	p1;
	unsigned char	*p2;

	p1 = e2();
	p2 = nxtarg(1);

	if ((p2 != 0) && eq(p2, "-a"))
		return(p1 & e1());
	ap--;
	return(p1);
}

e2()
{
	if (eq(nxtarg(0), "!"))
		return(!e3());
	ap--;
	return(e3());
}

e3()
{
	int	p1;
	register unsigned char	*a;
	unsigned char	*p2;
	long	atol();
	long	int1, int2;

	a = nxtarg(0);
	if (eq(a, "("))
	{
		p1 = expa();
		if (!eq(nxtarg(0), ")"))
			failed("test",") expected");
		return(p1);
	}
	p2 = nxtarg(1);
	ap--;
	if ((p2 == 0) || (!eq(p2, "=") && !eq(p2, "!=")))
	{
		if (eq(a, "-r"))
			return(chk_access(nxtarg(0), S_IREAD, 0) == 0);
		if (eq(a, "-w"))
			return(chk_access(nxtarg(0), S_IWRITE, 0) == 0);
		if (eq(a, "-x"))
			return(chk_access(nxtarg(0), S_IEXEC, 0) == 0);
		if (eq(a, "-d"))
			return(filtyp(nxtarg(0), S_IFDIR, 0));      /* L002 */
		if (eq(a, "-c"))
			return(filtyp(nxtarg(0), S_IFCHR, 0));      /* L002 */
		if (eq(a, "-b"))
			return(filtyp(nxtarg(0), S_IFBLK, 0));      /* L002 */
		if (eq(a, "-f"))
			return(filtyp(nxtarg(0), S_IFREG, 0));      /* L002 */
		if (eq(a, "-L") || eq(a, "-h"))			    /* L000 { */
		{
			int Result;

			SymLink = 1;
			Result = filtyp(nxtarg(0), S_IFLNK, 0);     /* L002 */
			SymLink = 0;
			return( Result);
		}/*if*/						    /* L000 } */
		if (eq(a, "-u"))
			return(ftype(nxtarg(0), S_ISUID));
		if (eq(a, "-g"))
			return(ftype(nxtarg(0), S_ISGID));
		if (eq(a, "-k"))
			return(ftype(nxtarg(0), S_ISVTX));
		if (eq(a, "-p"))
			return(filtyp(nxtarg(0), S_IFIFO, 0));      /* L002 */
#if defined(S_IFNAM) && defined(S_INSEM)
		if (eq(a, "-H"))                                   /* L002 */
			return(filtyp(nxtarg(0),S_IFNAM,S_INSEM)); /* L002 */
#endif
#if defined(S_IFNAM) && defined(S_INSHD)
		if (eq(a, "-M"))                                   /* L002 */
			return(filtyp(nxtarg(0),S_IFNAM,S_INSHD)); /* L002 */
#endif
   		if (eq(a, "-s"))
			return(fsizep(nxtarg(0)));
		if (eq(a, "-t"))
		{
			if (ap >= ac)		/* no args */
				return(isatty(1));
			else if (eq((a = nxtarg(0)), "-a") || eq(a, "-o"))
			{
				ap--;
				return(isatty(1));
			}
			else
				return(isatty(atoi(a)));
		}
		if (eq(a, "-n"))
			return(!eq(nxtarg(0), ""));
		if (eq(a, "-z"))
			return(eq(nxtarg(0), ""));
	}

	p2 = nxtarg(1);
	if (p2 == 0)
		return(!eq(a, ""));
	if (eq(p2, "-a") || eq(p2, "-o"))
	{
		ap--;
		return(!eq(a, ""));
	}
	if (eq(p2, "="))
		return(eq(nxtarg(0), a));
	if (eq(p2, "!="))
		return(!eq(nxtarg(0), a));
	int1 = atol(a);
	int2 = atol(nxtarg(0));
	if (eq(p2, "-eq"))
		return(int1 == int2);
	if (eq(p2, "-ne"))
		return(int1 != int2);
	if (eq(p2, "-gt"))
		return(int1 > int2);
	if (eq(p2, "-lt"))
		return(int1 < int2);
	if (eq(p2, "-ge"))
		return(int1 >= int2);
	if (eq(p2, "-le"))
		return(int1 <= int2);

	bfailed(btest, badop, p2);
/* NOTREACHED */
}


ftype(f, field)
unsigned char	*f;
int	field;
{
	struct stat statb;

	if (stat((char *)f, &statb) < 0)
		return(0);
	if ((statb.st_mode & field) == field)
		return(1);
	return(0);
}

filtyp(f,field,named)                                            /* L002 */
unsigned char	*f;
int field, named;
{
	struct stat statb;

	if ((SymLink?statlstat((char *)f, &statb):stat((char *)f, &statb)) < 0)	      /* L000 */
		return(0);
	if ((statb.st_mode & S_IFMT) == field) {
#ifdef S_IFNAM
		if (field == S_IFNAM)   /* it is a special named file - L002 */
			if (named == statb.st_rdev)                  /* L002 */
				return(1);                           /* L002 */
			else                                         /* L002 */
				return(0);                           /* L002 */
#endif
		return(1);
        }
	else
		return(0);
}


fsizep(f)
unsigned char	*f;
{
	struct stat statb;

	if (stat((char *)f, &statb) < 0)
		return(0);
	return(statb.st_size > 0);
}

/*
 * fake diagnostics to continue to look like original
 * test(1) diagnostics
 */
bfailed(s1, s2, s3) 
unsigned char	*s1;
unsigned char	*s2;
unsigned char	*s3;
{
	prp();
	prs(s1);
	if (s2)
	{
		prs(colon);
		prs(s2);
		prs(s3);
	}
	newline();
	exitsh(ERROR);
}
