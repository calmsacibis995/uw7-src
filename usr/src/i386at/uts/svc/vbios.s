	.ident	"@(#)kern-i386at:svc/vbios.s	1.3.1.1"
	.ident	"$Header$"
	.file	"svc/vbios.s"

include(KBASE/svc/asm.m4)
include(assym_include)

/
/ EGA font pointers (these start as real mode pointers)
/ The pointers point to the 8x8, 8x14, 9x14, 8x16 and 
/ the 9x16 fonts, respectively
/

	.data
	.align	8	
	.globl	egafontptr
	.type	egafontptr,@object
egafontptr:
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.size	egafontptr,.-egafontptr
	
	.globl	egafontsz
	.type	egafontsz,@object
egafontsz:
	.value	0
	.value	0
	.value	0
	.value	0
	.value	0
	.size	egafontsz,.-egafontsz

/ void
/ v86call
/	Cause a task switch to a V86 mode task (user-mode, IOPL=3)
/
/ Calling/Exit State:
/	KV86TSSSEL is the TSS selector for the V86 task. This
/	is called by v86bios(). This need not necessarily be located in
/	pstart.s. For maintenance and debugging reason, this is
/	colocated with the following functions.
/	
/		
ENTRY(v86call)
	movl	%cr0, %eax			/ ljmp sets TS flag
	testl	$_A_CR0_TS, %eax		/ ZF is stored in TSS:eflags
	ljmp	$_A_KV86TSSSEL, $0		/ tss selector
	jnz	.tsbit				/ stored ZF
	clts					/ clear TS if it was not set
.tsbit:	
	ret
SIZE(v86call)

/
/ void
/ v86bios_leave()
/
/ Calling/Exit State:
/	Executed for the V86 task to return to the kernel mode. This
/	is done by V86 mode BIOS calls as soon as it peforms the
/	"iret" instruction.
/				
ENTRY(v86bios_leave)	
	int	$_A_V86INT			/ defined in v86bios.h
SIZE(v86bios_leave)

/
/ int
/ v86bios_task()
/ Calling/Exit State:
/	Return 1 if the current the task is v86bios. Otherwise 0.
/	
			
ENTRY(v86bios_task)
	str	%ax
	cmpw	$_A_KV86TSSSEL, %ax
	je	.v86bios_mode
	movl	$0, %eax
	ret
.v86bios_mode:
	movl	$1, %eax
	ret
	SIZE(v86bios_task)


