#ident	"@(#)ksh93:src/lib/libast/sfio/sfscanf.c	1.1"
#include	"sfhdr.h"

/*	Read formated data from a stream
**
**	Written by Kiem-Phong Vo (06/27/90)
*/

#if __STD_C
sfscanf(Sfio_t *f, const char *form, ...)
#else
sfscanf(va_alist)
va_dcl
#endif
{
	va_list	args;
	reg int	rv;

#if __STD_C
	va_start(args,form);
#else
	reg Sfio_t	*f;
	reg char	*form;
	va_start(args);
	f = va_arg(args,Sfio_t*);
	form = va_arg(args,char*);
#endif

	rv = sfvscanf(f,form,args);
	va_end(args);
	return rv;
}

#if __STD_C
sfsscanf(const char *s, const char *form,...)
#else
sfsscanf(va_alist)
va_dcl
#endif
{
	va_list		args;
	Sfio_t		f;
	reg int		rv;
#if __STD_C
	va_start(args,form);
#else
	reg char	*s;
	reg char	*form;
	va_start(args);
	s = va_arg(args,char*);
	form = va_arg(args,char*);
#endif

	if(!s)
		return -1;

	/* make a fake stream */
	SFCLEAR(&f);
	f.flags = SF_STRING|SF_READ;
	f.mode = SF_READ;
	f.size = strlen((char*)s);
	f.data = f.next = f.endw = (uchar*)s;
	f.endb = f.endr = f.data+f.size;

	rv = sfvscanf(&f,form,args);
	va_end(args);

	return rv;
}
