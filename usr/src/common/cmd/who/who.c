#ident	"@(#)who:common/cmd/who/who.c	1.23.2.22"

/***************************************************************************
 * Command: who
 *
 * Inheritable Privileges: None
 *       Fixed Privileges: None
 * Inheritable Authorizations: None
 *       Fixed Authorizations: None
 *
 * Notes:	This program analyzes information found in /var/adm/utmp.
 *		Additionally information is gathered from /etc/inittab
 *		if requested.
 *	
 *		Syntax:
 *	
 *			who am i	Displays info on yourself
 *	
 *			who -a		Displays information with ALL options
 *	
 *			who -A		Displays ACCOUNTING information
 *					(non-functional)
 *	
 *			who -b		Displays info on last BOOT
 *	
 *			who -d		Displays info on DEAD PROCESSES
 *
 *			who -f		Don't display pseudo-ttys.  An OSR
 *					option that does nothing on UW, since
 *					UW doesn't log pseudo-ttys in utmp.
 *	
 *			who -H		Displays HEADERS for output
 *	
 *			who -l 		Displays info on LOGIN entries
 *					(plus port monitors if POSIX2 not set)
 *	
 *			who -L 		Displays info on port monitors
 *					(only)
 *	
 *			who -m 		Displays info on the current terminal
 *
 *			who -p 		Displays info on PROCESSES spawned
 *					by init
 *	
 *			who -q [ -n# ]	Displays short information on current
 *					users who are logged on,
 *					showing # entries per line (default 8)
 *	
 *			who -r		Displays info on current RUN-LEVEL
 *	
 *			who -s		Displays requested info in SHORT form
 *	
 *			who -t		Displays info on TIME changes
 *	
 *			who -T		Displays writeability of each user
 *					(+ writeable, - non-writeable, ? hung)
 *	
 *			who -u		Displays long info on users
 *					who have logged on
 *
 ***************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <utmp.h>
#include <locale.h>
#include <pfmt.h>
#include <libgen.h>

#define DATE_FMT	"%b %e %H:%M"	/* format used to print the date */
#define DATE_FMTID	":734"		/* the corresponding message id */
#define DATE_SIZE	40		/* cftime buffer size for date */

/*
 * Sizes of character arrays in utmp file entries.
 * The ut_user[] array is not necessarily null-terminated.
 * For safety's sake, we assume the same is true of ut_line[].
 */
#define USERSZ	sizeof(((struct utmp *)0)->ut_user)
#define LINESZ	sizeof(((struct utmp *)0)->ut_line)

/*
 * File-scope globals.
 */
static char	*myname;	/* pointer to invoker's name 	*/
static char	nameval[9];		/* holds invoker's name		*/
static char	*mytty;			/* device the invoker is on	*/
static char	outbuf[BUFSIZ];		/* stdio buffer for output	*/	
static time_t	timnow;			/* holds current time		*/
static int	totlusrs = 0;		/* counter for users on system	*/
static int	Hopt = 0;		/* 1 = who -H			*/
static int	justme = 0;		/* 1 = who am i			*/
static int	qopt = 0;		/* 1 = who -q			*/
static int	sopt = 0;		/* 1 = who -s 	       		*/
static int	Topt = 0;		/* 1 = who -T			*/
static int	terse = 1;		/* 1 = print terse msgs		*/
static int	uopt = 0;		/* 1 = who -u			*/
static int	validtype[UTMAXTYPE+1];	/* 1 = print this ut_type	*/
static int	number = 8;		/* number of users per -q line	*/
static int	optcnt = 0;		/* keeps count of options	*/
static	int	aopt = 0;		/* 1 = who -a			*/
static	int	bopt = 0;		/* 1 = who -b			*/
static	int	dopt = 0;		/* 1 = who -d			*/
static	int	lopt = 0;		/* 1 = who -l			*/
static	int	Lopt = 0;		/* 1 = who -L			*/
static	int	popt = 0;		/* 1 = who -p			*/
static	int	ropt = 0;		/* 1 = who -r			*/


static void	process();
static void	ck_file();
static int	statdevice();

#ifdef __STDC__
extern void	getinittab(void);
extern char *	getinitcomment(char *);
#else
extern void	getinittab();
extern char *	getinitcomment();
#endif

static char posix_var[] = "POSIX2";
static int posix;

main(argc, argv)
	int	argc;
	char	**argv;
{
	int	usage = 0;	/* non-zero indicates cmd error	*/
	int	optsw;		/* switch for while of getopt()	*/
	char	*program;	/* name of this program */

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxcore.abi");
	(void) setlabel("UX:who");

	if (getenv(posix_var) != 0)	{
		posix = 1;
	} else	{
		posix = 0;
	}

	validtype[USER_PROCESS] = 1;
	validtype[EMPTY] = 0;

	/*
	 * Strip off path name of this command.
	 */
	program = basename(argv[0]);

	/*
	 * Buffer stdout for speed.
	 */
	setbuf(stdout, outbuf);

	/*
	 * Retrieve options specified on command line.
	 */
	while ((optsw = getopt(argc, argv, "abdfHlLmn:pqrstTu")) != EOF) {
		optcnt++;
		switch (optsw) {
	
		case 'a':
			if (posix) {
				aopt = 1;
			} else {
      				optcnt += 8;	/* AbdlprTtu less one already counted */
				validtype[ACCOUNTING] = 1;
				validtype[BOOT_TIME] = 1;
				validtype[DEAD_PROCESS] = 1;
				validtype[LOGIN_PROCESS] = 1;
				validtype[INIT_PROCESS] = 1;
				validtype[RUN_LVL] = 1;
				validtype[OLD_TIME] = 1;
				validtype[NEW_TIME] = 1;
				validtype[USER_PROCESS] = 1;
				uopt = 1;
				Topt = 1;
				Hopt = 1;
				if (!sopt)
					terse = 0;
			}
			break;

		case 'A':
			validtype[ACCOUNTING] = 1;
			if (!uopt)
				validtype[USER_PROCESS] = 0;
			terse = 0;
			break;

		case 'b':
			if (posix) {
				bopt = 1;
			} else {
				validtype[BOOT_TIME] = 1;
				if (!uopt) validtype[USER_PROCESS] = 0;
			}
			break;

		case 'd':
			if (posix) {
				dopt = 1;
			} else {
				validtype[DEAD_PROCESS] = 1;
				if (!sopt) terse = 0;
				if (!uopt) validtype[USER_PROCESS] = 0;
			}
			break;

		case 'f':
			optcnt--;	/* don't count this option */
					/* Since it doesn't do anything */
			break;

		case 'H':
			optcnt--;	/* don't count this option */
			Hopt = 1;
			break;

		case 'l':
			lopt = 1;
			if (!posix) {
				Lopt=1;
				validtype[LOGIN_PROCESS] = 1;
				if (!uopt) validtype[USER_PROCESS] = 0;
				terse = 0;
			}
			break;

		case 'L':
			Lopt = 1;
			if (!posix) {
				validtype[LOGIN_PROCESS] = 1;
				if (!uopt) validtype[USER_PROCESS] = 0;
				terse = 0;
			}
			break;

		case 'm':
			justme = 1;
			myname = nameval;
			cuserid(myname);
			if ((mytty = ttyname(fileno(stdin))) == NULL &&
			    (mytty = ttyname(fileno(stdout))) == NULL &&
			    (mytty = ttyname(fileno(stderr))) == NULL) {
				pfmt(stderr, MM_ERROR,
					":1222:Must be attached to a terminal for the '-m' option\n");
				fflush(stderr);
				exit(1);
			} else mytty += 5; /* bump past "/dev/" */
			break;

		case 'n':
			number = atoi(optarg);
			if (number < 1) {
				pfmt(stderr, MM_ERROR,
					":735:Number of users per line must be at least 1\n");
				exit(1);
			}
			break;

		case 'p':
			if (posix) {
				popt = 1;
			} else {
				validtype[INIT_PROCESS] = 1;
				if (!sopt) terse = 0;
				if (!uopt) validtype[USER_PROCESS] = 0;
			}
			break;

		case 'q':
			qopt = 1;
			break;

		case 'r':
			if (posix) {
				ropt = 1;
			} else {
				validtype[RUN_LVL] = 1;
				terse = 0;
				if (!uopt) validtype[USER_PROCESS] = 0;
			}
			break;

		case 's':
			sopt = 1;
			if (!posix) {
				terse = 1;
			}
			break;

		case 't':
			validtype[OLD_TIME] = 1;
			validtype[NEW_TIME] = 1;
			if (!uopt)
				validtype[USER_PROCESS] = 0;
			break;

		case 'T':
			Topt = 1;
			validtype[USER_PROCESS] = 1;
			if (posix && !uopt) {
				terse = 1;
			} else {
				if (!sopt)
					terse = 0;
				uopt = 1;
			}
			break;
	
		case 'u':
			uopt = 1;
			validtype[USER_PROCESS] = 1;
			if (!sopt || posix) {
				terse = 0;
			}
			break;
	
		case '?':
			usage++;
			break;
		}
	}

	if (posix && !(justme || Topt || uopt)) {
		if (aopt) {
			uopt = 1;
			Topt = 1;
			if (!sopt) {
				terse = 0;
			}
			validtype[OLD_TIME] = 1;
			validtype[NEW_TIME] = 1;
			if (!uopt) validtype[USER_PROCESS] = 0;
		}
		if (bopt) {
			validtype[BOOT_TIME] = 1;
			if (!uopt) validtype[USER_PROCESS] = 0;
		}
		if (dopt) {
			validtype[DEAD_PROCESS] = 1;
			if (!sopt) terse = 0;
			if (!uopt) validtype[USER_PROCESS] = 0;
		}
		if (lopt || Lopt) {
			validtype[LOGIN_PROCESS] = 1;
			validtype[USER_PROCESS] = 0;
			terse = 0;
		}
		if (popt) {
			validtype[INIT_PROCESS] = 1;
			if (!sopt) terse = 0;
			if (!uopt) validtype[USER_PROCESS] = 0;
		}
		if (ropt) {
			validtype[RUN_LVL] = 1;
			terse = 0;
			if (!uopt) validtype[USER_PROCESS] = 0;
		}
	}

	/*
	 * -q suppresses other options.
	 */
	if (qopt) {
		if (posix && (Topt || justme || uopt)) {
			qopt = 0;
		} else {
			Hopt=sopt=Topt=uopt=0;
			validtype[EMPTY] = 0;
			validtype[ACCOUNTING] = 0;
			validtype[BOOT_TIME] = 0;
			validtype[DEAD_PROCESS] = 0;
			validtype[LOGIN_PROCESS] = 0;
			validtype[INIT_PROCESS] = 0;
			validtype[RUN_LVL] = 0;
			validtype[OLD_TIME] = 0;
			validtype[NEW_TIME] = 0;
			validtype[USER_PROCESS] = 1;
		}
	}

	/*
	 * We have one of the following cases:
	 * who [ options ]
	 * who [ options ] utmp_like_file
	 * who am i (or I)
	 * usage error
	 */
	if (argc <= optind)
		;
	else if (argc == optind + 1) {
		/*
		 * An alternate utmp file was specified; validate the file and
		 * arrange for getutent() to read it instead of the default.
		 */
		optcnt++;
		ck_file(argv[optind]);
		utmpname(argv[optind]);
	} else if (argc == 3 && strcmp(argv[1], "am") == 0
		&& (strcmp(argv[2], "i") == 0 || strcmp(argv[2], "I") == 0)) {
		/*
		 * "who am i" or "who am I".
		 */
		justme = 1;
		myname = nameval;
		(void) cuserid(myname);
		if ((mytty = ttyname(fileno(stdin))) == NULL &&
		    (mytty = ttyname(fileno(stdout))) == NULL &&
		    (mytty = ttyname(fileno(stderr))) == NULL) {
			pfmt(stderr, MM_ERROR,
				":750:Must be attached to a terminal for the 'am I' option\n");
			exit(1);
		} else
			mytty += 5; /* bump past "/dev/" */
	} else {
		pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
		usage++;
	}

	if (usage) {
		pfmt(stderr, MM_ACTION, ":1363:Usage:\t%s [-abdHlLmnpqrstTu] [am i] [utmp_like_file]\n", program);
		pfmt(stderr, MM_NOSTD, ":737:a\tall (Abdlprtu options)\n");
		pfmt(stderr, MM_NOSTD, ":738:b\tboot time\n");
		pfmt(stderr, MM_NOSTD, ":739:d\tdead processes\n");
		pfmt(stderr, MM_NOSTD, ":740:H\tprint header\n");
		pfmt(stderr, MM_NOSTD, ":741:l\tlogin processes\n");
		pfmt(stderr, MM_NOSTD, ":1364:L\tport monitors\n");
		pfmt(stderr, MM_NOSTD, ":1224:m\tinformation about the current terminal\n");
		pfmt(stderr, MM_NOSTD, ":742:n #\tspecify number of users per line for -q\n");
		pfmt(stderr, MM_NOSTD, ":743:p\tprocesses other than getty or users\n");
		pfmt(stderr, MM_NOSTD, ":744:q\tquick %s\n", program);
		pfmt(stderr, MM_NOSTD, ":745:r\trun level\n");
		pfmt(stderr, MM_NOSTD, ":746:s\tshort form of %s (no time since last output or pid)\n", program);
		pfmt(stderr, MM_NOSTD, ":747:t\ttime changes\n");
		pfmt(stderr, MM_NOSTD, ":748:T\tstatus of tty (+ writable, - not writable, ? hung)\n");
		pfmt(stderr, MM_NOSTD, ":749:u\tuseful information\n");
		exit(1);
	}

	if (terse) {
		if (Hopt)
			pfmt(stdout, MM_NOSTD, ":753:NAME       LINE         TIME\n");
	} else {
		if (Hopt)
			pfmt(stdout, MM_NOSTD, ":751:NAME       LINE         TIME          IDLE    PID  COMMENTS\n");

		timnow = time((time_t *)0);
		getinittab();
	}

	process();

	/*
	 * 'who -q' requires EOL upon exit, followed by total line.
	 */
	if (qopt)
		pfmt(stdout, MM_NOSTD, ":754:\n# users=%d\n", totlusrs);

	exit(0);
	/* NOTREACHED */
}

/*
 * Print the desired information for the specified utmp entry.
 * "line" is generally the same as up->ut_line, but see secondpass().
 */

static void
dump(up, line)
	register struct utmp *up;
	char	*line;
{
	char	user[USERSZ + 1];	/* null-terminated user name */
	char	device[LINESZ + 1];	/* null-terminated tty device */
	char	time_buf[DATE_SIZE];	/* format buffer for cftime() */
	struct	stat statb;		/* for stat of device */
	time_t	idle;			/* device idle time */
	time_t	hr;			/* idle hours */
	time_t	min;			/* idle minutes */
	char	pexit;			/* process exit status */
	char	pterm;			/* process termination status */
	char	w;			/* writeability indicator */

	/*
	 * Get and check user name.
	 */
	if (up->ut_user[0] == '\0')
		strcpy(user, "   .");
	else {
		strncpy(user, up->ut_user, USERSZ);
		user[USERSZ] = '\0';
	}

	/*
	 * Do print in 'who -q' format,
	 * and keep track of total number of users
	 * for later printing in main().
	 */
	if (qopt) {
		if ((totlusrs % number) == 0 && totlusrs != 0)
			printf("\n");
		totlusrs++;
		printf("%-8s ", user);
		return;
	}

	/*
	 * Get exit info if applicable.
	 */
	if (up->ut_type == RUN_LVL || up->ut_type == DEAD_PROCESS) {
		pexit = up->ut_exit.e_exit;
		pterm = up->ut_exit.e_termination;
	} else
		pexit = pterm = ' ';

	/*
	 * Get and format ut_time field.
	 */
	cftime(time_buf, gettxt(DATE_FMTID, DATE_FMT), &up->ut_time);

	/*
	 * Get and massage device.
	 */
	if (line[0] == '\0')
		strcpy(device, "     .");
	else {
		strncpy(device, line, LINESZ);
		device[LINESZ] = '\0';
	}

	/*
	 * Get writeability if requested.
	 */
	if (Topt && up->ut_type == USER_PROCESS) {
		if (statdevice(device, &statb) == -1)
			w = '?';
		else if (statb.st_mode & (S_IWGRP | S_IWOTH))
			w = '+';
		else
			w = '-';
	} else
		w = ' ';

	/*
	 * Print the TERSE portion of the output.
	 */
	printf("%-8s %c %-12s %s", user, w, device, time_buf);

	/*
	 * Print the VERBOSE portion of the output if requested.
	 */
	if (!terse) {
		/*
		 * Print the IDLE and PID fields.  Allowance is made here
		 * for printing different things depending on the type of
		 * entry we're dealing with.  Currently, all process entries
		 * are treated the same and nothing is printed for others.
		 */
		switch (up->ut_type) {
		case INIT_PROCESS:
		case DEAD_PROCESS:
		case LOGIN_PROCESS:
		case USER_PROCESS:
			/* stat device for idle time */
			if (statdevice(device, &statb) == -1)
				printf("   .  ");
			else {
				idle = timnow - statb.st_mtime;
				hr = idle / 3600;
				min = (unsigned)(idle / 60) % 60;
	
				if (hr == 0 && min == 0)
					printf("   .  ");
				else if (hr < 24)
					pfmt(stdout, MM_NOSTD,
						":755: %2d:%2.2d", hr, min);
				else
					pfmt(stdout, MM_NOSTD, ":756:  old ");
			}
			printf("  %5d", up->ut_pid);
			break;
		}

		/*
		 * Print the COMMENTS field.
		 */
		switch (up->ut_type) {
		case INIT_PROCESS:
			pfmt(stdout, MM_NOSTD, ":758:  id=%4.4s", up->ut_id);
			break;

		case DEAD_PROCESS:
			pfmt(stdout, MM_NOSTD,
				":757:  id=%4.4s term=%-3d exit=%d  ",
				up->ut_id, pterm, pexit);
			break;

		case LOGIN_PROCESS:
		case USER_PROCESS:
			printf("  %s", getinitcomment(up->ut_id));
			break;
		}
	}

	/*
	 * Special case for RUN_LVL; the three values we're printing here are
	 * actually the system's current init state (run level), the number of
	 * times previously in this state, and the prior state, respectively.
	 * The format of the RUN_LVL entry should be maintained, because it is
	 * used by scripts that need to determine the system's init state.
	 */
	if (up->ut_type == RUN_LVL)
		printf("    %c%5d    %c", pterm, up->ut_pid, pexit);

	/*
	 * ... and finally, terminate the line.
	 */
	printf("\n");

	/*
	 * If the user did a "who -r" or "who -b", with no other options and
	 * no alternate utmp file specified, then we're all done; the standard
	 * utmp file contains only one of each corresponding entry.
	 */
	if (optcnt == 1 && !validtype[USER_PROCESS] &&
		(up->ut_type == RUN_LVL || up->ut_type == BOOT_TIME))
		exit(0);
}

/*
 * Scan the utmp file, passing each valid entry
 * to func() for processing.
 */

static void
scanutmp(func)
	void (*func)();
{
	register struct utmp *up;

	setutent();

	while ((up = getutent()) != NULL) {
#ifdef DEBUG
		printf("user '%.8s', id '%.4s', line '%.12s', type '%d'\n",
			up->ut_user, up->ut_id, up->ut_line, up->ut_type);
#endif
		if (up->ut_type > UTMAXTYPE) {
			pfmt(stderr, MM_ERROR,
				":759:Entry has ut_type of %d when maximum is %d\n",
				up->ut_type, UTMAXTYPE);
			exit(1);
		}

		func(up);
	}
}

/*
 * Called via scanutmp() on first pass through the utmp file.
 * If this is an entry we're interested in, print it.
 * For the "who am I" case, once we find and print the user's
 * entry we can exit.
 */

static void
firstpass(up)
	register struct utmp *up;
{
	if (justme) {			/* "who am I" */
		if (strncmp(myname, up->ut_user, USERSZ) == 0
		 && strncmp(mytty,  up->ut_line, LINESZ) == 0) {
			dump(up, up->ut_line);
			exit(0);	/* all done */
		}
	} else if (validtype[up->ut_type]) {
		if (up->ut_type == LOGIN_PROCESS) {
			/*
			 * If -l print out only those with name "LOGIN",
			 * if -L, print out all those but "LOGIN"
			 * if both, print out both.
			 */
			if (strcmp(up->ut_user, "LOGIN") == 0) {
				if (Lopt && !lopt)
					return;
			} else if (lopt && !Lopt)
				return;
		}
		dump(up, up->ut_line);
	}
}

/*
 * Called via scanutmp() on second pass through the utmp file.
 * This takes place only in the "who am I" case when we did
 * not find the user's entry on the first pass.
 * This time through, we're looking for an entry that has
 * the right user name, regardless of the ut_line.
 * Once we find and print this entry we can exit.
 */

static void
secondpass(up)
	struct utmp *up;
{
	if (strncmp(myname, up->ut_user, USERSZ) == 0) {
		dump(up, mytty);
		exit(0);
	}
}

/*
 * Scan the utmp file looking for entries that we're interested in
 * and print them.  In the "who am I" (justme) case, we can exit
 * as soon as we find and print the user's entry.
 * If we don't find the user's entry with a name and line match
 * on the first pass through, make a second pass looking for just
 * a name match.
 */

static void
process()
{
	scanutmp(firstpass);

	/*
	 * If justme ("who am I" case) and got here, must be a vt.
	 * Scan the utmp file again looking for just a name match this time.
	 * Assuming utmp hasn't been updated with vt name.
	 */
	if (justme)
		scanutmp(secondpass);
}

/*
 * This routine checks the following:
 *
 * 1.	File exists
 *
 * 2.	We have read permissions
 *
 * 3.	It is a multiple of utmp entries in size
 *
 * Failing any of these conditions causes us to
 * abort processing.
 *
 * 4.	If file is empty we exit right away as there
 *	is no info to report on.
 */

static void
ck_file(name)
	char	*name;
{
	FILE	*file;
	struct	stat statb;

	/*
	 * Does file exist? Do stat to check, and save structure
	 * so that we can check on the file's size later on.
	 */
	if (stat(name, &statb) == -1) {
		pfmt(stderr, MM_ERROR,
			":5:Cannot access %s: %s\n", name, strerror(errno));
		exit(1);
	}

	/*
	 * The only real way we can be sure we can access the
	 * file is to try. If we succeed then we close it.
	 */
	if ((file = fopen(name, "r")) == NULL) {
		pfmt(stderr, MM_ERROR,
			":4:Cannot open %s: %s\n", name, strerror(errno));
		exit(1);
	}
	fclose(file);

	/*
	 * If the file is empty, we are all done.
	 */
	if (statb.st_size == 0)
		exit(0);

	/*
	 * Make sure the file is a utmp file.
	 * We can only check for size being a multiple of
	 * utmp structures in length.
	 */
	if (statb.st_size % sizeof(struct utmp)) {
		pfmt(stderr, MM_ERROR, ":760:File '%s' is not a utmp file\n",
			name);
		exit(1);
	}
}

/*
 * Stat the device whose name relative to /dev is given.
 * The name is from a ut_line[] array but is guaranteed
 * to be null-terminated.
 */

static int
statdevice(dev, statp)
	char	*dev;
	struct	stat *statp;
{
	char	path[sizeof("/dev/") + LINESZ];

	/*
	 * When there is no device, dump() passes us a string
	 * starting with blanks.  Don't bother trying to stat it!
	 */
	if (*dev == ' ')
		return -1;

	/*
	 * Construct the full pathname and stat it.
	 */
	strcpy(path, "/dev/");
	strcat(path, dev);
	return stat(path, statp);
}
