/*	copyright	"%c%"	*/

#ident	"@(#)head:head.c	1.1.6.1"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#include <stdio.h>
#include <ctype.h>
#include <locale.h>
#include <stdlib.h>
#include <pfmt.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

/*
 * head - give the first few lines of a stream or of each of a set of files
 *
 */

static int	linecnt	= 10;

static const char
	errnum_string[] =
		":56:%s: %s\n";
static const char
	usage[]	=
		":90:Usage: head [-n number] [filename...]\n"
		        "\thead [-number] [filename...]\n";
static const char
	badnum[] =
		":91:Bad number: %s\n";
static const char
	default_message_catalog[] =
		"uxdfm";
static const char
	pfmt_label[] =
		"UX:head";

static void	copyout(int);
static int	getnum(char *);

main(Argc, argv)
	int Argc;
	char *argv[];
{
	static int around;
	register int argc;
	int errcode = 0;
	char *name;
	int c;

	(void)setlocale(LC_ALL, "");
	(void)setlabel(pfmt_label);
	(void)setcat(default_message_catalog);

	if (argv[1] && argv[1][0] == '-' && 
	    argv[1][1] && isdigit((int)argv[1][1])) {
		argv++, Argc--;
 		while (Argc > 0 && argv[0][0] == '-') {
			if (argv[0][1] == '-') {
				argv++, Argc--;
				break;
			} else {
				linecnt = getnum(argv[0] + 1);
				argv++, Argc--;
			}
 		}
	} else {
		while ((c = getopt(Argc, argv, "n:")) != EOF) {
			switch (c) {
			case 'n' :
				linecnt = getnum(optarg);
				break;
			default :
				(void)pfmt(stderr, MM_ACTION, usage);
				exit(1);
				break;
			}
		}
		Argc -= optind;
		argv += optind;
	}
	argc = Argc;
	do {
		if (argc == 0 && around)
			break;
		if (argc > 0) {
			(void)close(0);
			if (freopen(argv[0], "r", stdin) == NULL) {
				(void)pfmt(stderr, MM_ERROR, errnum_string,
					argv[0], strerror(errno));
				errcode++;
				argc--, argv++;
				continue;
			}
			name = argv[0];
			argc--, argv++;
		} else
			name = 0;
		if (around)
			(void)putchar('\n');
		around++;
		if (Argc > 1 && name)
			(void)printf("==> %s <==\n", name);
		copyout(linecnt);
		(void)fflush(stdout);
	} while (argc > 0);
	return(errcode);
}

static void
copyout(cnt)
	register int cnt;
{
	int	c;

	while (cnt > 0) {
		do {
			if ((c = getchar()) == EOF)
				return;

			(void)putchar(c);
		} while (c != '\n');
 		(void)fflush(stdout);
		cnt--;
	}
}

static int
getnum(cp)
	register char *cp;
{
	register int i;
	char *tailp;

	i = (int)strtol(cp, &tailp, 10);
	if ((*tailp != '\0') || (i < 0)) {
		(void)pfmt(stderr, MM_ERROR, badnum, cp);
		(void)pfmt(stderr, MM_ACTION, usage);
		exit(1);
	}
	return (i);
}
