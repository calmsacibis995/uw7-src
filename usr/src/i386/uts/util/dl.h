#ifndef _UTIL_DL_H	/* wrapper symbol for kernel use */
#define _UTIL_DL_H	/* subject to change without notice */

#ident	"@(#)kern-i386:util/dl.h	1.11"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Support for double long (64-bit) values.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Definition of a double long (64-bit) value.
 */
typedef	struct dl {
	ulong_t	dl_lop;		/* Intel has LSB at lowest byte */
	long	dl_hop;
} dl_t;


#if defined(__STDC__)

extern dl_t	ladd(dl_t, dl_t);
extern dl_t	lsub(dl_t, dl_t);
extern dl_t	lmul(dl_t, dl_t);
extern dl_t	ldivide(dl_t, dl_t);
extern dl_t	lmod(dl_t, dl_t);
extern dl_t	lshiftl(dl_t, int);
extern int	lsign(dl_t);

#ifndef _KERNEL
extern dl_t	llog10(dl_t);
extern dl_t	lexp10(dl_t);
#endif /* !_KERNEL */

#else

extern dl_t	ladd();
extern dl_t	lsub();
extern dl_t	lmul();
extern dl_t	ldivide();
extern dl_t	lshiftl();
extern int	lsign();

#ifndef _KERNEL
extern dl_t	llog10();
extern dl_t	lexp10();
#endif /* !_KERNEL */

#endif /* !__STDC__ */

#define		SIGNBIT		0x80000000

/*
 * void
 * LDIVIDE(dl_t num, dl_t den, dl_t quot)
 *
 * 	Macro for efficient, approximate double long division. Caller 
 *	guarantees that either the high_order OR the low_order word of 
 *	divisor (den) is nonzero.
 *
 *
 */
#define LDIVIDE(num, den, quot) { \
	if (((num).dl_hop & SIGNBIT) || ((den).dl_hop & SIGNBIT)) { \
		quot = ldivide(num, den); \
	} else 	if ((den).dl_hop == 0) { \
		(quot).dl_lop = \
			((uint_t)((num).dl_lop)) / ((uint_t)((den).dl_lop)); \
		if ((den).dl_lop > (num).dl_hop) { \
			if ((den).dl_lop > SIGNBIT) { \
		   		(quot).dl_lop += (num).dl_hop; \
			} else { \
		   		(quot).dl_lop += (2 * (num).dl_hop * \
				  ((uint_t)SIGNBIT / (uint_t)((den).dl_lop))); \
			} \
		   	(quot).dl_hop = 0; \
		} else if ((den).dl_lop > SIGNBIT) { \
			(quot).dl_lop += ((num).dl_hop % (den).dl_lop); \
			(quot).dl_hop = (num).dl_hop / (den).dl_lop; \
		} else { \
			(quot).dl_lop += (2 * ((num).dl_hop % (den).dl_lop) * \
				((uint_t)SIGNBIT / (uint_t)((den).dl_lop))); \
			(quot).dl_hop = (num).dl_hop / (den).dl_lop; \
		} \
	} else { \
		(quot).dl_hop = 0; \
		(quot).dl_lop = ((num).dl_hop / (den).dl_hop); \
	} \
}


#ifdef _KERNEL
extern dl_t	dl_zero;
extern dl_t	dl_one;
#else /* !_KERNEL */
extern dl_t	lzero;
extern dl_t	lone;
extern dl_t	lten;
#endif /* !_KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_DL_H */
