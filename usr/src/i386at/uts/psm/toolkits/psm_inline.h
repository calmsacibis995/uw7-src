#ifndef _TOOLKITS_INLINE_H	/* wrapper symbol for kernel use */
#define _TOOLKITS_INLINE_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:psm/toolkits/psm_inline.h	1.1.2.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_PSM)
#if _PSM != 2
#error "Unsupported PSM version"
#endif
#else
#error "Header file not valid for Core OS"
#endif

#ifndef __PSM_SYMREF

struct __PSM_dummy_st {
	int *__dummy1__;
	const struct __PSM_dummy_st *__dummy2__;
};
#define __PSM_SYMREF(symbol) \
	extern int symbol; \
	static const struct __PSM_dummy_st __dummy_PSM = \
		{ &symbol, &__dummy_PSM }

#if !defined(_PSM)
__PSM_SYMREF(No_PSM_version_defined);
#pragma weak No_PSM_version_defined
#else
#define __PSM_ver(x) _PSM_##x
#define _PSM_ver(x) __PSM_ver(x)
__PSM_SYMREF(_PSM_ver(_PSM));
#endif

#endif  /* __PSM_SYMREF */

/*
 * Asm function definitions for functions
 * that are expanded inline at compile time by the compiler.
 * Used mostly for performance improvements and access to machine instructions
 * which are not available through C language statements.
 */

asm int
intr_disable()
{
	pushfl
	cli
	popl	%eax
}
/* #pragma asm partial_optimization intr_disable */

asm void
intr_restore(int efl)
{
%mem	efl;
	movl	efl, %eax
	pushl	%eax
	popfl
}
/* #pragma asm partial_optimization intr_restore */


asm int
is_intr_enabled()
{
	pushfl
	popl	%eax
	sarl	$9,%eax
	andl	$1,%eax
}
/* #pragma asm partial_optimization intr_restore */


asm void
outl(int port, unsigned long val)
{
%treg port, val;
	pushl	port
	movl	val, %eax
	popl	%edx
	outl	(%dx)
%treg port; mem val;
	movl	port, %edx
	movl	val, %eax
	outl	(%dx)
%mem port, val;
	movl	val, %eax
	movl	port, %edx
	outl	(%dx)
}
#pragma asm partial_optimization outl

asm void
outw(int port, unsigned short val)
{
%treg port, val;
	pushl	port
	movw	val, %ax
	popl	%edx
	data16
	outl	(%dx)
%treg port; mem val;
	movl	port, %edx
	movw	val, %ax
	data16
	outl	(%dx)
%mem port, val;
	movw	val, %ax
	movl	port, %edx
	data16
	outl	(%dx)
}
/* #pragma asm partial_optimization outw */

asm void
outb(int port, unsigned char val)
{
%treg port, val;
	pushl	port
	movb	val, %al
	popl	%edx
	outb	(%dx)
%treg port; mem val;
	movl	port, %edx
	movb	val, %al
	outb	(%dx)
%mem port, val;
	movb	val, %al
	movl	port, %edx
	outb	(%dx)
}
/* #pragma asm partial_optimization outb */

asm unsigned long
inl(int port)
{
%mem port;
	movl	port, %edx
	inl	(%dx)
}
#pragma asm partial_optimization inl

asm unsigned short
inw(int port)
{
%mem port;
	movl	port, %edx
	xorl	%eax, %eax
	data16
	inl	(%dx)
}
#pragma asm partial_optimization inw

asm unsigned char
inb(int port)
{
%mem port;
	movl	port, %edx
	xorl	%eax, %eax
	inb	(%dx)
}
#pragma asm partial_optimization inb


/*
 * uint_t atomic_fnc(volatile unsigned int *dst)
 *      Atomically zero the dst and obtain the previous value.
 *	(in other words atomic fetch and clear)
 *
 * Calling/Exit State:
 *      The caller holds whatever lock to stabilize dst.
 *
 * Remarks:
 *      The atomic "xchgl" instruction present in the Intel 80x86
 *      instruction set is used.
 */
asm unsigned int
atomic_fnc(volatile unsigned int *dst)
{
%ureg dst;
        xorl    %eax, %eax
        xchgl   %eax, (dst)
%mem dst;
        movl    dst, %ecx
        xorl    %eax, %eax
        xchgl   %eax, (%ecx)
}
#pragma asm partial_optimization atomic_fnc

/*
 * unsigned int atomic_fnc_bit(volatile unsigned int *dst, int bitno)
 *	Atomically clear the bit indicated by bitno in the 32 bit
 *	word pointed to by dst and return the previous value in the bit.
 *
 * Calling/Exit State:
 *	None.
 */
asm unsigned int
atomic_fnc_bit(volatile unsigned int *dst, int bitno)
{
%reg dst, bitno;
	lock
	btrl	bitno, (dst)
	setc	%al
	andl	$1, %eax	
%reg dst; con bitno
	lock
	btrl	bitno, (dst)
	setc	%al
	andl	$1, %eax
%mem dst; reg bitno
	movl	bitno, %ecx
	movl	dst, %edx
	lock
	btrl	%ecx, (%edx)
	setc	%al
	andl	$1, %eax
%mem dst; con bitno
	movl	dst, %edx
	lock
	btrl	bitno, (%edx)
	setc	%al
	andl	$1, %eax
%mem dst, bitno;
	pushl	dst
	movl	bitno, %ecx
	popl	%edx
	lock
	btrl	%ecx, (%edx)
	setc	%al
	andl	$1, %eax
}
#pragma asm partial_optimization atomic_fnc_bit

/*
 * void
 * atomic_and(volatile unsigned int *dst, unsigned int src)
 *	atomically AND in a long value into an unsigned integer
 *		*dst &= src
 *
 * Calling/Exit State:
 *	Returns: None.
 *
 * Description:
 *	Register Usage:
 *		%eax		Holds src value if src and dst are in mem
 *		%ecx		Holds src or dst if only one is in mem
 *
 */
asm void
atomic_and(volatile unsigned int *dst, unsigned int src)
{
%reg dst, src;
	lock
	andl	src, (dst)
%ureg dst; mem src;
	movl	src, %ecx
	lock
	andl	%ecx, (dst)
%mem dst; ureg src;
	movl	dst, %ecx
	lock
	andl	src, (%ecx)
%treg dst, mem src;
	movl	dst, %ecx
	movl	src, %eax
	lock
	andl	%eax, (%ecx)
%mem dst, src;
	movl	src, %eax
	movl	dst, %ecx
	lock
	andl	%eax, (%ecx)
}
#pragma asm partial_optimization atomic_and

/*
 * void
 * atomic_or(volatile unsigned int *dst, unsigned int src)
 *	atomically OR in a long value into an unsigned integer
 *		*dst |= src
 *
 * Calling/Exit State:
 *	Returns: None.
 *
 * Description:
 *	Register Usage:
 *		%eax		Holds src value if src and dst are in mem
 *		%ecx		Holds src or dst if only one is in mem
 *
 */
asm void
atomic_or(volatile unsigned int *dst, unsigned int src)
{
%reg dst, src;
	lock
	orl	src, (dst)
%ureg dst; mem src;
	movl	src, %ecx
	lock
	orl	%ecx, (dst)
%mem dst; ureg src;
	movl	dst, %ecx
	lock
	orl	src, (%ecx)
%treg dst, mem src;
	movl	dst, %ecx
	movl	src, %eax
	lock
	orl	%eax, (%ecx)
%mem dst, src;
	movl	src, %eax
	movl	dst, %ecx
	lock
	orl	%eax, (%ecx)
}
#pragma asm partial_optimization atomic_or

#if defined(__cplusplus)
	}
#endif

#endif /* _TOOLKITS_INLINE_H */
