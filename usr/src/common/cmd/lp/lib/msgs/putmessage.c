/*		copyright	"%c%" 	*/

#ident	"@(#)putmessage.c	1.2"
#ident	"$Header$"
/* LINTLIBRARY */

#if	defined(__STDC__)
# include	<stdarg.h>
#else
# include	<varargs.h>
#endif

/* VARARGS */
#if	defined(__STDC__)
int putmessage(char * buf, short type, ... )
#else
int putmessage(buf, type, va_alist)
    char	*buf;
    short	type;
    va_dcl
#endif
{
    int		size;
    va_list	arg;
    int		_putmessage();

#if	defined(__STDC__)
    va_start(arg, type);
#else
    va_start(arg);
#endif

    size = _putmessage(buf, type, arg);
    va_end(arg);
    return(size);
}
