#ident	"@(#)ksh93:src/lib/libast/string/strelapsed.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * parse elapsed time in 1/n secs from s
 * compatible with fmtelapsed()
 * if e!=0 then it is set to first unrecognized char
 */

#include <ast.h>
#include <ctype.h>

unsigned long
strelapsed(register const char* s, char** e, int n)
{
	register int		c;
	register unsigned long	v;
	unsigned long		t = 0;
	int			f = 0;
	int			m;
	const char*		last;

	while (isspace(*s)) s++;
	if (*s == '%')
	{
		t = ~t;
		last = s + 1;
	}
	else while (*(last = s))
	{
		v = 0;
		while ((c = *s++) >= '0' && c <= '9')
			v = v * 10 + c - '0';
		v *= n;
		if (c == '.')
			for (m = n; (c = *s++) >= '0' && c <= '9';)
				f += (m /= 10) * (c - '0');
		switch (c)
		{
		case 'S':
			v *= 20 * 12 * 4 * 7 * 24 * 60 * 60;
			break;
		case 'Y':
			v *= 12 * 4 * 7 * 24 * 60 * 60;
			break;
		case 'M':
			v *= 4 * 7 * 24 * 60 * 60;
			break;
		case 'w':
			v *= 7 * 24 * 60 * 60;
			break;
		case 'd':
			v *= 24 * 60 * 60;
			break;
		case 'h':
			v *= 60 * 60;
			break;
		case 'm':
			v *= 60;
			break;
		case 0:
			s--;
			/*FALLTHROUGH*/
		case 's':
			v += f;
			f = 0;
			break;
		default:
			if (isspace(c)) t += v + f;
			goto done;
		}
		t += v;
	}
 done:
	if (e) *e = (char*)last;
	return(t);
}
