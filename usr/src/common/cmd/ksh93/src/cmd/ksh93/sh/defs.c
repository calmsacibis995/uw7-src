#ident	"@(#)ksh93:src/cmd/ksh93/sh/defs.c	1.1"
/*
 * Ksh - AT&T Bell Laboratories
 * Written by David Korn
 * This file defines all the  read/write shell global variables
 */

#include	"defs.h"
#include	"jobs.h"
#include	"shlex.h"
#include	"edit.h"
#include	"timeout.h"

struct sh_static	sh;
struct shlex_t		shlex;

/* reserve room for writable state table */
char *sh_lexstates[ST_NONE];

#if defined(SHOPT_VSH) || defined(SHOPT_ESH)
    struct	edit	editb;
#endif	/* SHOPT_VSH||SHOPT_ESH */

struct jobs	job;
time_t		sh_mailchk = 600;

#ifdef SHOPT_MULTIBYTE
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
#endif /* SHOPT_MULTIBYTE */

