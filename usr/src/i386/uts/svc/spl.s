/	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995, 1996 Santa Cruz Operation, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Santa Cruz Operation, Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386:svc/spl.s	1.1"
	.ident	"$Header$"

include(KBASE/svc/asm.m4)
include(assym_include)
include(KBASE/util/debug.m4)
include(KBASE/svc/intr.m4)

FILE(`spl.s')

/
/       Set priority level routines
/
/	Routines in the "spl" family are used to set the system
/	priority level in order to block or enable interrupts.  In
/	a non-preemptable, uni-processor kernel, spl calls are
/	typically used to protect a data structure that is in the
/	process of being manipulated:
/
/	/*
/	 * Block interrupts that might affect
/	 * or rely on our data structure.
/	 */
/	saved_level = splN();
/
/		/*
/	...	 * Critical section that
/		 * manipulates data structure.
/		 */
/
/	/*
/	 * Restore interrupts by returning
/	 * to previous priority level.
/	 */
/	splx(saved_level);
/
/	On a multi-processor kernel, spl routines are typically used in
/	conjunction with multi-processor locks for the same purpose.
/	With the introduction of preemption, additional mechanisms for
/	disabling preemption are required as well.  The preemption mechanism
/	as currently implemented will not preempt the kernel if the priority
/	level is above PLBASE.
/
/	The numeric spl values date back to the PDP-11 days and are
/	maintained for reasons of tradition and compatibility.
/	The preferred method nowadays is to define symbolic names
/	(such as spltty or spltimeout) for use in particular subsystems,
/	and use the appropriate name to protect data structures
/	associated with a given subsystem.
/
/	spl0 is special; it sets the priority to its base level,
/	enabling all interrupts.  The historical associations for
/	the other numeric levels are as follows:
/
/	spl1 - minimal priority above base level, no particular device.
/	spl4 - character devices such as teletypes and paper tape.
/	spl5 - disk drives.
/	spl6 - the clock.
/	spl7 - maximal priority, blocks all interrupts.
/
/	Higher levels represent more critical interrupts, and an splN
/	blocks interrupts at levels N and below.
/
/	Care must be taken so as not to lower the priority unwittingly;
/	for example, if you are going to call splN() you must ensure
/	that your code is never called in circumstances where the
/	priority might already be higher than N.  This concern, and
/	the possibility of subtle bugs, could be eliminated if the
/	various spl routines were coded such that (with the exception
/	of spl0 and splx) they would automatically avoid lowering
/	the priority level.  The current implementation does not do this
/	for performance reasons.
/
/	spl(newpl) sets the priority level to newpl and returns the old
/	level.  It is called only with newpl greater than or equal to the
/	current pl, i.e., it is never called to lower the priority level.
/
/       splx(newpl) sets the priority level to newpl.  No value is returned.
/	It is typically, but not always, used to lower the priority level.
/
/	Each of the other spl routines (e.g. spl0, splhi, spltimeout) sets the
/	priority level specified by the spl to ipl mapping in ipl.h and
/	return the old level.
/
/	The effect of an spl() call is realized without actually
/	programming the physical interrupt hardware, and indeed there is
/	no particular relationship between the kernel's notion of
/	interrupt priority level and the interrupt prioritization
/	that the interrupt controller may indulge in.  All spl does is
/	set value of the variable "ipl"; when an interrupt is taken
/	its priority is compared with the value of ipl, and if it is lower,
/	the interrupt is masked off and recorded in a queue.  Whenever the
/	ipl is lowered by a call to splx(), the queue is checked and
/	interrupts in the queue are processed as appropriate.
/
/ MACRO
/ SPLXCHG(newpl)
/	Loads the specified newpl into ipl and returns the old value
/	of ipl in %eax.
/
define(`SPLXCHG',`
	xorl	%eax, %eax
	movb	ipl, %al
	movb	$1, ipl
')

/
/ MACRO
/ SPLN(numeric_pl)
/	Set system priority level to the numeric value specified.
/
/ Remarks:
/	Because no code ever runs above PLHI, picipl can never be
/	above PLHI.  Therefore, invoking `SPLPIC' on PLHI will never cause
/	the branch to be taken; thus, `SPLPIC' need not be invoked for
/	priority level equal to PLHI.
/
/	We know that no code runs above PLHI because:
/	    (1)	No interrupt service routine runs above PLHI
/	    (2)	No locks are acquired above PLHI, and no code ever
/		invokes spl or splx with values above PLHI.
/
/	Note also that it is necessary to change the ipl and then
/	make the deferred check.  Do it in the other order and an
/	interrupt could be deferred after we have made the check but
/	before the ipl is lowered.

define(`SPLN',`
	SPLXCHG($$1)
if(`$1 < _A_PLHI',`
	CHECKDEFER($$1)
')
	ret
')

/
/ MACRO
/ CHECKDEFER(pl)
/	Checks for interrupts in defer queue at a priority higher than
/	pl.  Used immediately after setting ipl to pl.
/

define(`CHECKDEFER',`
	cmpl	$1, picipl
	ja	splunwind
')

/
/ pl_t
/ splbase(void)
/	Set the system priority level to PLBASE.
/
/ Calling State/Exit State:
/	Returns the previous pl.
/
/	Must not be servicing an interrupt.
/
/ Remarks:
/	Aliased to spl0() until we convert base kernel references.
/
ENTRY(spl0)
ENTRY(splbase)
	ASSERT(ul,`plocal_intr_depth',==,`$0')	/ not servicing interrupt
	SPLN(_A_PLBASE)
	SIZE(splbase)

/
/ pl_t
/ splhi(void)
/	Set the system priority level to PLHI.
/
/ Calling State/Exit State:
/	Returns the previous pl.
/
ENTRY(splhi)
	SPLN(_A_PLHI)
	SIZE(splhi)

/
/ pl_t
/ spldisk(void)
/	Set the system priority level to PLDISK.
/
/ Calling State/Exit State:
/	Returns the previous pl.
/
ENTRY(spldisk)
	SPLN(_A_PLDISK)
	SIZE(spldisk)

/
/ pl_t
/ splstr(void)
/	Set the system priority level to PLSTR.
/
/ Calling State/Exit State:
/	Returns the previous pl.
/
ENTRY(splstr)
	SPLN(_A_PLSTR)
	SIZE(splstr)

/
/ pl_t
/ spltimeout(void)
/	Set the system priority level to PLTIMEOUT.
/
/ Calling State/Exit State:
/	Returns the previous pl.
/
ENTRY(spltimeout)
	SPLN(_A_PLTIMEOUT)
	SIZE(spltimeout)

/
/ pl_t
/ spltty(void)
/	Set the system priority level to PLTTY.
/
/ Calling State/Exit State:
/	Returns the previous pl.
/
ENTRY(spltty)
	SPLN(_A_PLTTY)
	SIZE(spltty)

/
/ pl_t
/ spl(pl_t newpl)
/	Set the system priority level to newpl.
/
/ Calling/Exit State:
/	Returns the previous pl
/
/ Remarks:
/	The new pl must be greater than or equal to the old pl.
/
ENTRY(spl)
	movl	SPARG0, %edx
	ASSERT(ul,%edx,<=,`$_A_PLHI')		/ newpl <= PLHI
/#	ASSERT(ub,%dl,>=,`ipl')			/ newpl >= ipl, BADASSERT
	SPLXCHG(%dl)
	ret
	SIZE(spl)

/
/ void
/ splx(pl_t newpl)
/	Set the system priority level to newpl.
/
/ Calling/Exit State:
/	None required.  As it happens, it returns the previous pl value.
/
ENTRY(splx)
	movl	SPARG0, %edx
	ASSERT(ul,%edx,<=,`$_A_PLHI')		/ newpl <= PLHI
ifdef(`DEBUG',`
	LABEL(`ok')
	cmpl	$0, plocal_intr_depth		/ if not servicing interrupt
	je	ok				/	no worries
	ASSERT(ul,%edx,!=,`$_A_PLBASE')		/ newpl better not be PLBASE
ok:
	popdef(`ok')
')
	SPLXCHG(%dl)
	CHECKDEFER(%edx)
	ret
	SIZE(splx)
	
/
/ getpl(void)
/	Get the current ipl.
/
/ Calling/Exit State:
/	None.
/

ENTRY(getpl)
	xorl	%eax, %eax	/ zero out upper three bytes of %eax
	movb	ipl, %al	/ Load the current ipl into lsb of %eax
	ret
	SIZE(getpl)

/
/ splunwind(void)
/
/ Interface to intr_undefer to process deferred interrupts.
/ intr_undefer must always be called with interrupts disabled.
/	Note: must also save/restore %eax 
/
/ Common branch point for spl* routines.  Branched here whenever there
/ is a deferred interrupt with ipl higher than the current ipl.
/ A conditional branch to spldodefer must always precede a return
/ instruction, since spldodefer returns when it is done.
/
/ Also used in inline lock routines to process deferred interrupts when lowering
/ ipl. 
/

ENTRY(splunwind)
	pushl	%eax
	pushf				/ save flags and disable interrupts
	cli
	call	intr_undefer		/ do deferred interrupt processing
	popf
	popl	%eax
	ret
	SIZE(splunwind)
