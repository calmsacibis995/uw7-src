#ident	"@(#)OSRcmds:more/less.h	1.1"
#pragma comment(exestr, "@(#) less.h 25.1 94/04/18 ")
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
 *
 *	@)#(less.h	8.1 (Berkeley) 6/6/93
 */
/*
 * Modification History
 * L000	scol!anthonys	15 Apr 94
 * - Created from BSD4.4 sources, with modifications to support some old
 *   functionality of the previous implementation, plus modifications
 *   to support the requirements of POSIX.2 and XPG4.
 */

#define	RECOMP

#define	NULL_POSITION	((off_t)(-1))

#define	EOI		(0)
#define	READ_INTR	(-2)

/* Special chars used to tell put_line() to do something special */
#define	UL_CHAR		'\201'		/* Enter underline mode */
#define	UE_CHAR		'\202'		/* Exit underline mode */
#define	BO_CHAR		'\203'		/* Enter boldface mode */
#define	BE_CHAR		'\204'		/* Exit boldface mode */

#define	CONTROL_CHAR(c)		(iscntrl(c))
#define	CARAT_CHAR(c)		((c == '\177') ? '?' : (c | 0100))

#define	TOP		(0)
#define	TOP_PLUS_ONE	(1)
#define	CURRENT_POS	(TOP)		/* Hack for the moment */
#ifdef OMIT
#define	CURRENT_POS	(2)		/* "Third" line on the screen */
#define	CURRENT_POS_MINUS_ONE	(CURRENT_POS - 1)	/* Start here going backwards */
#define	CURRENT_POS_PLUS_ONE	(CURRENT_POS + 1)	/* Start here going forwards */
#endif
#define	BOTTOM		(-1)
#define	BOTTOM_PLUS_ONE	(-2)
#define	MIDDLE		(-3)

#define	A_INVALID		-1

#define	A_AGAIN_SEARCH		1
#define	A_B_LINE		2
#define	A_B_SCREEN		3
#define	A_B_SCROLL		4
#define	A_B_SEARCH		5
#define	A_DIGIT			6
#define	A_EXAMINE		7
#define	A_FREPAINT		8
#define	A_F_LINE		9
#define	A_F_SCREEN		10
#define	A_F_SCROLL		11
#define	A_F_SEARCH		12
#define	A_GOEND			13
#define	A_GOLINE		14
#define	A_GOMARK		15
#define	A_HELP			16
#define	A_NEXT_FILE		17
#define	A_PERCENT		18
#define	A_PREFIX		19
#define	A_PREV_FILE		20
#define	A_QUIT			21
#define	A_REPAINT		22
#define	A_SETMARK		23
#define	A_STAT			24
#define	A_VISUAL		25
#define	A_TAGFILE		26
#define	A_FILE_LIST		27
#define	A_SH_ESCAPE		28
#define	A_AGAIN_REVSEARCH	29
#define	A_MINISTAT		30

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif
