#ifndef _STDLOCK_H
#define _STDLOCK_H
#ident	"@(#)libc-i386:inc/stdlock.h	1.7"
/*
* stdlock.h - locking internal to libc, x86 version.
*/

#include <sys/types.h>	/* for id_t */

typedef struct
{
	int	lock[2];	/* [0] is lock & wanted; [1] is an address */
} StdLock;

#ifdef _REENTRANT

#ifdef __STDC__
	extern void (*_libc_block)(int *);
	extern void (*_libc_unblock)(int *);
	extern id_t (*_libc_self)(void);
#else
	extern void (*_libc_block)();
	extern void (*_libc_unblock)();
	extern id_t (*_libc_self)();
#endif

extern int __multithreaded;

#if defined(i386) && defined(__USLC__)

#define STDLOCK(p)	(__multithreaded && (__stdlock(p), 0))
#define STDUNLOCK(p)	__stdunlock(p)
#define STDTRYLOCK(p)	__stdtrylock(p)
#define STDTRYUNLOCK(p)	__stdtryunlock(p)

asm void
__stdlock(StdLock *p)
{
%reg p; lab done;
	lock; btsl $0, (p)
	jnc	done
	pushl	p
#ifdef PIC
	movl	_libc_block@GOT(%ebx), %eax
	call	*(%eax)
#else
	call	*_libc_block
#endif
	popl	%eax
done:
%mem p; lab done;
	movl	p, %eax
	lock; btsl $0, (%eax)
	jnc	done
	pushl	%eax
#ifdef PIC
	movl	_libc_block@GOT(%ebx), %eax
	call	*(%eax)
#else
	call	*_libc_block
#endif
	popl	%eax
done:
%error;
}

asm void
__stdunlock(StdLock *p)
{
%reg p; lab done;
	andb	$0, (p)
	cmpw	$0, 2(p)
	jz	done
	pushl	p
#ifdef PIC
	movl	_libc_unblock@GOT(%ebx), %eax
	call	*(%eax)
#else
	call	*_libc_unblock
#endif
	popl	%eax
done:
%mem p; lab done;
	movl	p, %eax
	andb	$0, (%eax)
	cmpw	$0, 2(%eax)
	jz	done
	pushl	%eax
#ifdef PIC
	movl	_libc_unblock@GOT(%ebx), %eax
	call	*(%eax)
#else
	call	*_libc_unblock
#endif
	popl	%eax
done:
%error;
}

asm int
__stdtrylock(StdLock *p)
{
%reg p;
	movl	$1, %eax
	xchgb	%al, (p)
%mem p;
	movl	p, %ecx
	movl	$1, %eax
	xchgb	%al, (%ecx)
%error;
}

asm void
__stdtryunlock(StdLock *p)
{
%reg p;
	andb	$0, (p)
%mem p;
	movl	p, %eax
	andb	$0, (%eax)
%error;
}

	#pragma asm partial_optimization __stdlock
	#pragma asm partial_optimization __stdunlock
	#pragma asm partial_optimization __stdtrylock
	#pragma asm partial_optimization __stdtryunlock

#else /*!(defined(i386) && defined(__USLC__))*/

#define STDLOCK(p)	_stdlock(p)
#define STDUNLOCK(p)	_stdunlock(p)
#define STDTRYLOCK(p)	_stdtrylock(p)
#define STDTRYUNLOCK(p)	_stdtryunlock(p)

#ifdef __STDC__
	extern void _stdlock(StdLock *);
	extern void _stdunlock(StdLock *);
	extern int _stdtrylock(StdLock *);
	extern void _stdtryunlock(StdLock *);
#else
	extern void _stdlock();
	extern void _stdunlock();
	extern int _stdtrylock();
	extern void _stdtryunlock();
#endif

#endif /*defined(i386) && defined(__USLC__)*/

#else /*!_REENTRANT*/

#define STDLOCK(p)
#define STDUNLOCK(p)
#define STDTRYLOCK(p)	0	/* success */
#define STDTRYUNLOCK(p)

#endif /*_REENTRANT*/

#endif /*_STDLOCK_H*/
