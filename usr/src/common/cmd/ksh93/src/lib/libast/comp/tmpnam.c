#ident	"@(#)ksh93:src/lib/libast/comp/tmpnam.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _lib_tmpnam

NoN(tmpnam)

#else

#include <stdio.h>

#ifndef P_tmpdir
#define P_tmpdir	"/usr/tmp/"
#endif

#ifndef L_tmpnam
#define L_tmpnam	(sizeof(P_tmpdir)+10)
#endif

static char	buf[L_tmpnam];
static char	seed[] = { 'a', 'a', 'a', 0 };

char*
tmpnam(register char* p)
{
	register char*	q;

	if (!p) p = buf;
	strcopy(strcopy(strcopy(p, P_tmpdir), seed), "XXXXXX");
	q = seed;
	while (*q == 'z') *q++ = 'a';
	if (*q) ++*q;
	return(mktemp(p));
}

#endif
