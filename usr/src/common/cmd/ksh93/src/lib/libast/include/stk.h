#ident	"@(#)ksh93:src/lib/libast/include/stk.h	1.1"
#pragma prototyped
/*
 * David Korn
 * AT&T Bell Laboratories
 *
 * Interface definitions for a stack-like storage library
 *
 */

#ifndef _STK_H
#define _STK_H

#include <sfio.h>

#if _DLL_INDIRECT_DATA && !_DLL
#define _Stk_data	(*_Stak_data)
#else
#define _Stk_data	_Stak_data
#endif

extern Sfio_t		_Stk_data;
#define stkstd		(&_Stk_data)

#define	Stk_t		Sfio_t

#define STK_SMALL	1		/* argument to stkopen		*/

#define	stkptr(sp,n)	((char*)((sp)->data)+(n))
#define	stktell(sp)	((sp)->next-(sp)->data)
#define stkseek(sp,n)	((n)==0?(char*)((sp)->next=(sp)->data):_stkseek(sp,n))

extern Stk_t*		stkopen(int);
extern Stk_t*		stkinstall(Stk_t*, char*(*)(int));
extern int		stkclose(Stk_t*);
extern int		stklink(Stk_t*);
extern char*		stkalloc(Stk_t*, unsigned);
extern char*		stkcopy(Stk_t*,const char*);
extern char*		stkset(Stk_t*, char*, unsigned);
extern char*		_stkseek(Stk_t*, unsigned);
extern char*		stkfreeze(Stk_t*, unsigned);

#endif
