/*		copyright	"%c%" 	*/

#ident	"@(#)ksh:sh/defs.c	1.2.6.3"
#ident "$Header$"

/*
 * Ksh - AT&T Bell Laboratories
 * Written by David Korn
 * This file defines all the  read/write shell global variables
 */

#include	"defs.h"
#include	"jobs.h"
#include	"sym.h"
#include	"history.h"
#include	"edit.h"
#include	"timeout.h"


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

char	sibuf[IOBSIZE+1];
char	sobuf[IOBSIZE+1];

struct fileblk io_stdin = { sibuf, sibuf, sibuf, 0, IOREAD, 0, F_ISFILE};
struct fileblk io_stdout = { sobuf, sobuf, sobuf+IOBSIZE, 0, IOWRT,2};
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

