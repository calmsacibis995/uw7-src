/*	copyright	"%c%"	*/

#ident	"@(#)cat:cat.c	1.17.3.4"
/*
 * 	Concatenate files.
 */

/***************************************************************************
 * Command: cat
 * Inheritable Privileges: P_DACREAD,P_MACREAD
 *       Fixed Privileges: None
 * Notes: concatenate and print files
 *
 ***************************************************************************/

#include	<stdio.h>
#include	<ctype.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/euc.h>
#include	<locale.h>
#include	<pfmt.h>
#include	<errno.h>
#include	<string.h>

#define	IDENTICAL(A,B)	(A.st_dev==B.st_dev && A.st_ino==B.st_ino)

char	buffer[BUFSIZ];

int	silent = 0;		/* s flag */
int	visi_mode = 0;		/* v flag */
int	visi_tab = 0;		/* t flag */
int	visi_newline = 0;	/* e flag */
int	errnbr = 0;

eucwidth_t	wp;

char badwrite[] = ":1:Write error: %s\n";

char	stdin_file[] = "stdin";

/*
 * Procedure:     main
 *
 * Restrictions:
                 setlocale:	none
                 getopt:	none
                 pfmt:	none
                 fopen:	none
 */
main(argc, argv)
int    argc;
char **argv;
{
	register FILE *fi;
	register int c;
	extern	int optind;
	int	errflg = 0;
	int	stdinflg = 0;
	int	status = 0;
	struct stat source, target;
	char	*fname;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:cat");


	getwidth(&wp);

#ifdef STANDALONE
	/*
	 * If the first argument is NULL,
	 * discard arguments until we find cat.
	 */
	if (argv[0][0] == '\0')
		argc = getargv ("cat", &argv, 0);
#endif

	/*
	 * Process the options for cat.
	 */

	while( (c=getopt(argc,argv,"usvte")) != EOF ) {
		switch(c) {

		case 'u':

			/*
			 * If not standalone, set stdout to
	 		 * completely unbuffered I/O when
			 * the 'u' option is used.
			 */

#ifndef	STANDALONE
			setbuf(stdout, (char *)NULL);
#endif
			continue;

		case 's':
		
			/*
			 * The 's' option requests silent mode
			 * where no messages are written.
			 */

			silent++;
			continue;

		case 'v':
			
			/*
			 * The 'v' option requests that non-printing
			 * characters (with the exception of newlines,
			 * form-feeds, and tabs) be displayed visibly.
			 *
			 * Control characters are printed as "^x".
			 * DEL characters are printed as "^?".
			 * Non-printable  and non-contrlol characters with the
			 * 8th bit set are printed as "M-x".
			 */

			visi_mode++;
			continue;

		case 't':

			/*
			 * When in visi_mode, this option causes tabs
			 * to be displayed as "^I" and form-feeds
			 * to be displayed as "^L".
			 */

			visi_tab++;
			continue;

		case 'e':

			/*
			 * When in visi_mode, this option causes newlines
			 * to be displayed as "$" at the end
			 * of the line prior to the newline.
			 */

			visi_newline++;
			continue;

		case '?':
			errflg++;
			break;
		}
		break;
	}

	if (errflg) {
		if (!silent)
			pfmt(stderr, MM_ACTION, ":2:Usage: cat -usvte [-|file] ...\n");
		exit(2);
	}

	/*
	 * Stat stdout to be sure it is defined.
	 */

	if(fstat(fileno(stdout), &target) < 0) {
		if(!silent)
			pfmt(stderr, MM_ERROR, ":3:Cannot access stdout: %s\n",
				strerror(errno));
		exit(2);
	}

	/*
	 * If no arguments given, then use stdin for input.
	 */

	if (optind == argc) {
		argc++;
		stdinflg++;
	}

	/*
	 * Process each remaining argument,
	 * unless there is an error with stdout.
	 */


	for (argv = &argv[optind];
	     optind < argc && !ferror(stdout); optind++, argv++) {

		/*
		 * If the argument was '-' or there were no files
		 * specified, take the input from stdin.
		 */

		if (stdinflg
		 ||((*argv)[0]=='-' 
		 && (*argv)[1]=='\0')) {
			fi = stdin;
			fname = stdin_file;
		}
		else {
			/*
			 * Attempt to open each specified file.
			 */

			if ((fi = fopen(*argv, "r")) == NULL) {
				if (!silent)
				   pfmt(stderr, MM_ERROR, ":4:Cannot open %s: %s\n",
					*argv, strerror(errno));
				status = 2;
				continue;
			}
			else
				fname = *argv;
		}
		
		/*
		 * Stat source to make sure it is defined.
		 */

		if(fstat(fileno(fi), &source) < 0) {
			if(!silent)
			   pfmt(stderr, MM_ERROR, ":5:Cannot access %s: %s\n",
			   	fname, strerror(errno));
			status = 2;
			continue;
		}

		/*
		 * If the source is not a character special file or a
		 * block special file, make sure it is not identical
		 * to the target.
		 */
	
		if ((S_ISREG(target.st_mode) || S_ISFIFO(target.st_mode))
		 && IDENTICAL(target, source)) {
			if(!silent)
			   pfmt(stderr, MM_ERROR,
			   	":6:Input/output files '%s' identical\n",
				stdinflg?"-": *argv);
			if (fclose(fi) != 0 )
				pfmt(stderr, MM_ERROR, badwrite, strerror(errno));
			status = 2;
			continue;
		}

		/*
		 * If in visible mode, use vcat; otherwise, use cat.
		 */

		if (visi_mode) {
			if (vcat(fi) != 0) {
				status = 2;
			}
		} else {
			if (cat(fi, fname) != 0) {
				status = 2;
			}
		}

		/*
		 * If the input is not stdin, flush stdout.
		 */

		if (fi!=stdin) {
			fflush(stdout);
			
			/* 
			 * Attempt to close the source file.
			 */

			if (fclose(fi) != 0) 
				if (!silent)
					pfmt(stderr, MM_ERROR, badwrite, strerror(errno));
		}
	}
	
	/*
	 * When all done, flush stdout to make sure data was written.
	 */

	fflush(stdout);
	
	/*
	 * Display any error with stdout operations.
	 */

	if (errnbr = ferror(stdout)) {
		if (!silent)
			pfmt(stderr, MM_ERROR, badwrite, strerror(errno));
		status = 2;
	}
	exit(status);
}

/*
 * Procedure:     cat
 *
 * Restrictions:
                 read(2):	none
                 pfmt:	none
 */
int
cat(fi, fname)
FILE *fi;
char *fname;
{
	register int fi_desc;
	register int nitems;

	fi_desc = fileno(fi);

	/*
	 * While not end of file, copy blocks to stdout. 
	 */

	while ((nitems=read(fi_desc,buffer,BUFSIZ))  > 0) {
		if ((errnbr = write(1,buffer,(unsigned)nitems)) != nitems) {
			if (!silent) {
				if (errnbr == -1) {
					errnbr = 0;
					pfmt(stderr, MM_ERROR,
						":7:Write error (%d/%d characters written): %s\n",
						errnbr, nitems, strerror(errno));
				}
				else
					if (errno == 0)
						pfmt(stderr, MM_ERROR,
						":7:Write error (%d/%d characters written): %s\n",
						errnbr, nitems, strerror(EFBIG));
			}
			return(2);
		}
	}

	if (nitems == -1 && errno && !silent) {
		(void)pfmt(stderr, MM_ERROR,
			":1082:Read error on file '%s': %s\n", 
						fname, strerror(errno));
		return(2);
	}
	return(0);
}


vcat(fi)
FILE 	*fi;
{
	register int c;

	while ((c = getc(fi)) != EOF) 
	{
		/*
		 * For non-printable and non-cntrl  chars, use the "M-x" notation.
		 */
		if (!ISPRINT(c, wp) &&
		    !iscntrl(c) && !ISSET2(c) && !ISSET3(c))
			{
			putchar('M');
			putchar('-');
			c-= 0200;
			}
		/*
		 * Display plain characters
		 */
		 if (ISPRINT(c, wp) && (!iscntrl(c) || ISSET2(c) || ISSET3(c)))
			{
			putchar(c);
			continue;
			}
		/*
		 * Display tab as "^I" if visi_tab set
		 *
		 * Display form-feed as "^L" if visi_tab set
		 */

		if ( (c == '\t') || (c == '\f') )
			{
			if (! visi_tab)
				putchar(c);
			else
				{
				putchar('^');
				putchar(c^0100);
				}
			continue;
			}
		/*
		 * Display newlines as "$<newline>"
		 * if visi_newline set
		 */
		if ( c == '\n')
			{
			if (visi_newline) 
				putchar('$');
			putchar(c);
			continue;
			}
		/*
		 * Display control characters
		 */
		if ( c <  0200 )
			{
			putchar('^');
			putchar(c^0100);
			}
		else
			{
			putchar('M');
			putchar('-');
			putchar('x');
			}
	}

	/*
	 * Check that file reading ended cleanly.
	 */

	if (ferror(fi)) {
		if (!silent)
			pfmt(stderr, MM_ERROR, ":1207:Error reading file: %s\n",
				strerror(errno));
		return(2);
	}

	return(0);
}
