#ident	"@(#)ksh93:src/lib/libast/sfio/sfputd.c	1.1"
#include	"sfhdr.h"

/*	Write out a double value in a portable format
**
**	Written by Kiem-Phong Vo (08/05/90)
*/

#if __STD_C
_sfputd(Sfio_t* f, reg double v)
#else
_sfputd(f,v)
Sfio_t		*f;
reg double	v;
#endif
{
#define N_ARRAY		(16*sizeof(double))
	reg int		n, w;
	reg double	x;
	reg uchar	*s, *ends;
	int		exp;
	uchar		c[N_ARRAY];

	if(f->mode != SF_WRITE && _sfmode(f,SF_WRITE,0) < 0)
		return -1;
	SFLOCK(f,0);

	/* get the sign of v */
	if(v < 0.)
	{	v = -v;
		n = 1;
	}
	else	n = 0;

	/* make the magnitude of v < 1 */
	if(v != 0.)
		v = frexp(v,&exp);
	else	exp = 0;

	/* code the sign of v and exp */
	if((w = exp) < 0)
	{	n |= 02;
		w = -w;
	}

	/* write out the signs and the exp */
	SFOPEN(f,0);
	if(sfputc(f,n) < 0 || (w = sfputu(f,w)) < 0)
		return -1;
	SFLOCK(f,0);
	w += 1;

	s = (ends = &c[0])+sizeof(c);
	while(s > ends)
	{	/* get 2^SF_PRECIS precision at a time */
		n = (int)(x = ldexp(v,SF_PRECIS));
		*--s = n|SF_MORE;
		if((v = x-n) <= 0.)
			break;
	}

	/* last byte is not SF_MORE */
	ends = &c[0] + sizeof(c) -1;
	*ends &= ~SF_MORE;

	/* write out coded bytes */
	n = ends - s + 1;
	w = SFWRITE(f,(Void_t*)s,n) == n ? w+n : -1;

	SFOPEN(f,0);
	return w;
}
