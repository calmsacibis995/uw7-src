	.ident	"@(#)kern-i386:util/lsub.s	1.3.2.1"
	.file	"util/lsub.s"

include(KBASE/svc/asm.m4)

/
/ dl_t
/ lsub(dl_t, dl_t)
/	This function returns the 64-bit result of subtracting
/	the two 64-bit arguments.
/
/ Calling State/Exit State:
/	This function assumes caller has provided sufficient locking
/	on the arguments prior to the call.
/
/ Remarks:
/	The return value is an eight-byte structure; the C calling
/	convention is that the pointer to the structure is passed
/	both in %eax and as an implied first argument (i.e., SPARG0).
/	This routine uses the pointer in %eax, and ignores the implied
/	argument.
/
ENTRY(lsub)
	movl	SPARG1, %edx		/* left operand low */
	movl	SPARG3, %ecx		/* right operand low */
	subl	%ecx, %edx
	movl	SPARG2, %ecx		/* left operand high */
	movl	%edx, (%eax)		/* return low word */
	movl	SPARG4, %edx		/* right operand high */
	sbbl	%edx, %ecx		/* subtract high with borrow */
	movl	%ecx, 4(%eax)		/* return high word */
	ret	$4			/* clear structure pointer and ret */
	.size	lsub,.-lsub
