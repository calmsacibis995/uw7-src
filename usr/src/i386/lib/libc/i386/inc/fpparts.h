#ident	"@(#)libc-i386:inc/fpparts.h	1.6"

/* Macros to pull apart parts of single and  double precision
 * floating point numbers in IEEE format
 * Be sure to include /usr/include/values.h before including
 * this file to get the required definition of _IEEE
 */

 /* Don't do fp optimisations */
#pragma fenv_access on

#if _IEEE
#if M32 || u3b2
/* byte order with high order bits at lowest address */

/* double precision */
typedef  union {
	struct {
		unsigned  sign		:1;
		unsigned  exp		:11;
		unsigned  hi		:20;
		unsigned  lo		:32;
	} fparts;
	struct {
		unsigned  sign		:1;
		unsigned  exp		:11;
		unsigned  qnan_bit	:1;
		unsigned  hi		:19;
		unsigned  lo		:32;
	} nparts;
	struct {
		unsigned hi;
		unsigned lo;
	} fwords;
	double	d;
} _dval;

/* single precision */
typedef  union {
	struct {
		unsigned sign		:1;
		unsigned exp		:8;
		unsigned fract		:23;
	} fparts;
	struct {
		unsigned sign		:1;
		unsigned exp		:8;
		unsigned qnan_bit	:1;
		unsigned fract		:22;
	} nparts;
	unsigned long	fword;
	float	f;
} _fval;


#else
#if i386
/* byte order with low order bits at lowest address */

/* double precision */
typedef  union {
	struct {
		unsigned  lo		:32;
		unsigned  hi		:20;
		unsigned  exp		:11;
		unsigned  sign		:1;
	} fparts;
	struct {
		unsigned  lo		:32;
		unsigned  hi		:19;
		unsigned  qnan_bit	:1;
		unsigned  exp		:11;
		unsigned  sign		:1;
	} nparts;
	struct {
		unsigned  lo		:32;
		unsigned  hi		:32;
	} fwords;
	double	d;
} _dval;

/* single precision */
typedef  union {
	struct {
		unsigned fract		:23;
		unsigned exp		:8;
		unsigned sign		:1;
	} fparts;
	struct {
		unsigned fract		:22;
		unsigned qnan_bit	:1;
		unsigned exp		:8;
		unsigned sign		:1;
	} nparts;
	unsigned long	fword;
	float	f;
} _fval;

/* long double precision */
typedef union {
	struct {
		unsigned  lo		:32;
		unsigned  hi		:32;
		unsigned  exp		:15;
		unsigned  sign		:1;
		unsigned  unused	:16;
	} fparts;
	struct {
		unsigned  lo		:32;
		unsigned  hi		:31;
		unsigned  qnan_bit	:1;
		unsigned  exp		:15;
		unsigned  sign		:1;
		unsigned  unused	:16;
	} nparts;
	struct {
		unsigned  w0		:16;
		unsigned  w1		:16;
		unsigned  w2		:16;
		unsigned  w3		:16;
		unsigned  w4		:16;
		unsigned  unused	:16;
	} fwords;
	long double	ld;
} _ldval;

#endif /* i386 */
#endif /* 3B  */

# if i386	/* long double macros	*/
#define	SIGNBITLD(X)	(((_ldval *)&(X))->fparts.sign)
#define EXPONENTLD(X)	(((_ldval *)&(X))->fparts.exp)

#define HIFRACTIONLD(X)	(((_ldval *)&(X))->fparts.hi)

#define LOFRACTIONLD(X)	(((_ldval *)&(X))->fparts.lo)
#define HISHORTLD(X)	(((_ldval *)&(X))->fwords.w4)
#define QNANBITLD(X)	(((_ldval *)&(X))->nparts.qnan_bit)

#define MAXEXPLD	0x7fff /* maximum exponent of long double*/
#define ISMAXEXPLD(X)	((EXPONENTLD(X)) == MAXEXPLD)

#define SETQNANLD(X)	((((_ldval *)&(X))->nparts.qnan_bit) = 0x1)
#define HIQNANLD(X)	((EXPONENTLD(X))=MAXEXPLD,(HIFRACTIONLD(X))= 0x80000000)
#define LOQNANLD(X)	((LOFRACTIONLD(X)) = 0x0)

/* make a NaN out of a long double */
#define MKNANLD(X)        (HIQNANLD(X),LOQNANLD(X))

/* like libc's nan.h macros */
#ifndef NANorINFLD
#define NANorINFLD(X)     (ISMAXEXPLD(X))
#endif

/* NANorINF must be called before this */
#ifndef INFLD
#define INFLD(X)  (!LOFRACTIONLD(X) && HIFRACTIONLD(X)==0x80000000)
#endif

/*
 * If a NaN is not a quiet NaN, raise an invalid-op exception and
 * make it into a quiet NaN.
 */

#define SigNANLD(X) if (!QNANBITLD(X)) {double q1=0.0;          \
                                        double q2=0.0;          \
                                                                \
                                        q1 /= q2;               \
                                        SETQNANLD(X);           \
                                    }


#endif	/* long double macros for i386 */

/* parts of a double precision floating point number */
#define	SIGNBIT(X)	(((_dval *)&(X))->fparts.sign)
#define EXPONENT(X)	(((_dval *)&(X))->fparts.exp)

#if M32 || u3b2 || i386
#define HIFRACTION(X)	(((_dval *)&(X))->fparts.hi)
#endif

#define LOFRACTION(X)	(((_dval *)&(X))->fparts.lo)
#define QNANBIT(X)	(((_dval *)&(X))->nparts.qnan_bit)
#define HIWORD(X)	(((_dval *)&(X))->fwords.hi)
#define LOWORD(X)	(((_dval *)&(X))->fwords.lo)

#define MAXEXP	0x7ff /* maximum exponent of double*/
#define ISMAXEXP(X)	((EXPONENT(X)) == MAXEXP)

/* macros used to create quiet NaNs as return values */
#define SETQNAN(X)	((((_dval *)&(X))->nparts.qnan_bit) = 0x1)
#define HIQNAN(X)	((HIWORD(X)) = 0x7ff80000)
#define LOQNAN(X)	((((_dval *)&(X))->fwords.lo) = 0x0)

/* make a NaN out of a double */
#define MKNAN(X)        (HIQNAN(X),LOQNAN(X))

/* like libc's nan.h macros */
#ifndef NANorINF
#define NANorINF(X)     (ISMAXEXP(X))
#endif

/* NANorINF must be called before this */
#ifndef INF
#define INF(X)  (!LOWORD(X) && !HIFRACTION(X))
#endif

/*
 * If a NaN is not a quiet NaN, raise an invalid-op exception and
 * make it into a quiet NaN.
 */

#define SigNAN(X) if (!QNANBIT(X)) {    double q1=0.0;          \
                                        double q2=0.0;          \
                                                                \
                                        q1 /= q2;               \
                                        SETQNAN(X);             \
                                    }

/* macros used to extract parts of single precision values */
#define	FSIGNBIT(X)	(((_fval *)&(X))->fparts.sign)
#define FEXPONENT(X)	(((_fval *)&(X))->fparts.exp)

#if M32 || u3b2 || i386
#define FFRACTION(X)	(((_fval *)&(X))->fparts.fract)
#endif

#define FWORD(X)	(((_fval *)&(X))->fword)
#define FQNANBIT(X)	(((_fval *)&(X))->nparts.qnan_bit)
#define MAXEXPF	255 /* maximum exponent of single*/
#define FISMAXEXP(X)	((FEXPONENT(X)) == MAXEXPF)
#define FSETQNAN(X)	((((_fval *)&(X))->nparts.qnan_bit) = 0x1)

/* make a NaN out of a float */
#define FMKNAN(X)        ((((_fval*)&(X))->fword) = 0x7f800001)

#ifndef FNANorINF
#define FNANorINF(X)     (FISMAXEXP(X))
#endif

/* FNANorINF must be called before this */
#ifndef FINF
#define FINF(X)  (FFRACTION(X) == 0)
#endif
/*
 * If a NaN is not a quiet NaN, raise an invalid-op exception and
 * make it into a quiet NaN.
 */

#define FSigNAN(X) if (FQNANBIT(X) == 0) {double q1=0.0;          \
                                        double q2=0.0;          \
                                                                \
                                        q1 /= q2;               \
                                        FSETQNAN(X);           \
                                    }

#else /* _IEEE */

/* parts of a double precision floating point number */
#define SIGNBIT(X)      (0)
#define EXPONENT(X)     (0)
#define HIFRACTION(X)   (0)
#define LOFRACTION(X)   (0)
#define QNANBIT(X)      (0)
#define HIWORD(X)       (0)
#define LOWORD(X)       (0)

#define MAXEXP          (0)
#define ISMAXEXP(X)     (0)

/* macros used to create quiet NaNs as return values */
#define SETQNAN(X)
#define HIQNAN(X)
#define LOQNAN(X)

/* make a NaN out of a double */
#define MKNAN(X)

/* like libc's nan.h macros */
#ifndef NANorINF
#define NANorINF(X)     (0)
#endif

/* NANorINF must be called before this */
#ifndef INF
#define INF(X)  (0)
#endif

/*
 * If a NaN is not a quiet NaN, raise an invalid-op exception and
 * make it into a quiet NaN.
 */

#define SigNAN(X)

/* macros used to extract parts of single precision values */
#define FSIGNBIT(X)     (0)
#define FEXPONENT(X)    (0)
#define FFRACTION(X)    (0)
#define FWORD(X)        (0)
#define FQNANBIT(X)     (0)
#define MAXEXPF         (0)
#define FISMAXEXP(X)    (0)
#define FSETQNAN(X)
#define FMKNAN(X)
#ifndef FNANorINF
#define FNANorINF(X)     (0)
#endif
#ifndef FINF
#define FINF(X)  (0)
#endif
#define FSigNAN(X) 

/* long double macros */
#define	SIGNBITLD(X)	(0)
#define EXPONENTLD(X)	(0)

#define HIFRACTIONLD(X)	(0)
#define HISHORTLD(X)	(0)

#define LOFRACTIONLD(X)	(0)
#define QNANBITLD(X)	(0)

#define MAXEXPLD	(0)
#define ISMAXEXPLD(X)	(0)

#define SETQNANLD(X)	
#define HIQNANLD(X)	
#define LOQNANLD(X)	

/* make a NaN out of a long double */
#define MKNANLD(X)        

/* like libc's nan.h macros */
#ifndef NANorINFLD
#define NANorINFLD(X)     (0)
#endif

/* NANorINF must be called before this */
#ifndef INFLD
#define INFLD(X)  (0)
#endif

/*
 * If a NaN is not a quiet NaN, raise an invalid-op exception and
 * make it into a quiet NaN.
 */

#define SigNANLD(X) 

#endif  /*IEEE*/
