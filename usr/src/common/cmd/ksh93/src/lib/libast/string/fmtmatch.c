#ident	"@(#)ksh93:src/lib/libast/string/fmtmatch.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * return strmatch() expression given RE
 * 0 returned for invalid RE
 */

#include <ast.h>

#define CHUNK		32

char*
fmtmatch(const char* as)
{
	register char*	s = (char*)as;
	register int	c;
	register char*	t;
	register char**	p;
	register char*	b;
	char*		x;
	char*		y;
	int		a;
	int		e;
	int		n;
	char*		stack[32];

	static char*	buf;
	static int	bufsiz;

	c = 3 * (strlen(s) + 1);
	if (bufsiz < c)
	{
		bufsiz = ((c + CHUNK - 1) / CHUNK) * CHUNK;
		if (!(buf = newof(buf, char, bufsiz, 0))) return(0);
	}
	t = b = buf + 3;
	p = stack;
	if (a = *s == '^') s++;
	e = 0;
	for (;;)
	{
		switch (c = *s++)
		{
		case 0:
			break;
		case '\\':
			if (!(c = *s++)) return(0);
			switch (*s)
			{
			case '*':
			case '+':
			case '?':
				*t++ = *s++;
				*t++ = '(';
				*t++ = '\\';
				*t++ = c;
				c = ')';
				break;
			case '|':
				if (c == '(')
				{
					*t++ = c;
					c = *s++;
					goto alternate;
				}
				break;
			case '{':
			case '}':
				break;
			default:
				*t++ = '\\';
				break;
			}
			*t++ = c;
			continue;
		case '[':
			x = t;
			*t++ = c;
			if ((c = *s++) == '^')
			{
				*t++ = '!';
				c = *s++;
			}
			else if (c == '!')
			{
				*t++ = '\\';
				*t++ = c;
				c = *s++;
			}
			for (;;)
			{
				if (!(*t++ = c)) return(0);
				if (c == '\\') *t++ = c;
				if ((c = *s++) == ']')
				{
					*t++ = c;
					break;
				}
			}
			switch (*s)
			{
			case '*':
			case '+':
			case '?':
				for (y = t + 2, t--; t >= x; t--) *(t + 2) = *t;
				*++t = *s++;
				*++t = '(';
				t = y;
				*t++ = ')';
				break;
			}
			continue;
		case '(':
			if (p >= &stack[elementsof(stack)]) return(0);
			*p++ = t;
			*t++ = '@';
			*t++ = '(';
			continue;
		case ')':
			if (p == stack) return(0);
			p--;
			*t++ = c;
			switch (*s)
			{
			case 0:
				break;
			case '*':
			case '+':
			case '?':
				**p = *s++;
				continue;
			default:
				continue;
			}
			break;
		case '.':
			switch (*s)
			{
			case 0:
				*t++ = '?';
				break;
			case '*':
				s++;
				*t++ = '*';
				e = !*s;
				continue;
			case '+':
				s++;
				*t++ = '?';
				*t++ = '*';
				continue;
			case '?':
				s++;
				*t++ = '?';
				*t++ = '(';
				*t++ = '?';
				*t++ = ')';
				continue;
			default:
				*t++ = '?';
				continue;
			}
			break;
		case '*':
		case '+':
		case '?':
			n = *(t - 1);
			if (t == b || n == '(' || n == '|') return(0);
			*(t - 1) = c;
			*t++ = '(';
			*t++ = n;
			*t++ = ')';
			continue;
		case '|':
			if (t == b || *(t - 1) == '(') return(0);
		alternate:
			if (!*s || *s == ')') return(0);
			if (p == stack && b == buf + 3)
			{
				*--b = '(';
				*--b = '@';
			}
			*t++ = c;
			continue;
		case '$':
			if (e = !*s) break;
			/*FALLTHROUGH*/
		default:
			*t++ = c;
			continue;
		}
		break;
	}
	if (p != stack) return(0);
	if (b != buf + 3) *t++ = ')';
	if (!a && *b != '*') *--b = '*';
	if (!e) *t++ = '*';
	*t = 0;
	return(b);
}
