#ident	"@(#)ksh93:src/lib/libast/sfio/sfclrlock.c	1.1"
#include	"sfhdr.h"

/*	Function to clear a locked stream.
**	This is useful for programs that longjmp from the mid of an sfio function.
**	There is no guarantee on data integrity in such a case.
**
**	Written by Kiem-Phong Vo (07/20/90).
*/
#if __STD_C
sfclrlock(reg Sfio_t* f)
#else
sfclrlock(f)
reg Sfio_t	*f;
#endif
{
	/* already closed */
	if(f->mode&SF_AVAIL)
		return 0;

	/* clear these bits */
	f->flags &= ~(SF_ERROR|SF_EOF);

	if(!(f->mode&(SF_LOCK|SF_PEEK)) )
		return (f->flags&SF_FLAGS);

	/* clear peek locks */
	f->mode &= ~SF_PEEK;
	if(f->mode&SF_PKRD)
	{	f->here -= f->endb-f->next;
		f->endb = f->next;
		f->mode &= ~SF_PKRD;
	}

	f->mode &= (SF_RDWR|SF_INIT|SF_POOL|SF_PUSH|SF_SYNCED|SF_STDIO);

	return _sfmode(f,0,0) < 0 ? 0 : (f->flags&SF_FLAGS);
}
