#ident	"@(#)OSRcmds:more/output.c	1.1"
#pragma comment(exestr, "@(#) output.c 55.1 96/05/28 ")
/*
 *	Copyright (C) 1994-1996 The Santa Cruz Operation, Inc.
 *		All rights reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential. 
 */
/*
 * Copyright (c) 1988 Mark Nudleman
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 * S001 johng@sco.com	14 May 96
 * - Put -v option back in binary.
 * - Put -r option back in binary.
 * - Put -l option back in binary. ( partial )
 */

#ifndef lint
static char sccsid[] = "@)#(output.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

/*
 * High level routines dealing with the output to the screen.
 */

#include <stdio.h>
#include "less.h"

#ifdef INTL
#  include      <locale.h>
#  include      "more_msg.h"
   extern nl_catd catd;
#else
#  define MSGSTR(id,def) (def)
#endif

int errmsgs;	/* Count of messages displayed by error() */

extern int sigs;
extern int sc_width, sc_height;
extern int ul_width, ue_width;
extern int so_width, se_width;
extern int bo_width, be_width;
extern int tabstop;
extern int screen_trashed;
extern int any_display;
extern char *line;
extern int	vflag;
extern int	rflag;			/* S001 */
extern int	lflag;			/* S001 */
extern int	bs_mode;		/* S001 */

/* display the line which is in the line buffer. */
put_line()
{
	register char *p;
	register int c;
	register int column;
	extern int auto_wrap, ignaw;
	int	cnt;

	if (sigs)
	{
		/*
		 * Don't output if a signal is pending.
		 */
		screen_trashed = 1;
		return;
	}

	if (line == NULL)
		line = "";

	column = 0;
	for (p = line;  *p != '\0';  p++)
	{
		switch (c = *p ) 
		{
		case UL_CHAR:
			if ( ( ! bs_mode) && (! vflag) )
			{
				ul_enter();
				column += ul_width +1;
			}
			else
			{
				putchr('^');
				putchr(CARAT_CHAR(c&0x7f));
				column += 2;
			}
			break;
		case UE_CHAR:
			if ( (! bs_mode) && (! vflag) )
			{
				ul_exit();
				column += ue_width;
			}
			else
			{
				putchr('^');
				putchr(CARAT_CHAR(c&0x7f));
				column += 2;
			}
			break;
		case BO_CHAR:
			if ((! bs_mode) && (! vflag) )
			{
				bo_enter();
				column += bo_width +1;
			}
			else
			{
				putchr('^');
				putchr(CARAT_CHAR(c&0x7f));
				column += 2;
			}
			break;
		case BE_CHAR:
			if ((! bs_mode) && (! vflag ))
			{
				bo_exit();
				column += be_width;
			}
			else
			{
				putchr('^');
				putchr(CARAT_CHAR(c&0x7f));
				column += 2;
			}
			break;
		case '\t':
			if ( ! vflag ) 
			{
				do
				{
					putchr(' ');
					column++;
				} while ((column % tabstop) != 0);
			}
			else
			{
				putchr('^');
				putchr(CARAT_CHAR(c));
				column += 2;
			}
			break;
		case '\b':
			if ( ! vflag )
			{
				putbs();
				column--;
			}
			else
			{
				putchr('^');
				putchr(CARAT_CHAR(c));
				column += 2;
			}
			break;
		case '\f':					/* S001 */
			if ( lflag == 1 || vflag )  {
				putchr('^');
				putchr(CARAT_CHAR(c));
				column += 2;
			} else {
				putchr(c);
				column++;
			}
			break;
		case '\r':					/* S001 */
			if ((rflag == 1) || (bs_mode == 1) || (vflag ==1)) {
				putchr('^');
				putchr(CARAT_CHAR(c));
				column += 2;
			} else {
				putchr(c);
				column++;
			}
			break;

		default:
			if (vflag == 1) 
			{   		             /* 0 - 31 */
				if ( CONTROL_CHAR( c & 0xff ) ) /* S001  */
				{
					putchr('^');
					putchr(CARAT_CHAR(c));
					column += 2;
				}
				else if ( (c & 0xff) == 0200 ) 
				{  /* zero equals \0200          */
				   /* 0200 = zero. Thanks curses */
				}  /* Don't print anything       */
				else if ( !isprint( c & 0xff) ) /* high */
				{
					putchr('M' );
					if ( isprint( c & 0x7f ))
					{
						putchr('-' );
						putchr((char )c & 0x7f );
					}
					else
					{
						putchr('^');
						putchr(CARAT_CHAR(c & 0x7f));
					}
					column += 3;
				}
				else
				{
					putchr(c);
					column++;
				}
			}
			else
			{
				putchr(c);
				column++;
			}
		}
	}
	if (column < sc_width || !auto_wrap || ignaw)
		putchr('\n');
}

static char obuf[1024];
static char *ob = obuf;

/*
 * Flush buffered output.
 */
flush()
{
	register int n;

	n = ob - obuf;
	if (n == 0)
		return;
	if (write(1, obuf, n) != n)
		screen_trashed = 1;
	ob = obuf;
}

/*
 * Purge any pending output.
 */
purge()
{

	ob = obuf;
}

/*
 * Output a character.
 */
putchr(c)
	int c;
{
	if (ob >= &obuf[sizeof(obuf)])
		flush();
	*ob++ = c;
}

/*
 * Output a string.
 */
putstr(s)
	register char *s;
{
	while (*s != '\0')
		putchr(*s++);
}

int cmdstack;
static char *return_to_continue = NULL;

/*
 * Output a message in the lower left corner of the screen
 * and wait for carriage return.
 */
error(s)
	char *s;
{
	int ch;
	char *save();

	++errmsgs;
	if (!any_display) {
		/*
		 * Nothing has been displayed yet.  Output this message on
		 * error output (file descriptor 2) and don't wait for a
		 * keystroke to continue.
		 *
		 * This has the desirable effect of producing all error
		 * messages on error output if standard output is directed
		 * to a file.  It also does the same if we never produce
		 * any real output; for example, if the input file(s) cannot
		 * be opened.  If we do eventually produce output, code in
		 * edit() makes sure these messages can be seen before they
		 * are overwritten or scrolled away.
		 */
		(void)write(2, s, strlen(s));
		(void)write(2, "\n", 1);
		return;
	}

	lower_left();
	clear_eol();
	so_enter();
	if (s) {
		putstr(s);
		putstr("  ");
	}
	if (return_to_continue == NULL)
		return_to_continue = save(MSGSTR(MORE_MSG_RTC, "(press RETURN)"));
	putstr(return_to_continue);
	so_exit();

	if ((ch = getchr()) != '\n') {
		if (ch == 'q')
			quit();
		cmdstack = ch;
	}
	lower_left();

	if (strlen(s) + sizeof(return_to_continue) + 
		so_width + se_width + 1 > sc_width)
		/*
		 * Printing the message has probably scrolled the screen.
		 * {{ Unless the terminal doesn't have auto margins,
		 *    in which case we just hammered on the right margin. }}
		 */
		repaint();
	flush();
}

static char *intr_to_abort = NULL;

ierror(s)
	char *s;
{
	char *save();

	lower_left();
	clear_eol();
	so_enter();
	putstr(s);
	if (intr_to_abort == NULL)
		intr_to_abort = save(MSGSTR(MORE_MSG_ITA, "... (interrupt to abort)"));
	putstr(intr_to_abort);
	so_exit();
	flush();
}
