#ident	"@(#)ksh93:src/lib/libast/include/find.h	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * fast find interface definitions
 */

#ifndef _FIND_H
#define _FIND_H

extern void*		findopen(const char*);
extern char*		findnext(void*);
extern void		findclose(void*);

#endif
