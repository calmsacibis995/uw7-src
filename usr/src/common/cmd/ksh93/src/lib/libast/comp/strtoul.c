#ident	"@(#)ksh93:src/lib/libast/comp/strtoul.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _lib_strtoul

NoN(strtoul)

#else

unsigned long
strtoul(const char* str, char** ptr, int base)
{
	return((unsigned long)strtol(str, ptr, base));
}

#endif
