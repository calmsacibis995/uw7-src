#ifndef _STDLOCK_H
#define _STDLOCK_H
#ident	"@(#)libmalloc:i386/stdlock.h	1.2"
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

#if defined(i386) && defined(__USLC__)

#define STDLOCK(p)	__stdlock(p)
#define STDUNLOCK(p)	__stdunlock(p)
#define STDTRYLOCK(p)	__stdtrylock(p)

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
	lock; andb $0, (p)
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
	lock; andb $0, (%eax)
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

	#pragma asm partial_optimization __stdlock
	#pragma asm partial_optimization __stdunlock
	#pragma asm partial_optimization __stdtrylock

#else /*!(defined(i386) && defined(__USLC__))*/

#define STDLOCK(p)	_stdlock(p)
#define STDUNLOCK(p)	_stdunlock(p)
#define STDTRYLOCK(p)	_stdtrylock(p)

#ifdef __STDC__
	extern void _stdlock(StdLock *);
	extern void _stdunlock(StdLock *);
	extern int _stdtrylock(StdLock *);
#else
	extern void _stdlock();
	extern void _stdunlock();
	extern int _stdtrylock();
#endif

#endif /*defined(i386) && defined(__USLC__)*/

#else /*!_REENTRANT*/

#define STDLOCK(p)
#define STDUNLOCK(p)
#define STDTRYLOCK(p)	0	/* success */

#endif /*_REENTRANT*/

#endif /*_STDLOCK_H*/
