#ident	"@(#)OSRcmds:more/main.c	1.1"
#pragma comment(exestr, "@(#) main.c 55.1 96/05/30 ")
/*
 *	Copyright (C) 1994-1996 The Santa Cruz Operation, Inc.
 *		All rights reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential. 
 */
/*
 * Copyright (c) 1988 Mark Nudleman
 * Copyright (c) 1988, 1993
 *	Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Modification History
 * L000	scol!anthonys	15 Apr 94
 * - Created from BSD4.4 sources, with modifications to support some old
 *   functionality of the previous implementation, plus modifications
 *   to support the requirements of POSIX.2 and XPG4.
 *
 * L001	scol!anthonys	 1 Sep 94
 * - Moved the code that updates "next_name" so that it is updated
 *   even if we fail to open the new file.
 *
 * L002	scol!anthonys	17 Oct 94
 * - Corrected the size of the envargv array, so that it doesn't
 *   accidently get overwritten (by the call to option()).
 *
 * L003	scol!ashleyb	29 Aug 1995
 * - Added support for localised help file.
 *
 * S004 johng@sco.com   30 May 1996
 * - Fixed BUG LTD-245-1154 more now does a fstat on the input file
 *   and sets the ispipe variable if it's a FIFO.
 */

/* #ifndef lint
char copyright[] =
"@)#( Copyright (c) 1988 Mark Nudleman.\n\
@)#( Copyright (c) 1988, 1993
	Regents of the University of California.  All rights reserved.\n";
#endif */ /* not lint */

#ifndef lint
static char sccsid[] = "@)#(main.c	8.1 (Berkeley) 6/7/93";
#endif /* not lint */

/*
 * Entry point, initialization, miscellaneous routines.
 */

#include <sys/types.h>
#include <sys/file.h>
#include <stdio.h>
#include "less.h"
#include <fcntl.h>
#include <sys/stat.h>

#ifdef INTL
#  include <locale.h>
#  include "more_msg.h"
   nl_catd catd;
#else
#  define MSGSTR(id,def) (def)
#endif

int	ispipe;
int	new_file;
int	is_tty;
char	*current_file, *previous_file, *current_name, *next_name;
off_t	prev_pos;
int	any_display;
int	scroll;
int	ac;
char	**av;
int	curr_ac;
int	quitting;
int	exit_status;
char	*loc;							/* L003 */

extern int	file;
extern int	cbufs;
extern int	errmsgs;

extern char	*tagfile;
extern int	tagoption;

/*
 * Edit a new file.
 * Filename "-" means standard input.
 * No filename means the "current" file, from the command line.
 *
 * If flag is non-zero, and there is a problem opening the
 * new file, set exit_status to reflect the failure.
 */

edit(filename, flag)
	register char *filename;
	int flag;
{
	extern int errno;
	register int f;
	register char *m;
	off_t initial_pos, position();
	static int didpipe;
	struct stat stat_struct;
	char message[100], *p;
	char *strrchr(), *strerror(), *save(), *bad_file();
	extern char *tmp_firstcommand, *firstcommand;

	initial_pos = NULL_POSITION;
	if (filename == NULL || *filename == '\0') {
		if (curr_ac >= ac) {
			error(MSGSTR(MORE_MSG_NOFILE, "No current file"));
			return(0);
		}
		filename = save(av[curr_ac]);
	}
	else if (strcmp(filename, "#") == 0) {
		if (*previous_file == '\0') {
			error(MSGSTR(MORE_MSG_NOPREVFILE, "no previous file"));
			return(0);
		}
		filename = save(previous_file);
		initial_pos = prev_pos;
	} else
		filename = save(filename);

	if (curr_ac >= ac)				/* L001 */
		next_name = NULL;			/* L001 */
	else						/* L001 */
		next_name = av[curr_ac + 1];		/* L001 */

	/* use standard input. */
	if (!strcmp(filename, "-")) {
		if (didpipe) {
			error(MSGSTR(MORE_MSG_STDINONCE, "Can view standard input only once"));
			return(0);
		}
		f = 0;
	}
	else if ((m = bad_file(filename, message, sizeof(message))) != NULL) {
		if (flag)
			exit_status = 1;
		error(m);
		free(filename);
		return(0);
	}
	else if ((f = open(filename, O_RDONLY, 0)) < 0) {
		if (flag)
			exit_status = 1;
		(void)sprintf(message, "%s: %s", filename, strerror(errno));
		error(message);
		free(filename);
		return(0);
	}

	if (isatty(f)) {
		/*
		 * Not really necessary to call this an error,
		 * but if the control terminal (for commands)
		 * and the input file (for data) are the same,
		 * we get weird results at best.
		 */
		error(MSGSTR(MORE_MSG_INTERM, "Can't take input from a terminal"));
		if (f > 0)
			(void)close(f);
		(void)free(filename);
		return(0);
	}

	/*
	 * We are now committed to using the new file.
	 * Close the current input file and set up to use the new one.
	 */
	if (file > 0)
		(void)close(file);
	new_file = 1;
	if (previous_file != NULL)
		free(previous_file);
	previous_file = current_file;
	current_file = filename;
	pos_clear();
	prev_pos = position(TOP);
	if (fstat ( f, &stat_struct ) != 0 ) {		/* S002  vvv */
		error(MSGSTR(MORE_MSG_FSTAT, "Stat of input file failed"));
		return(0);
	}
	if( S_ISFIFO( stat_struct.st_mode ) || (f == 0) ){
		ispipe = 1;
	}
	/* ispipe = (f == 0); */			/* S002 ^^^  */
	if (ispipe) {
		didpipe = 1;
		current_name = "stdin";
	} else
		current_name = (p = strrchr(filename, '/')) ? p + 1 : filename;
	file = f;
	ch_init(cbufs, 0);
	init_mark();

	if (is_tty) {
		int no_display = !any_display;
		any_display = 1;
		if (no_display && errmsgs > 0) {
			/*
			 * We displayed some messages on error output
			 * (file descriptor 2; see error() function).
			 * Before erasing the screen contents,
			 * display the file name and wait for a keystroke.
			 */
			error(filename);
		}
		/*
		 * Indicate there is nothing displayed yet.
		 */
		if (initial_pos != NULL_POSITION)
			jump_loc(initial_pos);
		clr_linenum();
	}
	tmp_firstcommand = firstcommand;
	return(1);
}

/*
 * Edit the next file in the command line list.
 */
next_file(n)
	int n;
{
	extern int quit_at_eof;
	off_t position();

	if (curr_ac + n >= ac) {
		if (quit_at_eof || position(TOP) == NULL_POSITION)
			quit();
		error(MSGSTR(MORE_MSG_NONFILE, "No (N-th) next file"));
	}
	else
		(void)edit(av[curr_ac += n], 1);
}

/*
 * Edit the previous file in the command line list.
 */
prev_file(n)
	int n;
{
	if (curr_ac - n < 0)
		error(MSGSTR(MORE_MSG_NO_PREVNFILE, "No (N-th) previous file"));
	else
		(void)edit(av[curr_ac -= n], 1);
}

/*
 * copy a file directly to standard output; used if stdout is not a tty.
 * the only processing is to squeeze multiple blank input lines.
 */
static
cat_file()
{
	extern int squeeze;
	register int c, empty;

	if (squeeze) {
		empty = 0;
		while ((c = ch_forw_get()) != EOI)
			if (c != '\n') {
				putchr(c);
				empty = 0;
			}
			else if (empty < 2) {
				putchr(c);
				++empty;
			}
	}
	else while ((c = ch_forw_get()) != EOI)
		putchr(c);
	flush();
}

main(argc, argv)
	int argc;
	char **argv;
{
	int envargc, argcnt;
	char *envargv[3], *getenv();			/* L002 */

#ifdef INTL
	loc = setlocale (LC_ALL, "");			/* L003 */
	catd = catopen (MF_MORE, MC_FLAGS);
#endif
	/*
	 * Process command line arguments and MORE environment arguments.
	 * Command line arguments override environment arguments.
	 */
	if (envargv[1] = getenv("MORE")) {
		envargc = 2;
		envargv[0] = "more";
		envargv[2] = NULL;
		(void)option(envargc, envargv);
	}
	argcnt = option(argc, argv);
	argv += argcnt;
	argc -= argcnt;

	/*
	 * Set up list of files to be examined.
	 */
	ac = argc;
	av = argv;
	curr_ac = 0;

	exit_status = 0;

	/*
	 * Set up terminal, etc.
	 */
	is_tty = isatty(1);
	if (!is_tty) {
		/*
		 * Output is not a tty.
		 * Just copy the input file(s) to output.
		 */
		if (ac < 1) {
			(void)edit("-", 0);
			cat_file();
		} else {
			do {
				(void)edit((char *)NULL, 0);
				if (file >= 0)
					cat_file();
			} while (++curr_ac < ac);
		}
		exit(0);
	}

	raw_mode(1);
	get_term();
	open_getchr();
	init();
	init_signals(1);

	/* select the first file to examine. */
	if (tagoption) {
		/*
		 * A -t option was given; edit the file selected by the
		 * "tags" search, and search for the proper line in the file.
		 */
		if (!tagfile || !edit(tagfile, 0) || tagsearch())
			quit();
	}
	else if (ac < 1)
		(void)edit("-", 0);	/* Standard input */
	else {
		/*
		 * Try all the files named as command arguments.
		 * We are simply looking for one which can be
		 * opened without error.
		 */
		do {
			(void)edit((char *)NULL, 0);
		} while (file < 0 && ++curr_ac < ac);
	}

	if (file >= 0)
		commands();
	quit();
	/*NOTREACHED*/
}

/*
 * Copy a string to a "safe" place
 * (that is, to a buffer allocated by malloc).
 */
char *
save(s)
	char *s;
{
	char *p, *strcpy(), *malloc();

	p = malloc((u_int)strlen(s)+1);
	if (p == NULL)
	{
		error(MSGSTR(MORE_ERR_OUTOFMEM, "cannot allocate memory"));
		quit();
	}
	return(strcpy(p, s));
}

/*
 * Exit the program.
 */
quit()
{
	/*
	 * Put cursor at bottom left corner, clear the line,
	 * reset the terminal modes, and exit.
	 */
	quitting = 1;
	lower_left();
	clear_eol();
	deinit();
	flush();
	raw_mode(0);
	exit(exit_status);
}
