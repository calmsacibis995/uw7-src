#ident	"@(#)ksh93:src/lib/libast/sfio/sfexcept.c	1.1"
#include	"sfhdr.h"

/*	Function to handle io exceptions.
**	Written by Kiem-Phong Vo (8/18/90)
*/
#if __STD_C
_sfexcept(reg Sfio_t* f, reg int type, reg int io, reg Sfdisc_t* disc)
#else
_sfexcept(f,type,io,disc)
reg Sfio_t	*f;	/* stream where the exception happened */
reg int		type;	/* io type that was performed */
reg int		io;	/* the io return value that indicated exception */
reg Sfdisc_t	*disc;	/* discipline in use */
#endif
{
	reg int		d, local, lock;
	reg uchar	*data;

	GETLOCAL(f,local);
	lock = f->mode&SF_LOCK;

	if(local && io <= 0)
		f->flags |= io < 0 ? SF_ERROR : SF_EOF;

	if(disc && disc->exceptf)
	{	/* let the stream be generally accessible for this duration */
		if(local && lock)
			SFOPEN(f,0);

		/* so that exception handler knows what we are asking for */
		_Sfi = io;
		d = (*(disc->exceptf))(f,type,disc);

		/* relock if necessary */
		if(local && lock)
			SFLOCK(f,0);

		if(io > 0 && !(f->flags&SF_STRING) )
			return d;
		else if(d < 0)
			return SF_EDONE;
		else if(d > 0)
			return SF_EDISC;
	}

	if(f->flags&SF_STRING)
	{	if(type == SF_READ)
			goto chk_stack;
		else if(type != SF_WRITE && type != SF_SEEK)
			return SF_EDONE;
		if(local && io >= 0)
		{	if(f->size >= 0 && !(f->flags&SF_MALLOC))
				goto chk_stack;
			/* extend buffer */
			if((d = f->size) < 0)
				d = 0;
			if((io -= d) <= 0)
				io = SF_GRAIN;
			d = ((d+io+SF_GRAIN-1)/SF_GRAIN)*SF_GRAIN;
			if(f->size > 0)
				data = (uchar*)realloc((char*)f->data,d);
			else	data = (uchar*)malloc(d);
			if(!data)
				goto chk_stack;
			f->endb = data + d;
			f->next = data + (f->next - f->data);
			f->endr = f->endw = f->data = data;
			f->size = d;
		}
		return SF_EDISC;
	}

	if(errno == EINTR)
	{	/* if just an interrupt, we can continue */
		errno = 0;
		f->flags &= ~(SF_EOF|SF_ERROR);
		return SF_ECONT;
	}

chk_stack:
	if(local && f->push &&
	   ((type == SF_READ  && f->next >= f->endb) ||
	    (type == SF_WRITE && f->next <= f->data)))
	{	/* pop the stack */
		reg Sfio_t	*pf;

		if(lock)
			SFOPEN(f,0);

		/* pop and close */
		pf = (*_Sfstack)(f,NIL(Sfio_t*));
		if((d = sfclose(pf)) < 0) /* can't close, restack */
			(*_Sfstack)(f,pf);

		if(lock)
			SFLOCK(f,0);

		return d < 0 ? SF_EDONE : SF_ESTACK;
	}

	return SF_EDONE;
}
