	.ident	"@(#)kern-i386:util/minmax.s	1.2.2.1"

include(KBASE/svc/asm.m4)
include(assym_include)


/
/ int
/ min(uint, uint)
/	Returns the unsigned integer min of two arguments.
/
/ Calling State/Exit State:
/	None.
/
ENTRY(min)
	movl	SPARG0, %eax
	movl	SPARG1, %ecx
	cmpl	%ecx, %eax
	jbe	.minxit			/ NOTE: unsigned comparison/branch
	movl	%ecx, %eax
.minxit:
	ret
	SIZE(min)

/
/ int
/ max(uint, uint)
/	Returns the unsigned integer max of two arguments.
/
/ Calling State/Exit State:
/	None.
/
ENTRY(max)
	movl	SPARG0, %eax
	movl	SPARG1, %ecx
	cmpl	%ecx, %eax
	jae	.maxit			/ NOTE: unsigned comparison/branch
	movl	%ecx, %eax
.maxit:
	ret
	SIZE(max)
