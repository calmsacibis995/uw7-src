/*		copyright	"%c%" 	*/

#ident	"@(#)autopush.c	1.2"

/***************************************************************************
 * Command: autopush
 * Inheritable Privileges: P_DEV,P_DACREAD,P_DACWRITE,P_MACREAD,P_MACWRITE,P_DRIVER
 *       Fixed Privileges: None
 * 
 * Notes:
 *
 * autopush(1) is the command interface to the STREAMS
 * autopush mechanism.  The autopush command can be used
 * to configure autopush information about a STREAMS driver,
 * remove autopush information, and report on current configuration
 * information.  Its use is as follows:
 *
 *	autopush -f file
 *	autopush -r -M major -m minor
 *	autopush -g -M major -m minor
 *
 * The -f option allows autopush information to be set from a file.  The
 * format of the file is as follows:
 *
 * # Comment lines begin with a # in column one.
 * # The fields are separated by white space and are:
 * # major	minor	lastminor	module1 module2 ... module8
 *
 * "lastminor" is used to configure ranges of minor devices, from "minor"
 * to "lastminor" inclusive.  It should be set to zero when not in use.
 * The -r option allows autopush information to be removed for the given
 * major/minor pair.  The -g option allows the configuration information
 * to be printed.  The format of printing is the same as for the file.
 *
 */

#include <sys/types.h>
#include <sys/conf.h>
#include <sys/sad.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <memory.h>
#include <priv.h>
#include <ctype.h> 
#include <pfmt.h> 
#include <locale.h> 

#define OPTIONS	"M:f:gm:r"	/* command line options for getopt(3C) */
#define COMMENT	'#'
#define MINUS	'-'
#define SLASH	'/'

/*
 * Output format.
 */
#define OHEADER		":1:     Major      Minor  Lastminor\tModules\n"

#define OFORMAT1_ONE	":2:%10ld %10ld      -    \t"
#define OFORMAT1_RANGE	":3:%10ld %10ld %10ld\t"
#define OFORMAT1_ALL	":4:%10ld       ALL       -    \t"
#define OFORMAT2	"%s "
#define OFORMAT3	"%s\n"

static char *Openerr = ":5:Could not open %s\n";
static char *Digiterr = ":6:argument to %s option must be numeric\n";
static char *Badline = ":7:File %s: bad input line %d ignored\n";
static char *Cmdp;			/* command name */

static void usage();
static int rem_info(), get_info(), set_info();
static int is_white_space(), parse_line();

extern int errno;
extern long atol();
extern void exit();
extern int getopt();
extern int ioctl();
int saverrno;

/*
 * Procedure:     main
 *
 * Restrictions:
 *               getopt:  None
 *               fprintf: None
 *
 *	process command line arguments.
 */

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int c;			/* character read by getopt(3C) */
	char *filenamep;	/* name of configuration file */
	major_t major;		/* major device number */
	minor_t minor;		/* minor device number */
	char *cp;
	int exitcode;
	ushort minflag = 0;	/* -m option used */
	ushort majflag = 0;	/* -M option used */
	ushort fflag = 0;	/* -f option used */
	ushort rflag = 0;	/* -r option used */
	ushort gflag = 0;	/* -g option used */
	ushort errflag = 0;	/* options usage error */
	extern char *optarg;	/* for getopts - points to argument of option */
	extern int optind;	/* for getopts - index into argument list */

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxautopush");
	(void)setlabel("UX:autopush");
	/*
	 * Get command name.
	 */
	Cmdp = argv[0];
	for (filenamep = argv[0]; *filenamep; filenamep++)
		if (*filenamep == SLASH)
			Cmdp = filenamep + 1;

	/*
	 * Get options.
	 */
	while (!errflag && ((c = getopt(argc, argv, OPTIONS)) != -1)) {
		switch (c) {
		case 'M':
			if (fflag|majflag)
				errflag++;
			else {
				majflag++;
				for (cp = optarg; *cp; cp++)
					if (!isdigit((int)*cp)) {
						(void) pfmt(stderr, MM_ERROR,
						    Digiterr, "-M");
						exit(1);
					}
				major = (major_t)atol(optarg);
			}
			break;

		case 'm':
			if (fflag|minflag)
				errflag++;
			else {
				minflag++;
				for (cp = optarg; *cp; cp++)
					if (!isdigit((int)*cp)) {
						(void) pfmt(stderr, MM_ERROR,
						    Digiterr, "-m");
						exit(1);
					}
				minor = (minor_t)atol(optarg);
			}
			break;

		case 'f':
			if (fflag|gflag|rflag|majflag|minflag)
				errflag++;
			else {
				fflag++;
				filenamep = optarg;
			}
			break;

		case 'r':
			if (fflag|gflag|rflag)
				errflag++;
			else
				rflag++;
			break;

		case 'g':
			if (fflag|gflag|rflag)
				errflag++;
			else
				gflag++;
			break;

		default:
			errflag++;
			break;
		} /* switch */
		if (errflag) {
			usage();
			exit(1);
		}
	} /* while */
	if (((gflag || rflag) && (!majflag || !minflag)) || (optind != argc)) {
		usage();
		exit(1);
	}
	if (fflag)
		exitcode = set_info(filenamep);
	else if (rflag)
		exitcode = rem_info(major, minor);
	else if (gflag)
		exitcode = get_info(major, minor);
	else {
		usage();
		exit(1);
	}
	exit(exitcode);
	/* NOTREACHED */
}

/*
 * Procedure:     usage
 *
 * Restrictions:
 *               fprintf: None
 *
 *	print out usage statement.
*/

static void
usage()
{
	(void) pfmt(stderr, MM_ACTION,
		":8:Usage:\n\t%s -f filename\n", Cmdp);
	(void) pfmt(stderr, MM_NOSTD, 
		":9:\t%s -r -M major -m minor\n", Cmdp);
	(void) pfmt(stderr, MM_NOSTD,
		 ":10:\t%s -g -M major -m minor\n", Cmdp);
}

/*
 * Procedure:     set_info
 *
 * Restrictions:
                 open(2): None
                 fprintf: None
                 fopen: None
                 fgets: None
                 ioctl(2): None
 *  Notes:
 *	set autopush configuration information.
 */
static int
set_info(namep)
	char *namep;		/* autopush configuration filename */
{
	int line;		/* line number of file */
	FILE *fp;		/* file pointer of config file */
	char buf[256];		/* input buffer */
	struct strapush push;	/* configuration information */
	int sadfd;		/* file descriptor to SAD driver */
	int retcode = 0;	/* return code */

	if ((sadfd = open(ADMINDEV, O_RDWR)) < 0) {
		(void) pfmt(stderr, MM_ERROR, Openerr, ADMINDEV);
		(void) pfmt(stderr, MM_NOGET|MM_ERROR,
			"%s\n", strerror(errno));
		return (1);
	}

	if ((fp = fopen(namep, "r")) == NULL) {
		(void) pfmt(stderr, MM_ERROR, Openerr, namep);
		(void) pfmt(stderr, MM_NOGET|MM_ERROR,
			"%s\n", strerror(errno));
		return (1);
	}

	line = 0;
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		line++;
		if ((buf[0] == COMMENT) || is_white_space(buf))
			continue;
		(void) memset((char *)&push, 0, sizeof(struct strapush));
		if (parse_line(buf, line, namep, &push))
			continue;
		if (push.sap_minor == (minor_t)-1)
			push.sap_cmd = SAP_ALL;
		else if (push.sap_lastminor == 0)
			push.sap_cmd = SAP_ONE;
		else
			push.sap_cmd = SAP_RANGE;
		errno = 0;


		if (ioctl(sadfd, SAD_SAP, &push) < 0) {
			retcode = 1;
			saverrno = errno;
			(void) pfmt(stderr, MM_ERROR,
				":11:File %s: could not configure autopush for line %d\n",
					namep, line);
			errno = saverrno;
			switch (errno) {
			case EPERM:
				(void) pfmt(stderr, MM_ERROR,
					":12:You do not have permission to set autopush information\n");
				break;

			case EINVAL:
				(void) pfmt(stderr, MM_ERROR,
					":13:Invalid major device number or\n\t\tinvalid module name or too many modules\n");
				break;

			case ENOSTR:
				(void) pfmt(stderr, MM_ERROR,
					":14:Major device is not a STREAMS driver\n");
				break;

			case EEXIST:
				(void) pfmt(stderr, MM_ERROR,
					":15:Major/minor already configured\n");
				break;

			case ENOSR:
				(void) pfmt(stderr, MM_ERROR,
					":16:Ran out of autopush structures\n");
				break;

			case ERANGE:
				(void) pfmt(stderr, MM_ERROR,
					":17:lastminor must be greater than minor\n");
				break;

			default:
				(void) pfmt(stderr, MM_NOGET|MM_ERROR,
					"%s\n", strerror(errno));
				break;
			} /* switch */
		} /* if */
		/* clear all privileges after ioctl() success */
	} /* while */
	return (retcode);
}

/*
 * Procedure:     rem_info
 *
 * Restrictions:
                 open(2): None
                 fprintf: None
                 ioctl(2): None
 *  Notes:
 *	remove autopush configuration information.
 */

static int
rem_info(maj, min)
	major_t maj;
	minor_t min;
{
	struct strapush push;	/* configuration information */
	int sadfd;		/* file descriptor to SAD driver */
	int retcode = 0;	/* return code */


	if ((sadfd = open(ADMINDEV, O_RDWR)) < 0) {
		(void) pfmt(stderr, MM_ERROR, Openerr, ADMINDEV);
		(void) pfmt(stderr, MM_NOGET|MM_ERROR,
			"%s\n", strerror(errno));
		/* clear all privileges after open() failed */
		return (1);
	}
	push.sap_cmd = SAP_CLEAR;
	push.sap_minor = min;
	push.sap_major = maj;
	errno = 0;


	if (ioctl(sadfd, SAD_SAP, &push) < 0) {
		retcode = 1;
		saverrno = errno;
		(void) pfmt(stderr, MM_ERROR,
			":18:Could not remove autopush information\n");
		errno = saverrno;
		switch (errno) {
		case EPERM:
			(void) pfmt(stderr, MM_ERROR,
				":19:You do not have permission to remove autopush information\n");
			break;

		case EINVAL:
			if ((min != 0) && (ioctl(sadfd, SAD_GAP, &push) == 0) &&
			    (push.sap_cmd == SAP_ALL))
				(void) pfmt(stderr, MM_ERROR,	
					":20:When removing an entry for ALL minors, minor must be set to 0\n");
			else
				(void) pfmt(stderr, MM_ERROR,
					":21:Invalid major device number\n");
			break;

		case ENODEV:
			(void) pfmt(stderr, MM_ERROR,
				":22:Major/minor not configured for autopush\n");
			break;

		case ERANGE:
			(void) pfmt(stderr, MM_ERROR,
				":23:minor must be set to begining of range when clearing\n");
			break;

		default:
			(void) pfmt(stderr, MM_NOGET|MM_ERROR,
				"%s\n", strerror(errno));
			break;
		} /* switch */
	}
	/* clear all privileges after ioctl() success */
	return (retcode);
}

/*
 * Procedure:     get_info
 *
 * Restrictions:
                 open(2): P_MACREAD,P_DACREAD
                 fprintf: None
                 ioctl(2): None
                 printf: None
 *  Notes:
 *	get autopush configuration information.
 */
static int
get_info(maj, min)
	major_t maj;
	minor_t min;
{
	struct strapush push;	/* configuration information */
	int i;			/* counter */
	int sadfd;		/* file descriptor to SAD driver */


	procprivl (CLRPRV, pm_work(P_DACREAD), pm_work(P_MACREAD), (priv_t)0);

	if ((sadfd = open(USERDEV, O_RDWR)) < 0) {
		(void) pfmt(stderr, MM_ERROR, Openerr, USERDEV);
		(void) pfmt(stderr, MM_NOGET|MM_ERROR,
			"%s\n", strerror(errno));
		procprivl (SETPRV, pm_work(P_DACREAD), pm_work(P_MACREAD), (priv_t)0);
		return (1);
	}
	procprivl (SETPRV, pm_work(P_DACREAD), pm_work(P_MACREAD), (priv_t)0);
	push.sap_major = maj;
	push.sap_minor = min;
	errno = 0;
	if (ioctl(sadfd, SAD_GAP, &push) < 0) {
		saverrno = errno;
		(void) pfmt(stderr, MM_ERROR,
			":24:Could not get autopush information\n");
		errno = saverrno;
		switch (errno) {
		case EINVAL:
			(void) pfmt(stderr, MM_ERROR,
				":21:Invalid major device number\n");
			break;

		case ENOSTR:
			(void) pfmt(stderr, MM_ERROR,
				":21:Invalid major device number\n");
			break;

		case ENODEV:
			(void) pfmt(stderr, MM_ERROR,
				":22:Major/minor not configured for autopush\n");
			break;

		default:
			(void) pfmt(stderr, MM_NOGET|MM_ERROR,
				"%s\n", strerror(errno));
			break;
		} /* switch */
		return (1);
	}
	(void) pfmt(stdout, MM_NOSTD, OHEADER);
	switch (push.sap_cmd) {
	case SAP_ONE:
		(void) pfmt(stdout, MM_NOSTD,
			OFORMAT1_ONE, push.sap_major, push.sap_minor);
		break;

	case SAP_RANGE:
		(void) pfmt(stdout, MM_NOSTD,
			OFORMAT1_RANGE, push.sap_major, push.sap_minor, push.sap_lastminor);
		break;

	case SAP_ALL:
		(void) pfmt(stdout, MM_NOSTD,
			OFORMAT1_ALL, push.sap_major);
		break;

	default:
		(void) pfmt(stderr, MM_ERROR,
			 ":25:Unknown configuration type\n");
		return (1);
	}
	if (push.sap_npush > 1)
		for (i = 0; i < (push.sap_npush - 1); i++)

			(void) fprintf(stdout, OFORMAT2, push.sap_list[i]);
	(void) fprintf(stdout, OFORMAT3, push.sap_list[(push.sap_npush - 1)]);
	return (0);
}

/*
 * is_white_space():
 *	Return 1 if buffer is all white space.
 *	Return 0 otherwise.
 */
static int
is_white_space(bufp)
	register char *bufp;	/* pointer to the buffer */
{

	while (*bufp) {
		if (!isspace((int)*bufp))
			return (0);
		bufp++;
	}
	return (1);
}

/*
 * Procedure:     parse_line
 *
 * Restrictions:
 *               fprintf: None
 *
 *  Notes:
 *	Parse input line from file and report any
 *	errors found.  Fill strapush structure along
 *	the way.  Returns 1 if the line has errors
 *	and 0 if the line is well-formed.
 *	Another hidden dependency on MAXAPUSH.
 */
static int
parse_line(linep, lineno, namep, pushp)
	char *linep;				/* the input buffer */
	int lineno;				/* the line number */
	char *namep;				/* the file name */
	register struct strapush *pushp;	/* for ioctl */
{
	register char *wp;			/* word pointer */
	register char *cp;			/* character pointer */
	register int midx;			/* module index */
	register int npush;			/* number of modules to push */

	/*
	 * Find the major device number.
	 */
	for (wp = linep; isspace((int)*wp); wp++)
		;
	for (cp = wp; isdigit((int)*cp); cp++)
		;
	if (!isspace((int)*cp)) {
		(void) pfmt(stderr, MM_WARNING, Badline, namep, lineno);
		return (1);
	}
	pushp->sap_major = (major_t)atol(wp);

	/*
	 * Find the minor device number.  Must handle negative values here.
	 */
	for (wp = cp; isspace((int)*wp); wp++)
		;
	for (cp = wp; (isdigit((int)*cp) || (*cp == MINUS)); cp++)
		;
	if (!isspace((int)*cp)) {
		(void) pfmt(stderr, MM_WARNING, Badline, namep, lineno);
		return (1);
	}
	pushp->sap_minor = (minor_t)atol(wp);

	/*
	 * Find the lastminor.
	 */
	for (wp = cp; isspace((int)*wp); wp++)
		;
	for (cp = wp; isdigit((int)*cp); cp++)
		;
	if (!isspace((int)*cp)) {
		(void) pfmt(stderr, MM_WARNING, Badline, namep, lineno);
		return (1);
	}
	pushp->sap_lastminor = (minor_t)atol(wp);

	/*
	 * Read the list of module names.
	 */
	npush = 0;
	while ((npush < MAXAPUSH) && (*cp)) {
		while (isspace((int)*cp))
			cp++;
		for (midx = 0; ((midx < FMNAMESZ) && (*cp)); midx++)
			if (!isspace((int)*cp))
				pushp->sap_list[npush][midx] = *cp++;
		if ((midx >= FMNAMESZ) && (*cp) && !isspace((int)*cp)) {
			(void) pfmt(stderr, MM_ERROR,
				":26:File %s: module name too long, line %d ignored\n",
					namep, lineno);
			return (1);
		}
		if (midx > 0) {
			pushp->sap_list[npush][midx] = (char)0;
			npush++;
		}
	}
	pushp->sap_npush = npush;

	/*
	 * We have everything we want from the line.
	 * Now make sure there is no extra garbage on the line.
	 */
	while ((*cp) && isspace((int)*cp))
		cp++;
	if (*cp) {
		(void) pfmt(stderr, MM_ERROR,
			":27:File %s: too many modules, line %d ignored\n",
				namep, lineno);
		return (1);
	}
	return (0);
}

