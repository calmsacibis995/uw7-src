#ident	"@(#)ksh93:src/lib/libast/tm/tmdate.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * time conversion support
 *
 * relative times inspired by Steve Bellovin's netnews getdate(3)
 */

#include <ast.h>
#include <tm.h>
#include <ctype.h>

#define dig2(s,n)	((n)=((*(s)++)-'0')*10,(n)+=(*(s)++)-'0')
#define dig3(s,n)	((n)=((*(s)++)-'0')*100,(n)+=((*(s)++)-'0')*10,(n)+=(*(s)++)-'0')

#define BREAK		(1<<0)
#define DAY		(1<<1)
#define EXACT		(1<<2)
#define HOLD		(1<<3)
#define HOUR		(1<<4)
#define LAST		(1<<5)
#define MDAY		(1<<6)
#define MONTH		(1<<7)
#define NEXT		(1<<8)
#define THIS		(1<<9)
#define YEAR		(1<<10)
#define YYMMDDHHMMSS	(1<<11)
#define ZONE		(1<<12)

/*
 * parse date expression in s and return time_t value
 *
 * if non-null, e points to the first invalid sequence in s
 * clock provides default values
 */

time_t
tmdate(register const char* s, char** e, time_t* clock)
{
	register Tm_t*	tm;
	register int	n;
	int		state;
	time_t		now;
	char*		t;
	char*		last;
	char*		type;
	int		dst;
	int		zone;
	int		i;
	int		j;
	int		k;
	int		l;
	int		m;
	Tm_zone_t*	zp;

 reset:

	/*
	 * use clock for defaults
	 */

	tm = tmmake(clock);
	tm_info.date = tm_info.zone;
	dst = TM_DST;
	state = 0;
	type = 0;
	zone = 0;

	/*
	 * get <weekday year month day hour minutes seconds ?[ds]t [ap]m>
	 */

	for (;;)
	{
		state &= (state & HOLD) ? ~(HOLD) : ~(EXACT|LAST|NEXT|THIS);
		n = -1;
		while (isspace(*s) || *s == ',' || *s == '-') s++;
		if (!*(last = (char*)s)) break;
		if (*s == '#')
		{
			if (!isdigit(*++s)) break;
			now = strtol(s, &t, 0);
			clock = &now;
			s = t;
			goto reset;
		}
		if (isdigit(*s))
		{
			n = strtol(s, &t, 10);
			if (!(state & (LAST|NEXT|THIS)) && ((i = t - s) == 4 && *t == '.' || i > 4))
			{
				/*
				 * various date(1) formats
				 *
				 *	[yy[mm]]ddhhmm[.ss]
				 *	yyddd
				 *	hhmm.ss
				 */

				if (state & YYMMDDHHMMSS) break;
				state |= YYMMDDHHMMSS;
				if (i == 5 && !*t)
				{
					if (dig2(s, m) < 38) m += 100;
					dig3(s, k);
					l = 1;
					j = 0;
					i = 0;
					n = 0;
				}
				else
				{
					if (i < 10) m = tm->tm_year;
					else if (dig2(s, m) < 38) m += 100;
					if ((t - s) < 8) l = tm->tm_mon;
					else if (dig2(s, l) <= 0 || l > 12) break;
					if ((t - s) < 6) k = tm->tm_mon;
					else if (dig2(s, k) < 1 || k > 31) break;
					if ((t - s) < 4) break;
					if (dig2(s, j) > 24) break;
					if (dig2(s, i) > 59) break;
					if ((t - s) == 2) dig2(s, n);
					else if (t - s) break;
					else if (*t != '.') n = 0;
					else n = strtol(t + 1, &t, 10);
					if (n > (59 + TM_MAXLEAP)) break;
				}
				tm->tm_year = m;
				tm->tm_mon = l - 1;
				tm->tm_mday = k;
				tm->tm_hour = j;
				tm->tm_min = i;
				tm->tm_sec = n;
				s = t;
				continue;
			}
			s = t;
			if (*s == ':')
			{
				if ((state & HOUR) || n > 24 || *s++ != ':' || !isdigit(*s)) break;
				i = n;
				n = strtol(s, &t, 10);
				s = t;
				if (n > 59) break;
				j = n;
				if (*s == ':')
				{
					if (!isdigit(*++s)) break;
					n = strtol(s, &t, 10);
					s = t;
					if (n > (59 + TM_MAXLEAP)) break;
				}
				else n = 0;
				tm->tm_sec = n;
				tm->tm_min = j;
				tm->tm_hour = i;
				continue;
			}
		}
		while (isspace(*s) || *s == ',' || *s == '-') s++;
		if (isalpha(*s) && n < 1000)
		{
			if ((j = tmlex(s, &t, tm_info.format, TM_NFORM, tm_info.format + TM_SUFFIXES, TM_PARTS - TM_SUFFIXES)) >= 0)
			{
				s = t;
				switch (tm_data.lex[j])
				{
				case TM_EXACT:
					state |= HOLD|EXACT;
					continue;
				case TM_LAST:
					state |= HOLD|LAST;
					continue;
				case TM_THIS:
					state |= HOLD|THIS;
					n = 0;
					continue;
				case TM_NEXT:
					state |= HOLD|NEXT;
					continue;
				case TM_MERIDIAN:
					if (n > 0)
					{
						if (n > 24) goto done;
						tm->tm_hour = n;
					}
					if (j == TM_MERIDIAN)
					{
						if (tm->tm_hour == 12) tm->tm_hour = 0;
					}
					else if (tm->tm_hour < 12) tm->tm_hour += 12;
					if (n > 0) goto clear_min;
					continue;
				case TM_DAY_3:
					j += TM_DAY - TM_DAY_3;
					/*FALLTHROUGH*/
				case TM_DAY:
				case TM_PARTS:
				case TM_HOURS:
				case TM_DAYS:
					if (n == -1) n = 1;
					if (!(state & (LAST|NEXT|THIS))) for (;;)
					{
						while (isspace(*s) || *s == ',') s++;
						if ((k = tmlex(s, &t, tm_info.format + TM_LAST, TM_NOISE - TM_LAST, NiL, 0)) >= 0)
						{
							s = t;
							if (k <= 2) state |= LAST;
							else if (k <= 5) state |= THIS;
							else if (k <= 8) state |= NEXT;
							else state |= EXACT;
						}
						else break;
					}
					if (state & LAST) n = -n;
					else if (!(state & NEXT)) n--;
					switch (j)
					{
					case TM_DAYS+0:
						tm->tm_mday--;
						goto clear_hour;
					case TM_DAYS+1:
						goto clear_hour;
					case TM_DAYS+2:
						tm->tm_mday++;
						goto clear_hour;
					case TM_PARTS+0:
						tm->tm_sec += n;
						continue;
					case TM_PARTS+1:
						tm->tm_min += n;
						goto clear_sec;
					case TM_PARTS+2:
						tm->tm_hour += n;
						goto clear_min;
					case TM_PARTS+3:
						tm->tm_mday += n;
						goto clear_hour;
					case TM_PARTS+4:
						tm->tm_mday += 7 * n - tm->tm_wday + 1;
						goto clear_hour;
					case TM_PARTS+5:
						tm->tm_mon += n;
						goto clear_mday;
					case TM_PARTS+6:
						tm->tm_year += n;
						goto clear_mon;
					case TM_HOURS+0:
						tm->tm_mday += n;
						goto clear_hour;
					case TM_HOURS+1:
						tm->tm_mday += n;
						tm->tm_hour = 6;
						goto clear_min;
					case TM_HOURS+2:
						tm->tm_mday += n;
						tm->tm_hour = 12;
						goto clear_min;
					case TM_HOURS+3:
						tm->tm_mday += n;
						tm->tm_hour = 18;
						goto clear_min;
					default:
						j -= tm->tm_wday + TM_DAY;
						if (state & (LAST|NEXT|THIS))
						{
							if (j < 0) j += 7;
						}
						else if (j > 0) j -= 7;
						tm->tm_mday += j + n * 7;
						if (state & (LAST|NEXT|THIS)) goto clear_hour;
						continue;
					}
					break;
				case TM_MONTH_3:
					j += TM_MONTH - TM_MONTH_3;
					/*FALLTHROUGH*/
				case TM_MONTH:
					if (state & MONTH) goto done;
					state |= MONTH;
					i = tm->tm_mon;
					tm->tm_mon = j - TM_MONTH;
					if (n < 0)
					{
						while (isspace(*s) || *s == ',' || *s == '-') s++;
						if (isdigit(*s))
						{
							n = strtol(s, &t, 10);
							if (n <= 31) s = t;
							else n = -1;
						}
					}
					if (n >= 0)
					{
						if (n > 31) goto done;
						state |= MDAY;
						tm->tm_mday = n;
					}
					if (state & (LAST|NEXT|THIS))
					{
						n = i;
						goto rel_month;
					}
					continue;
				case TM_UT:
					if (state & ZONE) goto done;
					state |= ZONE;
					zone += tmgoff(s, &t, 0);
					s = t;
					continue;
				case TM_DT:
					if (!dst) goto done;
					if (!(state & ZONE))
					{
						dst = tm_info.zone->dst;
						zone = tm_info.zone->west;
					}
					zone += tmgoff(s, &t, dst);
					s = t;
					dst = 0;
					state |= ZONE;
					continue;
				case TM_NOISE:
					continue;
				}
			}
			if (!(state & ZONE) && (zp = tmzone(s, &t, type, &dst)))
			{
				s = t;
				zone += zp->west + dst;
				tm_info.date = zp;
				state |= ZONE;
				continue;
			}
			if (!type && (zp = tmtype(s, &t)))
			{
				s = t;
				type = zp->type;
				continue;
			}
			state |= BREAK;
		}
		else if (*s == '/')
		{
			if (!isdigit(*++s) || (state & MONTH) || n == 0 || n > 12) break;
			i = n - 1;
			n = strtol(s, &t, 10);
			s = t;
			if (n <= 0 || n > 31) break;
			if (*s == '/' && !isdigit(*(s + 1))) break;
			state |= MONTH|MDAY;
			tm->tm_mday = n;
			n = tm->tm_mon;
			tm->tm_mon = i;
			if (*s == '/')
			{
				n = strtol(s + 1, &t, 10);
				s = t;
			}
			else
			{
				if (state & (LAST|NEXT|THIS))
				{
				rel_month:
					if (state & LAST) tm->tm_year -= (tm->tm_mon < n) ? 0 : 1;
					else tm->tm_year += ((state & NEXT) ? 1 : 0) + ((tm->tm_mon < n) ? 1 : 0);
					if (state & MDAY) goto clear_hour;
					goto clear_mday;
				}
				continue;
			}
		}
		if (n < 0 || (state & YEAR)) break;
		state |= YEAR;
		if (n > 1900) n -= 1900;
		if (n < 38) n += 100;
		tm->tm_year = n;
		if (state & BREAK)
		{
			last = t;
			break;
		}
		continue;
	clear_mon:
		if (state & EXACT) continue;
		tm->tm_mon = 0;
	clear_mday:
		if (state & EXACT) continue;
		tm->tm_mday = 1;
	clear_hour:
		if (state & EXACT) continue;
		tm->tm_hour = 0;
	clear_min:
		if (state & EXACT) continue;
		tm->tm_min = 0;
	clear_sec:
		if (state & EXACT) continue;
		tm->tm_sec = 0;
	}
 done:
	if (e) *e = last;
	return(tmtime(tmfix(tm), (state & ZONE) ? zone : TM_LOCALZONE));
}
