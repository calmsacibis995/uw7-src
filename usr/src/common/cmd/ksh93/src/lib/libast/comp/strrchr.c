#ident	"@(#)ksh93:src/lib/libast/comp/strrchr.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _lib_strrchr

NoN(strrchr)

#else

#undef	strrchr

#ifdef _lib_rindex

#undef	rindex

extern char*	rindex(const char*, int);

char*
strrchr(const char* s, int c)
{
	return(rindex(s, c));
}

#else

char*
strrchr(register const char* s, register int c)
{
	register const char*	r;

	r = 0;
	do if (*s == c) r = s; while(*s++);
	return((char*)r);
}

#endif

#endif
