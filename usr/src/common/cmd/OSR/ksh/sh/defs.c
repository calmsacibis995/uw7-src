#ident	"@(#)OSRcmds:ksh/sh/defs.c	1.1"
#pragma comment(exestr, "@(#) defs.c 25.5 94/10/24 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1990-1994.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*									*/
/*		   Copyright (C) AT&T, 1984-1992			*/
/*			All Rights Reserved				*/
/*									*/
/*	  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T.		*/
/*	    The copyright notice above does not evidence any		*/
/*	   actual or intended publication of such source code.		*/
/*									*/

/*
 * Ksh - AT&T Bell Laboratories
 * Written by David Korn
 * This file defines all the  read/write shell global variables
 */

/*
 * Modification History
 *
 *	L000	scol!markhe	4 Nov 92
 *	- Hz holds value from gethz()
 *	L001	scol!markhe	18 Nov 92
 *	- added `catd', the file descriptor for ksh's message catalogue
 *	L002	scol!ianw	3 Aug 94
 *	- define NOBUF, necessary to work with the USL libc.
 *	L003	scol!ianw	24 Oct 94
 *	- moved the #define of NOBUF into sh_config.h.
 */

#include	"defs.h"
#include	<nl_types.h>
#include	"jobs.h"
#include	"sym.h"
#include	"history.h"
#include	"edit.h"
#include	"timeout.h"

int	Hz;							/* L000 */

#ifdef	INTL							/* L001 begin */
/* nl_catd		catd = -1; */
#endif	/* INTL */						/* L001 end */

struct _nl_catd	*catd;

struct sh_scoped	st;
struct sh_static	sh;

#ifdef VSH
    struct	edit	editb;
#else
#   ifdef ESH
	struct	edit	editb;
#   endif /* ESH */
#endif	/* VSH */

struct history	*hist_ptr;
struct jobs	job;
int		sh_lastbase = 10; 
time_t		sh_mailchk = 600;
#ifdef TIMEOUT
    long		sh_timeout = TIMEOUT;
#else
    long		sh_timeout = 0;
#endif /* TIMEOUT */
char		io_tmpname[] = "/tmp/shxxxxxx.aaa";

#ifdef 	NOBUF
    char	_sibuf[IOBSIZE+1];
    char	_sobuf[IOBSIZE+1];
#endif	/* NOBUF */

struct fileblk io_stdin = { _sibuf, _sibuf, _sibuf, 0, IOREAD, 0, F_ISFILE};
struct fileblk io_stdout = { _sobuf, _sobuf, _sobuf+IOBSIZE, 0, IOWRT,2};
struct fileblk *io_ftable[NFILE] = { 0, &io_stdout, &io_stdout};

#ifdef MULTIBYTE
/*
 * These are default values.  They can be changed with CSWIDTH
 */

char int_charsize[] =
{
	1, CCS1_IN_SIZE, CCS2_IN_SIZE, CCS3_IN_SIZE,	/* input sizes */
	1, CCS1_OUT_SIZE, CCS2_OUT_SIZE, CCS3_OUT_SIZE	/* output widths */
};
#else
char int_charsize[] =
{
	1, 0, 0, 0,	/* input sizes */
	1, 0, 0, 0	/* output widths */
};
#endif /* MULTIBYTE */

