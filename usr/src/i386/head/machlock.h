#ifndef _MACHLOCK_H
#define _MACHLOCK_H
#ident	"@(#)sgs-head:i386/head/machlock.h	1.2"

typedef volatile unsigned char _simplelock_t;

#if defined(__USLC__) && (defined(_LIBLWPSYNCH_H) || defined(_LIBTHREAD_H))

#define _SIMPLE_UNLOCKED	0
#define _SIMPLE_LOCKED		0xff

/*
 * _lock_try() tries to acquire a simple lock.
 * _lock_try() returns non-zero on success.
 */
asm int
_lock_try(_simplelock_t *lockp)
{
%reg lockp
	movl	$_SIMPLE_LOCKED, %eax	/ value to exchange
	xchgb	(lockp), %al		/ try for the lock atomically
	xorb	$_SIMPLE_LOCKED, %al	/ return non-zero if success

%mem lockp
	movl	lockp, %ecx
	movl	$_SIMPLE_LOCKED, %eax	/ value to exchange
	xchgb	(%ecx), %al		/ try for the lock atomically
	xorb	$_SIMPLE_LOCKED, %al	/ return non-zero if success
}
#pragma asm partial_optimization _lock_try

/* _lock_clear() unlocks a simple lock.
 */
asm void
_lock_clear(_simplelock_t *lockp)
{
%reg lockp
	movl	$_SIMPLE_UNLOCKED, %eax	/ value to exchange
	xchgb	(lockp), %al		/ clear the lock atomically

%mem lockp
	movl	lockp, %ecx
	movl	$_SIMPLE_UNLOCKED, %eax	/ value to exchange
	xchgb	(%ecx), %al		/ clear the lock atomically
}
#pragma asm partial_optimization _lock_clear

#endif /*__USLC__ && (_LIBLWPSYNCH_H || _LIBTHREAD_H)*/

#endif /*_MACHLOCK_H*/
