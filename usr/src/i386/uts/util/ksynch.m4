ifdef(`_UTIL_KSYNCH_M4', `',`
define(`_UTIL_KSYNCH_M4',`1')
	.ident	"@(#)kern-i386:util/ksynch.m4	1.15.1.1"
	.ident	"$Header$"

/
/ Include the platform specific macros to support synch primitives
/

include(KBASE/util/ksynch_p.m4)

/
/ i386 assembly-language macros to support synch primitives.
/


/
/ macro for FSPIN_LOCK. The first argument is a register pointing to the lock,
/ and the second argument is a 1-byte register that will be trashed.
/
define(`__FSPIN_LOCK_ASM',`
	LABEL(`loop',`spin',`done');
	movb	$_A_SP_LOCKED, $2;	/ the value to exchange
loop:	__DISABLE_ASM;			/ disable interrupts
	xchgb	$2, ($1);		/ lockp->rws_lock = SP_LOCKED
	cmpb	$_A_SP_UNLOCKED, $2;	/ was previously unlocked?
	je	done;			/ yes, we got the lock
	__ENABLE_ASM;			/ we did not: enable interrupts
spin:	cmpb	$_A_SP_UNLOCKED, ($1);	/ is the lock unlocked yet?
	je	loop;			/ yes, try for it again.
	jmp	spin
done:
	popdef(`loop',`spin',`done');
')

/
/ macro for FSPIN_TRYLOCK.
/ The first argument is a 32-bit register that has the pointer to the
/ fast spin lock.
/ The second argument is a 32-bit register that will have the outcome of
/ of this operation, and the third argument is a spare 8-bit register that
/ will be trashed.
/
define(`__FSPIN_TRYLOCK_ASM',`
	LABEL(`fail',`done');
	movl	$_A_B_TRUE, $2;		/ plan to succeed
	movb	$_A_SP_LOCKED, $3;
	cmpb	$3, ($1);
	je	fail;
	__DISABLE_ASM;
	xchgb	$3, ($1);
	cmpb	$_A_SP_UNLOCKED, $3;
	je	done;
	__ENABLE_ASM;
fail:	movl	$_A_B_FALSE, $2;
done:
	popdef(`fail',`done');
')
/
/ macro for FSPIN_UNLOCK. The argument is a 32-bit register having the
/ pointer to the lock.
/
define(`__FSPIN_UNLOCK_ASM',`
	movb	$_A_SP_UNLOCKED, ($1)	/ unlock the lock
	__ENABLE_ASM;            	/ enable interrupts
')
')
/
/ Macro for unlocking a regular spin lock. The first argument points to the
/ spin lock to be unlocked and the second argument specifies the ipl at
/ which the lock is to be dropped.
/
define(`__UNLOCK_ASM',`
	pushl	$2
ifdef(`UNIPROC',`
	call 	splx
	addl	$ 4,%esp
	decl	prmpt_state
',`
	pushl	$1
ifdef(`DEBUG',`
	call	unlock_dbg
',`
ifdef(`SPINDEBUG',`
	call	unlock_dbg
',`
	call	unlock_nodbg
')
')
	addl	$ 8,%esp
')
')

/
/ MACRO
/ __WRITE_SYNC_ASM(void)
/
/ Description:
/	Flush processor write buffers so that pending memory writes become
/	visible to other processors.  Takes no arguments.
/
/ Remarks:
/	A no-op on uniprocessors.
/
/	On multi-processors, any serializing instruction will do;
/	we use move to debug register.  Note that we save the old
/	value first, and then write it back.
/
define(`__WRITE_SYNC_ASM',`
ifdef(`UNIPROC',`',`
LABEL(`do486',`done')
	cmpl	$_A_CPU_P5, mycpuid
	jl	do486
	xorl	%eax, %eax
	pushl	%ebx
	cpuid
	popl	%ebx
	jmp	done
do486:
	xchgl	%edx, -4(%esp)
done:
popdef(`do486',`done')
')
')
