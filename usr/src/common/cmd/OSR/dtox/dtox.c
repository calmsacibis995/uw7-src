#ident	"@(#)OSRcmds:dtox/dtox.c	1.1"
#pragma comment(exestr, "@(#) dtox.c 26.1 95/08/07 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1986-1995.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

/*
 * History:
 *
 * 3/1/86	Brian Raney	Creation
 *
 *  L001   4 Dec 1990      scol!markhe
 *         Altered definition of ^Z from 026 to 032,
 *         since this constant is in octal.
 *  L002   4 Dec 1990      scol!markhe
 *         Altered cast of constant 0 from (char *) to (FILE *),
 *         since this caused a lint warning message.
 *  L003   5 Dec 1990      scol!markhe
 *         Void casts added to functions whose return value is not used,
 *         since this caused lint warning messages.
 *  S004   brianm@sco.com  Thu Apr 09 10:18:38 PDT 1992
 *         - added in a print of a control Z. requires detailed knowledge
 *           or ASCII character layout.  (FROM xtod.c S001)
 *  S005   23 Nov 1993     sco!belal
 *	   - Changed write(1,&ch,1) to putchar(ch) for performance (50:1
 *	     improvement in a simple test on my machine). xtod was already
 *	     using putchar and was much faster than dtox.
 *	   - Merged dtox and xtod since they are so similar.
 *	   - Changes are pervasive and unmarked.
 *  L006   02 Aug 1995     scol!ianw
 *	   - Don't choose behavior based on last character of argv[0], instead
 *	     only select xtod if the filename part of argv[0] is xtod.
 *	   - Changed function definitions into prototype form (not marked).
 *	   - Silenced some lint warnings (unmarked).
 */

/*
 * dtox [filename]
 * xtod [filename]
 *
 * Takes an optional argument as input, standard in if not present.
 *
 * dtox copies the input file to standard output, changing DOS file
 * format to UNIX file format: CR-LF pairs are converted to just LF,
 * and control-Z's are eliminated.
 *
 * xtod copies the input file to standard output, changing UNIX file
 * format to DOS file format: LF's are converted to CR-LF.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

static void usage(void);

#define		CNTLZ		'\032'        /* L001 */

char		*command = "dtox";				/* L006 */

main(int argc, char *argv[])
{
	int	ch1, ch2;
	int	xtod = 0, dtox = 0;
	FILE	*instream;

	/* default to dtox if argv[0] is not set */
	if (argc > 0)						/* L006 */
		command = argv[0];
								/* L006 begin */
	/* default to dtox if the filename part of argv[0] is not xtod */
	if (strcmp(basename(command), "xtod") == 0)
		xtod = 1;
	else
		dtox = 1;					/* L006 end */
	
	if (argc > 2)
		usage();

	if (argc == 1)
		instream = stdin;
	else
		if ((instream = fopen(argv[1],"r")) == (FILE *)0)  /* L002 */
		{
			/* L003 */
			(void) printf("%s: cannot open %s.\n",command, argv[1]);
			exit(1);
		}

#ifdef DOS
	setmode(1, O_BINARY);
#endif /* DOS */

	/* all set, do copy */

	while( ((ch1 = getc(instream)) != EOF) && ((ch1 != CNTLZ) || xtod) )
	{
		if (dtox && (ch1 == '\r'))
		{
			if ((ch2 = getc(instream)) != '\n')
				putchar(ch1);			/* L003 */
			(void) ungetc(ch2, instream);		/* L003 */
		}
		else
		{
			if (xtod && (ch1 == '\n'))
				putchar('\r');
			putchar(ch1);
		}
	}

	if (xtod)
		putchar(CNTLZ);			/* S004 print control Z */

	(void) fclose(instream);				/* L003 */
	exit(0);
	/* NOTREACHED */
}

static void
usage(void)
{
	(void) fprintf(stderr,"usage: %s [file]\n",command);	/* L003 */
	exit(1);
	/* NOTREACHED */
}
