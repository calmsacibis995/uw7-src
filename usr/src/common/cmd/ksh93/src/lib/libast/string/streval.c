#ident	"@(#)ksh93:src/lib/libast/string/streval.c	1.1"
#pragma prototyped
/*
 * obsolete streval() interface to strexpr()
 */

#include <ast.h>

typedef struct
{
	long	(*convert)(const char*, char**);
} Handle_t;

static long
userconv(const char* s, char** end, void* handle)
{
	return((*((Handle_t*)handle)->convert)(s, end));
}

long
streval(const char* s, char** end, long(*convert)(const char*, char**))
{
	Handle_t	handle;

	return((handle.convert = convert) ? strexpr(s, end, userconv, &handle) : strexpr(s, end, NiL, NiL));
}
