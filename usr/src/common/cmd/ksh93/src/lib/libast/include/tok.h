#ident	"@(#)ksh93:src/lib/libast/include/tok.h	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * token stream interface definitions
 */

#ifndef _TOK_H
#define _TOK_H

#include <ast.h>

extern Sfio_t*		tokline(const char*, int, int*);
extern int		tokscan(char*, char**, const char*, ...);
extern char*		tokopen(char*, int);
extern void		tokclose(char*);
extern char*		tokread(char*);

#endif
