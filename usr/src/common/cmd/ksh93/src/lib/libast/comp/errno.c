#ident	"@(#)ksh93:src/lib/libast/comp/errno.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _tst_errno

NoN(errno)

#else

/*
 * this avoids multiple definitions with some libc's
 * that define both an ast library supplied routine and
 * errno in the same .o
 */

int     errno;

#endif
