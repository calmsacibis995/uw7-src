#ident	"@(#)ksh93:src/lib/libast/tm/tminit.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * time conversion support
 */

#include <ast.h>
#include <tm.h>
#include <ctype.h>
#include <namval.h>

#ifndef tzname
#if _DLL_INDIRECT_DATA && _DLL
#define	tzname		(_ast_getdll()->_ast_tzname)
#else
#ifdef	tzname
#define _dat_tzname	1
#else
#if !_dat_tzname
#if _dat__tzname
#undef	_dat_tzname
#define _dat_tzname	1
#define tzname		_tzname
#endif
#endif
#endif
#if _dat_tzname
extern char*		tzname[];
#endif
#endif
#endif

#define TM_type		(-1)

static const Namval_t		options[] =
{
	"adjust",	TM_ADJUST,
	"format",	TM_DEFAULT,
	"leap",		TM_LEAP,
	"type",		TM_type,
	"utc",		TM_UTC,
	0,		0
};

Tm_info_t		tm_info;

/*
 * return minutes west of GMT for local time clock
 *
 * isdst will point to non-zero if DST is in effect
 * this routine also kicks in the local initialization
 */

static int
tzwest(time_t* clock, int* isdst)
{
	register Tm_t*	tp;
	register int	n;
	register int	m;
	int		h;

	/*
	 * convert to GMT assuming local time
	 */

	tp = (Tm_t*)gmtime(clock);
	n = tp->tm_yday;
	h = tp->tm_hour;
	m = tp->tm_min;

	/*
	 * localtime() handles DST and GMT offset
	 */

	tp = (Tm_t*)localtime(clock);
	if (n = tp->tm_yday - n)
	{
		if (n > 1) n = -1;
		else if (n < -1) n = 1;
	}
	*isdst = tp->tm_isdst;
	return((h - tp->tm_hour - n * 24) * 60 + m - tp->tm_min);
}

/*
 * stropt() option handler
 */

static int
tmopt(void* a, const void* p, int n, const char* v)
{
	Tm_zone_t*	zp;

	NoP(a);
	if (p) switch (((Namval_t*)p)->value)
	{
	case TM_DEFAULT:
		tm_info.deformat = (n && (n = strlen(v)) > 0 && (n < 2 || v[n-2] != '%' || v[n-1] != '?')) ? strdup(v) : tm_info.format[TM_DEFAULT];
		break;
	case TM_type:
		tm_info.local->type = (n && *v) ? ((zp = tmtype(v, NiL)) ? zp->type : strdup(v)) : 0;
		break;
	default:
		if (n) tm_info.flags |= ((Namval_t*)p)->value;
		else tm_info.flags &= ~((Namval_t*)p)->value;
		break;
	}
	return(0);
}

/*
 * initialize the local timezone
 */

static void
tmlocal(void)
{
	register Tm_zone_t*	zp;
	register int		n;
	register char*		s;
	int			i;
	int			m;
	int			isdst;
	char*			t;
	Tm_t*			tp;
	time_t			now;
	char			buf[20];

	static Tm_zone_t	local;

	/*
	 * tm_info.format language support goes here
	 */

#if _lib_tzset
	tzset();
#endif
#if _dat_tzname
	local.standard = tzname[0];
	local.daylight = tzname[1];
#endif
	tm_info.format = tm_data.format;
	tm_info.deformat = tm_data.format[TM_DEFAULT];

	/*
	 * tm_info.local
	 */

	tm_info.zone = tm_info.local = &local;
	time(&now);
	n = tzwest(&now, &isdst);

	/*
	 * compute local DST offset by roaming
	 * through the last 12 months until tzwest() changes
	 */

	for (i = 0; i < 12; i++)
	{
		now -= 31 * 24 * 60 * 60;
		if ((m = tzwest(&now, &isdst)) != n)
		{
			if (!isdst)
			{
				isdst = n;
				n = m;
				m = isdst;
			}
			m -= n;
			break;
		}
	}
	local.west = n;
	local.dst = m;

	/*
	 * now get the time zone names
	 */

#if _dat_tzname
	if (tzname[0])
	{
		/*
		 * POSIX
		 */

		if (!local.standard) local.standard = tzname[0];
		if (!local.daylight) local.daylight = tzname[1];
	}
	else
#endif
	if ((s = getenv("TZNAME")) && *s && (s = strdup(s)))
	{
		/*
		 * BSD
		 */

		local.standard = s;
		if (s = strchr(s, ',')) *s++ = 0;
		else s = "";
		local.daylight = s;
	}
	else if ((s = getenv("TZ")) && *s && *s != ':' && (s = strdup(s)))
	{
		/*
		 * POSIX style but skipped by localtime()
		 */

		local.standard = s;
		if (*++s && *++s && *++s)
		{
			*s++ = 0;
			tmgoff(s, &t, 0);
			s = t;
		}
		else s = "";
		local.daylight = s;
	}
	else
	{
		/*
		 * tm_data.zone table lookup
		 */

		t = 0;
		for (zp = tm_data.zone; zp->standard; zp++)
		{
			if (zp->type) t = zp->type;
			if (zp->west == n && zp->dst == m)
			{
				local.type = t;
				local.standard = zp->standard;
				if (!(s = zp->daylight))
				{
					s = tmpoff(buf, zp->standard, 0, 0);
					*s++ = ' ';
					tmpoff(s, tm_info.format[TM_DT], m, TM_DST);
					s = strdup(buf);
				}
				local.daylight = s;
				break;
			}
		}
		if (!zp->standard)
		{
			/*
			 * not in the table
			 */

			s = tmpoff(buf, tm_info.format[TM_UT], n, 0);
			local.standard = strdup(buf);
			*s++ = ' ';
			tmpoff(s, tm_info.format[TM_UT], m, TM_DST);
			local.daylight = strdup(buf);
		}
	}

	/*
	 * set the options
	 */

	stropt(getenv("TM_OPTIONS"), options, sizeof(*options), tmopt, NiL);

	/*
	 * the time zone type is probably related to the locale
	 */

	if (!local.type)
	{
		s = local.standard;
		t = 0;
		for (zp = tm_data.zone; zp->standard; zp++)
		{
			if (zp->type) t = zp->type;
			if (tmword(s, NiL, zp->standard, NiL, 0))
			{
				local.type = t;
				break;
			}
		}
	}

	/*
	 * tm_info.flags
	 */

	if (!(tm_info.flags & TM_ADJUST))
	{
		now = (time_t)78811200;		/* Jun 30 1972 23:59:60 */
		tp = (Tm_t*)localtime(&now);
		if (tp->tm_sec != 60) tm_info.flags |= TM_ADJUST;
	}
	if (!(tm_info.flags & TM_UTC))
	{
		s = local.standard;
		for (zp = tm_data.zone; !zp->type && zp->standard; zp++)
			if (tmword(s, NiL, zp->standard, NiL, 0))
			{
				tm_info.flags |= TM_UTC;
				break;
			}
	}
}

/*
 * initialize tm data
 */

void
tminit(register Tm_zone_t* zp)
{
	if (!tm_info.local) tmlocal();
	if (!zp) zp = tm_info.local;
#if HUH950804 /* it only worked on systems that ignored TZ=...! */
	if (zp != tm_info.zone)
	{
		register char*	s;
		time_t		clock;
		char		buf[128];

		tm_info.zone = zp;
		s = buf;
		s += sfsprintf(s, sizeof(buf), "TZ=%s%d", zp->standard, zp->west / 60);
		if (zp->daylight)
		{
			s += sfsprintf(s, sizeof(buf) - (s - buf), "%s", zp->daylight);
			if (zp->dst != TM_DST) sfsprintf(s, sizeof(buf) - (s - buf), "%d", zp->dst);
		}
		s = *environ;
		*environ = buf;
		time(&clock);
#if _lib_tzset
		tzset();
#endif
		localtime(&clock);
		*environ = s;
	}
#else
	tm_info.zone = zp;
#endif
}
