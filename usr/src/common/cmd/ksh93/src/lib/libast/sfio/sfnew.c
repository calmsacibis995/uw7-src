#ident	"@(#)ksh93:src/lib/libast/sfio/sfnew.c	1.1"
#include	"sfhdr.h"

/*	Fundamental function to create a new stream.
**	The argument flags defines the type of stream and the scheme
**	of buffering.
**
**	Written by Kiem-Phong Vo (06/27/90)
*/

#if __STD_C
Sfio_t* sfnew(Sfio_t* oldf, Void_t* buf, int size, int file, int flags)
#else
Sfio_t* sfnew(oldf,buf,size,file,flags)
Sfio_t* oldf;	/* old stream to be reused */
Void_t*	buf;	/* a buffer to read/write, if NULL, will be allocated */
int	size;	/* buffer size if buf is given or desired buffer size */
int	file;	/* file descriptor to read/write from */
int	flags;	/* type of file stream */
#endif
{
	reg Sfio_t*	f;
	reg int		sflags;

	if(!(flags&SF_RDWR))
		return NIL(Sfio_t*);

	sflags = 0;
	if((f = oldf) )
	{	if(flags&SF_EOF)
			oldf = NIL(Sfio_t*);
		else if(f->mode&SF_AVAIL)
		{	/* only allow SF_STATIC to be already closed */
			if(!(f->flags&SF_STATIC) )
				return NIL(Sfio_t*);
			sflags = f->flags;
			oldf = NIL(Sfio_t*);
		}
		else
		{	/* reopening an open stream, close it first */
			if((f->mode&SF_RDWR) != f->mode && _sfmode(f,0,0) < 0)
				return NIL(Sfio_t*);
			sflags = f->flags;
			if(SFCLOSE(f) < 0)
				return NIL(Sfio_t*);
			if(f->data && ((flags&SF_STRING) || size >= 0) )
			{	if(sflags&SF_MALLOC)
					free((Void_t*)f->data);
				f->data = NIL(uchar*);
			}
			if(!f->data)
				sflags &= ~SF_MALLOC;
		}
	}

	if(!f)
	{	/* reuse a standard stream structure if possible */
		if(!(flags&SF_STRING) && file >= 0 && file <= 2)
		{	f = file == 0 ? sfstdin : file == 1 ? sfstdout : sfstderr;
			if(!(f->mode&SF_AVAIL) )
				f = NIL(Sfio_t*);
			else	sflags = f->flags;
		}

		if(!f && !SFALLOC(f))	/* make a new one */
			return NIL(Sfio_t*);
	}

	if(!oldf)
		SFCLEAR(f);

	/* stream type */
	f->mode = (flags&SF_READ) ? SF_READ : SF_WRITE;
	f->flags = (flags&SF_FLAGS) | ((flags&SF_RDWR) == SF_RDWR ? SF_BOTH : 0);
	f->flags |= (sflags&(SF_MALLOC|SF_STATIC));
	f->file = file;
	f->here = f->extent = 0L;
	f->getr = f->tiny[0] = 0;

	f->mode |= SF_INIT | (flags&SF_OPEN);
	if(size >= 0)
	{	f->size = size;
		f->data = size <= 0 ? NIL(uchar*) : (uchar*)buf;
	}
	f->endb = f->endr = f->endw = f->next = f->data;

	if(_Sfnotify)
		(*_Sfnotify)(f,SF_NEW,f->file);

	if(f->flags&SF_STRING)
		(void)_sfmode(f,f->mode&SF_RDWR,0);

	return f;
}
