#ident	"@(#)ksh93:src/lib/libast/sfio/sfread.c	1.1"
#include	"sfhdr.h"

/*	Read n bytes from a stream into a buffer
**	Note that the reg declarations below must be kept in
**	their relative order so that the code will configured
**	correctly on Vaxes to use "asm()".
**
**	Written by Kiem-Phong Vo (06/27/90)
*/

#if __STD_C
sfread(reg Sfio_t* f, Void_t* buf, reg int n)
#else
sfread(f,buf,n)
reg Sfio_t*	f;	/* read from this stream. r11 on Vax	*/
Void_t*		buf;	/* buffer to read into			*/
reg int		n;	/* number of bytes to be read. r10	*/
#endif
{
	reg char*	s;	/* Vax r9	*/
	reg uchar*	next;	/* Vax r8	*/
	reg int		r;	/* Vax r7	*/
	reg char*	begs;
	reg int		local;

	GETLOCAL(f,local);

	/* release peek lock */
	if(f->mode&SF_PEEK)
	{	if(!(f->mode&SF_READ) || (uchar*)buf != f->next)
			return -1;

		f->mode &= ~SF_PEEK;
		if(f->mode&SF_PKRD)
		{	/* actually read the data now */
			f->mode &= ~SF_PKRD;
			if(n > 0 && (n = read(f->file,(char*)f->data,n)) < 0)
				n = 0;
			f->endb = f->data+n;
			f->here += n;
		}

		f->next += n;
		f->endr = f->endb;
		return n;
	}

	s = begs = (char*)buf;
	for(;; f->mode &= ~SF_LOCK)
	{	/* check stream mode */
		if(SFMODE(f,local) != SF_READ && _sfmode(f,SF_READ,local) < 0)
			return s > begs ? s-begs : -1;

		if(n <= 0)
			break;

		SFLOCK(f,local);

		/* determine current amount of buffered data */
		if((r = f->endb - f->next) > n)
			r = n;

		if(r > 0)
		{	/* copy data already in buffer */
			next = f->next;

#if _vax_asm		/* s is r9, next is r8, r is r7 */
			asm( "movc3	r7,(r8),(r9)" );
			s += r;
			next += r;
#else
			MEMCPY(s,next,r);
#endif
			f->next = next;
			if((n -= r) <= 0)
				break;
		}
		else if((f->flags&(SF_STRING|SF_MMAP)) ||
			(n < f->size && !(f->extent < 0 && (f->flags&SF_SHARE))) )
		{	if(SFFILBUF(f,-1) <= 0)
				break;
		}
		else
		{	/* stream buf is empty, do a direct read to user's buf */
			f->next = f->endb = f->data;
			if(f->extent < 0 && (f->flags&SF_SHARE) )
				r = n;
			else	r = n < _Sfpage ? n : (n/_Sfpage)*_Sfpage;
			if((r = SFRD(f,s,r,f->disc)) == 0)
				break;
			else if(r > 0)
			{	s += r;
				if((n -= r) <= 0)
					break;
			}
		}
	}

	SFOPEN(f,local);
	return s-begs;
}
