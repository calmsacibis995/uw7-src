#ident	"@(#)ksh93:src/lib/libast/sfio/sfstrtod.c	1.1"
#include	"sfhdr.h"

/*	Convert a Double_t value represented in an ASCII format into
**	the internal Double_t representation.
**
**	Written by David Korn and Kiem-Phong Vo (06/27/90)
*/

#if !_sfio_cvt
int	_Sfstrtod_already_defined;
#else

#define BATCH	(2*sizeof(int))	/* accumulate this many digits at a time */
#define IPART		0	/* doing integer part */
#define FPART		1	/* doing fractional part */
#define EPART		2	/* doing exponent part */

#if __STD_C
static Double_t pow10(reg int n)
#else
static Double_t pow10(n)
reg int	n;
#endif
{
	reg int		m, pow;
	reg Double_t	dval, d, *pow10;

	/* set up look up table */
	if((pow = n) < 0)
	{	pow10 = _Sfneg10;
		pow = -pow;
	}
	else	pow10 = _Sfpos10;

	/* reduce to a low exponent */
	dval = 1.;
	if(pow >= SF_MAXPOW10)
	{	d = pow10[SF_MAXEXP10-1]*pow10[SF_MAXEXP10-1];	
		for(m = pow/SF_MAXPOW10; m > 0; --m)
			dval *= d;
		pow = pow%SF_MAXPOW10;
	}

	/* fast loop for the rest */
	for(m = 1; m <= pow;)
	{	if(m&pow)
			dval *= *pow10;
		pow10 += 1;
		if(n < 0)
			pow >>= 1;
		else	m <<= 1;
	}

	return dval;
}

#if __STD_C
Double_t _sfstrtod(reg const char* s, char** retp)
#else
Double_t _sfstrtod(s,retp)
reg char*	s;	/* string to convert */
char**		retp;	/* to return the remainder of string */
#endif
{
	reg int		n, c, m;
	reg int		mode, fexp, sign, expsign;
	Double_t	dval;
#if _lib_locale
	struct lconv*	lv;
	int		decpoint = 0;
	GETDECIMAL(decpoint,lv);
#else
#define decpoint	'.'
#endif

	/* skip initial blanks */
	while(isspace(*s))
		++s;

	/* get the sign */
	if((sign = (*s == '-')) || *s == '+')
		s += 1;

	mode = IPART;
	fexp = expsign = 0;
	dval = 0.;
	while(*s)
	{	/* accumulate a handful of the digits */
		for(m = BATCH, n = 0; m > 0; --m, ++s)
		{	/* get and process a char */
			c = *s;
			if(isdigit(c))
				n = 10*n + (c - '0');
			else	break;
		}

		/* number of digits accumulated */
		m = BATCH-m;

		if(mode == IPART)
		{	/* doing the integer part */
			if(dval == 0.)
				dval = (Double_t)n;
			else	dval = dval*pow10(m) + (Double_t)n;
		}
		else if(mode == FPART)
		{	/* doing the fractional part */
			fexp -= m;
			if(n > 0)
				dval += n*pow10(fexp);
		}
		else if(n)
		{	/* doing the exponent part */
			if(expsign)
				n = -n;
			dval *= pow10(n);
		}

		if(!c)
			break;

		if(m < BATCH)
		{	/* detected a non-digit */
			if(c == decpoint)
			{	/* start the fractional part or no match */
				if(mode != IPART)
					break;
				mode = FPART;
				s += 1;
			}
			else if(c == 'e' || c == 'E')
			{	if(mode == EPART)
					break;
				mode = EPART;
				c = *++s;
				if((expsign = (c == '-')) || c == '+')
					s += 1;
			}
			else	break;
		}
	}

	if(retp)
		*retp = (char*)s;
	return sign ? -dval : dval;
}

#endif /* !_sfio_cvt */
