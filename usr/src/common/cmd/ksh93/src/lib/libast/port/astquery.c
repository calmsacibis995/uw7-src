#ident	"@(#)ksh93:src/lib/libast/port/astquery.c	1.1"
#pragma prototyped
/*
 * AT&T Bell Laboratories
 *
 * output printf prompt and read response
 * if format==0 then verify that interaction is possible
 *
 * return:
 *
 *	0	[1yY+]
 *	-1	[qQ] or EOF
 *	1	otherwise
 *
 * if quit>=0 then [qQ] or EOF calls exit(quit)
 */

#include <ast.h>
#include <error.h>

int
astquery(int quit, const char* format, ...)
{
	va_list		ap;
	register int	n;
	register int	c;

	static Sfio_t*	rfp;
	static Sfio_t*	wfp;

	va_start(ap, format);
	if (!format) return(0);
	if (!rfp)
	{
		c = errno;
		if (isatty(sffileno(sfstdin))) rfp = sfstdin;
		else if (!(rfp = sfopen(NiL, "/dev/tty", "r"))) return(-1);
		if (isatty(sffileno(sfstderr))) wfp = sfstderr;
		else if (!(wfp = sfopen(NiL, "/dev/tty", "w"))) return(-1);
		errno = c;
	}
	sfsync(sfstdout);
	sfvprintf(wfp, format, ap);
	sfsync(wfp);
	for (n = c = sfgetc(rfp);; c = sfgetc(rfp)) switch (c)
	{
	case EOF:
		n = c;
		/*FALLTHROUGH*/
	case '\n':
		switch (n)
		{
		case EOF:
		case 'q':
		case 'Q':
			if (quit >= 0) exit(quit);
			return(-1);
		case '1':
		case 'y':
		case 'Y':
		case '+':
			return(0);
		default:
			return(1);
		}
	}
	va_end(ap);
	/*NOTREACHED*/
}
