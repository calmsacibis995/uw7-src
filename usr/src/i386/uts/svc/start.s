	.ident	"@(#)kern-i386:svc/start.s	1.2.6.1"
	.ident	"$Header$"
	.file	"svc/start.s"

/ void
/ _start(void)
/
/	This code is not really a C callable procedure but instead
/	is code called by the low level startup or online code.
/
/ Calling State/Exit State:
/
/	This function never returns since there is no context to return
/	to, but either initializes the system or just calls swtch()
/	when the system is first booted or a procesor is onlined respectively.
/
/	The first call to this function is with only one processor running.
/
/	Subsequent calls are the result of online of other processors and
/	they are serialized at higher levels of code (ie. the online code),
/	thus we have simplified mutex issues to deal with.  These callers
/	pass the logical engine number (engine[] index) of this engine
/	in %edx.
/
/ Description:
/
/	If we are the first processor, do:
/		1. call sysinit
/		2. call p0init to handcraft the first process;
/		   p0init in turn invokes main to complete initialization.
/
/	If we are not the first processor, do:
/		1. call selfinit
/		2. call swtch.
/

include(KBASE/svc/asm.m4)
include(assym_include)

	.data
	.globl	upyet
	.align	4
upyet:	.long	0			/ upyet: non-zero when system is "up"

intel_cpu_id:
	.string	"GenuineIntel"		/ ID string for Intel CPUs

 	.text

ENTRY(_start)
ifdef(`DEBUG_SBSP',`
        mov     $0xfe0001, %eax
	pushl   %eax
	call    remote_graffiti
	addl    $4, %esp
')
	movl	$ueng, %ebp		/ running mapped so ...
	movl	%ebp, %esp		/ set stack pointer to engine stack

ifdef(`DEBUG_SBSP',`
        mov     $0xfe0002, %eax
	pushl   %eax
	call    remote_graffiti
	addl    $4, %esp
')
	pushl	$0			/ clear all flags
	popfl

	cmpl	$0, upyet
	jne	sysup

ifdef(`CCNUMA',`
	movl	this_cg_num, %eax
	testl	%eax, %eax		/ BOOTCG ?
	jne	.scg_start		/ NO, this is a secnodary CG
')

	/
	/ System not already up.  Call sysinit().
	/
	call	sysinit

	incl	upyet			/ indicate the system is initialized

	jmp	p0init			/ handcraft process 0.
	/ NOTREACHED

sysup:
	/
	/ System already up, so this must be the result
	/ of an online() call.  Just call selfinit and swtch.
	/
	incl	prmpt_state		/ executing on the idle stack
	movl    online_engno, %edx      / get logical engine number for _start
	pushl	%edx			/ %edx contains logical processor #
	call	selfinit		/ call selfinit(procid)
	movl	$0, (%esp)
	call	swtch			/ swtch((lwp_t *)NULL)
	/ NOTREACHED

ifdef(`CCNUMA',`
.scg_start:
	pushl	%eax
	call	scg_sysinit
	add	$4, %esp
	jmp	p0init		/ handcraft process 0 for this CG.
	/ NOTREACHED
')

	SIZE(_start)
/
/ boolean_t
/ pse_supported(void)
/	Indicate whether chip supports page size extension (PSE), i.e.,
/	4MB page.
/
/ Calling/Exit State:
/	Return value is TRUE/FALSE if chip does/does not support pse.
/
/ Remarks:
/	A simplified version of chip_detect routine; assumes only Intel
/	Pentium support pse.
/
	/ flags word bits
	.set	EFL_AC, 0x40000		/ alignment check (1->check)
	.set	EFL_ID, 0x200000	/ cpuid opcode (flippable->supported)
	.set	_A_CPUFEAT_PSE, 0x08

ENTRY(pse_supported)
	/ Identify i386 and i486 by the inability to set certain flag bits

	xorl	%eax, %eax		/ zero out %eax
	pushl	%ebx			/ save %ebx
	pushfl				/ push FLAGS value on stack
	movl	(%esp), %ecx		/ save copy of FLAGS

	xorl	$EFL_AC, (%esp)		/ flip AC bit for new FLAGS
	popfl				/ attempt changing FLAGS.AC
	pushfl				/ push resulting FLAGS on stack
	cmpl	(%esp), %ecx		/ succeeded in flipping AC?
	je	.no4mb			/ no it"s a 386, no 4mb support

	xorl	$EFL_AC+EFL_ID, (%esp)	/ restore AC bit and flip ID bit
	popfl				/ attempt changing FLAGS.ID
	pushfl				/ push resulting FLAGS on stack
	cmpl	(%esp), %ecx		/ succeeded in flipping ID?
	je	.no4mb			/ no, must be an i486, no 4mb support

	/ Since we were able to flip the ID bit, we are running on
	/ a processor which supports the cpuid instruction

	xchgl	%ecx, (%esp)		/ save original FLAGS on stack

	/ %eax is already zero"d out at this point, parameter to cpuid
	cpuid				/ get CPU hv & vendor

	xorl	%eax, %eax		/ zero out %eax
	cmpl	%ebx, intel_cpu_id
	jne	.no4mb
	cmpl	%edx, intel_cpu_id+4
	jne	.no4mb
	cmpl	%ecx, intel_cpu_id+8
	jne	.no4mb

	movl	$1, %eax		/ parameter for cpuid op
	cpuid				/ get CPU family-model-stepping
					/     and first 96 feature bits
	xorl	%eax, %eax		/ zero it out
	testl	$_A_CPUFEAT_PSE, %edx
	je	.no4mb
	incl	%eax			/ set %eax to 1 (TRUE)

.no4mb:
	popfl
	popl	%ebx			/ restore %ebx

	ret
SIZE(pse_supported)

/
/ boolean_t
/ pae_supported(void)
/	Indicate whether chip supports page address extension (PAE), i.e.,
/	36-bit physicall address.
/
/ Calling/Exit State:
/	Return value is TRUE/FALSE if chip does/does not support pae.
/
/ Remarks:
/	A simplified version of chip_detect routine; assumes only Intel
/	Pentium support pae.
/
	/ flags word bits
	.set	_A_CPUFEAT_PAE, 0x40

ENTRY(pae_supported)
	/ Identify i386 and i486 by the inability to set certain flag bits

	xorl	%eax, %eax		/ zero out %eax

	pushl	%ebx			/ save %ebx
	pushfl				/ push FLAGS value on stack
	movl	(%esp), %ecx		/ save copy of FLAGS

	xorl	$EFL_AC, (%esp)		/ flip AC bit for new FLAGS
	popfl				/ attempt changing FLAGS.AC
	pushfl				/ push resulting FLAGS on stack
	cmpl	(%esp), %ecx		/ succeeded in flipping AC?
	je	.nopae			/ no it"s a 386, no 4mb support

	xorl	$EFL_AC+EFL_ID, (%esp)	/ restore AC bit and flip ID bit
	popfl				/ attempt changing FLAGS.ID
	pushfl				/ push resulting FLAGS on stack
	cmpl	(%esp), %ecx		/ succeeded in flipping ID?
	je	.nopae			/ no, must be an i486, no 4mb support

	/ Since we were able to flip the ID bit, we are running on
	/ a processor which supports the cpuid instruction

	xchgl	%ecx, (%esp)		/ save original FLAGS on stack

	/ %eax is already zero"d out at this point, parameter to cpuid
	cpuid				/ get CPU hv & vendor

	xorl	%eax, %eax		/ zero out %eax
	cmpl	%ebx, intel_cpu_id
	jne	.nopae
	cmpl	%edx, intel_cpu_id+4
	jne	.nopae
	cmpl	%ecx, intel_cpu_id+8
	jne	.nopae

	movl	$1, %eax		/ parameter for cpuid op
	cpuid				/ get CPU family-model-stepping
					/     and first 96 feature bits
	xorl	%eax, %eax		/ zero it out
	testl	$_A_CPUFEAT_PAE, %edx
	je	.nopae
	incl	%eax			/ set %eax to 1 (TRUE)

.nopae:
	popfl
	popl	%ebx			/ restore %ebx
	ret
SIZE(pae_supported)

