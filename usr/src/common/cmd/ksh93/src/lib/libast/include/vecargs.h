#ident	"@(#)ksh93:src/lib/libast/include/vecargs.h	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * vector argument interface definitions
 */

#ifndef _VECARGS_H
#define _VECARGS_H

extern int		vecargs(char**, int*, char***);
extern char**		vecfile(const char*);
extern void		vecfree(char**, int);
extern char**		vecload(char*);
extern char**		vecstring(const char*);

#endif
