#ifndef _UTIL_KSINLINE_H	/* wrapper symbol for kernel use */
#define _UTIL_KSINLINE_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:util/ksinline.h	1.27.1.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * This file is a nested '#include' within the file ksynch.h.  It is
 *	expected that C source files will include ksynch.h and thus
 *	indirectly include this file, but that C source will not include
 *	this file directly.
 */

#ifdef _KERNEL_HEADERS

#include <svc/cpu.h>	/* REQUIRED */
#include <util/types.h>	/* REQUIRED */
#include <util/ipl.h>	/* PORTABILITY */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */
#include <sys/ipl.h>	/* PORTABILITY */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

/*
 * WARNING: ANY CHANGES TO THE VALUE OF THE  FOLLOWING SYMBOLS MUST BE
 * REFLECTED HERE. 
 */

#ifndef	_A_SP_LOCKED
#define _A_SP_LOCKED	1
#endif

#ifndef	_A_SP_UNLOCKED
#define _A_SP_UNLOCKED	0
#endif

#ifndef	_A_INVPL
#define _A_INVPL	-1
#endif

#if defined(__USLC__)

/*
 * If DEBUG or SPINDEBUG is defined, we want to use real function entry
 * points instead of these inline routines.  Only the functions have
 * DEBUG code.
 */
#if ! defined DEBUG && ! defined SPINDEBUG

/*
 * void
 * DISABLE(void)
 *	Disables interrupts at the chip.
 *
 * Calling/Exit State:
 *	Returns with interrupts disabled.
 *	Returns: None.
 *
 * Description:
 *	Register usage: no registers are directly used.
 *
 */
asm void
DISABLE(void)
{
	cli
}
#pragma asm partial_optimization DISABLE

/*
 * void
 * ENABLE(void)
 *	Enables interrupts at the chip.
 *
 * Calling/Exit State:
 *	Returns with interrupts enabled.
 *	Returns: None.
 *
 * Description:
 *	Register usage: no registers are directly used.
 *
 */
asm void
ENABLE(void)
{
        sti
}
#pragma asm partial_optimization ENABLE

/*
 * pl_t
 * __spl(pl_t newpl)
 *	Set the system priority level to newpl.
 *
 * Calling/Exit State:
 *	Returns the previous pl
 *
 * Remarks:
 *	The new pl must be greater than or equal to the old pl.
 */
asm pl_t
__spl(pl_t newpl)
{
%mem	newpl
	movl	newpl, %edx
	xorl	%eax, %eax
	movb	ipl, %al
	movb	%dl, ipl
}
#pragma asm partial_optimization __spl

/*
 * void
 * __splx(pl_t newpl)
 *	Set the system priority level to newpl.
 *
 * Calling/Exit State:
 *	None
 */
asm void
__splx(pl_t newpl)
{
%mem	newpl; lab done;
	movl	newpl, %eax
	movb	%al, ipl
	cmpl	%eax, picipl
	jbe	done
	call	splunwind
done:

}
#pragma asm partial_optimization __splx

#endif /* ! DEBUG && ! SPINDEBUG */

/*
 * void
 * atomic_int_add(volatile int *dst, int ival)
 *	Atomically add  ival to the integer at address dst. 
 *		*dst += ival
 *
 * Calling/Exit State: 
 *	None.
 *
 * Remarks:
 *	We assume that the caller guarantees the stability of 
 *	destination address dst.
 */
asm void
atomic_int_add(volatile int *dst, int ival)
{
%reg dst; con ival;
#ifndef	UNIPROC
	lock
#endif
	addl	ival, (dst)
/VOL_OPND 2
%mem dst; con ival;
	movl	dst, %ecx
#ifndef	UNIPROC
	lock
#endif
	addl	ival, (%ecx)
/VOL_OPND 2
%reg dst, ival;
#ifndef	UNIPROC
	lock
#endif
	addl 	ival, (dst)
/VOL_OPND 2
%ureg dst; mem ival;
	movl	ival, %eax
#ifndef	UNIPROC
	lock
#endif
	addl	%eax, (dst)
/VOL_OPND 2
%mem dst; ureg ival;
	movl	dst, %ecx
#ifndef	UNIPROC
	lock
#endif
	addl	ival, (%ecx)
/VOL_OPND 2
%treg dst; mem ival;
	movl	dst, %ecx
	movl	ival, %eax
#ifndef	UNIPROC
	lock
#endif
	addl	%eax, (%ecx)
/VOL_OPND 2
%mem dst, ival;
	movl	ival, %eax
	movl	dst, %ecx
#ifndef	UNIPROC
	lock
#endif
	addl 	%eax, (%ecx)
/VOL_OPND 2
}
#pragma asm partial_optimization atomic_int_add

/*
 * void
 * atomic_int_sub(volatile int *dst, int ival)
 *	Atomically subtract ival from the integer at address dst. 
 *		*dst -= ival
 *
 * Calling/Exit State: 
 *	None.
 *
 * Remarks:
 *	We assume that the caller guarantees the stability of 
 *	destination address dst.
 */
asm void
atomic_int_sub(volatile int *dst, int ival)
{
%reg dst; con ival;
#ifndef	UNIPROC
	lock
#endif
	subl	ival, (dst)
/VOL_OPND 2
%mem dst; con ival;
	movl	dst, %ecx
#ifndef	UNIPROC
	lock
#endif
	subl	ival, (%ecx)
/VOL_OPND 2
%reg dst, ival;
#ifndef	UNIPROC
	lock
#endif
	subl 	ival, (dst)
/VOL_OPND 2
%ureg dst; mem ival;
	movl	ival, %eax
#ifndef	UNIPROC
	lock
#endif
	subl	%eax, (dst)
/VOL_OPND 2
%mem dst; ureg ival;
	movl	dst, %ecx
#ifndef	UNIPROC
	lock
#endif
	subl	ival, (%ecx)
/VOL_OPND 2
%treg dst; mem ival;
	movl	dst, %ecx
	movl	ival, %eax
#ifndef	UNIPROC
	lock
#endif
	subl	%eax, (%ecx)
/VOL_OPND 2
%mem dst, ival;
	movl	ival, %eax
	movl	dst, %ecx
#ifndef	UNIPROC
	lock
#endif
	subl 	%eax, (%ecx)
/VOL_OPND 2
}
#pragma asm partial_optimization atomic_int_sub


/*
 * void
 * atomic_int_incr(volatile int *dst)
 *	Atomically increment the int value at address dst. 
 *
 * Calling/Exit State: 
 *	None.
 *
 * Remarks:
 *	We assume that the caller guarantees the stability of 
 *	destination address.
 */
asm void
atomic_int_incr(volatile int *dst)
{
%reg dst;
#ifndef	UNIPROC
	lock
#endif
	incl 	(dst)
/VOL_OPND 1
%mem dst;
	movl	dst, %ecx
#ifndef	UNIPROC
	lock
#endif
	incl 	(%ecx)
/VOL_OPND 1
}
#pragma asm partial_optimization atomic_int_incr

/*
 * boolean_t
 * atomic_int_decr(volatile int *dst)
 *	Atomically decrement the int value at address dst.
 *
 * Calling/Exit State: 
 *	Returns B_TRUE if (*dst) became zero.
 *
 * Remarks:
 *	We assume that the caller guarantees the stability of 
 *	destination address.
 */
asm boolean_t
atomic_int_decr(volatile int *dst)
{
%reg dst;
#ifndef	UNIPROC
	lock
#endif
	decl 	(dst)
/VOL_OPND 1
	setz	%al
	andl	$1, %eax
%mem dst;
	movl 	dst, %ecx
#ifndef	UNIPROC
	lock
#endif
	decl	(%ecx)
/VOL_OPND 1
	setz	%al
	andl	$1, %eax
}
#pragma asm partial_optimization atomic_int_decr


#ifndef UNIPROC
/*
 * void
 * __write_sync(void)
 *	Flush out the engine's write buffers.
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Generate an exchange instruction with the top of the stack.
 */
asm void
__write_sync(void)
{
%lab	do486, done;
	cmpl	$CPU_P5, mycpuid
	jl	do486
	xorl	%eax, %eax
	pushl	%ebx
	cpuid
	popl	%ebx
	jmp	done
do486:
	xchgl	%eax, -4(%esp)
done:
}
#pragma asm partial_optimization __write_sync

#endif /* !UNIPROC */

#endif /* defined(__USLC__) */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_KSINLINE_H */
