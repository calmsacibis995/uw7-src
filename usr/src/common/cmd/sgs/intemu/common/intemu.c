#ident	"@(#)intemu:common/intemu.c	1.5"
/*
* common/intemu.c -- large, fixed size, two's complement,
*			integer computation package
*/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <pfmt.h>
#ifdef __STDC__
#   include <stdarg.h>
#   include <stdlib.h>
#else
#   include <varargs.h>
    char *malloc(), *realloc();
    void exit();
#endif
#include "intemu.h"

#ifdef __STDC__
#   undef ULONG_MAX
#   define ULONG_MAX	(~0ul)	/* some <limits.h> values are bogus */
#   undef UINT_MAX
#   define UINT_MAX	(~0u)	/* some <limits.h> values are bogus */
#else
#   define ULONG_MAX	(~(Ulong)0)
#   define LONG_MAX	((long)(ULONG_MAX >> 1))
#   define LONG_MIN	(-LONG_MAX - 1)		/* assume 2's complement */
#   define UINT_MAX	(~(Uint)0)
#   define INT_MAX	((int)(UINT_MAX >> 1))
#   define INT_MIN	(-INT_MAX - 1)		/* assume 2's complement */
#endif

typedef unsigned char	Uchar;
typedef unsigned short	Ushort;
typedef unsigned int	Uint;
typedef unsigned long	Ulong;
#ifdef NUM_LONG_LONG
typedef long long		Llong;
typedef unsigned long long	ULlong;
#endif

#define BIT(n)	((Ulong)1 << (n))	/* n-th ulong bit */
#define LOW(n)	(~(~(Ulong)0 << (n)))	/* n low-order ulong bits */
#define HIGH(n)	(~(~(Ulong)0 >> (n)))	/* n high-order ulong bits */
#define HIGHINT(n) (~(~(Uint)0 >> (n)))	/* n high-order uint bits */
#define HIGHLL(n) (~(~(ULlong)0 >> (n))) /* n high-order ullong bits */

#define BITSIZE(t) (sizeof(t) * CHAR_BIT)	/* number of bits in a type */

	/*
	* Determine the actual integer type used as the basic unit of
	* computation and storage.  The idea is to use all the bits of
	* the largest unsigned type that is no bigger than half a Ulong.
	* Some of the computations (e.g. multiplication) require
	* intermediate results that are twice as big as the basic unit.
	*/
#ifdef __STDC__
	#if ULONG_MAX / UINT_MAX >= UINT_MAX
		#define pkt	ui
		#define PTYPE	Uint
	#elif ULONG_MAX / USHRT_MAX >= USHRT_MAX
		#define pkt	us
		#define PTYPE	Ushort
	#elif ULONG_MAX / UCHAR_MAX >= UCHAR_MAX
		#define pkt	uc
		#define PTYPE	Uchar
	#else
		#error "All integer types bigger than half a long"
	#endif
#else		/* reasonable best guess */
#	define pkt	us
#	define PTYPE	Ushort
#	define const
#endif

#define NPKT	(sizeof(IntNum) / sizeof(PTYPE))	/* array length */
#define NPKTBIT	BITSIZE(PTYPE)				/* bits/packet */
#define NTOPBIT	(NUMSIZE - (NPKT - 1) * NPKTBIT)	/* left-over bits */
#define UNUSED	(LOW(NPKTBIT - NTOPBIT) << NTOPBIT)	/* unused bits mask */
#define SIGN(p)	((p)[NPKT - 1] & BIT(NTOPBIT - 1))	/* sign bit value */

#ifdef __STDC__
static void (*numfatal)(const char *, ...);	/* internal error */
static void *(*numalloc)(void *, size_t);	/* allocate/deallocate */
#else
static void (*numfatal)();
static char *(*numalloc)();
#endif

#define NPRENUM	(1 << CHAR_BIT)			/* covers all byte values */
static IntNum	prenum[NPRENUM];		/* precomputed numbers */
static IntNum	pow10[(NUMSIZE + 2) / 3];	/* powers of ten */
static int	maxpow10;			/* last used pow10 element */

	/*
	* General utility functions.
	*/

void
#ifdef __STDC__
num_free(IntNum *np)	/* return allocated number */
#else
num_free(np)IntNum*np;
#endif
{
	if (np < &prenum[0] || np >= &prenum[NPRENUM])
		(void)(*numalloc)((void *)np, (size_t)0);
}

void
#ifdef __STDC__
num_size(const IntNum *np, NumSize *zp)	/* fill NumSize for number */
#else
num_size(np,zp)IntNum*np;NumSize*zp;
#endif
{
	static const Uchar negbit[] = {0,0,0,0,0,0,0,0,1,1,1,1,2,2,3,3};
	static const Uchar posbit[] = {3,3,2,2,1,1,1,1,0,0,0,0,0,0,0,0};
	register const PTYPE *p = &np->pkt[NPKT];
	register PTYPE val;
	register int nb = NPKT * NPKTBIT;	/* not NUMSIZE */

	if (SIGN(p - NPKT) != 0)	/* SIGN(np->pkt) != 0 */
	{
		zp->sz_nubit = NUMSIZE;	/* easy case */
		/*
		* Find highest-order packet with a zero bit.
		*/
		/*CONSTANTCONDITION*/
		if (NTOPBIT < NPKTBIT)
		{
			if (*--p != LOW(NTOPBIT)) /* fill out to NPKTBIT */
			{
				val = *p | UNUSED;
			}
			else
			{
				nb = (NPKT * NPKTBIT) - NPKTBIT;
				while (*--p == LOW(NPKTBIT))
				{
					if ((nb -= NPKTBIT) == 0) /* -1 value */
					{
						zp->sz_nsbit = 1;
						zp->sz_minbit = 1;
						return;
					}
				}
				val = *p;
			}
		}
		else	/* NTOPBIT == NPKTBIT */
		{
			while (*--p == LOW(NPKTBIT))
			{
				if ((nb -= NPKTBIT) == 0)	/* -1 value */
				{
					zp->sz_nsbit = 1;
					zp->sz_minbit = 1;
					return;
				}
			}
			val = *p;
		}
		/*
		* Check from the top down, 4 bits at a time.
		* If NPKTBIT is larger than 32 bits then it
		* would be cheaper to reduce val by halving
		* until the negbit array can be used.
		*/
		nb++;	/* need a 1 bit above */
		while ((val & (LOW(4) << (NPKTBIT - 4)))
			== (LOW(4) << (NPKTBIT - 4)))
		{
			nb -= 4;
			val <<= 4;
		}
		nb -= negbit[val >> (NPKTBIT - 4)];
		zp->sz_nsbit = nb;
		zp->sz_minbit = nb;
	}
	else	/* nonnegative value */
	{
		while (*--p == 0)
		{
			if ((nb -= NPKTBIT) == 0)	/* 0 value */
			{
				zp->sz_nsbit = 1;
				zp->sz_nubit = 1;
				zp->sz_minbit = 1;
				return;
			}
		}
		/*
		* Check from the top down, 4 bits at a time.
		* If NPKTBIT is larger than 32 bits then it
		* would be cheaper to reduce val by halving
		* until the posbit array can be used.
		*/
		val = *p;
		while ((val & (LOW(4) << (NPKTBIT - 4))) == 0)
		{
			nb -= 4;
			val <<= 4;
		}
		nb -= posbit[val >> (NPKTBIT - 4)];
		zp->sz_nubit = nb;
		zp->sz_minbit = nb;
		zp->sz_nsbit = nb + 1;	/* needs a 0 bit above */
	}
}

static void
#ifdef __STDC__
setnum(IntNum *np, register Ulong val)	/* set number to Ulong's value */
#else
setnum(np,val)IntNum*np;register Ulong val;
#endif
{
	register PTYPE *p = &np->pkt[0];
	int i = NPKT;

	do	/* from low to high */
	{
		*p++ = val;
		val >>= NPKTBIT;
	} while (--i != 0);
}

#ifdef NUM_LONG_LONG

static void
#ifdef __STDC__
llsetnum(IntNum *np, ULlong val) /* set number to ULlong's value */
#else
llsetnum(np,val)IntNum*np;ULlong val;
#endif
{
	register PTYPE *p = &np->pkt[0];
	int i = NPKT;

	do /* from low to high */
	{
		*p++ = val;
		val >>= NPKTBIT;
	} while (--i != 0);
}

#endif /*NUM_LONG_LONG*/

static int
#ifdef __STDC__
setul(const IntNum *np, register Ulong *dst)	/* set Ulong to number's value */
#else
setul(np,dst)IntNum*np;register Ulong*dst;
#endif
{
	register const PTYPE *p = &np->pkt[NPKT];
	int oflow = 0, i = NPKT;

	*dst = 0;
	do
	{
		if ((*dst & HIGH(NPKTBIT)) != 0)
			oflow = 1;
		*dst <<= NPKTBIT;
		*dst |= *--p;
	} while (--i != 0);
	return oflow;
}

static void
#ifdef __STDC__
myfatal(const char *fmt, ...)	/* default fatal function */
#else
myfatal(va_alist)va_dcl
#endif
{
	va_list ap;
	char msgbuf[512];
#ifdef __STDC__
	va_start(ap, fmt);
#else
	char *fmt;

	va_start(ap);
	fmt = va_arg(ap, char *);
#endif
	(void)vsprintf(msgbuf, fmt, ap);
	pfmt(stderr,MM_ERROR,":108:%s\n", msgbuf);
	va_end(ap);
	exit(2);
}

static
#ifdef __STDC__
void *
myalloc(void *p, size_t n)	/* default (re)allocation function */
#else
char *
myalloc(p,n)char*p;size_t n;
#endif
{
	if (n == 0)		/* deallocate *p */
	{
		free(p);
		return 0;
	}
	if (p == 0)		/* new allocation */
		return malloc(n);
	return realloc(p, n);	/* resizing */
}

void
#ifdef __STDC__
num_init(void (*f)(const char *, ...), void *(*a)(void *, size_t)) /* set up */
#else
num_init(f,a)void(*f)();char*(*a)();
#endif
{
	IntNum x, y;
	Ulong i;

	if ((numfatal = f) == 0)
		numfatal = myfatal;
	if ((numalloc = a) == 0)
		numalloc = myalloc;
	/*CONSTANTCONDITION*/
	if (NPKTBIT * 2 > BITSIZE(Ulong) || NUMSIZE < BITSIZE(Ulong)
		|| NUMSIZE % CHAR_BIT != 0)
	{
		(*numfatal)(gettxt(":1480","num_init():bad numeric package parameters"));
	}
	for (i = 1; i < NPRENUM; i++)	/* no need to set [0] */
		setnum(&prenum[i], i);
	x = prenum[1];
	for (;;)	/* x*10 == x<<1 + x<<3 */
	{
		pow10[maxpow10] = x;
		if (num_llshift(&x, 1) != 0)
			break;
		y = x;
		if (num_llshift(&x, 2) != 0 || num_uadd(&x, &y) != 0)
			break;
		if (++maxpow10 >= sizeof(pow10) / sizeof(IntNum))
			(*numfatal)(gettxt(":1481","num_init():pow 10 table too short"));
	}
}

	/*
	* Number computation functions.  Alphabetically ordered,
	* ignoring the s and u of num_s... and num_u... pairs.
	*/

int
#ifdef __STDC__
num_sadd(IntNum *dst, const IntNum *src)	/* dst += src (signed) */
#else
num_sadd(dst,src)IntNum*dst,*src;
#endif
{
	register PTYPE *d = &dst->pkt[0];
	register const PTYPE *s = &src->pkt[0];
	register Ulong carry = 0;
	int i = NPKT;
	int samesign = 0;
	PTYPE origsign = SIGN(d);	/* remember incoming sign */

	if (origsign == SIGN(s))
		samesign = 1;
	do	/* add low to high, carrying forward one bit */
	{
		carry += *s++ + (Ulong)*d;
		*d++ = carry;
		carry >>= NPKTBIT;
	} while (--i != 0);
	/*CONSTANTCONDITION*/
	if (NTOPBIT < NPKTBIT)
		dst->pkt[NPKT - 1] &= ~UNUSED;
	/*
	* Overflow only occurs if the original two sign bits were
	* the same and the result's sign bit is different.
	*/
	if (samesign && SIGN(dst->pkt) != origsign)
		return 1;
	return 0;
}

int
#ifdef __STDC__
num_uadd(IntNum *dst, const IntNum *src)	/* dst += src (unsigned) */
#else
num_uadd(dst,src)IntNum*dst,*src;
#endif
{
	register PTYPE *d = &dst->pkt[0];
	register const PTYPE *s = &src->pkt[0];
	register Ulong carry = 0;
	int i = NPKT;

	do	/* add from low to high, carrying forward one bit */
	{
		carry += *s++ + (Ulong)*d;
		*d++ = carry;
		carry >>= NPKTBIT;
	} while (--i != 0);
	/*CONSTANTCONDITION*/
	if (NTOPBIT < NPKTBIT)
	{
		if (dst->pkt[NPKT - 1] & UNUSED)
		{
			carry = 1;
			dst->pkt[NPKT - 1] &= ~UNUSED;
		}
	}
	return carry;
}

int
#ifdef __STDC__
num_alshift(IntNum *np, int n)	/* np <<= n (arithmetic) */
#else
num_alshift(np,n)IntNum*np;int n;
#endif
{
	register PTYPE *p;
	register int i, j;
	int pcnt;
	int oflow = 0;

	/*
	* Arithmetic left shift is the least likely of the four shifts
	* to be implemented on a machine, probably because it's the
	* trickiest.  The basic idea is to pretend as if the number has
	* no sign bit and is thus one bit smaller.  Each bit that is
	* shifted up into and past the sign bit must be the same as the
	* incoming sign bit; otherwise overflow occurred.  In other words,
	* the result should have the two's complement value that is 2**n
	* times the original value, where n is the number of bits shifted.
	*/
	if (SIGN(np->pkt) == 0)
	{
		if ((oflow = num_llshift(np, n)) == 0 && SIGN(np->pkt) != 0)
			oflow = 1;
		return oflow;
	}
	if (n < 0)
	{
		if (n < -NUMSIZE)
			n = -NUMSIZE;
		num_arshift(np, -n);
		return 1;
	}
	if (n > NUMSIZE)
	{
		oflow = 1;
		n = NUMSIZE;
	}
	/*CONSTANTCONDITION*/
	if (NTOPBIT < NPKTBIT)
		np->pkt[NPKT - 1] |= UNUSED;	/* fill to full NPKTBITs */
	if ((pcnt = n / NPKTBIT) != 0)
	{
		j = pcnt;
		p = &np->pkt[NPKT];
		do
		{
			if (*--p == LOW(NPKTBIT))
				continue;
			oflow = 1;
			break;
		} while (--j != 0);
		i = pcnt;
		j = NPKT - pcnt;
		p = &np->pkt[j];
		while (--j >= 0)
		{
			p--;
			p[i] = p[0];
		}
		do
			p[--i] = 0;
		while (i != 0);
	}
	if ((i = n % NPKTBIT) != 0)
	{
		register Ulong carry = 0;

		p = &np->pkt[pcnt];
		j = NPKT - pcnt;
		do
		{
			carry |= (Ulong)*p << i;
			*p++ = carry;
			carry >>= NPKTBIT;
		} while (--j != 0);
		if (oflow == 0 && carry != LOW(i))
			oflow = 1;
	}
	/*CONSTANTCONDITION*/
	if (NTOPBIT < NPKTBIT)
	{
		if (oflow == 0 && (np->pkt[NPKT - 1] & UNUSED) != UNUSED)
			oflow = 1;
		np->pkt[NPKT - 1] &= ~UNUSED;
	}
	if (oflow == 0 && SIGN(np->pkt) == 0)
		oflow = 1;
	return oflow;
}

int
#ifdef __STDC__
num_arshift(IntNum *np, int n)	/* np >>= n (arithmetic) */
#else
num_arshift(np,n)IntNum*np;int n;
#endif
{
	register PTYPE *p;
	register int i, j;
	int pcnt;
	int oflow = 0;

	if (SIGN(np->pkt) == 0)
		return num_lrshift(np, n);
	if (n < 0)
	{
		if (n < -NUMSIZE)
			n = -NUMSIZE;
		num_alshift(np, -n);
		return 1;
	}
	if (n > NUMSIZE)
	{
		oflow = 1;
		n = NUMSIZE;
	}
	if ((pcnt = n / NPKTBIT) != 0)
	{
		i = pcnt;
		p = &np->pkt[0];
		for (j = NPKT - i; --j >= 0; p++)
			p[0] = p[i];
		/*CONSTANTCONDITION*/
		if (NTOPBIT < NPKTBIT)
		{
			p[-1] |= UNUSED;	/* fill to NPKTBIT 1s */
			while (--i != 0)
				*p++ = LOW(NPKTBIT);
			*p = LOW(NTOPBIT);
		}
		else	/* NTOPBIT == NPKTBIT */
		{
			do
				*p++ = LOW(NPKTBIT);
			while (--i != 0);
		}
	}
	if ((i = n % NPKTBIT) != 0)
	{
		register Ulong carry = LOW(NPKTBIT) << NPKTBIT;

		j = NPKT - pcnt;
		p = &np->pkt[NPKT - pcnt];
		do
		{
			carry |= *--p;
			*p = carry >> i;
			carry <<= NPKTBIT;
		} while (--j != 0);
		/*CONSTANTCONDITION*/
		if (NTOPBIT < NPKTBIT)
			np->pkt[NPKT - 1] &= ~UNUSED;
	}
	return oflow;
}

void
#ifdef __STDC__
num_and(IntNum *dst, const IntNum *src)		/* dst &= src */
#else
num_and(dst,src)IntNum*dst,*src;
#endif
{
	register PTYPE *d = &dst->pkt[NPKT];
	register const PTYPE *s = &src->pkt[NPKT];
	int i = NPKT;

	do
		*--d &= *--s;
	while (--i != 0);
}

int
#ifdef __STDC__
num_scompare(const IntNum *n1, const IntNum *n2)	/* n1 ? n2 (signed) */
#else
num_scompare(n1,n2)IntNum*n1,*n2;
#endif
{
	PTYPE sign1 = SIGN(n1->pkt);

	if (sign1 == SIGN(n2->pkt))
		return num_ucompare(n1, n2);
	if (sign1 != 0)
		return -1;
	return 1;
}

int
#ifdef __STDC__
num_ucompare(const IntNum *n1, const IntNum *n2)	/* n1 ? n2 (unsigned) */
#else
num_ucompare(n1,n2)IntNum*n1,*n2;
#endif
{
	register const PTYPE *s1 = &n1->pkt[NPKT];
	register const PTYPE *s2 = &n2->pkt[NPKT];
	int i = NPKT;

	do
	{
		if (*--s1 < *--s2)
			return -1;
		else if (*s1 > *s2)
			return 1;
	} while (--i != 0);
	return 0;	/* equal */
}

void
#ifdef __STDC__
num_complement(IntNum *np)	/* num = ~num */
#else
num_complement(np)IntNum*np;
#endif
{
	register PTYPE *p = &np->pkt[NPKT];
	int i = NPKT;

	*--p ^= LOW(NTOPBIT);
	while (--i != 0)
		*--p ^= LOW(NPKTBIT);
}

int
#ifdef __STDC__
num_sdivide(IntNum *dst, const IntNum *src)	/* dst /= src (signed) */
#else
num_sdivide(dst,src)IntNum*dst,*src;
#endif
{
	IntNum tmp, numer;

	numer = *dst;
	return num_sdivrem(dst, &tmp, &numer, src) & 0x1;
}

int
#ifdef __STDC__
num_udivide(IntNum *dst, const IntNum *src)	/* dst /= src (unsigned) */
#else
num_udivide(dst,src)IntNum*dst,*src;
#endif
{
	IntNum tmp, numer;

	numer = *dst;
	return num_udivrem(dst, &tmp, &numer, src);
}

int
#ifdef __STDC__
num_sdivrem(IntNum *quot, IntNum *rem, const IntNum *numer, const IntNum *denom)
#else
num_sdivrem(quot,rem,numer,denom)IntNum*quot,*rem,*numer,*denom;
#endif
{
	IntNum tmpnumer, tmpdenom;
	int negquot, negrem, oflow;

	/*
	* Division and remainder chosen for negative numbers is as follows:
	*
	*	/,%|   3  |  -3  |
	*	---+------+------+
	*	 2 | Q: 1 | Q:-1 |	 3 /  2    quot  1	rem  1
	*	   | R: 1 | R:-1 |	-3 / -2    quot  1	rem -1
	*	---+------+------+	-3 /  2    quot -1	rem -1
	*	-2 | Q:-1 | Q: 1 |	 3 / -2    quot -1	rem  1
	*	   | R: 1 | R:-1 |
	*	---+------+------+
	*
	* Signed two's complement division has two overflow possibilities:
	*  - Division by 0 (recognized by num_udivrem), and
	*  - Division of the largest magnitude negative number by -1
	*
	* Moreover, the second overflow condition should be noted when
	* quotient is desired, not the remainder.
	*/
	negrem = 0;
	if (SIGN(numer->pkt) != 0)
	{
		negrem = 1;
		tmpnumer = *numer;
		num_negate(&tmpnumer);
		numer = &tmpnumer;
		if (SIGN(denom->pkt) == 0)
			negquot = 1;
		else
		{
			negquot = 0;
		do_neg:;
			tmpdenom = *denom;
			num_negate(&tmpdenom);
			denom = &tmpdenom;
		}
	}
	else if (SIGN(denom->pkt) != 0)
	{
		negquot = 1;
		goto do_neg;
	}
	else	/* both nonnegative */
		negquot = 0;
	if (num_udivrem(quot, rem, numer, denom) != 0)	/* denom == 0 */
	{
		if (negrem)
			num_negate(rem);
		if (negquot)
			num_complement(quot);	/* -"infinity" */
		return 0x3;			/* both overflowed */
	}
	oflow = 0;
	if (SIGN(quot->pkt) != 0)
		oflow |= 0x1;
	if (SIGN(rem->pkt) != 0)
		oflow |= 0x2;
	if (negrem && num_negate(rem) != 0)
		oflow &= ~0x2;
	if (negquot && num_negate(quot) != 0)
		oflow &= ~0x1;
	return oflow;
}

int
#ifdef __STDC__
num_udivrem(IntNum *quot, IntNum *rem, const IntNum *numer, const IntNum *denom)
#else
num_udivrem(quot,rem,numer,denom)IntNum*quot,*rem,*numer,*denom;
#endif
{
	IntNum negdenom;
	int n, high_denom_bit;

	/*
	* If denom is 0 then overflow (quot <- infinity, rem <- numer)
	* If denom is 1 then quot <- numer, rem <- 0
	* If numer < denom then quot <- 0, rem <- numer
	* Otherwise:
	*	quot <- 0
	*	for i <- high to low
	*		if numer[high...i] > denom then
	*			numer[high...i] -= denom
	*			quot[i] <- 1
	*	rem <- numer
	*/
	if ((high_denom_bit = num_highbitno(denom)) < 0) /* denom == 0 */
	{
		*rem = *numer;
		*quot = prenum[0];
		num_complement(quot);
		quot->pkt[NPKT - 1] &= LOW(NTOPBIT - 1); /* "infinity" */
		return 1;
	}
	if (high_denom_bit == 0)	/* denom == 1 */
	{
		*quot = *numer;
		*rem = prenum[0];
		return 0;
	}
	*quot = prenum[0];
	*rem = *numer;
	/*
	* Start division based on the highest bit set in the numerator,
	* adjusted according to the number of significant bits in the
	* denominator.
	*/
	if ((n = num_highbitno(numer) - high_denom_bit + 1) <= 0)
		return 0;	/* denom > numer */
	num_lrshift(rem, n);
	negdenom = *denom;
	num_negate(&negdenom);
	do
	{
		n--;
		/*
		* Shift in the next bit of the new numerator.
		*/
		num_llshift(rem, 1);
		if ((numer->pkt[n / NPKTBIT] & BIT(n % NPKTBIT)) != 0)
			rem->pkt[0] |= 0x1;
		/*
		* Make room for new 0 or 1 in the quotient.
		*/
		num_llshift(quot, 1);
		if (num_ucompare(rem, denom) >= 0)
		{
			num_uadd(rem, &negdenom);
			quot->pkt[0] |= 0x1;
		}
	} while (n != 0);
	return 0;
}

void
#ifdef __STDC__
num_gcd(IntNum *dst, const IntNum *src)	/* dst = GCD(dst, src) */
#else
num_gcd(dst,src)IntNum*dst,*src;
#endif
{
	IntNum num, tmp;
	PTYPE val;
	int flag, n, nbit;

	/*
	* Adapted from "Algorithm B" from Knuth Vol. 2, 4.5.2.,
	* using a flag instead of negative values.
	*/
	num = *src;
	n = 0;
	nbit = 0;
	while ((val = dst->pkt[0] | num.pkt[0]) == 0)
	{
		nbit += NPKTBIT;
		num_lrshift(dst, NPKTBIT);
		num_lrshift(&num, NPKTBIT);
	}
	if ((val & 0x1) == 0)
	{
		for (n = 1; ((val >>= 1) & 0x1) == 0; n++);
			;
		num_lrshift(dst, n);
		num_lrshift(&num, n);
		nbit += n;
	}
	if ((flag = dst->pkt[0] & 0x1) != 0)
		tmp = num;
	else
		tmp = *dst;
	for (;;)
	{
		while ((val = tmp.pkt[0]) == 0)
			num_lrshift(&tmp, NPKTBIT);
		if ((val & 0x1) == 0)
		{
			for (n = 1; ((val >>= 1) & 0x1) == 0; n++)
				;
			num_lrshift(&tmp, n);
		}
		if (flag)
			num = tmp;
		else
			*dst = tmp;
		if ((flag = num_ucompare(dst, &num)) > 0)
		{
			tmp = num;
			num_negate(&tmp);
			num_uadd(&tmp, dst);
			flag = 0;
		}
		else if (flag == 0)
			break;		/* only way out */
		else
		{
			tmp = *dst;
			num_negate(&tmp);
			num_uadd(&tmp, &num);
			flag = 1;
		}
	}
	num_llshift(dst, nbit);
}

int
#ifdef __STDC__
num_highbitno(const IntNum *np)	/* return number of highest set bit */
#else
num_highbitno(np)IntNum*np;
#endif
{
	register const PTYPE *p = &np->pkt[NPKT];
	register PTYPE bit;
	int i = NPKT * NPKTBIT;		/* not NUMSIZE */

	do	/* each packet, high to low */
	{
		if (*--p == 0)
			continue;
		bit = BIT(NPKTBIT - 1);
		do		/* each packet bit, high to low */
		{
			i--;
			if ((bit & *p) != 0)
				return i;
		} while ((bit >>= 1) != 0);
		(*numfatal)(gettxt(":1482","num_highbitno():nonzero but no bit set: %lu"),
			(Ulong)*p);
		/*NOTREACHED*/
	} while ((i -= NPKTBIT) != 0);
	return -1;	/* no bits set */
}

int
#ifdef __STDC__
num_lcm(IntNum *dst, const IntNum *src)	/* dst = LCM(dst, src) */
#else
num_lcm(dst,src)IntNum*dst,*src;
#endif
{
	IntNum gcd;

	gcd = *dst;
	num_gcd(&gcd, src);
	num_udivide(dst, &gcd);		/* guaranteed to divide evenly */
	return num_umultiply(dst, src);
}

int
#ifdef __STDC__
num_llshift(IntNum *np, int n)	/* np <<= n (logical) */
#else
num_llshift(np,n)IntNum*np;int n;
#endif
{
	register PTYPE *p;
	register int i, j;
	int pcnt;
	int oflow = 0;

	if (n < 0)
	{
		if (n < -NUMSIZE)
			n = -NUMSIZE;
		num_lrshift(np, -n);
		return 1;
	}
	if (n > NUMSIZE)
	{
		oflow = 1;
		n = NUMSIZE;
	}
	if ((pcnt = n / NPKTBIT) != 0)
	{
		j = i = pcnt;
		p = &np->pkt[NPKT];
		do
		{
			if (*--p == 0)
				continue;
			oflow = 1;
			break;
		} while (--j != 0);
		j = NPKT - i;
		p = &np->pkt[j];
		while (--j >= 0)
		{
			p--;
			p[i] = p[0];
		}
		do
			p[--i] = 0;
		while (i != 0);
	}
	if ((i = n % NPKTBIT) != 0)
	{
		register Ulong carry = 0;

		p = &np->pkt[pcnt];
		j = NPKT - pcnt;
		do
		{
			carry |= (Ulong)*p << i;
			*p++ = carry;
			carry >>= NPKTBIT;
		} while (--j != 0);
		if (carry != 0)
			oflow = 1;
	}
	/*CONSTANTCONDITION*/
	if (NTOPBIT < NPKTBIT)
	{
		if (np->pkt[NPKT - 1] & UNUSED)
		{
			oflow = 1;
			np->pkt[NPKT - 1] &= ~UNUSED;
		}
	}
	return oflow;
}

int
#ifdef __STDC__
num_lrshift(IntNum *np, int n)	/* np >>= n (logical) */
#else
num_lrshift(np,n)IntNum*np;int n;
#endif
{
	register PTYPE *p;
	register int i, j;
	int pcnt;
	int oflow = 0;

	if (n < 0)
	{
		if (n < -NUMSIZE)
			n = -NUMSIZE;
		num_llshift(np, -n);
		return 1;
	}
	if (n > NUMSIZE)
	{
		oflow = 1;
		n = NUMSIZE;
	}
	if ((pcnt = n / NPKTBIT) != 0)
	{
		i = pcnt;
		p = &np->pkt[0];
		for (j = NPKT - i; --j >= 0; p++)
			p[0] = p[i];
		do
			*p++ = 0;
		while (--i != 0);
	}
	if ((i = n % NPKTBIT) != 0)
	{
		register Ulong carry = 0;

		j = NPKT - pcnt;
		p = &np->pkt[NPKT - pcnt];
		do
		{
			carry |= *--p;
			*p = carry >> i;
			carry <<= NPKTBIT;
		} while (--j != 0);
	}
	return oflow;
}

int
#ifdef __STDC__
num_smultiply(IntNum *dst, const IntNum *src)	/* dst *= src (signed) */
#else
num_smultiply(dst,src)IntNum*dst,*src;
#endif
{
	IntNum tmpsrc;
	int neg = 0;
	int oflow;

	/*
	* Determine the sign of the result, convert to nonnegative,
	* multiply, check for overflow, and negate if appropriate.
	*/
	if (SIGN(dst->pkt) != 0)
	{
		num_negate(dst);
		if (SIGN(src->pkt) == 0)
			neg = 1;
		else
		{
		do_neg:;
			tmpsrc = *src;
			num_negate(&tmpsrc);
			src = &tmpsrc;
		}
	}
	else if (SIGN(src->pkt) != 0)
	{
		neg = 1;
		goto do_neg;
	}
	oflow = num_umultiply(dst, src);
	if (oflow == 0 && SIGN(dst->pkt) != 0)
		oflow = 1;
	if (neg && num_negate(dst) != 0)
		oflow = 0;
	return oflow;
}

int
#ifdef __STDC__
num_umultiply(IntNum *dst, const IntNum *src)	/* dst *= src (unsigned) */
#else
num_umultiply(dst,src)IntNum*dst,*src;
#endif
{
	union mul
	{
		IntNum	lowhalf;		/* overlays low order */
		PTYPE	result[NPKT + NPKT];	/* full result */
	} u;
	static const union mul zero;
	const PTYPE *s = &src->pkt[0];
	register PTYPE *r, *d;
	register Ulong carry, val;
	int i, j, oflow;

	/*
	* Need temporary result of size 2 * NUMSIZE.
	*
	* result <- 0
	* for i <- low to high
	*	carry <- 0
	*	for j <- low to high
	*		result[i+j] += src[i] * dst[j] + carry
	*		carry <- overflow of result[i+j]
	*	result[i+high+1] <- carry
	*
	* Overflow occurs only if the result doesn't fit.
	*/
	u = zero;
	for (i = 0; i < NPKT; i++)
	{
		if ((val = *s++) == 0)
			continue;	/* u.result[i+NPKT] already 0 */
		carry = 0;
		r = &u.result[i];
		d = &dst->pkt[0];
		j = NPKT;
		do
		{
			carry += val * *d++ + *r;
			*r++ = carry;
			carry >>= NPKTBIT;
		} while (--j != 0);
		*r = carry;
	}
	/*
	* Copy result into dst; check upper half for overflow.
	*/
	*dst = u.lowhalf;
	oflow = 0;
	r = &u.result[NPKT + NPKT];
	i = NPKT;
	do
	{
		if (*--r != 0)
		{
			oflow = 1;
			break;
		}
	} while (--i != 0);
	/*CONSTANTCONDITION*/
	if (NTOPBIT < NPKTBIT)
	{
		if (dst->pkt[NPKT - 1] & UNUSED)
		{
			oflow = 1;
			dst->pkt[NPKT - 1] &= ~UNUSED;
		}
	}
	return oflow;
}

int
#ifdef __STDC__
num_snarrow(IntNum *np, Uint nb)	/* truncate number to nb signed bits */
#else
num_snarrow(np,nb)IntNum*np;Uint nb;
#endif
{
	int oflow;

	if (nb > NUMSIZE)
		(*numfatal)(gettxt(":1483","num_snarrow():bad number of bits: %u"), nb);
	if ((nb = NUMSIZE - nb) == 0)
		return 0;
	oflow = num_alshift(np, nb);
	num_arshift(np, nb);
	return oflow;
}

int
#ifdef __STDC__
num_unarrow(IntNum *np, Uint nb)	/* truncate number to nb unsigned bits */
#else
num_unarrow(np,nb)IntNum*np;Uint nb;
#endif
{
	int oflow;

	if (nb > NUMSIZE)
		(*numfatal)(gettxt(":1484","num_unarrow():bad number of bits: %u"), nb);
	if ((nb = NUMSIZE - nb) == 0)
		return 0;
	oflow = num_llshift(np, nb);
	num_lrshift(np, nb);
	return oflow;
}

int
#ifdef __STDC__
num_negate(IntNum *np)	/* num = -num */
#else
num_negate(np)IntNum*np;
#endif
{
	num_complement(np);
	return num_sadd(np, &prenum[1]);
}

void
#ifdef __STDC__
num_not(IntNum *np)	/* num = !num */
#else
num_not(np)IntNum*np;
#endif
{
	*np = prenum[num_ucompare(np, &prenum[0]) == 0];
}

void
#ifdef __STDC__
num_or(IntNum *dst, const IntNum *src)		/* dst |= src */
#else
num_or(dst,src)IntNum*dst,*src;
#endif
{
	register PTYPE *d = &dst->pkt[NPKT];
	register const PTYPE *s = &src->pkt[NPKT];
	int i = NPKT;

	do
		*--d |= *--s;
	while (--i != 0);
}

int
#ifdef __STDC__
num_sremainder(IntNum *dst, const IntNum *src)	/* dst %= src (signed) */
#else
num_sremainder(dst,src)IntNum*dst,*src;
#endif
{
	IntNum tmp, numer;

	numer = *dst;
	return num_sdivrem(&tmp, dst, &numer, src) >> 1;
}

int
#ifdef __STDC__
num_uremainder(IntNum *dst, const IntNum *src)	/* dst %= src (unsigned) */
#else
num_uremainder(dst,src)IntNum*dst,*src;
#endif
{
	IntNum tmp, numer;

	numer = *dst;
	return num_udivrem(&tmp, dst, &numer, src);
}

void
#ifdef __STDC__
num_lrotate(IntNum *np, int n)	/* num <<<= n */
#else
num_lrotate(np,n)IntNum*np;int n;
#endif
{
	IntNum wrap;

	if (n < 0)
	{
		/*CONSTANTCONDITION*/
		if (INT_MIN < -INT_MAX)		/* two's complement */
		{
			if (n == INT_MIN)
			{
				/*CONSTANTCONDITION*/
				if (INT_MIN % -NUMSIZE < 0)
					n = INT_MIN % -NUMSIZE;
				else
					n = -(INT_MIN % -NUMSIZE);
			}
		}
		num_rrotate(np, -n);
		return;
	}
	if ((n %= NUMSIZE) == 0)
		return;
	wrap = *np;
	num_lrshift(&wrap, NUMSIZE - n);
	num_llshift(np, n);
	num_or(np, &wrap);
}

void
#ifdef __STDC__
num_rrotate(IntNum *np, int n)	/* num >>>= n */
#else
num_rrotate(np,n)IntNum*np;int n;
#endif
{
	IntNum wrap;

	if (n < 0)
	{
		/*CONSTANTCONDITION*/
		if (INT_MIN < -INT_MAX)		/* two's complement */
		{
			if (n == INT_MIN)
			{
				/*CONSTANTCONDITION*/
				if (INT_MIN % -NUMSIZE < 0)
					n = INT_MIN % -NUMSIZE;
				else
					n = -(INT_MIN % -NUMSIZE);
			}
		}
		num_lrotate(np, -n);
		return;
	}
	if ((n %= NUMSIZE) == 0)
		return;
	wrap = *np;
	num_llshift(&wrap, NUMSIZE - n);
	num_lrshift(np, n);
	num_or(np, &wrap);
}

int
#ifdef __STDC__
num_ssubtract(IntNum *dst, const IntNum *src)	/* dst -= src (signed) */
#else
num_ssubtract(dst,src)IntNum*dst,*src;
#endif
{
	IntNum negsrc;
	int oflow;

	negsrc = *src;
	oflow = num_negate(&negsrc);
	oflow |= num_sadd(dst, &negsrc);
	return oflow;
}

int
#ifdef __STDC__
num_usubtract(IntNum *dst, const IntNum *src)	/* dst -= src (unsigned) */
#else
num_usubtract(dst,src)IntNum*dst,*src;
#endif
{
	IntNum negsrc;
	int oflow = 0;

	/*
	* Overflows only when dst < src.
	*/
	if (num_ucompare(dst, src) < 0)
		oflow = 1;
	negsrc = *src;
	(void)num_negate(&negsrc);
	(void)num_uadd(dst, &negsrc);
	return oflow;
}

void
#ifdef __STDC__
num_xor(IntNum *dst, const IntNum *src)		/* dst ^= src */
#else
num_xor(dst,src)IntNum*dst,*src;
#endif
{
	register PTYPE *d = &dst->pkt[NPKT];
	register const PTYPE *s = &src->pkt[NPKT];
	int i = NPKT;

	do
		*--d ^= *--s;
	while (--i != 0);
}

	/*
	* Input functions.
	* Each of the num_from... functions take as a first argument,
	* and return a IntNum pointer.  If the argument is a null pointer,
	* a pointer to a possibly shared, possibly allocated IntNum
	* object will be returned.  Otherwise, the IntNum pointed to by
	* the first argument will contain the result, although if a
	* shared IntNum is also available, a pointer to it will always
	* be returned.
	*/

IntNum *
#ifdef __STDC__
num_fromslong(IntNum *dst, long val)	/* number from long value */
#else
num_fromslong(dst,val)IntNum*dst;long val;
#endif
{
	if (val >= 0)
		return num_fromulong(dst, (Ulong)val);
	if (dst == 0)
	{
		if ((dst = (IntNum *)(*numalloc)((void *)0, sizeof(IntNum)))
			== 0)
		{
			(*numfatal)(gettxt(":1485","num_fromslong():cannot allocate number"));
		}
	}
	/*CONSTANTCONDITION*/
	if (LONG_MIN < -LONG_MAX)	/* two's complement */
	{
		if (val != LONG_MIN)
			val = -val;	/* LONG_MIN untouched on 2's comp. */
	}
	else
		val = -val;
	setnum(dst, (Ulong)val);
	num_negate(dst);
	return dst;
}

IntNum *
#ifdef __STDC__
num_fromulong(IntNum *dst, Ulong val)	/* number from Ulong value */
#else
num_fromulong(dst,val)IntNum*dst;Ulong val;
#endif
{
	if (val < NPRENUM)
	{
		if (dst != 0)
			*dst = prenum[val];
		return &prenum[val];
	}
	if (dst == 0)
	{
		if ((dst = (IntNum *)(*numalloc)((void *)0, sizeof(IntNum)))
			== 0)
		{
			(*numfatal)(gettxt(":1485","num_fromulong():cannot allocate number"));
		}
	}
	setnum(dst, val);
	return dst;
}

#ifdef NUM_LONG_LONG

IntNum *
#ifdef __STDC__
num_fromslonglong(IntNum *dst, Llong val) /* number from Llong value */
#else
num_fromslonglong(dst,val)IntNum*dst;Llong val;
#endif
{
	if (val >= 0)
		return num_fromulonglong(dst, (ULlong)val);
	if (dst == 0)
	{
		if ((dst = (IntNum *)(*numalloc)((void *)0, sizeof(IntNum)))
			== 0)
		{
			(*numfatal)(gettxt(":0","num_fromslonglong():cannot allocate number"));
		}
	}
	/*CONSTANTCONDITION*/
	if (LONG_MIN < -LONG_MAX) /* two's complement */
	{
		if (val != (1ull << (BITSIZE(ULlong) - 1)))
			val = -val; /* LLONG_MIN untouched on 2's comp. */
	}
	else
		val = -val;
	llsetnum(dst, (ULlong)val);
	num_negate(dst);
	return dst;
}

IntNum *
#ifdef __STDC__
num_fromulonglong(IntNum *dst, ULlong val) /* number from ULlong value */
#else
num_fromulonglong(dst,val)IntNum*dst;ULlong val;
#endif
{
	if (val < NPRENUM)
	{
		if (dst != 0)
			*dst = prenum[val];
		return &prenum[val];
	}
	if (dst == 0)
	{
		if ((dst = (IntNum *)(*numalloc)((void *)0, sizeof(IntNum)))
			== 0)
		{
			(*numfatal)(gettxt(":0","num_fromulonglong():cannot allocate number"));
		}
	}
	llsetnum(dst, val);
	return dst;
}

#endif /*NUM_LONG_LONG*/

IntNum *
#ifdef __STDC__
num_fromnum(IntNum *dst, const IntNum *src) /* copy number; not prenums[] */
#else
num_fromnum(dst,val)IntNum*dst;IntNum*src;
#endif
{
	if (dst == 0)
	{
		if ((dst = (IntNum *)(*numalloc)((void *)0, sizeof(IntNum)))
			== 0)
		{
			(*numfatal)(gettxt(":0","num_fromnum():cannot allocate number"));
		}
	}
	*dst = *src;
	return dst;
}

static char *
#ifdef __STDC__
prtchar(int ch)		/* printable string version of character */
#else
prtchar(ch)int ch;
#endif
{
	static char ans[5];

	switch (ch)
	{
	default:
		if (ch >= 0 && isprint(ch))
		{
			ans[0] = ch;
			ans[1] = '\0';
			return ans;
		}
		break;
	case '\\':
		return "\\\\";
	case '\n':
		return "\\n";
	case '\t':
		return "\\t";
	}
	ans[3] = (ch & 07) + '0';
	ch >>= 3;
	ans[2] = (ch & 07) + '0';
	ch >>= 3;
	ans[1] = (ch & 07) + '0';
	ans[0] = '\\';
	return ans;
}

static void
#ifdef __STDC__
badchar(NumStrErr *err, const char *msg, const char *bad, int code)
#else
badchar(err,msg,bad,code)NumStrErr*err;char*msg,*bad;int code;
#endif
{
	static char buf[64]; /* assumed to be long enough */

	err->emsg = buf;
	err->next = bad;
	err->code = code;
	(void)strlist(buf, msg, prtchar(*(unsigned char *)bad), (char *)0);
}

static void
#ifdef __STDC__
baddig(NumStrErr *err, const char *bad, int base)
#else
baddig(err,bad,base)NumStrErr*err;char*bad;int base;
#endif
{
	static const char MSGbin[] = "binary";
	static const char MSGoct[] = "octal";
	static const char MSGdec[] = "decimal";
	static const char MSGhex[] = "hexadecimal";
	static const char MSGdig[] = " digit: ";
	static char errmsg[64] = "invalid ";
	const char *bstr;

	err->emsg = errmsg;
	err->next = bad;
	err->code = NUM_STRERR_INVALID;
	switch (base)
	{
	default:
		(*numfatal)(gettxt(":1486","baddig():bad base: %d"), base);
		/*NOTREACHED*/
	case 2:
		bstr = MSGbin;
		break;
	case 8:
		bstr = MSGoct;
		break;
	case 10:
		bstr = MSGdec;
		break;
	case 16:
		bstr = MSGhex;
		break;
	}
	(void)strlist(&errmsg[8], bstr, MSGdig,
		prtchar(*(unsigned char *)bad), (char *)0);
}

static IntNum *
#ifdef __STDC__
from2(IntNum *np, register const char *s, register size_t n, NumStrErr *err)
#else
from2(np,s,n,err)IntNum*np;register char*s;register size_t n;NumStrErr*err;
#endif
{
	IntNum x;
	register Ulong val = 0;
	register int nshift = 0;
	int spilled = 0, oflow = 0, dig;

	/*
	* Collect digits in a Ulong until the next shift would overflow.
	* OR the current Ulong into the big number and continue.
	*/
	do
	{
		dig = 0;
		if (*++s == '1')
			dig = 1;
		else if (*s != '0' && err != 0)
		{
			baddig(err, s, 2);
			break;
		}
		if ((val & HIGH(1)) != 0)
		{
			if (spilled)
			{
				oflow |= num_llshift(np, nshift);
				setnum(&x, val);
				num_or(np, &x);
			}
			else
			{
				spilled = 1;
				setnum(np, val);
			}
			nshift = 1;
			val = dig;
			continue;
		}
		nshift += 1;
		val <<= 1;
		val |= dig;
	} while (--n != 0);
	if (spilled)	/* OR in remaining value */
	{
		oflow |= num_llshift(np, nshift);
		setnum(&x, val);
		num_or(np, &x);
		if (oflow)
			return 0;
		return np;
	}
	if (val < NPRENUM)
		return &prenum[val];
	setnum(np, val);
	return np;
}

static IntNum *
#ifdef __STDC__
from8(IntNum *np, register const char *s, register size_t n, NumStrErr *err, int dig)
#else
from8(np,s,n,err,dig)IntNum*np;register char*s;register size_t n;NumStrErr*err;int dig;
#endif
{
	IntNum x;
	register Ulong val = dig;
	register int nshift = 0;
	int spilled = 0, oflow = 0;

	/*
	* Collect digits in a Ulong until the next shift would overflow.
	* OR the current Ulong into the big number and continue.
	*/
	do
	{
		dig = 0;
		if (*++s > '0' && *s <= '7')
			dig = *s - '0';
		else if (*s != '0' && err != 0)
		{
			baddig(err, s, 8);
			break;
		}
		if ((val & HIGH(3)) != 0)
		{
			if (spilled)
			{
				oflow |= num_llshift(np, nshift);
				setnum(&x, val);
				num_or(np, &x);
			}
			else
			{
				spilled = 1;
				setnum(np, val);
			}
			nshift = 3;
			val = dig;
			continue;
		}
		nshift += 3;
		val <<= 3;
		val |= dig;
	} while (--n != 0);
	if (spilled)	/* OR in remaining value */
	{
		oflow |= num_llshift(np, nshift);
		setnum(&x, val);
		num_or(np, &x);
		if (oflow)
			return 0;
		return np;
	}
	if (val < NPRENUM)
		return &prenum[val];
	setnum(np, val);
	return np;
}

static int
#ifdef __STDC__
spill10(IntNum *np, Ulong val, int ndig)	/* np *= 10**ndig; np += val */
#else
spill10(np,val,ndig)IntNum*np;Ulong val;int ndig;
#endif
{
	IntNum x;
	int oflow = 0;

	while (ndig > maxpow10)	/* loop on large values */
	{
		oflow |= num_umultiply(np, &pow10[maxpow10]);
		ndig -= maxpow10;
	}
	if (ndig <= (NPKT * NPKT) / (2 * NPKT))	/* probably cheaper to loop */
	{
		while (--ndig >= 0)	/* np*10 == np<<1 + np<<3 */
		{
			oflow |= num_llshift(np, 1);
			x = *np;
			oflow |= num_llshift(np, 2);
			oflow |= num_uadd(np, &x);
		}
	}
	else	/* probably cheaper to multiply */
	{
		oflow |= num_umultiply(np, &pow10[ndig]);
	}
	setnum(&x, val);
	oflow |= num_uadd(np, &x);
	return oflow;
}

static IntNum *
#ifdef __STDC__
from10(IntNum *np, register const char *s, register size_t n, NumStrErr *err, int dig)
#else
from10(np,s,n,err,dig)IntNum*np;register char*s;register size_t n;NumStrErr*err;int dig;
#endif
{
	register Ulong val = dig;
	int ndig = 1, oflow = 0, spilled = 0;

	/*
	* Collect digits in a Ulong until the next digit would cause
	* an overflow.
	*/
	do
	{
		dig = 0;
		if (isdigit(*++s))
			dig = *s - '0';
		else if (err != 0)
		{
			baddig(err, s, 10);
			break;
		}
		if (val < ULONG_MAX / 10
			|| val == ULONG_MAX / 10 && dig <= ULONG_MAX % 10)
		{
			ndig++;
			val *= 10;
			val += dig;
			continue;
		}
		if (spilled)
		{
			oflow |= spill10(np, val, ndig);
			ndig = 1;
			val = dig;
			continue;
		}
		spilled = 1;
		setnum(np, val);
		ndig = 1;
		val = dig;
	} while (--n != 0);
	if (spilled)
	{
		oflow |= spill10(np, val, ndig);
		if (oflow)
			return 0;
		return np;
	}
	if (val < NPRENUM)
		return &prenum[val];
	setnum(np, val);
	return np;
}

static IntNum *
#ifdef __STDC__
from16(IntNum *np, register const char *s, register size_t n, NumStrErr *err)
#else
from16(np,s,n,err)IntNum*np;register char*s;register size_t n;NumStrErr*err;
#endif
{
	IntNum x;
	register Ulong val = 0;
	register int nshift = 0;
	int spilled = 0, oflow = 0, dig;

	/*
	* Collect digits in a Ulong until the next shift would overflow.
	* OR the current Ulong into the big number and continue.
	*/
	do
	{
		dig = 0;
		if (isxdigit(*++s))
		{
			if (isdigit(*s))
				dig = *s - '0';
			else if (islower(*s))
				dig = *s - 'a' + 10;
			else
				dig = *s - 'A' + 10;
		}
		else if (err != 0)
		{
			baddig(err, s, 16);
			break;
		}
		if ((val & HIGH(4)) != 0)
		{
			if (spilled)
			{
				oflow |= num_llshift(np, nshift);
				setnum(&x, val);
				num_or(np, &x);
			}
			else
			{
				spilled = 1;
				setnum(np, val);
			}
			nshift = 4;
			val = dig;
			continue;
		}
		nshift += 4;
		val <<= 4;
		val |= dig;
	} while (--n != 0);
	if (spilled)	/* OR in remaining value */
	{
		oflow |= num_llshift(np, nshift);
		setnum(&x, val);
		num_or(np, &x);
		if (oflow)
			return 0;
		return np;
	}
	if (val < NPRENUM)
		return &prenum[val];
	setnum(np, val);
	return np;
}

IntNum *
#ifdef __STDC__
num_fromstr(IntNum *dst, const char *str, size_t len, NumStrErr *err) /* from digits */
#else
num_fromstr(dst,str,len,err)IntNum*dst;char*str;size_t len;NumStrErr*err;
#endif
{
	static const char MSGempty[] = "empty digit string";
	static const char MSGstart[] = "nonnumeric initial character: ";
	static const char MSGunkwn[] = "unknown base designator: 0";
	static const char MSGshort[] = "prefix-only number: 0";
	static const char MSGoflow[] = "overflow--number too big: ";
	static char *oflow_msg;
	static size_t len_oflow_msg;
	static IntNum *ans;	/* allocated result */
	IntNum *np;

	if ((np = dst) == 0 && (np = ans) == 0)
	{
		if ((np = (IntNum *)(*numalloc)((void *)0, sizeof(IntNum)))
			== 0)
		{
			(*numfatal)(gettxt(":1487","num_fromstr():cannot allocate number"));
		}
		ans = np;
	}
	/*
	* Handle both special cases of no digits.
	*/
	if (len == 0)
	{
		if (err != 0)
		{
			err->emsg = MSGempty;
			err->next = str;
			err->code = NUM_STRERR_EMPTY;
		}
		if (dst != 0)
			*dst = prenum[0];
		return &prenum[0];
	}
	if (!isdigit(str[0]))
	{
		if (err != 0)
			badchar(err, MSGstart, str, NUM_STRERR_INVALID);
		if (dst != 0)
			*dst = prenum[0];
		return &prenum[0];
	}
	if (err != 0)
	{
		err->emsg = 0;
		err->next = &str[len];
		err->code = NUM_STRERR_NONE;
	}
	/*
	* Determine base.  For bases 8 and 10, make sure the first
	* digit is good.  (We had to look at it anyway to determine
	* the base.)  Let fixed-base functions do the rest.
	*/
	if (len == 1)	/* done */
	{
		np = &prenum[str[0] - '0'];
	}
	else if (str[0] != '0')	/* decimal */
	{
		if (len > 2 || !isdigit(str[1])) /* broken or too long */
			np = from10(np, str, len - 1, err, str[0] - '0');
		else
			np = &prenum[(str[0] - '0') * 10 + (str[1] - '0')];
	}
	else	/* binary, octal, or hexadecimal */
	{
		switch (str[1])		/* len >= 2 */
		{
		default:
			if (err != 0)
				badchar(err, MSGunkwn, str + 1, NUM_STRERR_INVALID);
			if (dst != 0)
				*dst = prenum[0];
			return &prenum[0];
		too_short:;
			if (err != 0)
				badchar(err, MSGshort, str + 1, NUM_STRERR_PREFIX);
			if (dst != 0)
				*dst = prenum[0];
			return &prenum[0];
		case 'b':
		case 'B':
			if (len == 2)
				goto too_short;
			np = from2(np, str + 1, len - 2, err);
			break;
		case 'x':
		case 'X':
			if (len == 2)
				goto too_short;
			np = from16(np, str + 1, len - 2, err);
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
			if (len == 2)
			{
				np = &prenum[str[1] - '0'];
				break;
			}
			np = from8(np, str + 1, len - 2, err, str[1] - '0');
			break;
		}
	}
	if (np == 0)	/* overflow occurred */
	{
		if (err != 0)
		{
			if (sizeof(MSGoflow) + len > len_oflow_msg)
			{
				/*
				* Make sure that the allocation size cannot
				* match the sizeof(IntNum).  This makes an
				* allocation front-end function easier:
				* A request for sizeof(IntNum) bytes is
				* always a request for a new IntNum object.
				*/
				if ((len_oflow_msg = sizeof(MSGoflow) + len)
					== sizeof(IntNum))
				{
					len_oflow_msg++;
				}
				if ((oflow_msg = (*numalloc)((void *)oflow_msg,
					len_oflow_msg)) == 0)
				{
					(*numfatal)(gettxt(":1488","num_fromstr():cannot allocate string"));
				}
			}
			err->emsg = strcpy(oflow_msg, MSGoflow);
			err->next = &str[len];
			err->code = NUM_STRERR_RANGE;
			(void)strncpy(&oflow_msg[sizeof(MSGoflow) - 1], str, len);
			oflow_msg[len_oflow_msg - 1] = '\0';
		}
		/*
		* Restore np's original value:  Don't bother checking for
		* possible prenum[] match of wrapped-around value.
		*/
		if ((np = dst) == 0)
			np = ans;
	}
	if (np == ans)	/* dst == 0 && np is not prenum[] entry */
		ans = 0;
	else if (dst != 0 && np != dst)	/* np is prenum[] entry */
		*dst = *np;
	return np;
}

	/*
	* Printable output functions.
	* These functions return pointers to static buffers.
	* The next call to the same function will probably
	* cause the old result to be overwritten.
	*/

static const char zerostr[] = "0";

char *
#ifdef __STDC__
num_tobin(const IntNum *np)	/* binary digit sequence for number */
#else
num_tobin(np)IntNum*np;
#endif
{
	static char ans[2 + NPKT * NPKTBIT + 1]; /* 0b + digits + \0 */
	register char *res = &ans[sizeof(ans) - 1]; /* points at final '\0' */
	const PTYPE *p = &np->pkt[0];
	register PTYPE val;
	int i, j;

	i = NPKT;
	do
	{
		val = *p++;
		j = NPKTBIT;
		do
		{
			*--res = (val & LOW(1)) + '0';
			val >>= 1;
		} while (--j != 0);
	} while (--i != 0);
	/*
	* Trim leading zeroes; add "0b" prefix.
	*/
	while (*res == '0')
		res++;
	if (*res == '\0')	/* too far */
		return (char *)zerostr;
	*--res = 'b';
	*--res = '0';
	return res;
}

char *
#ifdef __STDC__
num_tosdec(const IntNum *np)	/* signed decimal digit sequence for number */
#else
num_tosdec(np)IntNum*np;
#endif
{
	IntNum negval;
	char *res;

	if (SIGN(np->pkt) == 0)
		return num_toudec(np);
	negval = *np;
	num_negate(&negval);
	res = num_toudec(&negval);
	*--res = '-';	/* room guaranteed by num_toudec */
	return res;
}

char *
#ifdef __STDC__
num_toudec(const IntNum *np)	/* unsigned decimal digit sequence for number */
#else
num_toudec(np)IntNum*np;
#endif
{
	static char ans[1 + (NPKT * NPKTBIT + 28) / 29 * 9 + 1];
	register char *res = &ans[sizeof(ans) - 1]; /* points at final '\0' */
	IntNum cur, quot, rem, ten_9;
	int i, n;
	Ulong val;

	/*
	* Ulongs hold at least 32 bits: decimal values up to at least
	* 4294967295 are covered.  Thus we take the value 1000000000
	* (10**9) as a workable value to reduce the number at each stage.
	* Each division by 10**9 produces a remainder with exactly 9
	* decimal digits and reduces the current value by at least 2**29
	* (999999999 == 0b111011100110101100100111111111).  But, the only
	* two uses for 29 are as an upper bound for the loop and the
	* length of the result array.
	*/
	setnum(&ten_9, (Ulong)1000000000);
	cur = *np;
	n = NPKT * NPKTBIT;	/* not NUMSIZE */
	do
	{
		num_udivrem(&quot, &rem, &cur, &ten_9);
		(void)setul(&rem, &val);
		i = 9;
		do
		{
			*--res = (val % 10) + '0';
			val /= 10;
		} while (--i != 0);
		cur = quot;
	} while ((n -= 29) >= 0);
	/*
	* Trim leading zeroes.
	*/
	while (*res == '0')
		res++;
	if (*res == '\0')	/* too far */
		return (char *)zerostr;
	return res;
}

char *
#ifdef __STDC__
num_tohex(const IntNum *np)	/* hexadecimal digit sequence for number */
#else
num_tohex(np)IntNum*np;
#endif
{
	static const char hex[] = "0123456789abcdef";
	static char ans[2 + NPKT * NPKTBIT / 4 + 1]; /* 0x + digits + \0 */
	register char *res = &ans[sizeof(ans) - 1]; /* points at final '\0' */
	const PTYPE *p = &np->pkt[0];
	int i, j;

	/*CONSTANTCONDITION*/
	if ((NPKTBIT % 4) == 0)	/* simple: each packet is a multiple of 4 */
	{
		register PTYPE val;

		i = NPKT;
		do
		{
			val = *p++;
			j = NPKTBIT;
			do
			{
				*--res = hex[val & LOW(4)];
				val >>= 4;
			} while ((j -= 4) != 0);
		} while (--i != 0);
	}
	else	/* convert an unsigned long at a time */
	{
		IntNum tmp;
		Ulong x;
		register Ulong val;
		int more;

		if ((more = num_toulong(np, &x)) != 0)
			tmp = *np;
		val = x;
		for (;;)
		{
			j = BITSIZE(Ulong);
			do
			{
				*--res = hex[val & LOW(4)];
				val >>= 4;
			} while ((j -= 4) >= 4);
			/*CONSTANTCONDITION*/
			if ((BITSIZE(Ulong) % 4) == 0)	/* entire Ulong used */
			{
				if (!more)
					break;
				num_lrshift(&tmp, (int)BITSIZE(Ulong));
				more = setul(&tmp, &x);
				val = x;
			}
			else	/* need to fold leftover into next Ulong */
			{
				if (!more)
				{
					*--res = hex[val];
					break;
				}
				num_lrshift(&tmp, (int)BITSIZE(Ulong) / 4 * 4);
				if ((more = setul(&tmp, &x)) == 0
					&& (x & HIGH(BITSIZE(Ulong) % 4)) != 0)
				{
					more = 1;
				}
				x <<= BITSIZE(Ulong) % 4;
				val |= x;
			}
		}
	}
	/*
	* Trim leading zeroes; add "0x" prefix.
	*/
	while (*res == '0')
		res++;
	if (*res == '\0')	/* too far */
		return (char *)zerostr;
	*--res = 'x';
	*--res = '0';
	return res;
}

char *
#ifdef __STDC__
num_tooct(const IntNum *np)	/* octal digit sequence for number */
#else
num_tooct(np)IntNum*np;
#endif
{
	static char ans[1 + NPKT * NPKTBIT / 3 + 1]; /* 0 + digits + \0 */
	register char *res = &ans[sizeof(ans) - 1]; /* points at final '\0' */
	const PTYPE *p = &np->pkt[0];
	int i, j;

	/*CONSTANTCONDITION*/
	if ((NPKTBIT % 3) == 0)	/* simple: each packet is a multiple of 3 */
	{
		register PTYPE val;

		i = NPKT;
		do
		{
			val = *p++;
			j = NPKTBIT;
			do
			{
				*--res = (val & LOW(3)) + '0';
				val >>= 3;
			} while ((j -= 3) != 0);
		} while (--i != 0);
	}
	else	/* convert an unsigned long at a time */
	{
		IntNum tmp;
		Ulong x;
		register Ulong val;
		int more;

		if ((more = num_toulong(np, &x)) != 0)
			tmp = *np;
		val = x;
		for (;;)
		{
			j = BITSIZE(Ulong);
			do
			{
				*--res = (val & LOW(3)) + '0';
				val >>= 3;
			} while ((j -= 3) >= 3);
			/*CONSTANTCONDITION*/
			if ((BITSIZE(Ulong) % 3) == 0)	/* entire Ulong used */
			{
				if (!more)
					break;
				num_lrshift(&tmp, (int)BITSIZE(Ulong));
				more = setul(&tmp, &x);
				val = x;
			}
			else	/* need to fold leftover into next Ulong */
			{
				if (!more)
				{
					*--res = val + '0';
					break;
				}
				num_lrshift(&tmp, (int)BITSIZE(Ulong) / 3 * 3);
				if ((more = setul(&tmp, &x)) == 0
					&& (x & HIGH(BITSIZE(Ulong) % 3)) != 0)
				{
					more = 1;
				}
				x <<= BITSIZE(Ulong) % 3;
				val |= x;
			}
		}
	}
	/*
	* Trim all but one leading zero.
	*/
	while (*res == '0')
		res++;
	if (*res == '\0')	/* too far */
		return (char *)zerostr;
	*--res = '0';
	return res;
}

	/*
	* Binary output functions.
	*/

void
#ifdef __STDC__
num_tosbendian(const IntNum *np, register Uchar *p, size_t n)
#else
num_tosbendian(np,p,n)IntNum*np;register Uchar*p;size_t n;
#endif
{
	IntNum tmp;
	register Ulong val;
	Ulong newval;
	int i;

	if (n == 0)
		return;
	if (n > sizeof(Ulong))	/* only copy when necessary */
		tmp = *np;
	p += n;
	num_toulong(np, &newval);
	val = newval;
	for (;;)
	{
		if ((i = n) > sizeof(Ulong))
			i = sizeof(Ulong);
		n -= i;
		do
		{
			*--p = val;
			val >>= CHAR_BIT;
		} while (--i != 0);
		if (n == 0)
			return;
		num_arshift(&tmp, (int)BITSIZE(Ulong));
		setul(&tmp, &newval);
		val = newval;
	}
}

void
#ifdef __STDC__
num_toubendian(const IntNum *np, register Uchar *p, size_t n)
#else
num_toubendian(np,p,n)IntNum*np;register Uchar*p;size_t n;
#endif
{
	IntNum tmp;
	register Ulong val;
	Ulong newval;
	int i;

	if (n == 0)
		return;
	if (n > sizeof(Ulong))	/* only copy when necessary */
		tmp = *np;
	p += n;
	num_toulong(np, &newval);
	val = newval;
	for (;;)
	{
		if ((i = n) > sizeof(Ulong))
			i = sizeof(Ulong);
		n -= i;
		do
		{
			*--p = val;
			val >>= CHAR_BIT;
		} while (--i != 0);
		if (n == 0)
			return;
		num_lrshift(&tmp, (int)BITSIZE(Ulong));
		setul(&tmp, &newval);
		val = newval;
	}
}

void
#ifdef __STDC__
num_toslendian(const IntNum *np, register Uchar *p, size_t n)
#else
num_toslendian(np,p,n)IntNum*np;register Uchar*p;size_t n;
#endif
{
	IntNum tmp;
	register Ulong val;
	Ulong newval;
	int i;

	if (n == 0)
		return;
	if (n > sizeof(Ulong))	/* only copy when necessary */
		tmp = *np;
	num_toulong(np, &newval);
	val = newval;
	for (;;)
	{
		if ((i = n) > sizeof(Ulong))
			i = sizeof(Ulong);
		n -= i;
		do
		{
			*p++ = val;
			val >>= CHAR_BIT;
		} while (--i != 0);
		if (n == 0)
			return;
		num_arshift(&tmp, (int)BITSIZE(Ulong));
		setul(&tmp, &newval);
		val = newval;
	}
}

void
#ifdef __STDC__
num_toulendian(const IntNum *np, register Uchar *p, size_t n)
#else
num_toulendian(np,p,n)IntNum*np;register Uchar*p;size_t n;
#endif
{
	IntNum tmp;
	register Ulong val;
	Ulong newval;
	int i;

	if (n == 0)
		return;
	if (n > sizeof(Ulong))	/* only copy when necessary */
		tmp = *np;
	num_toulong(np, &newval);
	val = newval;
	for (;;)
	{
		if ((i = n) > sizeof(Ulong))
			i = sizeof(Ulong);
		n -= i;
		do
		{
			*p++ = val;
			val >>= CHAR_BIT;
		} while (--i != 0);
		if (n == 0)
			return;
		num_lrshift(&tmp, (int)BITSIZE(Ulong));
		setul(&tmp, &newval);
		val = newval;
	}
}

int
#ifdef __STDC__
num_tosint(const IntNum *np, int *dst)
#else
num_tosint(np,dst)IntNum*np;int*dst;
#endif
{
	Uint tdst;
	int oflow;

	if (SIGN(np->pkt) == 0)
	{
		if (num_touint(np, (Uint *)dst) != 0 || *dst < 0)
			return 1;
		return 0;
	}
	/*CONSTANTCONDITION*/
	if (INT_MIN < -INT_MAX)		/* two's complement */
	{
		register const PTYPE *p = &np->pkt[NPKT];
		int i = NPKT;

		oflow = 0;
		tdst = ~(Uint)0;
		do
		{
			if ((tdst & HIGHINT(NPKTBIT)) != HIGHINT(NPKTBIT))
				oflow = 1;
			tdst <<= NPKTBIT;
			tdst |= *--p;
		} while (--i != 0);
		*dst = *(int *)&tdst;
	}
	else
	{
		IntNum pos;

		pos = *np;
		oflow = num_negate(&pos);
		if (num_touint(&pos, (Uint *)dst) != 0 || *dst < 0)
			oflow = 1;
		*dst = -*dst;	/* can't overflow: not 2's complement */
	}
	return oflow;
}

int
#ifdef __STDC__
num_touint(const IntNum *np, register Uint *dst)
#else
num_touint(np,dst)IntNum*np;register Uint*dst;
#endif
{
	register const PTYPE *p;
	register Uint tdst;
	int oflow, i;

	if (np >= &prenum[0] && np < &prenum[NPRENUM])
	{
		*dst = np - &prenum[0];
		return 0;
	}
	oflow = 0;
	p = &np->pkt[NPKT];
	i = NPKT;
	tdst = 0;
	do
	{
		if ((tdst & HIGHINT(NPKTBIT)) != 0)
			oflow = 1;
		tdst <<= NPKTBIT;
		tdst |= *--p;
	} while (--i != 0);
	*dst = tdst;
	return oflow;
}

int
#ifdef __STDC__
num_toslong(const IntNum *np, long *dst)
#else
num_toslong(np,dst)IntNum*np;long*dst;
#endif
{
	Ulong tdst;
	int oflow;

	if (SIGN(np->pkt) == 0)
	{
		if (num_toulong(np, (Ulong *)dst) != 0 || *dst < 0)
			return 1;
		return 0;
	}
	/*CONSTANTCONDITION*/
	if (LONG_MIN < -LONG_MAX)	/* two's complement */
	{
		register const PTYPE *p = &np->pkt[NPKT];
		int i = NPKT;

		oflow = 0;
		tdst = ~(Ulong)0;
		do
		{
			if ((tdst & HIGH(NPKTBIT)) != HIGH(NPKTBIT))
				oflow = 1;
			tdst <<= NPKTBIT;
			tdst |= *--p;
		} while (--i != 0);
		*dst = *(long *)&tdst;
	}
	else
	{
		IntNum pos;

		pos = *np;
		oflow = num_negate(&pos);
		if (setul(&pos, (Ulong *)dst) != 0 || *dst < 0)
			oflow = 1;
		*dst = -*dst;	/* can't overflow: not 2's complement */
	}
	return oflow;
}

int
#ifdef __STDC__
num_toulong(const IntNum *np, register Ulong *dst)
#else
num_toulong(np,dst)IntNum*np;register Ulong*dst;
#endif
{
	register const PTYPE *p;
	register Ulong tdst;
	int oflow, i;

	if (np >= &prenum[0] && np < &prenum[NPRENUM])
	{
		*dst = np - &prenum[0];
		return 0;
	}
	/*
	* Inline copy of setul().
	*/
	oflow = 0;
	p = &np->pkt[NPKT];
	i = NPKT;
	tdst = 0;
	do
	{
		if ((tdst & HIGH(NPKTBIT)) != 0)
			oflow = 1;
		tdst <<= NPKTBIT;
		tdst |= *--p;
	} while (--i != 0);
	*dst = tdst;
	return oflow;
}

#ifdef NUM_LONG_LONG

int
#ifdef __STDC__
num_toslonglong(const IntNum *np, Llong *dst)
#else
num_toslonglong(np,dst)IntNum*np;Llong*dst;
#endif
{
	ULlong tdst;
	int oflow;

	if (SIGN(np->pkt) == 0)
	{
		if (num_toulonglong(np, (ULlong *)dst) != 0 || *dst < 0)
			return 1;
		return 0;
	}
	/*CONSTANTCONDITION*/
	if (LONG_MIN < -LONG_MAX) /* two's complement */
	{
		register const PTYPE *p = &np->pkt[NPKT];
		int i = NPKT;

		oflow = 0;
		tdst = ~(ULlong)0;
		do
		{
			if ((tdst & HIGHLL(NPKTBIT)) != HIGHLL(NPKTBIT))
				oflow = 1;
			tdst <<= NPKTBIT;
			tdst |= *--p;
		} while (--i != 0);
		*dst = *(Llong *)&tdst;
	}
	else
	{
		IntNum pos;

		pos = *np;
		oflow = num_negate(&pos);
		if (num_toulonglong(&pos, (ULlong *)dst) != 0 || *dst < 0)
			oflow = 1;
		*dst = -*dst;	/* can't overflow: not 2's complement */
	}
	return oflow;
}

int
#ifdef __STDC__
num_toulonglong(const IntNum *np, ULlong *dst)
#else
num_toulonglong(np,dst)IntNum*np;ULlong*dst;
#endif
{
	register const PTYPE *p;
	register ULlong tdst;
	int oflow, i;

	if (np >= &prenum[0] && np < &prenum[NPRENUM])
	{
		*dst = np - &prenum[0];
		return 0;
	}
	oflow = 0;
	p = &np->pkt[NPKT];
	i = NPKT;
	tdst = 0;
	do
	{
		if ((tdst & HIGHLL(NPKTBIT)) != 0)
			oflow = 1;
		tdst <<= NPKTBIT;
		tdst |= *--p;
	} while (--i != 0);
	*dst = tdst;
	return oflow;
}

#endif /*NUM_LONG_LONG*/
