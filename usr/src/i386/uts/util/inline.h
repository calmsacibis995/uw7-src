#ifndef _UTIL_INLINE_H	/* wrapper symbol for kernel use */
#define _UTIL_INLINE_H	/* subject to change without notice */

#ident	"@(#)kern-i386:util/inline.h	1.23.3.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Asm function definitions for functions
 * that are expanded inline at compile time by the compiler.
 * Used mostly for performance improvements and access to machine instructions
 * which are not available through C language statements.
 *
 * Most of these functions also exist in the form of ordinary functions
 * defined somewhere in the kernel, since not all files are
 * or can be compiled with inline.h.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */
#include <util/dl.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */
#include <sys/dl.h>	/* REQUIRED */

#else

#include <sys/dl.h>	/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

asm void
_wcr0(int val)
{
%reg val;
	movl	val, %cr0
%mem val;
	movl	val, %eax
	movl	%eax, %cr0
}
#pragma asm partial_optimization _wcr0

asm void
_wcr3(int val)
{
%reg val;
	movl	val, %cr3
%mem val;
	movl	val, %eax
	movl	%eax, %cr3
}
#pragma asm partial_optimization _wcr3

asm void
_wcr4(int val)
{
%reg val;
	movl	val, %cr4
%mem val;
	movl	val, %eax
	movl	%eax, %cr4
}
#pragma asm partial_optimization _wcr4

asm int
_cr0(void)
{
	movl	%cr0, %eax
}
#pragma asm partial_optimization _cr0

asm int
_cr2(void)
{
	movl	%cr2, %eax
}
#pragma asm partial_optimization _cr2

asm int
_cr3(void)
{
	movl	%cr3, %eax
}
#pragma asm partial_optimization _cr3

asm int
_cr4(void)
{
	movl	%cr4, %eax
}
#pragma asm partial_optimization _cr4

asm void
_wdr0(int val)
{
%reg val;
	movl	val, %db0
%mem val;
	movl	val, %eax
	movl	%eax, %db0
}
#pragma asm partial_optimization _wdr0

asm void
_wdr1(int val)
{
%reg val;
	movl	val, %db1
%mem val;
	movl	val, %eax
	movl	%eax, %db1
}
#pragma asm partial_optimization _wdr1

asm void
_wdr2(int val)
{
%reg val;
	movl	val, %db2
%mem val;
	movl	val, %eax
	movl	%eax, %db2
}
#pragma asm partial_optimization _wdr2

asm void
_wdr3(int val)
{
%reg val;
	movl	val, %db3
%mem val;
	movl	val, %eax
	movl	%eax, %db3
}
#pragma asm partial_optimization _wdr3

asm void
_wdr6(int val)
{
%reg val;
	movl	val, %db6
%mem val;
	movl	val, %eax
	movl	%eax, %db6
}
#pragma asm partial_optimization _wdr6

asm void
_wdr7(int val)
{
%reg val;
	movl	val, %db7
%mem val;
	movl	val, %eax
	movl	%eax, %db7
}
#pragma asm partial_optimization _wdr7

asm int
_dr0(void)
{
	movl	%db0, %eax
}
#pragma asm partial_optimization _dr0

asm int
_dr1(void)
{
	movl	%db1, %eax
}
#pragma asm partial_optimization _dr1

asm int
_dr2(void)
{
	movl	%db2, %eax
}
#pragma asm partial_optimization _dr2

asm int
_dr3(void)
{
	movl	%db3, %eax
}
#pragma asm partial_optimization _dr3

asm int
_dr6(void)
{
	movl	%db6, %eax
}
#pragma asm partial_optimization _dr6

asm int
_dr7(void)
{
	movl	%db7, %eax
}
#pragma asm partial_optimization _dr7

asm int
_fs(void)
{
	movw	%fs, %ax
}
#pragma asm partial_optimization _fs

asm int
_gs(void)
{
	movw	%gs, %ax
}
#pragma asm partial_optimization _gs

asm void
_wfs(int val)
{
%mem val;
	movl	val, %eax
	movw	%ax, %fs
}
#pragma asm partial_optimization _wfs

asm void
_wgs(int val)
{
%mem val;
	movl	val, %eax
	movw	%ax, %gs
}
#pragma asm partial_optimization _wgs

asm ushort
get_tr(void)
{
	xorl	%eax, %eax
	str	%ax
}
#pragma asm partial_optimization get_tr

asm void
loadtr(ushort val)
{
%mem val;
	movw	val, %ax
	ltr	%ax
}
#pragma asm partial_optimization loadtr

struct desctab;
asm void
loadgdt(struct desctab *descp)
{
%reg descp;
	lgdt	(descp)
%mem descp;
	movl	descp, %eax
	lgdt	(%eax)
}
#pragma asm partial_optimization loadgdt

asm void
loadidt(struct desctab *descp)
{
%reg descp;
	lidt	(descp)
%mem descp;
	movl	descp, %eax
	lidt	(%eax)
}
#pragma asm partial_optimization loadidt

asm void
loadldt(ushort val)
{
%mem val;
	movw	val, %ax
	lldt	%ax
}
#pragma asm partial_optimization loadldt


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
INVAL_ONCHIP_CACHE(void)
{
	wbinvd		/* write-back and invalidate onchip cache */
}
#pragma asm partial_optimization INVAL_ONCHIP_CACHE

#define READ_MSW	_cr0
#define WRITE_MSW	_wcr0
#define READ_PTROOT	_cr3
#define WRITE_PTROOT	_wcr3

asm void
outl(int port, ulong_t val)
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
outw(int port, ushort_t val)
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
outb(int port, uchar_t val)
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

asm ulong_t
inl(int port)
{
%mem port;
	movl	port, %edx
	inl	(%dx)
}
#pragma asm partial_optimization inl

asm ushort_t
inw(int port)
{
%mem port;
	movl	port, %edx
	xorl	%eax, %eax
	data16
	inl	(%dx)
}
#pragma asm partial_optimization inw

asm uchar_t
inb(int port)
{
%mem port;
	movl	port, %edx
	xorl	%eax, %eax
	inb	(%dx)
}
#pragma asm partial_optimization inb


/*
 * void
 * ldladd(dst, src)
 *	adds a ulong to a double long (dl_t).
 *		*dst (64-bit) += src (32-bit)
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
ldladd(dl_t *dst, ulong src)	/* *dst (64-bit) += src (32-bit) */
{
%reg dst, src;
	addl	src, (dst)	// add low half
	adcl	$0, 4(dst)	// add carry into high half
%ureg dst; mem src;
	movl	src, %ecx
	addl	%ecx, (dst)	// add low half
	adcl	$0, 4(dst)	// add carry into high half
%mem dst; ureg src;
	movl	dst, %ecx
	addl	src, (%ecx)	// add low half
	adcl	$0, 4(%ecx)	// add carry into high half
%treg dst, mem src;
	movl	dst, %ecx
	movl	src, %eax
	addl	%eax, (%ecx)	// add low half
	adcl	$0, 4(%ecx)	// add carry into high half
%mem dst, src;
	movl	src, %eax
	movl	dst, %ecx
	addl	%eax, (%ecx)	// add low half
	adcl	$0, 4(%ecx)	// add carry into high half
}
#pragma asm partial_optimization ldladd

/*
 * uint_t atomic_fnc(volatile uint_t *dst)
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
asm uint_t
atomic_fnc(volatile uint_t *dst)
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
 * uint_t atomic_fnc_bit(volatile uint_t *dst, int bitno)
 *	Atomically clear the bit indicated by bitno in the 32 bit
 *	word pointed to by dst and return the previous value in the bit.
 *
 * Calling/Exit State:
 *	None.
 */
asm uint_t
atomic_fnc_bit(volatile uint_t *dst, int bitno)
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
 * atomic_and(volatile uint_t *dst, uint_t src)
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
atomic_and(volatile uint_t *dst, uint_t src)
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
 * atomic_or(volatile uint_t *dst, uint_t src)
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
atomic_or(volatile uint_t *dst, uint_t src)
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

#endif /* _UTIL_INLINE_H */
