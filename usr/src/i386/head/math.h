#ifndef _MATH_H
#define _MATH_H
#ident	"@(#)sgs-head:i386/head/math.h	2.11.7.9"

#ifdef __cplusplus
extern "C" {
#endif

#if (__STDC__ - 0 == 0) && !defined(_XOPEN_SOURCE) \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)

enum version {c_issue_4, ansi_1, strict_ansi};

extern const enum version _lib_version;

#ifndef __cplusplus

struct exception
{
	int	type;
	char	*name;
	double	arg1;
	double	arg2;
	double	retval;
};

extern int	matherr(struct exception *);

#endif /*__cplusplus*/

#endif /*(__STDC__ - 0 == 0) && ...*/

extern double	acos(double);
extern double	asin(double);
extern double	atan(double);
extern double	atan2(double, double);
extern double	cos(double);
extern double	sin(double);
extern double	tan(double);

extern double	cosh(double);
extern double	sinh(double);
extern double	tanh(double);

extern double	exp(double);
extern double	frexp(double, int *);
extern double	ldexp(double, int);
extern double	log(double);
extern double	log10(double);
extern double	modf(double, double *);

long double	frexpl(long double, int *);
long double	ldexpl(long double, int);
long double	modfl(long double, long double *);

extern double	pow(double, double);
extern double	sqrt(double);

extern double	ceil(double);
extern double	fabs(double);
extern double	floor(double);
extern double	fmod(double, double);

extern float	acosf(float);
extern float	asinf(float);
extern float	atanf(float);
extern float	atan2f(float, float);
extern float	cosf(float);
extern float	sinf(float);
extern float	tanf(float);

extern float	coshf(float);
extern float	sinhf(float);
extern float	tanhf(float);

extern float	expf(float);
extern float	logf(float);
extern float	log10f(float);

extern float	powf(float, float);
extern float	sqrtf(float);

extern float	ceilf(float);
extern float	fabsf(float);
extern float	floorf(float);
extern float	fmodf(float, float);
extern float	modff(float, float *);

#ifndef HUGE_VAL
extern const double __huge_val;
#define HUGE_VAL (+__huge_val)
#endif

#if defined(_XOPEN_SOURCE) || (__STDC__ - 0 == 0 \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE))

extern double	erf(double);
extern double	erfc(double);
extern double	gamma(double);
extern double	hypot(double, double);
extern double	j0(double);
extern double	j1(double);
extern double	jn(int, double);
extern double	y0(double);
extern double	y1(double);
extern double	yn(int, double);
extern double	lgamma(double);
extern int	isnan(double);

#ifndef MAXFLOAT
#define MAXFLOAT	3.40282346638528860e+38F
#endif

#endif /*defined(_XOPEN_SOURCE) || ...*/

#if (defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1) \
	|| (__STDC__ - 0 == 0 && !defined(_XOPEN_SOURCE) \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE))

extern double	acosh(double);
extern double	asinh(double);
extern double	atanh(double);
extern double	cbrt(double);
extern double	expm1(double);
extern int	ilogb(double);
extern double	logb(double);
extern double	log1p(double);
extern double	nextafter(double, double);
extern double	remainder(double, double);
extern double	rint(double);
extern double	scalb(double, double);

#endif /*(defined(_XOPEN_SOURCE) && ...*/

#if __STDC__ - 0 == 0 && !defined(_XOPEN_SOURCE) \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)

#define HUGE	MAXFLOAT

extern double	atof(const char *);

extern double	copysign(double, double);
extern int	unordered(double, double);
extern int	finite(double);

long double	scalbl(long double, long double);
long double	logbl(long double);
long double	nextafterl(long double, long double);
extern int	unorderedl(long double, long double);
extern int	finitel(long double);

#endif /*__STDC__ - 0 == 0 && ...*/

#if defined(_XOPEN_SOURCE) || (__STDC__ - 0 == 0 \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE))

extern int	signgam;

#define M_E		2.7182818284590452354
#define M_LOG2E		1.4426950408889634074
#define M_LOG10E	0.43429448190325182765
#define M_LN2		0.69314718055994530942
#define M_LN10		2.30258509299404568402
#define M_PI		3.14159265358979323846
#define M_PI_2		1.57079632679489661923
#define M_PI_4		0.78539816339744830962
#define M_1_PI		0.31830988618379067154
#define M_2_PI		0.63661977236758134308
#define M_2_SQRTPI	1.12837916709551257390
#define M_SQRT2		1.41421356237309504880
#define M_SQRT1_2	0.70710678118654752440

#endif /*defined(_XOPEN_SOURCE) || ...*/

#if (__STDC__ - 0 == 0) && !defined(_XOPEN_SOURCE) \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)

#define _ABS(x)		((x) < 0 ? -(x) : (x))

#define _REDUCE(TYPE, X, XN, C1, C2)	{ \
	double x1 = (double)(TYPE)X, x2 = X - x1; \
	X = x1 - (XN) * (C1); X += x2; X -= (XN) * (C2); }

#define DOMAIN		1
#define	SING		2
#define	OVERFLOW	3
#define	UNDERFLOW	4
#define	TLOSS		5
#define	PLOSS		6

#define _POLY1(x, c)	((c)[0] * (x) + (c)[1])
#define _POLY2(x, c)	(_POLY1((x), (c)) * (x) + (c)[2])
#define _POLY3(x, c)	(_POLY2((x), (c)) * (x) + (c)[3])
#define _POLY4(x, c)	(_POLY3((x), (c)) * (x) + (c)[4])
#define _POLY5(x, c)	(_POLY4((x), (c)) * (x) + (c)[5])
#define _POLY6(x, c)	(_POLY5((x), (c)) * (x) + (c)[6])
#define _POLY7(x, c)	(_POLY6((x), (c)) * (x) + (c)[7])
#define _POLY8(x, c)	(_POLY7((x), (c)) * (x) + (c)[8])
#define _POLY9(x, c)	(_POLY8((x), (c)) * (x) + (c)[9])

#endif /*(__STDC__ - 0 == 0) && ...*/

#ifdef __cplusplus
}
#endif

#endif /*_MATH_H*/
