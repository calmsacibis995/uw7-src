#ident	"@(#)ksh93:src/lib/libcmd/wc.h	1.1"
#pragma prototyped
/*
 * David Korn
 * AT&T Bell Laboratories
 *
 * header for wc library interface
 */

#ifndef _WC_H
#define _WC_H

#include <ast.h>

#define WC_LINES	1
#define WC_WORDS	2
#define WC_CHARS	4
#define WC_MBYTE	8

typedef struct
{
	signed char space[1<<CHAR_BIT];
	long words;
	long lines;
	long chars;
} Wc_t;


extern Wc_t*	wc_init(char*);
extern int	wc_count(Wc_t*, Sfio_t*);

#endif /* _WC_H */
