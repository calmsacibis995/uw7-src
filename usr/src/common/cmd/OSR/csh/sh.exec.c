#ident	"@(#)OSRcmds:csh/sh.exec.c	1.1"
/*
 *	@(#) sh.exec.c 25.1 94/02/18 
 *
 *	      UNIX is a registered trademark of AT&T
 *		Portions Copyright 1976-1989 AT&T
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1994 The Santa Cruz Operation, Inc
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

/* #ident	"@(#)csh:sh.exec.c	1.2" */
#pragma comment(exestr, "@(#) sh.exec.c 25.1 94/02/18 ")

/* Modification History
 * S000	28 june 1989	sco!hoeshuen
 * The following kludge to pass the full pathname of the command as argv[0] 
 * has been removed :
 *	kludge to enable shell scripts to be executed along path variable 
 *	without giving absolute path name or the script residing in the 
 *	current directory
 *	tsav = t[0]; *	t[0] = f;
 *	then do an execv(f,t);
 *	t[0] = tsav;
 * This breaks some commands which expect argv[0] to be just the last
 * component of the path. This kludge is needed if the execv here fails, meaning
 * shell has to be invoked to run the command. Then its full path name has to 
 * be supplied through execv to whatever shell we will use. But the work is 
 * already done in the code. 
 * 
 * S001	10 July 1989	sco!hoeshuen
 * - use opendir(), readdir() and closedir() instead of open(), read() and
 *   close() for directory operations.
 *
 * L002 17 February 1994	scol!gregw
 * - renamed malloc, calloc, and free to Malloc, Calloc, and Free to avoid
 *   clash with libc.
 */

#include "sh.h"

/*
 * C shell
 */

/*
 * System level search and execute of a command.
 * We look in each directory for the specified command name.
 * If the name contains a '/' then we execute only the full path name.
 * If there is no search path then we execute only full path names.
 */

/* 
 * As we search for the command we note the first non-trivial error
 * message for presentation to the user.  This allows us often
 * to show that a file has the wrong mode/no access when the file
 * is not in the last component of the search path, so we must
 * go on after first detecting the error.
 */
char	*exerr;			/* Execution error message */
char	*expath;		/* Path for exerr */

/*
 * Xhash is an array of HSHSIZ chars, which are used to hash execs.
 * If it is allocated, then to tell whether ``name'' is (possibly)
 * present in the i'th component of the variable path, you look at
 * the i'th bit of xhash[hash("name")].  This is setup automatically
 * after .login is executed, and recomputed whenever ``path'' is
 * changed.
 */
int	havhash;
#define	HSHSIZ	511
char	xhash[HSHSIZ];
#ifdef VFORK
int	hits, misses;
#endif

/* Dummy search path for just absolute search when no path */
char	*justabs[] =	{ "", 0 };

doexec(t)
	register struct command *t;
{
	char *sav;
	register char *dp, **pv, **av;
	register struct varent *v;
	bool slash = any('/', t->t_dcom[0]);
	int hashval, i;
	char *blk[2];

	/*
	 * Glob the command name.  If this does anything, then we
	 * will execute the command only relative to ".".  One special
	 * case: if there is no PATH, then we execute only commands
	 * which start with '/'.
	 */
	dp = globone(t->t_dcom[0]);
	sav = t->t_dcom[0];
	exerr = 0; expath = t->t_dcom[0] = dp;
	xfree(sav);
	v = adrof("path");
	if (v == 0 && expath[0] != '/')
		pexerr();
	slash |= gflag;

	/*
	 * Glob the argument list, if necessary.
	 * Otherwise trim off the quote bits.
	 */
	gflag = 0; av = &t->t_dcom[1];
	rscan(av, tglob);
	if (gflag) {
		av = glob(av);
		if (av == 0)
			error("No match");
	}
	blk[0] = t->t_dcom[0];
	blk[1] = 0;
	av = blkspl(blk, av);
#ifdef VFORK
	Vav = av;
#endif
	scan(av, trim);

	xechoit(av);		/* Echo command if -x */
	closech();		/* Close random fd's */

	/*
	 * We must do this after any possible forking (like `foo`
	 * in glob) so that this shell can still do subprocesses.
	 */
#ifdef VFORK
#if BSD
#ifdef notdef
	sigsys(SIGCHLD, SIG_IGN);	/* sigsys for vforks sake */
#endif
	sigsetmask(0);
#else
	sigsys(SIGCHLD, SIG_DFL);	/* sigsys for vforks sake */
#endif
#endif /* VFORK */

	/*
	 * If no path, no words in path, or a / in the filename
	 * then restrict the command search.
	 */
	if (v == 0 || v->vec[0] == 0 || slash)
		pv = justabs;
	else
		pv = v->vec;
	sav = strspl("/", *av);		/* / command name for postpending */
#ifdef VFORK
	Vsav = sav;
#endif
	if (havhash)
		hashval = xhash[hash(*av)];
	i = 0;
#ifdef VFORK
	hits++;
#endif
	do {

#ifdef VPIX
		if (dospath (*pv))
		{   char *dossimple, *dosfull, *dossav, **dossfx;
		    static char *suffixes[] =
			    { ".com", ".exe", ".bat", 0 };

		    for (dossfx = suffixes; *dossfx; dossfx++)
		    {   dossimple = strspl (*av, *dossfx);
			if (slash || pv[0][0] == 0 || eq (pv[0], "."))
				dosexec (dossimple, av);
			else if (pv[0][0] != '/' || (havhash &&
			    (xhash[hash (dossimple)] & (1 << (i % 8)))))
			{       dossav = strspl ("/", dossimple);
				dosfull = strspl (*pv, dossav);
				dosexec (dosfull, av);
				xfree (dossav);
				xfree (dosfull);
			}
			xfree (dossimple);
		    }
		}
#endif /* VPIX */

		if (!slash && pv[0][0] == '/' && havhash && (hashval & (1 << (i % 8))) == 0)
			goto cont;
		if (pv[0][0] == 0 || eq(pv[0], "."))	/* don't make ./xxx */
			texec(*av, av);
		else {
			dp = strspl(*pv, sav);
#ifdef VFORK
			Vdp = dp;
#endif
			texec(dp, av);
#ifdef VFORK
			Vdp = 0;
#endif
			xfree(dp);
		}
#ifdef VFORK
		misses++;
#endif
cont:
		pv++;
		i++;
	} while (*pv);
#ifdef VFORK
	hits--;
#endif
#ifdef VFORK
	Vsav = 0;
	Vav = 0;
#endif
	xfree(sav);
	xfree(av);
	pexerr();
}

#ifdef VPIX
/* see if path is in DOSPATH */
dospath(path)
	char *path;
{
	register char   *scanp, *scandp;
#define COLON   ':'

	if (path == 0) return 0;
	if ((scandp = getenv ("DOSPATH")) == 0) return 0;
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

/* execute a DOS program by running vpix with adjusted args */
dosexec(f, av)
char *f, **av;  /* f = full or relative path of file, av = arg vector */
{       register char **newav, *dp, **pv;
	char *sav;
	register struct varent *v;
	int i, hashval;
	static char *vpix = "vpix", *vpixflag = "-c";

	/* make sure cache hit was for this file and we can read it */
	if (access (f, 4) < 0) return;

	/* set up the new argument list */
	for (i = 0; av[i]; i++);        /* count args */
	newav = (char **)Calloc (i + 3, sizeof (char *));	/* L002 */
	newav[i + 2] = 0;
	while (--i > 0) newav[i + 2] = av[i];
	newav[2] = f;
	newav[1] = vpixflag;
	newav[0] = vpix;

	/* prepare to look for "vpix" program */
	v = adrof ("path");
	if (v == 0 || v->vec[0] == 0) pv = justabs;
	else pv = v->vec;
	sav = strspl ("/", vpix);
	if (havhash) hashval = xhash[hash (vpix)];
	i = 0;

	/* look for "vpix" program */
	do
	{       if (pv[0][0] == '/' && havhash &&
				(hashval & (1 << (i % 8))) == 0)
			goto cont;
		if (pv[0] == 0 || eq (pv[0], "."))
			execv (*newav, newav);
		else
		{       dp = strspl (*pv, sav);
			execv (dp, newav);
			xfree (dp);
		}
  cont:
		pv++;
		i++;
	}
	while (*pv);
	xfree (sav);
	xfree (newav);

	expath = vpix;          /* found DOS program but not vpix */
}
#endif /* VPIX */

pexerr()
{

	/* Couldn't find the damn thing */
	setname(expath);
	/* xfree(expath); */
	if (exerr)
		bferr(exerr);
	bferr("Command not found");
}

/* Last resort shell */
static char	*lastsh[] =	{ SHELLPATH, 0 };

/*
 * Execute command f, arg list t.
 * Record error message if not found.
 * Also do shell scripts here.
 */
texec(f, t)
	char *f;
	register char **t;
{
	register struct varent *v;
	register char **vp;
	extern char *sys_errlist[];

	/* S000 */
	execv(f, t);
	switch (errno) {

	case ENOEXEC:
		/*
		 * If there is an alias for shell, then
		 * put the words of the alias in front of the
		 * argument list replacing the command name.
		 * Note no interpretation of the words at this point.
		 */
		v = adrof1("shell", &aliases);
		if (v == 0) {
#if defined(OTHERSH) || !defined(BSD)
			register int ff = open(f, 0);
			char ch;
#endif
			vp = lastsh;
#if BSD
			vp[0] = adrof("shell") ? value("shell") : SHELLPATH;
#	ifdef OTHERSH
			if (ff != -1 && read(ff, &ch, 1) == 1 && ch != '#')
					vp[0] = OTHERSH;
#	endif
#else
	/* execute OTHERSH unless #! present */
			if (       ff != -1
				&& read(ff, &ch, 1) == 1
				&& ch == '#'
				&& read(ff, &ch, 1) == 1
				&& ch == '!') {
					char **getpath();
					vp = getpath(ff);
			} else
#	ifdef OTHERSH
					vp[0] = OTHERSH;
#	else
					vp[0] = SHELLPATH;
#	endif
#endif

#if defined(OTHERSH) || !defined(BSD)
			close(ff);
#endif
		} else
			vp = v->vec;
		t[0] = f;
		t = blkspl(vp, t);		/* Splice up the new arglst */
		f = *t;
		execv(f, t);
		xfree((char *)t);
		/* The sky is falling, the sky is falling! */

	case ENOMEM:
		Perror(f);

	case ENOENT:
		break;

	default:
		if (exerr == 0) {
			exerr = sys_errlist[errno];
			expath = savestr(f);
		}
	}
}

#if !BSD
char **
getpath(ff)
register int ff;
{
	register char **vp;
	char tmp[80];		/* max. pathname (or var. name) len. is 80 */
	register int i=0;
	register int j=0;
	char ch;

	/* max. number of arg.s is 10 */
	vp = (char **) Calloc(11, sizeof(char *));		/* L002 */
	
#ifdef OTHERSH
	vp[0] = OTHERSH;
#else
	vp[0] = SHELLPATH;
#endif	
	ch = '\n';	/* in case of "#!" only */
	if (rmwhite(ff, &ch) < 0)
		return(vp);
	while (ch != '\n') {
		do {
			tmp[i++] = ch;
		} while (read(ff, &ch, 1) == 1 &&
		       (ch != '\n' && ch != '\t' && ch != ' '));
		tmp[i] = '\0';
		vp[j] = (char *) Malloc(strlen(tmp) + 1);	/* L002 */
		strcpy(vp[j], tmp);
		j++;
		i = 0;
		if (ch != '\n')
			if (rmwhite(ff, &ch) < 0)
				break;
	}
	vp[j] = 0;
	return(vp);
}

rmwhite(ff, cptr)
int ff;
char *cptr;
{
	while (read(ff, cptr, 1) == 1)
		if (*cptr != '\n' && *cptr != '\t' && *cptr != ' ')
			return(0);
	return(-1);
}
#endif

execash(t, kp)
	register struct command *kp;
{

	didcch++;
	signal(SIGINT, parintr);
	signal(SIGQUIT, parintr);
	signal(SIGTERM, parterm);		/* if doexec loses, screw */
	lshift(kp->t_dcom, 1);
	exiterr++;
	doexec(kp);
	/*NOTREACHED*/
}

xechoit(t)
	char **t;
{

	if (adrof("echo")) {
		flush();
		haderr = 1;
		blkpr(t), printf("\n");
		haderr = 0;
	}
}

dohash()
{
	struct stat stb;
	char *d_name;						/* S001 */
	register int cnt;
	DIR *dirf; 						/* S001 */
	struct dirent *ep;					/* S001 */
	int i = 0;
	struct varent *v = adrof("path");
	char **pv;

	havhash = 1;
	for (cnt = 0; cnt < HSHSIZ; cnt++)
		xhash[cnt] = 0;
	if (v == 0)
		return;
	for (pv = v->vec; *pv; pv++, i = (i + 1) % 8) {
		if (pv[0][0] != '/')
			continue;
		dirf = opendir(*pv);				/* S001 */
		if (dirf == (DIR *)0)				/* S001 */
			continue;
		if (fstat(dirf->dd_fd, &stb) < 0 || !isdir(stb)) {  /* S001 */
			closedir(dirf);				/* S001 */
			continue;
		}
		while ((ep = readdir(dirf)) != (struct dirent *)0) { /* S001 */
			if (ep->d_ino == 0)
				continue;
								   /* L002 */
			d_name = (char *)Malloc(ep->d_reclen + 1); /* S001 */
			copdent(d_name, ep->d_name);
			xhash[hash(d_name)] |= (1 << i);
								/* L002 */
			Free(d_name);				/* S001 */
		}
		closedir(dirf);					/* S001 */
	}
}

dounhash()
{

	havhash = 0;
}

#ifdef VFORK
hashstat()
{

	if (hits+misses)
	printf("%d hits, %d misses, %2d%%\n", hits, misses, 100 * hits / (hits + misses));
}
#endif

hash(cp)
	register char *cp;
{
	register long hash = 0;
	int retval;

	while (*cp)
		hash += hash + *cp++;
	if (hash < 0)
		hash = -hash;
	retval = hash % HSHSIZ;
	return (retval);
}

