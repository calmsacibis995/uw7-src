#ident	"@(#)ksh93:src/lib/libast/string/strton.c	1.1"
#pragma prototyped
/*
 * AT&T Bell Laboratories
 *
 * convert string to long integer
 * if non-null e will point to first unrecognized char in s
 * if b!=0 it points to the default base on input and
 * will point to the explicit base on return
 * a default base of 0 will determine the base from the input
 * a default base of 1 will determine the base from the input excluding 0x?
 * a base prefix in the string overrides *b
 * *b will not be set if the string has no base prefix
 * if m>1 and no multipler was specified then the result is multiplied by m
 * if m<0 then multipliers are not consumed
 *
 * integer numbers are of the form:
 *
 *	[sign][base][number][multiplier]
 *
 *	base:		nnn#		base nnn (no multiplier)
 *			0[xX]		hex
 *			0		octal
 *			[1-9]		decimal
 *
 *	number:		[0-9a-zA-Z]*
 *
 *	multiplier:	.		pseudo-float (100) + subsequent digits
 *			[bB]		block (512)
 *			[cC]		char (1)
 *			[gG]		giga (1024*1024*1024)
 *			[kK]		kilo (1024)
 *			[mM]		mega (1024*1024)
 */

#include <ast.h>
#include <ctype.h>

long
strton(register const char* s, char** e, char* b, int m)
{
	register long		n;
	register int		c;
	register int		r;
	register const char*	p;
	int			z;

	if (!b || (r = *b) < 0 || r > 64) r = 0;
	while (isspace(*s)) s++;
	if ((z = (*s == '-')) || *s == '+') s++;
	p = s;
	if (r <= 1)
	{
		if ((c = *p++) >= '0' && c <= '9')
		{
			n = c - '0';
			if ((c = *p) >= '0' && c <= '9')
			{
				n = n * 10 + c - '0';
				p++;
			}
			if (*p == '#')
			{
				if (n >= 2 && n <= 64)
				{
					s = p + 1;
					r = n;
				}
			}
			else if (r) r = 0;
			else if (*s == '0')
			{
				if ((c = *(s + 1)) == 'x' || c == 'X')
				{
					s += 2;
					r = 16;
				}
				else if (c >= '0' && c <= '7') r = 8;
			}
		}
		if (b && r) *b = r;
	}
	n = 0;
	if (r > 36) for (;;)
	{
		if ((c = *s++) >= '0' && c <= '9') c -= '0';
		else if (c >= 'a' && c <= 'z') c -= 'a' - 10;
		else if (c >= 'A' && c <= 'Z') c -= 'A' - 36;
		else if (c == '_') c = 62;
		else if (c == '@') c = 63;
		else
		{
			s--;
			break;
		}
		n = n * r + c;
	}
	else
	{
		if (r) m = -1;
		else r = 10;
		if (r > 10) for (;;)
		{
			if ((c = *s++) >= '0' && c <= '9') c -= '0';
			else if (c >= 'a' && c <= 'z') c -= 'a' - 10;
			else if (c >= 'A' && c <= 'Z') c -= 'A' - 10;
			else break;
			n = n * r + c;
		}
		else for (;;)
		{
			if ((c = *s++) >= '0' && c <= '9') c -= '0';
			else break;
			n = n * r + c;
		}
		if (m >= 0) switch (c)
		{
		case 'b':
		case 'B':
			n *= 512;
			break;
		case 'c':
		case 'C':
			break;
		case 'g':
		case 'G':
			n *= 1024 * 1024 * 1024;
			break;
		case 'k':
		case 'K':
			n *= 1024;
			break;
		case 'm':
		case 'M':
			n *= 1024 * 1024;
			break;
		case '.':
			n *= 100;
			for (m = 10; *s >= '0' && *s <= '9'; m /= 10) 
				n += m * (*s++ - '0');
			break;
		default:
			s--;
			if (m > 1) n *= m;
			break;
		}
		else s--;
	}
	if (e) *e = (char*)s;
	return(z ? -n : n);
}
