#ident	"@(#)ksh93:src/lib/libast/re/ressub.c	1.1"
#pragma prototyped
/*
 * AT&T Bell Laboratories
 *
 * regular expression match substitution to sfio stream
 */

#include "relib.h"

#undef	next

#include <ctype.h>

/*
 * do a single substitution
 */

static void
sub(register Sfio_t* dp, register const char* sp, register Match_t* mp, register int flags)
{
	register int	c;
	char*		s;
	char*		e;

	flags &= (RE_LOWER|RE_UPPER);
	for (;;) switch (c = *sp++)
	{
	case 0:
		return;
	case '\\':
		switch (c = *sp++)
		{
		case 0:
			sp--;
			break;
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			c -= '0';
			goto group;
		default:
			sfputc(dp, chresc(sp - 2, &s));
			sp = s;
			break;
		}
		break;
	case '&':
		c = 0;
	group:
		if (s = mp[c].sp)
		{
			e = mp[c].ep;
			while (s < e)
			{
				c = *s++;
				switch (flags)
				{
				case RE_UPPER:
					if (islower(c)) c = toupper(c);
					break;
				case RE_LOWER:
					if (isupper(c)) c = tolower(c);
					break;
				}
				sfputc(dp, c);
			}
		}
		break;
	default:
		switch (flags)
		{
		case RE_UPPER:
			if (islower(c)) c = toupper(c);
			break;
		case RE_LOWER:
			if (isupper(c)) c = tolower(c);
			break;
		}
		sfputc(dp, c);
		break;
	}
}

/*
 * ed(1) style substitute using matches from last reexec()
 */

void
ressub(Re_program_t* re, Sfio_t* dp, register const char* op, const char* sp, int flags)
{
	register Match_t*	mp;

	mp = re->subexp.m;
	do
	{
		sfwrite(dp, op, mp->sp - op);
		sub(dp, sp, mp, flags);
		op = mp->ep;
	} while ((flags & RE_ALL) && *op && mp->sp != mp->ep && reexec(re, op));
	sfputr(dp, op, -1);
}
