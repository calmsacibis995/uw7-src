#ident	"@(#)ksh93:src/lib/libast/stdio/stdprintf.c	1.1"
#include	"sfhdr.h"
#include	"stdio.h"


/*	printf function
**
**	Written by Kiem-Phong Vo (12/10/90)
*/

#if __STD_C
_stdprintf(const char *form, ...)
#else
_stdprintf(va_alist)
va_dcl
#endif
{
	va_list		args;
	reg int		rv;

#if __STD_C
	va_start(args,form);
#else
	reg char	*form;
	va_start(args);
	form = va_arg(args,char*);
#endif

	rv = sfvprintf(sfstdout,form,args);

	va_end(args);
	return rv;
}
