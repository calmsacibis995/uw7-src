#ident	"@(#)ksh93:src/lib/libast/sfio/sfnputc.c	1.1"
#include	"sfhdr.h"

/*	Write out a character n times
**
**	Written by Kiem-Phong Vo (06/27/90)
*/

#if __STD_C
sfnputc(reg Sfio_t* f, reg int c, reg int n)
#else
sfnputc(f,c,n)
reg Sfio_t	*f;	/* file to write */
reg int		c;	/* char to be written */
reg int		n;	/* number of time to repeat */
#endif
{
	reg uchar	*ps;
	reg int		p, w;
	uchar		buf[128];
	reg int		local;

	GETLOCAL(f,local);
	if(SFMODE(f,local) != SF_WRITE && _sfmode(f,SF_WRITE,local) < 0)
		return -1;

	SFLOCK(f,local);

	/* write into a suitable buffer */
	if((p = (f->endb-(ps = f->next))) < n)
		{ ps = buf; p = sizeof(buf); }
	if(p > n)
		p = n;
	MEMSET(ps,c,p);
	ps -= p;

	w = n;
	if(ps == f->next)
	{	/* simple sfwrite */
		f->next += p;
		if(c == '\n')
			(void)SFFLSBUF(f,-1);
		goto done;
	}

	for(;;)
	{	/* hard write of data */
		if((p = SFWRITE(f,(Void_t*)ps,p)) <= 0 || (n -= p) <= 0)
		{	w -= n;
			goto done;
		}
		if(p > n)
			p = n;
	}
done :
	SFOPEN(f,local);
	return w;
}
