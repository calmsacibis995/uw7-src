	.ident	"@(#)kern-i386:util/fspins.s	1.6.1.1"
	.ident	"$Header$"
	.file	"util/fspins.s"

/
/ stuff for fast spin locks (fspin_t's):  init, lock, trylock, unlock.
/ Also DISABLE and ENABLE.  For architectures unlike the x86 which do
/ not have two distinct ways of masking interrupts (here, (1) at the
/ chip, and (2) at the interrupt controller), FSPIN_LOCK should get
/ the current ipl and save it away, either in processor-local memory
/ or in an additional member of the fspin_t, and set the ipl to PLHI.
/ FSPIN_UNLOCK should set the ipl to the saved value.
/

include(KBASE/svc/asm.m4)
include(assym_include)
include(KBASE/util/ksynch.m4)

/
/ void
/ FSPIN_INIT(fspin_t *lockp)
/	Initializes a fast spin lock.
/
/ Calling/Exit State:
/	Returns:  None.
/
/ Description:
/	Register usage:
/		%ecx		pointer to the lock
/
ENTRY(FSPIN_INIT)
	movl	SPARG0, %ecx		/ move &lock into known reg
	movb	$_A_SP_UNLOCKED, (%ecx)	/ store initial value
	ret

/
/ void
/ FSPIN_LOCK(fspin_t *lockp)
/	Acquires the fast spin lock and disables interrupts at the
/	CPU.
/
/ Calling/Exit State:
/	Interrupts are disabled upon return.
/
/	Returns:  None.
/
/ Description:
/	Register usage:
/		%ecx		pointer to the lock
/
/ Remarks:
/	Fast spin locks may not be nested.  Code like this:
/
/		FSPIN_LOCK(A);
/			FSPIN_LOCK(B);
/			FSPIN_UNLOCK(B);
/		FSPIN_UNLOCK(A);
/
/	has a bug, that interrupts are enabled after the first unlock,
/	before the caller wanted to take interrupts.  It's also a Bad
/	Idea to have code like:
/
/		FSPIN_LOCK(A);
/		LOCK(B);
/		FSPIN_UNLOCK(A);
/		UNLOCK(B);
/
/	because an interrupt may be pending after the FSPIN_LOCK
/	at any priority, and may be  delivered to the cpu after 
/	the FSPIN_UNLOCK,
/	creating an interrupt context at a lower ipl than this code is
/	prepared for.  It's also a bad idea to write code like
/	
/		FSPIN_LOCK(A)
/		LOCK(B)
/		UNLOCK(B)
/		FSPIN_UNLOCK(A)
/
/	because, on an architecture that has only one way of masking
/	interrupts (i.e. mc68k), interrupts may be taken after the LOCK,
/	when they aren't expected.  There is an exception when the regular
/	lock is acquired at PLHI, but code like this is opposed on general
/	principles.
/
ENTRY(FSPIN_LOCK)
	movl	SPARG0, %ecx		/ move &lock to known reg

ifdef(`DEBUG',`
ifdef(`_LOCKTEST', `
	/ ASSERT(! l.holdfastlock)
	cmpb	$_A_B_FALSE, KVPLOCAL_L_HOLDFASTLOCK
	je	.flnopanic		/ we hold no fspins, so do not panic
	cmpb	$_A_SP_LOCKED, panic_lock
	je	.flnopanic		/ Already panicking, don not panic.
	pushl	[lk_panicstr+0x14]	/ panic string 5
	pushl	$_A_CE_PANIC
	/
	/ A processor attempted to acquire a fast spin lock
	/ while holding another.  This nesting is illegal,
	/ and indicates a kernel software problem.
	/
	call	cmn_err			/ "nested fspin locks"
	addl	$8, %esp
	/ NOTREACHED (adjust %esp anyway, so stack traces will work)

.flnopanic:
')
')

	movb	$_A_SP_LOCKED, %al	/ the value to exchange
.flloop:	
	__DISABLE_ASM			/ disable interrupts
	xchgb	%al, (%ecx)		/ try for the lock
	cmpb	$_A_SP_UNLOCKED, %al	/ was it unlocked before?
	jne	.flfail			/ no, we did not get the lock
ifdef(`DEBUG',`
ifdef(`_LOCKTEST', `
	movb	$_A_B_TRUE, KVPLOCAL_L_HOLDFASTLOCK
	movl	%ecx, KVPLOCAL_L_FSPIN
')
')
	/ Got the lock; collect stats.
ifdef(`DEBUG',`
ifdef(`_MPSTATS',`
	incl	fspinlk_count
	movl	(%esp),%eax
	pushl	%eax			/ arg: return pc
	pushl	%ecx			/ arg: lock pointer
	call	begin_lkprocess
	addl	$8,%esp
')
',`
ifdef(`SPINDEBUG',`
ifdef(`_MPSTATS',`
	incl	fspinlk_count
	movl	(%esp),%eax
	pushl	%eax
	pushl	%ecx
	call	begin_lkprocess
	addl	$8,%esp
')
')
')

	ret
.flfail:
	__ENABLE_ASM			/ re-enable interrupts for spinning
.flspin:	
	cmpb	$_A_SP_UNLOCKED, (%ecx)	/ ...and spin until unlocked
	je	.flloop			/ unlocked: try again
	jmp	.flspin

/
/ boolean_t
/ FSPIN_TRYLOCK(fspin_t *lockp)
/	Attempts to lock the given fast spin lock.
/
/ Calling/Exit State:
/	returns B_TRUE if lock was acquired (with interrupts disabled),
/	B_FALSE if not.
/
/ Description:
/	Register usage:
/		%ecx		pointer to the lock
/
ENTRY(FSPIN_TRYLOCK)
ifdef(`DEBUG',`
ifdef(`_LOCKTEST', `
	cmpb	$_A_B_FALSE, KVPLOCAL_L_HOLDFASTLOCK
	je	.ftnopanic		/ we hold no fspins, so do not panic
	cmpb    $_A_SP_LOCKED, panic_lock
	je	.ftnopanic		/ Already panicking, don not panic.

	pushl	[lk_panicstr+0x14]	/ panic string 5
	pushl	$_A_CE_PANIC
	/
	/ A processor attempted to acquire a fast spin lock while
	/ holding another.  This nesting is illegal, and indicates
	/ a kernel software problem.
	/
	call	cmn_err			/ "nested fspin locks"
	addl	$8, %esp
	/ NOTREACHED (adjust %esp anyway, so stack traces will work)
.ftnopanic:
')
')

	movl	SPARG0, %ecx		/ &lock to known location
	movl	$_A_B_TRUE, %eax	/ plan to succeed
	movb	$_A_SP_LOCKED, %dl	/ value to exchange
	cmpb	%dl, (%ecx)		/ is it locked?
	je	.ftfail			/ yes, don't bother trying
	__DISABLE_ASM			/ disable interrupts
	xchgb	%dl, (%ecx)		/ try for the lock
	cmpb	$_A_SP_UNLOCKED, %dl	/ was previously unlocked?
	je	.ftdone			/ yes, succeed
	__ENABLE_ASM			/ no, re-enable interrupts
.ftfail:	
	movl	$_A_B_FALSE, %eax	/ return failure
	ret
.ftdone:
ifdef(`DEBUG',`
ifdef(`_LOCKTEST', `
	movb	$_A_B_TRUE, KVPLOCAL_L_HOLDFASTLOCK
	movl	%ecx, KVPLOCAL_L_FSPIN
')
')

ifdef(`DEBUG',`
ifdef(`_MPSTATS',`
	pushl	%eax
	incl	fspinlk_count
	movl	4(%esp),%edx
	pushl	%edx			/ arg: return pc
	pushl	%ecx			/ arg: lock ponter
	call	begin_lkprocess
	addl	$8,%esp
	popl	%eax
')
',`
ifdef(`SPINDEBUG',`
ifdef(`_MPSTATS',`
	pushl	%eax
	incl	fspinlk_count
	movl	4(%esp),%edx
	pushl	%edx
	pushl	%ecx
	call	begin_lkprocess
	addl	$8,%esp
	popl	%eax
')
')
')
	ret

/
/ void
/ FSPIN_UNLOCK(fspin_t *lockp)
/	Unlocks the given fast spin lock and re-enables interrupts.
/ 
/ Calling/Exit State:
/	Returns:  None.
/	Returns with interrupts enabled.
/
/ Description:
/	Register usage:
/		%ecx		pointer to the lock
/
ENTRY(FSPIN_UNLOCK)
	movl	SPARG0, %ecx		/ move &lock to known location

ifdef(`DEBUG',`
ifdef(`_LOCKTEST', `
	movb	$_A_B_FALSE, KVPLOCAL_L_HOLDFASTLOCK
	movl	$0, KVPLOCAL_L_FSPIN
	cmpb	$_A_SP_LOCKED, (%ecx)	/ is the lock locked?
	je 	.funopanic		/ yes, dont panic

	pushl	[lk_panicstr+0xC]	/ panic string 3
	pushl	$_A_CE_PANIC
	/ 
	/ A processor attempted to unlock a spin lock that was already
	/ unlocked.  This implies a kernel software problem.
	/
	call	cmn_err			/ "unlocking unlocked lock"
	addl	$8, %esp
	/ NOTREACHED (adjust %esp anyway, so stack traces will work)
.funopanic:
')
')

	movb	$_A_SP_UNLOCKED, (%ecx)
	__ENABLE_ASM			/ enable interrupts
ifdef(`DEBUG',`
ifdef(`_MPSTATS',`
	movl	(%esp),%edx
	pushl	%edx			/ arg: return pc
	pushl	%ecx			/ arg: lock pointer
	call	end_lkprocess
	addl	$8,%esp
')
',`
ifdef(`SPINDEBUG',`
ifdef(`_MPSTATS',`
	movl	(%esp),%edx
	pushl	%edx
	pushl	%ecx
	call	end_lkprocess
	addl	$8,%esp
')
')
')
	ret


/
/ void
/ DISABLE(void)
/	Block all interrupts regardless of current ipl value.
/
/ Calling/Exit State:
/	Returns with interrupts disabled.  Returns:  None.
/
/ Remarks:
/	We set HOLDFASTLOCK so we'll be warned (lambasted, really) if
/	somebody tries to nest DISABLE/ENABLE with fast spin locks.
/
ENTRY(DISABLE)
	__DISABLE_ASM			/ disable interrupts
ifdef(`DEBUG',`
ifdef(`_LOCKTEST', `
	/ ASSERT(! l.holdfastlock)
	cmpb	$_A_B_FALSE, KVPLOCAL_L_HOLDFASTLOCK
	je	.dinopanic		/ we hold no fspins, so do not panic
	cmpb    $_A_SP_LOCKED, panic_lock
	je	.dinopanic		/ already panicking	
	pushl	[lk_panicstr+0x18]	/ panic string 6
	pushl	$_A_CE_PANIC
	/
	/ A processor attempted to disable interrupts when interrupts
	/ were already disabled.  This indicates a kernel software problem.
	/
	call	cmn_err			/ "nested DISABLE"
	addl	$8, %esp
	/ NOTREACHED (adjust %esp anyway, so stack traces will work)

.dinopanic:
	movb	$_A_B_TRUE, KVPLOCAL_L_HOLDFASTLOCK
')
')
	ret

/
/ void
/ ENABLE(void)
/	Allow interrupts back in according to the current ipl value
/
/ Calling/Exit State:
/	Returns with interrupts enabled.  Returns:  None.
/
/	We clear HOLDFASTLOCK after making sure it's set, to let people
/	know if they enable interrupts before disabling them.
/
ENTRY(ENABLE)
ifdef(`DEBUG',`
ifdef(`_LOCKTEST', `
	cmpb	$_A_B_TRUE, KVPLOCAL_L_HOLDFASTLOCK
	je 	.ennopanic		/ yes, do not panic

	pushl	[lk_panicstr+0x1C]	/ panic string 7
	pushl	$_A_CE_PANIC
	/
	/ A processor attempted to enable interrupts when they were
	/ not previously disabled.  This indicates a kernel software
	/ problem.
	/
	call	cmn_err			/ "ENABLE without DISABLE"
	addl	$8, %esp
	/ NOTREACHED (adjust %esp anyway, so stack traces will work)
.ennopanic:
        movb    $_A_B_FALSE, KVPLOCAL_L_HOLDFASTLOCK
')
')
	__ENABLE_ASM			/ enable interrupts
	ret
