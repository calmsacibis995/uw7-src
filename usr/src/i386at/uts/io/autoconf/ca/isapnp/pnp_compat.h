/*
 *	@(#)pnp_compat.h	7.1	10/22/97	12:29:05
 */
#ifndef __PNP_COMPAT_H
#define __PNP_COMPAT_H

/* Unixware/OpenServer compatability stuff: */
#ifdef UNIXWARE

#	define ARCH		cm_bustypes()
#       define PHYSMAP(A, L)    physmap(A, L, KM_NOSLEEP)
#       define PHYSFREE(A, L)   physmap_free(A, L, 0)
#	define SUSPEND(X)	drv_usecwait(X)
#	define ENTRY_RETVAL	int
#	define ENTRY_NOERR	return(0)
#	define ENTRY_ERR(X)	return(X)
#	define MINOR(X)		getminor(*(X))
#	define ktop(X)		vtop(X, NULL)
#	include "sys/ksynch.h"
#	define PNP_MUTEXLOCK_T	lock_t
#	define PNP_LOCK(s)	LOCK(&(s), plstr)
#	define PNP_UNLOCK(s)	UNLOCK(&(s), plstr)

#	include <sys/xdebug.h> 
#	define BUGGER		(*cdebugger) (DR_OTHER, NO_FRAME)


/* Symbols that are in OSR5, are not in Unixware, and that we can fake */
#	ifndef KM_NO_DMA
#		define KM_NO_DMA	(0)
#	endif
#	ifndef EISA
#		define AT	CM_BUS_ISA
#		define EISA	CM_BUS_EISA
#	endif

/*
** Simulate putchar() using cmn_err().
** Show off my ability to write obfuscated C code in the process.
**
** Keep a buffer of all characters.
** Write it out using cmn_err() when we get a newline.
**
** variables:
**	_pcb		-- buffer in which we collect characters
**	_pcc		-- character we are currently writing
**	_pcp		-- PutChar Pointer -- Ptr to next char in buffer
**
** If _pcb isn't big enough, we kernel panic.  Oh well.
** We use _pcc so we only evaluate C once.
** We don't use _pcp because it's a dangerous narcotic.
*/
#	define PUTCHAR_INIT(SZ)	char _pcb[SZ], _pcc, *_pcp=_pcb
#	define putchar(C)	(((_pcc=(C))=='\n')\
		? (*_pcp=0),cmn_err(CE_CONT,"%s",_pcb),(_pcp=_pcb),_pcc\
		: (*_pcp=_pcc),++_pcp)

#else /* OpenServer */

#	define ARCH		(arch)
#       define PHYSMAP(A, L)    sptalloc(btoc(L), PG_P, btoc(A), NOSLEEP)
#       define PHYSFREE(A, L)   sptfree(A, btoc(L), 0)
#	define SUSPEND(X)	suspend(X)
#	define ENTRY_RETVAL	void
#	define ENTRY_NOERR	return
#	define ENTRY_ERR(X)	seterror(X)
#	define MINOR(X)		minor(X)
#	define PNP_MUTEXLOCK_T	struct lockb
#	define PNP_LOCK(s)	(s) = lockb5(&PnP_devices_lock)
#	define PNP_UNLOCK(s)	unlockb(&PnP_devices_lock, (s))
#	define PUTCHAR_INIT(SZ)	(void)0

#endif

#endif	/* incl. protect */
