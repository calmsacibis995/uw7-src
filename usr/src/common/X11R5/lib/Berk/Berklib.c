/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)berklib:Berklib.c	1.4"


/* $XConsortium: Berklib.c,v 1.15 91/09/10 08:50:04 rws Exp $ */

/*
 * These are routines found in BSD but not on all other systems.  The core
 * MIT distribution does not use them except for ffs in the server, unless
 * the system is seriously deficient.  You should enable only the ones that
 * you need for your system.  Use Xfuncs.h in clients to avoid using the
 * slow versions of bcopy, bcmp, and bzero provided here.
 */

#include <sys/types.h>

#ifdef hpux
#define WANT_BFUNCS
#define WANT_FFS
#define WANT_RANDOM
#endif

#ifdef macII
/* silly bcopy in A/UX 2.0.1 does not handle overlaps */
#define WANT_BFUNCS
#define NO_BZERO
#endif

#ifdef SVR4
#define WANT_BFUNCS
#define WANT_FFS
#define WANT_RANDOM
#define WANT_USLEEP
#endif

#ifdef hcx
#define WANT_FFS
#endif

#ifdef SYSV386
#ifdef SYSV
#define WANT_FFS
#endif
#endif

#ifdef WANT_USLEEP
#include <poll.h>
#endif

/* you should use Xfuncs.h in code instead of relying on Berklib */
#ifdef WANT_BFUNCS

#include <X11/Xosdefs.h>

#if (__STDC__ && defined(X_NOT_STDC_ENV)) || defined(SVR4) || defined(hpux)

#include <string.h>

void bcopy (b1, b2, length)
    register char *b1, *b2;
    register int length;
{
    memmove(b2, b1, (size_t)length);
}

int bcmp (b1, b2, length)
    register char *b1, *b2;
    register int length;
{
    return memcmp(b1, b2, (size_t)length);
}

void bzero (b, length)
    register char *b;
    register int length;
{
    memset(b, 0, (size_t)length);
}

#else

void bcopy (b1, b2, length)
    register char *b1, *b2;
    register int length;
{
    if (b1 < b2) {
	b2 += length;
	b1 += length;
	while (length--)
	    *--b2 = *--b1;
    } else {
	while (length--)
	    *b2++ = *b1++;
    }
}

#if defined(SYSV)

#include <memory.h>

int bcmp (b1, b2, length)
    register char *b1, *b2;
    register int length;
{
    return memcmp(b1, b2, length);
}

#ifndef NO_BZERO

bzero (b, length)
    register char *b;
    register int length;
{
    memset(b, 0, length);
}

#endif

#else

int bcmp (b1, b2, length)
    register char *b1, *b2;
    register int length;
{
    while (length--) {
	if (*b1++ != *b2++) return 1;
    }
    return 0;
}

void bzero (b, length)
    register char *b;
    register int length;
{
    while (length--)
	*b++ = '\0';
}

#endif
#endif
#endif /* WANT_BFUNCS */

#ifdef WANT_FFS
int
ffs(mask)
unsigned int	mask;
{
    register i;

    if ( ! mask ) return 0;
    i = 1;
    while (! (mask & 1)) {
	i++;
	mask = mask >> 1;
    }
    return i;
}
#endif

#ifdef WANT_RANDOM
#if defined(SYSV) || defined(SVR4)

long lrand48();

long random()
{
   return (lrand48());
}

void srandom(seed)
    int seed;
{
   srand48(seed);
}

#else

long random()
{
   return (rand());
}

void srandom(seed)
    int seed;
{
   srand(seed);
}

#endif
#endif /* WANT_RANDOM */
#ifdef WANT_USLEEP

usleep(useconds)
unsigned useconds;
{
static struct pollfd	dummy[1];

	poll (dummy, 0, useconds);
}

#endif

/*
 * insque, remque - insert/remove element from a queue
 *
 * DESCRIPTION
 *      Insque and remque manipulate queues built from doubly linked
 *      lists.  Each element in the queue must in the form of
 *      ``struct qelem''.  Insque inserts elem in a queue immedi-
 *      ately after pred; remque removes an entry elem from a queue.
 *
 * SEE ALSO
 *      ``VAX Architecture Handbook'', pp. 228-235.
 */

#ifdef WANT_QUE
struct qelem {
    struct    qelem *q_forw;
    struct    qelem *q_back;
    char *q_data;
    };

insque(elem, pred)
register struct qelem *elem, *pred;
{
    register struct qelem *q;
    /* Insert locking code here */
    if ( elem->q_forw = q = (pred ? pred->q_forw : pred) )
	q->q_back = elem;
    if ( elem->q_back = pred )
	pred->q_forw = elem;
    /* Insert unlocking code here */
}

remque(elem)
register struct qelem *elem;
{
    register struct qelem *q;
    if ( ! elem ) return;
    /* Insert locking code here */

    if ( q = elem->q_back ) q->q_forw = elem->q_forw;
    if ( q = elem->q_forw ) q->q_back = elem->q_back;

    /* insert unlocking code here */
}
#endif /* WANT_QUE */

/*
 * gettimeofday emulation
 * Caution -- emulation is incomplete
 *  - has only second, not microsecond, resolution.
 *  - does not return timezone info.
 */

#if WANT_GTOD
int gettimeofday (tvp, tzp)
    struct timeval *tvp;
    struct timezone *tzp;
{
    time (&tvp->tv_sec);
    tvp->tv_usec = 0L;

    if (tzp) {
	fprintf( stderr,
		 "Warning: gettimeofday() emulation does not return timezone\n"
		);
    }
}
#endif /* WANT_GTOD */

/*
 * USL additions
 */
char *
index (s, c)
char *s, c;
{
    return ((char *) strchr (s, c));
}

char *
rindex (s, c)
char *s, c;
{
    return ((char *) strrchr (s, c));
}

/*
 * Yanked from here to end of the file from libc-port:gen/drand48.c
 * If we don't do this, these will be picked up from libc.so which is fine
 * on our systems but these functions are not standard ABI, so there might
 * be compatibility problems. 6/11/92
 */

/*
 *	drand48, etc. pseudo-random number generator
 *	This implementation assumes unsigned short integers of at least
 *	16 bits, long integers of at least 32 bits, and ignores
 *	overflows on adding or multiplying two unsigned integers.
 *	Two's-complement representation is assumed in a few places.
 *	Some extra masking is done if unsigneds are exactly 16 bits
 *	or longs are exactly 32 bits, but so what?
 *	An assembly-language implementation would run significantly faster.
 */
#ifdef NOTNEEDED 
#ifdef __STDC__
	#pragma weak drand48 = _drand48
	#pragma weak erand48 = _erand48
	#pragma weak lrand48 = _lrand48
	#pragma weak mrand48 = _mrand48
	#pragma weak srand48 = _srand48
	#pragma weak seed48 = _seed48
	#pragma weak lcong48 = _lcong48
	#pragma weak nrand48 = _nrand48
	#pragma weak jrand48 = _jrand48
#endif
#include "synonyms.h"
#endif /* NOTNEEDED */
#ifndef HAVEFP
#define HAVEFP 1
#endif
#define N	16
#define MASK	((unsigned)(1 << (N - 1)) + (1 << (N - 1)) - 1)
#define LOW(x)	((unsigned)(x) & MASK)
#define HIGH(x)	LOW((x) >> N)
#define MUL(x, y, z)	{ long l = (long)(x) * (long)(y); \
		(z)[0] = LOW(l); (z)[1] = HIGH(l); }
#define CARRY(x, y)	((long)(x) + (long)(y) > MASK)
#define ADDEQU(x, y, z)	(z = CARRY(x, (y)), x = LOW(x + (y)))
#define X0	0x330E
#define X1	0xABCD
#define X2	0x1234
#define A0	0xE66D
#define A1	0xDEEC
#define A2	0x5
#define C	0xB
#define SET3(x, x0, x1, x2)	((x)[0] = (x0), (x)[1] = (x1), (x)[2] = (x2))
#define SETLOW(x, y, n) SET3(x, LOW((y)[n]), LOW((y)[(n)+1]), LOW((y)[(n)+2]))
#define SEED(x0, x1, x2) (SET3(x, x0, x1, x2), SET3(a, A0, A1, A2), c = C)
#define REST(v)	for (i = 0; i < 3; i++) { xsubi[i] = x[i]; x[i] = temp[i]; } \
		return (v)
#define NEST(TYPE, f, F) static TYPE f(xsubi) register unsigned short *xsubi; { \
	register int i; register TYPE v; unsigned temp[3]; \
	for (i = 0; i < 3; i++) { temp[i] = x[i]; x[i] = LOW(xsubi[i]); }  \
	v = F(); REST(v); }
#define HI_BIT	((unsigned)1L << (2 * N - 1))

static unsigned x[3] = { X0, X1, X2 }, a[3] = { A0, A1, A2 }, c = C;
static unsigned short lastx[3];
static void next();

#if HAVEFP
static double
drand48()
{
	static double two16m = 1.0 / (1L << N);
	next();
	return (two16m * (two16m * (two16m * x[0] + x[1]) + x[2]));
}

NEST(double, erand48, drand48)

#else

static long
irand48(m)
/* Treat x[i] as a 48-bit fraction, and multiply it by the 16-bit
 * multiplier m.  Return integer part as result.
 */
register unsigned short m;
{
	unsigned r[4], p[2], carry0 = 0;

	next();
	MUL(m, x[0], &r[0]);
	MUL(m, x[2], &r[2]);
	MUL(m, x[1], p);
	if (CARRY(r[1], p[0]))
		ADDEQU(r[2], 1, carry0);
	return (r[3] + carry0 + CARRY(r[2], p[1]));
}

static long
krand48(xsubi, m)
/* same as irand48, except user provides storage in xsubi[] */
register unsigned short *xsubi;
unsigned short m;
{
	register int i;
	register long iv;
	unsigned temp[3];

	for (i = 0; i < 3; i++) {
		temp[i] = x[i];
		x[i] = xsubi[i];
	}
	iv = irand48(m);
	REST(iv);
}
#endif

static long
lrand48()
{
	next();
	return (((long)x[2] << (N - 1)) + (x[1] >> 1));
}

static long
mrand48()
{
	register long l;

	next();
	/* sign-extend in case length of a long > 32 bits
						(as on Honeywell) */
	return ((l = ((long)x[2] << N) + x[1]) & HI_BIT ? l | -(unsigned long)HI_BIT : l);
}

static void
next()
{
	unsigned p[2], q[2], r[2], carry0, carry1;

	MUL(a[0], x[0], p);
	ADDEQU(p[0], c, carry0);
	ADDEQU(p[1], carry0, carry1);
	MUL(a[0], x[1], q);
	ADDEQU(p[1], q[0], carry0);
	MUL(a[1], x[0], r);
	x[2] = LOW(carry0 + carry1 + CARRY(p[1], r[0]) + q[1] + r[1] +
		a[0] * x[2] + a[1] * x[1] + a[2] * x[0]);
	x[1] = LOW(p[1] + r[0]);
	x[0] = LOW(p[0]);
}

static
srand48(seedval)
long seedval;
{
	SEED(X0, LOW(seedval), HIGH(seedval));
}

static unsigned short *
seed48(seed16v)
unsigned short seed16v[3];
{
	SETLOW(lastx, x, 0);
	SEED(LOW(seed16v[0]), LOW(seed16v[1]), LOW(seed16v[2]));
	return (lastx);
}

static
lcong48(param)
unsigned short param[7];
{
	SETLOW(x, param, 0);
	SETLOW(a, param, 3);
	c = LOW(param[6]);
}

NEST(long, nrand48, lrand48)

NEST(long, jrand48, mrand48)

#ifdef DRIVER
/*
	This should print the sequences of integers in Tables 2
		and 1 of the TM:
	1623, 3442, 1447, 1829, 1305, ...
	657EB7255101, D72A0C966378, 5A743C062A23, ...
 */
#include <stdio.h>

main()
{
	int i;

	for (i = 0; i < 80; i++) {
		printf("%4d ", (int)(4096 * drand48()));
		printf("%.4X%.4X%.4X\n", x[2], x[1], x[0]);
	}
}
#endif


#if defined (SVR4) && !defined (ARCHIVE) /* Note: this #define has been added */
#define select _abi_select               /* for ABI compliance. This #define  */
#endif /* SVR4 */                        /* appears in XlibInt.c, XConnDis.c  */
                                         /* NextEvent.c(libXt), Xmos.c(libXm) */
                                         /* fsio.c (libfont), and Berk.c      */					 /* (libX11.so.1)                     */
#include <sys/select.h>
int
XSelect( nfds, readfds, writefds, exceptfds, timeout )
int nfds;
unsigned long readfds[],
              writefds[],
              exceptfds[];
struct timeval *timeout;
{

  return  (select( nfds, (fd_set *)readfds, (fd_set *)writefds, 
                  (fd_set *)exceptfds, timeout ));
 
}


