#ident	"@(#)ksh93:src/lib/libast/tm/tmfmt.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * time conversion support
 */

#include <ast.h>
#include <tm.h>

#define warped(t,n)	((t)<((n)-6L*30L*24L*60L*60L)||(t)>((n)+24L*60L*60L))

/*
 * format n with padding p into s
 * return end of s
 *
 * p:	<0	blank padding
 *	 0	no padding
 *	>0	0 padding
 */

static char*
number(register char* s, register char* e, register long n, register int p)
{
	if (p > 0) s += sfsprintf(s, 64, "%0*lu", p, n);
	else if (p < 0) s += sfsprintf(s, 64, "%*lu", -p, n);
	else s += sfsprintf(s, e - s, "%lu", n);
	return(s);
}

/*
 * format date given clock into buf of length len
 * end of buf is returned
 */

char*
tmfmt(char* buf, size_t len, const char* format, time_t* clock)
{
	register char*	cp;
	register char*	ep;
	register char*	p;
	register int	n;
	int		c;
	int		i;
	int		flags;
	Tm_t*		tp;
	Tm_zone_t*	zp;
	char*		stack[4];
	char**		sp;
	time_t		now;

	tp = tmmake(clock);
	if (!format || !*format) format = tm_info.deformat;
	flags = tm_info.flags;
	sp = &stack[0];
	cp = buf;
	ep = buf + len - 1;
	for (;;)
	{
		if (!(c = *format++))
		{
			if (sp <= &stack[0]) break;
			format = *--sp;
			continue;
		}
		if (c != '%')
		{
			if (cp < ep) *cp++ = c;
			continue;
		}
		switch (*format++)
		{
		case 0:
			format--;
			continue;
		case '%':
			if (cp < ep) *cp++ = '%';
			continue;
		case '?':
			if (tm_info.deformat != tm_info.format[TM_DEFAULT]) format = tm_info.deformat;
			else if (!*format) format = tm_info.format[TM_DEFAULT];
			continue;
		case 'a':	/* abbreviated day of week name */
			n = TM_DAY_3 + tp->tm_wday;
			goto index;
		case 'A':	/* day of week name */
			n = TM_DAY + tp->tm_wday;
			goto index;
		case 'b':	/* abbreviated month name */
		case 'h':
			n = TM_MONTH_3 + tp->tm_mon;
			goto index;
		case 'B':	/* month name */
			n = TM_MONTH + tp->tm_mon;
			goto index;
		case 'c':	/* `ctime(3)' date sans newline */
			p = "%a %b %e %T %Y";
			goto push;
		case 'C':	/* `date(1)' date */
			p = "%a %b %e %T %Z %Y";
			goto push;
		case 'd':	/* day of month */
			cp = number(cp, ep, (long)tp->tm_mday, 2);
			continue;
		case 'D':	/* date */
			p = "%m/%d/%y";
			goto push;
		case 'E':       /* no pad day of month */
			cp = number(cp, ep, (long)tp->tm_mday, 0);
			continue;
		case 'e':       /* blank padded day of month */
			cp = number(cp, ep, (long)tp->tm_mday, -2);
			continue;
		case 'H':	/* hour (0 - 23) */
			cp = number(cp, ep, (long)tp->tm_hour, 2);
			continue;
		case 'i':	/* international `date(1)' date */
			p = "%a %b %e %T %z %Z %Y";
			goto push;
		case 'I':	/* hour (0 - 12) */
			if ((n = tp->tm_hour) > 12) n -= 12;
			else if (n == 0) n = 12;
			cp = number(cp, ep, (long)n, 2);
			continue;
		case 'J':	/* Julian date (0 offset) */
			cp = number(cp, ep, (long)tp->tm_yday, 3);
			continue;
		case 'j':	/* Julian date (1 offset) */
			cp = number(cp, ep, (long)(tp->tm_yday + 1), 3);
			continue;
		case 'l':	/* `ls -l' date */
			p = "%b %e %H:%M";
			if (clock)
			{
				time(&now);
				if (warped(*clock, now)) p = "%b %e  %Y";
			}
			goto push;
		case 'm':	/* month number */
			cp = number(cp, ep, (long)(tp->tm_mon + 1), 2);
			continue;
		case 'M':	/* minutes */
			cp = number(cp, ep, (long)tp->tm_min, 2);
			continue;
		case 'n':
			if (cp < ep) *cp++ = '\n';
			continue;
		case 'o':	/* TM_DEFAULT override */
			p = tm_info.deformat;
			goto push;
		case 'O':	/* TM_DEFAULT */
			p = tm_info.format[TM_DEFAULT];
			goto push;
		case 'p':	/* meridian */
			n = TM_MERIDIAN + (tp->tm_hour >= 12);
			goto index;
		case 'r':
			p = "%I:%M:%S %p";
			goto push;
		case 'R':
			p = "%H:%M";
			goto push;
		case 'S':	/* seconds */
			cp = number(cp, ep, (long)tp->tm_sec, 2);
			continue;
		case 't':
			if (cp < ep) *cp++ = '\t';
			continue;
		case 'T':
			p = "%H:%M:%S";
			goto push;
		case 'U':	/* week number, Sunday as first day */
			i = tp->tm_yday - tp->tm_wday;
		week:
			if (i >= -3 )
			{
				n = (i + 1) / 7 + 1;
				if (i % 7 >= 4) n++;
			}
			else n = 52;
			cp = number(cp, ep, (long)n, 2);
			continue;
		case 'w':	/* weekday number */
			cp = number(cp, ep, (long)tp->tm_wday, 0);
			continue;
		case 'W':	/* week number, Monday as first day */
			if (tp->tm_wday == 0) i = tp->tm_yday - 6;
			else i = tp->tm_yday - tp->tm_wday + 1;
			goto week;
		case 'x':
			p = tm_info.format[TM_DATE];
			goto push;
		case 'X':
			p = tm_info.format[TM_TIME];
			goto push;
		case 'y':	/* year in the form yy */
			cp = number(cp, ep, (long)tp->tm_year, 2);
			continue;
		case 'Y':	/* year in the form ccyy */
			cp = number(cp, ep, (long)(1900 + tp->tm_year), 4);
			continue;
		case 'z':	/* time zone type */
			if (!(tm_info.flags & TM_UTC))
			{
				if ((zp = tm_info.zone) != tm_info.local)
					for (; zp >= tm_data.zone; zp--)
						if (p = zp->type) goto string;
				else if (p = zp->type) goto string;
			}
			continue;
		case 'Z':	/* time zone */
			p = (tm_info.flags & TM_UTC) ? tm_info.format[TM_UT] : tp->tm_isdst && tm_info.zone->daylight ? tm_info.zone->daylight : tm_info.zone->standard;
			goto string;
		case '-':
		case '+':
			switch (*format++)
			{
			case 0:
				format--;
				continue;
			case 'l':
				n = TM_LEAP;
				break;
			case 'u':
				n = TM_UTC;
				break;
			}
			if (c == '+')
			{
				if (!(tm_info.flags & n))
				{
					tm_info.flags |= n;
					tp = tmmake(clock);
				}
			}
			else if (tm_info.flags & n)
			{
				tm_info.flags &= ~n;
				tp = tmmake(clock);
			}
			continue;
		case '#':	/* seconds since the epoch */
			if (clock) now = *clock;
			else time(&now);
			cp = number(cp, ep, (long)now, 0);
			continue;
		default:
			if (cp < ep) *cp++ = c;
			if (cp < ep) *cp++ = *(format - 1);
			continue;
		}
	index:
		p = tm_info.format[n];
	string:
		while (*cp = *p++) if (cp < ep) cp++;
		continue;
	push:
		if (sp < &stack[elementsof(stack)])
		{
			*sp++ = (char*)format;
			format = p;
		}
		continue;
	}
	tm_info.flags = flags;
	*cp = 0;
	return(cp);
}
