#ident  "@(#)demangler:common/dem_main.c	1.10"
/*ident	"@(#)cls4:tools/demangler/dem.c	1.13" */
/*******************************************************************************
C++ source for the C++ Language System, Release 3.0.  This product
is a new release of the original cfront developed in the computer
science research center of AT&T Bell Laboratories.

Copyright (c) 1993  UNIX System Laboratories, Inc.
Copyright (c) 1991, 1992 AT&T and UNIX System Laboratories, Inc.
Copyright (c) 1984, 1989, 1990 AT&T.  All Rights Reserved.

THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE of AT&T and UNIX System
Laboratories, Inc.  The copyright notice above does not evidence
any actual or intended publication of such source code.

*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <dem.h>
#include <locale.h>
#include <unistd.h>
#include <pfmt.h>
#include "sgs.h"

#define SKIP_WS  1
#define SKIP_ALL 2

#define MAXLINE 4096
/* process one file */
static void dofile(FILE* fp, int del)
{
	char buf[MAXLINE];
	char buf2[MAXLINE];
	char* s;
	char* start;
	char c;
	int x;

	while (fgets(buf, MAXLINE, fp) != NULL) {
		s = buf;
		for (;;) {

			/* skip whitespace */

			if (del == SKIP_WS) {
				while (*s && *s <= ' ') {
					putchar(*s);
					s++;
				}
			}
			else {
				while (*s && !isalnum(*s) && *s != '_') {
					putchar(*s);
					s++;
				}
			}
			if (!*s)
				break;
			start = s;

			/* unmangle one name in place */

			if (del == SKIP_WS) {
				while (*s && *s > ' ')
					s++;
			}
			else {
				while (*s && (isalnum(*s) || *s == '_'))
					s++;
			}
			c = *s;
			*s = 0;
			x = demangle(start, buf2, MAXLINE);
			if (x < 0 || x > MAXLINE)
				strcpy(buf2, start);
			printf("%s", buf2);
			*s = c;
		}
	}
}

main(int argc, char* argv[])
{
	int nerr = 0;
	FILE* fp;
	int del = SKIP_ALL;
	int Vflag = 0;
	int c;

	setlocale(LC_ALL, "");
	setcat("uxcplusplus");
	setlabel("c++filt");

	while ((c = getopt(argc, argv, "Vw")) != EOF)
		switch (c) {
		case 'V':
			if (!Vflag)
				pfmt(stderr, MM_INFO, ":18:%s %s\n", CPLUS_PKG, CPLUS_REL);
			Vflag++;
			break;
		case 'w':
			del = SKIP_WS;
			break;
		case '?':
			pfmt(stderr, MM_ERROR, ":25:Unrecognized option: %c\n", optopt);
			nerr++;
			break;
		}

	/* standard input */

	if (optind == argc) {
		dofile(stdin, del);
	}

	/* else iterate over all files */

	else {
		for ( ; optind < argc; optind++) {
			if ((fp = fopen(argv[optind], "r")) == NULL) {
				pfmt(stderr, MM_ERROR, ":3:cannot open %s for reading\n", argv[optind]);
				nerr++;
				continue;
			}
			dofile(fp, del);
			fclose(fp);
		}
	}

	exit(nerr);
}
