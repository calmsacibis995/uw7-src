#ifndef _UTIL_BITMASKS_H	/* wrapper symbol for kernel use */
#define _UTIL_BITMASKS_H	/* subject to change without notice */

#ident	"@(#)kern-i386:util/bitmasks.h	1.24"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Macros/functions to work with bits in arbitrarily long bit arrays.
 * The interfaces are common, and all of these functions will be available
 * on all systems, but the implementation is family-specific, to optimize
 * for particular processors.
 *
 * Since some of these interfaces may be implemented as macros, care must be
 * taken not to pass any arguments with side effects (such as "foo++")
 * since they may be evaluated multiple times.
 *
 * For each operation XXX (e.g. CLRALL, SET1), there are three interfaces:
 * BITMASKN_XXX, which is the general purpose form that allows any length
 * (> 0) of bit array; BITMASK1_XXX, which is optimized for the case of one
 * word (uint_t); and BITMASK2_XXX, which is optimized for two words.
 * Note that the general purpose interface (BITMASKN_XXX) is not available
 * if your compiler does not support new-style asm's.
 *
 * Available operations are:
 *	CLRALL		Clear all bits in the bit array.
 *	SETALL		Set all bits in the bit array.
 *	TESTALL		Return non-zero if any bit is set.
 *	CLR1		Clear an individual bit.
 *	SET1		Set an individual bit.
 *	TEST1		Test one bit; return non-zero if bit is set.
 *	CLRN		Clear all bits in dst_bits which are set in src_bits.
 *	SETN		Set all bits in dst_bits which are set in src_bits.
 *	TESTN		Return non-zero if any bit is set in both bit arrays.
 *	ANDN		Clear all bits in dst_bits which aren't set in src_bits.
 *	INIT		Clear all bits except one specified bit, which is set.
 *	FFS		"Find First Set"; return the bit number of the first
 *			set bit, starting from 0; if none, return -1.
 *	FLS		"Find Last Set"; return the bit number of the highest
 *			set bit; if none, return -1.
 *	FFSCLR		"Find First Set and Clear"; return the bit number
 *			of the first set bit, starting from 0, after clearing
 *			that bit; if no bits were set, return -1.
 *	FLSCLR		"Find Last Set and Clear"; return the bit number
 *			of the highest set bit, starting from 0, after clearing
 *			that bit; if no bits were set, return -1.
 *	FFC		"Find First Clear"; return the bit number of the first
 *			clear bit, starting from 0; if none, return -1.
 *	FLC		"Find Last Clear"; return the bit number of the highest
 *			clear bit; if none, return -1.
 *	FFCSET		"Find First Clear and Set"; return the bit number
 *			of the first clear bit, starting from 0, after
 *			setting that bit; if no bits were clear, return -1.
 *	FLCSET		"Find Last Clear and Set"; return the bit number
 *			of the highest clear bit, starting from 0, after
 *			setting that bit; if no bits were clear, return -1.
 *
 * The following operations are only available in BITMASKN_XXX form:
 *
 *	ALLOCRANGE	"Allocate a Range"; find the first run of "nbits"
 *			consecutive clear bits, set them all, and return
 *			the bit number of the first one; if no such run is
 *			found, return -1.
 *
 *	FREERANGE	"Free a Range"; clear "nbits" bits starting from
 *			a given bit number.
 *
 * Also provided is the symbol NBITPW, which gives the number of bits in
 * a word, and may be used by the caller to select between BITMASK1_XXX,
 * BITMASK2_XXX and BITMASKN_XXX.
 *
 * BITMASK_NWORDS(nbits) gives the number of words needed to hold nbits bits.
 *
 * For all of the following, the "bits" arguments are of type (uint_t *),
 * and "n" is the number of (uint_t) words and is of type (uint_t).
 * "bitno" arguments and return values are of type (int); "bitno" arguments
 * should never be negative, though.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */


#if defined(_KERNEL) || defined(_KMEMUSER)

#define NBITPW	32

#define BITMASK_NWORDS(nbits)	(((nbits) + NBITPW - 1) / NBITPW)

#endif /* _KERNEL || _KMEMUSER */


#if defined(_KERNEL)

#define BITMASK1_CLRALL(bits) \
		((bits)[0] = 0)
#define BITMASK2_CLRALL(bits) \
		((bits)[0] = (bits)[1] = 0)

#if defined(__USLC__)
asm void
BITMASKN_CLRALL(uint_t bits[], uint_t n)
{
%treg bits; mem n;
	pushl	%edi
	pushl	bits
	movl	n, %ecx
	popl	%edi
	xorl	%eax, %eax
	rep; stosl
	popl	%edi
%mem bits, n;
	pushl	%edi
	movl	n, %ecx
	movl	bits, %edi
	xorl	%eax, %eax
	rep; stosl
	popl	%edi
}
#pragma asm partial_optimization BITMASKN_CLRALL
#endif /* defined(__USLC__) */

#define BITMASK1_SETALL(bits) \
		((bits)[0] = ~0U)
#define BITMASK2_SETALL(bits) \
		((bits)[0] = (bits)[1] = ~0U)
#if defined(__USLC__)
asm void
BITMASKN_SETALL(uint_t bits[], uint_t n)
{
%treg bits; mem n;
	pushl	%edi
	pushl	bits
	movl	n, %ecx
	popl	%edi
	movl	$-1, %eax
	rep; stosl
	popl	%edi
%mem bits, n;
	pushl	%edi
	movl	n, %ecx
	movl	bits, %edi
	movl	$-1, %eax
	rep; stosl
	popl	%edi
}
#pragma asm partial_optimization BITMASKN_SETALL
#endif /* defined(__USLC__) */

#define BITMASK1_TESTALL(bits) \
		((bits)[0])
#define BITMASK2_TESTALL(bits) \
		((bits)[0] | (bits)[1])
#if defined(__USLC__)
asm int
BITMASKN_TESTALL(const uint_t bits[], uint_t n)
{
%treg bits; mem n; lab l;
	pushl	%esi
	pushl	bits
	movl	n, %ecx
	popl	%esi
l:
	lodsl
	orl	%eax, %eax
	loopz	l
	popl	%esi
%mem bits, n; lab l;
	pushl	%esi
	movl	n, %ecx
	movl	bits, %esi
l:
	lodsl
	orl	%eax, %eax
	loopz	l
	popl	%esi
}
#pragma asm partial_optimization BITMASKN_TESTALL
#endif /* defined(__USLC__) */

#define BITMASK1_CLR1(bits, bitno) \
		((bits)[0] &= ~(1U << (bitno)))
#define BITMASK2_CLR1(bits, bitno) \
		BITMASKN_CLR1(bits, bitno)
#if defined(__USLC__)
asm void
BITMASKN_CLR1(uint_t bits[], int bitno)
{
%reg bits, bitno;
	btrl	bitno, (bits)
%mem bits; ureg bitno;
	movl	bits, %edx
	btrl	bitno, (%edx)
%ureg bits; mem bitno;
	movl	bitno, %eax
	btrl	%eax, (bits)
%mem bits, bitno;
	pushl	bits
	movl	bitno, %eax
	popl	%edx
	btrl	%eax, (%edx)
}
#pragma asm partial_optimization BITMASKN_CLR1
#endif /* defined(__USLC__) */

#define BITMASK1_SET1(bits, bitno) \
		((bits)[0] |= (1U << (bitno)))
#define BITMASK2_SET1(bits, bitno) \
		BITMASKN_SET1(bits, bitno)
#if defined(__USLC__)
asm void
BITMASKN_SET1(uint_t bits[], int bitno)
{
%reg bits, bitno;
	btsl	bitno, (bits)
%mem bits; ureg bitno;
	movl	bits, %edx
	btsl	bitno, (%edx)
%ureg bits; mem bitno;
	movl	bitno, %eax
	btsl	%eax, (bits)
%mem bits, bitno;
	pushl	bits
	movl	bitno, %eax
	popl	%edx
	btsl	%eax, (%edx)
}
#pragma asm partial_optimization BITMASKN_SET1
#endif /* defined(__USLC__) */

#define BITMASK1_TEST1(bits, bitno) \
		((bits)[0] & (1U << (bitno)))
#define BITMASK2_TEST1(bits, bitno) \
		BITMASKN_TEST1(bits, bitno)
#if defined(__USLC__)
asm int
BITMASKN_TEST1(const uint_t bits[], int bitno)
{
%reg bits, bitno;
	btl	bitno, (bits)
	sbbl	%eax, %eax
%mem bits; ureg bitno;
	movl	bits, %edx
	btl	bitno, (%edx)
	sbbl	%eax, %eax
%ureg bits; mem bitno;
	movl	bitno, %eax
	btl	%eax, (bits)
	sbbl	%eax, %eax
%mem bits, bitno;
	pushl	bits
	movl	bitno, %eax
	popl	%edx
	btl	%eax, (%edx)
	sbbl	%eax, %eax
}
#pragma asm partial_optimization BITMASKN_TEST1
#endif /* defined(__USLC__) */

#define BITMASK1_CLRN(dst_bits, src_bits) \
		((dst_bits)[0] &= ~(src_bits)[0])
#define BITMASK2_CLRN(dst_bits, src_bits) \
		(((dst_bits)[0] &= ~(src_bits)[0]), \
		 ((dst_bits)[1] &= ~(src_bits)[1]))
#if defined(__USLC__)
asm void
BITMASKN_CLRN(uint_t dst_bits[], const uint_t src_bits[], uint_t n)
{
%mem dst_bits, src_bits, n; lab l;
	pushl	%edi
	pushl	%esi
	pushl	dst_bits
	pushl	src_bits
	movl	n, %ecx
	popl	%esi
	popl	%edi
l:
	lodsl
	notl	%eax
	andl	(%edi), %eax
	stosl
	loop	l
	popl	%esi
	popl	%edi
}
#pragma asm partial_optimization BITMASKN_CLRN
#endif /* defined(__USLC__) */

#define BITMASK1_ANDN(dst_bits, src_bits) \
		((dst_bits)[0] &= (src_bits)[0])
#define BITMASK2_ANDN(dst_bits, src_bits) \
		(((dst_bits)[0] &= (src_bits)[0]), \
		 ((dst_bits)[1] &= (src_bits)[1]))
#if defined(__USLC__)
asm void
BITMASKN_ANDN(uint_t dst_bits[], const uint_t src_bits[], uint_t n)
{
%mem dst_bits, src_bits, n; lab l;
	pushl	%edi
	pushl	%esi
	pushl	dst_bits
	pushl	src_bits
	movl	n, %ecx
	popl	%esi
	popl	%edi
l:
	lodsl
	andl	(%edi), %eax
	stosl
	loop	l
	popl	%esi
	popl	%edi
}
#pragma asm partial_optimization BITMASKN_ANDN
#endif /* defined(__USLC__) */

#define BITMASK1_SETN(dst_bits, src_bits) \
		((dst_bits)[0] |= (src_bits)[0])
#define BITMASK2_SETN(dst_bits, src_bits) \
		(((dst_bits)[0] |= (src_bits)[0]), \
		 ((dst_bits)[1] |= (src_bits)[1]))
#if defined(__USLC__)
asm void
BITMASKN_SETN(uint_t dst_bits[], const uint_t src_bits[], uint_t n)
{
%mem dst_bits, src_bits, n; lab l;
	pushl	%edi
	pushl	%esi
	pushl	dst_bits
	pushl	src_bits
	movl	n, %ecx
	popl	%esi
	popl	%edi
l:
	lodsl
	orl	(%edi), %eax
	stosl
	loop	l
	popl	%esi
	popl	%edi
}
#pragma asm partial_optimization BITMASKN_SETN
#endif /* defined(__USLC__) */

#define BITMASK1_TESTN(dst_bits, src_bits) \
		((dst_bits)[0] & (src_bits)[0])
#define BITMASK2_TESTN(dst_bits, src_bits) \
		(((dst_bits)[0] & (src_bits)[0]) || \
		 ((dst_bits)[1] & (src_bits)[1]))
#if defined(__USLC__)
asm int
BITMASKN_TESTN(const uint_t dst_bits[], const uint_t src_bits[], uint_t n)
{
%mem dst_bits, src_bits, n; lab l;
	pushl	%edi
	pushl	%esi
	pushl	dst_bits
	pushl	src_bits
	movl	n, %ecx
	popl	%esi
	popl	%edi
	leal	-4(%edx), %edx
l:
	lodsl
	leal	4(%edx), %edx
	andl	(%edx), %eax
	loopz	l
	popl	%esi
	popl	%edi
}
#pragma asm partial_optimization BITMASKN_TESTN
#endif /* defined(__USLC__) */

#define BITMASK1_INIT(bits, bitno) \
		((bits)[0] = (1U << (bitno)))
#define BITMASK2_INIT(bits, bitno) \
		(BITMASK2_CLRALL(bits, 2), \
		 BITMASK2_SET1(bits, bitno))
#define BITMASKN_INIT(bits, n, bitno) \
		(BITMASKN_CLRALL(bits, n), \
		 BITMASKN_SET1(bits, bitno))

extern int BITMASK1_FFS(const uint_t bits[]);
#define BITMASK2_FFS(bits) \
		BITMASKN_FFS(bits, 2)
extern int BITMASKN_FFS(const uint_t bits[], uint_t n);

extern int BITMASK1_FLS(const uint_t bits[]);
#define BITMASK2_FLS(bits) \
		BITMASKN_FLS(bits, 2)
extern int BITMASKN_FLS(const uint_t bits[], uint_t n);

extern int BITMASK1_FFSCLR(uint_t bits[]);
#define BITMASK2_FFSCLR(bits) \
		BITMASKN_FFSCLR(bits, 2)
extern int BITMASKN_FFSCLR(uint_t bits[], uint_t n);

extern int BITMASK1_FLSCLR(uint_t bits[]);
#define BITMASK2_FLSCLR(bits) \
		BITMASKN_FLSCLR(bits, 2)
extern int BITMASKN_FLSCLR(uint_t bits[], uint_t n);

extern int BITMASK1_FFC(const uint_t bits[]);
#define BITMASK2_FFC(bits) \
		BITMASKN_FFC(bits, 2)
extern int BITMASKN_FFC(const uint_t bits[], uint_t n);

extern int BITMASK1_FLC(const uint_t bits[]);
#define BITMASK2_FLC(bits) \
		BITMASKN_FLC(bits, 2)
extern int BITMASKN_FLC(const uint_t bits[], uint_t n);

extern int BITMASK1_FFCSET(uint_t bits[]);
#define BITMASK2_FFCSET(bits) \
		BITMASKN_FFCSET(bits, 2)
extern int BITMASKN_FFCSET(uint_t bits[], uint_t n);

extern int BITMASK1_FLCSET(uint_t bits[]);
#define BITMASK2_FLCSET(bits) \
		BITMASKN_FLCSET(bits, 2)
extern int BITMASKN_FLCSET(uint_t bits[], uint_t n);

extern int BITMASKN_ALLOCRANGE(uint_t bits[], uint_t totalbits, uint_t nbits);

extern void BITMASKN_FREERANGE(uint_t bits[], int bitno, uint_t nbits);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_BITMASKS_H */
