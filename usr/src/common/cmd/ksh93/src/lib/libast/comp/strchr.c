#ident	"@(#)ksh93:src/lib/libast/comp/strchr.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _lib_strchr

NoN(strchr)

#else

#undef	strchr

#ifdef _lib_index

#undef	index

extern char*	index(const char*, int);

char*
strchr(const char* s, int c)
{
	return(index(s, c));
}

#else

char*
strchr(register const char* s, register int c)
{
	do if (*s == c) return((char*)s); while(*s++);
	return(0);
}

#endif

#endif
