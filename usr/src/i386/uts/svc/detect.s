	.ident	"@(#)kern-i386:svc/detect.s	1.6.5.1"
	.ident	"$Header$"

include(KBASE/svc/asm.m4)
include(assym_include)

	/ flags word bits
	.set	EFL_T, 0x100		/ trace enable bit
	.set	EFL_AC, 0x40000		/ alignment check (1->check)
	.set	EFL_ID, 0x200000	/ cpuid opcode (flippable->supported)

	/ CR0 control register bits
	.set	CR0_MP, 0x02		/ math coprocessor present
	.set	CR0_EM, 0x04		/ use math emulation
	.set	CR0_TS, 0x08		/ task switched

/
/ void
/ detect_cpu(void)
/	Detect CPU type.
/
/ Calling/Exit State:
/	Called from identify_cpu() during processor initialization.
/	No locks needed because action is entirely processor-local.
/
/	Fills out fields in the local engine structure which identify
/	the type and stepping of the CPU.
/
/	Output is in l.cpu_vendor, l.cpu_family, l.cpu_model,
/	l.cpu_stepping and l.cpu_features.
/	Assumes these fields are initially zeroed.
/

ENTRY(detect_cpu)

	/ Identify i386 and i486 by the inability to set certain flag bits.

	pushfl				/ push FLAGS value on stack
	movl	(%esp), %ecx		/ save copy of FLAGS

	xorl	$EFL_AC, (%esp)		/ flip AC bit for new FLAGS
	popfl				/ attempt changing FLAGS.AC
	pushfl				/ push resulting FLAGS on stack
	cmpl	(%esp), %ecx		/ succeeded in flipping AC?
	je	.cpu_is_386		/ no, probably an i386

	xorl	$EFL_AC+EFL_ID, (%esp)	/ restore AC bit and flip ID bit
	popfl				/ attempt changing FLAGS.ID
	pushfl				/ push resulting FLAGS on stack
	cmpl	(%esp), %ecx		/ succeeded in flipping ID?
	je	.cpu_is_486		/ no, probably an i486

	/ Since we were able to flip the ID bit, we are running on
	/ a processor that supports the cpuid instruction.

	xchgl	%ecx, (%esp)		/ save original FLAGS on stack

	pushl	%ebx
	pushl	%esi
	pushl	%edi

	movl	$0, %eax		/ parameter for cpuid op
	cpuid				/ get CPU hv & vendor

	andl	$0x7, %eax		/ limit the cpuid-hv to 7
	movl	%eax, cpuidparm		/ save that high parm value
	movl	%ebx, l+_A_L_CPU_VENDOR		/ save the vendor ID string
	movl	%edx, l+_A_L_CPU_VENDOR+4	/	vendor ID continued
	movl	%ecx, l+_A_L_CPU_VENDOR+8	/	vendor ID continued

	/ The following code has not been tested for other than "GenuineIntel"
	/ processors.  We formerly compared the vendor ID against that string
	/ and skipped this code for any other vendor's processor (treating it
	/ as a 486).  There is reason to believe that the behavior of other
	/ chips is close enough to Intel's to be useful in identifying the
	/ processor type, so we're now gathering this information no matter
	/ what the vendor ID.  It is the responsibility of our caller,
	/ identify_cpu(), to massage this raw data if necessary before the
	/ rest of the kernel starts making decisions based on it.
	/ The stepping and feature bits are not expected to be compatible;
	/ there are vendor-specific ways to get such information for the
	/ non-Intel chips but we're not yet doing it.

	movl	$1, %eax		/ parameter for cpuid op
	cpuid				/ get CPU family-model-stepping
					/     and first 96 feature bits

	movl	%edx, l+_A_L_CPU_FEATURES	/ save feature bits
	movl	%ecx, l+_A_L_CPU_FEATURES+4
	movl	%ebx, l+_A_L_CPU_FEATURES+8

	movl	%eax, %ebx		/ extract stepping id
	andl	$0x00F, %ebx		/     from bits [3:0]
	movl	%ebx, l+_A_L_CPU_STEPPING

	movl	%eax, %ebx		/ extract model
	andl	$0x0F0, %ebx		/     from bits [7:4]
	shrl	$4, %ebx
	movl	%ebx, l+_A_L_CPU_MODEL

	movl	%eax, %ebx		/ extract family
	andl	$0xF00, %ebx		/     from bits [11:8]
	shrl	$8, %ebx
	movl	%ebx, l+_A_L_CPU_FAMILY

	movl	$2, %esi		/ initial index for remaining features
	leal	l+_A_L_CPU_FEATURES+12, %edi
.other_features:
	cmpl	%esi, cpuidparm		/ check loop variable against limit
	jb	.cpu_identified
	movl	%esi, %eax		/ set next cpuid parameter
	cpuid				/ get next set of features
	movl	%edx, (%edi)		/ save next dword of feature bits
	movl	%ecx, 4(%edi)		/ save next dword of feature bits
	movl	%ebx, 8(%edi)		/ save next dword of feature bits
	movl	%eax, 12(%edi)		/ save next dword of feature bits
	incl	%esi			/ increment loop variable
	addl	$16, %edi		/ adjust pointer to features
	jmp	.other_features

.cpu_identified:
	popl	%edi
	popl	%esi
	popl	%ebx

	popfl				/ restore original FLAGS
	ret

.cpu_is_486:
	/ The EFL_ID bit could not be toggled, so the processor is assumed to
	/ be an early 486 that does not support the cpuid instruction.  This is
	/ correct for Intel processors, but there are Cyrix chips for which the
	/ behavior is different.  OSR5 has code to detect these Cyrix chips
	/ and enable the cpuid instruction for them, but for now, we simply
	/ mis-identify such a chip as a 486.

	movl	$_A_CPU_486, l+_A_L_CPU_FAMILY
	popfl				/ restore original FLAGS
	ret

.cpu_is_386:
	/ The EFL_AC bit could not be toggled, so the processor must be
	/ a 386 or earlier.  We bother to identify these only so that we
	/ can inform the user that we don't support them.

	movl	$_A_CPU_386, l+_A_L_CPU_FAMILY
	popfl				/ restore original FLAGS
	ret

	.data
	.align	4
	/ local variables
cpuidparm:
	.long	0			/ highest cpuid parameter #

	.text

	SIZE(detect_cpu)


/
/ int
/ detect_fpu(void)
/	Detect FPU type.
/
/ Calling/Exit State:
/	No locks needed because action is entirely processor-local.
/	Called when a processor is initialized.
/
/	Returns the "fp_kind" value identifying the type of FPU.
/

ENTRY(detect_fpu)
/
/ Check for any chip at all by tring to do a reset.  If that succeeds,
/ differentiate via cr0.
/
	subl	$4, %esp		/ reserve space for local variable
	movl	%cr0, %edx		/ save CR0 in %edx
	movl	%edx, %eax
	orl	$CR0_MP, %eax		/ set CR0 bits to indicate FPU h/w
	andl	$~[CR0_EM|CR0_TS], %eax
	movl	%eax, %cr0
	fninit				/ initialize chip, if any
	movw	$0x5A5A, (%esp)
	fnstsw	(%esp)			/ get status
	cmpb	$0, (%esp)		/ status zero? 0 = chip present
	jne	.nomathchip
/
/ See if ones can be correctly written from the control word
/
	fnstcw	(%esp)			/ look at the control word
	movw	(%esp), %ax
	andw	$0x103F, %ax		/ see if selected parts of cw look ok
	cmpw	$0x3F, %ax		/ 0x3F = chip present
	jne	.nomathchip
/
/ At this point we know we have a chip of some sort; 
/ NPX and WAIT instructions are now safe.  Distinguish the type of chip
/ by its behavior w.r.t. infinity.
/
/ Note: must use default control word from fninit.
/
	fsetpm				/ in case it's an 80286
	fld1				/ form infinity
	fldz				/   by dividing 1./0.
	fdivr
	fld	%st(0)			/ form negative infinity
	fchs
	fcompp				/ compare +inf with -inf
	fstsw	(%esp)			/    287 considers +inf == -inf
	movw	(%esp), %ax		/    387 considers +inf != -inf
	sahf				/ get the fcompp status
	movl	$_A_FP_287, %eax	/ we have a 287 chip
	je	.fpu_ret		/    if compare equal
	movl	$_A_FP_387, %eax	/ else, we have a 387 chip
.fpu_ret:
	movl	%edx, %cr0		/ restore original CR0
	addl	$4, %esp		/ "pop" local variable off stack
	ret

/
/ No chip was found
/
.nomathchip:
	movl	$_A_FP_NO, %eax		/ indicate no hardware
	jmp	.fpu_ret

	SIZE(detect_fpu)
