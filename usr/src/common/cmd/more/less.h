/*	copyright "%c%"	*/

#ident	"@(#)more:less.h	1.1"

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
/* @(#)$RCSfile$ $Revision$ (OSF) $Date$ */
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
 *
 *	less.h	5.9 (Berkeley) 6/1/90
 */

#include <nl_types.h> /* XXX */
#include <sys/types.h>

#define	NULL_POSITION	((off_t)(-1))

#define	EOI		(-1)
#define	READ_INTR	(-2)

/*
 * Special chars used to tell put_line() to do something special.
 * Always preceded by ESC_CHAR if they are real control codes.
 * ESC_CHAR is escaped with itself if it is in input stream.
 */
#define	ESC_CHAR	'\001'		/* character preceding all above */
#define	UL_CHAR		'\002'		/* Enter underline mode */
#define	UE_CHAR		'\003'		/* Exit underline mode */
#define	BO_CHAR		'\004'		/* Enter boldface mode */
#define	BE_CHAR		'\005'		/* Exit boldface mode */

#define	CONTROL_CHAR(c)		(iscntrl(c))
#define	CARAT_CHAR(c)		((c == '\177') ? '?' : (c | 0100))

#define	TOP		(0)
#define	TOP_PLUS_ONE	(1)
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
#define	A_SF_SCROLL		28
#define	A_SKIP			29
#define	A_SHELL			30
#define	A_REV_SEARCH		31

#define MSGSTR(Num, Str)	gettxt(Num,Str)

#define HELPFILE        "/usr/lib/more.help"

/*
 * prototypes
 */

/* ch.c */
extern int 	ch_seek(off_t);
extern int	ch_end_seek(void);
extern int	ch_beg_seek(void);
extern off_t	ch_length(void);
extern off_t	ch_tell(void);
extern int	ch_forw_get(void);
extern int	ch_back_get(void);
extern void	ch_init(int, int);

/* command.c */
extern void	commands(void);

/* decode.c */
extern void	noprefix(void);
extern int	cmd_decode(int);

/* help.c */
extern void	help(void);

/* input.c */
extern off_t	forw_line(off_t);
extern off_t	back_line(off_t);

/* line.c */
extern void	prewind(void);
extern int	pappend(int);
extern off_t	forw_raw_line(off_t);
extern off_t	back_raw_line(off_t);

/* linenum.c */
extern void	clr_linenum(void);
extern void	add_lnum(int, off_t);
extern int 	find_linenum(off_t);
extern int	currline(int);

/* main.c */
extern int 	edit(char *);
extern void	next_file(int);
extern void 	prev_file(int);
extern char	*save(char *);
extern void	quit(int);

/* option.c */
extern int	option(int, char **);

/* os.c */
extern void	lsystem(char *);
extern int	iread(int, unsigned char *, int);
extern void	intread(void);
extern char	*mglob(char *);
extern char 	*bad_file(char *, char *);

/* output.c */
extern void	put_line(void);
extern void	flush(void);
extern void	purge(void);
extern void	putchr(int);
extern void	putstr(char *);
extern void	error(char *);
extern void	ierror(char *);

/* position.c */
extern off_t	position(int);
extern void	add_forw_pos(off_t);
extern void 	add_back_pos(off_t);
extern void	copytable(void);
extern void	pos_clear(void);
extern int	onscreen(off_t);

/* prim.c */
extern void	forward(int, int);
extern void	backward(int, int);
extern void	repaint(void);
extern void	jump_forw(void);
extern void	jump_back(int);
extern void	jump_percent(int);
extern void	jump_loc(off_t);
extern void	init_mark(void);
extern void	setmark(int);
extern void	gomark(int);
extern int	search(int, char *, int, int);

/* screen.c */
extern void	raw_mode(int);
extern void	get_term(void);
extern void	init(void);
extern void	deinit(void);
extern void	home(void);
extern void	add_line(void);
extern void	lower_left(void);
extern void	bell(void);
extern void	clear(void);
extern void	clear_eol(void);
extern void	so_enter(void);
extern void	so_exit(void);
extern void	ul_enter(void);
extern void	ul_exit(void);
extern void	bo_enter(void);
extern void	bo_exit(void);
extern void	backspace(void);
extern void	putbs(void);

/* signal.c */
extern void	init_signals(int);
extern void 	winch(int);
extern void	psignals(void);

/* tags.c */
extern void	findtag(char *);
extern int	tagsearch(void);

/* ttyin.c */
extern void	open_getchr(void);
extern int	getchr(void);

#define NOBUFF ":152"
#define FILEMSG ":153"
#define LINEMSG ":154"
#define BYTEMSG ":155"
#define PERCENT ":156"
#define EOFNEXT ":157"
#define END ":158"
#define PHELP ":159"
#define HELP ":160"
#define PTAG ":161"
#define EXAMINE ":162"
#define NOSTDIN ":163"
#define MARK ":164"
#define GMARK ":165"
#define PRESSH ":166"
#define NOFARGS ":167"
#define STDIN ":168"
#define SHDONE ":169"
#define ELINENO ":170"
#define NOCURR ":171"
#define NOPREV ":172"
#define NOVIEW ":173"
#define NOTERM ":174"
#define NONTHF ":175"
#define NONTHF2 ":176"
#define NOMEM ":177"
#define BADSCREEN ":178"
#define BADWARG ":179"
#define USAGE ":180"
#define ISADIR ":181"
#define PRETURN ":182"
#define INTRTOAB ":183"
#define SKIP ":184"
#define NOSEEK ":185"
#define NOBEGIN ":186"
#define ONLYN ":187"
#define NOLENGTH ":188"
#define CANTSEEK ":189"
#define BADLET ":190"
#define NOMARK ":191"
#define NOREG ":192"
#define NOSEARCH ":193"
#define NOPAT ":194"
#define NOTAGF ":195"
#define NOSUCHTAG ":196"
#define TNOTFOUND ":197"
#define HELPEND ":198"
#define PIPE_ERR ":199"
