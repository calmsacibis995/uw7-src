/*	copyright "%c%"	*/

#ident	"@(#)more:signal.c	1.1"

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
static char sccsid[] = "@(#)signal.c	5.8 (Berkeley) 3/1/91";
#endif /* not lint */

#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include "less.h"

/*
 * Routines dealing with signals.
 *
 * A signal usually merely causes a bit to be set in the "signals" word.
 * At some convenient time, the mainline code checks to see if any
 * signals need processing by calling psignal().
 * If we happen to be reading from a file [in iread()] at the time
 * the signal is received, we call intread to interrupt the iread.
 */


/*
 * "sigs" contains bits indicating signals which need to be processed.
 */
int sigs;

#define	S_STOP		02
#define S_WINCH		04
#define S_INT		010

extern int sc_width, sc_height;
extern int screen_trashed;
extern int lnloop;
extern int linenums;
extern int scroll;
extern int reading;

/*
 * "Stop" (^Z) signal handler.
 */
/*ARGSUSED*/
static void
stop(int s)
{
	(void)signal(SIGTSTP, stop);
	sigs |= S_STOP;
	if (reading)
		intread();
}

/*
 * "Window" change handler
 */
/*ARGSUSED*/
void
winch(int s)
{
	(void)signal(SIGWINCH, winch);
	sigs |= S_WINCH;
	if (reading)
		intread();
}

/*ARGSUSED*/
static void
purgeandquit(int s)
{

	if (lnloop) {
		sigs |= S_INT;
		lnloop=0;
	} else {
		purge();	/* purge buffered output */
		quit(0);
	}
}

/*
 * Set up the signal handlers.
 */
void
init_signals(int on)
{
	if (on)
	{
		/*
		 * Set signal handlers.
		 */
		(void)signal(SIGINT, purgeandquit);
		(void)signal(SIGTSTP, stop);
		(void)signal(SIGWINCH, winch);
	} else
	{
		/*
		 * Restore signals to defaults.
		 */
		(void)signal(SIGINT, SIG_DFL);
		(void)signal(SIGTSTP, SIG_DFL);
		(void)signal(SIGWINCH, SIG_IGN);
	}
}

/*
 * Process any signals we have received.
 * A received signal cause a bit to be set in "sigs".
 */
void
psignals(void)
{
	register int tsignals;

	if ((tsignals = sigs) == 0)
		return;
	sigs = 0;

	if (tsignals & S_WINCH)
	{
		int old_width, old_height;
		/*
		 * Re-execute get_term() to read the new window size.
		 */
		old_width = sc_width;
		old_height = sc_height;
		get_term();
		if (sc_width != old_width || sc_height != old_height)
		{
			scroll = (sc_height + 1) / 2;
			screen_trashed = 1;
		}
	}
	if (tsignals & S_STOP)
	{
		/*
		 * Clean up the terminal.
		 */
		(void)signal(SIGTTOU, SIG_IGN);
		lower_left();
		clear_eol();
		deinit();
		flush();
		raw_mode(0);
		(void)signal(SIGTTOU, SIG_DFL);
		(void)signal(SIGTSTP, SIG_DFL);
		(void)kill(getpid(), SIGTSTP);
		/*
		 * ... Bye bye. ...
		 * Hopefully we'll be back later and resume here...
		 * Reset the terminal and arrange to repaint the
		 * screen when we get back to the main command loop.
		 */
		(void)signal(SIGTSTP, stop);
		raw_mode(1);
		init();
		screen_trashed = 1;
	}
}
