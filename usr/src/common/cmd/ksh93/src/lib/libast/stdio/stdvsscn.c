#ident	"@(#)ksh93:src/lib/libast/stdio/stdvsscn.c	1.1"
#include	"sfhdr.h"

#if __STD_C
_stdvsscanf(char *s, const char *form, va_list args)
#else
_stdvsscanf(s,form,args)
register char	*s;
register char	*form;
va_list		args;
#endif
{
	Sfio_t	f;

	if(!s)
		return -1;

	/* make a fake stream */
	SFCLEAR(&f);
	f.flags = SF_STRING|SF_READ;
	f.mode = SF_READ;
	f.size = strlen((char*)s);
	f.data = f.next = f.endr = (uchar*)s;
	f.endb = f.endw = f.data+f.size;

	return sfvscanf(&f,form,args);
}
