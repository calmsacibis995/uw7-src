#ident	"@(#)sgs:libsgs/common/pfmt.c	1.6"

#ifdef __STDC__
	#pragma weak pfmt = _pfmt
#endif
#include "synonyms.h"
#include <pfmt.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>

/* pfmt() - format and print */

/*ARGSUSED*/
int
#ifdef __STDC__
pfmt(struct _FILE_ *stream, long flag, const char *format, ...)
#else
pfmt(stream, flag, format, va_alist)
struct _FILE_ *stream;
long flag;
const char *format;
va_dcl
#endif
{
	va_list args;
	const char *ptr;
	int status;
	register int length = 0;

#ifdef __STDC__
	va_start(args, format);
#else
	va_start(args);
#endif

	if (!(flag & MM_NOGET) && format) {
		ptr = format;
		while(*ptr++ != ':');
		*ptr++;
		while (isdigit(*ptr++));
		
		format = ptr;

	}

	if (stream){
		if ((status = vfprintf(stream, format, args)) < 0)
			return -1;
		length += status;
	}

	return length;
}
