/*		copyright	"%c%" 	*/

#ident	"@(#)getmessage.c	1.2"
#ident	"$Header$"
/* LINTLIBRARY */
/*
*/

#if	defined(__STDC__)
# include	<stdarg.h>
#else
# include	<varargs.h>
#endif

/* VARARGS */
#if	defined(__STDC__)
int getmessage ( char * buf, short type, ... )
#else
int getmessage (buf, type, va_alist)
    char	*buf;
    short	type;
    va_dcl
#endif
{
    va_list	arg;
    int		rval;
    int	_getmessage();

#if	defined(__STDC__)
    va_start(arg, type);
#else
    va_start(arg);
#endif
    
    rval = _getmessage(buf, type, arg);
    va_end(arg);
    return(rval);
}
