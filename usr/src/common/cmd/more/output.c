/*	copyright "%c%"	*/

#ident	"@(#)more:output.c	1.1"

/*
 * COPYRIGHT NOTICE
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
static char rcsid[] = "@(#)$RCSfile$ $Revision$ (OSF) $Date$";
#endif
/*
 * Copyright (c) 1988 Mark Nudleman
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
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

#ifndef lint
static char sccsid[] = "@(#)output.c	5.10 (Berkeley) 7/24/91";
#endif /* not lint */

/*
 * High level routines dealing with the output to the screen.
 */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "less.h"
#define _XOPEN_SOURCE 1
#include <wchar.h>

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
extern int show_all_opt, show_opt, bs_mode;
extern int mb_cur_max;

static void putwchr(wchar_t);

/* display the line which is in the line buffer. */
void
put_line(void)
{
	register unsigned char *p;
	register int c;
	register int column;
	extern int auto_wrap, ignaw;

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
	if (mb_cur_max > 1) {		/* use wchar_t's */
#define LINE_LENGTH	1024
		wchar_t	*wp;
		wchar_t	wc;
		wchar_t wline[LINE_LENGTH];

		if (mbstowcs(wline, line, LINE_LENGTH) == (size_t)-1) {
			perror("more");
			return;
		}
		for (wp = wline;  *wp != '\0';  wp++)
		{
			switch (wc = *wp)
			{
			case ESC_CHAR:
				switch (wc = *++wp)
				{
				case UL_CHAR:
					ul_enter();
					column += ul_width +1;
					break;
				case UE_CHAR:
					ul_exit();
					column += ue_width;
					break;
				case BO_CHAR:
					bo_enter();
					column += bo_width +1;
					break;
				case BE_CHAR:
					bo_exit();
					column += be_width;
					break;
				case ESC_CHAR:
					goto wcshow_it;
					/* NOTREACHED */
					break;
				default:
					error("default case in putline/ESC_CHAR!!");
				}
				break;
			case '\t':
				if (show_all_opt)
					goto wcshow_it;
				else
					do
					{
						putchr(' ');
						column++;
					} while ((column % tabstop) != 0);
				break;
			case '\b':
				if (show_all_opt || bs_mode)
					goto wcshow_it;
				else 
				{
					putbs();
					if (column != 0) column--;
				}
				break;
			case '\r':
				if (show_all_opt)
					goto wcshow_it;
				else
					column = 0;
				break;
			default:
wcshow_it:		
				/*
				 * This pretty much only works for ascii
				 * wide characters.  Can we have a 
				 * non-printable multi-byte char? 
				 * If so, how do we show it?
				 */
				if (show_opt) {
					if (!iswprint(wc) && !iswcntrl(wc)) {
						putstr("M-");
						column += 2;
						wc = toascii(wc);
					}
					if (iswcntrl(wc)) {
						putchr('^');
						column++;
						wc = CARAT_CHAR(wc);
					}
				}
				putwchr(wc);
				column++;
				break;

			}
		}
		if (column < sc_width || !auto_wrap || ignaw)
			putchr('\n');
	} else {			/* single byte output */
		for (p = (unsigned char *)line;  *p != '\0';  p++)
		{
			c = *p;
			switch (c)
			{
			case ESC_CHAR:
				switch (c = *++p)
				{
				case UL_CHAR:
					ul_enter();
					column += ul_width +1;
					break;
				case UE_CHAR:
					ul_exit();
					column += ue_width;
					break;
				case BO_CHAR:
					bo_enter();
					column += bo_width +1;
					break;
				case BE_CHAR:
					bo_exit();
					column += be_width;
					break;
				case ESC_CHAR:
					goto show_it;
					/* NOTREACHED */
					break;
				default:
					error("default case in putline/ESC_CHAR!!");
				}
				break;
			case '\t':
				if (show_all_opt)
					goto show_it;
				else
					do
					{
						putchr(' ');
						column++;
					} while ((column % tabstop) != 0);
				break;
			case '\b':
				if (show_all_opt || bs_mode)
					goto show_it;
				else 
				{
					putbs();
					if (column != 0) column--;
				}
				break;
			case '\r':
				if (show_all_opt)
					goto show_it;
				else
					column = 0;
				break;
			default:
show_it:		
				if (show_opt) {
					if (!isprint(c) && ! iscntrl(c)) {
						putstr("M-");
						column += 2;
						c = toascii(c);
					}
					if (iscntrl(c)) {
						c = CARAT_CHAR(c);
						putchr('^');
						column++;
					}
				}
				putchr(c);
				column++;
				break;
			}
		}
		if (column < sc_width || !auto_wrap || ignaw)
			putchr('\n');
	}
}

static char obuf[1024];
static char *ob = obuf;

/*
 * Flush buffered output.
 */
void
flush(void)
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
void
purge(void)
{

	ob = obuf;
}

/*
 * Output a character.
 */
void
putchr(int c)
{
	if (ob >= &obuf[sizeof(obuf)])
		flush();
	*ob++ = c;
}

/*
 * Output a wide character.
 */
static void
putwchr(wchar_t wc)
{
	char 	c[MB_LEN_MAX];
	int	len, i;

	len = wctomb(c, wc);
	if (ob + len >= &obuf[sizeof(obuf)])
		flush();	/* may flush a little early, but thats ok */

	for (i=0; i < len; i++)
		*ob++ = c[i];
}

/*
 * Output a string.
 */
void
putstr(register char *s)
{
	while (*s != '\0')
		putchr(*s++);
}

int cmdstack;
static char return_to_continue[] = "(press RETURN)";

/*
 * Output a message in the lower left corner of the screen
 * and wait for carriage return.
 */
void
error(char *s)
{
	int ch;
	static char *rets = NULL;

	if (rets == NULL)
		rets = MSGSTR(PRETURN, return_to_continue);

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
	putstr(rets);
	so_exit();

	if ((ch = getchr()) != '\n') {
		if (ch == 'q')
			quit(0);
		cmdstack = ch;
	}
	lower_left();

	if ((s ? strlen(s) : 0) + sizeof(rets) + 
		so_width + se_width + 1 > sc_width)
		/*
		 * Printing the message has probably scrolled the screen.
		 * {{ Unless the terminal doesn't have auto margins,
		 *    in which case we just hammered on the right margin. }}
		 */
		repaint();
	flush();
}

static char intr_to_abort[] = "... (interrupt to abort)";

void
ierror(char *s)
{
	lower_left();
	clear_eol();
	so_enter();
	putstr(s);
	putstr(MSGSTR(INTRTOAB, intr_to_abort));
	so_exit();
	flush();
}
