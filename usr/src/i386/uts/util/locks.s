	.ident	"@(#)kern-i386:util/locks.s	1.13.1.2"
	.file	"locks.s"

include(KBASE/svc/asm.m4)
include(assym_include)
include(KBASE/util/ksynch.m4)

/ 
/ pl_t
/ lock_nodbg(lock_t *lockp, pl_t ipl)
/ 	Acquire a lock in non-debug mode.
/
/ Calling/Exit State: 
/	lockp is the lock to be acquired.  ipl is the desired ipl, which
/	must be greater than or equal to the current ipl.  Upon return,
/	the lock is held at the given ipl.
/	Returns:  the previous ipl.
/
/ Description:
/	The ipl is raised and an atomic try is made for the lock.  If the
/	lock cannot be acquired, the ipl is lowered and we spin until the
/	lock becomes available.  At this point the process is repeated.
/
/	Register usage:
/		%ecx		pointer to lock
/		%eax		initially, the desired ipl, then
/				the old ipl
/		%edx		temporary
/
/ Remarks:
/	For UNIPROC, the lock argument is ignored; this routine just
/	disables preemption, raises the ipl, and returns the old ipl.
/	
ENTRY(lock_nodbg)
ifndef(`UNIPROC',`
	movl	SPARG0, %ecx		/ move &lock into known location
')
	movl	SPARG1, %eax		/ ipl into known register

ifndef(`UNIPROC',`
.lkloop:
')
	__DISABLE_PRMPT_ASM		/ Disable preemption
	__SPL_ASM			/ Raise pl to %eax, return old in %eax

ifndef(`UNIPROC',`
	movb	$_A_SP_LOCKED,%dl	/ value to exchange
	xchgb	%dl,_A_SP_LOCK(%ecx)	/ try for lock
	cmpb	$_A_SP_UNLOCKED,%dl	/ was previously unlocked?
	jne 	.lkfailed		/ if not, then go spin
')
	ret

ifndef(`UNIPROC',`
.lkfailed:				/ did not get lock on first try, so:
	__SPLX_ASM			/	(1) restore old pl
	__ENABLE_PRMPT_ASM		/	(2) Re-enable kernel preemption
	movl	SPARG0, %ecx		/	(3) Restore %ecx and %eax
	movl	SPARG1, %eax
.lkspin:				/	(4) then
	cmpb	$_A_SP_UNLOCKED,(%ecx)	/		spin until
	je	.lkloop			/		...lock is unlocked
	jmp 	.lkspin			/		spin while not clear
')
	SIZE(lock_nodbg)

/
/ pl_t
/ trylock_nodbg(lock_t *lockp, pl_t ipl)
/	Attempts to lock the given lock at the given ipl in non-debug mode.
/	If at first it does not succeed, gives up.
/
/ Calling/Exit State:
/	lockp is the lock to attempt to lock, ipl is the interrupt level
/	at which the acquisition should be attempted.  Returns the old
/	ipl if the lock is acquired, INVPL otherwise.
/
/ Description:
/	Register usage:
/		%ecx		address of lock
/		%eax		Initially, the desired ipl, then the
/				old ipl
/		%edx		temp
/
/ Remarks:
/	For UNIPROC, the lock argument is ignored; this routine just
/	disables preemption, raises the ipl, and returns the old ipl.
/
ENTRY(trylock_nodbg)
ifndef(`UNIPROC',`
	movl	SPARG0,%ecx		/ move &lock into known place
')
	movl	SPARG1,%eax		/ move ipl into known place

	__DISABLE_PRMPT_ASM		/ Disable kernel preemption
	__SPL_ASM			/ Raise pl to %eax, return old in %eax

ifndef(`UNIPROC',`
	movb	$_A_SP_LOCKED,%dl	/ value to exchange
	xchgb	%dl,_A_SP_LOCK(%ecx)	/ try for lock
	cmpb	$_A_SP_UNLOCKED,%dl	/ was previously unlocked?
	jne 	.tlfailed		/ no, failed
	ret

.tlfailed:				/ did not get the lock, so:
	__SPLX_ASM			/	(1) lower the ipl
	__ENABLE_PRMPT_ASM		/	(2) Re-enable kernel preemption
	movl	$_A_INVPL,%eax		/	(3) return failure
')
	ret
	SIZE(trylock_nodbg)

/
/ void
/ unlock_nodbg(lock_t *lockp, pl_t ipl)
/	Unlocks a lock in non-debug mode.
/
/ Calling/Exit State:
/	lockp is the lock to unlock, ipl is the ipl level to return at.
/	Returns:  None.
/
/ Description:
/	Register usage:
/		%ecx		pointer to the lock
/		%eax		new ipl
/		%edx		temp stuff
/
/ Remarks:
/	For UNIPROC, the lock argument is ignored; this routine just
/	lowers the ipl, and re-enables and checks for preemption.
/
ENTRY(unlock_nodbg)
ifndef(`UNIPROC',`
	movl	SPARG0,%ecx		/ &lock into known register
')
	movl	SPARG1,%eax		/ ipl into known register

ifndef(`UNIPROC',`
	movb	$_A_SP_UNLOCKED,%dl	/ value to exchange
	movb	%dl,(%ecx)		/ release the lock
')
	__SPLX_ASM			/ lower the ipl
	__ENABLE_PRMPT_ASM		/ Re-enable kernel preemption
	ret				/ return
	SIZE(unlock_nodbg)

/ 
/ pl_t
/ lock_dbg(lock_t *lockp, pl_t ipl)
/ 	Acquire a lock in debug mode.
/
/ Calling/Exit State:
/	lockp is the lock to be acquired.  ipl is the desired ipl, which
/	must be greater than or equal to the current ipl.  Upon return,
/	the lock is held at the given ipl.
/	Returns:  the previous ipl.
/
/ Description:
/	For UNIPROC, just call lock_nodbg.
/
/	For non-UNIPROC, push the arguments, and call lock_dbgC.
/	This is done so that the return address of lock_dbg appears
/	as an argument to lock_dbgC.
/	
ENTRY(lock_dbg)
	pushl	SPARG2
	pushl	4+SPARG1
	pushl	8+SPARG0
ifdef(`UNIPROC',`
	call	lock_nodbg
',`
	call	lock_dbgC
')
	addl	$12, %esp
	ret
	SIZE(lock_dbg)

/ 
/ pl_t
/ trylock_dbg(lock_t *lockp, pl_t ipl)
/	Attempts to lock the given lock at the given ipl in debug mode.
/	If at first it does not succeed, gives up.
/
/ Calling/Exit State:
/	lockp is the lock to attempt to lock, ipl is the interrupt level
/	at which the acquisition should be attempted.  Returns the old
/	ipl if the lock is acquired, INVPL otherwise.
/
/ Description:
/	For UNIPROC, just call trylock_nodbg.
/
/	For non-UNIPROC, push the arguments, and call trylock_dbgC.
/	This is done so that the return address of trylock_dbg appears
/	as an argument to trylock_dbgC.
/	
ENTRY(trylock_dbg)
	pushl	SPARG1
	pushl	4+SPARG0
ifdef(`UNIPROC',`
	call	trylock_nodbg
',`
	call	trylock_dbgC
')
	addl	$8, %esp
	ret
	SIZE(trylock_dbg)

/ 
/ void
/ unlock_dbg(lock_t *lockp, pl_t ipl)
/ 	Release a lock in debug mode.
/
/ Calling/Exit State:
/	lockp is the lock to unlock, ipl is the ipl level to return at.
/	Returns:  None.
/
/ Description:
/	For UNIPROC, just call unlock_nodbg.
/
/	For non-UNIPROC, push the arguments, and call unlock_dbgC.
/	This is done so that the return address of unlock_dbg appears
/	as an argument to unlock_dbgC.
/	
ENTRY(unlock_dbg)
	pushl	SPARG1
	pushl	4+SPARG0
ifdef(`UNIPROC',`
	call	unlock_nodbg
',`
	call	unlock_dbgC
')
	addl	$8, %esp
	ret
	SIZE(unlock_dbg)
