#ident	"@(#)OSRcmds:date/date.c	1.1"
#pragma comment(exestr, "@(#) date.c 26.2 95/07/11 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1985-1995.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 *
 */

/*
**	date - with format capabilities and international flair
*/


/*
 * Modification History
 *
 * M001		ericg	Folded in Microsoft changes and added some more
 *			calls to usage for more robust error diagnostics
 *			per bug report 525
 *
 *	15 Dec 1986	sco!blf	S002
 *		- Declared dmsize[] and dysize() external (in libc:ctime.c).
 *		- Don't assume summer time is 60 minutes ahead
 *		  of standard time.
 *		- Display the full timezone name, not just the
 *		  first three characters.
 *	22 Jan 1988	sco!ncm	S003
 *		- Fix bug preventing the date from being set to 2000.
 *		  Fix courtesy of sco!brianm.
 *	L004	13 Apr 1988	scol!bijan
 *	- Xenix Internationalisation Phase II.
 *	- Replacing asctime by strftime.
 *	S005	12 May 88	sco!craig
 *	- INTL - use strftime to process date "+..." formats
 *	S006	22 Mar 89	sco!hoeshuen
 *	- uncomment declaration of dmsize as it is defined as "static" 
 *	  in libc:ctime.c
 *	S007	sco!asharpe	Wed May 16 17:06:41 1990
 *	- The real time clock was not being synchronized because
 *	  uadmin(A_CLOCK, timezone) was not being called. This
 *	  routine is called to set the system's idea of local time,
 *	  needed for the wtodc() routine called from the stime()
 *	  system call.
 *	L008	scol!markhe	13 July 92
 *	- changes for XPG4/POSIX. Added u option and message catalogues.
 *	  Changed default date format (the final output looks the same).
 *	L009	scol!hughd	21sep92
 *	- allow date seconds to be set in "YYMMDDhhmmss" format:
 *	  if we didn't insist on the year, we couldn't distinguish
 *	  from "MMDDhhmmYY", which is required for back-compatibility
 *	- gtime() now allows [YYMMDD]hhmmss or MMDDhhmm[YY], to
 *	  avoid asktime needing /lib/cvtdate; usage message updated
 *	- rewrote gpair() and gtime(), to use new libos tmtime():
 *	  unlike earlier code, this correctly inverts localtime()
 *	- work done for Siemens, 3.2v2 and 3.2v4; and for 3.2v5
 *	L010	scol!hughd	23sep92
 *	- I've now found gtime() in libcmd.a, so update that one and use it
 *	- fixed L008 setting of corrected_tz if uflag, and braces if tfailed:
 *	  it was writing utmp & wtmp even when setting the time had failed
 *	- if we've successfully set the time, then we don't need
 *	  to call time() and localtime() again to find out what it is
 *	- if we've failed to set the time, then note that the time shown
 *	  is the time we've failed to set, not the current time - this was
 *	  probably a coding error (tfailed test wrong way round), but it's
 *	  been like that since XENIX, and could conceivably be of use (it
 *	  makes my testing easier!): so leave it unless there's a bugreport
 *	L011	scol!anthonys	19 Nov 92
 *	- Reworked L009. POSIX.2/XPG4 defines a syntax for specifying
 *	  seconds as a part of the touch command. For consistency, modified
 *	  date to use this same syntax - note most of the changes are in
 *	  the library function gtime().
 *	- Added the new option "-t", which enables the new syntax that
 *	  supports seconds.
 *	L012	scol!ianw	29 Jun 95
 *	- When in the POSIX or C locale's set the default format to that
 *	  specified in POSIX.2/XPG4, otherwise set the default format to "%c"
 *	  to ensure the date/time is displayed is a way appropriate to the
 *	  current locale.
 *	- Removed the optopt, optind, opterr and optarg extern declarations,
 *	  they are declared in unistd.h.
 *	L013	scol!ianw	05 Jul 95
 *	- Changed the second argument of catopen() to MC_FLAGS.
 */

#include	<sys/types.h>
#include	"utmp.h"
#include        <stdio.h>
#include	<time.h>
#include	<string.h>					/* L012 */
#include	<sys/uadmin.h>		/* S007	*/
#include	<ctype.h>				/* L008 begin */
#include	<unistd.h>
#include	<sys/uadmin.h>
/* #include	<errormsg.h> */
#ifdef	INTL
#  include	<locale.h>
#  include	<nl_types.h>
#  include	"date_msg.h"
#else
#  define	MSGSTR(num, str)	(str)
#  define	catgets(a)
#endif	/* INTL */					/* L008 end */

#define	MONTH	itoa(tim->tm_mon+1,cp,2)
#define	DAY	itoa(tim->tm_mday,cp,2)
#define	YEAR	itoa(tim->tm_year,cp,2)
#define	HOUR	itoa(tim->tm_hour,cp,2)
#define	MINUTE	itoa(tim->tm_min,cp,2)
#define	SECOND	itoa(tim->tm_sec,cp,2)
#define	JULIAN	itoa(tim->tm_yday+1,cp,3)
#define	WEEKDAY	itoa(tim->tm_wday,cp,1)
#define	MODHOUR	itoa(h,cp,2)

extern int gtime(char *, int, time_t *, int);		/* L010: libcmd.a */
time_t	timbuf;							/* L008 begin */
char	*command_name = "date";

#ifdef	INTL
static nl_catd	catd;
#endif	/* INTL */						/* L008 end */

struct	utmp	wtmp[2] = { {"","",OTIME_MSG,0,OLD_TIME,0,0,0},
			    {"","",NTIME_MSG,0,NEW_TIME,0,0,0} };

#define	GMT_TZ		"TZ=GMT0"	/* TZ if u option given	   L008 */
#define	LOCAL_DEFAULT_FMT	"%c"				/* L012 */
#define	POSIX_DEFAULT_FMT	"%a %b %e %H:%M:%S %Z %Y"	/* L008 L012 */

main(argc, argv)
int	argc;
char	**argv;							/* L008 */
{
	register char	c;					/* L008 begin */
	int wf, tfailed = 0;
	struct	tm  *tim;
	char	 buf[200];
	char *format = NULL;
	int tflag = 0;						/* L011 */
	int uflag = 0;
	char *str;
	char *def_fmt = LOCAL_DEFAULT_FMT;			/* L012 */

#ifdef	INTL
	setlocale(LC_ALL, "");
	catd = catopen(MF_DATE, MC_FLAGS);			/* L013 */
	{							/* L012 begin */
	char *dtlocale;
	if ((dtlocale = setlocale(LC_TIME, (char *) 0)) != (char *)0) {
		if (strcmp(dtlocale, "C_C.C") == 0 ||
		    strcmp(dtlocale, "C") == 0 ||
		    strcmp(dtlocale, "POSIX") == 0) {
			def_fmt = POSIX_DEFAULT_FMT;
		}
	}
	}							/* L012 end */
#endif	/* INTL */

	opterr = 0;
	while ((c = getopt(argc, argv, "t:u")) != -1)		/* L011 */
		switch(c) {
			case 't':				/* L011 */
				tflag = 1;			/* L011 */
				str = optarg;			/* L011 */
				break;				/* L011 */
			case 'u':
				uflag++;
				putenv(GMT_TZ);
				break;
			case '?':
				errorl(MSGSTR(DATE_OPT, "-%c illegal option"), optopt);
				usage();
				/* NOTREACHED */
		}

	if (!tflag){						/* L011 begin */
		if (argc-optind == 0)
			format = def_fmt;			/* L012 */
		else if (*argv[optind] == '+')
			format = argv[optind++] + 1;
		else 
			str = argv[optind++];
	}
	if (argc-optind != 0)
		usage();					/* L011 end */

	if (format != NULL) {
		/*
		 * display date in desired format
		 */
		(void) time(&timbuf);
		tim = localtime(&timbuf);

		if (strftime(buf, sizeof(buf), format, tim) == 0) {
			errorl(MSGSTR(DATE_CONV, "format conversion error"));
			exit(10);
		}

		printf("%s\n", buf);

		exit(0);
	}

	if (gtime(str, uflag, &timbuf, tflag)) {		/* L010 L011 */
		errorl(MSGSTR(DATE_BAD, "bad conversion"));
		usage();	/* M001 */
	}

	/* L009: GMT and DST conversions now done in gtime/tmtime */

	(void) time(&wtmp[0].ut_time);
	
	tim = localtime(&timbuf);				/* L010 */
	uadmin(A_CLOCK, tim->tm_tzadj);				/* S007	L010 */

	if(stime(&timbuf) >= 0) {				/* L010 */
		wtmp[1].ut_time = timbuf;			/* L010 */

/*	Attempt to write entries to the utmp file and to the wtmp file.	*/

		pututline(&wtmp[0]);
		pututline(&wtmp[1]);

		if ((wf = open(WTMP_FILE, 1)) >= 0) {
			(void) lseek(wf, 0L, 2);
			(void) write(wf, (char *)wtmp, sizeof(wtmp));
		}						/* L008 end */
	}							/* L010 */
	else {							/* L010 */
		tfailed++;
		errorl(MSGSTR(DATE_PERM, "no permission"));
	/*
	 *	L010: see Mod History comment: the following lines
	 *	are probably what was originally intended, but we've
	 *	shown the time we failed to set since XENIX days,
	 *	so it might be a bad idea to change it now
	 *
	 *	timbuf = wtmp[0].ut_time;
	 *	tim = localtime(&timbuf);
	 */
	}

	strftime(buf,sizeof(buf), def_fmt, tim);	/* L004 L008 L012 */
	printf("%s\n", buf);					/* L008 */

	exit(tfailed?2:0);
}

usage()
{
	(void) fprintf(stderr, MSGSTR(DATE_USAGE, "Usage: date [-u] [+format]\n       date [-u] [-t [[CC]YYMMDDhhmm[.SS] | MMDDhhmm[YY] ]\n"));		/* L008 L009 L011 */
	exit(2);
}
