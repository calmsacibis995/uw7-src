	.ident	"@(#)kern-i386:util/misc.s	1.2.2.1"
	.ident	"$Header$"
	.file	"util/misc.s"

/
/ Miscellaneous low level routines to access CPU registers, etc.
/ Family-specific.
/

include(KBASE/svc/asm.m4)
include(assym_include)

/
/ int
/ _cr0(void)
/
/	Return the contents of machine register %cr0.
/

ENTRY(_cr0)
	movl	%cr0, %eax
	ret

	SIZE(_cr0)

/
/ int
/ _cr2(void)
/
/	Return the contents of machine register %cr2.
/

ENTRY(_cr2)
	movl	%cr2, %eax
	ret

	SIZE(_cr2)

/
/ int
/ _cr3(void)
/
/	Return the contents of machine register %cr3.
/

ENTRY(_cr3)
	movl	%cr3, %eax
	ret

	SIZE(_cr3)

/
/ void
/ _wdr0(ulong)
/
/	Write argument into machine debug register %db0.
/

ENTRY(_wdr0)
	movl	4(%esp), %eax
	movl	%eax, %db0
	ret

	SIZE(_wdr0)

/
/ void
/ _wdr1(ulong)
/
/	Write argument into machine debug register %db1.
/

ENTRY(_wdr1)
	movl    4(%esp), %eax
	movl    %eax, %db1
	ret

	SIZE(_wdr1)

/
/ void
/ _wdr2(ulong)
/
/	Write argument into machine debug register %db2.
/

ENTRY(_wdr2)
	movl    4(%esp), %eax
	movl    %eax, %db2
	ret

	SIZE(_wdr2)

/
/ void
/ _wdr3(ulong)
/
/	Write argument into machine debug register %db3.
/

ENTRY(_wdr3)
	movl    4(%esp), %eax
	movl    %eax, %db3
	ret

	SIZE(_wdr3)

/
/ void
/ _wdr6(ulong)
/
/	Write argument into machine debug register %db6.
/

ENTRY(_wdr6)
	movl    4(%esp), %eax
	movl    %eax, %db6
	ret

	SIZE(_wdr6)

/
/ void
/ _wdr7(ulong)
/
/	Write argument into machine debug register %db7.
/

ENTRY(_wdr7)
	movl    4(%esp), %eax
	movl    %eax, %db7
	ret

	SIZE(_wdr7)

/
/ int
/ _dr0(void)
/
/	Return the contents of machine register %db0.
/

ENTRY(_dr0)
	movl    %db0, %eax
	ret

	SIZE(_dr0)

/
/ int
/ _dr1(void)
/
/	Return the contents of machine register %db1.
/

ENTRY(_dr1)
	movl    %db1, %eax
	ret

	SIZE(_dr1)

/
/ int
/ _dr2(void)
/
/	Return the contents of machine register %db2.
/

ENTRY(_dr2)
	movl    %db2, %eax
	ret

	SIZE(_dr2)

/
/ int
/ _dr3(void)
/
/	Return the contents of machine register %db3.
/

ENTRY(_dr3)
	movl    %db3, %eax
	ret

	SIZE(_dr3)

/
/ int
/ _dr6(void)
/
/	Return the contents of machine register %db6.
/

ENTRY(_dr6)
	movl    %db6, %eax
	ret

	SIZE(_dr6)

/
/ int
/ _dr7(void)
/
/	Return the contents of machine register %db7.
/

ENTRY(_dr7)
	movl    %db7, %eax
	ret

	SIZE(_dr7)

/
/ ushort
/ get_tr(void)
/
/	Returns the contents of the machine task register.
/

ENTRY(get_tr)
	xorl	%eax, %eax
	str	%ax
	ret

	SIZE(get_tr)

/
/ void
/ loadtr(ushort)
/
/	Write the machine task register.
/

ENTRY(loadtr)
	movw	4(%esp), %ax
	ltr	%ax
	ret

	SIZE(loadtr)

/
/ void
/ loadldt(ushort)
/
/	Write the local descriptor table register.
/

ENTRY(loadldt)
	movw	4(%esp), %ax
	lldt	%ax
	ret

	SIZE(loadldt)

/
/ void
/ _rdmsr(uint_t regnum, ulong_t val[2])
/
/	Read model-specific register
/
ENTRY(_rdmsr)
	movl	4(%esp), %ecx		/ which register to access
	rdmsr
	movl    8(%esp), %ecx
	movl	%eax, (%ecx)
	movl	%edx, 4(%ecx)
	ret

	SIZE(_rdmsr)

/
/ void
/ _wrmsr(uint_t regnum, ulong_t val[2])
/
/	Write model-specific register
/
ENTRY(_wrmsr)
	movl    8(%esp), %ecx
	movl	(%ecx), %eax
	movl	4(%ecx), %edx
	movl	4(%esp), %ecx		/ which register to access
	wrmsr
	ret

	SIZE(_wrmsr)

/
/ void
/ _rdtsc(ulong_t timestamp[2])
/
/	Read timestamp counter
/
ENTRY(_rdtsc)
	movl	4(%esp), %ecx
	rdtsc
	movl	%eax, (%ecx)
	movl	%edx, 4(%ecx)
	ret

	SIZE(_rdtsc)

/ int
/ upc_scale(int offset, unsigned long scale)
/	Returns the bucket number corresponding to offset for the 
/	specified scaling factor.
/
/ Calling/Exit State:
/	None.
/

ENTRY(upc_scale)
	movl	4(%esp), %eax	/ Get the offset.
	mull	8(%esp)		/ Multiply by the scaling factor
	shrdl	$17, %edx, %eax	/ Divide by 2**17 to scale down
				/ Note that there are 2 bytes per slot
				/ and scale has an implicit decimal point
				/ on the left.
	ret

	SIZE(upc_scale)
