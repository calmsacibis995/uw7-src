#ident	"@(#)ksh93:src/lib/libast/sfio/sfecvt.c	1.1"
#include	"sfhdr.h"

#if __STD_C
char* sfecvt(double dval, int n_digit, int* decpt, int* sign)
#else
char* sfecvt(dval,n_digit,decpt,sign)
double	dval;		/* value to convert */
int	n_digit;	/* number of digits wanted */
int*	decpt;		/* to return decimal point */
int*	sign;		/* to return sign */
#endif
{
	return _sfcvt((Double_t)dval,n_digit,decpt,sign,1);
}
