#ifndef _LWPSYNCH_H
#define _LWPSYNCH_H
#ident	"@(#)sgs-head:i386/head/lwpsynch.h	1.6"
/*
 * LWP synchronization
 */

#include <sys/time.h>
#include <machlock.h>
#include <sys/usync.h>

typedef volatile struct __lwp_mutex {
	char		wanted;
	_simplelock_t	lock;
#ifdef FATMUTEX
	long		fill[2];
#endif
} lwp_mutex_t;

typedef volatile struct __lwp_cond {
	char		wanted;
#ifdef FATMUTEX
	long		fill[2];
#endif
} lwp_cond_t;

#ifdef _LIBTHREAD_H
/*
 * LWP_MUTEX_TRYLOCK() returns zero on success.
 * LWP_MUTEX_ISLOCKED() returns non-zero if the mutex islocked.
 */
#define LWP_MUTEX_TRYLOCK(/* lwp_mutex_t * */ mp) (!_lock_try(&(mp)->lock))
#define LWP_MUTEX_ISLOCKED(/* lwp_mutex_t * */ mp) ((mp)->lock)
#endif /*_LIBTHREAD_H*/

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__USLC__) && (defined(_LIBLWPSYNCH_H) || defined(_LIBTHREAD_H))
/*
 * _lock_and_flag_clear() clears the first word of memory in an lwp_mutex_t
 * and returns the value of the wanted flag.  This is used by _lwp_mutex_unlock
 * to determine if LWPs are waiting on the lock while avoiding access to the
 * lock structure after clearing the lock to avoid the danger that the lock
 * might be unmapped after the lock is cleared.
 */
asm int
_lock_and_flag_clear(lwp_mutex_t *lmp)
{
%reg lmp;
	xorl	%eax, %eax	/ value to put into lock
	xchgw	(lmp), %ax	/ swap 0 with lmp->wanted, lmp->lock
	movb	$0, %ah		/ zero out lmp->lock portion of return
%mem lmp;
	movl	lmp, %ecx	/ ecx = lock address
	xorl	%eax, %eax	/ value to put into lock
	xchgw	(%ecx), %ax	/ swap 0 with lmp->wanted, lmp->lock
	movb	$0, %ah		/ zero out lmp->lock portion of return
}
#pragma asm partial_optimization _lock_and_flag_clear
#endif /*__USLC__ && (_LIBLWPSYNCH_H || _LIBTHREAD_H)*/

int	_lwp_mutex_trylock(lwp_mutex_t *);
int	_lwp_mutex_lock(lwp_mutex_t *);
int	_lwp_mutex_unlock(lwp_mutex_t *);
int	_lwp_cond_signal(lwp_cond_t *);
int	_lwp_cond_broadcast(lwp_cond_t *);
int	_lwp_cond_wait(lwp_cond_t *, lwp_mutex_t *);
int	_lwp_cond_timedwait(lwp_cond_t *, lwp_mutex_t *, const timestruc_t *);
int	_lwp_sema_init(_lwp_sema_t *, int);
int	_lwp_sema_post(_lwp_sema_t *);
int	_lwp_sema_trywait(_lwp_sema_t *);
int	_lwp_sema_wait(_lwp_sema_t *);

#ifdef __cplusplus
}
#endif

#endif /*_LWPSYNCH_H*/
