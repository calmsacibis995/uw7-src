#ident	"@(#)libm:i386/machfp.h	1.3"

#if _IEEE

/* byte order with low order bits at lowest address */
/* double precision */
typedef  union {
	struct {
		unsigned  lo	:32;
		unsigned  hi	:20;
		unsigned  exp	:11;
		unsigned  sign	:1;
	} fparts;
	struct {
		unsigned  lo	:32;
		unsigned  hi	:19;
		unsigned  qnan_bit	:1;
		unsigned  exp	:11;
		unsigned  sign	:1;
	} nparts;
	struct {
		unsigned  lo	:32;
		unsigned  hi	:32;
	} fwords;
	double	d;
} _dval;

/* single precision */
typedef  union {
	struct {
		unsigned fract	:23;
		unsigned exp	:8;
		unsigned sign	:1;
	} fparts;
	struct {
		unsigned fract	:22;
		unsigned qnan_bit	:1;
		unsigned exp	:8;
		unsigned sign	:1;
	} nparts;
	unsigned long	fword;
	float	f;
} _fval;

/* break up mantissa of float into 15 bit chunks */
/* 386 byte order */

union	rdval {
	double	d;
	struct {
		unsigned int	p4 : 15;
		unsigned int	p3 : 15;
		unsigned int	p2b : 2;
		unsigned int	p2a : 13;
		unsigned int	p1 : 7;
		unsigned int 	exp : 11; 
		unsigned int	sgn : 1;
	} dp;
} ;

union	rfval {
	float	d;
	struct {
		unsigned int	p2 : 15;
		unsigned int	p1 : 8;
		unsigned int 	exp : 8; 
		unsigned int	sgn : 1;
	} dp;
} ;

/*
 * A quicker way of checking whether the exponent is 0x7ff than 
 * checking if EXPONENT(X) == MAXEXP (see fpparts.h)
 */
#ifndef NANorINF
#define NANorINF(X) ((((int *)&X)[1] & 0x7ff00000) == 0x7ff00000)
#endif

/*
 * A quicker way of checking whether a double is an infinity given that
 * NANorINF returns true than checking that !(HIFRACTION(X) && !(LOWORD(X)
 */
#ifndef INF
#define INF(X)  ( !(((int*) &X )[0] | ( ((int*) &X)[1] & 0x000fffff) ) )
#endif

#ifndef FNANorINF
#define FNANorINF(X)	(((((_fval *)&(X))->fword) & 0x7f800000) == 0x7f800000)
#endif

#ifndef FINF
#define FINF(X)	 (((((_fval *)&(X))->fword) & 0x007fffff) == 0)
#endif

#endif	/* _IEEE */
