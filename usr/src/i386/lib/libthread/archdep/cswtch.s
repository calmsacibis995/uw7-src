	.ident	"@(#)libthread:i386/lib/libthread/archdep/cswtch.s	1.1.5.19"
	.ident	"$Header$"
	.file	"archdep/cswtch.s"

/*								            */	
/* void
/* _thr_resume(thread_desc_t *newtp, thread_desc_t *oldtp,                 
/*             boolean_t sigflag, boolean_t dontsave, boolean debug_on)
/*
/*	Switch context from one thread to another.
/*
/* Calling/Exit State:
/*
/*	Should be called holding the thread mutex of both newtp and oldtp 
/*	(if non-NULL).
/*	Returns with both locks released.
/*
/* Description:
/*
/*	Save the context of "old" (if non-NULL) and load the context of "new".
/*      If debug_on is TRUE we call debugger notify routine
/*      on both entry and exit to switch.
/*									    */

/ address of the pointer on the per-engine page that points to the
/ lwp private data area for the currently executing thread. The word
/ following this pointer is used as a boolean to determine if a floating
/ point operation was done by the current thread during its time slice
/ and is set by the kernel.

.globl __lwp_priv_datap

ENTRY(_thr_resume)
	/ first check if debugger is on
	movl	20(%esp),%eax		/ debug_on
	testl	%eax,%eax
	je	.no_dbg1

	/ debugger is on; call _thr_debug_notify
	movl	4(%esp),%ecx		/ %ecx = newtp
	pushl	$TC_SWITCH_BEGIN	
	movl	t_tid(%ecx),%eax
	cmpl	$0,%eax			/ is newtp a user thread?
	jl	.bnew_idle
	pushl	%ecx	
	jmp	.do_dbg_call
.bnew_idle:
	pushl	$0			/ idle thread, just push 0
.do_dbg_call:
	call	_thr_debug_notify
	addl	$8,%esp

.no_dbg1:
	movl	8(%esp),%edx		/ %edx = oldtp

	/ set the t_lwpp field of newtp to point at the LWP private area
	/ of the LWP it will run on.
	movl	4(%esp),%ecx            / %ecx = newtp
	movl	t_lwpp(%edx),%eax       / %eax = oldtp->t_lwpp
	movl	%eax,t_lwpp(%ecx)       / newtp->t_lwpp = %eax

	/ do not save the context of oldtp if dontsave is zero
	movl	16(%esp),%eax		/ eax = dontsave
	testl	%eax,%eax
	jz	.resume_nosave		/ do not save registers if %eax == 0

	/ set inconsistent flag for use of debugger
	movb	$1,t_dbg_switch(%edx)

	/
	/ Save the context of "oldtp".
	/
	leal	t_ucontext(%edx),%ecx
	movl	%ebx,UCTX_R_EBX(%ecx)	/ %ebx
	movl	%ebp,UCTX_R_EBP(%ecx)	/ %ebp
	movl	%edi,UCTX_R_EDI(%ecx)	/ %edi
	movl	%esi,UCTX_R_ESI(%ecx)	/ %esi
	movl	(%esp),%eax
	movl	%eax,UCTX_R_EIP(%ecx)	/ %eip
	leal	4(%esp),%eax		/ "pop" the PC
	movl	%eax,UCTX_R_ESP(%ecx)	/ %esp
	movw	%fs,UCTX_R_FS(%ecx)	/ %fs
	movw	%gs,UCTX_R_GS(%ecx)	/ %gs

	/ zero out the t_lwpp field of oldtp; this helps the debugger
	/ realize that the thread is off-LWP.  This is done only for
	/ threads whose contexts are being saved.  We must not do this
	/ for other threads (library threads or exiting user threads);
	/ zeroing idle threads' t_lwpp fields would break preemption.
	movl	$0, t_lwpp(%edx)        /oldtp->t_lwpp = 0 

	/ check whether fp has been done during the current time
	/ slice on this lwp before switching. If so save the fp
	/ state in the ucontext stucture of the current thread.
	/ Set the t_usingfpu flag in the thread structure and set
	/ the floating point valid flag UC_FP in the flags field of
	/ the ucontext structure so setcontext to restore the regiters
	/ on switch in will do the right thing.
	/
	/ the _daref_ instruction (below) is safe ONLY because this function is 
	/ called from _thr_swtch which performs the necessary instructions in 
	/ its prologue section to ensure that %ebx is initialized correctly.
	/
        movl    _daref_(__lwp_priv_datap),%eax
        movl    (%eax), %eax
        cmpl    $0, FPUSED_FLAG(%eax)
	jne	.save_fpu

.clear_flag:
	/ clear inconsistent flag for use of debugger
	movb	$0,t_dbg_switch(%edx)

.resume_nosave:
	movl	4(%esp),%edi		/ %edi = newtp

	/ set inconsistent flag for use of debugger
	movb	$1,t_dbg_switch(%edi)

	/ set pointers from newtp with info about its LWP
	movl	t_idle(%edx),%ecx	/ %ecx = oldtp->t_idle
	movl	%ecx,t_idle(%edi)	/ newtp->t_idle = %ecx
	movl	$TS_ONPROC,t_state(%edi)        / newtp->t_state = TS_ONPROC

	/ set pointers in LWP private area about newtp
	movl	t_lwpp(%edi),%eax       / %eax = newtp->t_lwpp
	movl	t_tls(%edi),%ecx	/ %ecx = newtp->t_tls
	movl	%ecx,lwp_thread(%eax)	/ _lwp_getprivate()->lwp_thread = %ecx

	movl	12(%esp),%ecx		/ save the value of sigflag
	movl	20(%esp),%esi		/ save the value of debug_on

	/
	/ Load the context of newtp.
	/ Save the pointers into callee-saved registers so we can count on
	/ them over the unlock.
	/

	/
	/ We're now off the context of the old thread, initialize the stack
	/ pointer of the new and release the old thread if necessary.
	/
	movl	t_ucontext+UCTX_R_ESP(%edi),%esp

	/
	/ call _thr_resendsig() to
	/ i)  _thr_setrun(oldtp) if it had received a signal during sigoff(),
	/ ii) re-send signal in t_psig that had been posted to this new thread, 
	/      and will be unblocked (t_hold) when it starts running; 
	/ iii) call _sys_sigprocmask() if sigflag is true.
	/ iv)  release the thread lock of oldtp
	/
	pushl	%edi			/ new tp
	pushl   %edx			/ oldtp, maybe NULL
	pushl   %ecx			/ sigflag: a boolean

	/ save the t_usingfpu state of oldtp;  this will be checked later
	/ to see if we need to restore the floating point registers of newtp
        movl    t_usingfpu(%edx), %ebp  / save oldtp->t_usingfpu for later

	call	_fref_(_thr_resendsig)	/ _thr_resendsig(lwpmask, oldtp, newtp)
	addl	$12,%esp
	
	/
	/ Unlock the new thread, as long as our %esp is restored, we're
	/ in business.
	/
	leal	t_lock(%edi),%edx
	pushl	%edx

	/ The call below to _lwp_mutex_unlock also depends on the fact that 
	/ _thr_resume is called by _thr_swtch, whose prologue code ensures
	/ proper setup of position-independent code
	/
	call	_fref_(_lwp_mutex_unlock)	/ _lwp_mutex_unlock(newtp->t_lock);

	/ save values of newtp and debug_on on stack
	movl	%edi,(%esp)		/ newtp
	pushl	%esi			/ debug_on
	/
	/ Restore the new thread's register set.
	/
	leal	t_ucontext(%edi),%ecx
	/

	/
	/ We have to check to see if we need to restore the
	/ fpu since this thread may have previously done fpu.
	/ The ucontext stuctures is used for other things so we only
	/ want to affect the fpu state in the lwp which is why the current
	/ flags value is saved in eax and destructively anded.
	/ Also if the previous thread has done floating point operations
	/ we must invalidate the fpu state to we now know this thread has
	/ not done any fp operations.
	/
        movl    UCTX_FLAGS(%ecx), %eax      / save old state, may have
        andl    $UC_FP, UCTX_FLAGS(%ecx)    / fp valid in new thread
        jnz     .restore_fp_regs            / yes, need setcontext and restore

        cmpl    $0, %ebp                    / last thread used fpu?
        jnz     .restore_fp_regs            / yes, need setcontext to clear 
        movl    %eax, UCTX_FLAGS(%ecx)      / fp state in ucontext structure

.restore_gen_regs:
	/
	/ Restore general register set.
	/
	movl	UCTX_R_EBX(%ecx),%ebx	/ %ebx
	movl	UCTX_R_EBP(%ecx),%ebp	/ %ebp
	movl	UCTX_R_EDI(%ecx),%edi	/ %edi
	movl	UCTX_R_ESI(%ecx),%esi	/ %esi
	movw	UCTX_R_FS(%ecx),%fs	/ %fs
	movw	UCTX_R_GS(%ecx),%gs	/ %gs
	movl	UCTX_R_EAX(%ecx),%eax	/ %eax for benefit of save
	movl	UCTX_R_EIP(%ecx),%ecx	/ %eip

	xchgl	%ecx,4(%esp)		/ %ecx now holds newtp
					/ and 4(%esp) is %eip

	/ clear inconsistent flag for use of debugger
	movb	$0,t_dbg_switch(%ecx)

	popl	%ecx			/ debug_on flag
	testl	%ecx,%ecx
	je	.no_dbg2
				/ call debugger notifier
	pushl	$TC_SWITCH_COMPLETE
	pushl	$0
	call	_thr_debug_notify
	addl	$8,%esp
.no_dbg2:
	popl	%ecx		/ %eip
	jmp	*%ecx

.save_fpu:
	/
	/ Save the FPU.
	/
        movl    $1,t_usingfpu(%edx)     / set oldtp->t_usingfpu to true
        orl     $UC_FP,UCTX_FLAGS(%ecx) / validate fp state in ucontext
	fnsave	UCTX_FPREGS+U_FPCHIP_STATE(%ecx)
	wait
	jmp	.clear_flag

        /
        / Restore floating point register set.
        /
.restore_fp_regs:
        pushl   %eax				/ save uc_flags
        pushl   %ecx				/ save ucontext pointer
        pushl   %ecx
        movl    _daref_(_sys_setcontext), %eax  / restore registers with
        call    *(%eax)                         / setcontext system call
        addl    $4,%esp
        popl    %ecx                            / restore ucontext pointer
        popl    %eax				/ restore flags
        movl    %eax, UCTX_FLAGS(%ecx)
	jmp	.restore_gen_regs

	SIZE(_thr_resume)

