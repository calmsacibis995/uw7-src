/	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386:util/lshiftl.s	1.2.2.1"
        .file   "util/lshiftl.s"

include(KBASE/svc/asm.m4)

/
/ dl_t
/ lshiftl(dl_t, int)
/	This function returns the 64-bit result of shifting
/	the first 64-bit argument left/right by the number
/	of bits given by the positive/negative second argument.
/	This function is valid when the shift count is -64 to +64.
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
ENTRY(lshiftl)
	movl	SPARG3, %ecx		/ get count
	orl	%ecx, %ecx
	jl	.lshiftlr		/ if cnt < 0 then right 
	jg	.lshiftll		/ if cnt > 0 then left 

.lshiftld:
	movl	SPARG2, %edx		/ get arg.high 
	movl	SPARG1, %ecx		/ get arg.low
	movl	%edx, 4(%eax)		/ return ans.high
	movl	%ecx, (%eax)		/ return low word
	ret	$4			/ clear structure pointer and ret
	
.lshiftll:
	movl	SPARG1, %edx		/ get arg.low
	cmpb	$32, %cl
	jge	.lshiftll2 		/ if cnt >= 32 then lshiftll2 

	shldl	%edx, SPARG2		/ implicitly uses %cl (high word)
	shll	%cl, %edx		/ %cl times shift left (low word)
	movl	SPARG2, %ecx		/ get high word
	movl	%edx, (%eax)		/ return low word
	movl	%ecx, 4(%eax)		/ return high word
	ret	$4			/ clear structure pointer and ret

/
/ case cnt >= 32
/
.lshiftll2:
	movl	$0, (%eax)		/ return low word
	shll	%cl, %edx		/ %cl time shift left (high word)
	movl	%edx, 4(%eax)		/ return high word
	ret	$4			/ clear structure pointer and ret

/
/ right shift
/
.lshiftlr:
	movl	SPARG2, %edx		/ get arg.high
	negl	%ecx
	cmpb	$32, %cl
	jge	.lshiftlr2		/ if cnt >= 32 then lshiftlr2

	shrdl	%edx, SPARG1		/ implicitly use %cl (low word)
	shrl	%cl, %edx		/ high word is shifted right
	movl	SPARG1, %ecx		/ get low word
	movl	%edx, 4(%eax)		/ return ans.high
	movl	%ecx, (%eax)		/ return ans.low
	ret	$4			/ clear structure pointer and ret
/
/ case cnt >= 32
/
.lshiftlr2:
	shrl	%cl, %edx		/ arg.high shifted right
	movl	$0, 4(%eax)		/ return ans.high
	movl	%edx, (%eax)		/ return ans.low 
	ret	$4			/ clear structure pointer and ret 
	SIZE(lshiftl)
