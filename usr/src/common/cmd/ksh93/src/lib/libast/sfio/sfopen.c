#ident	"@(#)ksh93:src/lib/libast/sfio/sfopen.c	1.1"
#include	"sfhdr.h"

/*	Open a file/string for IO.
**	If f is not nil, it is taken as an existing stream that should be
**	closed and its structure reused for the new stream.
**
**	Written by Kiem-Phong Vo (06/27/90)
*/

#if __STD_C
Sfio_t *sfopen(reg Sfio_t* f, const char* file, const char* mode)
#else
Sfio_t *sfopen(f,file,mode)
reg Sfio_t	*f;		/* old stream structure */
char		*file;		/* file/string to be opened */
reg char	*mode;		/* mode of the stream */
#endif
{
	int	fd, oldfd, oflags, sflags;

	if((sflags = _sftype(mode,&oflags)) == 0)
		return NIL(Sfio_t*);

	if(sflags&SF_STRING)
		fd = -1;
	else
	{	/* open the file */
		if(!file)
			return NIL(Sfio_t*);

#ifndef NO_OFLAGS
		while((fd = open((char*)file,oflags,0666)) < 0 && errno == EINTR)
			errno = 0;
#else
		while((fd = open(file,oflags&03)) < 0 && errno == EINTR)
			errno = 0;
		if(fd >= 0)
		{	if(oflags&O_TRUNC)
			{	reg int	tf;
				while((tf = creat(file,0666)) < 0 && errno == EINTR)
					errno = 0;
				CLOSE(tf);
			}
		}
		else if(oflags&O_CREAT)
		{	while((fd = creat(file,0666)) < 0 && errno == EINTR)
				errno = 0;
			if(!(oflags&O_WRONLY))
			{	/* the file now exists, reopen it for read/write */
				CLOSE(fd);
				while((fd = open(file,oflags&03)) < 0 && errno == EINTR)
					errno = 0;
			}
		}
#endif
		if(fd < 0)
			return NIL(Sfio_t*);
	}

	oldfd = (f && !(f->flags&SF_STRING)) ? f->file : -1;

	if(sflags&SF_STRING)
		f = sfnew(f,(char*)file,file ? (int)strlen((char*)file) : -1,fd,sflags);
	else if((f = sfnew(f,NIL(char*),-1,fd,sflags|SF_OPEN)) && oldfd >= 0)
		(void)sfsetfd(f,oldfd);

	return f;
}

#if __STD_C
_sftype(reg const char *mode, int *oflagsp)
#else
_sftype(mode, oflagsp)
reg char	*mode;
int		*oflagsp;
#endif
{
	reg int	sflags, oflags;

	if(!mode)
		return 0;

	/* construct the open flags */
	sflags = oflags = 0;
	while(1) switch(*mode++)
	{
	case 'w' :
		sflags |= SF_WRITE;
		oflags |= O_WRONLY | O_CREAT;
		if(!(sflags&SF_READ))
			oflags |= O_TRUNC;
		continue;
	case 'a' :
		sflags |= SF_WRITE | SF_APPEND;
		oflags |= O_WRONLY | O_APPEND | O_CREAT;
		continue;
	case 'r' :
		sflags |= SF_READ;
		oflags |= O_RDONLY;
		continue;
	case 's' :
		sflags |= SF_STRING;
		continue;
	case 'b' :
		oflags &= ~O_TEXT;
		oflags |= O_BINARY;
		continue;
	case 't' :
		oflags &= ~O_BINARY;
		oflags |= O_TEXT;
		continue;
	case '+' :
		if(sflags)
			sflags |= SF_READ|SF_WRITE;
		continue;
	default :
		if((sflags&SF_RDWR) == SF_RDWR)
			oflags = (oflags&~(O_RDONLY|O_WRONLY))|O_RDWR;
		if(!(oflags&O_BINARY) && (sflags&SF_READ))
			oflags |= O_TEXT;
		if(oflagsp)
			*oflagsp = oflags;
		if((sflags&(SF_STRING|SF_RDWR)) == SF_STRING)
			sflags |= SF_READ;
		return sflags;
	}
}
