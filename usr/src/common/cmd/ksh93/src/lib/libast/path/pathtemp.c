#ident	"@(#)ksh93:src/lib/libast/path/pathtemp.c	1.1"
#pragma prototyped
/*
 * AT&T Bell Laboratories
 *
 * generate a temp file path name
 *
 *	[<dir>/][<pfx>]<pid>.<suf>
 *
 * length(<pfx>)<=5
 * length(<pid>)==3
 * length(<suf>)==3
 *
 *	tmpnam(p)		pathtemp(p, 0, 0)
 *	tempnam(d, p)		pathtemp(0, d, p)
 *
 * if buf==0 then space is malloc'd
 * otherwise sizeof(buf) must be >= strlen(dir) + 14
 * dir and pfx may be 0
 * if dir is 0 then sizeof(buf) must be >= TOTAL or it must
 * be a previous pathtemp() return with the same dir and pfx
 * only first 5 chars of pfx are used
 */

#include <ast.h>

#define TOTAL	128

#define TMPENV	"TMPDIR"
#define TMP1	"/tmp"
#define TMP2	"/usr/tmp"

char*
pathtemp(char* buf, const char* dir, const char* pfx)
{
	register char*		d = (char*)dir;
	char*			p = (char*)pfx;
	register char*		b;
	register char*		s;
	unsigned long		loop;

	static char*		tmpdir;
	static int		tmpbad;
	static unsigned long	seed;

	if (!d || *d && access(d, W_OK|X_OK))
	{
		if (d = getenv(TMPENV))
		{
			if (!tmpdir || !streq(tmpdir, d))
			{
				if (tmpdir)
				{
					free(tmpdir);
					tmpdir = 0;
				}
				tmpdir = strdup(d);
				tmpbad = !*d || strlen(d) >= (TOTAL - 14) || access(d, W_OK|X_OK);
			}
		}
		else if (tmpdir)
		{
			free(tmpdir);
			tmpdir = 0;
		}
		if ((tmpbad || !(d = tmpdir)) && (!*(d = astconf("TMP", NiL, NiL)) || access(d, W_OK|X_OK)) && access(d = TMP1, W_OK|X_OK) && access(d = TMP2, W_OK|X_OK))
			return(0);
	}
	if (!(b = buf) && !(b = newof(0, char, strlen(d), 14))) return(0);
	s = b + sfsprintf(b, TOTAL, "%s%s%-.5s%3.3.64ld.", d ? d : "", d && *d ? "/" : "", p ? p : "ast", getpid());
	loop = seed;
	for (;;)
	{
		sfsprintf(s, 4, "%3.3.64ld", seed++);
		if (access(b, F_OK)) return(b);
		if (seed == loop)
		{
			if (!buf) free(b);
			return(0);
		}
	}
	/*NOTREACHED*/
}
