	.ident	"@(#)kern-i386:svc/intr.m4	1.14.3.1"
	.ident	"$Header$"

define(SAVE_DSEGREGS, `
	pushl	%ds;
	pushl	%es')

define(RESTORE_UDSEGREGS, `
	popl	%es;
	popl	%ds')

define(RESTORE_DSEGREGS, `
	LABEL(`restore',`done');
	cmpl	$_A_KDSSEL, (%esp);
	jne	restore;
	addl	$ 8, %esp;
	jmp	done;
restore:;
	popl	%es;
	popl	%ds;
done:;
	popdef(`restore',`done');
')

define(SETUP_KDSEGREGS, `
	movw	$_A_KDSSEL, %ax;
	movw	%ax, %es;
	movw	%ax, %ds')

define(INTR_SAVE_REGS, `
	pushl	%eax;
	pushl	%ecx;
	pushl	%edx')

define(INTR_RESTORE_REGS, `
	popl	%edx;
	popl	%ecx;
	popl	%eax')

define(TRAP_SAVE_REGS, `
	pushal')

define(TRAP_RESTORE_REGS, `
	popal')

define(INTR_TO_TRAP_REGS, `
	pushl	%ebx;
	subl	$ 4, %esp;
	pushl	%ebp;
	pushl	%esi;
	pushl	%edi')

/
/ MACRO
/ USER_IRET
/
/ Description:
/	Return to user mode from a trap or interrupt.
/
/	Interrupts must be disabled ("cli") before USER_IRET is called.
/
/ Remarks:
/	Workaround for Intel386(tm) B1 stepping errata #13:
/	Errata #13 requires any iret which changes privilege levels to
/	be executed with a stack page that has user read permissions
/	(at the page level, but not necessarily at the segment level).
/
define(USER_IRET, `
	iret;
')

/
/ /*VARARGS0*/
/ MACRO
/ INTR_ENTER(intno)
/
/ Description:
/	Enter an interrupt service routine by setting up an interrupt
/	stack frame.  Save segment registers and C-temporary registers.
/	If an intno is specified, load it into %ecx
/
/ Remarks:
/	Can be called with no arguments to simply set up an interrupt
/	frame (in which case no value is loaded into %ecx).
/
/	The interrupt number is passed into %ecx in order to limit
/	the number of times the register has to be saved and restored.
/	PIC programming on the AT requires inb and outb instructions
/	which use %eax and %edx.  Putting the interrupt number into %ecx
/	allows some of the PIC programming to be done without having to
/	reload the interrupt number; if the interrupt were in %eax, then
/	it would have to be reloaded after inb and outb instructions.
/
define(INTR_ENTER, `
	SAVE_DSEGREGS;
	INTR_SAVE_REGS;
	SETUP_KDSEGREGS;
if(`$# != 0',`
	movl	$$1, %ecx
')
')


if(`defined(`DEBUG') || defined(`SPINDEBUG')',`

/
/ MACRO
/ STK_ADJUST(operand, stack-offset)
/
/ Description:
/	Adjusts operands which use %esp-based index addressing by the
/	specified stack offset.  The first argument is an assembly
/	language operand, and the second argument specifies an additional
/	stack offset.
/
/	If the operand uses index addressing with %esp, then STK_ADJUST
/	prints the operand adjusted for the specified stack offset.  If
/	the operand does not use %esp addressing, then STK_ADJUST simply
/	prints the original arg.
/
/ Remarks:
/	For example,
/
/			STK_ADJUST(4(%esp), 12)
/
/	produces
/
/			12+4(%esp)
/
/	while
/
/			STK_ADJUST(%edx, 12)
/
/	simply produces
/
/			%edx
/
/	STK_ADJUST is useful in other macros, when the macro modifies
/	the stack before accessing its arguments.  It is used in
/	BEGIN_INT below for just this purpose.
/
define(`STK_ADJUST',`if(`index($1,`(%esp') < 0', $1, $2+$1)')
')

/
/ MACRO
/ BEGIN_INT(handler_address, old_ip, ipl)
/
/ Description:
/	Signal beginning of interrupt processing.  The arguments
/	are:
/		handler_address		The address of the handler
/					for this interrupt
/
/		old_ip			Address of instruction about to
/					be executed when interrupt occurred
/
/		ipl			Priority level of interrupt
/
/ Remarks:
/	If DEBUG or SPINDEBUG is turned on, this calls begin_intprocess
/	to maintain interrupt stats.  In any case it also increments the
/	interrupt depth counter.
/
define(`BEGIN_INT',`
if(`defined(`DEBUG') || defined(`SPINDEBUG')',`
	pushl	%eax			/ save registers
	pushl	%ecx
	pushl	%edx
	pushl	STK_ADJUST($3, 12)	/ so far, 12 extra bytes on stack
	pushl	STK_ADJUST($2, 16)	/ now 16 bytes on stack
	pushl	STK_ADJUST($1, 20)	/ and now 20
	call	begin_intprocess	/ call routine
	addl	$ 12, %esp		/ go past argument list
	popl	%edx			/ restore registers
	popl	%ecx
	popl	%eax
')
	incl    plocal_intr_depth
')

/
/ MACRO
/ END_INT
/
/ Description:
/	Signal end of interrupt processing.
/
/ Remarks:
/	If DEBUG or SPINDEBUG is turned on, this calls end_intprocess
/	to maintain interrupt stats.  In any case it also decrements the
/	interrupt depth counter.
/
define(`END_INT',`
	decl    plocal_intr_depth
if(`defined(`DEBUG') || defined(`SPINDEBUG')',`
	pushl	%eax			/ save registers
	pushl	%ecx
	pushl	%edx
	call	end_intprocess
	popl	%edx			/restore registers
	popl	%ecx
	popl	%eax
')
')

/
/ MACRO
/ IF_USERMODE(iret_ptr, jmp_label)
/
/ Description:
/	iret_ptr is a pointer to the kernel stack location at which the iret
/	will be done.  The H/W saved state at that location will be examined.
/	If the iret will return to user mode, a jump to jmp_label will be made.
define(`IF_USERMODE',`
	LABEL(`non_v86mode', `kmode')
ifdef(`V86MODE',`
	testl	$ 0x20000, 8 + $1		/ check EFL for V86 flag
	jz	non_v86mode			/ if not, go to non_v86mode
	testw	$_A_SPECF_VM86, l+_A_L_SPECIAL_LWP	/ check Merge or not
	jnz	$2				/ if it is
	jmp 	kmode				/ else v86bios
')
non_v86mode:
	testb	$ 3, 4 + $1		/ check CS for non-zero privilege level
	jnz	$2			/ user mode if non-zero
kmode:
')

/
/ MACRO
/ PL_CHECK
/
/ Description:
/	If DEBUG is defined, PL_CHECK checks that priority level of the
/	interrupt context and the interrupted context meet certain constraints.
/	If DEBUG is not defined, this is a no-op.
/
/ Remarks:
/	Invoked during interrupt return sequence, with old pl on top of stack.
/
define(`PL_CHECK',`
ifdef(`DEBUG',`
	LABEL(`usermode',`less',`done')
/
/ The following should be true regardless of whether interrupted context
/ was kernel or user:
/
	ASSERT(ub,`ipl',!=,`$_A_PLBASE')	/ current pl != PLBASE
	ASSERT(ub,`ipl',<=,`$_A_PLHI')		/ current pl <= PLHI
/
/ See if interrupt context was user or kernel
/
	IF_USERMODE(_A_INTR_SP_IP(%esp), usermode)
/
/ If interrupted context was kernel, then either:
/	(1) (return pl == current pl) && (current pl == PLHI)
/ or
/	(2) return pl < current pl
/
	movl	(%esp), %eax			/ %eax = return pl
	cmpl	$_A_PLHI, %eax			/ if return pl == PLHI &&
	jne	less
	cmpb	ipl, %al			/ return pl == current pl
	je	done				/	=> OK
less:
	ASSERT(ub,%al,<,`ipl')			/ return pl < current pl
	jmp	done
/
/ If interrupted context was user:
/
usermode:
	ASSERT(ul,(%esp),==,`$_A_PLBASE')	/ return pl == PLBASE
done:
	popdef(`usermode',`less',`done')
')
')
