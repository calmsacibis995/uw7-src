/*		copyright	"%c%" 	*/
#ident	"@(#)date:i386/cmd/date/date.c	1.24.5.1"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/***************************************************************************
 * Command: date
 * Inheritable Privileges: P_SYSOPS,P_MACWRITE,P_DACWRITE,P_SETFLEVEL
 *       Fixed Privileges: None
 * Inheritable Authorizations: None
 *       Fixed Authorizations: None
 * Notes:
 *	Set and/or print the date.
 ***************************************************************************/

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

/*
 * date - with format capabilities and international flair.
 */

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <locale.h>
#include <utmp.h>
#include <utmpx.h>
#include <priv.h>
#include <pfmt.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/uadmin.h>

#define	SYSTZ_FILE	"/etc/TIMEZONE"

/* number of days in year, OK until the year 2100 which is NOT a leap year */
#define	year_size(y)	(((y) % 4) ? 365 : 366)
#define	real_year_size(y) ( (((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0) ) ? 366 : 365)

/* number of days in month for non leap-year */
static short month_size[12] = {
			31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
		};

/* entries to be written in utmp and wtmp files */
static struct utmp wtmp[2] = {
			{"", "", OTIME_MSG, 0, OLD_TIME, {0, 0}, 0}, 
			{"", "", NTIME_MSG, 0, NEW_TIME, {0, 0}, 0}
		};

/* error messages */
static char	incorrect_usage[] = ":8:Incorrect usage\n";
static char	bad_conversion[]  = ":144:Bad conversion\n";
static char	uflag = 0;
static char	*SYSTZ = NULL;	/* System timezone, taken from /etc/TIMEZONE */

static time_t	setdate();
static void	adjustdate();
static void	printdate();
static void	usage();

main(argc, argv)
	int	argc;
	char	**argv;
{
	time_t	now;		/* the current date */
	char	*fmt = NULL;	/* format for printing the date */
	FILE	*fp;
	char	buf[BUFSIZ];
	int	c;
	int	aflag=0;
	char	*aarg;
	
	/*
	 * Clear working privileges.
	 */
	(void) procprivc(CLRPRV, PRIVS_W, (priv_t)0);

	/*
	 * Set locale and error message info.
	 */
	if (setlocale(LC_ALL, "") == NULL) {
		(void) setlocale(LC_TIME, "");
		(void) setlocale(LC_CTYPE, "");
	}
	(void) setcat("uxcore.abi");
	(void) setlabel("UX:date");

	/*
	 * Interpret flag arguments.
	 */
	while ((c = getopt(argc,argv,"a:u")) != -1)
		switch(c) {
			case 'a':
				/* date -a [-]seconds[.fraction] */
				aflag++;
				aarg = optarg;
				break;
			case 'u':
				/* date -u [ +format ]  OR  date -u datespec */
				uflag++;
				break;
			default: 
				usage(incorrect_usage);
				break;
		}

	if (aflag) {
		if (uflag)
			usage(incorrect_usage);
		adjustdate(aarg);
		exit(0);
	} else if (uflag) {
		(void) putenv("TZ=GMT");
		/*
		 * Get the system's timezone to be used to adjust
		 * the CMOS clock in set date.
		 */
		if ((fp = fopen(SYSTZ_FILE, "r")) != NULL) {
			while (fgets(buf, sizeof(buf), fp) != NULL) {
				if (*buf == '#') {
					continue;
				}
				if (*buf != '\0' &&
				    strncmp(buf, "TZ=", 3) == 0) {
					SYSTZ = strrchr(buf, '\n');
					if (SYSTZ) {
						*SYSTZ = '\0';
						}
						SYSTZ = buf;
						break;
				}
			}
			(void)fclose(fp);
		}
	}

	/*
	 * Get the current date/time.
	 */
	(void) time(&now);

	/*
	 * Set and/or print the date as requested.
	 */
	switch (argc - optind) {
	case 1:
		if (*argv[optind] == '+') 
			fmt = &argv[optind][1];
		else
			now = setdate(localtime(&now), argv[optind]);
		/* FALLTHROUGH */

	case 0:
		printdate(localtime(&now), fmt);
		exit(0);

	default:
		usage(incorrect_usage);
	}

	/* NOTREACHED */
}

/*
 * Set the date from the given command-line argument,
 * which may take one of several forms.
 * If successful, record the "before" and "after" dates
 * in the utmp and wtmp files.  Return the new date.
 */

static time_t
setdate(current_date, date)
	struct tm *current_date;	/* to supply default year/month/day */
	char *date;			/* mmddHHMMccyy or subset thereof */
{
	register char *minp = &date[6];	/* pointer to minutes in date arg */
	register int i;			/* scratch variable */
	int mm, dd = 0, hh, min, yy;	/* month, day, hour, minute, year */
	int err;			/* saved value of errno from syscall */
	time_t clock_val;		/* the new date */

	time_t correction;

	/*
	 * Parse the date string.
	 * First make sure it is entirely numeric.
	 */
	i = strlen(date);

	if (strspn(date, "0123456789") != i)
		usage(bad_conversion);

	switch (i) {
	case 12:
		/* mmddHHMMccyy */
		yy = atoi(&date[8]);
		if( yy>= 2038 )
		{
			if( sizeof(time_t) == 4 )/*not yet 64bit with long long*/
				yy = 1970;
		}
		date[8] = '\0';
		break;
	case 10:
		/* mmddHHMMyy */
		/* yy = 1900 + atoi(&date[8]); */
		yy = atoi(&date[8]);
		if( yy >= 70 )
			yy += 1900;
		else if( yy < 38 )
			yy += 2000;
		else /* 38 <= yy <= 69 */
		{
			if( sizeof(time_t) > 4 )/*hopefully 64bit with long long*/
				yy += 2000;
			else
				yy = 1970;
		}
		date[8] = '\0';
		break;

	case 8:
		/* mmddHHMM */
		yy = 1900 + current_date->tm_year;
		break;
	case 4: 
		/* HHMM */
		yy = 1900 + current_date->tm_year;
		mm = current_date->tm_mon + 1; 	/* tm_mon goes from 0 to 11 */
		dd = current_date->tm_mday;
		minp = &date[2];
		break;	
	default:
		usage(bad_conversion);
	}

	/* extract the hour and minute */
	min = atoi(minp);
	minp[0] = '\0';
	hh = atoi(&minp[-2]);
	minp[-2] = '\0';

	/* if user supplied month and day, extract them too */
	if (dd == 0) {
		dd = atoi(&date[2]);
		date[2] = '\0';
		mm = atoi(&date[0]);
	}

	/* hour should be from 0 to 23; if 24, wrap to next day */
	if (hh == 24) {
		hh = 0;
		dd++;
	}

	/* validate ranges of date elements */
	if (mm<1 || mm>12 || dd<1 || dd>31 || hh<0 || hh>23 || min<0 || min>59)
		usage(bad_conversion);

	/* build date and time number */
	for (clock_val = 0, i = 1970; i < yy; i++)
		clock_val += real_year_size(i);

	/* adjust for leap year */
	if (real_year_size(yy) == 366 && mm >= 3)
		clock_val += 1;

	/* adjust for current month */
	while (--mm)
		clock_val += (time_t)month_size[mm - 1];

	/* load up the rest */
	clock_val += (time_t)(dd - 1);
	clock_val *= 24;
	clock_val += (time_t)hh;
	clock_val *= 60;
	clock_val += (time_t)min;
	clock_val *= 60;

	/*
	 * Determine the time adjustment required if date was invoked
	 * with the -u option.  The -u option specifies date in GMT.
	 */
	if (uflag && SYSTZ != NULL) {
		(void)putenv(SYSTZ);
		if (localtime(&clock_val)->tm_isdst) {
			correction = altzone;
		} else {
			correction = timezone;
		}
		(void)putenv("TZ=GMT");
	} else {
		/* convert to GMT using timezone info from localtime(3C) */
		clock_val += (time_t)timezone;
		correction = timezone;

		/* correct if daylight savings time in effect */
		if (localtime(&clock_val)->tm_isdst) {
			clock_val -= (time_t)(timezone - altzone); 
			correction = altzone;
		}
	}

	(void) time(&wtmp[0].ut_time);	/* "before" date for wtmp entry */

	/*
	 * uadmin(A_CLOCK, ...) and stime() both require SYSOPS privilege.
	 * CAUTION: do not insert here any calls
	 * that do not require SYSOPS privilege.
	 */

	(void) procprivc(SETPRV, SYSOPS_W, (priv_t)0);
		uadmin(A_CLOCK, correction, 0);
		i = stime(&clock_val); err = errno;
	(void) procprivc(CLRPRV, PRIVS_W, (priv_t)0);

	if (i < 0) {
		(void) pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n", strerror(err));
		exit(1);
	}

	(void) time(&wtmp[1].ut_time);	/* "after" date for wtmp entry */

	/*
	 * Write "before" and "after" entries to the utmp and wtmp files.
	 * Both files are at level SYS_PUBLIC and not generally writeable,
	 * hence MACWRITE and DACWRITE privileges may be required.
	 * If pututline() or updwtmp() has to create one of the files,
	 * SETFLEVEL privilege will be required as well.
	 *
	 * CAUTION: do not insert here any calls that do not
	 * require MACWRITE, DACWRITE and SETFLEVEL privileges.
	 */
	
	(void) procprivc(SETPRV, MACWRITE_W, DACWRITE_W, SETFLEVEL_W, (priv_t)0);
		(void) pututline(&wtmp[0]);
		(void) pututline(&wtmp[1]);
		updwtmp(WTMP_FILE, &wtmp[0]);
		updwtmp(WTMP_FILE, &wtmp[1]);
	(void) procprivc(CLRPRV, PRIVS_W, (priv_t)0);

	return clock_val;
}

/*
 * Adjust the date gradually (date -a [-]seconds[.fraction]).
 */
static void
adjustdate(adj)
        register char *adj;
{
	struct timeval tv = { 0, 0 };
        register int mult;
        int sign;			/* sign of adjustment, +1 or -1 */
	int ret;			/* return value from system call */
	int err;			/* saved errno from system call */

	/*
	 * Leading `-' for negative adjustment.
	 */
        if (*adj == '-') {
                sign = -1;
                adj++;
        } else
                sign = 1;

	if (*adj == '\0')
		usage(incorrect_usage);

	/*
	 * Compute number of full seconds.
	 */
        while (*adj >= '0' && *adj <= '9') {
                tv.tv_sec *= 10;
                tv.tv_sec += *adj++ - '0';
        }

	/*
	 * Compute additional number of microseconds
	 * if a fractional adjustment is present.
	 */
        if (*adj == '.') {
                adj++;
                mult = 100000;
                while (*adj >= '0' && *adj <= '9') {
                        tv.tv_usec += (*adj++ - '0') * mult;
                        mult /= 10;
                }
        }

        if (*adj)
        	usage(incorrect_usage);

        tv.tv_sec *= sign;
        tv.tv_usec *= sign;

	(void) procprivc(SETPRV, SYSOPS_W, (priv_t)0);
	ret = adjtime(&tv, 0); err = errno;
	(void) procprivc(CLRPRV, PRIVS_W, (priv_t)0);

	if (ret < 0) {
		pfmt(stderr, MM_ERROR, ":142:Cannot adjust date: %s\n",
			strerror(err));
		exit(1);
	}
}

/*
 * Print the date according to the supplied format.
 * If the format is NULL or an empty string, use
 * an appropriate default.
 */

static void
printdate(tp, format)
	struct tm *tp;
	char *format;
{
	struct tm tmp;			/* to save caller's tm structure */
	char buf[1024];			/* arbitrary size */

	if (format == NULL || *format == '\0') 
		if ((format = getenv("CFTIME")) == NULL || *format == 0)
#ifdef OLDDATE
			format = "%C";
#else
			format = "%N";
#endif

	/*
	 * strftime uses the same static tm buffer as ctime and localtime,
	 * so we preserve the caller's buffer by copying it to a local var.
	 */
	tmp = *tp;			/* structure assignment */

	(void) strftime(buf, sizeof buf, format, &tmp);
	buf[(sizeof buf) - 1] = '\0';	/* ensure null termination */
	(void) puts(buf);
}

/*
 * Print the supplied error, followed by a usage message.
 * Exit with an error status.
 */

static void
usage(complaint)
char *complaint;
{
	pfmt(stderr, MM_ERROR, complaint);
	pfmt(stderr, MM_ACTION,
		":1119:Usage:\n\tdate [-u] [+format]\n\tdate [-u] [[mmdd]HHMM | mmddHHMM[[cc]yy]] \n\tdate [-a [-]sss.fff]\n");
	exit(1);
}
