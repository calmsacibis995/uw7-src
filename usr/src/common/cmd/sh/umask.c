#ident	"@(#)sh:common/cmd/sh/umask.c	1.1"

/*
 * umask builtin
 */

#include <pfmt.h>
#include "defs.h"

#define	USER	0700
#define	GROUP	0070
#define	OTHER	0007
#define	ALL	0777
#define	READ	0444
#define	WRITE	0222
#define	EXEC	0111

unsigned char *msp;
static mode_t newmask(), oct(), who();
static int what();

void
sysumask(argc, argv)
int argc;
unsigned char **argv;
{
	register int sflag = 0;
	unsigned char *a1 = argv[1];
	mode_t curmask, i;
	int j;

	if (a1)	{
		if (eq(a1, "-S"))
			sflag = 1;
		else if (eq(a1, "--"))
			a1 = argv[2];
		if (a1 && !sflag)	{
			msp = a1;
			(void)umask(curmask = umask(0));
			i = newmask(curmask);
			(void)umask(i);
			return;
		}
	}
	(void)umask(curmask = umask(0));
	if (sflag)	{
		char ogu[] = "ogu";

		for (j = 2; j >=0; j--)	{
			prc_buff(ogu[j]);
			prc_buff('=');
			i = ~(curmask >> (j*3)) & 07;
			if (i & READ)
				prc_buff('r');
			if (i & WRITE)
				prc_buff('w');
			if (i & EXEC)
				prc_buff('x');
			if (j)
				prc_buff(',');
		}
		prc_buff(NL);
	}
	else	{
		prc_buff('0');
		for (j = 6; j >= 0; j -= 3)
		prc_buff(((curmask >> j) & 07) +'0');
		prc_buff(NL);
	}
}
static mode_t
newmask(cm)
mode_t cm;
{
	/*	newmask() parses the symbolic_mode argument to
	 *	create a new umask.  The resulting umask is the
	 *	logical complement of the file permission bits
	 *	specified by the symbolic_mode.
	 *
	 *	m contains USER|GROUP|OTHER information
	 *	b contains rwx information
	 *	o contains +|-|= information
	 */
	mode_t m, b;
	register int o;
	register int goon;
	mode_t om = cm;		/* save current mask */

	if (isdigit(*msp))
		return(oct(om));
	do	{
		m = who();
		while (o = what())	{
			b = 0;
			goon = 0;
			switch (*msp)	{
			case 'u':	/* use USER info from current mask */
				b = (cm & USER) >> 6;
				goto dup;
			case 'g':	/* use GROUP info from current mask */
				b = (cm & GROUP) >> 3;
				goto dup;
			case 'o':	/* use OTHER info from current mask */
				b = (cm & OTHER);
		dup:
				b |= (b << 3) | (b << 6);
				b = ~b;
				msp++;
				goon = 1;
			}
			while (goon == 0) switch (*msp++)	{
			case 'r':
				b |= READ;
				continue;
			case 'w':
				b |= WRITE;
				continue;
			case 'x':
				b |= EXEC;
				continue;
			default:
				msp--;
				goon = 1;
			}

			b &= m;

			switch(o)	{
			case '+':	/* allow perm; i.e. remove from mask */
				cm &= ~b;
				break;
			case '-':	/* disallow perm; i.e. add to mask */
				cm |= b;
				break;
			case '=':	/* mask is inverse of allowed perms */
				cm |= m;
				cm &= ~b;
				break;
			}
		}
	} while (*msp++ == ',');
	if (*--msp)	{
		error_fail(SYSUMASK, badumask, badumaskid);
		return(om);
	}
	return(cm);
}

static mode_t
oct(om)
mode_t om;
{
	char *badchr;
	mode_t i;

	i = (mode_t)strtol((char *)msp, &badchr, 8) ;
	if (*badchr != '\0')	{
		error_fail(SYSUMASK, badumask, badumaskid);
		return(om);
	}
	return(i);
}

static mode_t
who()
{
	register mode_t m = 0;

	for (;; msp++) switch (*msp)	{
	case 'u':
		m |= USER;
		continue;
	case 'g':
		m |= GROUP;
		continue;
	case 'o':
		m |= OTHER;
		continue;
	case 'a':
		m |= ALL;
		continue;
	default:
		if (m == 0)
			m = ALL;
		return m;
	}
}

static int
what()
{
	switch (*msp)	{
	case '+':
	case '-':
	case '=':
		return *msp++;
	}
	return(0);
}
