#ident	"@(#)ksh93:src/lib/libast/string/fmtesc.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * return string with expanded escape chars
 */

#include <ast.h>

#define CHUNK	32

char*
fmtesc(const char* as)
{
	register unsigned char*	s = (unsigned char*)as;
	register char*		b;
	register int		c;

	static char*		buf;
	static int		bufsiz;

	c = 4 * (strlen((char*)s) + 1);
	if (bufsiz < c)
	{
		bufsiz = ((c + CHUNK - 1) / CHUNK) * CHUNK;
		if (!(buf = newof(buf, char, bufsiz, 0))) return(0);
	}
	b = buf;
	while (c = *s++)
	{
		if (c < 040 || c >= 0177)
		{
			*b++ = '\\';
			switch (c)
			{
			case '\007':
				c = 'a';
				break;
			case '\b':
				c = 'b';
				break;
			case '\f':
				c = 'f';
				break;
			case '\n':
				c = 'n';
				break;
			case '\r':
				c = 'r';
				break;
			case '\t':
				c = 't';
				break;
			case '\v':
				c = 'v';
				break;
			case '\033':
				c = 'E';
				break;
			default:
				b += sfsprintf(b, 4, "%03o", c);
				continue;
			}
		}
		*b++ = c;
	}
	*b = 0;
	return(buf);
}
