#ident	"@(#)ksh93:src/lib/libast/vmalloc/vmstrdup.c	1.1"
#pragma prototyped

#include <ast.h>
#include <vmalloc.h>

/*
 * return a copy of s using vmalloc
 */

char*
vmstrdup(Vmalloc_t* v, register const char* s)
{
	register char*	t;
	register int	n;

	return((t = vmalloc(v, n = strlen(s) + 1)) ? (char*)memcpy(t, s, n) : (char*)0);
}
