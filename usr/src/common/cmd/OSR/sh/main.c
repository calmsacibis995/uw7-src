#ident	"@(#)OSRcmds:sh/main.c	1.1"
#pragma comment(exestr, "@(#) main.c 25.2 93/08/06 ")
/*
 *	sh:main - main function of bourne shell
 */
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

/*
 *	MODIFICATION HISTORY
 *
 *	L000	16 Jan 91	scol!ianw
 *		- The code which sets the EU/GIDs to the RU/GIDs (under
 *		  certain circumstances) is made conditional on RESETIDS.
 *		  RESETIDS is not normally defined, so the code is
 *		  effectively removed, it is only left to enable a shell
 *		  with the old behaviour (pre 3.2v3), to be built if required.
 *	S001	sco!asharpe	Mon Aug 24 16:44:12 1992
 *		- The shell was using file descriptor 19 (INIO in defs.h)
 *		  for its own purposes, whether it was open or not. So, we
 *		  now use ulimit to determine the maximum number of open
 *		  files, and try to use the last one. If we can't use it
 *		  because it's already open, print an error message and
 *		  give up.
 *	L002	21 Jun 93	scol!peters
 *		- The problem with S001 is that, in 3.2v4.2 onwards, NOFILES
 *		  can be increased to 11000: if we open file NOFILES-1 then
 *		  the U-block will be grown accordingly, wasting a great deal
 *		  of memory per shell.  Changed to simply allocate the next
 *		  file descriptor (equal to or higher than INIO).  (I've also
 *		  modified the '#ifdef RES' stuff for completeness - is it
 *		  ever used?)
 */

/*
 * UNIX shell
 */

#include	"defs.h"
#include	"sym.h"
#include	"timeout.h"
#include	<sys/types.h>
#include	<sys/stat.h>
#include        "dup.h"

#ifdef RES
#include	<sgtty.h>
#endif

static BOOL	beenhere = FALSE;
unsigned char		tmpout[20] = "/tmp/sh-";
struct fileblk	stdfile;
struct fileblk *standin = &stdfile;
int mailchk = 0;

static unsigned char	*mailp;
static long	*mod_time = 0;

#if vax
char **execargs = (char**)(0x7ffffffc);
#endif

#if pdp11
char **execargs = (char**)(-2);
#endif


extern int	exfile();
extern unsigned char 	*simple();



main(c, v, e)
int	c;
unsigned char	*v[];
unsigned char	*e[];
{
	register int	rflag = ttyflg;
	int		rsflag = 1;	/* local restricted flag */
	struct namnod	*n;
#ifdef RESETIDS						/* L000 */
	register int euid;
	register int egid;
	register int ruid;
	register int rgid;
#endif							/* L000 */

	stdsigs();

	/*
	 * initialise storage allocation
	 */

	stakbot = 0;
	addblok((unsigned)0);

	/*
	 * set names from userenv
	 */

	setup_env();

	/*
	 * 'rsflag' is zero if SHELL variable is
	 *  set in environment and 
	 *  the simple file part of the value.
	 *  is rsh
	 */
	if (n = findnam("SHELL"))
	{
		if (!strcmp("rsh", (char *)simple(n->namval)))
			rsflag = 0;
	}

	/*
	 * a shell is also restricted if the simple name of argv(0) is
	 * rsh or -rsh in its simple name
	 */

#ifndef RES

	if (c > 0 && (!strcmp("rsh", (char *)simple(*v)) || !strcmp("-rsh", (char *)simple(*v))))
		rflag = 0;

#endif

	hcreate();
	set_dotpath();

#ifdef VPIX
	set_vpixdir ();
#endif

	/*
	 * look for options
	 * dolc is $#
	 */
	dolc = options(c, v);
#ifdef RESETIDS						/* L000 */
	/*
	 Determine all of the user's id #'s for this process and
	 then decide if this shell is being entered as a result
	 of a fork/exec.
	 If the effective uid/gid do NOT match and the euid/egid
	 is < 100 and the egid is NOT 1, reset the uid and gid to
	 the user originally calling this process.
	 */
	euid = geteuid();
	ruid = getuid();
	egid = getegid();
	rgid = getgid();

	if ((euid != ruid) && (euid < 100))
		setuid(ruid);	/* reset the uid to the orig user */

	if ((egid != rgid) && ((egid < 100) && (egid != 1)))
		setgid(rgid);	/* reset the gid to the orig user */
#endif							/* L000 */

	if (dolc < 2)
	{
		flags |= stdflg;
		{
			register unsigned char *flagc = flagadr;

			while (*flagc)
				flagc++;
			*flagc++ = STDFLG;
			*flagc = 0;
		}
	}
	if ((flags & stdflg) == 0)
		dolc--;
	dolv = (unsigned char **)v + c - dolc;
	dolc--;

	/*
	 * return here for shell file execution
	 * but not for parenthesis subshells
	 */
	setjmp(subshell);

	/*
	 * number of positional parameters
	 */
	replace(&cmdadr, dolv[0]);	/* cmdadr is $0 */

	/*
	 * set pidname '$$'
	 */
	assnum(&pidadr, getpid());

	/*
	 * set up temp file names
	 */
	settmp();

	/*
	 * default internal field separators - $IFS
	 */
	dfault(&ifsnod, sptbnl);

	dfault(&mchknod, MAILCHECK);
	mailchk = stoi(mchknod.namval);

/*	initialize OPTIND for getopt */
	
	n = lookup("OPTIND");
	assign(n, "1");

	if ((beenhere++) == FALSE)	/* ? profile */
	{
		if (*(simple(cmdadr)) == '-')
		{			/* system profile */

#ifndef RES

			if ((input = pathopen(nullstr, sysprofile)) >= 0)
				exfile(rflag);		/* file exists */

#endif

			if ((input = pathopen(nullstr, profile)) >= 0)
			{
				exfile(rflag);
				flags &= ~ttyflg;
			}
		}
		if (rsflag == 0 || rflag == 0)
			flags |= rshflg;
		/*
		 * open input file if specified
		 */
		if (comdiv)
		{
			estabf(comdiv);
			input = -1;
		}
		else
		{
			input = ((flags & stdflg) ? 0 : chkopen(cmdadr));

#ifdef ACCT
			if (input != 0)
				preacct(cmdadr);
#endif
			comdiv--;
		}
	}
#ifdef pdp11
	else
		*execargs = (char *)dolv;	/* for `ps' cmd */
#endif
		
	exfile(0);
	done();
}

static int
exfile(prof)
BOOL	prof;
{
	long	mailtime = 0;	/* Must not be a register variable */
	long 	curtime = 0;
	register int	userid;
	register int	ninput;	/* Temporary file desc for L002	   */

	/*
	 * move input
	 */
	if (input > 0)
	{
		/* Start L002 ...
		 *
		 * Dup file >= INIO from input, save file allocated.
		 */
		if ((ninput = Ldup(input, INIO)) < 0) {
			failure(openmax, (char *)0);		/* S001 */
			close(input);				/* S001 */
			return;					/* S001 */
		}
		input = ninput;
		/* ... end L002 */
	}

	userid = geteuid();

	/*
	 * decide whether interactive
	 */
	if ((flags & intflg) ||
	    ((flags&oneflg) == 0 &&
	    isatty(output) &&
	    isatty(input)) )
	    
	{
		dfault(&ps1nod, (userid ? stdprompt : supprompt));
		dfault(&ps2nod, readmsg);
		flags |= ttyflg | prompt;
		ignsig(SIGTERM);
		if (mailpnod.namflg != N_DEFAULT)
			setmail(mailpnod.namval);
		else
			setmail(mailnod.namval);
	}
	else
	{
		flags |= prof;
		flags &= ~prompt;
	}

	if (setjmp(errshell) && prof)
	{
		close(input);
		return;
	}
	/*
	 * error return here
	 */

	loopcnt = peekc = peekn = 0;
	fndef = 0;
	nohash = 0;
	iopend = 0;

	if (input >= 0)
		initf(input);
	/*
	 * command loop
	 */
	for (;;)
	{
		tdystak(0);
		stakchk();	/* may reduce sbrk */
		exitset();

		if ((flags & prompt) && standin->fstak == 0 && !eof)
		{

			if (mailp)
			{
				time(&curtime);

				if ((curtime - mailtime) >= mailchk)
				{
					chkmail();
				        mailtime = curtime;
				}
			}

			prs(ps1nod.namval);

#ifdef TIME_OUT
			alarm(TIMEOUT);
#endif

			flags |= waiting;
		}

		trapnote = 0;
		peekc = readc();
		if (eof)
			return;

#ifdef TIME_OUT
		alarm(0);
#endif

		flags &= ~waiting;

		execute(cmd(NL, MTFLG), 0, eflag);
		eof |= (flags & oneflg);
	}
}

chkpr()
{
	if ((flags & prompt) && standin->fstak == 0)
		prs(ps2nod.namval);
}

settmp()
{
	itos(getpid());
	serial = 0;
	tmpnamo = movstr(numbuf, &tmpout[TMPNAM]);
}

Ldup(fa, fb)
register int	fa, fb;
{
#ifdef RES

	if (fa != fb) {						/* L002 */
		fb = dup(fa | DUPFLG, fb));			/* L002 */
		close(fa);
	}							/* L002 */
	ioctl(fb, FIOCLEX, 0);
	return(fb);						/* L002 */

#else
	/* Start L002 ...
	 *
	 * If we haven't already got the requested file attempt to get it or
	 * the first higher unallocated file; don't close 'fb' first, it may
	 * already be allocated to the invoker of the shell.
	 */
	if (fa != fb) {
		if ((fb = fcntl(fa,0,fb)) < 0)	/* normal dup */
			return(-1);
		close(fa);
	}
	fcntl(fb, 2, 1);			/* autoclose for fb */

	return(fb);
	/* ... end L002 */
#endif
}


chkmail()
{
	register unsigned char 	*s = mailp;
	register unsigned char	*save;

	long	*ptr = mod_time;
	unsigned char	*start;
	BOOL	flg; 
	struct stat	statb;

	while (*s)
	{
		start = s;
		save = 0;
		flg = 0;

		while (*s)
		{
			if (*s != COLON)	
			{
				if (*s == '%' && save == 0)
					save = s;
			
				s++;
			}
			else
			{
				flg = 1;
				*s = 0;
			}
		}

		if (save)
			*save = 0;

		if (*start && stat((char *)start, &statb) >= 0)
		{
			if(statb.st_size && *ptr
				&& statb.st_mtime != *ptr)
			{
				if (save)
				{
					prs(save+1);
					newline();
				}
				else
					prs(mailmsg);
			}
			*ptr = statb.st_mtime;
		}
		else if (*ptr == 0)
			*ptr = 1;

		if (save)
			*save = '%';

		if (flg)
			*s++ = COLON;

		ptr++;
	}
}


setmail(mailpath)
	unsigned char *mailpath;
{
	register unsigned char	*s = mailpath;
	register int 	cnt = 1;

	long	*ptr;

	free(mod_time);
	if (mailp = mailpath)
	{
		while (*s)
		{
			if (*s == COLON)
				cnt += 1;

			s++;
		}

		ptr = mod_time = (long *)alloc(sizeof(long) * cnt);

		while (cnt)
		{
			*ptr = 0;
			ptr++;
			cnt--;
		}
	}
}
