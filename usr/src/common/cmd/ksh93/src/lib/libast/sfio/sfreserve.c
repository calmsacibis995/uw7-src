#ident	"@(#)ksh93:src/lib/libast/sfio/sfreserve.c	1.1"
#include	"sfhdr.h"

/*	Reserve a segment of data or buffer.
**
**	Written by Kiem-Phong Vo (01/15/93).
*/

#if __STD_C
Void_t* sfreserve(reg Sfio_t* f, int size, int lock)
#else
Void_t* sfreserve(f,size,lock)
reg Sfio_t	*f;	/* file to peek */
int		size;	/* size of peek */
int		lock;	/* 1 to lock stream and not advance pointer */
#endif
{
	reg int		n;
	reg Sfrsrv_t*	frs;
	reg int		mode;

	/* initialize io states */
	frs = NIL(Sfrsrv_t*);
	_Sfi = -1;

	if(lock && size == 0)
	{	/* this case only returns the current status and lock */
		if((f->mode&SF_RDWR) != f->mode && _sfmode(f,0,0) < 0)
			return NIL(Void_t*);

		SFLOCK(f,0);
		if((n = f->endb - f->next) < 0)
			n = 0;
		if(!f->data)
			frs = _sfrsrv(f,0);
		goto done;
	}

	/* iterate until get to a stream that has data or buffer space */
	for(;;)
	{	/* prefer read mode so that data is always valid */
		if(!(mode = (f->flags&SF_READ)) )
			mode = SF_WRITE;
		if(f->mode != mode && _sfmode(f,mode,0) < 0)
		{	n = -1;
			goto done;
		}

		SFLOCK(f,0);

		if((n = f->endb - f->next) < 0)	/* possible for string+rw */
			n = 0;

		if(n > 0 && n >= size)	/* all done */
			break;

		/* do a buffer refill or flush */
		if(f->mode&SF_WRITE)
			(void)SFFLSBUF(f, -1);
		/* make sure no peek-read if there is data in buffer */
		else if(n == 0 || f->extent >= 0 || !(f->flags&SF_SHARE))
		{	if(lock && n == 0 && f->extent < 0 && (f->flags&SF_SHARE) )
				f->mode |= SF_RV;
			(void)SFFILBUF(f, size <= 0 ? -1 : (size-n) );
		}

		/* now have data */
		if((n = f->endb - f->next) > 0)
			break;
		else if(n < 0)
			n = 0;

		/* this type of stream requires immediate return */
		if(lock && f->extent < 0 && (f->flags&SF_SHARE) )
			break;

		/* this test fails only if unstacked to an opposite
		   stream or if SF_PKRD was set during _sffilbuf() */
		if((f->mode&mode) != 0)
			break;
	}

	/* try to accomodate request size */	
	if(n > 0 && n < size && (f->mode&mode) != 0 )
	{
		if(f->flags&SF_STRING)
		{	if((f->mode&SF_WRITE) && (f->flags&SF_MALLOC) )
			{	(void)SFWR(f,f->next,size,f->disc);
				n = f->endb - f->next;
			}
		}
		else if(f->mode&SF_WRITE)
		{	if(lock && (frs = _sfrsrv(f,size)) )
				n = size;
		}
		/* (f->extent - f->here) >= (size-n) ensures no stack pop */
		else if(!(f->flags&SF_MMAP) && !lock && f->extent >= 0L &&
			(f->extent - f->here) >= (size-n) && (frs = _sfrsrv(f,size)) )
		{	if((n = SFREAD(f,(Void_t*)frs->data,size)) < size)
			{	if(n > 0)
					(void)SFSEEK(f,(long)(-n),1);
				frs = NIL(Sfrsrv_t*);
				n = f->endb - f->next;
			}
		}
	}

done:
	/* return true buffer size */
	_Sfi = n;

	SFOPEN(f,0);

	if(n < 0 || (size > 0 && n < size) || (size < 0 && n == 0) )
		return NIL(Void_t*);
	else
	{	reg Void_t* rsrv = frs ? (Void_t*)frs->data : (Void_t*)f->next;

		if(lock && rsrv)
		{	f->mode |= SF_PEEK;
			f->endr = f->endw = f->data;
		}
		else if(n == 0)
			rsrv = NIL(Void_t*);
		else if(!frs)
			f->next += (size >= 0 ? size : n);

		return rsrv;
	}
}
