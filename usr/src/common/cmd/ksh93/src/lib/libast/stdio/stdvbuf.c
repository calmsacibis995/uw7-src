#ident	"@(#)ksh93:src/lib/libast/stdio/stdvbuf.c	1.1"
#include	"sfhdr.h"
#include	"stdio.h"

/*	Stdio function setvbuf()
**
**	Written by Kiem-Phong Vo (12/10/90)
*/

#if __STD_C
_stdsetvbuf(Sfio_t* f, char *buf, int type, int size)
#else
_stdsetvbuf(f,buf,type,size)
Sfio_t	*f;
char	*buf;
int	type;
int	size;
#endif
{
	if(type == _IOLBF)
		sfset(f,SF_LINE,1);
	else if((f->flags&SF_STRING))
		return -1;
	else if(type == _IONBF)
	{	sfsync(f);
		sfsetbuf(f,NIL(Void_t*),0);
	}
	else if(type == _IOFBF)
	{	if(size == 0)
			size = SF_BUFSIZE;
		sfsync(f);
		sfsetbuf(f,(Void_t*)buf,size);
	}

	return 0;
}
