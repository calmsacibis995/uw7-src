#ident	"@(#)ksh93:src/lib/libast/re/resub.c	1.1"
#pragma prototyped
/*
 * AT&T Bell Laboratories
 *
 * regular expression match substitution
 */

#include "relib.h"

#include <ctype.h>

/*
 * do a single substitution
 */

static char*
sub(register const char* sp, register char* dp, register Match_t* mp, int flags)
{
	register int	i;
	char*		s;

	NoP(flags);
	for (;;) switch (*dp = *sp++)
	{
	case 0:
		return(dp);
	case '\\':
		switch (i = *sp++)
		{
		case 0:
			sp--;
			break;
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			i -= '0';
			if (mp[i].sp)
			{
				s = mp[i].sp;
				while (s < mp[i].ep) *dp++ = *s++;
			}
			break;
		default:
			*dp++ = chresc(sp - 2, &s);
			sp = s;
			break;
		}
		break;
	case '&':
		if (mp[0].sp)
		{
			s = mp[0].sp;
			while (s < mp[0].ep) *dp++ = *s++;
		}
		break;
	default:
		dp++;
		break;
	}
}

/*
 * ed(1) style substitute using matches from last reexec()
 */

char*
resub(Re_program_t* re, register const char* op, const char* sp, register char* dp, int flags)
{
	register Match_t*	mp;

	mp = re->subexp.m;
	do
	{
		while (op < mp->sp) *dp++ = *op++;
		op = dp;
		dp = sub(sp, dp, mp, flags);
		if (flags & (RE_LOWER|RE_UPPER))
		{
			while (op < dp)
			{
				if (flags & RE_LOWER)
				{
					if (isupper(*op)) *(char*)op = tolower(*op);
				}
				else if (islower(*op)) *(char*)op = toupper(*op);
				op++;
			}
		}
		op = mp->ep;
	} while ((flags & RE_ALL) && *op && mp->sp != mp->ep && reexec(re, op));
	while (*dp++ = *op++);
	return(--dp);
}
