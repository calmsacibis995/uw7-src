#ident	"@(#)ksh93:src/lib/libast/obsolete/tablook.c	1.1"
#pragma prototyped

/* OBSOLETE 950401 -- use strlook */

#include <ast.h>

void*
tablook(const void* tab, int siz, register const char* name)
{
	return(strlook(tab, siz, name));
}
