	.ident	"@(#)kern-i386:util/rwlocks.s	1.30.1.2"
	.ident	"$Header$"
	.file	"util/rwlocks.s"

include(KBASE/svc/asm.m4)
include(assym_include)
include(KBASE/util/ksynch.m4)

/
/ pl_t
/ RW_RDLOCK(rwlock_t *lockp, pl_t ipl)
/	Acquires the given lock in read mode at the given ipl.
/
/ Calling/Exit State:
/	Returns the previous ipl.
/
/ Description:
/	Raise the ipl and try for the lock via an atomic
/	xchg instruction.  If the lock was not previously held,
/	then the acquisition is successful: set the state to
/	RWS_READ and then return.
/
/	If the lock was previously held, then see whether it
/	was held in read mode (i.e., whether rws_state
/	is equal to RWS_READ).  If it's held in read mode,
/	then try to acquire it as a shared reader, as described
/	in the next paragraph.  If it's not held in read mode,
/	then spin for the lock as described below.
/
/	To acquire the lock as a shared reader, first, acquire the
/	fspin lock which protects certain fields of the lock.  Once
/	the fspin lock is acquired, then increment the reader count.
/	If, after incrementing the reader count, the lock is still
/	held in read mode, then the shared acquisition has been
/	successful: release the fspin lock and then return.
/
/	However, if the lock is not held in read mode at this point,
/	it means that the attempt to acquire as a shared reader failed
/	because of a race with one or more unlocking readers.  If this
/	occurs, then decrement the reader count, release the fspin lock,
/	and spin for the lock.
/	
/	To spin for the lock, restore the entry ipl and check whether
/	preemption should be taken.  Then, spin until either the lock
/	is released or it's held in read mode; once either of these
/	conditions is met, jump to the beginning and start the whole
/	process over.
/
/ Remarks:
/	For UNIPROC, the lock argument is ignored; this routine just
/	disables preemption, raises the ipl, and returns the old ipl.
/	
/	Register usage:
/		%ecx		pointer to lock
/		%eax		switches between new and old ipl
/		%edx		temp stuff
/
ENTRY(rw_rdlock)
ifndef(`UNIPROC',`
	movl	SPARG0, %ecx		/ move &lock into known location
')
	movl	SPARG1, %eax		/ ipl into known register

ifndef(`UNIPROC',`
.rdloop:
')
	__DISABLE_PRMPT_ASM		/ Disable kernel preemption
	__SPL_ASM			/ Raise pl to %eax, return old in %eax

ifndef(`UNIPROC',`
	movb	$_A_SP_LOCKED,%dl	/ value to exchange
	xchgb	%dl,_A_RWS_LOCK(%ecx)	/ try for lock
	cmpb	$_A_SP_UNLOCKED,%dl	/ was previously unlocked?
	jne 	.rdbusy			/ if not, then see if in read mode
	movb	$_A_RWS_READ, _A_RWS_STATE(%ecx)
') / UNIPROC
	ret

ifndef(`UNIPROC',`
.rdbusy:
	cmpb	$_A_RWS_READ, _A_RWS_STATE(%ecx)
	jne	.rdfailed
	
	/ Acquire the fspin lock protecting read count and
	/ increment the read count.   The order in which the
	/ following is done is significant to the unlock path;
	/ see comments in rw_rdunlock.
	__FSPIN_LOCK_ASM(%ecx, %dl)
	incb	_A_RWS_RDCOUNT(%ecx)

	/
	/ Make sure the lock is still held in read mode.  If it is, then
	/ return.  Otherwise, decrement the read count and try again.
	/
	cmpb	$_A_SP_UNLOCKED, _A_RWS_LOCK(%ecx)
	je	.rdfailed1
	cmpb	$_A_RWS_READ, _A_RWS_STATE(%ecx)
	jne	.rdfailed1
	__FSPIN_UNLOCK_ASM(%ecx, %dl)
	ret

.rdfailed1:
	decb	_A_RWS_RDCOUNT(%ecx)
	__FSPIN_UNLOCK_ASM(%ecx, %dl)
.rdfailed:				/ did not get lock on first try, so:
	__SPLX_ASM			/	(1) restore old pl
	__ENABLE_PRMPT_ASM		/	(2) Re-enable kernel preemption
	movl	SPARG0, %ecx		/	(3) Restore %ecx and %eax
	movl	SPARG1, %eax
.rdspin:				/	(4) then spin until
	cmpb	$_A_SP_UNLOCKED,_A_RWS_LOCK(%ecx)	/ lock is unlocked
	je	.rdloop					/ or
	cmpb	$_A_RWS_READ, _A_RWS_STATE(%ecx)	/ lock is in read state
	je	.rdloop
	jmp 	.rdspin
') / UNIPROC
	SIZE(rw_rdlock)

/
/ pl_t
/ RW_WRLOCK(rwlock_t *lockp, pl_t ipl)
/	Acquires the given rwspin lock in write mode at the given ipl.
/
/ Calling/Exit State:
/ 	Returns the previous ipl.
/
/ Description:
/	Raise the ipl and try for the lock via an atomic
/	xchg instruction.  If the lock was not previously held,
/	then the acquisition is successful: set the state to
/	RWS_WRITE and then return.
/
/	If the lock was previously held, then spin for the lock.
/	First, restore the entry ipl and check whether preemption
/	should be taken.  Then, spin until the lock is released,
/	then jump to the beginning and start the whole process over.
/
/ Remarks:
/	For UNIPROC, the lock argument is ignored; this routine just
/	disables preemption, raises the ipl, and returns the old ipl.
/	
/	Register usage:
/		%ecx		pointer to lock
/		%eax		switches between new and old ipl
/		%edx		temp stuff
/
ENTRY(rw_wrlock)
ifndef(`UNIPROC',`
	movl	SPARG0, %ecx		/ move &lock into known location
')
	movl	SPARG1, %eax		/ ipl into known register

ifndef(`UNIPROC',`
.wrloop:
')
	__DISABLE_PRMPT_ASM		/ Disable kernel preemption
	__SPL_ASM			/ Raise pl to %eax, return old in %eax

ifndef(`UNIPROC',`
	movb	$_A_SP_LOCKED,%dl	/ value to exchange
	xchgb	%dl,_A_RWS_LOCK(%ecx)	/ try for lock
	cmpb	$_A_SP_UNLOCKED,%dl	/ was previously unlocked?
	jne 	.wrfailed		/ if not, then go spin
	movb	$_A_RWS_WRITE, _A_RWS_STATE(%ecx)
') / UNIPROC
	ret

ifndef(`UNIPROC',`
.wrfailed:				/ did not get lock on first try, so:
	__SPLX_ASM			/	(1) restore old pl
	__ENABLE_PRMPT_ASM		/	(2) Re-enable kernel preemption
	movl	SPARG0, %ecx		/	(3) Restore %ecx and %eax
	movl	SPARG1, %eax
.wrspin:				/	(4) then spin until
	cmpb	$_A_SP_UNLOCKED,_A_RWS_LOCK(%ecx)
	je	.wrloop			/ ...lock is unlocked
	jmp 	.wrspin			/ spin while not clear
') / UNIPROC
	SIZE(rw_wrlock)

/
/ pl_t
/ RW_TRYRDLOCK(rwlock_t *lockp, pl_t ipl)
/	Attempts to acquire the given lock in read mode at the given ipl.
/	If at first we do not succeed, give up.
/
/ Calling/Exit State:
/	Returns the previous ipl if the lock is acquired, INVPL
/	otherwise.  The ipl is unchanged if the lock is not acquired.
/
/ Description:
/	For a description of how a lock is acquired in read mode,
/	see the Description of rw_rdlock above.  If the lock cannot
/	be acquired, this routines gives up and returns INVPL; it
/	does not spin.
/
/ Remarks:
/	For UNIPROC, the lock argument is ignored; this routine just
/	disables preemption, raises the ipl, and returns the old ipl.
/
/	Register usage:
/		%ecx		pointer to lock
/		%eax		switches between new and old ipl
/		%edx		temp stuff
/
ENTRY(rw_tryrdlock)
ifndef(`UNIPROC',`
	movl	SPARG0, %ecx		/ move &lock into known location
')
	movl	SPARG1, %eax		/ ipl into known register

	__DISABLE_PRMPT_ASM		/ Disable kernel preemption
	__SPL_ASM			/ Raise pl to %eax, return old in %eax

ifndef(`UNIPROC',`
	movb	$_A_SP_LOCKED,%dl	/ value to exchange
	xchgb	%dl,_A_RWS_LOCK(%ecx)	/ try for lock
	cmpb	$_A_SP_UNLOCKED,%dl	/ was previously unlocked?
	jne 	.trbusy			/ if not, then see if in read mode
	movb	$_A_RWS_READ, _A_RWS_STATE(%ecx)
') / UNIPROC
	ret

ifndef(`UNIPROC',`
.trbusy:
	cmpb	$_A_RWS_READ, _A_RWS_STATE(%ecx)
	jne	.trfailed
	
	/ Acquire the fspin lock protecting read count and
	/ increment the read count.   The order in which the
	/ following is done is significant to the unlock path;
	/ see comments in rw_rdunlock.
	__FSPIN_LOCK_ASM(%ecx, %dl)

	incb	_A_RWS_RDCOUNT(%ecx)
	/
	/ make sure the lock is still held in read mode.  If so, then return
	/	otherwise return failure.
	/
	cmpb	$_A_SP_UNLOCKED, _A_RWS_LOCK(%ecx)
	je	.trfailed1
	cmpb	$_A_RWS_READ, _A_RWS_STATE(%ecx)
	jne	.trfailed1
	__FSPIN_UNLOCK_ASM(%ecx, %dl)
	ret

.trfailed1:
	decb	_A_RWS_RDCOUNT(%ecx)
	__FSPIN_UNLOCK_ASM(%ecx, %dl)
.trfailed:				/ did not get the lock, so:
	__SPLX_ASM			/	(1) lower the ipl
	__ENABLE_PRMPT_ASM		/	(2) Re-enable kernel preemption
	movl	$_A_INVPL,%eax		/	(3) return failure
	ret
') / UNIPROC
	SIZE(rw_tryrdlock)

/
/ pl_t
/ RW_TRYWRLOCK(rwlock_t *lockp, pl_t ipl)
/	Attempts to lock the given lock in write mode at the given ipl.
/	If at first we do not succeed, give up.
/
/ Calling/Exit State:
/	lockp is the lock to attempt to acquire, ipl is the level at which
/	to acquire it.  If the lock cannot be acquired, the ipl is unchanged.
/
/	Returns: the previous ipl if successful, INVPL if the lock
/	cannot be acquired.
/
/ Description:
/	For a description of how a lock is acquired in write mode,
/	see the Description of rw_wrlock above.  If the lock cannot
/	be acquired, this routines gives up and returns INVPL; it
/	does not spin.
/
/ Remarks:
/	For UNIPROC, the lock argument is ignored; this routine just
/	disables preemption, raises the ipl, and returns the old ipl.
/
/	Register usage:
/		%ecx		pointer to lock
/		%eax		switches between new and old ipl
/		%edx		temp stuff
/
ENTRY(rw_trywrlock)
ifndef(`UNIPROC',`
	movl	SPARG0, %ecx		/ move &lock into known location
')
	movl	SPARG1, %eax		/ ipl into known register

	__DISABLE_PRMPT_ASM		/ Disable kernel preemption
	__SPL_ASM			/ Raise pl to %eax, return old in %eax

ifndef(`UNIPROC',`
	movb	$_A_SP_LOCKED,%dl	/ value to exchange
	xchgb	%dl,_A_RWS_LOCK(%ecx)	/ try for lock
	cmpb	$_A_SP_UNLOCKED,%dl	/ was previously unlocked?
	jne 	.twfailed		/ if not, then return failure
	movb	$_A_RWS_WRITE, _A_RWS_STATE(%ecx)
') / UNIPROC
	ret

ifndef(`UNIPROC',`
.twfailed:				/ did not get the lock, so:
	__SPLX_ASM			/	(1) lower the ipl
	__ENABLE_PRMPT_ASM		/	(2) Re-enable kernel preemption
	movl	$_A_INVPL,%eax		/	(3) return failure
	ret
') / UNIPROC
	SIZE(rw_trywrlock)

/
/ void
/ RW_UNLOCK(rwlock_t *lockp, pl_t ipl)
/	Unlocks the given lock and returns at the given ipl,
/ 	whether the lock was for read or for write.
/ 
/ Calling/Exit State:
/	lockp is the lock to release, ipl is the level at which to
/	return.
/
/	Returns:  None.
/
/ Description:
/	If the lock is held in write mode, then release the lock
/	by setting the state to RWS_UNLOCKED and the lock to SP_UNLOCKED.
/
/	If the lock is held in read mode (which means that the rws_state
/	field is either RWS_READ or RWS_UNLOCKED; see Remarks below),
/	then check to see if this is the last unlocking reader.  If
/	the rdcount field is 0 and the fspin lock protecting the rdcount
/	field is not held, then the unlock is likely to be the last
/	unlocking reader.  Set the rws_state field to RWS_UNLOCKED,
/	and then flush the cpu's write buffers to synchronize this change
/	with other processors.  If, after the write flush, the rdcount
/	field is still 0 and the fspin lock is still not held, then
/	handle as in the write mode case: set the state to RWS_UNLOCKED
/	(which is redundant) and set the lock to SP_UNLOCKED.
/
/	If at any point during the release of a read mode lock it is
/	detected that the rdcount field is non-zero or the fspin lock
/	is held, then the lock release is treated as a shared release,
/	as follows.  Acquire the fspin lock; if the rdcount field is
/	non-zero, decrement the rdcount field, and release the fspin lock.
/	If the rdcount field is 0, then set the state to RWS_UNLOCKED
/	and the lock to SP_UNLOCKED, and release the fspin lock.
/
/	Once the lock has been released based on the various cases above,
/	lower the ipl, enable preemption, and check whether a preemption
/	should be taken.
/
/ Remarks:
/	For UNIPROC, the lock argument is ignored; this routine just
/	lowers the ipl, and re-enables and checks for preemption.
/
/	Because of races, a lock which is held in read mode may have
/	its state set to RWS_UNLOCKED.  This is a benign condition,
/	but must be accounted for.
/
/	Register usage:
/		%ecx		pointer to lock
/		%eax		switches between new and old ipl
/		%edx		temp stuff
/ 
ENTRY(rw_unlock)
ifndef(`UNIPROC',`
	movl	SPARG0,%ecx		/ &lock into known register
')
	movl	SPARG1,%eax		/ ipl into known register

ifndef(`UNIPROC',`
	/ if mode is write, go unlock it
	cmpb	$_A_RWS_WRITE, _A_RWS_STATE(%ecx)
	je	.ulunlock

	/
	/ If the fspin lock is held or if there are multiple
	/	readers, then go handle the shared reader case.
	/	Check the fspin lock before checking the read
	/	count to prevent races with any simultaneous
	/	rw_rdlocks.  Note that acquiring readers will
	/	increment the read count and then release the
	/	fspin lock.
	/
	cmpb	$_A_SP_UNLOCKED, _A_RWS_FSPIN(%ecx)
	jne	.ulshared
	cmpb	$0, _A_RWS_RDCOUNT(%ecx)
	jne	.ulshared

	/
	/ No other readers and fspin lock not held, so handle
	/	as last releasing reader.
	/ First, put in unlocked state and do a write flush
	/
	movb	$_A_RWS_UNLOCKED, _A_RWS_STATE(%ecx)
	pushl	%ecx
	pushl	%eax
	__WRITE_SYNC_ASM
	popl	%eax
	popl	%ecx

	/
	/ Next, check for races: make sure the fspin is still unlocked,
	/	there are still no other readers, and the state
	/	is still unlocked.  If any of these tests fail, then
	/	handle as a shared unlock, below.  Given the order in
	/	which the reader does things, we first check the
	/	fspin and then the read count.
	/
	cmpb	$_A_SP_UNLOCKED, _A_RWS_FSPIN(%ecx)
	jne	.ulshared
	cmpb	$0, _A_RWS_RDCOUNT(%ecx)
	jne	.ulshared
	cmpb	$_A_RWS_UNLOCKED, _A_RWS_STATE(%ecx)
	je	.ulunlock

.ulshared:
	/
	/ Handle shared reader case
	/ First, acquire fspin lock
	/
	__FSPIN_LOCK_ASM(%ecx, %dl)

	/
	/ Make sure lock count is still non-zero.  If zero,
	/	go to handle as last reader
	/
	cmpb	$0, _A_RWS_RDCOUNT(%ecx)
	je	.ulsfree

	/
	/ Next, set to read mode (in case in unlocked mode from above),
	/	decrement shared read counter, release fspin, and then
	/	go off to lower ipl and enable preemption
	/
	movb	$_A_RWS_READ, _A_RWS_STATE(%ecx)
	decb	_A_RWS_RDCOUNT(%ecx)
	__FSPIN_UNLOCK_ASM(%ecx, %dl)
	jmp	.ulsplx

.ulsfree:
	/
	/ lock count was zero from above - we raced with other unlocking
	/	readers, but now we are the last one.
	/ Set state to RWS_UNLOCKED and release the FSPIN, and then
	/	fall through to the last reader case.  Note that we
	/	have to set the rws_state to RWS_UNLOCKED _before_ releasing
	/	the fspin and then set rws_lock to SP_UNLOCKED _after_
	/	releasing the fspin in order to prevent races.
	/
	movb	$_A_RWS_UNLOCKED, _A_RWS_STATE(%ecx)
	__FSPIN_UNLOCK_ASM(%ecx, %dl)
.ulunlock:
	movb	$_A_RWS_UNLOCKED, _A_RWS_STATE(%ecx)
	movb	$_A_SP_UNLOCKED, _A_RWS_LOCK(%ecx)	/ release the lock

.ulsplx:
')
	__SPLX_ASM			/ lower the ipl
	__ENABLE_PRMPT_ASM		/ Re-enable kernel preemption
	ret				/ return
	SIZE(rw_unlock)

/
/ pl_t
/ rw_rdlock_dbg(rwlock_t *lockp, pl_t ipl)
/ 	Acquire a read-write spin lock in read mode in debug mode.
/
/ Calling/Exit State:
/	lockp is the read-write spin lock to be acquired.  ipl is the
/	desired ipl, which must be greater than or equal to the current
/	ipl.  Upon return, the lock is held at the given ipl.
/
/	Returns:  the previous ipl.
/
/ Description:
/	For UNIPROC, just call (actually, branch to) rw_rdlock.
/
/	For non-UNIPROC, push the arguments, and call rw_rdlock_dbgC.
/	This is done so that the return address of rw_rdlock_dbg appears
/	as an argument to rw_rdlock_dbgC.
/
ENTRY(rw_rdlock_dbg)
ifdef(`UNIPROC',`
	jmp	rw_rdlock
',`
	pushl	SPARG2
	pushl	4+SPARG1
	pushl	8+SPARG0
	call	rw_rdlock_dbgC
	addl	$12, %esp
	ret
')
	SIZE(rw_rdlock_dbg)
/
/ pl_t
/ rw_wrlock_dbg(rwlock_t *lockp, pl_t ipl)
/ 	Acquire a read-write spin lock in write mode in debug mode.
/
/ Calling/Exit State:
/	lockp is the read-write spin lock to be acquired.  ipl is the
/	desired ipl, which must be greater than or equal to the current
/	ipl.  Upon return, the lock is held at the given ipl.
/
/	Returns:  the previous ipl.
/
/ Description:
/	For UNIPROC, just call (actually, branch to) rw_wrlock.
/
/	For non-UNIPROC, push the arguments, and call rw_wrlock_dbgC.
/	This is done so that the return address of rw_wrlock_dbg appears
/	as an argument to rw_wrlock_dbgC.
/
ENTRY(rw_wrlock_dbg)
ifdef(`UNIPROC',`
	jmp	rw_wrlock
',`
	pushl	SPARG2
	pushl	4+SPARG1
	pushl	8+SPARG0
	call	rw_wrlock_dbgC
	addl	$12, %esp
	ret
')
	SIZE(rw_wrlock_dbg)

/
/ pl_t
/ rw_tryrdlock_dbg(rwlock_t *lockp, pl_t ipl)
/	Attempts to lock the given read-write spin lock at the given
/	ipl in read mode and in debug mode.  If at first it does not
/	succeed, gives up.
/
/ Calling/Exit State:
/	lockp is the read-write spin lock to attempt to lock, ipl is
/	the interrupt level at which the acquisition should be attempted.
/	Returns the old ipl if the lock is acquired, INVPL otherwise.
/
/ Description:
/	For UNIPROC, just call (actually branch to) rw_tryrdlock.
/
/	For non-UNIPROC, push the arguments, and call rw_tryrdlock_dbgC.
/	This is done so that the return address of rw_tryrdlock_dbg appears
/	as an argument to rw_tryrdlock_dbgC.
/	
ENTRY(rw_tryrdlock_dbg)
ifdef(`UNIPROC',`
	jmp	rw_tryrdlock
',`
	pushl	SPARG1
	pushl	4+SPARG0
	call	rw_tryrdlock_dbgC
	addl	$8, %esp
	ret
')
	SIZE(rw_tryrdlock_dbg)

/
/ pl_t
/ rw_trywrlock_dbg(rwlock_t *lockp, pl_t ipl)
/	Attempts to lock the given read-write spin lock at the given
/	ipl in write mode and in debug mode.  If at first it does not
/	succeed, gives up.
/
/ Calling/Exit State:
/	lockp is the read-write spin lock to attempt to lock, ipl is
/	the interrupt level at which the acquisition should be attempted.
/	Returns the old ipl if the lock is acquired, INVPL otherwise.
/
/ Description:
/	For UNIPROC, just call (actually branch to) rw_trywrlock.
/
/	For non-UNIPROC, push the arguments, and call rw_trywrlock_dbgC.
/	This is done so that the return address of rw_trywrlock_dbg appears
/	as an argument to rw_trywrlock_dbgC.
/	
ENTRY(rw_trywrlock_dbg)
ifdef(`UNIPROC',`
	jmp	rw_trywrlock
',`
	pushl	SPARG1
	pushl	4+SPARG0
	call	rw_trywrlock_dbgC
	addl	$8, %esp
	ret
')
	SIZE(rw_trywrlock_dbg)

/ 
/ void
/ rw_unlock_dbg(rwlock_t *lockp, pl_t ipl)
/ 	Release a read-write spin lock in debug mode.
/
/ Calling/Exit State:
/	lockp is the lock to unlock, ipl is the ipl level to return at.
/	Returns:  None.
/
/ Description:
/	For UNIPROC, just call (actually branch to) rw_unlock.
/
/	For non-UNIPROC, push the arguments, and call rw_unlock_dbgC.
/	This is done so that the return address of rw_unlock_dbg appears
/	as an argument to rw_unlock_dbgC.
/	
ENTRY(rw_unlock_dbg)
ifdef(`UNIPROC',`
	jmp	rw_unlock
',`
	pushl	SPARG1
	pushl	4+SPARG0
	call	rw_unlock_dbgC
	addl	$8, %esp
	ret
')
	SIZE(rw_unlock_dbg)
