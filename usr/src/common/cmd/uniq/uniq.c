/*		copyright	"%c%" 	*/

#ident	"@(#)uniq:uniq.c	1.4.3.3"

/* COPYRIGHT NOTICE
 *
 * This source code is designated as Restricted Confidential Information
 * and is subject to special restrictions in a confidential disclosure
 * agreement between HP, IBM, SUN, NOVELL and OSF.  Do not distribute
 * this source code outside your company without OSF's specific written
 * approval.  This source code, and all copies and derivative works
 * thereof, must be returned or destroyed at request. You must retain
 * this notice on any copies which you make.
 *
 * (c) Copyright 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC.
 * ALL RIGHTS RESERVED
 */
/*
 * OSF/1 1.2
 */
#if !defined(lint) && !defined(_NOIDENT)
/* static char rcsid[] = "@(#)$RCSfile$ $Revision$ (OSF) $Date$"; */
#endif   
/*
 * COMPONENT_NAME: (CMDFILES) commands that manipulate files
 *
 * FUNCTIONS: uniq
 *
 *
 * (C) COPYRIGHT International Business Machines Corp. 1985, 1989, 1991
 * All Rights Reserved
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 *
 * 1.10  com/cmd/files/uniq.c, bos320,9130320g 7/23/91 10:02:27
 */

/*
 * Deal with duplicated lines in a file
 */

#include <stdio.h>
#include <ctype.h>
#include <locale.h>
#include <pfmt.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#define _XOPEN_SOURCE
#include <wchar.h>

#ifndef	LINE_MAX	/* in case of LINE_MAX not defined */
#define	LINE_MAX	2048
#endif

static int	fields = 0;
static int	letters = 0;
static int	linec;
static char	mode;
static int	uniq;

#define	NEW	1		/* New synopsis */
#define	OLD	-1		/* Obsolescent version */

static char oldinv[] =
	":126:Invalid option for obsolescent synopsis\n";
static char usage[] =
	":127:Usage:\n"
	"\tuniq [-c|-d|-u] [-f fields] [-s chars] [input [output]]\n"
	"\tuniq [-c|-d|-u] [+n] [-n] [input [output]]\n";
static char linetoolong[] =
	":128:Line too long\n";
static char badusg[] =
	":93:Invalid argument to option -%c\n";

static char	*skip();
static void newsynopsis();
static int  gline();
static void pline();
static int  equal();
static void printe();


main(argc, argv)
int argc;
char *argv[];
{
	static char b1[LINE_MAX], b2[LINE_MAX];
	FILE *temp;
	int version = 0;
	int sargc = argc;
	char **sargv = argv;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxdfm");
	(void)setlabel("UX:uniq");

	while(argc > 1) {
		if(*argv[1] == '+') {
			version = OLD;
			letters = atoi(&argv[1][1]);
			argc--;
			argv++;
			continue;
		}
		if(*argv[1] == '-') {
			switch (argv[1][1]) {
			case '\0':	/* stdin */
				break;

			case '-':	/* end of options */
				argc--; argv++;
				break;

			case 'c':
			case 'd':
			case 'u':
				mode = argv[1][1];
				argc--; argv++;
				continue;

			default:
				if (isdigit(argv[1][1])) {
					fields = atoi(&argv[1][1]);
					version = OLD;
					argc--; argv++;
					continue;
				} else {
					if (version == OLD) {
						(void)pfmt(stderr, MM_ERROR,
							oldinv);
						exit(1);
					} else
						version = NEW;
				}
			}
			break;
		}
		break;
	}
	if (version == NEW)
		newsynopsis(sargc, sargv);
	else {
		if (argc > 1)
			if (strcmp(argv[1], "-") != 0) {
				if ( (temp = fopen(argv[1], "r")) == NULL)
					printe(":3:Cannot open %s: %s\n", argv[1],
						strerror(errno));
				else {  (void) fclose(temp);
					(void) freopen(argv[1], "r", stdin);
				     }
			}
		if(argc > 2 && freopen(argv[2], "w", stdout) == NULL)
			printe(":12:Cannot create %s: %s\n", argv[2],
				strerror(errno));
	}

	if(gline(b1))
		exit(0);
	for(;;) {
		linec++;
		if(gline(b2)) {
			pline(b1);
			exit(0);
		}
		if(!equal(b1, b2)) {
			pline(b1);
			linec = 0;
			do {
				linec++;
				if(gline(b1)) {
					pline(b2);
					exit(0);
				}
			} while(equal(b1, b2));
			pline(b2);
			linec = 0;
		}
	}
}

static int
gline(buf)
register char buf[];
{
	register c;
	register char *bp = buf;

	while((c = getchar()) != '\n') {
		if(c == EOF)
			return(1);
		if (bp >= buf + (LINE_MAX - 1)) {
			(void)pfmt(stderr, MM_ERROR, linetoolong);
			exit(1);
		}
		*bp++ = (char) c;
	}
	*bp = 0;
	return(0);
}

static void
pline(buf)
register char buf[];
{

	switch(mode) {

	case 'u':
		if(uniq) {
			uniq = 0;
			return;
		}
		break;

	case 'd':
		if(uniq) break;
		return;

	case 'c':
		(void) printf("%d ", linec);
	}
	uniq = 0;
	(void) fputs(buf, stdout);
	(void) putchar('\n');
}

static int
equal(b1, b2)
register char b1[], b2[];
{
	register char c;

	b1 = skip(b1);
	b2 = skip(b2);
	while((c = *b1++) != 0)
		if(c != *b2++) return(0);
	if(*b2 != 0)
		return(0);
	uniq++;
	return(1);
}

/*
 * Return a wide character from a string of multibyte characters,
 * or return <= 0 on error/EOS.
 */
static int
Getwc(wchar_t *wcp, const char *s) {
int len;
  len = mbtowc(wcp, s, MB_CUR_MAX);
  return (*wcp == L'\0') ? 0 : len;
}

static char *
skip(s)
register char *s;
{
	register nf, nl;
	static wctype_t blank = 0;
	int len;
	wchar_t wc;

	if (blank == 0) 
		blank = wctype("blank");

	nf = nl = 0;
	if (MB_CUR_MAX> 1) {
		while (nf++ < fields) {
			do {
				if ((len = Getwc(&wc,s)) <= 0)
					return s;
				s += len;
			} while (iswctype(wc,blank));
			do {
				if ((len = Getwc(&wc,s)) <= 0)
					return s;
				s += len;
			} while (!iswctype(wc,blank));
		}
		while (nl++ < letters && (len = Getwc(&wc,s)) > 0)
			s += len;
	} else {
		while(nf++ < fields) {
			while( iswctype(*s, blank) )
				s++;
			while( !iswctype(*s, blank) && *s != 0 )
				s++;
		}
		while(nl++ < letters && *s != 0)
			s++;
	}
	return s ;
}

static void
printe(p,s, s2)
char *p,*s, *s2;
{
	(void)pfmt(stderr, MM_ERROR, p, s, s2);
	exit(1);
}

static void
newsynopsis(argc, argv)
char *argv[];
{
	extern char *optarg;
	extern int optind;
	int c;
	char *cp;
	FILE *temp;

	while ((c = getopt(argc, argv, "cduf:s:")) != EOF)
		switch (c) {
		case 'c':
		case 'd':
		case 'u':
			mode =(char) c;
			break;

		case 'f':
			fields = (int) strtol(optarg, &cp, 10);
			if (fields < 0 || *cp != 0) {
				(void)pfmt(stderr, MM_ERROR, badusg, c);
				goto pusage;
			}
			break;

		case 's':
			letters = (int) strtol(optarg, &cp, 10);
			if (letters < 0 || *cp != 0) {
				(void)pfmt(stderr, MM_ERROR, badusg, c);
				goto pusage;
			}
			break;

		case '?':
		pusage:
			(void)pfmt(stderr, MM_ACTION, usage);
			exit(1);
		}

	if (argc - optind >= 1) {
		if (strcmp(argv[optind], "-") != 0) {
			if ( (temp = fopen(argv[optind], "r")) == NULL)
				printe(":3:Cannot open %s: %s\n",
					argv[optind], strerror(errno));
			else {  (void) fclose(temp);
				(void) freopen(argv[optind], "r", stdin);
			     }
		}
	}

	if(argc - optind > 1
		&& freopen(argv[optind+1], "w", stdout) == NULL)
		printe(":12:Cannot create %s: %s\n", argv[optind+1],
			strerror(errno));
}
