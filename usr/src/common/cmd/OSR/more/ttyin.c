#ident	"@(#)OSRcmds:more/ttyin.c	1.1"
#pragma comment(exestr, "@(#) ttyin.c 25.1 94/04/18 ")
/*
 *	Copyright (C) 1994 The Santa Cruz Operation, Inc.
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
 */

#ifndef lint
static char sccsid[] = "@)#(ttyin.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

/*
 * Routines dealing with getting input from the keyboard (i.e. from the user).
 */

#include "less.h"

static int tty;

char *tmp_firstcommand;

extern int exit_status;

/*
 * Open keyboard for input.
 * (Just use file descriptor 2.)
 */
open_getchr()
{
	tty = 2;
}

/*
 * Get a character from the keyboard.
 */
getchr()
{
	char c;
	int result;

	if (tmp_firstcommand && *tmp_firstcommand)
		return (*tmp_firstcommand++);

	do
	{
		result = iread(tty, &c, 1);
		if (result == READ_INTR)
			return (READ_INTR);
		if (result < 0)
		{
			exit_status = 2;
			/*
			 * Don't call error() here,
			 * because error calls getchr!
			 */
			quit();
		}
	} while (result != 1);
	return (c);
}
