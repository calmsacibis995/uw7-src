	.ident	"@(#)kern-i386:svc/intr.s	1.42.3.1"
	.ident	"$Header$"
	.file	"svc/intr.s"

/
/ Machine dependent low-level kernel entry points for interrupt
/ and trap handling.
/

include(KBASE/svc/asm.m4)
include(assym_include)
include(KBASE/svc/intr.m4)
include(KBASE/util/debug.m4)

FILE(`intr.s')

/
/ Processor Trap Handlers.
/
/ There are two forms of trap entry -- those that push an error code
/ and those that don't.
/
/ TRAP_ENTER_NOERR assumes no trap error code, no need to pop it.
/ TRAP_ENTER_ERR assumes HW pushed the error code, thus pops it into
/	l.trap_err_code.
/
/ All entries are thru trap gates, thus interrupts are on at entry.
/
/ No save/restore of SPL -- code is supposed to nest this properly.
/
define(TRAP_ENTER_NOERR, `
	SAVE_DSEGREGS;
	TRAP_SAVE_REGS;
	pushl	$$1')

/
/ Canonicalizes the stack to the no error case, popping the error
/ code into l.trap_err_code.
/
define(TRAP_ENTER_ERR, `
	popl	%ss:l+_A_L_TRAP_ERR_CODE;
	SAVE_DSEGREGS;
	TRAP_SAVE_REGS;
	pushl	$$1')

	.set	EFL_DF, 0x0400		/ direction flag
	.set	EFL_NT, 0x4000		/ nested task

ENTRY(t_diverr)
	TRAP_ENTER_NOERR(_A_DIVERR)
	jmp	.trap_common
	SIZE(t_diverr)

ENTRY(t_dbg)
	TRAP_ENTER_NOERR(_A_SGLSTP)
	jmp	.trap_common
	SIZE(t_dbg)

ENTRY(t_int3)
	TRAP_ENTER_NOERR(_A_BPTFLT)
	jmp	.trap_common
	SIZE(t_int3)

ENTRY(t_into)
	TRAP_ENTER_NOERR(_A_INTOFLT)
	jmp	.trap_common
	SIZE(t_into)

ENTRY(t_check)
	TRAP_ENTER_NOERR(_A_BOUNDFLT)
	jmp	.trap_common
	SIZE(t_check)

ENTRY(t_und)
	TRAP_ENTER_NOERR(_A_INVOPFLT)
	jmp	.trap_common
	SIZE(t_und)

ENTRY(t_dna)
	TRAP_ENTER_NOERR(_A_NOEXTFLT)
	jmp	.trap_common
	SIZE(t_dna)

ENTRY(t_syserr)
	/ t_syserr didn't come in from a normal trap (see dftss),
	/ so we have to simulate the saving of %cs, %eip, and %efl that
	/ would normally occur
	pushfl
	lcall	$_A_KCSSEL, $.syserr
.syserr:
	incl	%ss:prmpt_state		/ Disable preemption
	pushl	$0xfffffff		/ dummy error code
	TRAP_ENTER_ERR(_A_DBLFLT)
	jmp	.trap_common
	SIZE(t_syserr)

ENTRY(t_extovr)
	incl	%ss:prmpt_state		/ Disable preemption
	sti
	TRAP_ENTER_NOERR(_A_EXTOVRFLT)
	jmp	.trap_common
	SIZE(t_extovr)

ENTRY(t_badtss)
	TRAP_ENTER_ERR(_A_INVTSSFLT)
	jmp	.trap_common
	SIZE(t_badtss)

ENTRY(t_notpres)
	TRAP_ENTER_ERR(_A_SEGNPFLT)
	jmp	.trap_common
	SIZE(t_notpres)

ENTRY(t_stkflt)
	TRAP_ENTER_ERR(_A_STKFLT)
	jmp	.trap_common
	SIZE(t_stkflt)

ENTRY(t_gpflt)
	TRAP_ENTER_ERR(_A_GPFLT)
	jmp	.trap_common
	SIZE(t_gpflt)
 
ENTRY(t_coperr)
	TRAP_ENTER_NOERR(_A_EXTERRFLT)
	jmp	.trap_common
	SIZE(t_coperr)
 
ENTRY(t_alignflt)
	TRAP_ENTER_NOERR(_A_ALIGNFLT)
	jmp	.trap_common
	SIZE(t_alignflt)
 
ENTRY(t_mceflt)
	incl	%ss:prmpt_state		/ Disable preemption
					/ sti will be done in _mca_hook
	TRAP_ENTER_NOERR(_A_MCEFLT)
	jmp	.trap_common
	SIZE(t_mceflt)

ENTRY(t_res)
	TRAP_ENTER_NOERR(_A_TRP_UNUSED)
	jmp	.trap_common
	SIZE(t_res)

ENTRY(t_pgflt)
	incl	%ss:prmpt_state		/ Disable preemption
	sti
	TRAP_ENTER_ERR(_A_PGFLT)
        / FALLTHROUGH

.trap_common:
	SETUP_KDSEGREGS			/ load up kernel segment registers
	cld				/ clear direction flag
.trap_kcommon:
	call	trap
	addl	$4, %esp
	IF_USERMODE(_A_SP_EIP(%esp), process_trapret)

	/
	/ Unconditionally clear the flag that marks the LWP as being 
	/ in the process of being preempted. Note that we do this 
	/ while masking interrupts.                
	cli
	movl	upointer,%ebx
	movl	_A_U_LWPP(%ebx),%ebx
	orl	%ebx, %ebx
	jz	.kmode_done
	movl	$_A_B_FALSE,_A_LWP_BPT(%ebx)
.kmode_done:
	TRAP_RESTORE_REGS		/ restore general registers
	RESTORE_DSEGREGS
	iret

	SIZE(t_pgflt)

ENTRY(process_trapret)
	movl	upointer,%ebx
	movl	_A_U_LWPP(%ebx),%ebx	/ %ebx has lwpp
	cli
.trapret_again:	
	testl	$_A_EVT_UPREEMPT, l+_A_L_EVENTFLAGS
	jnz	do_resched
	/
	/ Prior to returning to USER mode check to
	/ see if we need to do any trap event processing.
	/
	testl	$_A_TRAPEXIT_FLAGS, _A_LWP_TRAPEVF(%ebx)
	jnz	int_trapret
.uiret:
	movl	upointer,%ebx
	movl	_A_U_LWPP(%ebx),%ebx	/ %ebx has lwpp
	movl	$0x0,_A_LWP_START(%ebx)	/ lwpp->l_start = 0	
ifdef(`DEBUG',`
	call	check_basepl
')
	TRAP_RESTORE_REGS		/ restore user registers
	RESTORE_UDSEGREGS
	USER_IRET			/ return to user mode

	SIZE(process_trapret)

/
/ The int_trapret entry point is entered from the interrupt return path when:
/	(1) The system is returning from an interrupt to user level; and
/	(2) There are pending trap event flags on the lwp.
/
/ "Calling" state:
/	(1) lwp pointer is in %ebx
/	(2) The stack is setup as a trap frame (i.e., is has been
/		converted from an interrupt frame)
/
/ Remarks:
/	The "calling" state is not really a calling state, since the
/	interrupt code jumps here, and the code just continues to return
/	to user level.
/
ENTRY(int_trapret)
	sti
	call	evt_process		/ process event flags.     
	/ Check again for trap events.
	cli
	jmp	.trapret_again

	SIZE(int_trapret)

/
/ ...
/
ENTRY(intr_return)
ifdef(`DEBUG',`
	pushf
	popl	%eax
	andl	$0x200, %eax
	ASSERT(ul,`%eax',==,`$0')
')
/ if returning to user mode, check user preemption
	IF_USERMODE(_A_INTR_SP_IP-4(%esp), .ucheck_preempt)

/
/ check for pending kernel preemptions.  If any are pending, then
/	check to see if preempt state allows preemptions
/
	testl	$_A_EVT_KPRUNRUN, engine_evtflags
	jnz	.kcheck_preempt_state
/
/ return from interrupt to kernel mode
/
.kintret:
	INTR_RESTORE_REGS		/ restore scratch registers
	RESTORE_DSEGREGS
	iret				/ return from interrupt

/
/ Check preemption state to see if a pending kernel preemption should be done.
/	The kernel is not preemptable under the following conditions:
/		a) The engine is servicing an interrupt.
/		b) The engine has been marked non-preemptable (prmpt_state > 0).
/		c) The engine is not at base ipl.
/
/	Since all interrupts are serviced at an ipl greater than base ipl,
/	the check for condition (a) is superfluous.
/
/ If the checks for conditions (b) and (c) pass, then go off to trap_sched to
/	do the preemption.
/
.align	8
.kcheck_preempt_state:
	cmpl	$0, prmpt_state
	jne	.kintret
	cmpb	$_A_PLBASE, ipl	
	jne	.kintret
	/
	/ If the context is being preempted, simply return. If this is not
	/ done, we can blow the kernel stack.
	movl	upointer,%eax
	movl	_A_U_LWPP(%eax),%eax
	cmpl	$_A_B_FALSE, _A_LWP_BPT(%eax)
	jne	.kintret
	/
	/ Check if preemption is enabled in kernel
	/ 
	cmpl 	$0, prmpt_enable
	je	.kintret
	/ 
	/ Since we are going to preempt ourselves, clear the kernel
	/ preemption pending flag and mark the context as being in the
	/ process of being preempted.
	andl	$~_A_EVT_KPRUNRUN, engine_evtflags
	movl	$_A_B_TRUE, _A_LWP_BPT(%eax) / %eax has the LWP pointer
	jmp	trap_sched	/ Force a preemption.

.align	8
.ucheck_preempt:
/ check for pending user preemptions
	testl	$_A_EVT_RUNRUN, engine_evtflags
	jnz	trap_sched

/ Check for trap event processing
	movl	upointer, %eax
	movl	_A_U_LWPP(%eax), %eax
	testl	$_A_TRAPEXIT_FLAGS, _A_LWP_TRAPEVF(%eax)
	jnz	.utrapevt
/
/ return from interrupt to user mode
/
ifdef(`DEBUG',`
	call	check_basepl
')
	INTR_RESTORE_REGS		/ restore scratch registers
	RESTORE_UDSEGREGS
	USER_IRET			/ return from interrupt

/
/ handle trap event processing
/
.align	8
.utrapevt:
	INTR_TO_TRAP_REGS	/ Convert interrupt frame to trap frame
	movl	%eax,%ebx	/ Save lwpp in %ebx
	jmp	int_trapret	/ Go handle trap events
	SIZE(intr_return)

	/
	/ Arrange to redispatch (call trap() with TRP_PREEMPT type code).
	/ At entry here, stack is "standard" interrupt stack, except saved
	/ PL has been popped.  Must complete "pushal" and otherwise behave
	/ like a trap.
	/
ENTRY(trap_sched)
	INTR_TO_TRAP_REGS
do_resched:
ifdef(`_MPSTATS',`
	leal	l+_A_L_UPRMPTCNT, %eax		/ int *pi = &l.prmpt_user;
	IF_USERMODE(_A_SP_EIP(%esp), .utrap_update)
	leal	l+_A_L_KPRMPTCNT, %eax		/      pi = &l.prmpt_kern;
.utrap_update:
	incl	(%eax)				/ ++*pi;
')
	sti
	pushl	$_A_TRP_PREEMPT		/ "switch" trap-type
	jmp	.trap_kcommon		/ now handle as a trap

	SIZE(trap_sched)

/
/ NMI handler
/
/ Handle NMIs similar to traps, but call nmi() instead of trap().
/ However, increment the interrupt depth counter before calling nmi().
/ Also, disable preemption.  (Technically, preemption is implicitly
/ disabled since we leave the interrupt enable flag clear, but the
/ various ASSERTs do not check this.  Disabling preemption explicitly
/ also makes us more robust in case someone inadvertantly enables
/ interrupts during the NMI handling.)
/

ENTRY(t_nmi)
	SAVE_DSEGREGS			/ save all registers
	TRAP_SAVE_REGS
	SETUP_KDSEGREGS			/ load up kernel segment registers
	cld				/ clear direction flag

	incl	prmpt_state
	incl	plocal_intr_depth
	call	nmi
	decl	plocal_intr_depth
	decl	prmpt_state

	IF_USERMODE(_A_SP_EIP(%esp), .uiret)

	TRAP_RESTORE_REGS		/ restore general registers
	RESTORE_DSEGREGS
	iret

	SIZE(t_nmi)

/
/ void sys_call(void)
/
/	System call call gate handler.  Setup and call the ANSI-C system
/	call handler.
/
/ Calling/Exit State:
/
/	Must be called from user-mode.
/
/ Description:
/
/	Save the user-mode registers; initialize the segment registers
/	to their kernel values.  Note that we do not save and restore
/	the floating point registers, as the kernel does not use
/	floating point, thus, this need only be done at context switch time.
/
/	After the system call has been completed, we turn off interrupts
/	and recheck the runrun flag.  Turning off interrupts is necessary
/	to close a preemption race whereby we could have a preemption interrupt
/	occur after we checked the flag, but before we return to user-mode.
/	This would result in going back to user mode and not fielding the
/	preemption request.
/
ENTRY(sys_call)
	SAVE_DSEGREGS			/ save all registers
	TRAP_SAVE_REGS

	/ We came in via a far call through a call gate,
	/ but will eventually return with an iret, so we
	/ have to make it look like we came in via an interrupt.

	pushfl				/ write EFL on stack
	movl	(%esp), %eax		/ because "lcall" does not push
	movl	%eax, _A_SP_EFL+4(%esp)	/ flags (segment desc has arg count
					/ set to 1 to allocate space for it).
	/ clear NT flag like interrupt does;
	/ while we're at it, clear the direction flag
	andl	$~[EFL_NT+EFL_DF], (%esp)
	popfl

	SETUP_KDSEGREGS			/ load up kernel segment registers

	call	systrap			/ handle system call

	jmp	process_trapret

	SIZE(sys_call)

/
/ void sig_clean(void)
/
/	Signal cleanup call gate handler.  Set up and call the ANSI-C signal
/	clean handler.
/
/ Calling/Exit State:
/
/	Must be called from user-mode.
/
ENTRY(sig_clean)
	SAVE_DSEGREGS			/ save all registers
	TRAP_SAVE_REGS

	/ We came in via a far call through a call gate,
	/ but will eventually return with an iret, so we
	/ have to make it look like we came in via an interrupt.

	pushfl				/ write EFL on stack
	movl	(%esp), %eax		/ because "lcall" does not push
	movl	%eax, _A_SP_EFL+4(%esp)	/ flags (segment desc has arg count
					/ set to 1 to allocate space for it).
	/ clear NT flag like interrupt does;
	/ while we're at it, clear the direction flag
	andl	$~[EFL_NT+EFL_DF], (%esp)
	popfl

	SETUP_KDSEGREGS			/ load up kernel segment registers

	call	sigclean		/ system call return

	jmp	process_trapret

	SIZE(sig_clean)
 
/
/ void
/ enable_nmi(void)
/
/	This function re-enables NMIs internal to the processor.
/
/ Calling State/Exit State:
/
/	There is no return value and this function call should
/	only be called in the context of handling an NMI.
/
/ Description:
/
/	On entry, stack looks like
/		<return_eip>
/	Change to (stack grows down)
/		<flags>
/		<kernel_cs>
/		<return_eip>
/	so the iret works
/

ENTRY(enable_nmi)
	popl	%eax		/ return addr
	pushfl
	pushl	$_A_KCSSEL
	pushl	%eax
	iret

	SIZE(enable_nmi)


/
/ void initproc_return(..)
/	Return newly created init process to user level.
/
/ Calling/Exit State:
/ 	The stack has an argument that needs to be cleared prior to
/	returning to user land.
/

ENTRY(initproc_return)
	addl	$4,%esp
	jmp	.uiret			/ continue with normal user-mode iret
	SIZE(initproc_return)

/
/ void t_v86()
/
/ Calling/Exit State:
/	Executed for the V86 monitor to cause a task switch back to
/	the usual kernel mode. When v86call() is performed, the
/	context is saved in the v86_tss TSS, and this function
/	restores the context.
/		
ENTRY(t_v86)
	pushl	%eax
	str	%ax
	cmpw	$_A_KV86TSSSEL, %ax
	jne	.othermode
	popl	%eax
	ljmp	$_A_KTSSSEL, $0		/ KTSSSEL
.othermode:
	popl	%eax
	TRAP_ENTER_ERR(_A_GPFLT)
	jmp	.trap_common
	SIZE(t_v86)

/
/	- Intel Pentium "f0 0f c7 c8" fix: lock cmpxchg8b %eax locked Pentium,
/	This is only used for Merge processes.
/	Set by mki_set_idt_dpl() and mki_set_idt_entry() in mki.c
	
ENTRY(p5_pgflt)				/ just like intr.s t_pgflt()
	incl	%ss:prmpt_state		/ disable preemption
	sti
	TRAP_ENTER_ERR(_A_PGFLT)
	SETUP_KDSEGREGS
	cld
	movl	%cr2, %eax		/ get faulting address
	subl	p5_idt_pagebase, %eax	/ fault below unwritable idts?
	jae	.hiaddr			/ no: go to check it further
.pftrap:				/ yes: most faults fall through
	call	trap			/ just like intr.s .trap_kcommon
	addl	$4, %esp
	IF_USERMODE(_A_SP_EIP(%esp), process_trapret)
	/
	/ Unconditionally clear the flag that marks the LWP as being 
	/ in the process of being preempted. Note that we do this 
	/ while masking interrupts.                
	/
	cli
	movl	upointer,%ebx
	movl	_A_U_LWPP(%ebx),%ebx
	orl	%ebx, %ebx
	jz	.p5_kmode_done
	movl	$_A_B_FALSE,_A_LWP_BPT(%ebx)
.p5_kmode_done:
	TRAP_RESTORE_REGS		/ restore general registers
	RESTORE_DSEGREGS
	iret

.hiaddr:			
	cmpl	$[_A_PAGESIZE\*3], %eax / fault above unwritable idts?
	jae	.pftrap			/ yes: to normal page fault processing
	movl	trap_err_code, %eax	/ no: examine faultcode
	andl	$[_A_PF_ERR_PROT|_A_PF_ERR_WRITE|_A_PF_ERR_USER], %eax
					/ though the bug would have user cs,
					/ its idt fault is in supervisor mode
	cmpl	$[_A_PF_ERR_PROT|_A_PF_ERR_WRITE], %eax / supervisor write?
	jne	.pftrap			/ no: to normal page fault processing
					/ fault was on current INVOPFLT entry?
	movl	%cr2, %eax		/ get faulting address again
	subl	$[_A_INVOPFLT\*8], %eax	/ adjust to idt base (if INVOPFLT)
	subl	$8, %esp		/ make way for two longs on stack
	sidt	(%esp)			/ put current idt limit & base there
	cmpl	2(%esp), %eax		/ idt base matches adjusted faultaddr?
	popl	%eax			/  readjust stack with flags untouched
	popl	%eax			/  readjust stack with flags untouched
	jne	.pftrap			/ no: to normal page fault processing
					/ yes: Pentium "f0 0f c7 c8" bug
					/ (or panic trap E mistaken for it)
	popl	%eax			/ throw away the PGFLT we pushed
	TRAP_RESTORE_REGS		/ restore general registers for retrap
	RESTORE_UDSEGREGS		/ restore segment registers for retrap
	pushl	%eax			/ resave original %eax
	pushl	%ebp			/ resave original %ebp
	movl	%cr2, %ebp		/ get address yet again (%ebp for %ss)
	movl	4(%ebp), %eax		/ get top long from idt[INVOPFLT]
	testw	$0x1000, %ax		/ trap (not interrupt) gate?
	movw	(%ebp), %ax		/  get bottom short from idt[INVOPFLT]
	jnz	.reprmpt		/ yes: leave interrupts enabled
	cli				/ no: disable interrupts like gate
.reprmpt:				/ should we check_preemption() below?
	decl	%ss:prmpt_state		/ re-enable disabled preemption
	popl	%ebp			/ restore original %ebp
	xchgl	%eax, (%esp)		/ restore original %eax, and
	ret				/ ret through to INVOPFLT handler
	SIZE(p5_pgflt)			
