/*		copyright	"%c%" 	*/

/*	Portions Copyright (c) 1988, Sun Microsystems, Inc.	*/
/*	All Rights Reserved. 					*/

#ident	"@(#)touch:touch.c	1.11.3.2"
#ident "$Header$"
/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <time.h>
#include <unistd.h>
#include <locale.h>
#include <pfmt.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

struct	stat	stbuf;
int	status;

char	*tp;	/* pointer to -t's argument or date_time operand */
time_t	timbuf;

static char posix_var[] = "POSIX2";
static int posix;
static int length = 0;

/*
 * gtime(topt)
 * 
 * returns:
 *	0 - success
 *	1 - baddate error
 * 	2 - usage error
 *
 * NOTE:  The gtime() routine is also used in the "at" command.
 *        Any changes made here should probably also be made in 
 *        the "at" code to keep them in sync.
 */
gtime(int topt)
{
	register int	i;
	register char   *temp;
	struct   tm	time_str;
	time_t		nt;
	int		time_len, dotflg=0;

	time_len = length;

	if (topt) {
		if (strchr(tp, (int)'.') != (char *)NULL) {
			dotflg++;
			time_len -= 3;
		}
	}

	switch (time_len) {

	case 12:	/* CCYYMMDDhhmm[.SS] */
		i = gpair();
		if ((i < 19) || (i > 20)) 	{
			return (1);
		} else {
			time_str.tm_year = (i - 19) * 100 + gpair();
		}
		break;

	case 10:	/* YYMMDDhhmm[.SS] or MMDDhhmmYY */
		/* If date_time, temporarily bump pointer over to year. */
		if (!topt) {
			temp = tp;
			tp += 8;
		}

		i =  gpair();
		if ((i >= 0) && (i <= 68)) {
			time_str.tm_year = 100 + i;
		} else if ((i >= 69) && (i <= 99)) {
			time_str.tm_year = i;
		} else {
			return (1);
		}

		/* If date_time, reset pointer back to where we were. */
		if (!topt)
			tp = temp;

		break;

	case 8:		/* MMDDhhmm[.SS] or MMDDhhmm*/
		(void)time(&nt);
		time_str.tm_year = localtime(&nt)->tm_year;
		break;

	default:
		return (2);
	}

	time_str.tm_mon = gpair() - 1;
	if ((time_str.tm_mon < 0) || (time_str.tm_mon > 11)) {
		return (1);
	}

	time_str.tm_mday = gpair();
	if ((time_str.tm_mday < 1) || (time_str.tm_mday > 31)) {
		return (1);
	}

	time_str.tm_hour = gpair();
	if ((time_str.tm_hour < 0) || (time_str.tm_hour > 23)) {
		return (1);
	}

	time_str.tm_min = gpair();
	if ((time_str.tm_min < 0) || (time_str.tm_min > 59)) {
		return (1);
	}

	if (dotflg) {
		if (*tp == '.') {
			tp++;
			time_str.tm_sec = gpair();
			if ((time_str.tm_sec < 0) || (time_str.tm_sec > 61))
				return (1);
		}
		else  /* dot in wrong place */
			return(2);

	} 
	else  /* either -t without [.SS] or date_time operand */
		time_str.tm_sec = 0;

	time_str.tm_isdst = -1;

	if ((timbuf = mktime(&time_str)) < (time_t)0) {
		/* mktime() failed or resulting time precedes Epoch. */
		return (1);
	} 
	else
		return (0);
}

int
gpair()
{
	register char *cp;
	register char *bufp;
	char buf[3];

	cp = tp;
	bufp = buf;
	if (isdigit(*cp)) {
		*bufp++ = *cp++;
	} else {
		return (-1);
	}
	if (isdigit(*cp)) {
		*bufp++ = *cp++;
	} else {
		return (-1);
	}
	*bufp = '\0';
	tp = cp;
	return ((int)strtol(buf, (char **)NULL, 10));
}

main(argc, argv)
char *argv[];
{
	register c;
	struct utbuf { time_t actime, modtime; } times;

	int mflg=1, aflg=1, cflg=0;
	int rflg=0, tflg=0, dtflg=0, stflg=0, fflg=0, nflg=0, errflg=0; 
	int optc, fd;
	register int ret;
	char *proto;
	char *ref_file;
	struct  stat prstbuf; 
	char *baddate = ":585:Bad date conversion\n";
	char *usage = ":583:Usage: %s [%s] [mmddhhmm[yy]] file ...\n";
	char *tusage = "-amc";
	char *posix_usage =
	    ":1201:Usage: %s [%s]"
	    " [-r ref_file | -t [[CC]YY]MMDDhhmm[.SS]] file ...\n"
	    "\t\t\t%s [%s] [MMDDhhmm[YY]] file ...\n";
	char *susage = "-f file";
	char *susageid = ":584";
	extern char *optarg, *basename();
	extern int optind;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");

	if (getenv(posix_var) != 0)	{
		posix = 1;
	} else	{
		posix = 0;
	}

	argv[0] = basename(argv[0]);
	if (!strcmp(argv[0], "settime")) {
		(void)setlabel("UX:settime");
		while ((optc = getopt(argc, argv, "f:")) != EOF)
			switch (optc) {
			case 'f':
				fflg++;
				proto = optarg;
				break;
			default:
				errflg++;
				break;
			};
		stflg = 1;
		++cflg;
	} 
	else {
		(void)setlabel("UX:touch");
		while ((optc=getopt(argc, argv, "amcfr:t:")) != EOF)
			switch(optc) {
			case 'm':
				mflg++;
				aflg--;
				break;
			case 'a':
				aflg++;
				mflg--;
				break;
			case 'c':
				cflg++;
				break;
			case 'f':
				break;		/* silently ignore   */ 
			case 'r':
				rflg++;
				ref_file = optarg;
				break;
			case 't':
				tflg++;
				tp = optarg;
				length = (int) strlen(tp);
				break;
			case '?':
				errflg++;
			}
	}

	if(((argc-optind) < 1) || errflg || (rflg && tflg)) {
		if (!errflg)
			pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
		if (stflg) {
			(void) pfmt(stderr, MM_ACTION, usage, argv[0],
				gettxt(susageid, susage));
		} else {
			(void) pfmt(stderr, MM_ACTION, posix_usage,
				argv[0], tusage, argv[0], tusage);
		}
		exit(2);

	}

	/*
	 * If no -r, -t, or -f, and at least 2 operands, and first 
	 * is 8- or 10-digit number, assume date_time operand.
	 */
	if ( (!tflg && !rflg && !fflg) && 
		((argc - optind) >= 2) &&  (isnumber(argv[optind])) &&
		(((length = (int)strlen(argv[optind])) == 8) || (length == 10)) ) {
		dtflg++;
		tp = (char *)argv[optind++];
	}

	status = 0;
	if (fflg) {
		if (stat(proto, &prstbuf) == -1) {
			pfmt(stderr, MM_ERROR, ":12:%s: %s\n", proto,
				strerror(errno));
			exit(2);
		}
	}
	else if (rflg) {
			if (stat(ref_file, &prstbuf) == -1) {
				pfmt(stderr, MM_ERROR, ":12:%s: %s\n",
					ref_file, strerror(errno));
				exit(2);
			}
	}

	else if (tflg || dtflg) {
		if ((ret=gtime(tflg)) != 0) {
			if (ret == 1)
				(void) pfmt(stderr, MM_ERROR, baddate);
			else {
				(void) pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
				(void) pfmt(stderr, MM_ACTION, posix_usage,
					argv[0], tusage, argv[0], tusage);
			}
			exit(2);
		}

		prstbuf.st_mtime = prstbuf.st_atime = timbuf;
	}

	else {  /* first operand is file operand; current time is assumed */
		if((aflg <= 0) || (mflg <= 0))
                        prstbuf.st_atime = prstbuf.st_mtime = time((long *) 0);
		else
			nflg++;
	}

	for(c=optind; c<argc; c++) {
		if(stat(argv[c], &stbuf)) {
			if(cflg) {
				if (!posix)
					status++;
				continue;
			}
			else if ((fd = creat (argv[c], (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH))) < 0) {
				(void) pfmt(stderr, MM_ERROR, 
					":148:Cannot create %s: %s\n", 
					argv[c], strerror(errno));
				status++;
				continue;
			}
			else {
				(void) close(fd);
				if(stat(argv[c], &stbuf)) {
					(void) pfmt(stderr, MM_ERROR,
						":5:Cannot access %s: %s\n",
						argv[c], strerror(errno));
					status++;
					continue;
				}
			}
		}
		times.actime = prstbuf.st_atime;
		times.modtime = prstbuf.st_mtime;
		if (mflg <= 0)
			times.modtime = stbuf.st_mtime;
		if (aflg <= 0)
			times.actime = stbuf.st_atime;

		if(utime(argv[c], (struct utbuf *)(nflg? 0: &times))) {
			(void) pfmt(stderr, MM_ERROR,
				":586:Cannot change times on %s: %s\n",
				argv[c], strerror(errno));
			status++;
			continue;
		}
	}
	exit(status);	/*NOTREACHED*/
}

isnumber(s)
char *s;
{
	register c;

	while((c = *s++) != '\0')
		if(!isdigit(c))
			return(0);
	return(1);
}

