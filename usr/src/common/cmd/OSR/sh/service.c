#ident	"@(#)OSRcmds:sh/service.c	1.1"
#pragma comment(exestr, "@(#) service.c 26.1 95/08/23 ")
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
 *	L001	scol!hughd	25apr92
 *	- beware overrunning the end of fdmap[]: for now fail with
 *	  duperr (otherwise unused), but perhaps it should not be an
 *	  error at all: observe that no notice is taken if dup fails
 *	L002	scol!ianw	06aug94
 *	- Removed #include of acctdef.h, it is unnecessary and caused
 *	  redeclaration errors.
 *	L003	scol!hughd	24aug94
 *	- stop "Invalid reference" notices: nextpath() and catpath() often
 *	  called with NULL path, treat this like "" before dereferencing it
 *	- this is rather dubious in the case of catpath(), particularly
 *	  since COFF tended to have a string of three non-NUL bytes at NULL
 *	  (which would have been treated as path not found); but the obvious
 *	  danger, that a binary might be picked up from current directory
 *	  though not in the PATH, does not happen; and working out what
 *	  all the calls to catpath() ought to be doing is more difficult
 *	- fixed compiler warning with cast
 *	L004	scol!ianw	28sept94
 *	- added declarations, necessary to stop redeclaration errors when
 *	  building sh.fl.
 *	L005	scol!ianw	23aug95
 *	- Moved the #include of sys/types.h to earlier in the file, necessary
 *	  as sh.fl is built with -a addon and the 5.0.0Ke build supplement
 *	  includes old (mostly BL7) OS sys headers.
 */

/* #ident	"@(#)sh:service.c	1.5.1.1" */
/*
 * UNIX shell
 */

#ifdef VPIX
#include        "hash.h"
#endif
#include	<fcntl.h>
#include	"defs.h"
#include	<sys/types.h>					/* L005 */
#include	<errno.h>

#define ARGMK	01

extern int	gsort();
extern unsigned char	*sysmsg[];
extern short topfd;

extern gid_t	getgid(void);					/* L004 */
extern uid_t	getuid(void);					/* L004 */
extern off_t	lseek(int, off_t, int);				/* L004 */

/*
 * service routines for `execute'
 */
initio(iop, save)
	struct ionod	*iop;
	int		save;
{
	register unsigned char	*ion;
	register int	iof, fd;
	int		ioufd;
	short	lastfd;

	lastfd = topfd;
	while (iop)
	{
		iof = iop->iofile;
		ion = mactrim(iop->ioname);
		ioufd = iof & IOUFD;

		if (*ion && (flags&noexec) == 0)
		{
			if (save)
			{
				if (topfd >= FDSAVES)		/* L001 */
					failed(ion, duperr);	/* L001 */
				fdmap[topfd].org_fd = ioufd;
				fdmap[topfd++].dup_fd = savefd(ioufd);
			}

			if (iof & IODOC)
			{
				struct tempblk tb;

				subst(chkopen(ion), (fd = tmpfil(&tb)));

				poptemp();	/* pushed in tmpfil() --
						   bug fix for problem with
						   in-line scripts
						*/

				fd = chkopen(tmpout);
				unlink(tmpout);
			}
			else if (iof & IOMOV)
			{
				if (eq(minus, ion))
				{
					fd = -1;
					close(ioufd);
				}
				else if ((fd = stoi(ion)) >= USERIO)
					failed(ion, badfile);
				else
					fd = dup(fd);
			}
			else if ((iof & IOPUT) == 0)
				fd = chkopen(ion);
			else if (flags & rshflg)
				failed(ion, restricted);
			else if (iof & IOAPP && (fd = open((char *)ion, 1)) >= 0)
				lseek(fd, 0L, 2);
			else
				fd = create(ion);
			if (fd >= 0)
				rename(fd, ioufd);
		}

		iop = iop->ionxt;
	}
	return(lastfd);
}

unsigned char *
simple(s)
unsigned char	*s;
{
	unsigned char	*sname;

	sname = s;
	while (1)
	{
		if (any('/', sname))
			while (*sname++ != '/')
				;
		else
			return(sname);
	}
}

unsigned char *
getpath(s)
	unsigned char	*s;
{
	register unsigned char	*path, *newpath;
	register int pathlen;	
	
	if (any('/', s))
	{
		if (flags & rshflg)
			failed(s, restricted);
		else
			return(nullstr);
	}
	else if ((path = pathnod.namval) == 0)
		return(defpath);
	else {
		pathlen = length(path)-1;
		/* Add extra ':' if PATH variable ends in ':' */
		if(pathlen > 2 && path[pathlen - 1] == ':' && path[pathlen - 2] != ':') {
			newpath = locstak();
			(void) memcpy(newpath, path, pathlen);
			newpath[pathlen] = ':';
			endstak(newpath + pathlen + 1);
			return(newpath);
		} else
			return(cpystak(path));
	}
}

pathopen(path, name)
register unsigned char *path, *name;
{
	register int	f;

	do
	{
#ifdef VPIX
		path = catpath(path, name, (char *)0);
#else
		path = catpath(path, name);
#endif
	} while ((f = open((char *)curstak(), 0)) < 0 && path);
	return(f);
}

unsigned char *
#ifdef VPIX
catpath(path, name, ext)
unsigned char *ext;
#else
catpath(path, name)
#endif
register unsigned char	*path;
unsigned char	*name;
{
	/*
	 * leaves result on top of stack
	 */
	register unsigned char	*scanp = path;
	register unsigned char	*argp = locstak();

	if (scanp != (unsigned char *)0) {			/* L003 */
		while (*scanp && *scanp != COLON)
			*argp++ = *scanp++;
		if (scanp != path) 
			*argp++ = '/';
		if (*scanp == COLON)
			scanp++;
		path = (*scanp ? scanp : 0);
	}							/* L003 */

	scanp = name;
	while ((*argp++ = *scanp++))
		;

#ifdef VPIX
	/* append DOS filename extension if asked */
	if ((scanp = ext))
	{       argp--;
		while (*argp++ = *scanp++)
			;
	}
#endif

	return(path);
}

unsigned char *
nextpath(path)
	register unsigned char	*path;
{
	register unsigned char	*scanp = path;

	if (scanp == (unsigned char *)0)			/* L003 */
		return (unsigned char *)0;			/* L003 */

	while (*scanp && *scanp != COLON)
		scanp++;

	if (*scanp == COLON)
		scanp++;

	return(*scanp ? scanp : 0);
}

#ifdef VPIX
/* see if path is in DOSPATH */
dospath(path)
	char *path;
{
	register char   *scanp;
	register unsigned char *scandp;

	if (path == 0) return 0;
	if ((scandp = dpathnod.namval) == 0) return 0;
	for (;;)
	{       scanp = path;
		for (;;)
		{       if ((*scanp == 0 || *scanp == COLON) &&
					(*scandp == 0 || *scandp == COLON))
				return 1;
			if (*scanp != *scandp) break;
			scanp++;
			scandp++;
		}
		while (*scandp && *scandp != COLON) scandp++;
		if (*scandp == 0) return 0;
		scandp++;        /* scanp points after COLON */
	}
}

unsigned char **
dosargs(args, path, dosext)
	unsigned char    **args;
	char    *path;
	char    *dosext;
{       int     i;
	unsigned char    **answer;
	unsigned char    *p;

	/* determine how big the args array is (including null) */
	for (i = 0; args[i++];)
		;
	i += 2;         /* we will add two more */
	answer = (unsigned char **)alloc (i * sizeof path);
	/* check for out of space.  Unlikely to happen, so handle it
	 * just well enough to prevent disaster */
	if (answer == 0) return args;
	answer[0] = (unsigned char *)vpix;
	answer[1] = (unsigned char *)vpixflag;
	answer[2] = alloc (length (path) +
			length (args[0]) + length (dosext) + 2);
	/* again merely stave off disaster if out of space */
	if (answer[2] == 0) answer[2] = args[0];
	else
	{       p = answer[2];
		while (*path && *path != COLON) *p++ = *path++;
		if (p != answer[2]) *p++ = '/';
		movstr (dosext, movstr (args[0], p));
	}
	i--;
	while (--i > 0) answer[i + 2] = args[i];
	return answer;
}
#endif

static unsigned char	*xecmsg;
static unsigned char	**xecenv;

int	execa(at, pos)
	unsigned char	*at[];
	short pos;
{
	register unsigned char	*path;
	register unsigned char	**t = at;
	int		cnt;
#ifdef VPIX
	short           dosval;
	char            *dosext;
	unsigned char   **dost;
	unsigned char            *dospath;
#endif

	if ((flags & noexec) == 0)
	{
		xecmsg = notfound;
		path = getpath(*t);
#ifdef VPIX
		dosval = (pos < 0) ? 0 : (pos & DOSFIELD);
		pos = hashdata(pos);
		dospath = vpixdirname;
#endif

		xecenv = setenv();

		if (pos > 0)
		{
			cnt = 1;
			while (cnt != pos)
			{
				++cnt;
				path = nextpath(path);
			}
#ifdef VPIX
			switch (dosval)
			{ case DOSDOTCOM:
				dosext = dotcom;
				break;
			case DOSDOTEXE:
				dosext = dotexe;
				break;
			case DOSDOTBAT:
				dosext = dotbat;
				break;
			default:
				dosext = 0;
				break;
			}
			if (dosext == 0)
			{       dost = t;
				dospath = path;
			}
			else dost = (unsigned char **)dosargs (t, path, dosext);
			execs(dospath, dost);
#else
			execs(path, t);
#endif
			path = getpath(*t);
		}
		while (path = execs(path,t))
			;
		failed(*t, xecmsg);
	}
}

static unsigned char *
execs(ap, t)
unsigned char	*ap;
register unsigned char	*t[];
{
	register unsigned char *p, *prefix;

	prefix = catpath(ap, t[0], (char *)0);
	trim(p = curstak());
	sigchk();
	
	execve(p, &t[0] ,xecenv);
	switch (errno)
	{
	case ENOEXEC:		/* could be a shell script */
		funcnt = 0;
		flags = 0;
		*flagadr = 0;
		comdiv = 0;
		ioset = 0;
		clearup();	/* remove open files and for loop junk */
		if (input)
			close(input);
		input = chkopen(p);
	
#ifdef ACCT
		preacct(p);	/* reset accounting */
#endif

		/*
		 * set up new args
		 */
		
		setargs(t);
		longjmp(subshell, 1);

	case ENOMEM:
		failed(p, toobig);

	case E2BIG:
		failed(p, arglist);

	case ETXTBSY:
		failed(p, txtbsy);

	case ELIBACC:
		failed(p, libacc);

	case ELIBBAD:
		failed(p, libbad);

	case ELIBSCN:
		failed(p, libscn);

	case ELIBMAX:
		failed(p, libmax);

	default:
		xecmsg = badexec;
	case ENOENT:
		return(prefix);
	}
}


/*
 * for processes to be waited for
 */
#define MAXP 20
static int	pwlist[MAXP];
static int	pwc;

postclr()
{
	register int	*pw = pwlist;

	while (pw <= &pwlist[pwc])
		*pw++ = 0;
	pwc = 0;
}

post(pcsid)
int	pcsid;
{
	register int	*pw = pwlist;

	if (pcsid)
	{
		while (*pw)
			pw++;
		if (pwc >= MAXP - 1)
			pw--;
		else
			pwc++;
		*pw = pcsid;
	}
}

await(i, bckg)
int	i, bckg;
{
	int	rc = 0, wx = 0;
	int	w;
	int	ipwc = pwc;

	post(i);
	while (pwc)
	{
		register int	p;
		register int	sig;
		int		w_hi;
		int	found = 0;

		{
			register int	*pw = pwlist;

			p = wait(&w);
			if (wasintr)
			{
				wasintr = 0;
				if (bckg)
				{
					break;
				}
			}
			while (pw <= &pwlist[ipwc])
			{
				if (*pw == p)
				{
					*pw = 0;
					pwc--;
					found++;
				}
				else
					pw++;
			}
		}
		if (p == -1)
		{
			if (bckg)
			{
				register int *pw = pwlist;

				while (pw <= &pwlist[ipwc] && i != *pw)
					pw++;
				if (i == *pw)
				{
					*pw = 0;
					pwc--;
				}
			} else if(errno == ECHILD) 
				break;
			continue;
		}
		w_hi = (w >> 8) & LOBYTE;
		if (sig = w & 0177)
		{
			if (sig == 0177)	/* ptrace! return */
			{
				prs("ptrace: ");
				sig = w_hi;
			}
			if (sig > SIGPWR || sysmsg[sig])
			{
				if (i != p || (flags & prompt) == 0)
				{
					prp();
					prn(p);
					blank();
				}
				if(sig > SIGPWR) {
					prs("Signal ");
					prn(sig);
				}
				else
					prs(sysmsg[sig]);
				if (w & 0200)
					prs(coredump);
				newline();
			}
			else if (flags & prompt)
				newline();
		}
		if (rc == 0 && found != 0)
			rc = (sig ? sig | SIGFLG : w_hi);
		wx |= w;
		if (p == i)
		{
			break;
		}
	}
	if (wx && flags & errflg)
		exitsh(rc);
	flags |= eflag;
	exitval = rc;
	exitset();
}

BOOL		nosubst;

trim(at)
unsigned char	*at;
{
	register unsigned char	*last;
	register unsigned char 	*current;
	register unsigned char	c;
	register unsigned char	q = 0;

	nosubst = 0;
	if (current = at)
	{
		last = at;
		while (c = *current++)
		{
			if(c == '\\')  { /* remove \ and quoted nulls */
				nosubst = 1;
				if(c = *current++)
					*last++ = c;
			} else
				*last++ = c;
		}

		*last = 0;
	}
}

unsigned char *
mactrim(s)
unsigned char	*s;
{
	register unsigned char	*t = macro(s);

	trim(t);
	return(t);
}

unsigned char **
scan(argn)
int	argn;
{
	register struct argnod *argp = (struct argnod *)(Rcheat(gchain) & ~ARGMK);
	register unsigned char **comargn, **comargm;

	comargn = (unsigned char **)getstak(BYTESPERWORD * argn + BYTESPERWORD);
	comargm = comargn += argn;
	*comargn = ENDARGS;
	while (argp)
	{
		*--comargn = argp->argval;

		trim(*comargn);
		argp = argp->argnxt;

		if (argp == 0 || Rcheat(argp) & ARGMK)
		{
			gsort(comargn, comargm);
			comargm = comargn;
		}
		argp = (struct argnod *)(Rcheat(argp) & ~ARGMK);
	}
	return(comargn);
}

static int
gsort(from, to)
unsigned char	*from[], *to[];
{
	int	k, m, n;
	register int	i, j;

	if ((n = to - from) <= 1)
		return;
	for (j = 1; j <= n; j *= 2)
		;
	for (m = 2 * j - 1; m /= 2; )
	{
		k = n - m;
		for (j = 0; j < k; j++)
		{
			for (i = j; i >= 0; i -= m)
			{
				register unsigned char **fromi;

				fromi = &from[i];
				if (cf(fromi[m], fromi[0]) > 0)
				{
					break;
				}
				else
				{
					unsigned char *s;

					s = fromi[m];
					fromi[m] = fromi[0];
					fromi[0] = s;
				}
			}
		}
	}
}

/*
 * Argument list generation
 */
getarg(ac)
struct comnod	*ac;
{
	register struct argnod	*argp;
	register int		count = 0;
	register struct comnod	*c;

	if (c = ac)
	{
		argp = c->comarg;
		while (argp)
		{
			count += split(macro(argp->argval),1);
			argp = argp->argnxt;
		}
	}
	return(count);
}

static int
split(s)		/* blank interpretation routine */
register unsigned char	*s;
{
	register unsigned char	*argp;
	register int	c;
	register int	d;
	int		count = 0;
	for (;;)
	{
		sigchk();
		argp = locstak() + BYTESPERWORD;
		while (c = *s++) { 
			if(c == '\\') { /* skip over quoted characters */
				*argp++ = c;
				*argp++ = *s++;
			}
			else if (any(c, ifsnod.namval))
				break;
			else
				*argp++ = c;
		}
		if (argp == staktop + BYTESPERWORD)
		{
			if (c)
			{
				continue;
			}
			else
			{
				return(count);
			}
		}
		else if (c == 0)
			s--;
		/*
		 * file name generation
		 */

		argp = endstak(argp);

		if ((flags & nofngflg) == 0 && 
			(c = expand(((struct argnod *)argp)->argval, 0)))
			count += c;
		else
		{
			makearg(argp);
			count++;
		}
		gchain = (struct argnod *)((int)gchain | ARGMK);
	}
}

#ifdef ACCT
#include	<sys/acct.h>
#include 	<sys/times.h>

struct acct sabuf;
struct tms buffer;
extern long times();
static long before;
static int shaccton;	/* 0 implies do not write record on exit
			   1 implies write acct record on exit
			*/


/*
 *	suspend accounting until turned on by preacct()
 */

suspacct()
{
	shaccton = 0;
}

preacct(cmdadr)
	unsigned char *cmdadr;
{
	unsigned char *simple();

	if (acctnod.namval && *acctnod.namval)
	{
		sabuf.ac_btime = time((long *)0);
		before = times(&buffer);
		sabuf.ac_uid = getuid();
		sabuf.ac_gid = getgid();
		movstrn(simple(cmdadr), sabuf.ac_comm, sizeof(sabuf.ac_comm));
		shaccton = 1;
	}
}

doacct()
{
	int fd;
	long int after;

	if (shaccton)
	{
		after = times(&buffer);
		sabuf.ac_utime = compress(buffer.tms_utime + buffer.tms_cutime);
		sabuf.ac_stime = compress(buffer.tms_stime + buffer.tms_cstime);
		sabuf.ac_etime = compress(after - before);

		if ((fd = open((char *)acctnod.namval,		/* L003 */
			O_WRONLY | O_APPEND | O_CREAT, 0666)) != -1)
		{
			write(fd, &sabuf, sizeof(sabuf));
			close(fd);
		}
	}
}

/*
 *	Produce a pseudo-floating point representation
 *	with 3 bits base-8 exponent, 13 bits fraction
 */

compress(t)
	register time_t t;
{
	register exp = 0;
	register rund = 0;

	while (t >= 8192)
	{
		exp++;
		rund = t & 04;
		t >>= 3;
	}

	if (rund)
	{
		t++;
		if (t >= 8192)
		{
			t >>= 3;
			exp++;
		}
	}

	return((exp << 13) + t);
}
#endif
