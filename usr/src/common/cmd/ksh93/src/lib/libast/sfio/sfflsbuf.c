#ident	"@(#)ksh93:src/lib/libast/sfio/sfflsbuf.c	1.1"
#include	"sfhdr.h"

/*	Write a buffer out to a file descriptor or
**	extending a buffer for a SF_STRING stream.
**
**	Written by Kiem-Phong Vo (06/27/90)
*/

#if __STD_C
_sfflsbuf(reg Sfio_t* f, reg int c)
#else
_sfflsbuf(f,c)
reg Sfio_t	*f;	/* write out the buffered content of this stream */
reg int		c;	/* if c>=0, c is also written out */ 
#endif
{
	reg int		n, w;
	reg uchar	*data;
	uchar		outc;
	reg int		local;
	int		inpc = c;

	GETLOCAL(f,local);

	for(;; f->mode &= ~SF_LOCK)
	{	/* check stream mode */
		if(SFMODE(f,local) != SF_WRITE && _sfmode(f,SF_WRITE,local) < 0)
			return -1;
		SFLOCK(f,local);

		/* current data extent */
		n = f->next - (data = f->data);

		if(n == (f->endb-f->data) && (f->flags&SF_STRING))
		{	/* extend string stream buffer */
			(void)SFWR(f,data,1,f->disc);

			/* !(f->flags&SF_STRING) is required because exception
			   handlers may turn a string stream to a file stream */
			if(f->next < f->endb || !(f->flags&SF_STRING) )
				n = f->next - (data = f->data);
			else
			{	SFOPEN(f,local);
				return -1;
			}
		}

		if(c >= 0)
		{	/* write into buffer */
			if(n < (f->endb - (data = f->data)))
			{	*f->next++ = c;
				if(c != '\n' ||
				   !(f->flags&SF_LINE) || (f->flags&SF_STRING))
					break;
				c = -1;
				n += 1;
			}
			else if(n == 0)
			{	/* unbuffered io */
				outc = (uchar)c;
				data = &outc;
				c = -1;
				n = 1;
			}
		}

		/* writing data */
		if(n == 0 || (f->flags&SF_STRING))
			break;
		else if((w = SFWR(f,data,n,f->disc)) > 0)
		{	if((n -= w) > 0) /* save unwritten data, then resume */
				memcpy((char*)f->data,(char*)data+w,n);
			f->next = f->data+n;
			if(n == 0 && c < 0)
				break;
		}
		else if(w == 0)
		{	SFOPEN(f,local);
			return -1;
		}
		else if(c < 0)
			break;
	}

	SFOPEN(f,local);
	return inpc < 0 ? (f->endb-f->next) : inpc;
}
