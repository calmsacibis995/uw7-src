/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mkfifo:mkfifo.c	1.1.3.2"

#include <sys/types.h>
#include <sys/stat.h>

#include    <stdio.h>
#include    <errno.h>
#include    <string.h>
#include    <ctype.h>	/* for isdigit  */
#include    <stdlib.h>	/* for getopt, strtol etc */
#include    <locale.h>	/* for LC_*, setlocale, etc */
#include    <pfmt.h>	/* for pfmt, MM_*	*/

extern int mkfifo();

static const char bad_usage[] =
		":1:Incorrect usage\n";
static const char Usage[] =
	":48:Usage: mkfifo [-m mode] file ...\n";
static const char bad_mkfifo[] =
	":49:%s: %s\n";
static const char badmode[] =
	":50:Invalid mode\n";

#ifdef __STDC__
static mode_t newmode (mode_t, char *,int *, mode_t);
static mode_t who(void);
static int what(void);

static void usage(int);
#else
static mode_t newmode();
static mode_t who();
static int what();

static void usage();
#endif 

main( argc, argv )
int   argc;
char       *argv[];
{
	char *path;
	int exitval = 0;
	int retval;
	int mode_err = 0;
	int errflg = 0;
	int optc ;
	mode_t mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH ;
	mode_t cmask ;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxsysadm");
	(void)setlabel("UX:mkfifo");

	while ((optc = getopt(argc, argv, "m:")) != EOF) {
		switch (optc) {
		case 'm':
			cmask = umask(0) ;
			mode &= ~cmask ;
			mode = newmode(mode, optarg, &mode_err, cmask);
			if(mode_err)
				exit (1) ;
			break;
		case '?':
		default:
			errflg++;
			break;
		}
	}

	argc -= optind;
	argv = &argv[optind];
	if(argc < 1 || errflg) {
		usage(errflg) ;
		exit(1);
	}

        while (argc--) {
		path = *argv++;

		retval = mkfifo (path, mode);
		if (retval) {
			pfmt(stderr, MM_ERROR, bad_mkfifo,
						path, strerror(errno));
	    		exitval = 1;

		}

    	}

    	exit(exitval);
	/*NOTREACHED*/

}


static void
#ifdef __STDC__
usage(int erropt)
#else
usage(erropt)
int erropt;
#endif 
{
 	if (erropt == 0)
		pfmt(stderr, MM_ERROR, bad_usage);
	pfmt(stderr, MM_ACTION, Usage) ;
}

/*
 * Procedure: newmode
 */ 

#define	USER	00700	/* user's bits */
#define	GROUP	00070	/* group's bits */
#define	OTHER	00007	/* other's bits */
#define	ALL	00777	/* all */

#define	READ	00444	/* read permit */
#define	WRITE	00222	/* write permit */
#define	EXEC	00111	/* exec permit */

static char *msp;		/* mode string */

static mode_t
#ifdef __STDC__
newmode(mode_t nm, char *ms, int *errmode, mode_t mask)
#else
newmode(nm, ms, errmode, mask)
mode_t  nm;	/* file mode bits*/
char  *ms;	/* file mode string */
int *errmode;
mode_t mask;	/* process file creation mask */
#endif
{
	/* m contains USER|GROUP|OTHER information
	   o contains +|-|= information	
	   b contains rwx(slt) information  */
	mode_t 		m, b;
	mode_t 		m_lost;
		/* umask bit */
	register int 	o;
	char		*errflg;
	register int 	goon;
	mode_t 		om = nm;
		/* save mode for file mode incompatibility return */

	*errmode = 0;
	msp = ms;
	if (isdigit(*msp)) {
		nm = (mode_t)strtol(msp, &errflg, 8) ;
		if (*errflg != '\0') {
			pfmt(stderr, MM_ERROR, badmode) ;
			*errmode = 5 ;
			return(om);
		}
		return(nm & ALL);
	}
	do {
		m = who();
		/* P2D11/4.7.7*/
		/* if who is not specified */
		if(m == 0) {
			m = ALL & ~mask ;
			/*
		 	 * The m_lost is supplementing umask bits
			 * for '=' operation.
			 */
			m_lost = mask ;
		} else
			m_lost = 0 ;
			
		while (o = what()) {
/*		
	this section processes permissions
*/
			b = 0;
			goon = 0;
			switch (*msp) {
			case 'u':
				b = (nm & USER) >> 6;
				goto dup;
			case 'g':
				b = (nm & GROUP) >> 3;
				goto dup;
			case 'o':
				b = (nm & OTHER);
		    dup:
				b &= (READ|WRITE|EXEC);
				b |= (b << 3) | (b << 6);
				msp++;
				goon = 1;
			}
			while (goon == 0) switch (*msp++) {
			case 'r':
				b |= READ;
				continue;
			case 'w':
				b |= WRITE;
				continue;
			case 'X':
				if(!S_ISDIR(om) && ((om & EXEC) == 0))
					continue;
				/* FALLTHRU */

			case 'x':
				b |= EXEC;
				continue;
			case 'l':
				/* ignore LOCK */
				continue;
			case 's':
				/* ignore SETID */
				continue;
			case 't':
				/* ignore STICKY */
				continue;
			default:
				msp--;
				goon = 1;
			}

			b &= m;

			switch (o) {
			case '+':
				/* create new mode */
				nm |= b;
				break;
			case '-':

				/* create new mode */
				nm &= ~b;
				break;
			case '=':

				/* create new mode */
				nm &= ~(m|m_lost);
				nm |= b;
				break;
			}
		}
	} while (*msp++ == ',');
	if (*--msp) {
		pfmt(stderr, MM_ERROR, badmode) ;
		*errmode = 5 ;
		return(om);
	}
	return(nm);
}

static mode_t
#ifdef __STDC__
who(void)
#else
who()
#endif
{
	register mode_t m;

	m = 0;
	for (;; msp++) switch (*msp) {
	case 'u':
		m |= USER;
		continue;
	case 'g':
		m |= GROUP;
		continue;
	case 'o':
		m |= OTHER;
		continue;
	case 'a':
		m |= ALL;
		continue;
	default:
		return m;
	}
}

static int
#ifdef __STDC__
what(void)
#else
what()
#endif
{
	switch (*msp) {
	case '+':
	case '-':
	case '=':
		return *msp++;
	}
	return(0);
}

