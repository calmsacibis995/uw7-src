#ident	"@(#)OSRcmds:sh/name.c	1.1"
/*
 *	@(#) name.c 25.1 95/03/22 
 *
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

/* #ident	"@(#)sh:name.c	1.5.1.1" */
#pragma comment(exestr, "@(#) name.c 25.1 95/03/22 ")
/*
 * UNIX shell
 */

/*
 *	MODIFICATION HISTORY
 *
 *	L000	21 Sep 89	scol!howardf
 *	- Allowed for dynamically changing the locale of the
 *	  running shell.
 *	L001	17th Mar 95	scol!ashleyb
 *	- Make sure that the variable we pass into setlocale()
 *	  is initialised.
 */

#include	"defs.h"
#include	<locale.h>				/* L000 */

extern BOOL	chkid();
extern unsigned char	*simple();
extern int	mailchk;

struct namnod ps2nod =
{
	(struct namnod *)NIL,
	&acctnod,
	ps2name
};
struct namnod cdpnod = 
{
	(struct namnod *)NIL,
#ifdef VPIX
	&dpathnod,
#else
	(struct namnod *)NIL,
#endif
	cdpname
};
#ifdef VPIX
struct namnod dpathnod =
{       (struct namnod *)NIL,
	(struct namnod *)NIL,
	(unsigned char *)dpathname
};
#endif
struct namnod pathnod =
{
	&mailpnod,
	(struct namnod *)NIL,
	pathname
};
struct namnod ifsnod =
{
	&homenod,
	&mailnod,
	ifsname
};
struct namnod ps1nod =
{
	&pathnod,
	&ps2nod,
	ps1name
};
struct namnod homenod =
{
	&cdpnod,
	(struct namnod *)NIL,
	homename
};
struct namnod mailnod =
{
	(struct namnod *)NIL,
	(struct namnod *)NIL,
	mailname
};
struct namnod mchknod =
{
	&ifsnod,
	&ps1nod,
	mchkname
};
struct namnod acctnod =
{
	(struct namnod *)NIL,
	(struct namnod *)NIL,
	acctname
};
struct namnod mailpnod =
{
	(struct namnod *)NIL,
	(struct namnod *)NIL,
	mailpname
};


struct namnod *namep = &mchknod;

/* ========	variable and string handling	======== */

syslook(w, syswds, n)
	register unsigned char *w;
	register struct sysnod syswds[];
	int n;
{
	int	low;
	int	high;
	int	mid;
	register int cond;

	if (w == 0 || *w == 0)
		return(0);

	low = 0;
	high = n - 1;

	while (low <= high)
	{
		mid = (low + high) / 2;

		if ((cond = cf(w, syswds[mid].sysnam)) < 0)
			high = mid - 1;
		else if (cond > 0)
			low = mid + 1;
		else
			return(syswds[mid].sysval);
	}
	return(0);
}

setlist(arg, xp)
register struct argnod *arg;
int	xp;
{
	if (flags & exportflg)
		xp |= N_EXPORT;

	while (arg)
	{
		register unsigned char *s = mactrim(arg->argval);
		setname(s, xp);
		arg = arg->argnxt;
		if (flags & execpr)
		{
			prs(s);
			if (arg)
				blank();
			else
				newline();
		}
	}
}


setname(argi, xp)	/* does parameter assignments */
unsigned char	*argi;
int	xp;
{
	register unsigned char *argscan = argi;
	register struct namnod *n;
	int category = -1;					/* L001 */

	if (letter(*argscan))
	{
		while (alphanum(*argscan))
			argscan++;

		if (*argscan == '=')
		{
			*argscan = 0;	/* make name a cohesive string */

			n = lookup(argi);
			*argscan++ = '=';
			attrib(n, xp);
			if (xp & N_ENVNAM)
				n->namenv = n->namval = argscan;
			else
				assign(n, argscan);
			/* L000 start */
			if (strcmp((char *)n->namid, "LANG") == 0) {
				putenv(make(argi));
				setlocale(LC_ALL, "");
			} else if (strncmp((char *)n->namid, "LC_", 3) == 0) {
								/* L001 Begin */
				if (strcmp((char *)n->namid, "LC_ALL") == 0)
					category = LC_ALL;
				else if (strcmp((char *)n->namid, "LC_CTYPE") == 0)
					category = LC_CTYPE;
				else if (strcmp((char *)n->namid, "LC_NUMERIC") == 0)
					category = LC_NUMERIC;
				else if (strcmp((char *)n->namid, "LC_TIME") == 0)
					category = LC_TIME;
				else if (strcmp((char *)n->namid, "LC_COLLATE") == 0)
					category = LC_COLLATE;
				else if (strcmp((char *)n->namid, "LC_MESSAGES") == 0)
					category = LC_MESSAGES;
				else if (strcmp((char *)n->namid, "LC_MONETARY") == 0)
					category = LC_MONETARY;
				putenv(make(argi));

				/* If we have set category, call setlocale() */
				if (category != -1)
					setlocale(category, "");
								/* L001 End */
			}
			/* L000 end */
			return;
		}
	}
	failed(argi, notid);
}

replace(a, v)
register unsigned char	**a;
unsigned char	*v;
{
	free(*a);
	*a = make(v);
}

dfault(n, v)
struct namnod *n;
unsigned char	*v;
{
	if (n->namval == 0)
		assign(n, v);
}

assign(n, v)
struct namnod *n;
unsigned char	*v;
{
	if (n->namflg & N_RDONLY)
		failed(n->namid, wtfailed);

#ifndef RES
	
	else if (flags & rshflg)
	{
		if (n == &pathnod || eq(n->namid,"SHELL"))
			failed(n->namid, restricted);
	}
#endif

	else if (n->namflg & N_FUNCTN)
	{
		func_unhash(n->namid);
		freefunc(n);

		n->namenv = 0;
		n->namflg = N_DEFAULT;
	}

	if (n == &mchknod)
	{
		mailchk = stoi(v);
	}
		
	replace(&n->namval, v);
	attrib(n, N_ENVCHG);

	if (n == &pathnod)
	{
		zaphash();
		set_dotpath();
#ifdef VPIX
		set_vpixdir ();
#endif
		return;
	}
	
#ifdef VPIX
	if (n == &dpathnod)
	{
		set_vpixdir ();
		return;
	}
#endif

	if (flags & prompt)
	{
		if ((n == &mailpnod) || (n == &mailnod && mailpnod.namflg == N_DEFAULT))
			setmail(n->namval);
	}
}

readvar(names)
unsigned char	**names;
{
	struct fileblk	fb;
	register struct fileblk *f = &fb;
	register unsigned char	c;
	register int	rc = 0;
	struct namnod *n = lookup(*names++);	/* done now to avoid storage mess */
	unsigned long rel = relstak();

	push(f);
	initf(dup(0));

	if (lseek(0, 0L, 1) == -1)
		f->fsiz = 1;

	/*
	 * strip leading IFS characters
	 */
	while ((any((c = nextc()), ifsnod.namval)) && !(eolchar(c)))
			;
	for (;;)
	{
		if ((*names && any(c, ifsnod.namval)) || eolchar(c))
		{
			zerostak();
			assign(n, absstak(rel));
			setstak(rel);
			if (*names)
				n = lookup(*names++);
			else
				n = 0;
			if (eolchar(c))
			{
				break;
			}
			else		/* strip imbedded IFS characters */
			{
				while ((any((c = nextc()), ifsnod.namval)) &&
					!(eolchar(c)))
					;
			}
		}
		else
		{
			if(c == '\\')
				c = readc();
			pushstak(c);
			c = nextc();

			if (eolchar(c))
			{
				unsigned char *top = staktop;
			
				while (any(*(--top), ifsnod.namval))
					;
				staktop = top + 1;
			}


		}
	}
	while (n)
	{
		assign(n, nullstr);
		if (*names)
			n = lookup(*names++);
		else
			n = 0;
	}

	if (eof)
		rc = 1;
	lseek(0, (long)(f->fnxt - f->fend), 1);
	pop();
	return(rc);
}

assnum(p, i)
unsigned char	**p;
int	i;
{
	itos(i);
	replace(p, numbuf);
}

unsigned char *
make(v)
unsigned char	*v;
{
	register unsigned char	*p;

	if (v)
	{
		movstr(v, p = alloc(length(v)));
		return(p);
	}
	else
		return(0);
}


struct namnod *
lookup(nam)
	register unsigned char	*nam;
{
	register struct namnod *nscan = namep;
	register struct namnod **prev;
	int		LR;

	if (!chkid(nam))
		failed(nam, notid);
	
	while (nscan)
	{
		if ((LR = cf(nam, nscan->namid)) == 0)
			return(nscan);

		else if (LR < 0)
			prev = &(nscan->namlft);
		else
			prev = &(nscan->namrgt);
		nscan = *prev;
	}
	/*
	 * add name node
	 */
	nscan = (struct namnod *)alloc(sizeof *nscan);
	nscan->namlft = nscan->namrgt = (struct namnod *)NIL;
	nscan->namid = make(nam);
	nscan->namval = 0;
	nscan->namflg = N_DEFAULT;
	nscan->namenv = 0;

	return(*prev = nscan);
}

BOOL
chkid(nam)
unsigned char	*nam;
{
	register unsigned char *cp = nam;

	if (!letter(*cp))
		return(FALSE);
	else
	{
		while (*++cp)
		{
			if (!alphanum(*cp))
				return(FALSE);
		}
	}
	return(TRUE);
}

static int (*namfn)();
namscan(fn)
	int	(*fn)();
{
	namfn = fn;
	namwalk(namep);
}

static int
namwalk(np)
register struct namnod *np;
{
	if (np)
	{
		namwalk(np->namlft);
		(*namfn)(np);
		namwalk(np->namrgt);
	}
}

printnam(n)
struct namnod *n;
{
	register unsigned char	*s;

	sigchk();

	if (n->namflg & N_FUNCTN)
	{
		prs_buff(n->namid);
		prs_buff("(){\n");
		prf(n->namenv);
		prs_buff("\n}\n");
	}
	else if (s = n->namval)
	{
		prs_buff(n->namid);
		prc_buff('=');
		prs_buff(s);
		prc_buff(NL);
	}
}

static unsigned char *
staknam(n)
register struct namnod *n;
{
	register unsigned char	*p;

	p = movstr(n->namid, staktop);
	p = movstr("=", p);
	p = movstr(n->namval, p);
	return(getstak(p + 1 - stakbot));
}

static int namec;

exname(n)
	register struct namnod *n;
{
	register int 	flg = n->namflg;

	if (flg & N_ENVCHG)
	{

		if (flg & N_EXPORT)
		{
			free(n->namenv);
			n->namenv = make(n->namval);
		}
		else
		{
			free(n->namval);
			n->namval = make(n->namenv);
		}
	}

	
	if (!(flg & N_FUNCTN))
		n->namflg = N_DEFAULT;

	if (n->namval)
		namec++;

}

printro(n)
register struct namnod *n;
{
	if (n->namflg & N_RDONLY)
	{
		prs_buff(readonly);
		prc_buff(SP);
		prs_buff(n->namid);
		prc_buff(NL);
	}
}

printexp(n)
register struct namnod *n;
{
	if (n->namflg & N_EXPORT)
	{
		prs_buff(export);
		prc_buff(SP);
		prs_buff(n->namid);
		prc_buff(NL);
	}
}

setup_env()
{
	register unsigned char **e = environ;

	while (*e)
		setname(*e++, N_ENVNAM);
}


static unsigned char **argnam;

pushnam(n)
struct namnod *n;
{
	if (n->namval)
		*argnam++ = staknam(n);
}

unsigned char **
setenv()
{
	register unsigned char	**er;

	namec = 0;
	namscan(exname);

	argnam = er = (unsigned char **)getstak(namec * BYTESPERWORD + BYTESPERWORD);
	namscan(pushnam);
	*argnam++ = 0;
	return(er);
}

struct namnod *
findnam(nam)
	register unsigned char	*nam;
{
	register struct namnod *nscan = namep;
	int		LR;

	if (!chkid(nam))
		return(0);
	while (nscan)
	{
		if ((LR = cf(nam, nscan->namid)) == 0)
			return(nscan);
		else if (LR < 0)
			nscan = nscan->namlft;
		else
			nscan = nscan->namrgt;
	}
	return(0); 
}


unset_name(name)
	register unsigned char 	*name;
{
	register struct namnod	*n;

	if (n = findnam(name))
	{
		if (n->namflg & N_RDONLY)
			failed(name, wtfailed);

		if (n == &pathnod ||
		    n == &ifsnod ||
		    n == &ps1nod ||
		    n == &ps2nod ||
		    n == &mchknod)
		{
			failed(name, badunset);
		}

#ifndef RES

		if ((flags & rshflg) && eq(name, "SHELL"))
			failed(name, restricted);

#endif

		if (n->namflg & N_FUNCTN)
		{
			func_unhash(name);
			freefunc(n);
		}
		else
		{
			free(n->namval);
			free(n->namenv);
		}

		n->namval = n->namenv = 0;
		n->namflg = N_DEFAULT;

		if (flags & prompt)
		{
			if (n == &mailpnod)
				setmail(mailnod.namval);
			else if (n == &mailnod && mailpnod.namflg == N_DEFAULT)
				setmail(0);
		}
	}
}
