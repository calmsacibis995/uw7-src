#ident	"@(#)ksh93:src/lib/libast/sfio/sfnotify.c	1.1"
#include	"sfhdr.h"


/*	Set the function to be called when a stream is opened or closed
**
**	Written by Kiem-Phong Vo (01/06/91)
*/
#if __STD_C
sfnotify(void (*notify)(Sfio_t*, int, int))
#else
sfnotify(notify)
void	(*notify)();
#endif
{
	_Sfnotify = notify;
	return 0;
}
