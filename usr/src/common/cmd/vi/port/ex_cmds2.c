/*		copyright	"%c%" 	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/* Copyright (c) 1981 Regents of the University of California */
#ident	"@(#)vi:port/ex_cmds2.c	1.13.1.15"
#ident "$Header$"

#include "ex.h"
#include "ex_argv.h"
#include "ex_temp.h"
#include "ex_tty.h"
#include "ex_vis.h"
#include <stdarg.h>
#include <pfmt.h>

extern bool	pflag, nflag;		/* extern; also in ex_cmds.c */
extern int	poffset;		/* extern; also in ex_cmds.c */
extern short	slevel;			/* extern; has level of source() */

void error(unsigned char *id, unsigned char * str, ...);
void serror(unsigned char *id, unsigned char *str, ...);

extern void merror(unsigned char *id, unsigned char *seekpt, ...);
extern void vsmerror(unsigned char *id, unsigned char *seekpt, va_list ap);
extern void vmerror(unsigned char *id, unsigned char *seekpt, va_list ap);


/*
 * Subroutines for major command loop.
 */

/*
 * Is there a single letter indicating a named buffer next?
 */
cmdreg()
{
	register int c = 0;
	register int wh = skipwh();

	if (wh && isalpha(c = peekchar()) && isascii(c))
		c = getchar();
	return (c);
}

/*
 * Tell whether the character ends a command
 */
endcmd(ch)
	int ch;
{
	switch (ch) {
	
	case '\n':
	case EOF:
		endline = 1;
		return (1);
	
	case '|':
	case '"':
		endline = 0;
		return (1);
	}
	return (0);
}

/*
 * Insist on the end of the command.
 */
eol()
{

	if (!skipend())
		error((unsigned char *)":29", (unsigned char *)"Extra chars|Extra characters at end of command");
	ignnEOF();
}

/*
 * Print out the message in the error message file at str,
 * with i an integer argument to printf.
 */
/*VARARGS2*/
void
error(unsigned char *id, unsigned char *str, ...)
{
	va_list ap;

	va_start(ap, str);
	errcnt++;
	error0();
	vmerror(id, str, ap);
	if (writing) 
		serror((unsigned char *)":30", (unsigned char *)" [Warning - %s is incomplete]", file);
	error1(str);
	writing = 0;
	va_end(ap);
}

/*
 * Same as error() but don't increment exit value.
 */
/*VARARGS2*/
void
notice(unsigned char *id, unsigned char *str, ...)
{
	va_list ap;

	va_start(ap, str);
	error0();
	vmerror(id, str, ap);
	if (writing)
		serror((unsigned char *)":30", (unsigned char *)" [Warning - %s is incomplete]", file);
	error1(str);
	writing = 0;
	va_end(ap);
}

/*
 * Rewind the argument list.
 */
erewind()
{

	argc = argc0;
	argv = argv0;
	args = args0;
	if (argc > 1 && !hush && cur_term) {
		mprintf(":31", "%d files|%d files to edit", argc);
		if (inopen)
			putchar(' ');
		else
			putNFL();
	}
}

/*
 * Guts of the pre-printing error processing.
 * If in visual and catching errors, then we don't mung up the internals,
 * just fixing up the echo area for the print.
 * Otherwise we reset a number of externals, and discard unused input.
 */
error0()
{

	if (laste) {
#ifdef VMUNIX
		tlaste();
#endif
		laste = 0;
		esync();
	}
	if (vcatch) {
		if (splitw == 0)
			fixech();
		if (!enter_standout_mode || !exit_bold)
			dingdong();
		return;
	}
	if (input) {
		input = strend(input) - 1;
		if (*input == '\n')
			setlastchar('\n');
		input = 0;
	}
	setoutt();
	flush();
	resetflav();
	if (!enter_standout_mode || !exit_bold)
		dingdong();
	if (inopen) {
		/*
		 * We are coming out of open/visual ungracefully.
		 * Restore columns, undo, and fix tty mode.
		 */
		columns = OCOLUMNS;
		undvis();
		ostop(normf);
		/* ostop should be doing this
		putpad(cursor_normal);
		putpad(key_eol);
		*/
		putnl();
	}
	inopen = 0;
	holdcm = 0;
}

/*
 * Post error printing processing.
 * Close the i/o file if left open.
 * If catching in visual then throw to the visual catch,
 * else if a child after a fork, then exit.
 * Otherwise, in the normal command mode error case,
 * finish state reset, and throw to top.
 */
error1(str)
	unsigned char *str;
{
	bool die;

	if (io > 0) {
		close(io);
		io = -1;
	}
	die = (getpid() != ppid);	/* Only children die */
	inappend = inglobal = 0;
	globp = vglobp = vmacp = 0;
	if (vcatch && !die) {
		inopen = 1;
		vcatch = 0;
		if (str)
			noonl();
		fixol(writing);
		if (slevel > 0)
			reset();
		longjmp(vreslab,1);
	}
	if (str && !vcatch)
		putNFL();
	if (die)
		exit(++errcnt);
	lseek(0, 0L, 2);
	if (inglobal)
		setlastchar('\n');
	while (lastchar() != '\n' && lastchar() != EOF)
		ignchar();
	ungetchar(0);
	endline = 1;
	reset();
}

fixol(int prob)
{
	if (Outchar != vputchar) {
		flush();
		if (state == ONEOPEN || state == HARDOPEN)
			outline = destline = 0;
		Outchar = vputchar;
		vcontin(1);
	} else {
		if (destcol)
			vclreol();
		vclean();
		if (prob) {
			/* reset terminal to 'vi' mode */
			fixterm();
			putpad(enter_ca_mode);
			tostart();
		}
	}
}

/*
 * Does an ! character follow in the command stream?
 */
exclam()
{

	if (peekchar() == '!') {
		ignchar();
		return (1);
	}
	return (0);
}

/*
 * Make an argument list for e.g. next.
 */
makargs()
{

	glob(&frob);
	argc0 = frob.argc0;
	argv0 = frob.argv;
	args0 = argv0[0];
	erewind();
}

/*
 * Advance to next file in argument list.
 */
next()
{
	extern short isalt;	/* defined in ex_io.c */

	if (argc == 0)
		error((unsigned char *)":32", (unsigned char *)"No more files|No more files to edit");
	morargc = argc;
	isalt = (strcmp((const char *)altfile, (const char *)args)==0) + 1;
	if (savedfile[0])
		CP(altfile, savedfile);
	CP(savedfile, args);
	argc--;
	args = argv ? *++argv : strend(args) + 1;
	destcol = 0;
}

/*
 * Eat trailing flags and offsets after a command,
 * saving for possible later post-command prints.
 */
donewline()
{
	register int c;

	resetflav();
	for (;;) {
		c = getchar();
		switch (c) {

		case '^':
		case '-':
			poffset--;
			break;

		case '+':
			poffset++;
			break;

		case 'l':
			listf++;
			break;

		case '#':
			nflag++;
			break;

		case 'p':
			listf = 0;
			break;

		case ' ':
		case '\t':
			continue;

		case '"':
			comment();
			setflav();
			return;

		default:
			if (!endcmd(c))
serror((unsigned char *)":33", (unsigned char *)"Extra chars|Extra characters at end of \"%s\" command", Command);
			if (c == EOF)
				ungetchar(c);
			setflav();
			return;
		}
		pflag++;
	}
}

/*
 * Before quit or respec of arg list, check that there are
 * no more files in the arg list.
 */
nomore()
{

	if (argc == 0 || morargc == argc)
		return;
	morargc = argc;
	if (argc == 1)
	{
		serror((unsigned char *)":34", (unsigned char *)"1 more file|1 more file to edit");
	} else
	{
		serror((unsigned char *)":35", (unsigned char *)"%d more files|%d more files to edit", argc);
	}
}

/*
 * Before edit of new file check that either an ! follows
 * or the file has not been changed.
 */
quickly()
{

	if (exclam())
		return (1);
	if (chng && dol > zero) {
/*
		chng = 0;
*/
		xchng = 0;
		error((unsigned char *)":36",
			(unsigned char *)"No write|No write since last change (\":%s!\" overrides)",
			Command);
	}
	return (0);
}

/*
 * Reset the flavor of the output to print mode with no numbering.
 */
resetflav()
{

	if (inopen)
		return;
	listf = 0;
	nflag = 0;
	pflag = 0;
	poffset = 0;
	setflav();
}

/*
 * Print an error message with a %s type argument to printf.
 */
void
serror(unsigned char *id, unsigned char *str, ...)
{
	va_list ap;

	va_start(ap, str);
	error0();
	vsmerror(id, str, ap);
	error1(str);
	va_end(ap);
}

/*
 * Print an error message with a %s type argument to printf.
 * then wait so the user can see it.
 */
void
werror(unsigned char *id, unsigned char *str, ...)
{
	va_list ap;
	int c;

	va_start(ap, str);
	error0();
	vsmerror(id, str, ap);
	sleep(1);
	error1(str);
	va_end(ap);
}

/*
 * Set the flavor of the output based on the flags given
 * and the number and list options to either number or not number lines
 * and either use normally decoded (ARPAnet standard) characters or list mode,
 * where end of lines are marked and tabs print as ^I.
 */
setflav()
{

	if (inopen)
		return;
	setnumb(nflag || value(vi_NUMBER));
	setlist(listf || value(vi_LIST));
	setoutt();
}

/*
 * Skip white space and tell whether command ends then.
 */
skipend()
{

	pastwh();
	return (endcmd(peekchar()) && peekchar() != '"');
}

/*
 * Set the command name for non-word commands.
 */
tailspec(c)
	int c;
{
	static unsigned char foocmd[2];

	foocmd[0] = c;
	Command = foocmd;
}

/*
 * Try to read off the rest of the command word.
 * If alphabetics follow, then this is not the command we seek.
 */
tail(comm)
	unsigned char *comm;
{

	tailprim(comm, 1, 0);
}

tail2of(comm)
	unsigned char *comm;
{

	tailprim(comm, 2, 0);
}

unsigned char	tcommand[20];

tailprim(comm, i, notinvis)
	register unsigned char *comm;
	int i;
	bool notinvis;
{
	register unsigned char *cp;
	register int c;

	Command = comm;
	for (cp = tcommand; i > 0; i--)
		*cp++ = *comm++;
	while (*comm && peekchar() == *comm)
		*cp++ = getchar(), comm++;
	c = peekchar();
	if (notinvis || (isalpha(c) && isascii(c))) {
		/*
		 * Of the trailing lp funny business, only dl and dp
		 * survive the move from ed to ex.
		 */
		if (tcommand[0] == 'd' && any(c, "lp"))
			goto ret;
		if (tcommand[0] == 's' && any(c, "gcr"))
			goto ret;
		while (cp < &tcommand[19] && isalpha(c = peekchar()) && isascii(c))
			*cp++ = getchar();
		*cp = 0;
		if (notinvis)
			serror((unsigned char *)":37", 
				(unsigned char *)"What?|%s: No such command from open/visual",
				tcommand);
		else
			serror((unsigned char *)":38",
				(unsigned char *)"What?|%s: Not an editor command",
				tcommand);
	}
ret:
	*cp = 0;
}

/*
 * Continue after a : command from open/visual.
 */
vcontin(ask)
	bool ask;
{

	if (vcnt > 0)
		vcnt = -vcnt;
	if (inopen) {
		if (state != VISUAL) {
			/*
			 * We don't know what a shell command may have left on
			 * the screen, so we move the cursor to the right place
			 * and then put out a newline.  But this makes an extra
			 * blank line most of the time so we only do it for :sh
			 * since the prompt gets left on the screen.
			 *
			 * BUG: :!echo longer than current line \\c
			 * will mess it up.
			 */
			if (state == CRTOPEN) {
				termreset();
				vgoto(WECHO, 0);
			}
			if (!ask) {
				putch('\r');
				putch('\n');
			}
			return;
		}
		if (ask) {
			merror((unsigned char *)":39", (unsigned char *)"[Hit return to continue] ");
			flush();
		}
#ifndef CBREAK
		vraw();
#endif
		if (ask) {
#ifdef notdef
			/*
			 * Gobble ^Q/^S since the tty driver should be eating
			 * them (as far as the user can see)
			 */
			while (peekkey() == CTRL('Q') || peekkey() == CTRL('S'))
				(void)getkey();
#endif
			if(getkey() == ':') {
				/* Extra newlines, but no other way */
				putch('\n');
				outline = WECHO;
				ungetkey(':');
			}
		}
		vclrech(1);
		if (Peekkey != ':') {
			fixterm();
			putpad(enter_ca_mode);
			tostart();
		}
	}
}

/*
 * Put out a newline (before a shell escape)
 * if in open/visual.
 */
vnfl()
{

	if (inopen) {
		if (state != VISUAL && state != CRTOPEN && destline <= WECHO)
			vclean();
		else
			vmoveitup(1, 0);
		vgoto(WECHO, 0);
		vclrbyte(vtube[WECHO], WCOLS);
		tostop();
		/* replaced by the ostop above
		putpad(cursor_normal);
		putpad(key_eol);
		*/
	}
	flush();
}
