#ident	"@(#)ksh93:src/lib/libast/string/chresc.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * return the next character in the string s
 * \ character constants are converted
 * p is updated to point to the next character in s
 */

#include <ast.h>

int
chresc(register const char* s, char** p)
{
	register const char*	q;
	register int		c;

	switch (c = *s++)
	{
	case 0:
		s--;
		break;
	case '\\':
		switch (c = *s++)
		{
		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
			c -= '0';
			q = s + 2;
			while (s < q) switch (*s)
			{
			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7':
				c = (c << 3) + *s++ - '0';
				break;
			default:
				q = s;
				break;
			}
			break;
		case 'a':
			c = '\007';
			break;
		case 'b':
			c = '\b';
			break;
		case 'f':
			c = '\f';
			break;
		case 'n':
			c = '\n';
			break;
		case 'r':
			c = '\r';
			break;
		case 's':
			c = ' ';
			break;
		case 't':
			c = '\t';
			break;
		case 'v':
			c = '\013';
			break;
		case 'x':
			c = 0;
			q = s;
			while (q) switch (*s)
			{
			case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
				c = (c << 4) + *s++ - 'a' + 10;
				break;
			case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
				c = (c << 4) + *s++ - 'A' + 10;
				break;
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				c = (c << 4) + *s++ - '0';
				break;
			default:
				q = 0;
				break;
			}
			break;
		case 'E':
			c = '\033';
			break;
		case 0:
			s--;
			break;
		}
		break;
	}
	if (p) *p = (char*)s;
	return(c);
}
