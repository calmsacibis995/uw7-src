#ident	"@(#)ksh93:src/lib/libast/sfio/sfgetr.c	1.1"
#include	"sfhdr.h"

/*	Read a record delineated by a character.
**	The record length can be accessed via sfslen().
**	Note that the reg declarations below must be kept in
**	their relative order so that the code will configured
**	correctly on Vaxes to use "asm()".
**
**	Written by Kiem-Phong Vo (06/27/90)
*/

#if __STD_C
char* sfgetr(reg Sfio_t *f, reg int rc, int string)
#else
char* sfgetr(f,rc,string)
reg Sfio_t*	f;	/* stream to read from. r11 on vax		*/
reg int		rc;	/* record separator. r10 on Vax			*/
int		string; /* <0: get last, 0: rc as is, 1: rc to nul	*/
#endif
{
	reg int		n;		/* r9 on vax		*/
	reg uchar	*s, *ends, *us;	/* r8, r7, r6 on vax	*/
	reg int		un, found;
	reg Sfrsrv_t*	frs;

	/* buffer to be returned */
	frs = NIL(Sfrsrv_t*);
	us = NIL(uchar*);
	un = 0;
	found = 0;

	/* restore the byte changed by the last getr */
	if(f->mode&SF_GETR)
	{	f->mode &= ~SF_GETR;
		f->next[-1] = f->getr;
	}

	/* set the right mode */
	if(rc < 0 || (f->mode != SF_READ && _sfmode(f,SF_READ,0) < 0) )
		goto done;

	SFLOCK(f,0);

	if(string < 0) /* return the previously read string only */
	{	if((frs = _sfrsrv(f,0)) && (un = -frs->slen) > 0)
		{	us = frs->data;
			found = 1;
		}
		goto done;
	}

	while(!found)
	{	/* fill buffer if necessary */
		if((n = (ends = f->endb) - (s = f->next)) <= 0)
		{	/* for unseekable devices, peek-read 1 record */
			f->getr = rc;
			f->mode |= SF_RC;

			/* fill buffer the conventional way */
			if(SFRPEEK(f,s,n) <= 0)
			{	us = NIL(uchar*);
				goto done;
			}
			else
			{	ends = s+n;
				if(f->mode&SF_RC)
				{	s = ends[-1] == rc ? ends-1 : ends;
					goto do_copy;
				}
			}
		}

#if _vax_asm	/* rc is r10, n is r9, s is r8, ends is r7 */
		asm( "locc	r10,r9,(r8)" );	/* find rc */
		asm( "movl	r1,r8" );	/* set s to be where it is */
#else
#if _lib_memchr
		if(!(s = (uchar*)memchr((char*)s,rc,n)))
			s = ends;
#else
		while(s < ends)
		{	if(*s++ == rc)
			{	s -= 1;
				break;
			}
		}
#endif
#endif
	do_copy:
		if(s < ends)
		{	s += 1;		/* include the separator */
			found = 1;
			if(!us && (!string || !(f->flags&(SF_STRING|SF_BOTH))) )
			{	/* just returning the buffer */
				us = f->next;
				un = s - f->next;
				f->next = s;
				goto done;
			}
		}

		/* amount to be read */
		n = s - f->next;

		/* get internal buffer */
		if(!frs || frs->size < un+n+1)
		{	if(frs)
				frs->slen = un;
			if((frs = _sfrsrv(f,un+n+1)) != NIL(Sfrsrv_t*))
				us = frs->data;
			else
			{	us = NIL(uchar*);
				goto done;
			}
		}

		/* now copy data */
		s = us+un;
		un += n;
		ends = f->next;
		f->next += n;
#if vax_asm
		asm( "movc3	r9,(r7),(r8)" );
#else
		MEMCPY(s,ends,n);
#endif
	}

done:
	_Sfi = un;
	if(found && string > 0)
	{	us[un-1] = '\0';
		if(us >= f->data && us < f->endb)
		{	f->getr = rc;
			f->mode |= SF_GETR;
		}
	}

	/* prepare for a call with string < 0 */
	if(frs)
		frs->slen = found ? 0 : -un;

	SFOPEN(f,0);
	return (char*)us;
}
