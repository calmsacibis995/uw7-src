/*	copyright	"%c%"	*/

#ident	"@(#)awk:b.c	2.12.5.1"

#include "awk.h"
#include <ctype.h>
#include <stdio.h>
#include "y.tab.h"
#include <pfmt.h>

uchar	*patbeg;
int	patlen;

static void
#ifdef __STDC__
reprob(fa *f, int e)
#else
reprob(f, e, p)fa *f; int e;
#endif
{
	char msg[BUFSIZ];

	regerror(e, &f->re, msg, sizeof(msg));
	error(MM_ERROR, ":104:Error in RE `%s': %s", f->restr, msg);
}

static fa *
#ifdef __STDC__
mkdfa(uchar *s)	/* build DFA from s */
#else
mkdfa(s)uchar *s;
#endif
{
	fa *pfa;
	int i;

	if ((pfa = (fa *)malloc(sizeof(fa))) == 0)
	{
		error(MM_ERROR,
			"5:Regular expression too big: out of space in %s", s);
	}
	if ((i = regcomp(&pfa->re, (char *)s, REG_EXTENDED | REG_ONESUB
		| REG_BKTEMPTY | REG_BKTESCAPE | REG_ESCSEQ))
		!= 0)
	{
		pfa->restr = s;
		reprob(pfa, i);
	}
	pfa->restr = tostring(s);
	pfa->use = 1;
	pfa->notbol = 0;
	return pfa;
}

fa *
#ifdef __STDC__
makedfa(uchar *s, int leftmost)	/* build and cache DFA from s */
#else
makedfa(s, leftmost)uchar *s; int leftmost;
#endif
{
	static fa *fatab[20];
	static int nfatab;
	int i, n, u;
	fa *pfa;

	if (compile_time)
		return mkdfa(s);
	/*
	* Search for a match to those cached.
	* If not found, save it, tossing least used one when full.
	*/
	for (i = 0; i < nfatab; i++)
	{
		if (strcmp(fatab[i]->restr, s) == 0)
		{
			fatab[i]->use++;
			return fatab[i];
		}
	}
	pfa = mkdfa(s);
	if ((n = nfatab) < sizeof(fatab) / sizeof(fa *))
		nfatab++;
	else
	{
		n = 0;
		u = fatab[0]->use;
		for (i = 1; i < sizeof(fatab) / sizeof(fa *); i++)
		{
			if (fatab[i]->use < u)
			{
				n = i;
				u = fatab[n]->use;
			}
		}
		free((void *)fatab[n]->restr);
		regfree(&fatab[n]->re);
		free((void *)fatab[n]);
	}
	fatab[n] = pfa;
	return pfa;
}

int
#ifdef __STDC__
match(fa *f, uchar *p)	/* does p match f anywhere? */
#else
match(f, p)fa *f; uchar *p;
#endif
{
	int err;

	if ((err = regexec(&f->re, (char *)p, (size_t)0, (regmatch_t *)0, 0)) == 0)
		return 1;
	if (err != REG_NOMATCH)
		reprob(f, err);
	return 0;
}

int
#ifdef __STDC__
pmatch(fa *f, uchar *p) /* find leftmost longest (maybe empty) match */
#else
pmatch(f, p)fa *f; uchar *p;
#endif
{
	regmatch_t m;
	int err;

	if ((err = regexec(&f->re, (char *)p, (size_t)1, &m, f->notbol)) == 0)
	{
		patbeg = &p[m.rm_so];
		patlen = m.rm_eo - m.rm_so;
		return 1;
	}
	if (err != REG_NOMATCH)
		reprob(f, err);
	patlen = -1;
	return 0;
}

int
#ifdef __STDC__
nematch(fa *f, uchar *p) /* find leftmost longest nonempty match */
#else
nematch(f, p)fa *f; uchar *p;
#endif
{
	regmatch_t m;
	int err;

	for (;;)
	{
		if ((err = regexec(&f->re, (char *)p, (size_t)1, &m,
			f->notbol | REG_NONEMPTY)) == 0)
		{
			if ((patlen = m.rm_eo - m.rm_so) == 0)
			{
				p += m.rm_eo;
				continue;
			}
			patbeg = &p[m.rm_so];
			return 1;
		}
		if (err != REG_NOMATCH)
			reprob(f, err);
		patlen = -1;
		return 0;
	}
}
