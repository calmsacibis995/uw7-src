#ident	"@(#)glob.c	1.5"

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char sccsid[] = "@(#)$Id$ from glob.c 5.9 (Berkeley) 2/25/91";
#endif /* not lint */

/*
 * C-shell glob for random programs.
 */

#include "config.h"

#include <sys/param.h>
#include <sys/stat.h>

#ifdef HAVE_DIRENT
#include <dirent.h>
#else
#include <sys/dir.h>
#endif

#include <pwd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifdef	INTL
#  include <locale.h>
#  include "ftpd_msg.h"
   extern nl_catd catd;
#else
#  define MSGSTR(num, str)	(str)
#endif	/* INTL */

#define	QUOTE 0200
#define	TRIM 0177
#define	eq(a,b)		(strcmp(a, b)==0)
#define	GAVSIZ		(NCARGS/6)
#define	isdir(d)	((d.st_mode & S_IFMT) == S_IFDIR)

static	char **gargv;		/* Pointer to the (stack) arglist */
static	int gargc;		/* Number args in gargv */
static	int gnleft;
static	short gflag;
#ifdef __STDC__
static int tglob(register char);
#else
static int tglob();
#endif
char	**ftpglob();
char	*globerr;
char	*home;
extern	int errno;
char	*strspl();
static	char *strend();
char	**copyblk();

static void acollect(), collect(), expand(), Gcat();
static void ginit(), matchdir(), rscan(), sort();
static int amatch(), execbrc(), match();

static	int globcnt;

char	*globchars = "`{[*?";

static	char *gpath, *gpathp, *lastgpathp;
static	int globbed;
static	char *entp;
static	char **sortbas;

#ifdef __STDC__
static void addpath(char); 
void blkfree(char **);
int letter(register char);
int digit(register char);
int gethdir(char *);
int any(register int, register char *);
#else
static void addpath();
void blkfree();
int letter();
int digit();
int gethdir();
int any();
#endif

char **
#ifdef __STDC__
ftpglob(register char *v)
#else
ftpglob(v)
register char *v;
#endif
{
	char agpath[BUFSIZ];
	char *agargv[GAVSIZ];
	char *vv[2];
	if (strstr(v, "../*/..") != (char *)NULL){
		globerr = MSGSTR(MSG_ARGS_TOO_LONG, "Arguments too long");
		return(0);
	}
	vv[0] = v;
	vv[1] = NULL;
	globerr = NULL;
	gflag = 0;
	rscan(vv, tglob);
	if (gflag == 0) {
		vv[0] = strspl(v, "");
		return (copyblk(vv));
	}

	globerr = NULL;
	gpath = agpath; gpathp = gpath; *gpathp = 0;
	lastgpathp = &gpath[sizeof agpath - 2];
	ginit(agargv); globcnt = 0;
	collect(v);
	if (globcnt == 0 && (gflag&1)) {
		blkfree(gargv), gargv = 0;
		return (0);
	} else
		return (gargv = copyblk(gargv));
}

static void
#ifdef __STDC__
ginit(char ** agargv)
#else
ginit(agargv)
char ** agargv;
#endif
{

	agargv[0] = 0; gargv = agargv; sortbas = agargv; gargc = 0;
	gnleft = NCARGS - 4;
}

static void
#ifdef __STDC__
collect(register char *as)
#else
collect(as)
register char *as;
#endif
{
	if (eq(as, "{") || eq(as, "{}")) {
		Gcat(as, "");
		sort();
	} else
		acollect(as);
}

static void
#ifdef __STDC__
acollect(register char *as)
#else
acollect(as)
register char *as;
#endif
{
	register int ogargc = gargc;

	gpathp = gpath; *gpathp = 0; globbed = 0;
	expand(as);
	if (gargc != ogargc)
		sort();
}

static void
sort()
{
	register char **p1, **p2, *c;
	char **Gvp = &gargv[gargc];

	p1 = sortbas;
	while (p1 < Gvp-1) {
		p2 = p1;
		while (++p2 < Gvp)
			if (strcmp(*p1, *p2) > 0)
				c = *p1, *p1 = *p2, *p2 = c;
		p1++;
	}
	sortbas = Gvp;
}

static void
#ifdef __STDC__
expand(char *as)
#else
expand(as)
char *as;
#endif
{
	register char *cs;
	register char *sgpathp, *oldcs;
	struct stat stb;

	sgpathp = gpathp;
	cs = as;
	if (*cs == '~' && gpathp == gpath) {
		addpath('~');
		for (cs++; letter(*cs) || digit(*cs) || *cs == '-';)
			addpath(*cs++);
		if (!*cs || *cs == '/') {
			if (gpathp != gpath + 1) {
				*gpathp = 0;
				if (gethdir(gpath + 1))
					globerr = MSGSTR(MSG_UKNOWN_USER_AFTER,
						"Unknown user name after ~");
				(void) strcpy(gpath, gpath + 1);
			} else
				(void) strcpy(gpath, home);
			gpathp = strend(gpath);
		}
	}
	while (!any(*cs, globchars)) {
		if (*cs == 0) {
			if (!globbed)
				Gcat(gpath, "");
			else if (stat(gpath, &stb) >= 0) {
				Gcat(gpath, "");
				globcnt++;
			}
			goto endit;
		}
		addpath(*cs++);
	}
	oldcs = cs;
	while (cs > as && *cs != '/')
		cs--, gpathp--;
	if (*cs == '/')
		cs++, gpathp++;
	*gpathp = 0;
	if (*oldcs == '{') {
		(void) execbrc(cs, ((char *)0));
		return;
	}
	matchdir(cs);
endit:
	gpathp = sgpathp;
	*gpathp = 0;
}

static void
#ifdef __STDC__
matchdir(char *pattern)
#else
matchdir(pattern)
char *pattern;
#endif
{
	struct stat stb;

#ifdef HAVE_DIRENT
	register struct dirent *dp;
#else
	register struct direct *dp;
#endif

	DIR *dirp;

    dirp = opendir(*gpath == '\0' ? "." : gpath);
	if (dirp == NULL) {
		if (globbed)
			return;
		goto patherr2;
	}
#ifdef HAVE_DIRFD
       if (fstat(dirfd(dirp), &stb) < 0)
#else /* HAVE_DIRFD */
        if (fstat(dirp->dd_fd, &stb) < 0)
#endif /* HAVE_DIRFD */
		goto patherr1;
	if (!isdir(stb)) {
		errno = ENOTDIR;
		goto patherr1;
	}
	while ((dp = readdir(dirp)) != NULL) {
		if (dp->d_ino == 0)
			continue;
		if (match(dp->d_name, pattern)) {
			Gcat(gpath, dp->d_name);
			globcnt++;
		}
	}
	closedir(dirp);
	return;

patherr1:
	closedir(dirp);
patherr2:
	globerr = MSGSTR(MSG_BAD_DIR_COMP, "Bad directory components");
}

static int
#ifdef __STDC__
execbrc(char *p, char *s)
#else
execbrc(p, s)
char *p, *s;
#endif
{
	char restbuf[BUFSIZ + 2];
	register char *pe, *pm, *pl;
	int brclev = 0;
	char *lm, savec, *sgpathp;

	for (lm = restbuf; *p != '{'; *lm++ = *p++)
		continue;
	for (pe = ++p; *pe; pe++)
	switch (*pe) {

	case '{':
		brclev++;
		continue;

	case '}':
		if (brclev == 0)
			goto pend;
		brclev--;
		continue;

	case '[':
		for (pe++; *pe && *pe != ']'; pe++)
			continue;
		continue;
	}
pend:
	brclev = 0;
	for (pl = pm = p; pm <= pe; pm++)
	switch (*pm & (QUOTE|TRIM)) {

	case '{':
		brclev++;
		continue;

	case '}':
		if (brclev) {
			brclev--;
			continue;
		}
		goto doit;

	case ','|QUOTE:
	case ',':
		if (brclev)
			continue;
doit:
		savec = *pm;
		*pm = 0;
		(void) strcpy(lm, pl);
		(void) strcat(restbuf, pe + 1);
		*pm = savec;
		if (s == 0) {
			sgpathp = gpathp;
			expand(restbuf);
			gpathp = sgpathp;
			*gpathp = 0;
		} else if (amatch(s, restbuf))
			return (1);
		sort();
		pl = pm + 1;
		if (brclev)
			return (0);
		continue;

	case '[':
		for (pm++; *pm && *pm != ']'; pm++)
			continue;
		if (!*pm)
			pm--;
		continue;
	}
	if (brclev)
		goto doit;
	return (0);
}

static int
#ifdef __STDC__
match(char *s, char *p)
#else
match(s,p)
char *s, *p;
#endif
{
	register int c;
	register char *sentp;
	char sglobbed = globbed;

	if (*s == '.' && *p != '.')
		return (0);
	sentp = entp;
	entp = s;
	c = amatch(s, p);
	entp = sentp;
	globbed = sglobbed;
	return (c);
}

static int
#ifdef __STDC__
amatch(char *s, char *p)
#else
amatch(s, p)
char *s, *p;
#endif
{
	register int scc;
	int ok, lc;
	char *sgpathp;
	struct stat stb;
	int c, cc;

	globbed = 1;
	for (;;) {
		scc = *s++ & TRIM;
		switch (c = *p++) {

		case '{':
			return (execbrc(p - 1, s - 1));

		case '[':
			ok = 0;
			lc = 077777;
			while (cc = *p++) {
				if (cc == ']') {
					if (ok)
						break;
					return (0);
				}
				if (cc == '-') {
					if (lc <= scc && scc <= *p++)
						ok++;
				} else
					if (scc == (lc = cc))
						ok++;
			}
			if (cc == 0)
				if (ok)
					p--;
				else
					return 0;
			continue;

		case '*':
			if (!*p)
				return (1);
			if (*p == '/') {
				p++;
				goto slash;
			}
			s--;
			do {
				if (amatch(s, p))
					return (1);
			} while (*s++);
			return (0);

		case 0:
			return (scc == 0);

		default:
			if (c != scc)
				return (0);
			continue;

		case '?':
			if (scc == 0)
				return (0);
			continue;

		case '/':
			if (scc)
				return (0);
slash:
			s = entp;
			sgpathp = gpathp;
			while (*s)
				addpath(*s++);
			addpath('/');
			if (stat(gpath, &stb) == 0 && isdir(stb))
				if (*p == 0) {
					Gcat(gpath, "");
					globcnt++;
				} else
					expand(p);
			gpathp = sgpathp;
			*gpathp = 0;
			return (0);
		}
	}
}

static int
#ifdef __STDC__
Gmatch(register char *s, register char *p)
#else
Gmatch(s,p)
register char *s, *p;
#endif
{
	register int scc;
	int ok, lc;
	int c, cc;

	for (;;) {
		scc = *s++ & TRIM;
		switch (c = *p++) {

		case '[':
			ok = 0;
			lc = 077777;
			while (cc = *p++) {
				if (cc == ']') {
					if (ok)
						break;
					return (0);
				}
				if (cc == '-') {
					if (lc <= scc && scc <= *p++)
						ok++;
				} else
					if (scc == (lc = cc))
						ok++;
			}
			if (cc == 0)
				if (ok)
					p--;
				else
					return 0;
			continue;

		case '*':
			if (!*p)
				return (1);
			for (s--; *s; s++)
				if (Gmatch(s, p))
					return (1);
			return (0);

		case 0:
			return (scc == 0);

		default:
			if ((c & TRIM) != scc)
				return (0);
			continue;

		case '?':
			if (scc == 0)
				return (0);
			continue;

		}
	}
}

static void
#ifdef __STDC__
Gcat(register char *s1, register char *s2)
#else
Gcat(s1,s2)
register char *s1, *s2;
#endif
{
	register int len = strlen(s1) + strlen(s2) + 1;

	if (len >= gnleft || gargc >= GAVSIZ - 1)
		globerr = MSGSTR(MSG_ARGS_TOO_LONG, "Arguments too long");
	else {
		gargc++;
		gnleft -= len;
		gargv[gargc] = 0;
		gargv[gargc - 1] = strspl(s1, s2);
	}
}

static void
#ifdef __STDC__
addpath(char c)
#else
addpath(c)
char c;
#endif
{

	if (gpathp >= lastgpathp)
		globerr = MSGSTR(MSG_PATH_TOO_LONG, "Pathname too long");
	else {
		*gpathp++ = c;
		*gpathp = 0;
	}
}

static void
#ifdef __STDC__
rscan(register char **t, int (*f)())
#else
rscan(t,f)
register char **t;
int (*f)();
#endif
{
	register char *p, c;

	while (p = *t++) {
		if (*p == '~')
			gflag |= 2;
		else if (eq(p, "{") || eq(p, "{}"))
			continue;
		while (c = *p++)
			(*f)(c);
	}
}
static int
#ifdef __STDC__
tglob(register char c)
#else
tglob(c)
register char c;
#endif
{

	if (any(c, globchars))
		gflag |= c == '{' ? 2 : 1;
	return (c);
}

int
#ifdef __STDC__
letter(register char c)
#else
letter(c)
register char c;
#endif
{

	return ((c >= 'a') && (c <= 'z') || (c >= 'A') && (c <= 'Z')
		|| (c == '_'));
}

int
#ifdef __STDC__
digit(register char c)
#else
digit(c)
register char c;
#endif
{

	return (c >= '0' && c <= '9');
}

int
#ifdef __STDC__
any(register int c, register char *s)
#else
any(c,s)
register int c;
register char *s;
#endif
{

	while (*s)
		if (*s++ == c)
			return(1);
	return(0);
}

int
#ifdef __STDC__
blklen(register char **av)
#else
blklen(av)
register char **av;
#endif
{
	register int i = 0;

	while (*av++)
		i++;
	return (i);
}

char **
#ifdef __STDC__
blkcpy(char **oav, register char **bv)
#else
blkcpy(oav, bv)
char **oav, **bv;
#endif
{
	register char **av = oav;

	while (*av++ = *bv++)
		continue;
	return (oav);
}

void
#ifdef __STDC__
blkfree(char **av0)
#else
blkfree(av0)
char **av0;
#endif
{
	register char **av = av0;

	while (*av)
		free(*av++);
}

char *
#ifdef __STDC__
strspl(register char *cp, register char *dp)
#else
strspl(cp, dp)
register char *cp, *dp;
#endif
{
	register char *ep = 
	  (char *)malloc((unsigned)(strlen(cp) + strlen(dp) + 1));

	if (ep == (char *)0)
		fatal(MSGSTR(MSG_OUT_OF_MEMORY, "Out of memory"));
	(void) strcpy(ep, cp);
	(void) strcat(ep, dp);
	return (ep);
}

char **
#ifdef __STDC__
copyblk(register char **v)
#else
copyblk(v)
register char **v;
#endif
{
	register char **nv = (char **)malloc((unsigned)((blklen(v) + 1) *
						sizeof(char **)));
	if (nv == (char **)0)
		fatal(MSGSTR(MSG_OUT_OF_MEMORY,"Out of memory"));

	return (blkcpy(nv, v));
}

static
char *
#ifdef __STDC__
strend(register char *cp)
#else
strend(cp)
register char *cp;
#endif
{

	while (*cp)
		cp++;
	return (cp);
}
/*
 * Extract a home directory from the password file
 * The argument points to a buffer where the name of the
 * user whose home directory is sought is currently.
 * We write the home directory of the user back there.
 */
int
#ifdef __STDC__
gethdir(char *home)
#else
gethdir(home)
char *home;
#endif
{
	register struct passwd *pp = getpwnam(home);

	if (!pp || home + strlen(pp->pw_dir) >= lastgpathp)
		return (1);
	(void) strcpy(home, pp->pw_dir);
	return (0);
}
